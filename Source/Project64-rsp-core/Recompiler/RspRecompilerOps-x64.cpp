#if defined(__amd64__) || defined(_M_X64)

#include "RspRecompilerOps-x64.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/Recompiler/RspAssembler.h>
#include <Project64-rsp-core/Recompiler/RspCodeBlock.h>
#include <Project64-rsp-core/Recompiler/RspProfiling.h>
#include <Project64-rsp-core/cpu/RSPInstruction-x64.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>
#include <algorithm>

extern p_Recompfunc RSP_Recomp_RegImm[32];
extern p_Recompfunc RSP_Recomp_Special[64];
extern p_Recompfunc RSP_Recomp_Cop0[32];
extern p_Recompfunc RSP_Recomp_Cop2[32];
extern p_Recompfunc RSP_Recomp_Vector[64];
extern p_Recompfunc RSP_Recomp_Lc2[32];
extern p_Recompfunc RSP_Recomp_Sc2[32];

uint32_t BranchCompare = 0;

CRSPRecompilerOps::CRSPRecompilerOps(CRSPSystem & System, CRSPRecompiler & Recompiler) :
    m_System(System),
    m_Recompiler(Recompiler),
    m_OpCode(Recompiler.m_OpCode),
    m_CompilePC(Recompiler.m_CompilePC),
    m_CurrentBlock(Recompiler.m_CurrentBlock),
    m_NextInstruction(Recompiler.m_NextInstruction),
    m_DMEM(System.m_DMEM),
    m_Reg(System.m_Reg),
    m_GPR(System.m_Reg.m_GPR),
    m_Vect(System.m_Reg.m_Vect),
    m_ACCUM(System.m_Reg.m_ACCUM),
    m_VCOL(System.m_Reg.m_VCOL),
    m_VCOH(System.m_Reg.m_VCOH),
    m_VCCL(System.m_Reg.m_VCCL),
    m_VCCH(System.m_Reg.m_VCCH),
    m_VCE(System.m_Reg.m_VCE),
    m_Assembler(Recompiler.m_Assembler),
    m_DelayAffectBranch(false),
    m_RegState(Recompiler.m_RegState)
{
}

void CRSPRecompilerOps::Cheat_r4300iOpcode(RSPOp::Func FunctAddress, const char * FunctName, bool CommentOp)
{
    if (CommentOp)
    {
        m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    }
    if (SyncCPU)
    {
        m_Assembler->MoveConstToVariable(m_System.m_SP_PC_REG, "RSP PC", m_CompilePC);
    }
    m_RegState.WriteBackRegisters();
    m_Assembler->MoveConstToVariable(&m_System.m_OpCode.Value, "m_OpCode.Value", m_OpCode.Value);
    m_Assembler->CallThis(&RSPSystem.m_Op, AddressOf(FunctAddress), FunctName);
}

// Opcode functions

void CRSPRecompilerOps::SPECIAL(void)
{
    (this->*RSP_Recomp_Special[m_OpCode.funct])();
}

void CRSPRecompilerOps::REGIMM(void)
{
    (this->*RSP_Recomp_RegImm[m_OpCode.rt])();
}

void CRSPRecompilerOps::J(void)
{
    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (m_OpCode.target << 2) & 0x1FFC;
        asmjit::Label Jump;
        if (m_CurrentBlock->IsEnd(m_CompilePC) && m_CurrentBlock->CodeType() == RspCodeType_TASK)
        {
            m_Assembler->MoveConstToVariable(m_System.m_SP_PC_REG, "RSP PC", Target);
            ExitCodeBlock();
        }
        else if (m_Recompiler.FindBranchJump(Target, Jump))
        {
            m_RegState.WriteBackRegisters();
            m_Assembler->JmpLabel(stdstr_f("0x%X", Target).c_str(), Jump);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompilerWarning(stdstr_f("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", m_NextInstruction).c_str());
        BreakPoint();
#endif
    }
}

void CRSPRecompilerOps::JAL(void)
{
    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
        m_Assembler->MoveConstToVariable(&m_GPR[31].UW, "RA.W", (m_CompilePC + 8) & 0x1FFC);
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE || m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE_BRANCH_TARGET)
    {
        uint32_t Target = (m_OpCode.target << 2) & 0x1FFC;
        if (m_CurrentBlock->IsEnd(m_CompilePC) && m_CurrentBlock->CodeType() == RspCodeType_TASK)
        {
            m_Assembler->MoveConstToVariable(m_System.m_SP_PC_REG, "RSP PC", Target);
            ExitCodeBlock();
        }
        else
        {
            const RspCodeBlock * FunctionBlock = m_CurrentBlock ? m_CurrentBlock->GetFunctionBlock(Target) : nullptr;
            if (FunctionBlock != nullptr)
            {
                m_RegState.WriteBackRegisters();
                if (SyncCPU)
                {
                    m_Assembler->MoveConstToVariable(m_System.m_SP_PC_REG, "RSP PC", Target);
                    m_Assembler->mov(asmjit::x86::rdx, asmjit::imm(0x2000));
                    m_Assembler->mov(asmjit::x86::r8, asmjit::imm(Target & 0xFFF));
                    m_Assembler->CallThis(RSPSystem.SyncSystem(), AddressOf(&CRSPSystem::ExecuteOps), "CRSPSystem::ExecuteOps");
                    m_Assembler->CallThis(&RSPSystem, AddressOf(&CRSPSystem::BasicSyncCheck), "CRSPSystem::BasicSyncCheck");
                }
                m_Assembler->CallFunc(FunctionBlock->GetCompiledLocation(), stdstr_f("0x%X", Target).c_str());
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }

        if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE_BRANCH_TARGET)
        {
            asmjit::Label Jump = m_Assembler->newLabel();
            m_Assembler->JmpLabel(stdstr_f("0x%X_continue", m_CompilePC).c_str(), Jump);
            m_Recompiler.CompileOpcode((m_CompilePC + 4) & 0x1FFC);
            m_Assembler->bind(Jump);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        //CompilerWarning(stdstr_f("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", m_NextInstruction).c_str());
        //BreakPoint();
    }
}

void CRSPRecompilerOps::BEQ(void)
{
    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        RSPInstruction Instruction(m_CompilePC, m_OpCode.Value);
        m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, Instruction.NameAndParam().c_str()).c_str());
        m_DelayAffectBranch = Instruction.DelaySlotAffectBranch();
        if (!m_DelayAffectBranch)
        {
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        if (m_OpCode.rt == 0)
        {
            m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
        }
        else if (m_OpCode.rs == 0)
        {
            m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), 0);
        }
        else
        {
            m_Assembler->MoveVariableToX86reg(asmjit::x86::r11, &m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt));
            m_Assembler->CompX86regToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), asmjit::x86::r11);
        }
        m_Assembler->SetzVariable(&BranchCompare, "BranchCompare");
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0x1FFC;

        if (m_OpCode.rs == 0 && m_OpCode.rt == 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            JmpLabel32("BranchToJump", 0);
            m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
            m_NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
#endif
            return;
        }

        if (m_OpCode.rs == 0 && m_OpCode.rt == 0)
        {
            return;
        }

        if (!m_DelayAffectBranch)
        {
            if (m_OpCode.rt == 0)
            {
                m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
            }
            else if (m_OpCode.rs == 0)
            {
                m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), 0);
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
                m_Assembler->MoveVariableToX86reg(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), x86_EAX);
                m_Assembler->CompX86regToVariable(x86_EAX, &m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs));
