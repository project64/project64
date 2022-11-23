#include "stdafx.h"

#include <Project64-core/Debugger.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/R4300iInstruction.h>
#include <Project64-core/N64System/Mips/R4300iOpcode.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/LoopAnalysis.h>
#include <Project64-core/N64System/Recompiler/SectionInfo.h>
#include <Project64-core/N64System/SystemGlobals.h>

CCodeSection::CCodeSection(CCodeBlock & CodeBlock, uint32_t EnterPC, uint32_t ID, bool LinkAllowed) :
    m_CodeBlock(CodeBlock),
    m_SectionID(ID),
    m_EnterPC(EnterPC),
    m_EndPC((uint32_t)-1),
    m_ContinueSection(nullptr),
    m_JumpSection(nullptr),
    m_EndSection(false),
    m_LinkAllowed(LinkAllowed),
    m_Test(0),
    m_Test2(0),
    m_InLoop(false),
    m_DelaySlot(false),
    m_RecompilerOps(CodeBlock.RecompilerOps()),
    m_RegEnter(CodeBlock, CodeBlock.RecompilerOps()->Assembler()),
    m_Jump(CodeBlock),
    m_Cont(CodeBlock)
{
    m_CodeBlock.Log("%s: ID %d EnterPC 0x%08X", __FUNCTION__, ID, EnterPC);
    m_RecompilerOps->SetCurrentSection(this);
}

CCodeSection::~CCodeSection()
{
}

