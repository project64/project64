#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/x64-86/x64RecompilerOps.h>
#include <Project64-core/N64System/SystemGlobals.h>
uint32_t CX64RecompilerOps::m_TempValue32 = 0;

CX64RecompilerOps::CX64RecompilerOps(CN64System & System, CCodeBlock & CodeBlock) :
    CRecompilerOpsBase(System, CodeBlock),
    m_Recompiler(System.m_Recomp),
    m_Rom(*g_Rom),
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler),
    m_MMU(System.m_MMU_VM),
    m_PipelineStage(PIPELINE_STAGE_NORMAL),
    m_EffectDelaySlot(false),
    m_CompilePC(m_Instruction.Address32()),
    m_ColdEntryOffset(0),
    m_WarmEntryOffset(0),
    m_ExitLabelCount(0)
{
}

CX64RecompilerOps::~CX64RecompilerOps()
{
}

void CX64RecompilerOps::Compile_TrapCompare(RecompilerTrapCompare /*CompareType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileCop1Test()
{
    if (m_RegWorkingSet.GetFpuBeenUsed())
    {
        return;
    }

    m_Assembler.finit();
    m_Assembler.TestVariable(&g_Reg->STATUS_REGISTER, "STATUS_REGISTER", STATUS_CU1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_COP1Unuseable, &CX64Ops::JeLabel);
    m_RegWorkingSet.SetFpuBeenUsed(true);
}

void CX64RecompilerOps::Compile_BranchCompare(RecompilerBranchCompare CompareType)
{
    switch (CompareType)
    {
    case RecompilerBranchCompare_BEQ: BEQ_Compare(); break;
    case RecompilerBranchCompare_BNE: BNE_Compare(); break;
    case RecompilerBranchCompare_BLTZ: BLTZ_Compare(); break;
    case RecompilerBranchCompare_BLEZ: BLEZ_Compare(); break;
    case RecompilerBranchCompare_BGTZ: BGTZ_Compare(); break;
    case RecompilerBranchCompare_BGEZ: BGEZ_Compare(); break;
    case RecompilerBranchCompare_COP1BCF: COP1_BCF_Compare(); break;
    case RecompilerBranchCompare_COP1BCT: COP1_BCT_Compare(); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::Compile_Branch(RecompilerBranchCompare CompareType, bool Link)
{
    if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT)
    {
        Compile_BranchCompare(CompareType);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if (CompareType == RecompilerBranchCompare_COP1BCF || CompareType == RecompilerBranchCompare_COP1BCT)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8 && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        if ((m_CompilePC & 0xFFC) != 0xFFC)
        {
            R4300iOpcode DelaySlot;
            m_EffectDelaySlot = g_MMU->MemoryValue32((uint32_t)(m_CompilePC + 4), DelaySlot.Value) && R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value);
        }
        else
        {
            m_EffectDelaySlot = true;
        }
        m_Section->m_Jump.JumpPC = (uint32_t)m_CompilePC;
        m_Section->m_Jump.TargetPC = (uint32_t)(m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4);
        if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT)
        {
            m_Section->m_Jump.TargetPC += 4;
            m_EffectDelaySlot = true;
        }
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Exit_%X_jump_%X", m_Section->m_EnterPC, m_Section->m_Jump.TargetPC);
        }
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Jump.DoneDelaySlot = false;
        m_Section->m_Cont.JumpPC = (uint32_t)m_CompilePC;
        m_Section->m_Cont.TargetPC = (uint32_t)(m_CompilePC + 8);
        if (m_Section->m_ContinueSection != nullptr)
        {
            m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel = stdstr_f("Exit_%X_continue_%X", m_Section->m_EnterPC, m_Section->m_Cont.TargetPC);
        }
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.DoneDelaySlot = false;
        if (m_Section->m_Jump.TargetPC < m_Section->m_Cont.TargetPC)
        {
            m_Section->m_Cont.FallThrough = false;
            m_Section->m_Jump.FallThrough = true;
        }
        else
        {
            m_Section->m_Cont.FallThrough = true;
            m_Section->m_Jump.FallThrough = false;
        }

        if (Link)
        {
            R4300iInstruction Instruction(m_CompilePC, m_Opcode.Value);
            uint32_t ReadReg1, ReadReg2;
            Instruction.ReadsGPR(ReadReg1, ReadReg2);

            if (ReadReg1 != 31 && ReadReg2 != 31)
            {
                m_RegWorkingSet.UnMap_GPR(31, false);
                m_RegWorkingSet.SetMipsRegLo(31, (uint32_t)m_CompilePC + 8);
                m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_Section->m_Cont.LinkAddress = (uint32_t)(m_CompilePC + 8);
                m_Section->m_Jump.LinkAddress = (uint32_t)(m_CompilePC + 8);
            }
        }
        if (m_EffectDelaySlot)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (m_EffectDelaySlot)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            if (m_Section->m_Jump.TargetPC != m_Section->m_Cont.TargetPC)
            {
                Compile_BranchCompare(CompareType);
                m_RegWorkingSet.ResetRegisterProtection();
                m_Section->m_Cont.RegSet = m_RegWorkingSet;
                m_Section->m_Jump.RegSet = m_RegWorkingSet;
                if (m_Section->m_Cont.LinkAddress != (uint32_t)-1)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                if (m_Section->m_Jump.LinkAddress != (uint32_t)-1)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        m_Section->GenerateSectionLinkage();
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::Compile_BranchLikely(RecompilerBranchCompare CompareType, bool Link)
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if (CompareType == RecompilerBranchCompare_COP1BCF || CompareType == RecompilerBranchCompare_COP1BCT)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (g_GameSettings.blockLinkingMode != BlockLinking_Eager || (m_CompilePC & 0xFFC) == 0xFFC)
        {
            m_Section->m_Jump.JumpPC = (uint32_t)m_CompilePC;
            m_Section->m_Jump.TargetPC = (uint32_t)(m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4);
            m_Section->m_Cont.JumpPC = (uint32_t)m_CompilePC;
            m_Section->m_Cont.TargetPC = (uint32_t)(m_CompilePC + 8);
        }
        else
        {
            if (m_Section->m_Jump.JumpPC != (uint32_t)m_CompilePC)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_Section->m_Cont.JumpPC != (uint32_t)m_CompilePC)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_Section->m_Cont.TargetPC != (uint32_t)(m_CompilePC + 8))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }

        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", m_Section->m_JumpSection->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }

        if (m_Section->m_ContinueSection != nullptr)
        {
            m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", m_Section->m_ContinueSection->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel = "ExitBlock";
        }

        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();
        if (Link)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        Compile_BranchCompare(CompareType);
        m_RegWorkingSet.ResetRegisterProtection();

        m_Section->m_Cont.RegSet = m_RegWorkingSet;
        m_Section->m_Cont.RegSet.SetBlockCycleCount(m_Section->m_Cont.RegSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
        if (m_Section->m_Cont.LinkAddress != (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
        }

        if (g_GameSettings.blockLinkingMode == BlockLinking_Eager)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->m_Jump.RegSet.SetBlockCycleCount(m_Section->m_Jump.RegSet.GetBlockCycleCount());
        if (m_Section->m_Jump.LinkAddress != (uint32_t)-1)
        {
            m_Section->m_Jump.RegSet.UnMap_GPR(31, false);
            m_Section->m_Jump.RegSet.SetMipsRegLo(31, m_Section->m_Jump.LinkAddress);
            m_Section->m_Jump.RegSet.SetMipsRegState(31, CRegBase::STATE_CONST_32_SIGN);
            m_Section->m_Jump.LinkAddress = (uint32_t)-1;
        }
        m_Section->GenerateSectionLinkage();
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (g_DebugSettings.haveDebugger)
    {
        g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_PipelineStage).c_str());
    }
}

void CX64RecompilerOps::BNE_Compare()
{
    asmjit::Label Jump;

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                m_Section->m_Jump.FallThrough = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) != m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
                m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(ConstReg) || m_RegWorkingSet.Is64Bit(MappedReg))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(MappedReg).r32(), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rs) || m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!g_GameSettings.core32Bit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                m_Assembler.CmpConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
            }
            else
            {
                m_Assembler.CmpRegToVariable(m_RegWorkingSet.GetMipsRegMap(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
        }
        if (m_Section->m_Cont.FallThrough)
        {
            if (g_GameSettings.core32Bit)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);

            if (Jump.isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump);
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::BEQ_Compare()
{
    asmjit::Label Jump;

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                m_Section->m_Jump.FallThrough = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) == m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
                m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(ConstReg) || m_RegWorkingSet.Is64Bit(MappedReg))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(MappedReg).r32(), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rs) || m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!g_GameSettings.core32Bit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            m_Assembler.CmpConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
        }
        else
        {
            m_Assembler.CmpRegToVariable(m_RegWorkingSet.GetMipsRegMap(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            if (Jump.isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::BGTZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BLEZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BLTZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BGEZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            m_Section->m_Jump.FallThrough = m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) >= 0;
            m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        }
        else
        {
            m_Section->m_Jump.FallThrough = true;
            m_Section->m_Cont.FallThrough = false;
        }
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::COP1_BCF_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_BCT_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::J()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::JAL()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        m_RegWorkingSet.Map_GPR_32bit(31, true, -1);
        m_Assembler.MoveVariableToX64reg(m_RegWorkingSet.GetMipsRegMap(31), &m_Reg.m_PROGRAM_COUNTER, "_PROGRAM_COUNTER", false);
        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMap(31).r32(), 0xF0000000);
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMap(31).r32(), (m_CompilePC + 8) & ~0xF0000000);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
        m_Section->m_Jump.JumpPC = (uint32_t)m_CompilePC;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }
        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        if (m_Section->m_JumpSection)
        {
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->GenerateSectionLinkage();
        }
        else
        {
            m_RegWorkingSet.WriteBackRegisters();

            const asmjit::x86::Gp PCReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
            m_Assembler.MoveVariableToX64reg(PCReg, &m_Reg.m_PROGRAM_COUNTER, "_PROGRAM_COUNTER", false);
            m_Assembler.and_(PCReg.r32(), 0xF0000000);
            m_Assembler.add(PCReg.r32(), m_Opcode.target << 2);
            m_Assembler.MovDwordToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", PCReg);

            const uint64_t TargetPC = (m_CompilePC & 0xFFFFFFFFF0000000) + (m_Opcode.target << 2);
            const bool bCheck = TargetPC <= m_CompilePC;
            if (bCheck)
            {
                UpdateCounters(m_RegWorkingSet, bCheck, true);
            }
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, bCheck ? ExitReason_Normal : ExitReason_NormalNoSysCheck);
        }
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::ADDI()
{
    if (g_GameSettings.fastSP && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int32_t rs = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs);
        const int32_t imm = (int16_t)m_Opcode.immediate;
        const int32_t sum = rs + imm;
        if ((~(rs ^ imm) & (rs ^ sum)) & 0x80000000)
        {
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rt != 0)
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, sum);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
        }
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        const asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gp(), m_Opcode.rs);
        m_Assembler.add(Reg.r32(), (int32_t)((int16_t)m_Opcode.immediate));
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow, &CX64Ops::JoLabel);
        if (m_Opcode.rt != 0)
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), Reg.r32());
        }
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::ADDIU()
{
    if (m_Opcode.rt == 0 || (m_Opcode.immediate == 0 && m_Opcode.rs == m_Opcode.rt && m_RegWorkingSet.Is32BitMapped(m_Opcode.rt)))
    {
        return;
    }

    if (g_GameSettings.fastSP && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        const asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(asmjit::x86::Gpq(), true, true);
        m_Assembler.add(StackReg.r64(), (int32_t)((int16_t)m_Opcode.immediate));
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) + (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), (int32_t)((int16_t)m_Opcode.immediate));
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SLTI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SLTIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::ANDI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & m_Opcode.immediate);
    }
    else if (m_Opcode.immediate != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.rs);
        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), m_Opcode.immediate);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, 0);
    }
}

void CX64RecompilerOps::ORI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_GameSettings.fastSP && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, m_RegWorkingSet.GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) | m_Opcode.immediate);

        if (g_GameSettings.fastSP && m_Opcode.rt == 29 && m_Opcode.rs != 29)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::XORI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LUI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        uint32_t Address = 0;

        m_MMU.VAddrToPAddr(((int16_t)m_Opcode.offset << 16), Address);
        const uint64_t stackPtrVal = (uint64_t)(Address + m_MMU.Rdram());
        void * const var = &m_Recompiler->MemoryStackPos();
        m_Assembler.mov(asmjit::x86::dword_ptr(reinterpret_cast<uintptr_t>(var)), (uint32_t)stackPtrVal);
        m_Assembler.mov(asmjit::x86::dword_ptr(reinterpret_cast<uintptr_t>(var) + 4u), (uint32_t)(stackPtrVal >> 32));
    }

    m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, (uint32_t)((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
}

void CX64RecompilerOps::DADDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::DADDIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LDL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LDR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::RESERVED31()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LH()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LW()
{
    if (m_Opcode.base == 29 && g_GameSettings.fastSP && m_Opcode.rt != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        const asmjit::x86::Gp & DestReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
        const asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(asmjit::x86::Gpq(), true, true);
        if (!g_GameSettings.core32Bit)
        {
            m_Assembler.movsxd(DestReg.r64(), asmjit::x86::dword_ptr(StackReg, (int32_t)((int16_t)m_Opcode.offset)));
        }
        else
        {
            m_Assembler.mov(DestReg.r32(), asmjit::x86::dword_ptr(StackReg, (int32_t)((int16_t)m_Opcode.offset)));
        }
        if (m_Opcode.rt == 29)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        if (!m_RegWorkingSet.Is32Bit(m_Opcode.base))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if ((Address & 3u) != 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        if (m_Opcode.rt == 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        LW_KnownAddress(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), Address, true);
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    const asmjit::x86::Gp AddressReg = BaseOffsetAddress(false);
    CRegInfo ExitRegSet = m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp);

    m_Assembler.test(AddressReg.r32(), 3);
    CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionRead32, &CX64Ops::JneLabel);

    const asmjit::x86::Gp PageIndexReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    m_Assembler.mov(PageIndexReg.r32(), AddressReg.r32());
    m_Assembler.shr(PageIndexReg.r32(), 12);

    const asmjit::x86::Gp HostOffsetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(HostOffsetReg, &m_MMU.m_MemoryReadMap, "MMU->m_MemoryReadMap");
    m_Assembler.mov(HostOffsetReg.r64(), asmjit::x86::qword_ptr(HostOffsetReg, PageIndexReg, 3));
    m_Assembler.cmp(HostOffsetReg.r64(), (int64_t)-1);
    const stdstr AfterLoadLabel = stdstr_f("MemoryReadMap_%X_AfterLoad", m_CompilePC);
    const stdstr MapMissLabel = stdstr_f("MemoryReadMap_%X_Miss", m_CompilePC);
    asmjit::Label AfterLoad = m_Assembler.newLabel();
    asmjit::Label SlowPath = m_Assembler.newLabel();

    const CX64RegInfo PreMapRegSet(m_RegWorkingSet);
    m_Assembler.JeLabel(MapMissLabel.c_str(), SlowPath);

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        const asmjit::x86::Gp & DestReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
        if (!g_GameSettings.core32Bit)
        {
            m_Assembler.movsxd(DestReg.r64(), asmjit::x86::dword_ptr(AddressReg, HostOffsetReg));
        }
        else
        {
            m_Assembler.mov(DestReg.r32(), asmjit::x86::dword_ptr(AddressReg, HostOffsetReg));
        }
    }

    m_Assembler.EnterSecondarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      %s:", MapMissLabel.c_str());
    m_Assembler.bind(SlowPath);

    m_RegWorkingSet = PreMapRegSet;
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_JUMP);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::edx, AddressReg.r32());
    m_Assembler.MoveConstToX64reg(asmjit::x86::r8, reinterpret_cast<uintptr_t>(&m_TempValue32), "TempValue32");
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LW_VAddr32), "CMipsMemoryVM::LW_VAddr32");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
    m_RegWorkingSet.AfterCallDirect();

    asmjit::Label SlowPathException = m_Assembler.newLabel();
    m_Assembler.JeLabel(stdstr_f("MemoryReadMap_%X_Exception", m_CompilePC).c_str(), SlowPathException);

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        m_Assembler.MoveVariableToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), &m_TempValue32, "TempValue32", true);
    }

    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
    }
    m_Assembler.JmpLabel(AfterLoadLabel.c_str(), AfterLoad);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MemoryReadMap_%X_Exception:", m_CompilePC);
    m_Assembler.bind(SlowPathException);
    CompileExit(m_CompilePC, (uint32_t)-1, PreMapRegSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_Exception);

    m_Assembler.EnterPrimarySection();

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      %s:", AfterLoadLabel.c_str());
    m_Assembler.bind(AfterLoad);
    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::LBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LHU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SH()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SWL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SW()
{
    if (!g_DebugSettings.haveWriteBP && m_Opcode.base == 29 && g_GameSettings.fastSP)
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }

        const asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(asmjit::x86::Gpq(), true, true);
        const int32_t offset = (int16_t)m_Opcode.offset;

        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(StackReg, offset), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(StackReg, offset), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
        }
        else
        {
            const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
            m_Assembler.mov(asmjit::x86::dword_ptr(StackReg, offset), ValueReg.r32());
        }
        return;
    }

    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        if (!m_RegWorkingSet.Is32Bit(m_Opcode.base))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if ((Address & 3u) != 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        if (g_DebugSettings.haveWriteBP)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            SW_KnownAddress(Address, nullptr, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            SW_KnownAddress(Address, &m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), 0);
        }
        else
        {
            const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
            SW_KnownAddress(Address, &TempReg, 0);
        }
        return;
    }

    if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    }

    asmjit::x86::Gp ValueReg;
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        ValueReg = m_RegWorkingSet.IsMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
        m_RegWorkingSet.SetX64Protected(ValueReg.id(), true);
    }

    const asmjit::x86::Gp AddressReg = BaseOffsetAddress(false);
    CRegInfo ExitRegSet = m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp);
    m_Assembler.test(AddressReg.r32(), 3);
    CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionWrite32, &CX64Ops::JneLabel, &AddressReg);

    const asmjit::x86::Gp PageIndexReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    m_Assembler.mov(PageIndexReg.r32(), AddressReg.r32());
    m_Assembler.shr(PageIndexReg.r32(), 12);

    const asmjit::x86::Gp HostOffsetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(HostOffsetReg, &m_MMU.m_MemoryWriteMap, "MMU->m_MemoryWriteMap");
    m_Assembler.mov(HostOffsetReg.r64(), asmjit::x86::qword_ptr(HostOffsetReg, PageIndexReg, 3));
    m_Assembler.cmp(HostOffsetReg.r64(), (int64_t)-1);
    const stdstr AfterStoreLabel = stdstr_f("MemoryWriteMap_%X_AfterStore", m_CompilePC);
    asmjit::Label AfterStore = m_Assembler.newLabel();
    asmjit::Label SlowPath = m_Assembler.newLabel();
    m_Assembler.JeLabel(stdstr_f("MemoryWriteMap_%X_Miss", m_CompilePC).c_str(), SlowPath);

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, HostOffsetReg), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, HostOffsetReg), ValueReg.r32());
    }
    m_Assembler.EnterSecondarySection();
    m_Assembler.bind(SlowPath);

    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_JUMP);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::edx, AddressReg.r32());
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::r8d, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.mov(asmjit::x86::r8d, ValueReg.r32());
    }
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::SW_VAddr32), "CMipsMemoryVM::SW_VAddr32");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
    m_RegWorkingSet.AfterCallDirect();

    asmjit::Label SlowPathException = m_Assembler.newLabel();
    m_Assembler.JeLabel(stdstr_f("MemoryWriteMap_%X_Exception", m_CompilePC).c_str(), SlowPathException);

    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
    }
    m_Assembler.JmpLabel(AfterStoreLabel.c_str(), AfterStore);
    m_Assembler.bind(SlowPathException);
    CompileExit(m_CompilePC, (uint32_t)-1, ExitRegSet, ExitReason_Exception);

    m_Assembler.EnterPrimarySection();
    m_Assembler.bind(AfterStore);
}

void CX64RecompilerOps::SWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SDL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SDR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CACHE()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LDC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SC()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SWC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SDC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SRL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SRA()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLLV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs);
        m_Assembler.and_(asmjit::x86::ecx, 0x1F);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shl(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), asmjit::x86::cl);
    }
}

void CX64RecompilerOps::SPECIAL_SRLV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs);
        m_Assembler.and_(asmjit::x86::ecx, 0x1F);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shr(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), asmjit::x86::cl);
    }
}

void CX64RecompilerOps::SPECIAL_SRAV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_JR()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();

        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32((uint32_t)(m_CompilePC + 4), DelaySlot.Value) &&
            R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32((uint32_t)(m_CompilePC + 4), DelaySlot.Value) && R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            if (m_RegWorkingSet.IsConst(m_Opcode.rs))
            {
                if (((int)m_CompilePC >> 31) != (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? (int)m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs) : (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) >> 31)))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                else
                {
                    m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
                }
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                else
                {
                    const asmjit::x86::Gp PcReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                    m_Assembler.movsxd(PcReg, m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32());
                    m_Assembler.MovQwordToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", PcReg);
                }
            }
            else
            {
                const asmjit::x86::Gp PcReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
                m_Assembler.MovQwordToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", PcReg);
            }
            UpdateCounters(m_RegWorkingSet, true, true, false);
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_CheckPCAlignment);
            if (m_Section->m_JumpSection)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_JALR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SYSCALL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_BREAK()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SYNC()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MFLO()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, -1);
    m_Assembler.MoveVariable64ToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd), &m_Reg.m_LO.UDW, "RegLO.UDW");
}

void CX64RecompilerOps::SPECIAL_MTLO()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MFHI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MTHI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSLLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRAV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MULTU()
{
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), true);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, m_Opcode.rt);

    m_Assembler.mul(asmjit::x86::edx);

    m_Assembler.MovDwordToVariable(&m_Reg.m_LO.UW[0], "RegLO.UW[0]", asmjit::x86::eax);
    m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[0], "RegHI.UW[0]", asmjit::x86::edx);
    m_Assembler.sar(asmjit::x86::eax, 31);
    m_Assembler.sar(asmjit::x86::edx, 31);
    m_Assembler.MovDwordToVariable(&m_Reg.m_LO.UW[1], "RegLO.UW[1]", asmjit::x86::eax);
    m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[1], "RegHI.UW[1]", asmjit::x86::edx);
}

void CX64RecompilerOps::SPECIAL_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DIVU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DMULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DMULTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DDIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DDIVU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_ADD()
{
    const int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    const int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    if (m_RegWorkingSet.IsConst(source1) && m_RegWorkingSet.IsConst(source2))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
    const asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gp(), source1);
    if (m_RegWorkingSet.IsConst(source2))
    {
        m_Assembler.add(Reg.r32(), m_RegWorkingSet.GetMipsRegLo(source2));
    }
    else if (m_RegWorkingSet.IsKnown(source2) && m_RegWorkingSet.IsMapped(source2))
    {
        m_Assembler.add(Reg.r32(), m_RegWorkingSet.GetMipsRegMap(source2).r32());
    }
    else
    {
        m_Assembler.AddDwordFromVariable(Reg, &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow, &CX64Ops::JoLabel);

    if (m_Opcode.rd != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), Reg.r32());
    }
}

void CX64RecompilerOps::SPECIAL_ADDU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    const int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    const int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    if (m_RegWorkingSet.IsConst(source1) && m_RegWorkingSet.IsConst(source2))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, source1);
    if (m_RegWorkingSet.IsConst(source2))
    {
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegLo(source2));
    }
    else if (m_RegWorkingSet.IsKnown(source2) && m_RegWorkingSet.IsMapped(source2))
    {
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegMap(source2).r32());
    }
    else
    {
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), asmjit::x86::dword_ptr((uintptr_t)&m_Reg.m_GPR[source2].W[0]));
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SUBU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            const asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gp(), m_Opcode.rt);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), Reg.r32());
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
        }
        else
        {
            m_Assembler.SubVariableFromX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
    }
    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_AND()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            int ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            int MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(ConstReg))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else if (m_RegWorkingSet.Is64Bit(MappedReg))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                uint32_t Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                bool Sign = m_RegWorkingSet.IsSigned(ConstReg) && m_RegWorkingSet.IsSigned(MappedReg);

                if (Value != 0)
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, Sign, MappedReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), Value);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, 0);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            m_RegWorkingSet.ProtectGPR(KnownReg);
            if (KnownReg == m_Opcode.rd)
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg) || !g_GameSettings.core32Bit)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(KnownReg), KnownReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), asmjit::x86::dword_ptr((uintptr_t)&m_Reg.m_GPR[UnknownReg].UW[0]));
                }
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_OR()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            const uint64_t rtValue = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
            const uint64_t rsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, (uint64_t)(rtValue | rsValue));

            if ((m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1) ||
                (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0))
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_64);
            }
        }
        else
        {
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) | m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
        }
        return;
    }

    const bool RsConstZero = m_RegWorkingSet.IsConst(m_Opcode.rs) && (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) == 0 : m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) == 0);
    const bool RtConstZero = m_RegWorkingSet.IsConst(m_Opcode.rt) && (m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) == 0 : m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == 0);
    if (RsConstZero || RtConstZero)
    {
        const int Source = RsConstZero ? m_Opcode.rt : m_Opcode.rs;

        if (Source == 0 || m_RegWorkingSet.IsConst(Source))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (m_RegWorkingSet.Is32Bit(Source))
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(Source), Source);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, Source);
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rt) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
    {
        const int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        const int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        m_RegWorkingSet.ProtectGPR(source1);
        m_RegWorkingSet.ProtectGPR(source2);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, source1);

        if (m_RegWorkingSet.IsMapped(source2))
        {
            m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegMap(source2).r32());
        }
        else if (m_RegWorkingSet.IsConst(source2))
        {
            const uint32_t value = m_RegWorkingSet.GetMipsRegLo(source2);
            if (value != 0)
            {
                m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), value);
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            const uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            const uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
            uint64_t ConstValue;
            if (m_RegWorkingSet.Is64Bit(ConstReg))
            {
                ConstValue = m_RegWorkingSet.GetMipsReg(ConstReg);
            }
            else if (m_RegWorkingSet.IsSigned(ConstReg))
            {
                ConstValue = (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(ConstReg);
            }
            else
            {
                ConstValue = m_RegWorkingSet.GetMipsRegLo(ConstReg);
            }
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, MappedReg);
            if (ConstValue != 0)
            {
                const asmjit::x86::Gp temp64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                m_Assembler.mov(temp64.r64(), ConstValue);
                m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64(), temp64.r64());
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        const int KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        const int UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_XOR()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_Opcode.rt == m_Opcode.rs)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        const int KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        const int UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsMapped(KnownReg))
        {
            m_RegWorkingSet.ProtectGPR(KnownReg);
        }
        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (g_GameSettings.core32Bit)
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
            m_Assembler.XorVariableToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (g_GameSettings.core32Bit)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.XorVariableToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_NOR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLTU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            if ((m_RegWorkingSet.Is64Bit(m_Opcode.rt) && m_RegWorkingSet.Is64Bit(m_Opcode.rs)) ||
                (!g_GameSettings.core32Bit && (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
                const asmjit::x86::Gp & Rd = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd);
                m_Assembler.xor_(Rd.r32(), Rd.r32());
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
                m_Assembler.setb(Rd.r8Lo());
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        const uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        const uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        asmjit::Label Jump[2];

        if (m_RegWorkingSet.IsMapped(KnownReg))
        {
            m_RegWorkingSet.ProtectGPR(KnownReg);
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        const asmjit::x86::Gp & Rd = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd);
        if (KnownReg == m_Opcode.rd)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (g_GameSettings.core32Bit)
        {
            const bool bConstant = m_RegWorkingSet.IsConst(KnownReg);
            m_Assembler.xor_(Rd.r32(), Rd.r32());
            if (bConstant)
            {
                m_Assembler.CmpConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
            }
            else
            {
                m_Assembler.CmpRegToVariable(m_RegWorkingSet.GetMipsRegMap(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.seta(Rd.r8Lo());
            }
            else
            {
                m_Assembler.setb(Rd.r8Lo());
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (g_GameSettings.core32Bit)
    {
        const asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rs);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        const asmjit::x86::Gp & Rd = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd);
        m_Assembler.xor_(Rd.r32(), Rd.r32());
        m_Assembler.CmpRegToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.setb(Rd.r8Lo());
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_DADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DADDU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSUBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSLL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRA()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSLL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRA32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_MF()
{
    if (m_Opcode.rd == CRegisters::COP0Reg_Count)
    {
        UpdateCounters(m_RegWorkingSet, false, true);
    }

    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::edx, m_Opcode.rd);
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_Reg, MemberFuncAddress(&CRegisters::Cop0_MF), "CRegisters::Cop0_MF");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], asmjit::x86::eax);
    m_RegWorkingSet.AfterCallDirect();
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_Assembler.MoveVariableToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], true);
}

void CX64RecompilerOps::COP0_DMF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_MT()
{
    if (m_Opcode.rd == CRegisters::COP0Reg_Wired || m_Opcode.rd == CRegisters::COP0Reg_Compare || m_Opcode.rd == CRegisters::COP0Reg_Count)
    {
        UpdateCounters(m_RegWorkingSet, false, true);
    }
    m_RegWorkingSet.BeforeCallDirect();
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_Assembler.movsxd(asmjit::x86::r8, m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
    }
    else
    {
        m_Assembler.MoveVariableToX64reg(asmjit::x86::r8, &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], true);
    }
    m_Assembler.mov(asmjit::x86::edx, m_Opcode.rd);
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_Reg, MemberFuncAddress(&CRegisters::Cop0_MT), "CRegisters::Cop0_MT");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();
}

void CX64RecompilerOps::COP0_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBWI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_ERET()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_MF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_DMF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_CF()
{
    CompileCop1Test();

    if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
    {
        UnknownOpcode();
        return;
    }

    if (m_Opcode.rt == 0)
    {
        return;
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_Assembler.MoveVariableToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), &m_Reg.m_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], true);
}

void CX64RecompilerOps::COP1_MT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_CT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_MUL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ABS()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_NEG()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_SQRT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_MOV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ROUND_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_TRUNC_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CEIL_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_FLOOR_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ROUND_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_TRUNC_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CEIL_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_FLOOR_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CVT_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CVT_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CMP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_MUL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ABS()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_NEG()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_SQRT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_MOV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ROUND_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_TRUNC_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CEIL_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_FLOOR_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ROUND_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_TRUNC_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CEIL_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_FLOOR_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CVT_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CVT_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CMP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_W_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_W_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_L_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_L_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::UnknownOpcode()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::EnterCodeBlock()
{
    m_Assembler.sub(asmjit::x86::rsp, FunctionStackSize);
}

void CX64RecompilerOps::CompileExitCode()
{
}

void CX64RecompilerOps::CompileInPermLoop(CRegInfo & /*RegSet*/, uint32_t /*ProgramCounter*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SyncRegState(const CRegInfo & /*SyncTo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CX64RegInfo & CX64RecompilerOps::GetRegWorkingSet(void)
{
    return m_RegWorkingSet;
}

void CX64RecompilerOps::SetRegWorkingSet(const CX64RegInfo & RegInfo)
{
    m_RegWorkingSet = RegInfo;
}

bool CX64RecompilerOps::InheritParentInfo()
{
    m_Section->DisplaySectionInformation();

    if (m_Section->m_ParentSection.empty())
    {
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    if (m_Section->m_ParentSection.size() == 1)
    {
        CCodeSection * Parent = *(m_Section->m_ParentSection.begin());
        if (!Parent->m_EnterLabel.isValid())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        CJumpInfo * JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;

        m_Section->m_RegEnter = JumpInfo->RegSet;
        LinkJump(*JumpInfo);
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CX64RecompilerOps::LinkJump(CJumpInfo & JumpInfo, uint32_t /*SectionID*/, uint32_t /*FromSectionID*/)
{
    if (JumpInfo.LinkLocation.isValid())
    {
        m_CodeBlock.Log("");
        m_Assembler.bind(JumpInfo.LinkLocation);
        JumpInfo.LinkLocation = asmjit::Label();
        if (JumpInfo.LinkLocation2.isValid())
        {
            m_Assembler.bind(JumpInfo.LinkLocation2);
            JumpInfo.LinkLocation2 = asmjit::Label();
        }
    }
}