#endif
            }
            if (Target == m_CurrentBlock->GetDispatchAddress())
            {
                asmjit::Label ContinueLabel = m_Assembler->newLabel();
                m_Assembler->JneLabel(stdstr_f("Continue-%X", m_CompilePC).c_str(), ContinueLabel);
                m_Assembler->MoveConstToVariable(m_System.m_SP_PC_REG, "RSP PC", Target);
                ExitCodeBlock();
                m_Assembler->bind(ContinueLabel);
            }
            else
            {
                asmjit::Label Jump;
                if (!m_Recompiler.FindBranchJump(Target, Jump))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                if (m_RegState.HasMappedRegisters())
                {
                    asmjit::Label JumpCave = m_Assembler->newLabel();
                    m_Assembler->JeLabel(stdstr_f("0x%X_From_0x%X", Target, m_CompilePC).c_str(), JumpCave);
                    m_Assembler->SetSecondarySection();
                    m_Assembler->bind(JumpCave);
                    CRspRegState CaveState = m_RegState;
                    CaveState.WriteBackRegisters();
                    m_Assembler->JmpLabel(stdstr_f("0x%X", Target).c_str(), Jump);
                    m_Assembler->SetPrimarySection();
                }
                else
                {
                    m_Assembler->JeLabel(stdstr_f("0x%X", Target).c_str(), Jump);
                }
            }
        }
        else
        {
            asmjit::Label Jump;
            if (!m_Recompiler.FindBranchJump(Target, Jump))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegState.WriteBackRegisters();
            m_Assembler->CompConstToVariable(&BranchCompare, "BranchCompare", true);
            m_Assembler->JeLabel(stdstr_f("0x%X", Target).c_str(), Jump);
        }
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, m_CompilePC + 8);
#endif
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompilerWarning(stdstr_f("BNE error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", m_NextInstruction).c_str());
        BreakPoint();
#endif
    }
}

void CRSPRecompilerOps::BNE(void)
{
    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        RSPInstruction Instruction(m_CompilePC, m_OpCode.Value);
        m_Recompiler.Log("  %X %s", m_CompilePC, Instruction.NameAndParam().c_str());
        m_DelayAffectBranch = Instruction.DelaySlotAffectBranch();
        if (!m_DelayAffectBranch)
        {
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        if (m_OpCode.rs == 0 && m_OpCode.rt == 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            MoveConstByteToVariable(0, &BranchCompare, "BranchCompare");
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
#endif
            return;
        }

        if (m_OpCode.rt == 0)
        {
            m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
        }
        else if (m_OpCode.rs == 0)
        {
            m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), 0);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            MoveVariableToX86reg(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), x86_EAX);
            CompX86regToVariable(x86_EAX, &m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs));
#endif
        }
        m_Assembler->SetnzVariable(&BranchCompare, "BranchCompare");
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0x1FFC;

        if (m_OpCode.rs == 0 && m_OpCode.rt == 0)
        {
            return;
        }

        if (!m_DelayAffectBranch)
        {
            if (m_OpCode.rt == 0)
            {
                m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
            }
            else if (m_OpCode.rs == 0)
            {
                m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), 0);
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
                MoveVariableToX86reg(&m_GPR[m_OpCode.rt].W, GPR_Name(m_OpCode.rt), x86_EAX);
                CompX86regToVariable(x86_EAX, &m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs));
#endif
            }
            asmjit::Label Jump;
            if (m_Recompiler.FindBranchJump(Target, Jump))
            {
                if (m_RegState.HasMappedRegisters())
                {
                    asmjit::Label JumpCave = m_Assembler->newLabel();
                    m_Assembler->JneLabel(stdstr_f("0x%X_From_0x%X", Target, m_CompilePC).c_str(), JumpCave);
                    m_Assembler->SetSecondarySection();
                    m_Assembler->bind(JumpCave);
                    CRspRegState CaveState = m_RegState;
                    CaveState.WriteBackRegisters();
                    m_Assembler->JmpLabel(stdstr_f("0x%X", Target).c_str(), Jump);
                    m_Assembler->SetPrimarySection();
                }
                else
                {
                    m_Assembler->JneLabel(stdstr_f("0x%X", Target).c_str(), Jump);
                }
            }
            else
            {
                const RspCodeBlock * FunctionBlock = m_CurrentBlock ? m_CurrentBlock->GetFunctionBlock(Target) : nullptr;
                if (FunctionBlock != nullptr)
                {
                    asmjit::Label ContinuJump = m_Assembler->newLabel();
                    m_Assembler->JeLabel(stdstr_f("continue_0x%X", m_CompilePC).c_str(), ContinuJump);
                    m_Assembler->add(asmjit::x86::rsp, FunctionStackSize);
                    m_Assembler->JFunc(FunctionBlock->GetCompiledLocation(), stdstr_f("0x%X", Target).c_str());
                    m_Assembler->bind(ContinuJump);
                }
                else
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        else
        {
            asmjit::Label Jump;
            if (!m_Recompiler.FindBranchJump(Target, Jump))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegState.WriteBackRegisters();
            m_Assembler->CompConstToVariable(&BranchCompare, "BranchCompare", true);
            m_Assembler->JeLabel(stdstr_f("0x%X", Target).c_str(), Jump);
        }
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, m_CompilePC + 8);
#endif
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompilerWarning(stdstr_f("BNE error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", m_NextInstruction).c_str());
        BreakPoint();
#endif
    }
}

void CRSPRecompilerOps::BLEZ(void)
{
    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        RSPInstruction Instruction(m_CompilePC, m_OpCode.Value);
        m_Recompiler.Log("  %X %s", m_CompilePC, Instruction.NameAndParam().c_str());
        if (m_OpCode.rs == 0)
        {
            m_DelayAffectBranch = false;
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        m_DelayAffectBranch = Instruction.DelaySlotAffectBranch();
        if (!m_DelayAffectBranch)
        {
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompConstToVariable(0, &m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs));
        SetleVariable(&BranchCompare, "BranchCompare");
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
#endif
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0x1FFC;

        if (m_OpCode.rs == 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            JmpLabel32("BranchToJump", 0);
            m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
            m_NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
#endif
            return;
        }
        if (!m_DelayAffectBranch)
        {
            m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
            asmjit::Label Jump;
            if (!m_Recompiler.FindBranchJump(Target, Jump))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_RegState.HasMappedRegisters())
            {
                asmjit::Label JumpCave = m_Assembler->newLabel();
                m_Assembler->JleLabel(stdstr_f("0x%X_From_0x%X", Target, m_CompilePC).c_str(), JumpCave);
                m_Assembler->SetSecondarySection();
                m_Assembler->bind(JumpCave);
                CRspRegState CaveState = m_RegState;
                CaveState.WriteBackRegisters();
                m_Assembler->JmpLabel(stdstr_f("0x%X", Target).c_str(), Jump);
                m_Assembler->SetPrimarySection();
            }
            else
            {
                m_Assembler->JleLabel(stdstr_f("0x%X", Target).c_str(), Jump);
            }
        }
        else
        {
            // Take a look at the branch compare variable
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchLessEqual", 0);
#endif
        }
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, m_CompilePC + 8);
#endif
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompilerWarning(stdstr_f("BGTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", m_NextInstruction).c_str());
        BreakPoint();
#endif
    }
}

void CRSPRecompilerOps::BGTZ(void)
{
    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        RSPInstruction Instruction(m_CompilePC, m_OpCode.Value);
        m_Recompiler.Log("  %X %s", m_CompilePC, Instruction.NameAndParam().c_str());
        if (m_OpCode.rs == 0)
        {
            m_DelayAffectBranch = false;
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        m_DelayAffectBranch = Instruction.DelaySlotAffectBranch();
        if (!m_DelayAffectBranch)
        {
            m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
        m_Assembler->SetgVariable(&BranchCompare, "BranchCompare");
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0x1FFC;

        if (m_OpCode.rs == 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            m_NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
#endif
            return;
        }
        asmjit::Label Jump;
        if (!m_Recompiler.FindBranchJump(Target, Jump))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        m_RegState.WriteBackRegisters();
        if (!m_DelayAffectBranch)
        {
            m_Assembler->CompConstToVariable(&m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs), 0);
            m_Assembler->JgLabel(stdstr_f("0x%X", Target).c_str(), Jump);
        }
        else
        {
            m_Assembler->CompConstToVariable(&BranchCompare, "BranchCompare", true);
            m_Assembler->JeLabel(stdstr_f("0x%X", Target).c_str(), Jump);
        }
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        uint32_t Target = (m_CompilePC + ((short)m_OpCode.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, m_CompilePC + 8);
#endif
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompilerWarning(stdstr_f("BGTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", m_NextInstruction).c_str());
        BreakPoint();
#endif
    }
}

void CRSPRecompilerOps::ADDI(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    if (m_OpCode.rt == 0)
    {
        return;
    }
    if (m_RegState.IsGprConst(m_OpCode.rs))
    {
        uint32_t result = m_RegState.GetGprConstValue(m_OpCode.rs) + (int16_t)m_OpCode.immediate;
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)), result);
        m_RegState.SetGprConst(m_OpCode.rt, result);
    }
    else
    {
        m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));

        if (m_OpCode.immediate != 0)
        {
            m_Assembler->add(asmjit::x86::eax, (int32_t)((int16_t)m_OpCode.immediate));
        }
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)), asmjit::x86::eax);
        m_RegState.SetGprUnknown(m_OpCode.rt);
    }
}