void CCodeSection::GenerateSectionLinkage()
{
    CCodeSection * TargetSection[] = {m_ContinueSection, m_JumpSection};
    CJumpInfo * JumpInfo[] = {&m_Cont, &m_Jump};
    int i;

    for (i = 0; i < 2; i++)
    {
        if (!JumpInfo[i]->LinkLocation.isValid() &&
            JumpInfo[i]->FallThrough == false)
        {
            JumpInfo[i]->TargetPC = (uint32_t)-1;
        }
    }

    if ((m_RecompilerOps->GetCurrentPC() & 0xFFC) == 0xFFC)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    // Handle permanent loop
    if (m_RecompilerOps->GetCurrentPC() == m_Jump.TargetPC && (m_Cont.FallThrough == false))
    {
        R4300iOpcode JumpOp, DelaySlot;
        if (g_MMU->MemoryValue32(m_RecompilerOps->GetCurrentPC(), JumpOp.Value) &&
            g_MMU->MemoryValue32(m_RecompilerOps->GetCurrentPC() + 4, DelaySlot.Value) &&
            !R4300iInstruction(m_RecompilerOps->GetCurrentPC(), JumpOp.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            m_RecompilerOps->CompileInPermLoop(m_Jump.RegSet, m_RecompilerOps->GetCurrentPC());
        }
    }
    if (TargetSection[0] != TargetSection[1] || TargetSection[0] == nullptr)
    {
        for (i = 0; i < 2; i++)
        {
            if (!JumpInfo[i]->LinkLocation.isValid() && JumpInfo[i]->FallThrough == false)
            {
                if (TargetSection[i])
                {
                    TargetSection[i]->UnlinkParent(this, i == 0);
                    TargetSection[i] = nullptr;
                }
            }
            else if (TargetSection[i] == nullptr && JumpInfo[i]->FallThrough)
            {
                m_RecompilerOps->LinkJump(*JumpInfo[i], (uint32_t)-1);
                m_RecompilerOps->CompileExit(JumpInfo[i]->JumpPC, JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet, JumpInfo[i]->Reason);
                JumpInfo[i]->FallThrough = false;
            }
            else if (TargetSection[i] != nullptr && JumpInfo[i] != nullptr)
            {
                if (!JumpInfo[i]->FallThrough)
                {
                    continue;
                }
                if (JumpInfo[i]->TargetPC == TargetSection[i]->m_EnterPC)
                {
                    continue;
                }
                m_RecompilerOps->LinkJump(*JumpInfo[i], (uint32_t)-1);
                m_RecompilerOps->CompileExit(JumpInfo[i]->JumpPC, JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet, JumpInfo[i]->Reason);
                //FreeSection(TargetSection[i],Section);
            }
        }
    }
    else
    {
        if (!m_Cont.LinkLocation.isValid() && m_Cont.FallThrough == false)
        {
            m_ContinueSection = nullptr;
        }
        if (!m_Jump.LinkLocation.isValid() && m_Jump.FallThrough == false)
        {
            m_JumpSection = nullptr;
        }
        if (m_JumpSection == nullptr && m_ContinueSection == nullptr)
        {
            //FreeSection(TargetSection[0],Section);
        }
    }

    TargetSection[0] = m_ContinueSection;
    TargetSection[1] = m_JumpSection;

    for (i = 0; i < 2; i++)
    {
        if (TargetSection[i] == nullptr)
        {
            continue;
        }
        if (!JumpInfo[i]->FallThrough)
        {
            continue;
        }

        if (TargetSection[i]->m_EnterLabel.isValid())
        {
            JumpInfo[i]->FallThrough = false;
            m_RecompilerOps->LinkJump(*JumpInfo[i], TargetSection[i]->m_SectionID);
            if (JumpInfo[i]->TargetPC <= m_RecompilerOps->GetCurrentPC())
            {
                if (JumpInfo[i]->PermLoop)
                {
                    m_CodeBlock.Log("PermLoop *** 1");
                    m_RecompilerOps->CompileInPermLoop(JumpInfo[i]->RegSet, JumpInfo[i]->TargetPC);
                }
                else
                {
                    m_RecompilerOps->UpdateCounters(JumpInfo[i]->RegSet, true, true);
                    m_CodeBlock.Log("CompileSystemCheck 5");
                    m_RecompilerOps->CompileSystemCheck(JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet);
                }
            }
            else
            {
                m_RecompilerOps->UpdateCounters(JumpInfo[i]->RegSet, false, true);
            }

            JumpInfo[i]->RegSet.SetBlockCycleCount(0);
            m_RecompilerOps->SetRegWorkingSet(JumpInfo[i]->RegSet);
            m_RecompilerOps->SyncRegState(TargetSection[i]->m_RegEnter);
            m_RecompilerOps->JumpToSection(TargetSection[i]);
        }
    }

    for (i = 0; i < 2; i++)
    {
        if (TargetSection[i] == nullptr)
        {
            continue;
        }
        if (TargetSection[i]->m_ParentSection.empty())
        {
            continue;
        }
        for (SECTION_LIST::iterator iter = TargetSection[i]->m_ParentSection.begin(); iter != TargetSection[i]->m_ParentSection.end(); iter++)
        {
            CCodeSection * Parent = *iter;
            if (Parent->m_EnterLabel.isValid())
            {
                continue;
            }
            if (Parent->m_InLoop)
            {
                continue;
            }
            if (JumpInfo[i]->PermLoop)
            {
                m_CodeBlock.Log("PermLoop *** 2");
                m_RecompilerOps->CompileInPermLoop(JumpInfo[i]->RegSet, JumpInfo[i]->TargetPC);
            }
            if (JumpInfo[i]->FallThrough)
            {
                JumpInfo[i]->FallThrough = false;
                m_RecompilerOps->JumpToUnknown(JumpInfo[i]);
            }
        }
    }

    for (i = 0; i < 2; i++)
    {
        if (JumpInfo[i]->FallThrough)
        {
            if (JumpInfo[i]->TargetPC < m_RecompilerOps->GetCurrentPC())
            {
                m_RecompilerOps->UpdateCounters(JumpInfo[i]->RegSet, true, true);
                m_CodeBlock.Log("CompileSystemCheck 7");
                m_RecompilerOps->CompileSystemCheck(JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet);
            }
        }
    }

    m_CodeBlock.Log("====== End of Section %d ======", m_SectionID);

    for (i = 0; i < 2; i++)
    {
        if (JumpInfo[i]->FallThrough && (TargetSection[i] == nullptr || !TargetSection[i]->GenerateNativeCode(m_CodeBlock.NextTest())))
        {
            JumpInfo[i]->FallThrough = false;
            m_RecompilerOps->JumpToUnknown(JumpInfo[i]);
        }
    }

    for (i = 0; i < 2; i++)
    {
        if (!JumpInfo[i]->LinkLocation.isValid())
        {
            continue;
        }
        if (TargetSection[i] == nullptr)
        {
            m_CodeBlock.Log("ExitBlock (from %d):", m_SectionID);
            m_RecompilerOps->LinkJump(*JumpInfo[i], (uint32_t)-1);
            m_RecompilerOps->CompileExit(JumpInfo[i]->JumpPC, JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet, JumpInfo[i]->Reason);
            continue;
        }
        if (JumpInfo[i]->TargetPC != TargetSection[i]->m_EnterPC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (!TargetSection[i]->m_EnterLabel.isValid())
        {
            TargetSection[i]->GenerateNativeCode(m_CodeBlock.NextTest());
        }
        else
        {
            stdstr_f Label("Section_%d (from %d):", TargetSection[i]->m_SectionID, m_SectionID);

            m_CodeBlock.Log(Label.c_str());
            m_RecompilerOps->LinkJump(*JumpInfo[i], (uint32_t)-1);
            m_RecompilerOps->SetRegWorkingSet(JumpInfo[i]->RegSet);
            if (JumpInfo[i]->TargetPC <= JumpInfo[i]->JumpPC)
            {
                m_RecompilerOps->UpdateCounters(m_RecompilerOps->GetRegWorkingSet(), true, true);
                if (JumpInfo[i]->PermLoop)
                {
                    m_CodeBlock.Log("PermLoop *** 3");
                    m_RecompilerOps->CompileInPermLoop(m_RecompilerOps->GetRegWorkingSet(), JumpInfo[i]->TargetPC);
                }
                else
                {
                    m_CodeBlock.Log("CompileSystemCheck 9");
                    m_RecompilerOps->CompileSystemCheck(JumpInfo[i]->TargetPC, m_RecompilerOps->GetRegWorkingSet());
                }
            }
            else
            {
                m_RecompilerOps->UpdateCounters(m_RecompilerOps->GetRegWorkingSet(), false, true);
            }
            m_RecompilerOps->SyncRegState(TargetSection[i]->m_RegEnter);
            m_RecompilerOps->JumpToSection(TargetSection[i]);
        }
    }
}

void CCodeSection::SetDelaySlot()
{
    m_DelaySlot = true;
}

void CCodeSection::SetJumpAddress(uint32_t JumpPC, uint32_t TargetPC, bool PermLoop)
{
    m_Jump.JumpPC = JumpPC;
    m_Jump.TargetPC = TargetPC;
    m_Jump.BranchLabel = stdstr_f("0x%08X", TargetPC);
    m_Jump.PermLoop = PermLoop;
}

void CCodeSection::SetContinueAddress(uint32_t JumpPC, uint32_t TargetPC)
{
    m_Cont.JumpPC = JumpPC;
    m_Cont.TargetPC = TargetPC;
    m_Cont.BranchLabel = stdstr_f("0x%08X", TargetPC);
}

bool CCodeSection::ParentContinue()
{
    if (m_ParentSection.size() > 0)
    {
        for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
        {
            CCodeSection * Parent = *iter;
            if (Parent->m_EnterLabel.isValid())
            {
                continue;
            }
            if (IsAllParentLoops(Parent, true, m_CodeBlock.NextTest()))
            {
                continue;
            }
            return false;
        }
        m_RecompilerOps->SetCurrentSection(this);
        if (!m_RecompilerOps->InheritParentInfo())
        {
            return false;
        }
    }
    return true;
}

bool CCodeSection::GenerateNativeCode(uint32_t Test)
{
    if (m_EnterLabel.isValid())
    {
        if (m_Test == Test)
        {
            return false;
        }
        m_Test = Test;
        if (m_ContinueSection != nullptr && m_ContinueSection->GenerateNativeCode(Test))
        {
            return true;
        }
        if (m_JumpSection != nullptr && m_JumpSection->GenerateNativeCode(Test))
        {
            return true;
        }
        return false;
    }

    if (!ParentContinue())
    {
        return false;
    }
    m_EnterLabel = m_RecompilerOps->Assembler().newLabel();
    m_RecompilerOps->Assembler().bind(m_EnterLabel);
    m_RecompilerOps->SetRegWorkingSet(m_RegEnter);
    m_RecompilerOps->SetCurrentPC(m_EnterPC);
    m_RecompilerOps->SetNextStepType(m_DelaySlot ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);

    if (m_RecompilerOps->GetCurrentPC() < m_CodeBlock.VAddrFirst())
    {
        m_CodeBlock.SetVAddrFirst(m_RecompilerOps->GetCurrentPC());
    }

    uint32_t ContinueSectionPC = m_ContinueSection ? m_ContinueSection->m_EnterPC : (uint32_t)-1;
    const R4300iOpcode & Opcode = m_RecompilerOps->GetOpcode();
    R4300iInstruction Instruction(m_RecompilerOps->GetCurrentPC(), Opcode.Value);
    do
    {
        if (m_RecompilerOps->GetCurrentPC() > m_CodeBlock.VAddrLast())
        {
            m_CodeBlock.SetVAddrLast(m_RecompilerOps->GetCurrentPC());
        }

        if (isDebugging() && HaveExecutionBP() && Instruction.HasDelaySlot() && g_Debugger->ExecutionBP(m_RecompilerOps->GetCurrentPC() + 4))
        {
            m_RecompilerOps->CompileExecuteDelaySlotBP();
            break;
        }

        if (isDebugging() && HaveExecutionBP() && g_Debugger->ExecutionBP(m_RecompilerOps->GetCurrentPC()))
        {
            m_RecompilerOps->CompileExecuteBP();
            break;
        }

        m_RecompilerOps->PreCompileOpcode();

        switch (Opcode.op)
        {
        case R4300i_SPECIAL:
            switch (Opcode.funct)
            {
            case R4300i_SPECIAL_SLL: m_RecompilerOps->SPECIAL_SLL(); break;
            case R4300i_SPECIAL_SRL: m_RecompilerOps->SPECIAL_SRL(); break;
            case R4300i_SPECIAL_SRA: m_RecompilerOps->SPECIAL_SRA(); break;
            case R4300i_SPECIAL_SLLV: m_RecompilerOps->SPECIAL_SLLV(); break;
            case R4300i_SPECIAL_SRLV: m_RecompilerOps->SPECIAL_SRLV(); break;
            case R4300i_SPECIAL_SRAV: m_RecompilerOps->SPECIAL_SRAV(); break;
            case R4300i_SPECIAL_JR: m_RecompilerOps->SPECIAL_JR(); break;
            case R4300i_SPECIAL_JALR: m_RecompilerOps->SPECIAL_JALR(); break;
            case R4300i_SPECIAL_MFLO: m_RecompilerOps->SPECIAL_MFLO(); break;
            case R4300i_SPECIAL_SYSCALL: m_RecompilerOps->SPECIAL_SYSCALL(); break;
            case R4300i_SPECIAL_BREAK: m_RecompilerOps->SPECIAL_BREAK(); break;
            case R4300i_SPECIAL_MTLO: m_RecompilerOps->SPECIAL_MTLO(); break;
            case R4300i_SPECIAL_MFHI: m_RecompilerOps->SPECIAL_MFHI(); break;
            case R4300i_SPECIAL_MTHI: m_RecompilerOps->SPECIAL_MTHI(); break;
            case R4300i_SPECIAL_DSLLV: m_RecompilerOps->SPECIAL_DSLLV(); break;
            case R4300i_SPECIAL_DSRLV: m_RecompilerOps->SPECIAL_DSRLV(); break;
            case R4300i_SPECIAL_DSRAV: m_RecompilerOps->SPECIAL_DSRAV(); break;
            case R4300i_SPECIAL_MULT: m_RecompilerOps->SPECIAL_MULT(); break;
            case R4300i_SPECIAL_DIV: m_RecompilerOps->SPECIAL_DIV(); break;
            case R4300i_SPECIAL_DIVU: m_RecompilerOps->SPECIAL_DIVU(); break;
            case R4300i_SPECIAL_MULTU: m_RecompilerOps->SPECIAL_MULTU(); break;
            case R4300i_SPECIAL_DMULT: m_RecompilerOps->SPECIAL_DMULT(); break;
            case R4300i_SPECIAL_DMULTU: m_RecompilerOps->SPECIAL_DMULTU(); break;
            case R4300i_SPECIAL_DDIV: m_RecompilerOps->SPECIAL_DDIV(); break;
            case R4300i_SPECIAL_DDIVU: m_RecompilerOps->SPECIAL_DDIVU(); break;
            case R4300i_SPECIAL_ADD: m_RecompilerOps->SPECIAL_ADD(); break;
            case R4300i_SPECIAL_ADDU: m_RecompilerOps->SPECIAL_ADDU(); break;
            case R4300i_SPECIAL_SUB: m_RecompilerOps->SPECIAL_SUB(); break;
            case R4300i_SPECIAL_SUBU: m_RecompilerOps->SPECIAL_SUBU(); break;
            case R4300i_SPECIAL_AND: m_RecompilerOps->SPECIAL_AND(); break;
            case R4300i_SPECIAL_OR: m_RecompilerOps->SPECIAL_OR(); break;
            case R4300i_SPECIAL_XOR: m_RecompilerOps->SPECIAL_XOR(); break;
            case R4300i_SPECIAL_NOR: m_RecompilerOps->SPECIAL_NOR(); break;
            case R4300i_SPECIAL_SLT: m_RecompilerOps->SPECIAL_SLT(); break;
            case R4300i_SPECIAL_SLTU: m_RecompilerOps->SPECIAL_SLTU(); break;
            case R4300i_SPECIAL_DADD: m_RecompilerOps->SPECIAL_DADD(); break;
            case R4300i_SPECIAL_DADDU: m_RecompilerOps->SPECIAL_DADDU(); break;
            case R4300i_SPECIAL_DSUB: m_RecompilerOps->SPECIAL_DSUB(); break;
            case R4300i_SPECIAL_DSUBU: m_RecompilerOps->SPECIAL_DSUBU(); break;
            case R4300i_SPECIAL_DSLL: m_RecompilerOps->SPECIAL_DSLL(); break;
            case R4300i_SPECIAL_DSRL: m_RecompilerOps->SPECIAL_DSRL(); break;
            case R4300i_SPECIAL_DSRA: m_RecompilerOps->SPECIAL_DSRA(); break;
            case R4300i_SPECIAL_DSLL32: m_RecompilerOps->SPECIAL_DSLL32(); break;
            case R4300i_SPECIAL_DSRL32: m_RecompilerOps->SPECIAL_DSRL32(); break;
            case R4300i_SPECIAL_DSRA32: m_RecompilerOps->SPECIAL_DSRA32(); break;
            case R4300i_SPECIAL_TEQ: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TEQ); break;
            case R4300i_SPECIAL_TNE: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TNE); break;
            case R4300i_SPECIAL_TGE: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TGE); break;
            case R4300i_SPECIAL_TGEU: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TGEU); break;
            case R4300i_SPECIAL_TLT: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TLT); break;
            case R4300i_SPECIAL_TLTU: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TLTU); break;
            default:
                m_RecompilerOps->UnknownOpcode();
                break;
            }
            break;
        case R4300i_REGIMM:
            switch (Opcode.rt)
            {
            case R4300i_REGIMM_BLTZ: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BLTZ, false); break;
            case R4300i_REGIMM_BGEZ: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BGEZ, false); break;
            case R4300i_REGIMM_BLTZL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_BLTZ, false); break;
            case R4300i_REGIMM_BGEZL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_BGEZ, false); break;
            case R4300i_REGIMM_BLTZAL: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BLTZ, true); break;
            case R4300i_REGIMM_BGEZAL: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BGEZ, true); break;
            case R4300i_REGIMM_TEQI: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TEQI); break;
            case R4300i_REGIMM_TNEI: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TNEI); break;
            case R4300i_REGIMM_TGEI: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TGEI); break;
            case R4300i_REGIMM_TGEIU: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TGEIU); break;
            case R4300i_REGIMM_TLTI: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TLTI); break;
            case R4300i_REGIMM_TLTIU: m_RecompilerOps->Compile_TrapCompare(RecompilerTrapCompare_TLTIU); break;
            default:
                m_RecompilerOps->UnknownOpcode();
                break;
            }
            break;
        case R4300i_BEQ: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BEQ, false); break;
        case R4300i_BNE: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BNE, false); break;
        case R4300i_BGTZ: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BGTZ, false); break;
        case R4300i_BLEZ: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_BLEZ, false); break;
        case R4300i_J: m_RecompilerOps->J(); break;
        case R4300i_JAL: m_RecompilerOps->JAL(); break;
        case R4300i_ADDI: m_RecompilerOps->ADDI(); break;
        case R4300i_ADDIU: m_RecompilerOps->ADDIU(); break;
        case R4300i_SLTI: m_RecompilerOps->SLTI(); break;
        case R4300i_SLTIU: m_RecompilerOps->SLTIU(); break;
        case R4300i_ANDI: m_RecompilerOps->ANDI(); break;
        case R4300i_ORI: m_RecompilerOps->ORI(); break;
        case R4300i_XORI: m_RecompilerOps->XORI(); break;
        case R4300i_LUI: m_RecompilerOps->LUI(); break;
        case R4300i_CP0:
            switch (Opcode.rs)
            {
            case R4300i_COP0_MF: m_RecompilerOps->COP0_MF(); break;
            case R4300i_COP0_DMF: m_RecompilerOps->COP0_DMF(); break;
            case R4300i_COP0_MT: m_RecompilerOps->COP0_MT(); break;
            case R4300i_COP0_DMT: m_RecompilerOps->COP0_DMT(); break;
            default:
                if ((Opcode.rs & 0x10) != 0)
                {
                    switch (Opcode.funct)
                    {
                    case R4300i_COP0_CO_TLBR: m_RecompilerOps->COP0_CO_TLBR(); break;
                    case R4300i_COP0_CO_TLBWI: m_RecompilerOps->COP0_CO_TLBWI(); break;
                    case R4300i_COP0_CO_TLBWR: m_RecompilerOps->COP0_CO_TLBWR(); break;
                    case R4300i_COP0_CO_TLBP: m_RecompilerOps->COP0_CO_TLBP(); break;
                    case R4300i_COP0_CO_ERET: m_RecompilerOps->COP0_CO_ERET(); break;
                    default: m_RecompilerOps->UnknownOpcode(); break;
                    }
                }
                else
                {
                    m_RecompilerOps->UnknownOpcode();
                }
            }
            break;
        case R4300i_CP1:
            switch (Opcode.rs)
            {
            case R4300i_COP1_MF: m_RecompilerOps->COP1_MF(); break;
            case R4300i_COP1_DMF: m_RecompilerOps->COP1_DMF(); break;
            case R4300i_COP1_CF: m_RecompilerOps->COP1_CF(); break;
            case R4300i_COP1_MT: m_RecompilerOps->COP1_MT(); break;
            case R4300i_COP1_DMT: m_RecompilerOps->COP1_DMT(); break;
            case R4300i_COP1_CT: m_RecompilerOps->COP1_CT(); break;
            case R4300i_COP1_BC:
                switch (Opcode.ft)
                {
                case R4300i_COP1_BC_BCF: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_COP1BCF, false); break;
                case R4300i_COP1_BC_BCT: m_RecompilerOps->Compile_Branch(RecompilerBranchCompare_COP1BCT, false); break;
                case R4300i_COP1_BC_BCFL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_COP1BCF, false); break;
                case R4300i_COP1_BC_BCTL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_COP1BCT, false); break;
                default:
                    m_RecompilerOps->UnknownOpcode();
                    break;
                }
                break;
            case R4300i_COP1_S:
                switch (Opcode.funct)
                {
                case R4300i_COP1_FUNCT_ADD: m_RecompilerOps->COP1_S_ADD(); break;
                case R4300i_COP1_FUNCT_SUB: m_RecompilerOps->COP1_S_SUB(); break;
                case R4300i_COP1_FUNCT_MUL: m_RecompilerOps->COP1_S_MUL(); break;
                case R4300i_COP1_FUNCT_DIV: m_RecompilerOps->COP1_S_DIV(); break;
                case R4300i_COP1_FUNCT_ABS: m_RecompilerOps->COP1_S_ABS(); break;
                case R4300i_COP1_FUNCT_NEG: m_RecompilerOps->COP1_S_NEG(); break;
                case R4300i_COP1_FUNCT_SQRT: m_RecompilerOps->COP1_S_SQRT(); break;
                case R4300i_COP1_FUNCT_MOV: m_RecompilerOps->COP1_S_MOV(); break;
                case R4300i_COP1_FUNCT_ROUND_L: m_RecompilerOps->COP1_S_ROUND_L(); break;
                case R4300i_COP1_FUNCT_TRUNC_L: m_RecompilerOps->COP1_S_TRUNC_L(); break;
                case R4300i_COP1_FUNCT_CEIL_L: m_RecompilerOps->COP1_S_CEIL_L(); break;
                case R4300i_COP1_FUNCT_FLOOR_L: m_RecompilerOps->COP1_S_FLOOR_L(); break;
                case R4300i_COP1_FUNCT_ROUND_W: m_RecompilerOps->COP1_S_ROUND_W(); break;
                case R4300i_COP1_FUNCT_TRUNC_W: m_RecompilerOps->COP1_S_TRUNC_W(); break;
                case R4300i_COP1_FUNCT_CEIL_W: m_RecompilerOps->COP1_S_CEIL_W(); break;
                case R4300i_COP1_FUNCT_FLOOR_W: m_RecompilerOps->COP1_S_FLOOR_W(); break;
                case R4300i_COP1_FUNCT_CVT_D: m_RecompilerOps->COP1_S_CVT_D(); break;
                case R4300i_COP1_FUNCT_CVT_W: m_RecompilerOps->COP1_S_CVT_W(); break;
                case R4300i_COP1_FUNCT_CVT_L: m_RecompilerOps->COP1_S_CVT_L(); break;
                case R4300i_COP1_FUNCT_C_F:
                case R4300i_COP1_FUNCT_C_UN:
                case R4300i_COP1_FUNCT_C_EQ:
                case R4300i_COP1_FUNCT_C_UEQ:
                case R4300i_COP1_FUNCT_C_OLT:
                case R4300i_COP1_FUNCT_C_ULT:
                case R4300i_COP1_FUNCT_C_OLE:
                case R4300i_COP1_FUNCT_C_ULE:
                case R4300i_COP1_FUNCT_C_SF:
                case R4300i_COP1_FUNCT_C_NGLE:
                case R4300i_COP1_FUNCT_C_SEQ:
                case R4300i_COP1_FUNCT_C_NGL:
                case R4300i_COP1_FUNCT_C_LT:
                case R4300i_COP1_FUNCT_C_NGE:
                case R4300i_COP1_FUNCT_C_LE:
                case R4300i_COP1_FUNCT_C_NGT:
                    m_RecompilerOps->COP1_S_CMP();
                    break;
                default:
                    m_RecompilerOps->UnknownOpcode();
                    break;
                }
                break;
            case R4300i_COP1_D:
                switch (Opcode.funct)
                {
                case R4300i_COP1_FUNCT_ADD: m_RecompilerOps->COP1_D_ADD(); break;
                case R4300i_COP1_FUNCT_SUB: m_RecompilerOps->COP1_D_SUB(); break;
                case R4300i_COP1_FUNCT_MUL: m_RecompilerOps->COP1_D_MUL(); break;
                case R4300i_COP1_FUNCT_DIV: m_RecompilerOps->COP1_D_DIV(); break;
                case R4300i_COP1_FUNCT_ABS: m_RecompilerOps->COP1_D_ABS(); break;
                case R4300i_COP1_FUNCT_NEG: m_RecompilerOps->COP1_D_NEG(); break;
                case R4300i_COP1_FUNCT_SQRT: m_RecompilerOps->COP1_D_SQRT(); break;
                case R4300i_COP1_FUNCT_MOV: m_RecompilerOps->COP1_D_MOV(); break;
                case R4300i_COP1_FUNCT_ROUND_L: m_RecompilerOps->COP1_D_ROUND_L(); break;
                case R4300i_COP1_FUNCT_TRUNC_L: m_RecompilerOps->COP1_D_TRUNC_L(); break;
                case R4300i_COP1_FUNCT_CEIL_L: m_RecompilerOps->COP1_D_CEIL_L(); break;
                case R4300i_COP1_FUNCT_FLOOR_L: m_RecompilerOps->COP1_D_FLOOR_L(); break;
                case R4300i_COP1_FUNCT_ROUND_W: m_RecompilerOps->COP1_D_ROUND_W(); break;
                case R4300i_COP1_FUNCT_TRUNC_W: m_RecompilerOps->COP1_D_TRUNC_W(); break;
                case R4300i_COP1_FUNCT_CEIL_W: m_RecompilerOps->COP1_D_CEIL_W(); break;
                case R4300i_COP1_FUNCT_FLOOR_W: m_RecompilerOps->COP1_D_FLOOR_W(); break;
                case R4300i_COP1_FUNCT_CVT_S: m_RecompilerOps->COP1_D_CVT_S(); break;
                case R4300i_COP1_FUNCT_CVT_W: m_RecompilerOps->COP1_D_CVT_W(); break;
                case R4300i_COP1_FUNCT_CVT_L: m_RecompilerOps->COP1_D_CVT_L(); break;
                case R4300i_COP1_FUNCT_C_F:
                case R4300i_COP1_FUNCT_C_UN:
                case R4300i_COP1_FUNCT_C_EQ:
                case R4300i_COP1_FUNCT_C_UEQ:
                case R4300i_COP1_FUNCT_C_OLT:
                case R4300i_COP1_FUNCT_C_ULT:
                case R4300i_COP1_FUNCT_C_OLE:
                case R4300i_COP1_FUNCT_C_ULE:
                case R4300i_COP1_FUNCT_C_SF:
                case R4300i_COP1_FUNCT_C_NGLE:
                case R4300i_COP1_FUNCT_C_SEQ:
                case R4300i_COP1_FUNCT_C_NGL:
                case R4300i_COP1_FUNCT_C_LT:
                case R4300i_COP1_FUNCT_C_NGE:
                case R4300i_COP1_FUNCT_C_LE:
                case R4300i_COP1_FUNCT_C_NGT:
                    m_RecompilerOps->COP1_D_CMP();
                    break;
                default:
                    m_RecompilerOps->UnknownOpcode();
                    break;
                }
                break;
            case R4300i_COP1_W:
                switch (Opcode.funct)
                {
                case R4300i_COP1_FUNCT_CVT_S: m_RecompilerOps->COP1_W_CVT_S(); break;
                case R4300i_COP1_FUNCT_CVT_D: m_RecompilerOps->COP1_W_CVT_D(); break;
                default:
                    m_RecompilerOps->UnknownOpcode();
                    break;
                }
                break;
            case R4300i_COP1_L:
                switch (Opcode.funct)
                {
                case R4300i_COP1_FUNCT_CVT_S: m_RecompilerOps->COP1_L_CVT_S(); break;
                case R4300i_COP1_FUNCT_CVT_D: m_RecompilerOps->COP1_L_CVT_D(); break;
                default:
                    m_RecompilerOps->UnknownOpcode();
                    break;
                }
                break;
            default:
                m_RecompilerOps->UnknownOpcode();
                break;
            }
            break;
        case R4300i_BEQL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_BEQ, false); break;
        case R4300i_BNEL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_BNE, false); break;
        case R4300i_BGTZL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_BGTZ, false); break;
        case R4300i_BLEZL: m_RecompilerOps->Compile_BranchLikely(RecompilerBranchCompare_BLEZ, false); break;
        case R4300i_DADDI: m_RecompilerOps->DADDI(); break;
        case R4300i_DADDIU: m_RecompilerOps->DADDIU(); break;
        case R4300i_LDL: m_RecompilerOps->LDL(); break;
        case R4300i_LDR: m_RecompilerOps->LDR(); break;
        case R4300i_RESERVED31: m_RecompilerOps->RESERVED31(); break;
        case R4300i_LB: m_RecompilerOps->LB(); break;
        case R4300i_LH: m_RecompilerOps->LH(); break;
        case R4300i_LWL: m_RecompilerOps->LWL(); break;
        case R4300i_LW: m_RecompilerOps->LW(); break;
        case R4300i_LBU: m_RecompilerOps->LBU(); break;
        case R4300i_LHU: m_RecompilerOps->LHU(); break;
        case R4300i_LWR: m_RecompilerOps->LWR(); break;
        case R4300i_LWU: m_RecompilerOps->LWU(); break;
        case R4300i_SB: m_RecompilerOps->SB(); break;
        case R4300i_SH: m_RecompilerOps->SH(); break;
        case R4300i_SWL: m_RecompilerOps->SWL(); break;
        case R4300i_SW: m_RecompilerOps->SW(); break;
        case R4300i_SWR: m_RecompilerOps->SWR(); break;
        case R4300i_SDL: m_RecompilerOps->SDL(); break;
        case R4300i_SDR: m_RecompilerOps->SDR(); break;
        case R4300i_CACHE: m_RecompilerOps->CACHE(); break;
        case R4300i_LL: m_RecompilerOps->LL(); break;
        case R4300i_LWC1: m_RecompilerOps->LWC1(); break;
        case R4300i_LDC1: m_RecompilerOps->LDC1(); break;
        case R4300i_SC: m_RecompilerOps->SC(); break;
        case R4300i_LD: m_RecompilerOps->LD(); break;
        case R4300i_SWC1: m_RecompilerOps->SWC1(); break;
        case R4300i_SDC1: m_RecompilerOps->SDC1(); break;
        case R4300i_SD: m_RecompilerOps->SD(); break;
        default:
            m_RecompilerOps->UnknownOpcode();
            break;
        }

        m_RecompilerOps->PostCompileOpcode();

        if ((m_RecompilerOps->GetCurrentPC() & 0xFFC) == 0xFFC)
        {
            if (m_RecompilerOps->GetNextStepType() == PIPELINE_STAGE_DO_DELAY_SLOT)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_RecompilerOps->GetNextStepType() == PIPELINE_STAGE_NORMAL)
            {
                if (m_DelaySlot)
                {
                    m_RecompilerOps->CompileExit(m_RecompilerOps->GetCurrentPC(), m_Jump.TargetPC, m_RecompilerOps->GetRegWorkingSet(), ExitReason_Normal);
                }
                else
                {
                    m_RecompilerOps->CompileExit(m_RecompilerOps->GetCurrentPC(), m_RecompilerOps->GetCurrentPC() + 4, m_RecompilerOps->GetRegWorkingSet(), ExitReason_Normal);
                }
                m_RecompilerOps->SetNextStepType(PIPELINE_STAGE_END_BLOCK);
            }
        }

        switch (m_RecompilerOps->GetNextStepType())
        {
        case PIPELINE_STAGE_NORMAL:
            m_RecompilerOps->SetCurrentPC(m_RecompilerOps->GetCurrentPC() + 4);
            break;
        case PIPELINE_STAGE_DO_DELAY_SLOT:
            m_RecompilerOps->SetNextStepType(PIPELINE_STAGE_DELAY_SLOT);
            m_RecompilerOps->SetCurrentPC(m_RecompilerOps->GetCurrentPC() + 4);
            break;
        case PIPELINE_STAGE_DELAY_SLOT:
            m_RecompilerOps->SetNextStepType(PIPELINE_STAGE_DELAY_SLOT_DONE);
            m_RecompilerOps->SetCurrentPC(m_RecompilerOps->GetCurrentPC() - 4);
            break;
        case PIPELINE_STAGE_JUMP:
        case PIPELINE_STAGE_END_BLOCK:
            // Do nothing, the block will end
            break;
        default:
            m_CodeBlock.Log("m_RecompilerOps->GetNextStepType() = %d", m_RecompilerOps->GetNextStepType());
            g_Notify->BreakPoint(__FILE__, __LINE__);
            break;
        }

        if (m_DelaySlot)
        {
            if ((m_RecompilerOps->GetCurrentPC() & 0xFFC) != 0xFFC && m_Jump.JumpPC != (uint32_t)-1)
            {
                m_RecompilerOps->SetCurrentPC(m_Jump.JumpPC);
                m_Jump.RegSet = m_RecompilerOps->GetRegWorkingSet();
                m_Jump.FallThrough = true;
                GenerateSectionLinkage();
            }
            else
            {
                m_RecompilerOps->CompileExit(m_Jump.JumpPC, m_Jump.TargetPC, m_RecompilerOps->GetRegWorkingSet(), ExitReason_Normal);
            }
            m_RecompilerOps->SetNextStepType(PIPELINE_STAGE_END_BLOCK);
        }
        else if (m_RecompilerOps->GetNextStepType() != PIPELINE_STAGE_END_BLOCK && m_RecompilerOps->GetCurrentPC() == ContinueSectionPC)
        {
            if (m_RecompilerOps->GetNextStepType() != PIPELINE_STAGE_NORMAL)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RecompilerOps->SetCurrentPC(m_RecompilerOps->GetCurrentPC() - 4);
            m_Cont.RegSet = m_RecompilerOps->GetRegWorkingSet();
            m_Cont.FallThrough = true;
            m_Cont.JumpPC = m_RecompilerOps->GetCurrentPC();
            GenerateSectionLinkage();
            m_RecompilerOps->SetNextStepType(PIPELINE_STAGE_END_BLOCK);
        }
    } while (m_RecompilerOps->GetNextStepType() != PIPELINE_STAGE_END_BLOCK);
    return true;
}

