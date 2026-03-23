#include "RspCodeBlock.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>

RspCodeBlock::RspCodeBlock(CRSPSystem & System, uint32_t StartAddress, RspCodeType type, uint32_t DispatchAddress, RspCodeBlocks & Functions) :
    m_Functions(Functions),
    m_DispatchAddress(DispatchAddress),
    m_System(System),
    m_StartAddress(StartAddress),
    m_CodeType(type),
    m_CompiledLoction(nullptr),
    m_Valid(true)
{
    Analyze();
#if defined(__amd64__) || defined(_M_X64)
    ReOrderInstructions();
#endif
    BuildInstructionIndex();
}

const RspCodeBlock::Addresses & RspCodeBlock::GetBranchTargets() const
{
    return m_BranchTargets;
}

void * RspCodeBlock::GetCompiledLocation() const
{
    return m_CompiledLoction;
}

uint32_t RspCodeBlock::GetDispatchAddress() const
{
    return m_DispatchAddress;
}

const RspCodeBlock::Addresses & RspCodeBlock::GetFunctionCalls() const
{
    return m_FunctionCalls;
}

const RSPInstructions & RspCodeBlock::GetInstructions() const
{
    return m_Instructions;
}

const RspCodeBlock * RspCodeBlock::GetFunctionBlock(uint32_t Address) const
{
    RspCodeBlocks::const_iterator itr = m_Functions.find(Address);
    if (itr != m_Functions.end())
    {
        return itr->second.get();
    }
    return nullptr;
}

uint32_t RspCodeBlock::GetStartAddress() const
{
    return m_StartAddress;
}

size_t RspCodeBlock::InstructionIndex(uint32_t pc) const
{
    InstructionIndexMap::const_iterator it = m_InstructionIndex.find(pc);
    return (it != m_InstructionIndex.end()) ? it->second : m_InstructionIndex.size();
}

void RspCodeBlock::SetCompiledLocation(void * CompiledLoction)
{
    m_CompiledLoction = CompiledLoction;
}

RspCodeType RspCodeBlock::CodeType() const
{
    return m_CodeType;
}

bool RspCodeBlock::IsEnd(uint32_t Address) const
{
    return m_End.find(Address) != m_End.end();
}

bool RspCodeBlock::IsValid() const
{
    return m_Valid;
}

bool RspCodeBlock::IsAddressInInstructions(uint32_t address) const
{
    for (RSPInstructions::const_iterator itr = m_Instructions.begin(); itr != m_Instructions.end(); itr++)
    {
        if (itr->Address() == address)
        {
            return true;
        }
    }
    return false;
}