void CRSPRecompilerOps::ADDIU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::SLTI(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::SLTIU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::ANDI(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    if (m_OpCode.rt == 0)
    {
        return;
    }
    if (m_RegState.IsGprConst(m_OpCode.rs))
    {
        uint32_t result = m_RegState.GetGprConstValue(m_OpCode.rs) & (uint16_t)m_OpCode.immediate;
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)), result);
        m_RegState.SetGprConst(m_OpCode.rt, result);
    }
    else
    {
        m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));

        if (m_OpCode.immediate == 0)
        {
            m_Assembler->xor_(asmjit::x86::eax, asmjit::x86::eax);
        }
        else
        {
            m_Assembler->and_(asmjit::x86::eax, (uint16_t)m_OpCode.immediate);
        }
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)), asmjit::x86::eax);
        m_RegState.SetGprUnknown(m_OpCode.rt);
    }
}

void CRSPRecompilerOps::ORI(void)
{
    Cheat_r4300iOpcode(&RSPOp::ORI, "RSPOp::ORI");
}

void CRSPRecompilerOps::XORI(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::LUI(void)
{
    Cheat_r4300iOpcode(&RSPOp::LUI, "RSPOp::LUI");
}

void CRSPRecompilerOps::COP0(void)
{
    (this->*RSP_Recomp_Cop0[m_OpCode.rs])();
}

void CRSPRecompilerOps::COP2(void)
{
    (this->*RSP_Recomp_Cop2[m_OpCode.rs])();
}

void CRSPRecompilerOps::LB(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::LH(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    if (m_OpCode.rt == 0)
        return;

    m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));
    if (m_OpCode.offset != 0)
    {
        m_Assembler->add(asmjit::x86::eax, (int16_t)m_OpCode.offset);
    }
    m_Assembler->and_(asmjit::x86::eax, 0xFFF);
    m_Assembler->test(asmjit::x86::eax, 1);
    asmjit::Label unaligned = m_Assembler->newLabel();
    asmjit::Label done = m_Assembler->newLabel();

    m_Assembler->jnz(unaligned);
    // Aligned path
    m_Assembler->xor_(asmjit::x86::eax, 2);
    m_Assembler->movzx(asmjit::x86::eax, asmjit::x86::word_ptr(asmjit::x86::r15, asmjit::x86::rax));
    m_Assembler->movsx(asmjit::x86::eax, asmjit::x86::ax);

    // Unaligned path
    m_Assembler->SetSecondarySection();
    m_Assembler->bind(unaligned);
    m_Assembler->mov(asmjit::x86::ecx, asmjit::x86::eax);
    m_Assembler->xor_(asmjit::x86::ecx, 3);
    m_Assembler->movzx(asmjit::x86::edx, asmjit::x86::byte_ptr(asmjit::x86::r15, asmjit::x86::rcx));
    m_Assembler->shl(asmjit::x86::edx, 8);
    m_Assembler->mov(asmjit::x86::ecx, asmjit::x86::eax);
    m_Assembler->add(asmjit::x86::ecx, 1);
    m_Assembler->and_(asmjit::x86::ecx, 0xFFF);
    m_Assembler->xor_(asmjit::x86::ecx, 3);
    m_Assembler->movzx(asmjit::x86::ecx, asmjit::x86::byte_ptr(asmjit::x86::r15, asmjit::x86::rcx));
    m_Assembler->or_(asmjit::x86::edx, asmjit::x86::ecx);
    m_Assembler->movsx(asmjit::x86::eax, asmjit::x86::dx);
    m_Assembler->jmp(done);

    m_Assembler->SetPrimarySection();
    m_Assembler->bind(done);

    m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)), asmjit::x86::eax);
}

void CRSPRecompilerOps::LW(void)
{
    Cheat_r4300iOpcode(&RSPOp::LW, "RSPOp::LW");
}

void CRSPRecompilerOps::LBU(void)
{
    Cheat_r4300iOpcode(&RSPOp::LBU, "RSPOp::LBU");
}

void CRSPRecompilerOps::LHU(void)
{
    Cheat_r4300iOpcode(&RSPOp::LHU, "RSPOp::LHU");
}

void CRSPRecompilerOps::LWU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::SB(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::SH(void)
{
    Cheat_r4300iOpcode(&RSPOp::SH, "RSPOp::SH");
}

void CRSPRecompilerOps::SW(void)
{
    Cheat_r4300iOpcode(&RSPOp::SW, "RSPOp::SW");
}

void CRSPRecompilerOps::LC2(void)
{
    (this->*RSP_Recomp_Lc2[m_OpCode.rd])();
}

void CRSPRecompilerOps::SC2(void)
{
    (this->*RSP_Recomp_Sc2[m_OpCode.rd])();
}

// R4300i Opcodes: Special

void CRSPRecompilerOps::Special_SLL(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    if (m_OpCode.rd == 0)
    {
        return;
    }
    if (m_RegState.IsGprConst(m_OpCode.rt))
    {
        uint32_t result = m_RegState.GetGprConstValue(m_OpCode.rt) << m_OpCode.sa;
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), result);
        m_RegState.SetGprConst(m_OpCode.rd, result);
    }
    else
    {
        if (m_OpCode.sa == 0)
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        else
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            m_Assembler->shl(asmjit::x86::eax, m_OpCode.sa);
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        m_RegState.SetGprUnknown(m_OpCode.rd);
    }
}

void CRSPRecompilerOps::Special_SRL(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    if (m_OpCode.rd == 0)
    {
        return;
    }

    if (m_RegState.IsGprConst(m_OpCode.rt))
    {
        uint32_t result = m_RegState.GetGprConstValue(m_OpCode.rt) >> m_OpCode.sa;
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), result);
        m_RegState.SetGprConst(m_OpCode.rd, result);
    }
    else
    {
        if (m_OpCode.sa == 0)
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        else
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            m_Assembler->shr(asmjit::x86::eax, m_OpCode.sa);
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        m_RegState.SetGprUnknown(m_OpCode.rd);
    }
}

void CRSPRecompilerOps::Special_SRA(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_SLLV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_SRLV(void)
{
    Cheat_r4300iOpcode(&RSPOp::Special_SRLV, "RSPOp::Special_SRLV");
}

void CRSPRecompilerOps::Special_SRAV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_JR(void)
{
    //uint8_t * Jump = nullptr;

    if (m_NextInstruction == RSPPIPELINE_NORMAL)
    {
        m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
        m_Assembler->MoveVariableToX86reg(asmjit::x86::eax, &m_GPR[m_OpCode.rs].W, GPR_Name(m_OpCode.rs));
        m_Assembler->and_(asmjit::x86::eax, 0x1FFC);
        m_Assembler->MoveX86regToVariable(m_System.m_SP_PC_REG, "RSP PC", asmjit::x86::eax);
        m_NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        if (m_CurrentBlock && m_CurrentBlock->CodeType() == RspCodeType_SUBROUTINE)
        {
            ExitCodeBlock();
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
#ifdef tofix
        MoveVariableToX86reg(m_System.m_SP_PC_REG, "RSP PC", x86_EAX);
        AddVariableToX86reg(x86_EAX, &JumpTable, "JumpTable");
        MoveX86regPointerToX86reg(x86_EAX, x86_EAX);

        TestX86RegToX86Reg(x86_EAX, x86_EAX);
        JeLabel8("Null", 0);
        Jump = RecompPos - 1;
        JumpX86Reg(x86_EAX);

        x86_SetBranch8b(Jump, RecompPos);
        CPU_Message(" Null:");
        if (CRSPSettings::CPUMethod() == RSPCpuMethod::HighLevelEmulation)
        {
            BreakPoint();
        }
        Ret();
        ChangedPC = false;
        m_NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
#endif
    }
    else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        m_NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
        Ret();
#endif
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        CompilerWarning(stdstr_f("WTF\n\nJR\nNextInstruction = %X", m_NextInstruction).c_str());
        BreakPoint();
#endif
    }
}

void CRSPRecompilerOps::Special_JALR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_BREAK(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_ADD(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    if (m_RegState.IsGprConst(m_OpCode.rs) && m_RegState.IsGprConst(m_OpCode.rt))
    {
        uint32_t result = m_RegState.GetGprConstValue(m_OpCode.rs) + m_RegState.GetGprConstValue(m_OpCode.rt);
        m_RegState.SetGprConst(m_OpCode.rd, result);
        m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), result);
    }
    else if (m_RegState.IsGprConst(m_OpCode.rs) || m_RegState.IsGprConst(m_OpCode.rt))
    {
        m_RegState.SetGprUnknown(m_OpCode.rd);
        if (m_RegState.IsGprConst(m_OpCode.rt))
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));
            if (m_RegState.GetGprConstValue(m_OpCode.rt) != 0)
            {
                m_Assembler->add(asmjit::x86::eax, m_RegState.GetGprConstValue(m_OpCode.rt));
            }
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        else
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            if (m_RegState.GetGprConstValue(m_OpCode.rs) != 0)
            {
                m_Assembler->add(asmjit::x86::eax, m_RegState.GetGprConstValue(m_OpCode.rs));
            }
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
    }
    else
    {
        m_RegState.SetGprUnknown(m_OpCode.rd);
        if (m_OpCode.rd == m_OpCode.rs)
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            m_Assembler->add(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        else if (m_OpCode.rd == m_OpCode.rt)
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));
            m_Assembler->add(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        else if (m_OpCode.rs == m_OpCode.rt)
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));
            m_Assembler->add(asmjit::x86::eax, asmjit::x86::eax);
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
        else
        {
            m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rs)));
            m_Assembler->add(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rt)));
            m_Assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.rd)), asmjit::x86::eax);
        }
    }
}