void CCodeSection::AddParent(CCodeSection * Parent)
{
    if (Parent == nullptr)
    {
        m_RecompilerOps->SetRegWorkingSet(m_RegEnter);
        return;
    }

    // Check to see if we already have the parent in the list
    for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
    {
        if (*iter == Parent)
        {
            return;
        }
    }
    m_ParentSection.push_back(Parent);

    if (m_ParentSection.size() == 1)
    {
        if (Parent->m_ContinueSection == this)
        {
            m_RegEnter = Parent->m_Cont.RegSet;
        }
        else if (Parent->m_JumpSection == this)
        {
            m_RegEnter = Parent->m_Jump.RegSet;
        }
        else
        {
            g_Notify->DisplayError("How are these sections joined?");
        }
    }
    else
    {
        if (Parent->m_ContinueSection == this)
        {
            TestRegConstantStates(Parent->m_Cont.RegSet, m_RegEnter);
        }
        if (Parent->m_JumpSection == this)
        {
            TestRegConstantStates(Parent->m_Jump.RegSet, m_RegEnter);
        }
    }
    m_RecompilerOps->SetRegWorkingSet(m_RegEnter);
}

void CCodeSection::SwitchParent(CCodeSection * OldParent, CCodeSection * NewParent)
{
    bool bFoundOldParent = false;
    for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
    {
        if (*iter != OldParent)
        {
            continue;
        }
        bFoundOldParent = true;
        m_ParentSection.erase(iter);
        break;
    }

    if (!bFoundOldParent)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_ParentSection.push_back(NewParent);
}