void RspCodeBlock::Analyze(void)
{
    uint32_t Address = m_StartAddress;
    uint8_t * IMEM = m_System.m_IMEM;

    bool FoundEnd = false;
    for (;;)
    {
        RSPInstruction Instruction(Address, *(uint32_t *)(IMEM + (Address & 0xFFF)));
        if (Instruction.IsJump())
        {
            uint32_t target = Instruction.JumpTarget();
            if (target == m_DispatchAddress)
            {
                if (m_CodeType != RspCodeType_TASK)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                if (std::find(m_End.begin(), m_End.end(), Address) == m_End.end())
                {
                    m_End.insert(Address);
                }

                m_Instructions.push_back(Instruction);
                RSPInstruction DelayInstruction(((Address + 4) & 0x1FFF), *(uint32_t *)(IMEM + ((Address + 4) & 0xFFF)));
                m_Instructions.push_back(DelayInstruction);

                uint32_t nextAddress = 0x2000;
                bool JumpBeyond = false;
                for (Addresses::const_iterator itr = m_BranchTargets.begin(); itr != m_BranchTargets.end(); itr++)
                {
                    uint32_t branchTarget = *itr;
                    if (branchTarget <= Address || IsAddressInInstructions(branchTarget))
                    {
                        continue;
                    }
                    if (*itr < nextAddress)
                    {
                        nextAddress = *itr;
                    }
                    JumpBeyond = true;
                }
                if (!JumpBeyond)
                {
                    break;
                }
                Address = nextAddress;
                continue;
            }
            else if (IsAddressInInstructions(target))
            {
                if (std::find(m_BranchTargets.begin(), m_BranchTargets.end(), target) == m_BranchTargets.end())
                {
                    m_BranchTargets.insert(target);
                }
            }
            else
            {
                m_Instructions.push_back(Instruction);
                RSPInstruction DelayInstruction(((Address + 4) & 0x1FFF), *(uint32_t *)(IMEM + ((Address + 4) & 0xFFF)));
                m_Instructions.push_back(DelayInstruction);

                uint32_t nextAddress = target;
                bool foundEarlierTarget = false;
                for (Addresses::const_iterator itr = m_BranchTargets.begin(); itr != m_BranchTargets.end(); itr++)
                {
                    uint32_t branchTarget = *itr;
                    if (branchTarget > Address && branchTarget < target)
                    {
                        if (!foundEarlierTarget || branchTarget < nextAddress)
                        {
                            nextAddress = branchTarget;
                            foundEarlierTarget = true;
                        }
                    }
                }

                if (foundEarlierTarget)
                {
                    if (std::find(m_BranchTargets.begin(), m_BranchTargets.end(), target) == m_BranchTargets.end())
                    {
                        m_BranchTargets.insert(target);
                    }
                }
                Address = nextAddress;
                continue;
            }
        }
        else if (Instruction.IsConditionalBranch())
        {
            uint32_t target = Instruction.ConditionalBranchTarget();
            if (target != m_DispatchAddress)
            {
                if (std::find(m_BranchTargets.begin(), m_BranchTargets.end(), target) == m_BranchTargets.end())
                {
                    m_BranchTargets.insert(target);
                }
            }
        }
        else if (Instruction.IsStaticCall())
        {
            uint32_t target = Instruction.StaticCallTarget();
            if (target == m_DispatchAddress)
            {
                if (m_CodeType != RspCodeType_TASK)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                if (std::find(m_End.begin(), m_End.end(), Address) == m_End.end())
                {
                    m_End.insert(Address);
                }
            }
            else if (std::find(m_FunctionCalls.begin(), m_FunctionCalls.end(), target) == m_FunctionCalls.end())
            {
                m_FunctionCalls.insert(target);
            }
        }

        m_Instructions.push_back(Instruction);
        if (FoundEnd)
        {
            break;
        }
        if ((m_CodeType == RspCodeType_SUBROUTINE && Instruction.IsJumpReturn()) ||
            (m_CodeType == RspCodeType_TASK && Instruction.IsJump() && Instruction.JumpTarget() == m_DispatchAddress))
        {
            bool JumpBeyond = false;
            for (Addresses::iterator itr = m_BranchTargets.begin(); itr != m_BranchTargets.end(); itr++)
            {
                if (*itr > Address)
                {
                    JumpBeyond = true;
                    break;
                }
            }
            FoundEnd = !JumpBeyond;
        }
        Address += 4;
        if (Address == 0x2000)
        {
            m_Valid = false;
            return;
        }
    }
    for (Addresses::iterator itr = m_BranchTargets.begin(); itr != m_BranchTargets.end();)
    {
        if (*itr < m_StartAddress || *itr > Address)
        {
            if (m_CodeType == RspCodeType_SUBROUTINE)
            {
                uint32_t target = *itr;
                if (std::find(m_FunctionCalls.begin(), m_FunctionCalls.end(), target) == m_FunctionCalls.end())
                {
                    m_FunctionCalls.insert(target);
                }
                itr = m_BranchTargets.erase(itr);
            }
            else
            {
                m_Valid = false;
                return;
            }
        }
        else
        {
            itr++;
        }
    }
    for (Addresses::iterator itr = m_FunctionCalls.begin(); itr != m_FunctionCalls.end(); itr++)
    {
        if (m_Functions.find(*itr) == m_Functions.end())
        {
            RspCodeBlockPtr FunctionCall = std::make_unique<RspCodeBlock>(m_System, *itr, RspCodeType_SUBROUTINE, m_DispatchAddress, m_Functions);
            if (!FunctionCall->IsValid())
            {
                m_Valid = false;
                return;
            }
            m_Functions[*itr] = std::move(FunctionCall);
        }
    }
}