void CRSPRecompilerOps::Special_ADDU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_SUB(void)
{
    Cheat_r4300iOpcode(&RSPOp::Special_SUB, "RSPOp::Special_SUB");
}

void CRSPRecompilerOps::Special_SUBU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_AND(void)
{
    Cheat_r4300iOpcode(&RSPOp::Special_AND, "RSPOp::Special_AND");
}

void CRSPRecompilerOps::Special_OR(void)
{
    Cheat_r4300iOpcode(&RSPOp::Special_OR, "RSPOp::Special_OR");
}

void CRSPRecompilerOps::Special_XOR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_NOR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_SLT(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Special_SLTU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// R4300i Opcodes: RegImm
void CRSPRecompilerOps::RegImm_BLTZ(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::RegImm_BGEZ(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::RegImm_BLTZAL(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::RegImm_BGEZAL(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// COP0 functions

void CRSPRecompilerOps::Cop0_MF(void)
{
    Cheat_r4300iOpcode(&RSPOp::Cop0_MF, "RSPOp::Cop0_MF");
}

void CRSPRecompilerOps::Cop0_MT(void)
{
    Cheat_r4300iOpcode(&RSPOp::Cop0_MT, "RSPOp::Cop0_MT");
}

// COP2 functions

void CRSPRecompilerOps::Cop2_MF(void)
{
    Cheat_r4300iOpcode(&RSPOp::Cop2_MF, "RSPOp::Cop2_MF");
}

void CRSPRecompilerOps::Cop2_CF(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Cop2_MT(void)
{
    Cheat_r4300iOpcode(&RSPOp::Cop2_MT, "RSPOp::Cop2_MT");
}

void CRSPRecompilerOps::Cop2_CT(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::COP2_VECTOR(void)
{
    (this->*RSP_Recomp_Vector[m_OpCode.funct])();
}

// Vector functions

void CRSPRecompilerOps::Vector_VMULF(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm lo = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm hi = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm round = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm sign1 = m_RegState.MapXmmTemp(false, 0, 0);

    // Phase 1: Multiply and shift left by 1 (×2)
    m_Assembler->movdqa(lo, vs);
    m_Assembler->pmullw(lo, vte); // lo = vs * vte (low)

    m_Assembler->pcmpeqw(round, round); // round = all 1s
    m_Assembler->movdqa(sign1, lo);
    m_Assembler->psrlw(sign1, 15); // sign1 = lo >> 15 (carry bit)

    m_Assembler->paddw(lo, lo);    // lo *= 2
    m_Assembler->psllw(round, 15); // round = 0x8000

    m_Assembler->movdqa(hi, vs);
    m_Assembler->pmulhw(hi, vte); // hi = vs * vte (high)

    asmjit::x86::Xmm sign2 = m_RegState.MapXmmTemp(false, 0, 0);
    m_Assembler->movdqa(sign2, lo);
    m_Assembler->psrlw(sign2, 15);    // sign2 = lo >> 15
    m_Assembler->paddw(sign1, sign2); // sign1 = sign1 + sign2 (total carry)
    m_RegState.UnprotectXmm(sign2);

    m_Assembler->psllw(hi, 1); // hi *= 2

    // Phase 2: Add 0x8000 to ACCL
    m_Assembler->paddw(lo, round); // ACCL = lo + 0x8000
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), lo);

    // Phase 3: ACCM = hi + carry
    m_Assembler->paddw(hi, sign1); // ACCM = hi + carry
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), hi);

    // Phase 4: Calculate ACCH and saturate
    asmjit::x86::Xmm neg = m_RegState.MapXmmTemp(false, 0, 0);
    m_Assembler->movdqa(neg, hi);
    m_Assembler->psraw(neg, 15); // neg = sign extend ACCM

    m_Assembler->pcmpeqw(vs, vte); // vs = (vs == vte)

    m_Assembler->movdqa(round, vs); // round = (vs == vte)
    m_Assembler->pandn(round, neg); // ACCH = ~(vs==vte) & neg
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), round);

    m_Assembler->pand(vs, neg); // vs = (vs==vte) & neg
    m_Assembler->paddw(hi, vs); // vd = ACCM + ((vs==vte) & neg)

    // Store result
    m_RegState.UnprotectXmm(vte);
    m_RegState.UnprotectXmm(vs);
    m_RegState.UnprotectXmm(sign1);
    m_RegState.UnprotectXmm(neg);
    m_RegState.UnprotectXmm(round);
    m_RegState.UnprotectXmm(lo);
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);
    m_Assembler->movdqa(vd, hi);
}

void CRSPRecompilerOps::Vector_VMULU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRNDN(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRNDP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMULQ(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMUDL(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm accl = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm zero = m_RegState.MapXmmTemp(false, 0, 0);

    m_Assembler->movdqa(accl, vs);
    m_Assembler->pmulhuw(accl, vte);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), accl);

    // Zero out ACCM and ACCH
    m_Assembler->pxor(zero, zero);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), zero);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), zero);

    // Free temps before mapping vd
    m_RegState.UnprotectXmm(vte);
    m_RegState.UnprotectXmm(vs);
    m_RegState.UnprotectXmm(zero);

    // vd = ACCL
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);
    m_Assembler->movdqa(vd, accl);
}

void CRSPRecompilerOps::Vector_VMUDM(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm sign = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm vta = m_RegState.MapXmmTemp(false, 0, 0);

    // Phase 1: Multiply signed × unsigned
    m_Assembler->movdqa(vta, vs);
    m_Assembler->pmullw(vta, vte); // ACCL = low 16 bits
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), vta);

    m_Assembler->movdqa(sign, vs);
    m_Assembler->pmulhuw(sign, vte); // High 16 bits (unsigned)

    // Phase 2: Correct for signed vs
    m_Assembler->movdqa(vta, vs);
    m_Assembler->psraw(vta, 15);   // sign = sign extend vs
    m_Assembler->pand(vta, vte);   // vta = vte & sign
    m_Assembler->psubw(sign, vta); // ACCM = high - correction
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), sign);

    // Phase 3: Sign extend ACCM to ACCH
    m_Assembler->movdqa(vta, sign);
    m_Assembler->psraw(vta, 15); // ACCH = sign extend ACCM
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), vta);

    // vd = ACCM
    m_RegState.UnprotectXmm(vte);
    m_RegState.UnprotectXmm(vs);
    m_RegState.UnprotectXmm(sign);
    m_RegState.UnprotectXmm(vta);
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);
    m_Assembler->movdqa(vd, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
}

void CRSPRecompilerOps::Vector_VMUDN(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);

    asmjit::x86::Xmm sign = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm vsa = m_RegState.MapXmmTemp(false, 0, 0);

    m_Assembler->movdqa(vd, vs);
    m_Assembler->pmullw(vd, vte);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), vd);
    m_Assembler->movdqa(vsa, vs);
    m_Assembler->pmulhuw(vsa, vte);

    m_Assembler->movdqa(sign, vte);
    m_Assembler->psraw(sign, 15);
    m_Assembler->pand(sign, vs);
    m_Assembler->psubw(vsa, sign);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), vsa);

    m_Assembler->psraw(vsa, 15);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), vsa);
}