void CCodeSection::TestRegConstantStates(CRegInfo & Base, CRegInfo & Reg)
{
    for (int i = 0; i < 32; i++)
    {
        if (Reg.GetMipsRegState(i) != Base.GetMipsRegState(i))
        {
            Reg.SetMipsRegState(i, CRegInfo::STATE_UNKNOWN);
        }
        if (Reg.IsConst(i))
        {
            if (Reg.Is32Bit(i))
            {
                if (Reg.GetMipsRegLo(i) != Base.GetMipsRegLo(i))
                {
                    Reg.SetMipsRegState(i, CRegInfo::STATE_UNKNOWN);
                }
            }
            else
            {
                if (Reg.GetMipsReg(i) != Base.GetMipsReg(i))
                {
                    Reg.SetMipsRegState(i, CRegInfo::STATE_UNKNOWN);
                }
            }
        }
    }
}

void CCodeSection::DetermineLoop(uint32_t Test, uint32_t Test2, uint32_t TestID)
{
    if (m_SectionID == TestID)
    {
        if (m_Test2 != Test2)
        {
            m_Test2 = Test2;
            if (m_ContinueSection)
            {
                m_ContinueSection->DetermineLoop(Test, Test2, TestID);
            }
            if (m_JumpSection)
            {
                m_JumpSection->DetermineLoop(Test, Test2, TestID);
            }

            if (m_Test != Test)
            {
                m_Test = Test;
                if (m_ContinueSection != nullptr)
                {
                    m_ContinueSection->DetermineLoop(Test, m_CodeBlock.NextTest(), m_ContinueSection->m_SectionID);
                }
                if (m_JumpSection != nullptr)
                {
                    m_JumpSection->DetermineLoop(Test, m_CodeBlock.NextTest(), m_JumpSection->m_SectionID);
                }
            }
        }
        else
        {
            m_InLoop = true;
        }
    }
    else
    {
        if (m_Test2 != Test2)
        {
            m_Test2 = Test2;
            if (m_ContinueSection)
            {
                m_ContinueSection->DetermineLoop(Test, Test2, TestID);
            }
            if (m_JumpSection)
            {
                m_JumpSection->DetermineLoop(Test, Test2, TestID);
            }
        }
    }
}