#if defined(__amd64__) || defined(_M_X64)
void RspCodeBlock::ReOrderInstructions(void)
{
    if (LogAsmCode)
    {
        CRSPRecompiler & Recompiler = m_System.m_Recompiler;

        Recompiler.Log("=== Before reorder (0x%X) ===", m_StartAddress);
        for (size_t i = 0; i < m_Instructions.size(); i++)
        {
            Recompiler.Log("  %X: %s", m_Instructions[i].Address(), m_Instructions[i].NameAndParam().c_str());
        }
    }

    bool changed = true;
    size_t iterations = 0;
    while (changed)
    {
        changed = false;
        iterations++;

        if (iterations > 1000)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            break;
        }

        for (size_t i = 0; i + 2 < m_Instructions.size(); i++)
        {
            RSPInstruction & op0 = m_Instructions[i];
            RSPInstruction & op1 = m_Instructions[i + 1];
            RSPInstruction & op2 = m_Instructions[i + 2];

            if (ShouldSwapInstructions(op0, op1, op2))
            {
                std::swap(m_Instructions[i + 1], m_Instructions[i + 2]);
                changed = true;
            }
        }
    }

    if (LogAsmCode)
    {
        CRSPRecompiler & Recompiler = m_System.m_Recompiler;

        Recompiler.Log("=== After reorder (%d iterations) ===", iterations);
        for (size_t i = 0; i < m_Instructions.size(); i++)
        {
            Recompiler.Log("  %X: %s", m_Instructions[i].Address(), m_Instructions[i].NameAndParam().c_str());
        }
    }
}

bool RspCodeBlock::ShouldSwapInstructions(const RSPInstruction & op0, const RSPInstruction & op1, const RSPInstruction & op2)
{
    // Don't move instructions across branch targets
    if (m_BranchTargets.find(op1.Address()) != m_BranchTargets.end() ||
        m_BranchTargets.find(op2.Address()) != m_BranchTargets.end())
    {
        return false;
    }

    // Don't reorder around control flow
    if (op0.ChangesControlFlow() || op1.ChangesControlFlow() || op2.ChangesControlFlow())
    {
        return false;
    }

    // Determine if op2 should come before op1
    bool shouldSwap = false;

    if (op2.ReadsMemory() && !op1.ReadsMemory())
    {
        // Pull loads ahead of non-loads
        shouldSwap = true;
    }
    else if (op1.ReadsMemory() && op2.ReadsMemory())
    {
        // Both loads — sort by (op, base, offset)
        if (op1.Op() != op2.Op())
        {
            shouldSwap = (op2.Op() < op1.Op());
        }
        else if (op1.Base() != op2.Base())
        {
            shouldSwap = (op2.Base() < op1.Base());
        }
        else
        {
            shouldSwap = (op2.Offset() < op1.Offset());
        }
    }

    if (!shouldSwap)
    {
        return false;
    }

    // Dependency checks: after swap, execution order is op0, op2, op1
    // op1 writes a register — does op2 read it?
    if (op1.WritesGpr())
    {
        if (op2.ReadsGpr(op1.WriteGprReg()))
        {
            return false;
        }
    }

    // op2 writes a register — does op1 read it?
    if (op2.WritesGpr())
    {
        if (op1.ReadsGpr(op2.WriteGprReg()))
        {
            return false;
        }
    }

    // Both write to the same register — order of writes matters
    if (op1.WritesGpr() && op2.WritesGpr())
    {
        if (op1.WriteGprReg() == op2.WriteGprReg())
        {
            return false;
        }
    }

    // Dependency checks: after swap, execution order is op0, op2, op1
    // op1 writes a register — does op2 read it?
    if (op1.WritesVector() && op2.ReadsVector(op1.WriteVectorReg()))
    {
        return false;
    }

    // op2 writes a register — does op1 read it?
    if (op2.WritesVector() && op1.ReadsVector(op2.WriteVectorReg()))
    {
        return false;
    }

    // Both write to the same register — order of writes matters
    if (op1.WritesVector() && op2.WritesVector() && op1.WriteVectorReg() == op2.WriteVectorReg())
    {
        return false;
    }

    // op1 writes to memory — don't move a load past a store
    if (op1.WritesMemory() && op2.ReadsMemory())
    {
        return false;
    }

    // Don't reorder loads and stores
    if (op1.WritesMemory() && op2.ReadsMemory())
    {
        return false;
    }
    if (op1.ReadsMemory() && op2.WritesMemory())
    {
        return false;
    }
    return true;
}
#endif

void RspCodeBlock::BuildInstructionIndex()
{
    for (size_t i = 0, n = m_Instructions.size(); i < n; i++)
    {
        m_InstructionIndex[m_Instructions[i].Address()] = i;
    }
}