void CRSPRecompilerOps::Vector_VMUDH(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VMUDH, "RSPOp::Vector_VMUDH");
}

void CRSPRecompilerOps::Vector_VMACF(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm lo = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm hi = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm md = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm carry = m_RegState.MapXmmTemp(false, 0, 0);

    // Phase 1: Multiply signed × signed
    m_Assembler->movdqa(lo, vs);
    m_Assembler->pmullw(lo, vte); // lo = low 16 bits
    m_Assembler->movdqa(hi, vs);
    m_Assembler->pmulhw(hi, vte); // hi = high 16 bits (signed)

    // Phase 2: Shift left by 1 (fraction multiply)
    m_Assembler->movdqa(md, hi);
    m_Assembler->psllw(md, 1); // md = hi << 1
    m_Assembler->movdqa(carry, lo);
    m_Assembler->psrlw(carry, 15); // carry = lo >> 15
    m_Assembler->psraw(hi, 15);    // hi = sign extend
    m_Assembler->por(md, carry);   // md |= carry (from lo to md)
    m_Assembler->psllw(lo, 1);     // lo = lo << 1

    // Phase 3: Add to ACCL (reuse vte as omask)
    asmjit::x86::Xmm omask = vte;
    m_Assembler->movdqa(omask, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)));
    m_Assembler->paddusw(omask, lo);
    m_Assembler->movdqa(carry, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)));
    m_Assembler->paddw(carry, lo);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), carry);
    m_Assembler->pcmpeqw(omask, carry);
    m_Assembler->pcmpeqw(carry, carry); // all 1s
    m_Assembler->pxor(omask, carry);    // invert
    m_Assembler->psubw(md, omask);

    // Propagate carry from md to hi
    m_Assembler->pxor(carry, carry);
    m_Assembler->pcmpeqw(carry, md); // carry = (md == 0)
    m_Assembler->pand(carry, omask);
    m_Assembler->psubw(hi, carry);

    // Phase 4: Add to ACCM
    m_Assembler->movdqa(omask, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddusw(omask, md);
    m_Assembler->movdqa(carry, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddw(carry, md);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), carry);
    m_Assembler->pcmpeqw(omask, carry);
    m_Assembler->pcmpeqw(carry, carry);
    m_Assembler->pxor(omask, carry);

    // Phase 5: Add to ACCH
    m_Assembler->movdqa(carry, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->paddw(carry, hi);
    m_Assembler->psubw(carry, omask);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), carry);

    // Phase 6: Signed saturation (pack ACCM:ACCH)
    m_RegState.UnprotectXmm(vte);
    m_RegState.UnprotectXmm(md);
    m_RegState.UnprotectXmm(carry);
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);

    m_Assembler->movdqa(lo, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->movdqa(hi, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->movdqa(vs, lo);    // reuse vs
    m_Assembler->punpcklwd(vs, hi); // vs = unpacklo(ACCM, ACCH)
    m_Assembler->punpckhwd(lo, hi); // lo = unpackhi(ACCM, ACCH)
    m_Assembler->packssdw(vs, lo);  // signed pack
    m_Assembler->movdqa(vd, vs);
}

void CRSPRecompilerOps::Vector_VMACU(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMACQ(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMADL(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMADM(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm lo = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm hi = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm temp = m_RegState.MapXmmTemp(false, 0, 0);

    // Phase 1: Multiply signed vs × unsigned vte
    m_Assembler->movdqa(lo, vs);
    m_Assembler->pmullw(lo, vte); // lo = low 16 bits

    m_Assembler->movdqa(hi, vs);
    m_Assembler->pmulhuw(hi, vte); // hi = high 16 bits (unsigned)

    m_Assembler->movdqa(temp, vs);
    m_Assembler->psraw(temp, 15); // temp = sign extend vs
    m_Assembler->pand(temp, vte); // temp = vte & sign
    m_Assembler->psubw(hi, temp); // hi -= correction

    // Phase 2: Add to ACCL with overflow detection
    m_Assembler->movdqa(temp, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)));
    m_Assembler->paddusw(temp, lo); // temp = saturated add
    m_Assembler->movdqa(vs, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)));
    m_Assembler->paddw(vs, lo); // vs = ACCL + lo
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), vs);
    m_Assembler->pcmpeqw(temp, vs);
    m_Assembler->pcmpeqw(lo, lo); // lo = all 1s
    m_Assembler->pxor(temp, lo);  // temp = overflow mask
    m_Assembler->psubw(hi, temp); // Carry propagation

    // Phase 3: Add to ACCM with overflow detection
    m_Assembler->movdqa(temp, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddusw(temp, hi); // temp = saturated add
    m_Assembler->movdqa(vs, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddw(vs, hi); // vs = ACCM + hi
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), vs);
    m_Assembler->pcmpeqw(temp, vs);
    m_Assembler->pcmpeqw(lo, lo); // lo = all 1s (reuse)
    m_Assembler->pxor(temp, lo);  // temp = overflow mask

    // Phase 4: Add to ACCH
    m_Assembler->psraw(hi, 15); // Sign extend hi
    m_Assembler->movdqa(vs, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->paddw(vs, hi);
    m_Assembler->psubw(vs, temp);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), vs);

    // Phase 5: Saturate ACCM:ACCH for output
    m_RegState.UnprotectXmm(vte);
    m_RegState.UnprotectXmm(temp);
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);

    m_Assembler->movdqa(lo, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->movdqa(hi, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->movdqa(vs, lo);
    m_Assembler->punpcklwd(vs, hi);
    m_Assembler->punpckhwd(lo, hi);
    m_Assembler->packssdw(vs, lo);
    m_Assembler->movdqa(vd, vs);
}

void CRSPRecompilerOps::Vector_VMADN(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm lo = m_RegState.MapSpecificXmmTemp(0, false, 0, 0);
    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm hi = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm sign = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm vsa = m_RegState.MapXmmTemp(false, 0, 0);

    // Phase 1: Multiply
    m_Assembler->movdqa(lo, vs);
    m_Assembler->pmullw(lo, vte);
    m_Assembler->movdqa(hi, vs);
    m_Assembler->pmulhuw(hi, vte);
    m_Assembler->movdqa(sign, vte);
    m_Assembler->psraw(sign, 15);
    m_Assembler->movdqa(vsa, vs);
    m_Assembler->pand(vsa, sign);
    m_Assembler->psubw(hi, vsa);

    // Phase 2: Accumulate - reuse sign as omask
    asmjit::x86::Xmm omask = sign;

    // Add to ACCL
    m_Assembler->movdqa(omask, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)));
    m_Assembler->paddusw(omask, lo);
    m_Assembler->movdqa(vsa, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)));
    m_Assembler->paddw(vsa, lo);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), vsa);
    m_Assembler->pcmpeqw(omask, vsa);
    // Invert omask without needing zero: NOT(omask)
    m_Assembler->pcmpeqw(vsa, vsa); // vsa = all 1s
    m_Assembler->pxor(omask, vsa);  // omask = NOT(omask)
    m_Assembler->psubw(hi, omask);

    // Add to ACCM
    m_Assembler->movdqa(omask, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddusw(omask, hi);
    m_Assembler->movdqa(vsa, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddw(vsa, hi);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), vsa);
    m_Assembler->pcmpeqw(omask, vsa);
    m_Assembler->pcmpeqw(vsa, vsa); // vsa = all 1s
    m_Assembler->pxor(omask, vsa);  // omask = NOT(omask)

    // Add to ACCH
    m_Assembler->psraw(hi, 15);
    m_Assembler->movdqa(vsa, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->paddw(vsa, hi);
    m_Assembler->psubw(vsa, omask);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), vsa);

    // Phase 3: Saturation
    m_RegState.UnprotectXmm(sign);
    m_RegState.UnprotectXmm(vsa);
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);
    asmjit::x86::Xmm nhi = vte;
    asmjit::x86::Xmm nmd = vs;
    asmjit::x86::Xmm cmask = lo;

    m_Assembler->movdqa(nhi, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->psraw(nhi, 15);
    m_Assembler->movdqa(nmd, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->psraw(nmd, 15);

    m_Assembler->movdqa(cmask, nhi);
    m_Assembler->pcmpeqw(cmask, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->movdqa(hi, nhi);
    m_Assembler->pcmpeqw(hi, nmd);
    m_Assembler->pand(cmask, hi);

    // Create zero inline for final comparison
    m_Assembler->pxor(hi, hi);     // hi = zero
    m_Assembler->pcmpeqw(nhi, hi); // cval = (nhi == 0)
    m_Assembler->movdqa(vd, nhi);
    m_Assembler->pblendvb(vd, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), cmask);
}

void CRSPRecompilerOps::Vector_VMADH(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    asmjit::x86::Xmm vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
    asmjit::x86::Xmm vs = m_RegState.MapXmmTemp(true, m_OpCode.vs, 0);
    asmjit::x86::Xmm lo = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm hi = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm omask = m_RegState.MapXmmTemp(false, 0, 0);
    asmjit::x86::Xmm accm = m_RegState.MapXmmTemp(false, 0, 0);

    // Phase 1: Multiply signed × signed
    m_Assembler->movdqa(lo, vs);
    m_Assembler->pmullw(lo, vte);
    m_Assembler->movdqa(hi, vs);
    m_Assembler->pmulhw(hi, vte);

    // Phase 2: Add to ACCM with overflow detection
    m_Assembler->movdqa(omask, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddusw(omask, lo);
    m_Assembler->movdqa(accm, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->paddw(accm, lo);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)), accm);
    m_Assembler->pcmpeqw(omask, accm);
    m_Assembler->pcmpeqw(accm, accm); // all 1s
    m_Assembler->pxor(omask, accm);   // invert (overflow mask)
    m_Assembler->psubw(hi, omask);

    // Phase 3: Add to ACCH
    m_Assembler->movdqa(accm, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->paddw(accm, hi);
    m_Assembler->movdqa(asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)), accm);

    // Free temps before mapping vd (keep vs, lo, hi for packing)
    m_RegState.UnprotectXmm(vte);
    m_RegState.UnprotectXmm(omask);
    m_RegState.UnprotectXmm(accm);

    // Phase 4: Saturate (pack ACCM:ACCH)
    asmjit::x86::Xmm vd = m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vd, false);

    m_Assembler->movdqa(lo, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Middle)));
    m_Assembler->movdqa(hi, asmjit::x86::xmmword_ptr(asmjit::x86::r14, AccumOffset(AccumLocation::High)));
    m_Assembler->movdqa(vs, lo);
    m_Assembler->punpcklwd(vs, hi);
    m_Assembler->punpckhwd(lo, hi);
    m_Assembler->packssdw(vs, lo);
    m_Assembler->movdqa(vd, vs);
}