CCodeSection * CCodeSection::ExistingSection(uint32_t Addr, uint32_t Test)
{
    if (m_EnterPC == Addr && m_LinkAllowed)
    {
        return this;
    }
    if (m_Test == Test)
    {
        return nullptr;
    }
    m_Test = Test;

    CCodeSection * Section = m_JumpSection ? m_JumpSection->ExistingSection(Addr, Test) : nullptr;
    if (Section != nullptr)
    {
        return Section;
    }
    Section = m_ContinueSection ? m_ContinueSection->ExistingSection(Addr, Test) : nullptr;
    if (Section != nullptr)
    {
        return Section;
    }

    return nullptr;
}

bool CCodeSection::SectionAccessible(uint32_t SectionId, uint32_t Test)
{
    if (m_SectionID == SectionId)
    {
        return true;
    }

    if (m_Test == Test)
    {
        return false;
    }
    m_Test = Test;

    if (m_ContinueSection && m_ContinueSection->SectionAccessible(SectionId, Test))
    {
        return true;
    }
    return m_JumpSection && m_JumpSection->SectionAccessible(SectionId, Test);
}

void CCodeSection::UnlinkParent(CCodeSection * Parent, bool ContinueSection)
{
    m_CodeBlock.Log("%s: Section %d Parent: %d ContinueSection = %s", __FUNCTION__, m_SectionID, Parent->m_SectionID, ContinueSection ? "Yes" : "No");
    if (Parent->m_ContinueSection == this && Parent->m_JumpSection == this)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    SECTION_LIST::iterator ParentIter = m_ParentSection.begin();
    while (ParentIter != m_ParentSection.end())
    {
        if (*ParentIter == Parent && (Parent->m_ContinueSection != this || Parent->m_JumpSection != this))
        {
            m_ParentSection.erase(ParentIter);
            ParentIter = m_ParentSection.begin();
        }
        else
        {
            ParentIter++;
        }
    }

    if (ContinueSection && Parent->m_ContinueSection == this)
    {
        Parent->m_ContinueSection = nullptr;
    }

    if (!ContinueSection && Parent->m_JumpSection == this)
    {
        Parent->m_JumpSection = nullptr;
    }

    bool bRemove = false;
    if (m_ParentSection.size() > 0)
    {
        if (!m_CodeBlock.SectionAccessible(m_SectionID))
        {
            for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
            {
                CCodeSection * CodeSection = *iter;
                if (CodeSection->m_ContinueSection == this)
                {
                    CodeSection->m_ContinueSection = nullptr;
                }

                if (CodeSection->m_JumpSection == this)
                {
                    CodeSection->m_JumpSection = nullptr;
                }
            }
            bRemove = true;
        }
    }
    else
    {
        bRemove = true;
    }
    if (bRemove)
    {
        if (m_JumpSection != nullptr)
        {
            m_JumpSection->UnlinkParent(this, false);
        }
        if (m_ContinueSection != nullptr)
        {
            m_ContinueSection->UnlinkParent(this, true);
        }
    }
}