void CX64RecompilerOps::JumpToSection(CCodeSection * /*Section*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::JumpToUnknown(CJumpInfo * /*JumpInfo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SetCurrentPC(uint32_t ProgramCounter)
{
    uint32_t Value;
    if (!m_MMU.MemoryValue32(ProgramCounter, Value))
    {
        g_Notify->FatalError(GS(MSG_FAIL_LOAD_WORD));
    }
    m_Instruction = R4300iInstruction((int32_t)ProgramCounter, Value);
}

uint32_t CX64RecompilerOps::GetCurrentPC(void)
{
    return m_CompilePC;
}

void CX64RecompilerOps::SetCurrentSection(CCodeSection * section)
{
    m_Section = section;
}

void CX64RecompilerOps::SetNextStepType(PIPELINE_STAGE StepType)
{
    m_PipelineStage = StepType;
}

PIPELINE_STAGE CX64RecompilerOps::GetNextStepType(void)
{
    return m_PipelineStage;
}

const R4300iOpcode & CX64RecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
}

void CX64RecompilerOps::PreCompileOpcode(void)
{
    if (m_PipelineStage != PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        m_CodeBlock.Log("  %X %s", (uint32_t)m_CompilePC, m_Instruction.NameAndParam().c_str());
    }
    m_RegWorkingSet.ResetRegisterProtection();
}

void CX64RecompilerOps::PostCompileOpcode(void)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
    if (!g_GameSettings.regCaching)
    {
        m_RegWorkingSet.WriteBackRegisters();
    }
}

void CX64RecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo ExitRegSet, ExitReason reason, void (CX64Ops::*x64Jmp)(const char * LabelName, asmjit::Label & JumpLabel), const asmjit::x86::Gp * BadVAddrReg)
{
    if (x64Jmp != nullptr)
    {
        asmjit::Label ExitLabel = m_Assembler.newLabel();
        const stdstr ExitName = stdstr_f("Exit_%08X_%d", JumpPC, m_ExitLabelCount++);
        (m_Assembler.*x64Jmp)(ExitName.c_str(), ExitLabel);
        m_Assembler.EnterSecondarySection();
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      %s:", ExitName.c_str());
        m_Assembler.bind(ExitLabel);
        CompileExit((uint32_t)-1, TargetPC, ExitRegSet, reason, nullptr, BadVAddrReg);
        m_Assembler.EnterPrimarySection();
        return;
    }

    ExitRegSet.WriteBackRegisters();
    if (TargetPC != (uint32_t)-1)
    {
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", TargetPC);
        UpdateCounters(ExitRegSet, TargetPC <= JumpPC && JumpPC != (uint32_t)-1, reason == ExitReason_Normal);
    }
    else
    {
        UpdateCounters(ExitRegSet, false, reason == ExitReason_Normal, reason != ExitReason_Normal);
    }

    switch (reason)
    {
    case ExitReason_Normal:
    case ExitReason_CheckPCAlignment:
    case ExitReason_NormalNoSysCheck:
        ExitRegSet.SetBlockCycleCount(0);
        if ((reason == ExitReason_Normal || reason == ExitReason_CheckPCAlignment) && (TargetPC == (uint32_t)-1 || TargetPC <= JumpPC))
        {
            CompileSystemCheck((uint32_t)-1, ExitRegSet);
        }
        if (reason == ExitReason_CheckPCAlignment)
        {
            m_Assembler.MoveVariable64ToX64reg(asmjit::x86::rax, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
            m_Assembler.test(asmjit::x86::eax, 3);
            asmjit::Label ValidPCJump = m_Assembler.newLabel();
            m_Assembler.JeLabel("ValidPC", ValidPCJump);
            m_Assembler.mov(asmjit::x86::rdx, asmjit::x86::rax);
            m_Assembler.mov(asmjit::x86::r8d, 1);
            m_Assembler.sub(asmjit::x86::rsp, 32);
            m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::DoAddressError), "CRegisters::DoAddressError");
            m_Assembler.add(asmjit::x86::rsp, 32);
            m_Assembler.MoveVariableToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation", false);
            m_Assembler.MovDwordToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::eax);
            m_Assembler.cdq();
            m_Assembler.MovDwordToVariable((void *)(((uint8_t *)&g_Reg->m_PROGRAM_COUNTER) + 4), "PROGRAM_COUNTER+4", asmjit::x86::edx);
            m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
            ExitCodeBlock();
            m_Assembler.bind(ValidPCJump);
        }
        ExitCodeBlock();
        break;
    case ExitReason_ExceptionOverflow:
    {
        const bool InDelaySlot = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", InDelaySlot ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.mov(asmjit::x86::rdx, EXC_OV);
        m_Assembler.xor_(asmjit::x86::r8d, asmjit::x86::r8d);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::TriggerException), "CRegisters::TriggerException");
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_Assembler.MoveVariable32ToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MovDwordToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::eax);
        m_Assembler.cdq();
        m_Assembler.MovDwordToVariable((void *)(((uint8_t *)&g_Reg->m_PROGRAM_COUNTER) + 4), "PROGRAM_COUNTER+4", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    }
    case ExitReason_COP1Unuseable:
    {
        const bool InDelaySlot = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", InDelaySlot ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.mov(asmjit::x86::rdx, EXC_CPU);
        m_Assembler.mov(asmjit::x86::r8d, 1);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::TriggerException), "CRegisters::TriggerException");
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_Assembler.MoveVariableToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation", false);
        m_Assembler.MovDwordToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::eax);
        m_Assembler.cdq();
        m_Assembler.MovDwordToVariable((void *)(((uint8_t *)&g_Reg->m_PROGRAM_COUNTER) + 4), "PROGRAM_COUNTER+4", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitRegSet.SetBlockCycleCount(0);
        UpdateCounters(ExitRegSet, true, false, false);
        ExitCodeBlock();
        break;
    }
    case ExitReason_AddressErrorExceptionRead32:
    {
        const bool InDelaySlot = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", InDelaySlot ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.movsxd(asmjit::x86::rdx, asmjit::x86::dword_ptr((uintptr_t)(&m_TempValue32)));
        m_Assembler.mov(asmjit::x86::r8d, 1);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::DoAddressError), "CRegisters::DoAddressError");
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_Assembler.MoveVariable32ToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MovDwordToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::eax);
        m_Assembler.cdq();
        m_Assembler.MovDwordToVariable((void *)(((uint8_t *)&g_Reg->m_PROGRAM_COUNTER) + 4), "PROGRAM_COUNTER+4", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    }
    case ExitReason_AddressErrorExceptionWrite32:
    {
        const bool InDelaySlot = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", InDelaySlot ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        if (BadVAddrReg != nullptr && BadVAddrReg->isValid())
        {
            m_Assembler.mov(asmjit::x86::rdx, asmjit::x86::r11);
        }
        else
        {
            m_Assembler.MoveVariableToX64reg(asmjit::x86::rdx, &m_TempValue32, "m_TempValue32", true);
        }
        m_Assembler.xor_(asmjit::x86::r8d, asmjit::x86::r8d);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::DoAddressError), "CRegisters::DoAddressError");
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_Assembler.MoveVariableToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation", false);
        m_Assembler.MovDwordToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::eax);
        m_Assembler.cdq();
        m_Assembler.MovDwordToVariable((void *)(((uint8_t *)&g_Reg->m_PROGRAM_COUNTER) + 4), "PROGRAM_COUNTER+4", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    }

    case ExitReason_Exception:
        m_Assembler.MoveVariableToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation", false);
        m_Assembler.MovDwordToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::eax);
        m_Assembler.cdq();
        m_Assembler.MovDwordToVariable(reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(&g_Reg->m_PROGRAM_COUNTER) + 4), "PROGRAM_COUNTER+4", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        if (TargetPC == (uint32_t)-1)
        {
            ExitRegSet.SetBlockCycleCount(0);
            UpdateCounters(ExitRegSet, false, false, false);
        }
        ExitCodeBlock();
        break;
    default:
        WriteTrace(TraceRecompiler, TraceError, "CX64RecompilerOps::CompileExit: unhandled exit reason (%d)", reason);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::ExitCodeBlock(void)
{
    if (g_SyncSystem)
    {
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_BaseSystem, MemberFuncAddress(&CN64System::SyncSystem), "CN64System::SyncSystem");
        m_Assembler.add(asmjit::x86::rsp, 32);
    }
    m_Assembler.add(asmjit::x86::rsp, FunctionStackSize);
    m_Assembler.ret();
}

void CX64RecompilerOps::UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles)
{
    if (!g_SyncSystem)
    {
        return;
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      // Updating sync CPU");
    RegSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::rdx, Cycles);
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(g_System, MemberFuncAddress(&CN64System::UpdateSyncCPU), "CN64System::UpdateSyncCPU");
    m_Assembler.add(asmjit::x86::rsp, 32);
    RegSet.AfterCallDirect();
}

void CX64RecompilerOps::UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues, bool UpdateTimer)
{
    if (RegSet.GetBlockCycleCount() != 0)
    {
        UpdateSyncCPU(RegSet, RegSet.GetBlockCycleCount());
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      // Update counter");
        m_Assembler.SubConstFromVariable(RegSet.GetBlockCycleCount(), g_NextTimer, "g_NextTimer");
        if (ClearValues)
        {
            RegSet.SetBlockCycleCount(0);
        }
    }
    else if (CheckTimer)
    {
        m_Assembler.CmpConstToVariable(g_NextTimer, "g_NextTimer", 0);
    }

    if (CheckTimer)
    {
        asmjit::Label TimerDonePath = m_Assembler.newLabel();
        asmjit::Label ContinueFromTimerTest = m_Assembler.newLabel();
        m_Assembler.JsLabel("Timer_Done_Path", TimerDonePath);

        m_Assembler.EnterSecondarySection();
        m_CodeBlock.Log("      Timer_Done_Path:");
        m_Assembler.bind(TimerDonePath);

        RegSet.BeforeCallDirect();
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_SystemTimer, MemberFuncAddress(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
        m_Assembler.add(asmjit::x86::rsp, 32);
        RegSet.AfterCallDirect();
        m_Assembler.jmp(ContinueFromTimerTest);

        m_Assembler.EnterPrimarySection();

        m_CodeBlock.Log("");
        m_Assembler.bind(ContinueFromTimerTest);
    }

    if ((UpdateTimer || g_GameSettings.overClockModifier != 1) && g_SyncSystem)
    {
        RegSet.BeforeCallDirect();
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_SystemTimer, MemberFuncAddress(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        m_Assembler.add(asmjit::x86::rsp, 32);
        RegSet.AfterCallDirect();
    }
}

void CX64RecompilerOps::CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet)
{
    m_Assembler.cmp(asmjit::x86::byte_ptr((uintptr_t)&m_SystemEvents.DoSomething()), 0);
    asmjit::Label ContinueFromInterruptTest = m_Assembler.newLabel();
    m_Assembler.JeLabel("Continue_From_Interrupt_Test", ContinueFromInterruptTest);

    if (TargetPC != (uint32_t)-1)
    {
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", TargetPC);
    }

    CRegInfo RegSetCopy(RegSet);
    RegSetCopy.WriteBackRegisters();

    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_SystemEvents, MemberFuncAddress(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
    m_Assembler.add(asmjit::x86::rsp, 32);

    ExitCodeBlock();
    m_CodeBlock.Log("");
    m_Assembler.bind(ContinueFromInterruptTest);
}

void CX64RecompilerOps::CompileExecuteBP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileExecuteDelaySlotBP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

uint32_t CX64RecompilerOps::ColdEntryOffset(void) const
{
    return m_ColdEntryOffset;
}

uint32_t CX64RecompilerOps::WarmEntryOffset(void) const
{
    return m_WarmEntryOffset;
}

bool CX64RecompilerOps::LW_KnownAddress(const asmjit::x86::Gp & Reg, uint32_t VAddr, bool ResultSigned)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__); // LW_KnownAddress: TLB / CompileLoadMemoryValue path
        return false;
    }

    uint32_t PAddr = 0;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }

    switch (PAddr & 0xFFF00000u)
    {
    case 0x04000000u:
        if (PAddr < 0x04001000u)
        {
            m_Assembler.MoveVariableToX64reg(Reg, (PAddr - 0x04000000u) + m_MMU.Dmem(), stdstr_f("Dmem + 0x%X", PAddr - 0x04000000u).c_str(), ResultSigned);
        }
        else if (PAddr < 0x04002000u)
        {
            m_Assembler.MoveVariableToX64reg(Reg, (PAddr - 0x04001000u) + m_MMU.Imem(), stdstr_f("Imem + 0x%X", PAddr - 0x04001000u).c_str(), ResultSigned);
        }
        else
        {
            switch (PAddr)
            {
            case 0x04040010u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.SP_STATUS_REG, "SP_STATUS_REG", ResultSigned); break;
            case 0x04040014u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.SP_DMA_FULL_REG, "SP_DMA_FULL_REG", ResultSigned); break;
            case 0x04040018u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG", ResultSigned); break;
            case 0x0404001Cu:
                m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", ResultSigned);
                m_Assembler.MoveConstToVariable(&m_Reg.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", 1);
                break;
            case 0x04080000u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.SP_PC_REG, "SP_PC_REG", ResultSigned); break;
            default:
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr, stdstr_f("PAddr 0x%08X", PAddr).c_str());
                m_Assembler.MoveConstToX64reg(asmjit::x86::r8, reinterpret_cast<uintptr_t>(&m_TempValue32), "m_TempValue32");
                m_Assembler.sub(asmjit::x86::rsp, 32);
                m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LW_PhysicalAddress), "CMipsMemoryVM::LW_PhysicalAddress");
                m_Assembler.add(asmjit::x86::rsp, 32);
                m_RegWorkingSet.AfterCallDirect();
                m_Assembler.MoveVariableToX64reg(Reg, &m_TempValue32, "m_TempValue32", ResultSigned);
                break;
            }
        }
        return false;
    case 0x04600000u:
        switch (PAddr)
        {
        case 0x04600000u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG", ResultSigned); break;
        case 0x04600004u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_CART_ADDR_REG, "PI_CART_ADDR_REG", ResultSigned); break;
        case 0x04600008u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_RD_LEN_REG, "PI_RD_LEN_REG", ResultSigned); break;
        case 0x0460000Cu: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_WR_LEN_REG, "PI_WR_LEN_REG", ResultSigned); break;
        case 0x04600010u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_STATUS_REG, "PI_STATUS_REG", ResultSigned); break;
        case 0x04600014u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_DOMAIN1_REG, "PI_DOMAIN1_REG", ResultSigned); break;
        case 0x04600018u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", ResultSigned); break;
        case 0x0460001Cu: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", ResultSigned); break;
        case 0x04600020u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", ResultSigned); break;
        case 0x04600024u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_DOMAIN2_REG, "PI_DOMAIN2_REG", ResultSigned); break;
        case 0x04600028u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", ResultSigned); break;
        case 0x0460002Cu: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", ResultSigned); break;
        case 0x04600030u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", ResultSigned); break;
        default:
            m_Assembler.xor_(Reg, Reg);
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            break;
        }
        return false;
    default:
        if ((PAddr & 0xF0000000u) == 0x10000000u && (PAddr - 0x10000000u) < m_Rom.GetRomSize())
        {
            const uint32_t RomPAddr = PAddr & 0x1FFFFFFFu;
            MemoryHandler * const pThis = (MemoryHandler *)&m_MMU.RomMemory();
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(pThis), "&g_MMU->m_RomMemoryHandler");
            m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, RomPAddr);
            m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uintptr_t)&m_TempValue32, "m_TempValue32");
            m_Assembler.sub(asmjit::x86::rsp, 32);
            m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
            m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11));
            m_Assembler.add(asmjit::x86::rsp, 32);
            m_RegWorkingSet.AfterCallDirect();
            if (ResultSigned)
            {
                m_Assembler.MoveVariable32SignExtendToX64reg(Reg, &m_TempValue32, "m_TempValue32");
            }
            else
            {
                m_Assembler.MoveVariable32ToX64reg(Reg, &m_TempValue32, "m_TempValue32");
            }
            return true;
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    }
    return false;
}

void CX64RecompilerOps::SW_KnownAddress(uint32_t VAddr, const asmjit::x86::Gp * ValueReg, uint32_t ValueConst)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    uint32_t PAddr = 0;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    switch (PAddr & 0xFFF00000u)
    {
    case 0x00000000u:
    case 0x00100000u:
    case 0x00200000u:
    case 0x00300000u:
    case 0x00400000u:
    case 0x00500000u:
    case 0x00600000u:
    case 0x00700000u:
        if (g_GameSettings.smmStoreInstruc)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (PAddr < m_MMU.RdramSize())
        {
            const uintptr_t dst = reinterpret_cast<uintptr_t>(m_MMU.Rdram() + PAddr);
            if (ValueReg != nullptr)
            {
                m_Assembler.mov(asmjit::x86::dword_ptr(dst), ValueReg->r32());
            }
            else
            {
                m_Assembler.mov(asmjit::x86::dword_ptr(dst), ValueConst);
            }
        }
        else if (g_DebugSettings.breakOnUnhandledMemory)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case 0x04000000u:
        if (PAddr < 0x04001000u)
        {
            const uintptr_t dst = (uintptr_t)(m_MMU.Dmem() + (PAddr - 0x04000000u));
            if (ValueReg != nullptr)
            {
                m_Assembler.mov(asmjit::x86::dword_ptr(dst), ValueReg->r32());
            }
            else
            {
                m_Assembler.mov(asmjit::x86::dword_ptr(dst), ValueConst);
            }
        }
        else if (PAddr < 0x04002000u)
        {
            const uintptr_t dst = (uintptr_t)(m_MMU.Imem() + (PAddr - 0x04001000u));
            if (ValueReg != nullptr)
            {
                m_Assembler.mov(asmjit::x86::dword_ptr(dst), ValueReg->r32());
            }
            else
            {
                m_Assembler.mov(asmjit::x86::dword_ptr(dst), ValueConst);
            }
        }
        else
        {
            switch (PAddr)
            {
            case 0x04040000u:
            case 0x04040004u:
            case 0x04040008u:
            case 0x0404000Cu:
            case 0x04040010u:
            {
                if (PAddr == 0x04040010u)
                {
                    UpdateCounters(m_RegWorkingSet, false, true, false);
                }
                m_RegWorkingSet.BeforeCallDirect();
                if (ValueReg != nullptr)
                {
                    if (*ValueReg != asmjit::x86::r8)
                    {
                        m_Assembler.mov(asmjit::x86::r8d, ValueReg->r32());
                    }
                }
                else
                {
                    m_Assembler.mov(asmjit::x86::r8d, ValueConst);
                }
                m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_SPRegistersHandler), "g_MMU->m_SPRegistersHandler");
                m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
                m_Assembler.mov(asmjit::x86::r9d, 0xFFFFFFFFu);
                m_Assembler.sub(asmjit::x86::rsp, 32);
                m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
                m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11, 8));
                m_Assembler.add(asmjit::x86::rsp, 32);
                m_RegWorkingSet.AfterCallDirect();
                break;
            }
            case 0x0404001Cu:
                m_Assembler.MoveConstToVariable(&m_Reg.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", 0);
                break;
            case 0x04080000u:
                if (ValueReg != nullptr)
                {
                    m_Assembler.MovDwordToVariable(&m_Reg.SP_PC_REG, "SP_PC_REG", *ValueReg);
                    {
                        m_Assembler.AndConstToVariable(&m_Reg.SP_PC_REG, "SP_PC_REG", 0xFFCu);
                    }
                }
                else
                {
                    m_Assembler.MoveConstToVariable(&m_Reg.SP_PC_REG, "SP_PC_REG", ValueConst & 0xFFCu);
                }
                break;
            default:
                if (g_DebugSettings.breakOnUnhandledMemory)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                break;
            }
        }
        break;
    case 0x04300000u:
        switch (PAddr)
        {
        case 0x04300000u:
        case 0x0430000Cu:
        {
            m_RegWorkingSet.BeforeCallDirect();
            if (ValueReg != nullptr)
            {
                if (*ValueReg != asmjit::x86::r8)
                {
                    m_Assembler.mov(asmjit::x86::r8d, ValueReg->r32());
                }
            }
            else
            {
                m_Assembler.mov(asmjit::x86::r8d, ValueConst);
            }
            m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, (uintptr_t)(&m_MMU.m_MIPSInterfaceHandler), "g_MMU->m_MIPSInterfaceHandler");
            m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
            m_Assembler.mov(asmjit::x86::r9d, 0xFFFFFFFFu);
            m_Assembler.sub(asmjit::x86::rsp, 32);
            m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
            m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11, 8));
            m_Assembler.add(asmjit::x86::rsp, 32);
            m_RegWorkingSet.AfterCallDirect();
            break;
        }
        default:
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            break;
        }
        break;
    case 0x04500000u:
        UpdateCounters(m_RegWorkingSet, false, true, false);
        m_RegWorkingSet.BeforeCallDirect();
        if (ValueReg != nullptr)
        {
            if (*ValueReg != asmjit::x86::r8)
            {
                m_Assembler.mov(asmjit::x86::r8d, ValueReg->r32());
            }
        }
        else
        {
            m_Assembler.mov(asmjit::x86::r8d, ValueConst);
        }
        m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_AudioInterfaceHandler), "g_MMU->m_AudioInterfaceHandler");
        m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
        m_Assembler.mov(asmjit::x86::r9d, 0xFFFFFFFFu);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
        m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11, 8));
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04600000u:
        switch (PAddr)
        {
        case 0x04600000u:
        case 0x04600004u:
        case 0x04600008u:
        case 0x0460000Cu:
        case 0x04600010u:
        {
            if (PAddr == 0x04600008u || PAddr == 0x0460000Cu)
            {
                UpdateCounters(m_RegWorkingSet, false, true, false);
            }

            m_RegWorkingSet.BeforeCallDirect();
            if (ValueReg != nullptr)
            {
                if (*ValueReg != asmjit::x86::r8)
                {
                    m_Assembler.mov(asmjit::x86::r8d, ValueReg->r32());
                }
            }
            else
            {
                m_Assembler.mov(asmjit::x86::r8d, ValueConst);
            }
            m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_PeripheralInterfaceHandler), "g_MMU->m_PeripheralInterfaceHandler");
            m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
            m_Assembler.mov(asmjit::x86::r9d, 0xFFFFFFFFu);
            m_Assembler.sub(asmjit::x86::rsp, 32);
            m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
            m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11, 8));
            m_Assembler.add(asmjit::x86::rsp, 32);
            m_RegWorkingSet.AfterCallDirect();
            break;
        }
        default:
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            break;
        }
        break;
    case 0x04800000u:
        switch (PAddr)
        {
        case 0x04800000u:
        case 0x04800004u:
        case 0x04800010u:
        case 0x04800018u:
        {
            if (PAddr == 0x04800004u || PAddr == 0x04800010u)
            {
                UpdateCounters(m_RegWorkingSet, false, true, false);
            }
            m_RegWorkingSet.BeforeCallDirect();
            if (ValueReg != nullptr)
            {
                if (*ValueReg != asmjit::x86::r8)
                {
                    m_Assembler.mov(asmjit::x86::r8d, ValueReg->r32());
                }
            }
            else
            {
                m_Assembler.mov(asmjit::x86::r8d, ValueConst);
            }
            m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_SerialInterfaceHandler), "g_MMU->m_SerialInterfaceHandler");
            m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
            m_Assembler.mov(asmjit::x86::r9d, 0xFFFFFFFFu);
            m_Assembler.sub(asmjit::x86::rsp, 32);
            m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
            m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11, 8));
            m_Assembler.add(asmjit::x86::rsp, 32);
            m_RegWorkingSet.AfterCallDirect();
            break;
        }
        default:
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            break;
        }
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

asmjit::x86::Gp CX64RecompilerOps::BaseOffsetAddress(bool UseBaseRegister)
{
    asmjit::x86::Gp AddressReg;
    if (m_RegWorkingSet.IsMapped(m_Opcode.base))
    {
        const asmjit::x86::Gp & BaseReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.base);
        if (m_Opcode.offset != 0)
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.base);
            AddressReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
            m_Assembler.lea(AddressReg, asmjit::x86::ptr(BaseReg.r32(), (int16_t)m_Opcode.offset));
        }
        else if (UseBaseRegister)
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.base);
            AddressReg = BaseReg;
        }
        else
        {
            AddressReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.base);
        }
    }
    else
    {
        AddressReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.base);
        if (m_Opcode.offset != 0)
        {
            m_Assembler.add(AddressReg.r32(), (int32_t)((int16_t)m_Opcode.offset));
        }
    }
    return AddressReg;
}

#endif