void CRSPRecompilerOps::Vector_VADD(void)
{
    bool writeToDest = WriteToVectorDest(m_OpCode.vd, m_CompilePC);
    bool writeToAccum = WriteToAccum(AccumLocation::Low, m_CompilePC);

    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    asmjit::x86::Xmm vs, vte, vcol;
    if (writeToAccum || writeToDest)
    {
        vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
        vs = writeToDest ? m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vs) : m_RegState.MapXmmTemp(true, m_OpCode.vs);
        if (!m_RegState.IsFlagZero(RspFlags::VCOL))
        {
            vcol = m_RegState.MapXmmTemp(false, 0);
            m_Assembler->movdqa(vcol, asmjit::x86::ptr(asmjit::x86::r14, FlagOffset(RspFlags::VCOL)));
        }
    }
    if (writeToAccum)
    {
        asmjit::x86::Xmm accum = m_RegState.MapXmmTemp(false, 0);
        m_Assembler->movdqa(accum, vs);
        m_Assembler->paddw(accum, vte);
        if (!m_RegState.IsFlagZero(RspFlags::VCOL))
        {
            m_Assembler->paddw(accum, vcol);
        }
        m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), accum);
        m_RegState.UnprotectXmm(accum);
    }
    if (writeToDest)
    {
        m_Assembler->paddsw(vs, vte);
        if (!m_RegState.IsFlagZero(RspFlags::VCOL))
        {
            m_Assembler->paddsw(vs, vcol);
        }
    }
    if (vcol.isValid())
    {
        m_RegState.UnprotectXmm(vcol);
    }

    if (!m_RegState.IsFlagZero(RspFlags::VCOL) || !m_RegState.IsFlagZero(RspFlags::VCOH))
    {
        asmjit::x86::Xmm zero = m_RegState.MapXmmZero();
        if (!m_RegState.IsFlagZero(RspFlags::VCOL))
        {
            m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, FlagOffset(RspFlags::VCOL)), zero);
            m_RegState.SetFlagZero(RspFlags::VCOL);
        }
        if (!m_RegState.IsFlagZero(RspFlags::VCOH))
        {
            m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, FlagOffset(RspFlags::VCOH)), zero);
            m_RegState.SetFlagZero(RspFlags::VCOH);
        }
    }
}

void CRSPRecompilerOps::Vector_VSUB(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VSUB, "RSPOp::Vector_VSUB");
}

void CRSPRecompilerOps::Vector_VABS(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VADDC(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VADDC, "RSPOp::Vector_VADDC");
}

void CRSPRecompilerOps::Vector_VSUBC(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VSUBC, "RSPOp::Vector_VSUBC");
}

void CRSPRecompilerOps::Vector_VSAW(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VSAW, "RSPOp::Vector_VSAW");
}

void CRSPRecompilerOps::Vector_VLT(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VEQ(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VNE(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VGE(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VGE, "RSPOp::Vector_VGE");
}

void CRSPRecompilerOps::Vector_VCL(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VCL, "RSPOp::Vector_VCL");
}

void CRSPRecompilerOps::Vector_VCH(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VCR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMRG(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VAND(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    bool writeToAccum = WriteToAccum(AccumLocation::Low, m_CompilePC);
    bool writeToDest = WriteToVectorDest(m_OpCode.vd, m_CompilePC);

    asmjit::x86::Xmm vs, vte;
    if (writeToAccum || writeToDest)
    {
        vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
        vs = writeToDest ? m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vs) : m_RegState.MapXmmTemp(true, m_OpCode.vs);
        m_Assembler->pand(vs, vte);
    }
    if (writeToAccum)
    {
        m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), vs);
    }
}

void CRSPRecompilerOps::Vector_VNAND(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VOR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VNOR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VXOR(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    bool writeToAccum = WriteToAccum(AccumLocation::Low, m_CompilePC);
    bool writeToDest = WriteToVectorDest(m_OpCode.vd, m_CompilePC);
    if (m_OpCode.vs == m_OpCode.vt && m_OpCode.e == 0)
    {
        asmjit::x86::Xmm reg;
        if (writeToAccum || writeToDest)
        {
            reg = m_RegState.MapXmmZero();
        }
        if (writeToDest)
        {
            m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, VectorOffset(m_OpCode.vd)), reg);
        }
        if (writeToAccum)
        {
            m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), reg);
        }
    }
    else
    {
        asmjit::x86::Xmm vs, vte;
        if (writeToAccum || writeToDest)
        {
            vte = m_RegState.MapXmmTemp(true, m_OpCode.vt, m_OpCode.e);
            vs = writeToDest ? m_RegState.MapXmmReg(m_OpCode.vd, m_OpCode.vs) : m_RegState.MapXmmTemp(true, m_OpCode.vs);
            m_Assembler->pxor(vs, vte);
        }
        if (writeToAccum)
        {
            m_Assembler->movdqa(asmjit::x86::ptr(asmjit::x86::r14, AccumOffset(AccumLocation::Low)), vs);
        }
    }
}