bool CCodeSection::IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, uint32_t Test)
{
    if (IgnoreIfCompiled && Parent->m_EnterLabel.isValid())
    {
        return true;
    }
    if (!m_InLoop)
    {
        return false;
    }
    if (!Parent->m_InLoop)
    {
        return false;
    }
    if (Parent->m_ParentSection.empty())
    {
        return false;
    }
    if (this == Parent)
    {
        return true;
    }
    if (Parent->m_Test == Test)
    {
        return true;
    }
    Parent->m_Test = Test;

    for (SECTION_LIST::iterator iter = Parent->m_ParentSection.begin(); iter != Parent->m_ParentSection.end(); iter++)
    {
        CCodeSection * ParentSection = *iter;
        if (!IsAllParentLoops(ParentSection, IgnoreIfCompiled, Test))
        {
            return false;
        }
    }
    return true;
}

bool CCodeSection::DisplaySectionInformation(uint32_t ID, uint32_t Test)
{
    if (!CDebugSettings::bRecordRecompilerAsm())
    {
        return false;
    }
    if (m_Test == Test)
    {
        return false;
    }
    m_Test = Test;
    if (m_SectionID != ID)
    {
        if (m_ContinueSection != nullptr && m_ContinueSection->DisplaySectionInformation(ID, Test))
        {
            return true;
        }
        if (m_JumpSection != nullptr && m_JumpSection->DisplaySectionInformation(ID, Test))
        {
            return true;
        }
        return false;
    }
    DisplaySectionInformation();
    return true;
}