void CRSPRecompilerOps::Vector_VNXOR(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRCP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRCPL(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRCPH(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VMOV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRSQ(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRSQL(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VRSQH(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Vector_VNOOP(void)
{
}

void CRSPRecompilerOps::Vector_Reserved(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// LC2 functions

void CRSPRecompilerOps::Opcode_LBV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_LSV(void)
{
    Cheat_r4300iOpcode(&RSPOp::LSV, "RSPOp::LSV");
}

void CRSPRecompilerOps::Opcode_LLV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_LDV(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());
    if ((m_OpCode.del & 0x3) != 0)
    {
        Cheat_r4300iOpcode(&RSPOp::LDV, "RSPOp::LDV", false);
        return;
    }
    if (m_RegState.IsGprConst(m_OpCode.base))
    {
        Cheat_r4300iOpcode(&RSPOp::LDV, "RSPOp::LDV", false);
        return;
    }
    uint8_t Length = std::min((uint8_t)8, (uint8_t)(16 - m_OpCode.del));
    if (Length != 8)
    {
        Cheat_r4300iOpcode(&RSPOp::LDV, "RSPOp::LDV", false);
        return;
    }
    asmjit::x86::Xmm vt = m_RegState.MapXmmReg(m_OpCode.vt, m_OpCode.vt);
    m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.base)));
    if (m_OpCode.voffset != 0)
    {
        m_Assembler->add(asmjit::x86::eax, m_OpCode.voffset << 3);
    }
    m_Assembler->and_(asmjit::x86::eax, 0xFFF);
    m_Assembler->test(asmjit::x86::eax, 7);
    asmjit::Label Unaligned = m_Assembler->newLabel();
    asmjit::Label LoadDoneLabel = m_Assembler->newLabel();
    m_Assembler->jnz(Unaligned);
    m_Assembler->mov(asmjit::x86::ecx, asmjit::x86::dword_ptr(asmjit::x86::r15, asmjit::x86::rax));
    m_Assembler->mov(asmjit::x86::edx, asmjit::x86::dword_ptr(asmjit::x86::r15, asmjit::x86::rax, 0, 4));
    m_Assembler->SetSecondarySection();
    m_Assembler->bind(Unaligned);

    // rcx will hold the result, edi is counter
    m_Assembler->xor_(asmjit::x86::rcx, asmjit::x86::rcx);
    m_Assembler->xor_(asmjit::x86::edi, asmjit::x86::edi); // Start from 0

    asmjit::Label LoopStart = m_Assembler->newLabel();
    m_Assembler->bind(LoopStart);

    // Shift previous bytes left (except first iteration)
    m_Assembler->test(asmjit::x86::edi, asmjit::x86::edi);
    asmjit::Label SkipShift = m_Assembler->newLabel();
    m_Assembler->jz(SkipShift);
    m_Assembler->shl(asmjit::x86::rcx, 8);
    m_Assembler->bind(SkipShift);

    // Load byte at (Address + edi) ^ 3
    m_Assembler->mov(asmjit::x86::esi, asmjit::x86::eax);
    m_Assembler->add(asmjit::x86::esi, asmjit::x86::edi);
    m_Assembler->and_(asmjit::x86::esi, 0xFFF);
    m_Assembler->xor_(asmjit::x86::esi, 3);
    m_Assembler->movzx(asmjit::x86::esi, asmjit::x86::byte_ptr(asmjit::x86::r15, asmjit::x86::rsi));
    m_Assembler->or_(asmjit::x86::rcx, asmjit::x86::rsi);

    // Increment and loop
    m_Assembler->inc(asmjit::x86::edi);
    m_Assembler->cmp(asmjit::x86::edi, Length);
    m_Assembler->jl(LoopStart); // Loop while edi < Length

    m_Assembler->mov(asmjit::x86::rdx, asmjit::x86::rcx);
    m_Assembler->xor_(asmjit::x86::rcx, asmjit::x86::rcx);

    m_Assembler->jmp(LoadDoneLabel);
    m_Assembler->SetPrimarySection();
    m_Assembler->bind(LoadDoneLabel);
    m_Assembler->shl(asmjit::x86::rcx, 32);
    m_Assembler->or_(asmjit::x86::rcx, asmjit::x86::rdx);
    if (m_OpCode.del == 0)
    {
        m_Assembler->pinsrq(vt, asmjit::x86::ecx, 1);
    }
    else
    {
        m_Assembler->pinsrq(vt, asmjit::x86::ecx, 0);
    }
}

void CRSPRecompilerOps::Opcode_LQV(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    if (m_RegState.IsGprConst(m_OpCode.base))
    {
        uint32_t Address = (uint32_t)(m_RegState.GetGprConstValue(m_OpCode.base) + (m_OpCode.voffset << 4)) & 0xFFF;
        uint8_t Length = std::min((uint8_t)(((Address + 0x10) & ~0xF) - Address), (uint8_t)(16 - m_OpCode.del));
        if (Length == 16 && Address % 16 == 0 && m_OpCode.del == 0)
        {
            asmjit::x86::Xmm vt = m_RegState.MapXmmReg(m_OpCode.vt, m_OpCode.vt, false);
            m_Assembler->movdqu(vt, asmjit::x86::ptr(asmjit::x86::r15, Address));
            m_Assembler->pshufd(vt, vt, 0x1B);
        }
        else
        {
            Cheat_r4300iOpcode(&RSPOp::LQV, "RSPOp::LQV", false);
        }
    }
    else if (m_OpCode.del == 0)
    {
        asmjit::x86::Xmm vt = m_RegState.MapXmmReg(m_OpCode.vt, m_OpCode.vt, false);

        asmjit::x86::Gpd addr = asmjit::x86::eax;
        m_Assembler->mov(addr, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.base)));
        m_Assembler->add(addr, m_OpCode.voffset << 4);
        m_Assembler->and_(addr, 0xFFF);

        asmjit::Label unaligned = m_Assembler->newLabel();
        asmjit::Label done = m_Assembler->newLabel();
        m_Assembler->test(addr, 0xF);
        m_Assembler->jnz(unaligned);

        m_Assembler->movdqu(vt, asmjit::x86::ptr(asmjit::x86::r15, addr));
        m_Assembler->pshufd(vt, vt, 0x1B);

        m_Assembler->SetSecondarySection();
        m_Assembler->bind(unaligned);
        m_Assembler->int3();
        m_Assembler->MoveConstToVariable(&m_System.m_OpCode.Value, "m_OpCode.Value", m_OpCode.Value);
        m_Assembler->CallThis(&RSPSystem.m_Op, AddressOf(&RSPOp::LQV), "RSPOp::LQV");
        m_Assembler->movdqa(vt, asmjit::x86::ptr(asmjit::x86::r14, VectorOffset(m_OpCode.vt)));
        m_Assembler->jmp(done);
        m_Assembler->SetPrimarySection();
        m_Assembler->bind(done);
    }
    else
    {
        Cheat_r4300iOpcode(&RSPOp::LQV, "RSPOp::LQV", false);
    }
}

void CRSPRecompilerOps::Opcode_LRV(void)
{
    Cheat_r4300iOpcode(&RSPOp::LRV, "RSPOp::LRV");
}

void CRSPRecompilerOps::Opcode_LPV(void)
{
    Cheat_r4300iOpcode(&RSPOp::LPV, "RSPOp::LPV");
}

void CRSPRecompilerOps::Opcode_LUV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_LHV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_LFV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_LWV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_LTV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// SC2 functions

void CRSPRecompilerOps::Opcode_SBV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_SSV(void)
{
    m_Assembler->comment(stdstr_f("%X %s", m_CompilePC, RSPInstruction(m_CompilePC, m_OpCode.Value).NameAndParam().c_str()).c_str());

    // Calculate address: (base + voffset << 1) & 0xFFF
    m_Assembler->mov(asmjit::x86::eax, asmjit::x86::dword_ptr(asmjit::x86::r14, GprOffset(m_OpCode.base)));
    if (m_OpCode.voffset != 0)
    {
        m_Assembler->add(asmjit::x86::eax, m_OpCode.voffset << 1);
    }
    m_Assembler->and_(asmjit::x86::eax, 0xFFF);

    // Load the vector register
    asmjit::x86::Xmm vt = m_RegState.MapXmmTemp(true, m_OpCode.vt, 0);

    // Extract and store first byte: vt.u8[15 - (del & 0xF)]
    uint8_t element = 15 - (m_OpCode.del & 0xF);
    m_Assembler->pextrb(asmjit::x86::ecx, vt, element);

    m_Assembler->mov(asmjit::x86::edx, asmjit::x86::eax);
    m_Assembler->xor_(asmjit::x86::edx, 3);
    m_Assembler->mov(asmjit::x86::byte_ptr(asmjit::x86::r15, asmjit::x86::rdx), asmjit::x86::cl);

    // Extract and store second byte: vt.u8[15 - ((del+1) & 0xF)]
    element = 15 - ((m_OpCode.del + 1) & 0xF);
    m_Assembler->pextrb(asmjit::x86::ecx, vt, element);

    m_Assembler->inc(asmjit::x86::eax);
    m_Assembler->and_(asmjit::x86::eax, 0xFFF);
    m_Assembler->xor_(asmjit::x86::eax, 3);
    m_Assembler->mov(asmjit::x86::byte_ptr(asmjit::x86::r15, asmjit::x86::rax), asmjit::x86::cl);
}

void CRSPRecompilerOps::Opcode_SLV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SLV, "RSPOp::SLV");
}