void CCodeSection::DisplaySectionInformation()
{
    if (m_SectionID == 0)
    {
        return;
    }

    m_CodeBlock.Log("====== Section %d ======", m_SectionID);
    m_CodeBlock.Log("Start PC: 0x%X", m_EnterPC);
    if (g_System->bLinkBlocks())
    {
        m_CodeBlock.Log("End PC: 0x%X", m_EndPC);
    }
    if (g_System->bLinkBlocks() && !m_ParentSection.empty())
    {
        stdstr ParentList;
        for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
        {
            CCodeSection * Parent = *iter;
            if (!ParentList.empty())
            {
                ParentList += ", ";
            }
            ParentList += stdstr_f("%d", Parent->m_SectionID);
        }
        m_CodeBlock.Log("Number of parents: %d (%s)", m_ParentSection.size(), ParentList.c_str());
    }

    if (g_System->bLinkBlocks())
    {
        m_CodeBlock.Log("Jump address: 0x%08X", m_Jump.JumpPC);
        m_CodeBlock.Log("Jump target address: 0x%08X", m_Jump.TargetPC);
        if (m_JumpSection != nullptr)
        {
            m_CodeBlock.Log("Jump section: %d", m_JumpSection->m_SectionID);
        }
        else
        {
            m_CodeBlock.Log("Jump section: None");
        }
        m_CodeBlock.Log("Continue address: 0x%08X", m_Cont.JumpPC);
        m_CodeBlock.Log("Continue target address: 0x%08X", m_Cont.TargetPC);
        if (m_ContinueSection != nullptr)
        {
            m_CodeBlock.Log("Continue section: %d", m_ContinueSection->m_SectionID);
        }
        else
        {
            m_CodeBlock.Log("Continue section: None");
        }
        m_CodeBlock.Log("In loop: %s", m_InLoop ? "Yes" : "No");
    }
    m_CodeBlock.Log("=======================");
}