void CRSPRecompilerOps::Opcode_SDV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SDV, "RSPOp::SDV");
}

void CRSPRecompilerOps::Opcode_SQV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SQV, "RSPOp::SQV");
}

void CRSPRecompilerOps::Opcode_SRV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_SPV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_SUV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_SHV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_SFV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_STV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::Opcode_SWV(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// Other functions

void CRSPRecompilerOps::UnknownOpcode(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompilerOps::EnterCodeBlock(void)
{
    m_Assembler->sub(asmjit::x86::rsp, FunctionStackSize);
    if (Profiling && m_CurrentBlock->CodeType() == RspCodeType_TASK)
    {
        m_Assembler->mov(asmjit::x86::rcx, asmjit::imm((uintptr_t)m_CompilePC));
        m_Assembler->CallFunc(AddressOf(&StartTimer), "StartTimer");
    }
}

void CRSPRecompilerOps::ExitCodeBlock(void)
{
    if (Profiling && m_CurrentBlock->CodeType() == RspCodeType_TASK)
    {
        m_Assembler->CallFunc(AddressOf(&StopTimer), "StopTimer");
    }
    m_Assembler->add(asmjit::x86::rsp, FunctionStackSize);
    m_Assembler->ret();
}

void CRSPRecompilerOps::LoadVectorRegister(asmjit::x86::Xmm xmmReg, uint8_t vectorReg, uint8_t e)
{
    if (e < 8)
    {
        m_Assembler->movdqa(xmmReg, asmjit::x86::ptr(asmjit::x86::r14, VectorOffset(vectorReg)));
        if (e > 1)
        {
            switch (e)
            {
            case 2: // 0q
                m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(3, 3, 1, 1));
                m_Assembler->pshufhw(xmmReg, xmmReg, _MM_SHUFFLE(3, 3, 1, 1));
                break;
            case 3: // 1q
                m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(2, 2, 0, 0));
                m_Assembler->pshufhw(xmmReg, xmmReg, _MM_SHUFFLE(2, 2, 0, 0));
                break;
            case 4: // 0h
                m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(3, 3, 3, 3));
                m_Assembler->pshufhw(xmmReg, xmmReg, _MM_SHUFFLE(3, 3, 3, 3));
                break;
            case 5: // 1h
                m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(2, 2, 2, 2));
                m_Assembler->pshufhw(xmmReg, xmmReg, _MM_SHUFFLE(2, 2, 2, 2));
                break;
            case 6: // 2h
                m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(1, 1, 1, 1));
                m_Assembler->pshufhw(xmmReg, xmmReg, _MM_SHUFFLE(1, 1, 1, 1));
                break;
            case 7: // 3h
                m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(0, 0, 0, 0));
                m_Assembler->pshufhw(xmmReg, xmmReg, _MM_SHUFFLE(0, 0, 0, 0));
                break;
            }
        }
    }
    else
    {
        m_Assembler->movzx(asmjit::x86::eax, asmjit::x86::word_ptr(asmjit::x86::r14, (uint32_t)((uint8_t *)&m_Vect[vectorReg].se(0, e) - (uint8_t *)&m_Reg)));
        m_Assembler->movd(xmmReg, asmjit::x86::eax);
        m_Assembler->pshuflw(xmmReg, xmmReg, _MM_SHUFFLE(0, 0, 0, 0));
        m_Assembler->pshufd(xmmReg, xmmReg, _MM_SHUFFLE(0, 0, 0, 0));
    }
}

bool CRSPRecompilerOps::WriteToVectorDest(uint32_t DestReg, uint32_t PC)
{
    const RSPInstructions & instructions = m_CurrentBlock->GetInstructions();
    size_t start = m_CurrentBlock->InstructionIndex(PC);
    if (start > 0)
    {
        const RSPInstruction & instruction = instructions[start - 1];
        if (instruction.IsJump() || instruction.isBranch())
        {
            return true;
        }
    }
    for (size_t i = start + 1, n = instructions.size(); i < n; i++)
    {
        const RSPInstruction & instruction = instructions[i];
        if (instruction.IsJump() || instruction.isBranch())
        {
            n = i + 1;
        }
        if (instruction.isVectorOp())
        {
            if (instruction.SourceReg0() == DestReg || instruction.SourceReg1() == DestReg)
            {
                return true;
            }
            if (instruction.DestReg() == DestReg)
            {
                return false;
            }
        }
        if ((instruction.isVectorStoreOp() && instruction.SourceReg0() == DestReg) ||
            (instruction.isMfCop2() && instruction.SourceReg0() == DestReg))
        {
            return true;
        }
    }
    return true;
}

bool CRSPRecompilerOps::WriteToAccum(AccumLocation Location, uint32_t PC)
{
    const RSPInstructions & instructions = m_CurrentBlock->GetInstructions();
    size_t start = m_CurrentBlock->InstructionIndex(PC);
    if (start > 0)
    {
        const RSPInstruction & instruction = instructions[start - 1];
        if (instruction.IsJump() || instruction.isBranch())
        {
            return true;
        }
    }
    for (size_t i = start + 1, n = instructions.size(); i < n; i++)
    {
        const RSPInstruction & instruction = instructions[i];
        if (instruction.IsJump() || instruction.isBranch())
        {
            n = i + 1;
        }

        switch (Location)
        {
        case AccumLocation::Low:
            if (instruction.ReadAccumLow())
            {
                return true;
            }
            if (instruction.SetAccumLow())
            {
                return false;
            }
            break;
        case AccumLocation::Middle:
            if (instruction.ReadAccumMid())
            {
                return true;
            }
            if (instruction.SetAccumMid())
            {
                return false;
            }
            break;
        case AccumLocation::High:
            if (instruction.ReadAccumHigh())
            {
                return true;
            }
            if (instruction.SetAccumHigh())
            {
                return false;
            }
            break;
        case AccumLocation::Entire:
            if (instruction.ReadAccumLow() || instruction.ReadAccumMid() || instruction.ReadAccumHigh())
            {
                return true;
            }
            if (instruction.SetAccumLow() && instruction.SetAccumMid() && instruction.SetAccumHigh())
            {
                return false;
            }
            break;
        }
    }
    return true;
}

uint32_t CRSPRecompilerOps::GprOffset(uint8_t gpReg) const
{
    return (uint32_t)((uint8_t *)&m_GPR[gpReg] - (uint8_t *)&m_Reg);
}

uint32_t CRSPRecompilerOps::VectorOffset(uint8_t vectorReg) const
{
    return (uint32_t)((uint8_t *)&m_Vect[vectorReg] - (uint8_t *)&m_Reg);
}

uint32_t CRSPRecompilerOps::AccumOffset(AccumLocation location) const
{
    switch (location)
    {
    case AccumLocation::Low:
        return (uint32_t)((uint8_t *)&m_ACCUM.Low(0) - (uint8_t *)&m_Reg);
    case AccumLocation::Middle:
        return (uint32_t)((uint8_t *)&m_ACCUM.Mid(0) - (uint8_t *)&m_Reg);
    case AccumLocation::High:
        return (uint32_t)((uint8_t *)&m_ACCUM.High(0) - (uint8_t *)&m_Reg);
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return 0;
}

uint32_t CRSPRecompilerOps::FlagOffset(RspFlags flag) const
{
    switch (flag)
    {
    case RspFlags::VCOL:
        return (uint32_t)(m_VCOL.Value() - (uint8_t *)&m_Reg);
    case RspFlags::VCOH:
        return (uint32_t)(m_VCOH.Value() - (uint8_t *)&m_Reg);
    case RspFlags::VCCL:
        return (uint32_t)(m_VCCL.Value() - (uint8_t *)&m_Reg);
    case RspFlags::VCCH:
        return (uint32_t)(m_VCCH.Value() - (uint8_t *)&m_Reg);
    case RspFlags::VCE:
        return (uint32_t)(m_VCE.Value() - (uint8_t *)&m_Reg);
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return 0;
}
#endif