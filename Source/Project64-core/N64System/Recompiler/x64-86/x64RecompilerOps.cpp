#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/x64-86/x64RecompilerOps.h>
#include <Project64-core/N64System/SystemGlobals.h>

extern "C"
{
#include "softfloat.h"
}

uint32_t CX64RecompilerOps::m_TempValue32 = 0;
uint64_t CX64RecompilerOps::m_TempValue64 = 0;

CX64RecompilerOps::CX64RecompilerOps(CN64System & System, CCodeBlock & CodeBlock) :
    CRecompilerOpsBase(System, CodeBlock),
    m_Recompiler(System.m_Recomp),
    m_Rom(*g_Rom),
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler),
    m_MMU(System.m_MMU_VM),
    m_RegBeforeDelay(CodeBlock, m_Assembler),
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
            CompileCop1Test();
            m_Assembler.AndConstToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", ~0x0003F000u);
        }
        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8 && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
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
        m_Section->m_Jump.FallThrough = m_Section->m_Jump.TargetPC < m_Section->m_Cont.TargetPC;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;

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
            if ((m_CompilePC & 0xFFC) != 0xFFC)
            {
                m_Section->m_Cont.BranchLabel = m_Section->m_ContinueSection != nullptr ? "Continue" : stdstr_f("ExitBlock_%X_Continue", m_Section->m_EnterPC);
                m_Section->m_Jump.BranchLabel = m_Section->m_JumpSection != nullptr ? "Jump" : stdstr_f("ExitBlock_%X_Jump", m_Section->m_EnterPC);
            }
            else
            {
                m_Section->m_Cont.BranchLabel = "Continue";
                m_Section->m_Jump.BranchLabel = "Jump";
            }
            if (m_Section->m_Jump.TargetPC != m_Section->m_Cont.TargetPC)
            {
                Compile_BranchCompare(CompareType);
            }
            if (!m_Section->m_Jump.FallThrough && !m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation.isValid())
                {
                    LinkJump(m_Section->m_Jump);
                    m_Section->m_Jump.FallThrough = true;
                }
                else if (m_Section->m_Cont.LinkLocation.isValid())
                {
                    LinkJump(m_Section->m_Cont);
                    m_Section->m_Cont.FallThrough = true;
                }
            }
            if ((m_CompilePC & 0xFFC) == 0xFFC)
            {
                // The delay slot is on the next page, so the interpreter runs it and then jumps to m_JumpToLocation
                asmjit::Label DelayLinkLocation;
                if (m_Section->m_Jump.FallThrough)
                {
                    if (m_Section->m_Jump.LinkLocation.isValid() || m_Section->m_Jump.LinkLocation2.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    m_Assembler.MoveConst64ToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (uint64_t)(int64_t)(int32_t)m_Section->m_Jump.TargetPC);
                }
                else if (m_Section->m_Cont.FallThrough)
                {
                    if (m_Section->m_Cont.LinkLocation.isValid() || m_Section->m_Cont.LinkLocation2.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    m_Assembler.MoveConst64ToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (uint64_t)(int64_t)(int32_t)m_Section->m_Cont.TargetPC);
                }

                if (m_Section->m_Jump.LinkLocation.isValid() || m_Section->m_Jump.LinkLocation2.isValid())
                {
                    if (DelayLinkLocation.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    DelayLinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel("DoDelaySlot", DelayLinkLocation);

                    LinkJump(m_Section->m_Jump);
                    m_Assembler.MoveConst64ToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (uint64_t)(int64_t)(int32_t)m_Section->m_Jump.TargetPC);
                }
                if (m_Section->m_Cont.LinkLocation.isValid() || m_Section->m_Cont.LinkLocation2.isValid())
                {
                    if (DelayLinkLocation.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    DelayLinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel("DoDelaySlot", DelayLinkLocation);

                    LinkJump(m_Section->m_Cont);
                    m_Assembler.MoveConst64ToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (uint64_t)(int64_t)(int32_t)m_Section->m_Cont.TargetPC);
                }
                if (DelayLinkLocation.isValid())
                {
                    m_CodeBlock.Log("");
                    m_Assembler.bind(DelayLinkLocation);
                }
                OverflowDelaySlot(false);
                return;
            }
            m_RegWorkingSet.ResetRegisterProtection();
            m_RegBeforeDelay = m_RegWorkingSet;
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
            m_PipelineStage = PIPELINE_STAGE_NORMAL;
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_GameSettings.countPerOp);
            SetCurrentPC((uint32_t)(GetCurrentPC() + 4));
            return;
        }
        if (m_EffectDelaySlot)
        {
            CJumpInfo * FallInfo = m_Section->m_Jump.FallThrough ? &m_Section->m_Jump : &m_Section->m_Cont;
            CJumpInfo * JumpInfo = m_Section->m_Jump.FallThrough ? &m_Section->m_Cont : &m_Section->m_Jump;

            if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot)
            {
                m_RegWorkingSet.ResetRegisterProtection();
                FallInfo->RegSet = m_RegWorkingSet;
                if (FallInfo == &m_Section->m_Jump)
                {
                    if (m_Section->m_JumpSection != nullptr)
                    {
                        m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Jump.BranchLabel = "ExitBlock";
                    }
                    if (FallInfo->TargetPC <= (uint32_t)m_CompilePC)
                    {
                        UpdateCounters(FallInfo->RegSet, true, true, true);
                        m_CodeBlock.Log("CompileSystemCheck 12");
                        CompileSystemCheck(FallInfo->TargetPC, FallInfo->RegSet);
                        m_RegWorkingSet.ResetRegisterProtection();
                        FallInfo->Reason = ExitReason_NormalNoSysCheck;
                        FallInfo->JumpPC = (uint32_t)-1;
                    }
                }
                else
                {
                    if (m_Section->m_ContinueSection != nullptr)
                    {
                        m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Cont.BranchLabel = stdstr_f("ExitBlock_%X_Continue", m_Section->m_EnterPC);
                    }
                }
                FallInfo->DoneDelaySlot = true;
                if (!JumpInfo->DoneDelaySlot)
                {
                    FallInfo->FallThrough = false;
                    FallInfo->LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(FallInfo->BranchLabel.c_str(), FallInfo->LinkLocation);

                    if (JumpInfo->LinkLocation.isValid())
                    {
                        LinkJump(*JumpInfo);
                        JumpInfo->FallThrough = true;
                        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
                        m_RegWorkingSet = m_RegBeforeDelay;
                        return;
                    }
                }
            }
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
            CompileCop1Test();
            m_Assembler.AndConstToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", ~0x0003F000u);
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
            R4300iInstruction Instruction(m_CompilePC, m_Opcode.Value);
            uint32_t ReadReg1, ReadReg2;
            Instruction.ReadsGPR(ReadReg1, ReadReg2);

            if (ReadReg1 != 31 && ReadReg2 != 31)
            {
                m_RegWorkingSet.UnMap_GPR(31, false);
                m_RegWorkingSet.SetMipsRegLo(31, (uint32_t)(m_CompilePC + 8));
                m_RegWorkingSet.SetMipsRegState(31, CRegBase::STATE_CONST_32_SIGN);
            }
            else
            {
                m_Section->m_Cont.LinkAddress = (uint32_t)(m_CompilePC + 8);
                m_Section->m_Jump.LinkAddress = (uint32_t)(m_CompilePC + 8);
            }
        }

        Compile_BranchCompare(CompareType);
        m_RegWorkingSet.ResetRegisterProtection();

        m_Section->m_Cont.RegSet = m_RegWorkingSet;
        m_Section->m_Cont.RegSet.SetBlockCycleCount(m_Section->m_Cont.RegSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
        if (m_Section->m_Cont.LinkAddress != (uint32_t)-1)
        {
            m_Section->m_Cont.RegSet.UnMap_GPR(31, false);
            m_Section->m_Cont.RegSet.SetMipsRegLo(31, m_Section->m_Cont.LinkAddress);
            m_Section->m_Cont.RegSet.SetMipsRegState(31, CRegBase::STATE_CONST_32_SIGN);
            m_Section->m_Cont.LinkAddress = (uint32_t)-1;
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
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->m_Jump.RegSet.SetBlockCycleCount(m_Section->m_Jump.RegSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
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
        else if (m_Section->m_Cont.FallThrough)
        {
            if (m_Section->m_Jump.LinkLocation.isValid())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_Section->GenerateSectionLinkage();
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
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
    if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        const uint64_t RsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        const uint64_t RtValue = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        m_Section->m_Jump.FallThrough = RsValue != RtValue;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);

    // At most one of the registers is a constant, compare the other register against it
    const uint32_t CompareReg = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;
    const uint32_t OtherReg = CompareReg == m_Opcode.rs ? m_Opcode.rt : m_Opcode.rs;

    // Two 32-bit values extended the same way are equal when their low words are equal
    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt) &&
        m_RegWorkingSet.Is32Bit(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rt) &&
        m_RegWorkingSet.IsSigned(m_Opcode.rs) == m_RegWorkingSet.IsSigned(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(OtherReg))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(CompareReg).r32(), m_RegWorkingSet.GetMipsRegLo(OtherReg));
        }
        else
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(CompareReg).r32(), m_RegWorkingSet.GetMipsRegMap(OtherReg).r32());
        }
    }
    else
    {
        const asmjit::x86::Gp Compare64 = m_RegWorkingSet.Is64BitMapped(CompareReg) ? m_RegWorkingSet.GetMipsRegMap(CompareReg) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), CompareReg, asmjit::RegType::kX86_Gpq);
        if (m_RegWorkingSet.IsConst(OtherReg))
        {
            const uint64_t ConstValue = m_RegWorkingSet.Is64Bit(OtherReg) ? m_RegWorkingSet.GetMipsReg(OtherReg) : (m_RegWorkingSet.IsSigned(OtherReg) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(OtherReg) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(OtherReg));
            if (asmjit::Support::isInt32((int64_t)ConstValue))
            {
                m_Assembler.cmp(Compare64.r64(), (int32_t)ConstValue);
            }
            else
            {
                const asmjit::x86::Gp Const64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                m_Assembler.MoveConstToX64reg(Const64, ConstValue);
                m_Assembler.cmp(Compare64.r64(), Const64.r64());
            }
        }
        else
        {
            const asmjit::x86::Gp Other64 = m_RegWorkingSet.Is64BitMapped(OtherReg) ? m_RegWorkingSet.GetMipsRegMap(OtherReg) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), OtherReg, asmjit::RegType::kX86_Gpq);
            m_Assembler.cmp(Compare64.r64(), Other64.r64());
        }
    }

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

void CX64RecompilerOps::BEQ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        const uint64_t RsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        const uint64_t RtValue = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        m_Section->m_Jump.FallThrough = RsValue == RtValue;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);

    // At most one of the registers is a constant, compare the other register against it
    const uint32_t CompareReg = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;
    const uint32_t OtherReg = CompareReg == m_Opcode.rs ? m_Opcode.rt : m_Opcode.rs;

    // Two 32-bit values extended the same way are equal when their low words are equal
    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt) &&
        m_RegWorkingSet.Is32Bit(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rt) &&
        m_RegWorkingSet.IsSigned(m_Opcode.rs) == m_RegWorkingSet.IsSigned(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(OtherReg))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(CompareReg).r32(), m_RegWorkingSet.GetMipsRegLo(OtherReg));
        }
        else
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(CompareReg).r32(), m_RegWorkingSet.GetMipsRegMap(OtherReg).r32());
        }
    }
    else
    {
        const asmjit::x86::Gp Compare64 = m_RegWorkingSet.Is64BitMapped(CompareReg) ? m_RegWorkingSet.GetMipsRegMap(CompareReg) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), CompareReg, asmjit::RegType::kX86_Gpq);
        if (m_RegWorkingSet.IsConst(OtherReg))
        {
            const uint64_t ConstValue = m_RegWorkingSet.Is64Bit(OtherReg) ? m_RegWorkingSet.GetMipsReg(OtherReg) : (m_RegWorkingSet.IsSigned(OtherReg) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(OtherReg) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(OtherReg));
            if (asmjit::Support::isInt32((int64_t)ConstValue))
            {
                m_Assembler.cmp(Compare64.r64(), (int32_t)ConstValue);
            }
            else
            {
                const asmjit::x86::Gp Const64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                m_Assembler.MoveConstToX64reg(Const64, ConstValue);
                m_Assembler.cmp(Compare64.r64(), Const64.r64());
            }
        }
        else
        {
            const asmjit::x86::Gp Other64 = m_RegWorkingSet.Is64BitMapped(OtherReg) ? m_RegWorkingSet.GetMipsRegMap(OtherReg) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), OtherReg, asmjit::RegType::kX86_Gpq);
            m_Assembler.cmp(Compare64.r64(), Other64.r64());
        }
    }

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

void CX64RecompilerOps::BGTZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        m_Section->m_Jump.FallThrough = Value > 0;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        return;
    }

    // A zero extended 32-bit value is never negative, so it is only > 0 when it is not zero
    const bool ZeroExtended = m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && !m_RegWorkingSet.IsSigned(m_Opcode.rs);
    if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
    {
        m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), 0);
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
        const asmjit::x86::Gp Rs64 = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        m_Assembler.cmp(Rs64.r64(), 0);
    }

    if (m_Section->m_Jump.FallThrough)
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        if (ZeroExtended)
        {
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else
        {
            m_Assembler.JleLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
    }
    else if (m_Section->m_Cont.FallThrough)
    {
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        if (ZeroExtended)
        {
            m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else
        {
            m_Assembler.JgLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
    else
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        if (ZeroExtended)
        {
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else
        {
            m_Assembler.JleLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
}

void CX64RecompilerOps::BLEZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        m_Section->m_Jump.FallThrough = Value <= 0;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        return;
    }

    // A zero extended 32-bit value is never negative, so it is only <= 0 when it is zero
    const bool ZeroExtended = m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && !m_RegWorkingSet.IsSigned(m_Opcode.rs);
    if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
    {
        m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), 0);
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
        const asmjit::x86::Gp Rs64 = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        m_Assembler.cmp(Rs64.r64(), 0);
    }

    if (m_Section->m_Jump.FallThrough)
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        if (ZeroExtended)
        {
            m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else
        {
            m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
    }
    else if (m_Section->m_Cont.FallThrough)
    {
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        if (ZeroExtended)
        {
            m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else
        {
            m_Assembler.JleLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
    else
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        if (ZeroExtended)
        {
            m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else
        {
            m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
}

void CX64RecompilerOps::BLTZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        m_Section->m_Jump.FallThrough = Value < 0;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        return;
    }

    if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && !m_RegWorkingSet.IsSigned(m_Opcode.rs))
    {
        // A zero extended 32-bit value is never negative
        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Cont.FallThrough = true;
        return;
    }

    // Only the sign matters: a sign extended 32-bit value, the full 64-bit value or the high word in memory
    if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r64(), 0);
        }
        else
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), 0);
        }
    }
    else
    {
        m_Assembler.CmpConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
    }

    if (m_Section->m_Jump.FallThrough)
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
    }
    else if (m_Section->m_Cont.FallThrough)
    {
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
    else
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
}

void CX64RecompilerOps::BGEZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        m_Section->m_Jump.FallThrough = Value >= 0;
        m_Section->m_Cont.FallThrough = !m_Section->m_Jump.FallThrough;
        return;
    }

    if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && !m_RegWorkingSet.IsSigned(m_Opcode.rs))
    {
        // A zero extended 32-bit value is never negative
        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Cont.FallThrough = false;
        return;
    }

    // Only the sign matters: a sign extended 32-bit value, the full 64-bit value or the high word in memory
    if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r64(), 0);
        }
        else
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), 0);
        }
    }
    else
    {
        m_Assembler.CmpConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
    }

    if (m_Section->m_Cont.FallThrough)
    {
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JgeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
    }
    else
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
}

void CX64RecompilerOps::COP1_BCF_Compare()
{
    m_Assembler.TestVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", FPCSR_C);
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

void CX64RecompilerOps::COP1_BCT_Compare()
{
    m_Assembler.TestVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", FPCSR_C);
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

void CX64RecompilerOps::J()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            m_Assembler.MoveConst64ToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (uint64_t)(int64_t)(int32_t)((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2)));
            OverflowDelaySlot(false);
            return;
        }
        R4300iOpcode DelaySlot;
        g_MMU->MemoryValue32((uint32_t)(m_CompilePC + 4), DelaySlot.Value);
        if (R4300iInstruction(m_CompilePC + 4, DelaySlot.Value).HasDelaySlot())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
        m_Section->m_Jump.JumpPC = (uint32_t)(m_CompilePC);
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
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->GenerateSectionLinkage();
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (g_DebugSettings.haveDebugger)
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nJ\nNextInstruction = %X", m_PipelineStage).c_str());
    }
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
            m_Assembler.MoveConst64ToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (uint64_t)(int64_t)(int32_t)((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2)));
            OverflowDelaySlot(false);
            return;
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
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int32_t rs = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs);
        const int32_t imm = (int16_t)m_Opcode.immediate;
        const int32_t sum = rs + imm;
        if ((~(rs ^ imm) & (rs ^ sum)) & 0x80000000)
        {
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
            return;
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

    // The memory stack is only updated once the overflow check has passed and sp has been written
    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        if (m_Opcode.rs == 29)
        {
            const asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(asmjit::x86::Gpq(), true, true);
            m_Assembler.add(StackReg.r64(), (int32_t)((int16_t)m_Opcode.immediate));
        }
        else
        {
            m_RegWorkingSet.ResetRegisterProtection();
            ResetMemoryStack();
        }
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
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::SLTI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    const int64_t Imm = (int16_t)m_Opcode.immediate;
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t RsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, RsValue < Imm ? 1 : 0);
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

    // rt can only be written before the compare if it is not also the source register
    const bool preMap = m_Opcode.rt != m_Opcode.rs;
    asmjit::x86::Gp Result;
    if (preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        Result = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32();
    }
    else
    {
        Result = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    }
    m_Assembler.xor_(Result.r32(), Result.r32());

    if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && m_RegWorkingSet.IsSigned(m_Opcode.rs))
    {
        // A sign extended 32-bit value compares the same as its 64-bit form against the sign extended immediate
        m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), (int32_t)Imm);
    }
    else
    {
        const asmjit::x86::Gp Rs64 = m_RegWorkingSet.Is64BitMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        m_Assembler.cmp(Rs64.r64(), (int32_t)Imm);
    }
    m_Assembler.setl(Result.r8Lo());

    if (!preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), Result.r32());
    }
}

void CX64RecompilerOps::SLTIU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    // The immediate is sign extended to 64 bits and then compared unsigned
    const uint64_t Imm = (uint64_t)(int64_t)(int16_t)m_Opcode.immediate;
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const uint64_t RsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, RsValue < Imm ? 1 : 0);
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

    // rt can only be written before the compare if it is not also the source register
    const bool preMap = m_Opcode.rt != m_Opcode.rs;
    asmjit::x86::Gp Result;
    if (preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        Result = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32();
    }
    else
    {
        Result = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    }
    m_Assembler.xor_(Result.r32(), Result.r32());

    if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && m_RegWorkingSet.IsSigned(m_Opcode.rs))
    {
        // Two sign extended 32-bit values compare the same unsigned as their 64-bit forms
        m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), (int32_t)(int16_t)m_Opcode.immediate);
    }
    else
    {
        const asmjit::x86::Gp Rs64 = m_RegWorkingSet.Is64BitMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        m_Assembler.cmp(Rs64.r64(), (int32_t)(int16_t)m_Opcode.immediate);
    }
    m_Assembler.setb(Result.r8Lo());

    if (!preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), Result.r32());
    }
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
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
    {
        // The immediate is zero extended and only sets the low 16 bits, so the sign extension of rs is kept
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, m_RegWorkingSet.IsSigned(m_Opcode.rs), m_Opcode.rs);
        if (m_Opcode.immediate != 0)
        {
            m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), m_Opcode.immediate);
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        if (m_Opcode.immediate != 0)
        {
            m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r64(), m_Opcode.immediate);
        }
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::XORI()
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

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, m_RegWorkingSet.GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) ^ m_Opcode.immediate);
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
    {
        // The immediate is zero extended and only changes the low 16 bits, so the sign extension of rs is kept
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, m_RegWorkingSet.IsSigned(m_Opcode.rs), m_Opcode.rs);
        if (m_Opcode.immediate != 0)
        {
            m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), m_Opcode.immediate);
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        if (m_Opcode.immediate != 0)
        {
            m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r64(), m_Opcode.immediate);
        }
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
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
    const int64_t Imm = (int16_t)m_Opcode.immediate;
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        const int64_t Result = (int64_t)((uint64_t)Rs + (uint64_t)Imm);
        if ((~(Rs ^ Imm) & (Rs ^ Result)) & 0x8000000000000000ll)
        {
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
            return;
        }
        if (m_Opcode.rt == 0)
        {
            return;
        }
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rt, (uint64_t)Result);
        if ((m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == -1) ||
            (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == 0))
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_64);
        }
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
        const asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        m_Assembler.add(Reg.r64(), (int32_t)Imm);

        // rt is not written when the addition overflows
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow, &CX64Ops::JoLabel);
        if (m_Opcode.rt == 0)
        {
            return;
        }
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r64(), Reg.r64());
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::DADDIU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    const int64_t Imm = (int16_t)m_Opcode.immediate;
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        const int64_t Result = (int64_t)((uint64_t)Rs + (uint64_t)Imm);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rt, (uint64_t)Result);
        if ((m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == -1) ||
            (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == 0))
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_64);
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        if (Imm != 0)
        {
            m_Assembler.add(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r64(), (int32_t)Imm);
        }
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
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
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveReadBP && g_Debugger != nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        // A known address directly in RDRAM can be read without going through the memory map,
        // everything else uses the general path below
        uint32_t PAddr = 0;
        if (Address >= 0x80000000 && Address < 0xC0000000 && m_MMU.VAddrToPAddr(Address, PAddr) && PAddr < m_MMU.RdramSize())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
            LB_KnownAddress(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), Address, true);
            return;
        }
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    asmjit::x86::Gp AddressReg;
    CompileLoadMemoryValue(AddressReg, m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), asmjit::x86::Gp(), 8, true);
}

void CX64RecompilerOps::LH()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveReadBP && g_Debugger != nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        // An aligned known address directly in RDRAM can be read without going through the memory map,
        // everything else (including the address error for a misaligned address) uses the general path below
        uint32_t PAddr = 0;
        if ((Address & 1) == 0 && Address >= 0x80000000 && Address < 0xC0000000 && m_MMU.VAddrToPAddr(Address, PAddr) && PAddr < m_MMU.RdramSize())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
            LH_KnownAddress(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), Address, true);
            return;
        }
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    asmjit::x86::Gp AddressReg;
    CompileLoadMemoryValue(AddressReg, m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), asmjit::x86::Gp(), 16, true);
}

void CX64RecompilerOps::LWL()
{
    LoadWordPartial(true);
}

void CX64RecompilerOps::LW()
{
    if (m_Opcode.base == 29 && g_GameSettings.fastSP && m_Opcode.rt != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        const asmjit::x86::Gp & DestReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
        const asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(asmjit::x86::Gpq(), true, true);
        m_Assembler.movsxd(DestReg.r64(), asmjit::x86::dword_ptr(StackReg, (int32_t)((int16_t)m_Opcode.offset)));
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
    const asmjit::x86::Gp AddressReg = BaseOffsetAddress();
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
        m_Assembler.movsxd(DestReg.r64(), asmjit::x86::dword_ptr(AddressReg, HostOffsetReg));
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
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveReadBP && g_Debugger != nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        // A known address directly in RDRAM can be read without going through the memory map,
        // everything else uses the general path below
        uint32_t PAddr = 0;
        if (Address >= 0x80000000 && Address < 0xC0000000 && m_MMU.VAddrToPAddr(Address, PAddr) && PAddr < m_MMU.RdramSize())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            LB_KnownAddress(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), Address, false);
            return;
        }
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    asmjit::x86::Gp AddressReg;
    CompileLoadMemoryValue(AddressReg, m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), asmjit::x86::Gp(), 8, false);
}

void CX64RecompilerOps::LHU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveReadBP && g_Debugger != nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        // An aligned known address directly in RDRAM can be read without going through the memory map,
        // everything else (including the address error for a misaligned address) uses the general path below
        uint32_t PAddr = 0;
        if ((Address & 1) == 0 && Address >= 0x80000000 && Address < 0xC0000000 && m_MMU.VAddrToPAddr(Address, PAddr) && PAddr < m_MMU.RdramSize())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            LH_KnownAddress(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), Address, false);
            return;
        }
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    asmjit::x86::Gp AddressReg;
    CompileLoadMemoryValue(AddressReg, m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), asmjit::x86::Gp(), 16, false);
}

void CX64RecompilerOps::LWR()
{
    LoadWordPartial(false);
}

void CX64RecompilerOps::LWU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SB()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveWriteBP)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        // A known address directly in RDRAM can be written without going through the memory map,
        // everything else uses the general path below
        uint32_t PAddr = 0;
        if (Address >= 0x80000000 && Address < 0xC0000000 && !g_GameSettings.smmStoreInstruc && m_MMU.VAddrToPAddr(Address, PAddr) && PAddr < m_MMU.RdramSize())
        {
            if (m_RegWorkingSet.IsConst(m_Opcode.rt))
            {
                SB_KnownAddress(Address, nullptr, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
                SB_KnownAddress(Address, &m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), 0);
            }
            else
            {
                const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
                SB_KnownAddress(Address, &TempReg, 0);
            }
            return;
        }
    }

    asmjit::x86::Gp ValueReg;
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        ValueReg = m_RegWorkingSet.IsUnknown(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt) : m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
    }
    CompileStoreMemoryValue(asmjit::x86::Gp(), ValueReg, asmjit::x86::Gp(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), 8);
}

void CX64RecompilerOps::SH()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveWriteBP)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        // An aligned known address directly in RDRAM can be written without going through the memory map,
        // everything else (including the address error for a misaligned address) uses the general path below
        uint32_t PAddr = 0;
        if ((Address & 1) == 0 && Address >= 0x80000000 && Address < 0xC0000000 && !g_GameSettings.smmStoreInstruc && m_MMU.VAddrToPAddr(Address, PAddr) && PAddr < m_MMU.RdramSize())
        {
            if (m_RegWorkingSet.IsConst(m_Opcode.rt))
            {
                SH_KnownAddress(Address, nullptr, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
                SH_KnownAddress(Address, &m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt), 0);
            }
            else
            {
                const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
                SH_KnownAddress(Address, &TempReg, 0);
            }
            return;
        }
    }

    asmjit::x86::Gp ValueReg;
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        ValueReg = m_RegWorkingSet.IsUnknown(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt) : m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
    }
    CompileStoreMemoryValue(asmjit::x86::Gp(), ValueReg, asmjit::x86::Gp(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), 16);
}

void CX64RecompilerOps::SWL()
{
    StoreWordPartial(true);
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

    const asmjit::x86::Gp AddressReg = BaseOffsetAddress();
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
    StoreWordPartial(false);
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
    if (g_Settings->LoadDword(Game_SMM_Cache) == 0)
    {
        return;
    }

    switch (m_Opcode.rt)
    {
    case 0:
    case 16:
        m_RegWorkingSet.BeforeCallDirect();
        if (m_RegWorkingSet.IsConst(m_Opcode.base))
        {
            const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            m_Assembler.mov(asmjit::x86::edx, Address);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.base))
        {
            m_Assembler.lea(asmjit::x86::edx, asmjit::x86::dword_ptr(m_RegWorkingSet.GetMipsRegMap(m_Opcode.base).r32(), (int32_t)(int16_t)m_Opcode.offset));
        }
        else
        {
            m_Assembler.MoveVariableToX64reg(asmjit::x86::edx, &m_Reg.m_GPR[m_Opcode.base].UW[0], CRegName::GPR_Lo[m_Opcode.base], false);
            m_Assembler.add(asmjit::x86::edx, (int32_t)(int16_t)m_Opcode.offset);
        }
        m_Assembler.mov(asmjit::x86::r8d, 0x20);
        m_Assembler.mov(asmjit::x86::r9d, (uint32_t)CRecompiler::Remove_Cache);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(m_Recompiler, MemberFuncAddress(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 1:
    case 3:
    case 13:
    case 5:
    case 8:
    case 9:
    case 17:
    case 21:
    case 25:
        break;
    default:
        if (g_DebugSettings.haveDebugger)
        {
            g_Notify->DisplayError(stdstr_f("cache: %d", m_Opcode.rt).c_str());
        }
        break;
    }
}

void CX64RecompilerOps::LL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWC1()
{
    CompileCop1Test();

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    asmjit::x86::Gp AddressReg;
    CompileLoadMemoryValue(AddressReg, ValueReg, asmjit::x86::Gp(), 32, false);

    const asmjit::x86::Gp FprPtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(FprPtrReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("m_FPR_S[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(asmjit::x86::dword_ptr(FprPtrReg), ValueReg.r32());
}

void CX64RecompilerOps::LDC1()
{
    CompileCop1Test();

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    asmjit::x86::Gp AddressReg;
    CompileLoadMemoryValue(AddressReg, ValueReg, asmjit::x86::Gp(), 64, false);

    const asmjit::x86::Gp FprPtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(FprPtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("m_FPR_D[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(FprPtrReg), ValueReg.r64());
}

void CX64RecompilerOps::LD()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_Opcode.base == 29 && g_GameSettings.fastSP)
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
        const asmjit::x86::Gp & DestReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
        const asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(asmjit::x86::Gpq(), true, true);
        m_Assembler.mov(DestReg.r64(), asmjit::x86::qword_ptr(StackReg, (int32_t)((int16_t)m_Opcode.offset)));
        m_Assembler.ror(DestReg.r64(), 32);
    }
    else if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.base);
        if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        asmjit::x86::Gp AddressReg;
        CompileLoadMemoryValue(AddressReg, asmjit::x86::Gp(), asmjit::x86::Gp(), 64, false);
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::SC()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SWC1()
{
    CompileCop1Test();

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(ValueReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("m_FPR_S[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(ValueReg.r32(), asmjit::x86::dword_ptr(ValueReg));
    CompileStoreMemoryValue(asmjit::x86::Gp(), ValueReg, asmjit::x86::Gp(), 0, 32);
}

void CX64RecompilerOps::SDC1()
{
    CompileCop1Test();

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(ValueReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("m_FPR_D[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(ValueReg.r64(), asmjit::x86::qword_ptr(ValueReg));
    CompileStoreMemoryValue(asmjit::x86::Gp(), ValueReg, asmjit::x86::Gp(), 0, 64);
}

void CX64RecompilerOps::SD()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        const uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (g_DebugSettings.haveWriteBP)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else
        {
            asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rt, asmjit::RegType::kX86_Gpq);
            asmjit::x86::Gp HiReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
            m_Assembler.mov(HiReg.r64(), TempReg.r64());
            m_Assembler.shr(HiReg.r64(), 32);
            SW_KnownAddress(Address, &HiReg, 0);
            SW_KnownAddress(Address + 4, &TempReg, 0);
        }
    }
    else
    {
        asmjit::x86::Gp ValueReg;
        if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            }
            if (m_RegWorkingSet.IsUnknown(m_Opcode.rt) || !m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rt, asmjit::RegType::kX86_Gpq);
            }
            else
            {
                ValueReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
            }
        }
        uint64_t RtValue = 0;
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            RtValue = ((uint64_t)(m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt) : (uint32_t)(m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> 31)) << 32) | m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
        }
        CompileStoreMemoryValue(asmjit::x86::Gp(), ValueReg, asmjit::x86::Gp(), RtValue, 64);
    }
}

void CX64RecompilerOps::SPECIAL_SLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shl(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), (uint8_t)m_Opcode.sa);
    }
}

void CX64RecompilerOps::SPECIAL_SRL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shr(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), (uint8_t)m_Opcode.sa);
    }
}

void CX64RecompilerOps::SPECIAL_SRA()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        const int64_t RtValue = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)(RtValue >> m_Opcode.sa));
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.Is32Bit(m_Opcode.rt))
    {
        // The low word of the 64-bit shift only depends on the high word through the sign/zero extension
        const bool Signed = m_RegWorkingSet.IsSigned(m_Opcode.rt);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        if (Signed)
        {
            m_Assembler.sar(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), (uint8_t)m_Opcode.sa);
        }
        else
        {
            m_Assembler.shr(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), (uint8_t)m_Opcode.sa);
        }
    }
    else
    {
        // Shift the full 64-bit value, the result is the sign extended low word
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.sar(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64(), (uint8_t)m_Opcode.sa);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rd);
    }
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
            const asmjit::x86::Gp TargetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
            m_Assembler.MovQwordToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", TargetReg);
            OverflowDelaySlot(true);
            return;
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
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32((uint32_t)(m_CompilePC + 4), DelaySlot.Value) &&
            R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value) && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            const asmjit::x86::Gp TargetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
            m_Assembler.MovQwordToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", TargetReg);
            if (m_Opcode.rd != 0)
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)(m_CompilePC + 8));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            OverflowDelaySlot(true);
            return;
        }

        if (m_Opcode.rd != 0)
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)(m_CompilePC + 8));
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();

        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32((uint32_t)(m_CompilePC + 4), DelaySlot.Value) && R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, ExitReason_CheckPCAlignment);
        }
        else
        {
            if (m_RegWorkingSet.IsConst(m_Opcode.rs))
            {
                if (((int)m_CompilePC >> 31) != (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? (int)m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs) : (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) >> 31)))
                {
                    m_Assembler.MoveConst64ToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs));
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
                    m_Assembler.MovQwordToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs));
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
                m_Section->GenerateSectionLinkage();
            }
        }
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (g_DebugSettings.haveDebugger)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
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
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs));
        }
        else
        {
            const asmjit::x86::Gp Val64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
            m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", Val64);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_MFHI()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, -1);
    m_Assembler.MoveVariable64ToX64reg(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd), &m_Reg.m_HI.UDW, "RegHI.UDW");
}

void CX64RecompilerOps::SPECIAL_MTHI()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        const asmjit::x86::Gp Val64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        m_Assembler.MovQwordToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", Val64);
    }
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
    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);
        m_RegWorkingSet.UnMap_X64reg(asmjit::x86::edx);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), true);
        m_RegWorkingSet.UnMap_X64reg(asmjit::x86::eax);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);

        asmjit::x86::Gp RegRs = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rs);
        m_RegWorkingSet.SetX64Protected(RegRs.id(), true);
        asmjit::x86::Gp DivReg = m_RegWorkingSet.IsMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
        m_RegWorkingSet.SetX64Protected(DivReg.id(), true);

        asmjit::Label JumpEnd, JumpEnd2;
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (!m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.cmp(DivReg.r32(), 0);
            asmjit::Label JumpNotDiv0 = m_Assembler.newLabel();
            m_Assembler.JneLabel(stdstr_f("NotDiv0_%08X", m_CompilePC).c_str(), JumpNotDiv0);

            m_Assembler.cmp(RegRs.r32(), 0);
            asmjit::Label JumpPositive = m_Assembler.newLabel();
            m_Assembler.JgeLabel(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), JumpPositive);
            m_Assembler.MoveConst64ToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", 1ull);
            asmjit::Label JumpLoSet = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), JumpLoSet);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpPositive);
            m_Assembler.MoveConst64ToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", (uint64_t)-1);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpLoSet);
            m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[0], "RegHI.UW[0]", RegRs);
            const asmjit::x86::Gp SignReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
            m_Assembler.mov(SignReg.r32(), RegRs.r32());
            m_Assembler.sar(SignReg.r32(), 31);
            m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[1], "RegHI.UW[1]", SignReg);
            JumpEnd = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), JumpEnd);

            m_CodeBlock.Log("");
            m_Assembler.bind(JumpNotDiv0);
            m_Assembler.cmp(DivReg.r32(), (uint32_t)-1);
            asmjit::Label JumpValidDiv0 = m_Assembler.newLabel();
            m_Assembler.JneLabel(stdstr_f("ValidDiv0_%08X", m_CompilePC).c_str(), JumpValidDiv0);

            m_Assembler.cmp(RegRs.r32(), (uint32_t)0x80000000);
            asmjit::Label JumpValidDiv1 = m_Assembler.newLabel();
            m_Assembler.JneLabel(stdstr_f("ValidDiv1_%08X", m_CompilePC).c_str(), JumpValidDiv1);

            m_Assembler.MoveConst64ToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", 0xFFFFFFFF80000000ull);
            m_Assembler.MoveConst64ToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", 0);
            JumpEnd2 = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), JumpEnd2);

            m_CodeBlock.Log("");
            m_Assembler.bind(JumpValidDiv0);
            m_Assembler.bind(JumpValidDiv1);
        }
        m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), false);
        m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, -1);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
        m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs);
        m_Assembler.cdq();
        m_Assembler.idiv(DivReg.r32());

        m_Assembler.MovDwordToVariable(&m_Reg.m_LO.UW[0], "RegLO.UW[0]", asmjit::x86::eax);
        m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[0], "RegHI.UW[0]", asmjit::x86::edx);
        m_Assembler.sar(asmjit::x86::eax, 31);
        m_Assembler.sar(asmjit::x86::edx, 31);
        m_Assembler.MovDwordToVariable(&m_Reg.m_LO.UW[1], "RegLO.UW[1]", asmjit::x86::eax);
        m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[1], "RegHI.UW[1]", asmjit::x86::edx);

        if (JumpEnd.isValid())
        {
            m_Assembler.bind(JumpEnd);
        }
        if (JumpEnd2.isValid())
        {
            m_Assembler.bind(JumpEnd2);
        }
    }
}

void CX64RecompilerOps::SPECIAL_DIVU()
{
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);
    m_RegWorkingSet.UnMap_X64reg(asmjit::x86::edx);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), true);
    m_RegWorkingSet.UnMap_X64reg(asmjit::x86::eax);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);

    const asmjit::x86::Gp RegRs = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rs);
    m_RegWorkingSet.SetX64Protected(RegRs.id(), true);
    const asmjit::x86::Gp DivReg = m_RegWorkingSet.IsMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
    m_RegWorkingSet.SetX64Protected(DivReg.id(), true);

    // Dividing by zero sets LO to -1 and HI to the sign extended dividend
    const bool RtConstNonZero = m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) != 0;
    asmjit::Label JumpEnd;
    if (!RtConstNonZero)
    {
        m_Assembler.test(DivReg.r32(), DivReg.r32());
        asmjit::Label JumpNotDiv0 = m_Assembler.newLabel();
        m_Assembler.JneLabel(stdstr_f("NotDiv0_%08X", m_CompilePC).c_str(), JumpNotDiv0);

        m_Assembler.MoveConst64ToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", (uint64_t)-1);
        m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[0], "RegHI.UW[0]", RegRs);
        const asmjit::x86::Gp SignReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
        m_Assembler.mov(SignReg.r32(), RegRs.r32());
        m_Assembler.sar(SignReg.r32(), 31);
        m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[1], "RegHI.UW[1]", SignReg);
        JumpEnd = m_Assembler.newLabel();
        m_Assembler.JmpLabel(stdstr_f("EndDivu_%08X", m_CompilePC).c_str(), JumpEnd);

        m_CodeBlock.Log("");
        m_Assembler.bind(JumpNotDiv0);
    }

    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, -1);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs);
    m_Assembler.xor_(asmjit::x86::edx, asmjit::x86::edx);
    m_Assembler.div(DivReg.r32());

    // The quotient and remainder are each sign extended from 32 bits
    m_Assembler.MovDwordToVariable(&m_Reg.m_LO.UW[0], "RegLO.UW[0]", asmjit::x86::eax);
    m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[0], "RegHI.UW[0]", asmjit::x86::edx);
    m_Assembler.sar(asmjit::x86::eax, 31);
    m_Assembler.sar(asmjit::x86::edx, 31);
    m_Assembler.MovDwordToVariable(&m_Reg.m_LO.UW[1], "RegLO.UW[1]", asmjit::x86::eax);
    m_Assembler.MovDwordToVariable(&m_Reg.m_HI.UW[1], "RegHI.UW[1]", asmjit::x86::edx);

    if (JumpEnd.isValid())
    {
        m_CodeBlock.Log("");
        m_Assembler.bind(JumpEnd);
    }
}

void CX64RecompilerOps::SPECIAL_DMULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DMULTU()
{
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), true);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::rax, m_Opcode.rs, asmjit::RegType::kX86_Gpq);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::rdx, m_Opcode.rt, asmjit::RegType::kX86_Gpq);

    m_Assembler.mul(asmjit::x86::rdx);

    m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", asmjit::x86::rax);
    m_Assembler.MovQwordToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", asmjit::x86::rdx);
}

void CX64RecompilerOps::SPECIAL_DDIV()
{
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);
    m_RegWorkingSet.UnMap_X64reg(asmjit::x86::rdx);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), true);
    m_RegWorkingSet.UnMap_X64reg(asmjit::x86::rax);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);
    const asmjit::x86::Gp DivReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rt, asmjit::RegType::kX86_Gpq);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::rdx, -1, asmjit::RegType::kX86_Gpq);
    m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::rax, m_Opcode.rs, asmjit::RegType::kX86_Gpq);

    asmjit::Label JumpEnd = m_Assembler.newLabel();
    const bool RtIsConstNonZero = m_RegWorkingSet.IsConst(m_Opcode.rt) && (m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) != 0 : m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) != 0);
    if (!RtIsConstNonZero)
    {
        // Dividing by zero sets LO to 1 for a negative dividend and -1 otherwise, HI to the dividend
        asmjit::Label JumpNotDiv0 = m_Assembler.newLabel();
        m_Assembler.test(DivReg.r64(), DivReg.r64());
        m_Assembler.JneLabel("NotDiv0", JumpNotDiv0);
        m_Assembler.MovQwordToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", asmjit::x86::rax);
        m_Assembler.mov(asmjit::x86::rdx, asmjit::x86::rax);
        m_Assembler.sar(asmjit::x86::rdx, 63);
        m_Assembler.shl(asmjit::x86::rdx, 1);
        m_Assembler.not_(asmjit::x86::rdx);
        m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", asmjit::x86::rdx);
        m_Assembler.JmpLabel("EndDdiv", JumpEnd);
        m_CodeBlock.Log("");
        m_Assembler.bind(JumpNotDiv0);
    }

    // A dividend of 0x8000000000000000 sets LO to the dividend and HI to 0, this also avoids the idiv overflow for -1
    asmjit::Label JumpDivide = m_Assembler.newLabel();
    m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, 0x8000000000000000ull);
    m_Assembler.cmp(asmjit::x86::rax, asmjit::x86::rdx);
    m_Assembler.JneLabel("Divide", JumpDivide);
    m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", asmjit::x86::rax);
    m_Assembler.MoveConst64ToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", 0);
    m_Assembler.JmpLabel("EndDdiv", JumpEnd);
    m_CodeBlock.Log("");
    m_Assembler.bind(JumpDivide);

    m_Assembler.cqo();
    m_Assembler.idiv(DivReg.r64());
    m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", asmjit::x86::rax);
    m_Assembler.MovQwordToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", asmjit::x86::rdx);
    m_CodeBlock.Log("");
    m_Assembler.bind(JumpEnd);
}

void CX64RecompilerOps::SPECIAL_DDIVU()
{
    const bool RtIsConstZero = m_RegWorkingSet.IsConst(m_Opcode.rt) && (m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) == 0 : m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == 0);
    if (RtIsConstZero)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);
        m_RegWorkingSet.UnMap_X64reg(asmjit::x86::rdx);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), true);
        m_RegWorkingSet.UnMap_X64reg(asmjit::x86::rax);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), true);
        const asmjit::x86::Gp DivReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rt, asmjit::RegType::kX86_Gpq);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::edx.id(), false);
        m_RegWorkingSet.Map_TempReg(asmjit::x86::rdx, -1, asmjit::RegType::kX86_Gpq);
        m_RegWorkingSet.SetX64Protected(asmjit::x86::eax.id(), false);
        m_RegWorkingSet.Map_TempReg(asmjit::x86::rax, m_Opcode.rs, asmjit::RegType::kX86_Gpq);

        asmjit::Label JumpEnd;
        if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            asmjit::Label JumpNoExcept = m_Assembler.newLabel();
            m_Assembler.test(DivReg.r64(), DivReg.r64());
            m_Assembler.JneLabel("NoExcept", JumpNoExcept);

            m_Assembler.MoveConst64ToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", (uint64_t)-1);
            m_Assembler.MovQwordToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", asmjit::x86::rax);
            JumpEnd = m_Assembler.newLabel();
            m_Assembler.JmpLabel("EndDivu", JumpEnd);

            m_CodeBlock.Log("");
            m_Assembler.bind(JumpNoExcept);
        }

        m_Assembler.xor_(asmjit::x86::rdx, asmjit::x86::rdx);
        m_Assembler.div(DivReg.r64());
        m_Assembler.MovQwordToVariable(&m_Reg.m_LO.UDW, "RegLO.UDW", asmjit::x86::rax);
        m_Assembler.MovQwordToVariable(&m_Reg.m_HI.UDW, "RegHI.UDW", asmjit::x86::rdx);

        if (JumpEnd.isValid())
        {
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpEnd);
        }
    }
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
        const uint32_t temp = m_RegWorkingSet.GetMipsRegLo(source1) + m_RegWorkingSet.GetMipsRegLo(source2);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
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
        m_Assembler.AddDwordFromVariable(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd), &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX64RecompilerOps::SPECIAL_SUB()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        const int32_t Rs = m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
        const int32_t Rt = m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        const int32_t Result = (int32_t)((uint32_t)Rs - (uint32_t)Rt);

        // If the subtraction overflows the code below always raises the exception
        if ((((Rs ^ Rt) & (Rs ^ Result)) & 0x80000000) == 0)
        {
            if (m_Opcode.rd != 0)
            {
                if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
                {
                    m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
                }
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)Result);
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
            }
            return;
        }
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    const asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rs);
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.sub(Reg.r32(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_Assembler.sub(Reg.r32(), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
    }
    else
    {
        m_Assembler.SubVariableFromX64reg(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
    }

    // rd is not written when the subtraction overflows
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionOverflow, &CX64Ops::JoLabel);

    if (m_Opcode.rd != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), Reg.r32());
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::SPECIAL_SUBU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const uint32_t temp = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) - m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
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

    const bool RsConst = m_RegWorkingSet.IsConst(m_Opcode.rs);
    const bool RtConst = m_RegWorkingSet.IsConst(m_Opcode.rt);
    const uint64_t RsValue = !RsConst ? 0 : (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs)));
    const uint64_t RtValue = !RtConst ? 0 : (m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt)));

    if ((RsConst && RtConst) || (RsConst && RsValue == 0) || (RtConst && RtValue == 0))
    {
        // Both constant, or anded with zero, so the result is known
        const uint64_t Result = (RsConst ? RsValue : ~0ull) & (RtConst ? RtValue : ~0ull);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Result);
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
        // Load the non constant register into rd, if rd is also a source it has to be the one loaded
        uint32_t Source, Other;
        if (m_Opcode.rd == m_Opcode.rs && !RsConst)
        {
            Source = m_Opcode.rs;
        }
        else if (m_Opcode.rd == m_Opcode.rt && !RtConst)
        {
            Source = m_Opcode.rt;
        }
        else
        {
            Source = RsConst ? m_Opcode.rt : m_Opcode.rs;
        }
        Other = Source == m_Opcode.rs ? m_Opcode.rt : m_Opcode.rs;

        // Other can be rd when it is a constant, so read everything needed before rd gets mapped
        const bool OtherConst = Other == m_Opcode.rs ? RsConst : RtConst;
        const uint64_t OtherValue = Other == m_Opcode.rs ? RsValue : RtValue;
        const bool SameReg = m_Opcode.rs == m_Opcode.rt;
        const bool RsZeroExtended = m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && !m_RegWorkingSet.IsSigned(m_Opcode.rs);
        const bool RtZeroExtended = m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.Is32Bit(m_Opcode.rt) && !m_RegWorkingSet.IsSigned(m_Opcode.rt);
        const bool Both32Bit = m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rt);
        const bool BothSigned = Both32Bit && m_RegWorkingSet.IsSigned(m_Opcode.rs) && m_RegWorkingSet.IsSigned(m_Opcode.rt);

        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

        // Two sign extended values and to a sign extended value, and a zero extended operand has a high word
        // of zero so the result is zero extended whatever the other operand is
        if (Both32Bit || RsZeroExtended || RtZeroExtended)
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, BothSigned, Source);
            const asmjit::x86::Gp Rd32 = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32();
            if (OtherConst)
            {
                if ((uint32_t)OtherValue != 0xFFFFFFFF)
                {
                    m_Assembler.and_(Rd32, (uint32_t)OtherValue);
                }
            }
            else if (!SameReg)
            {
                if (m_RegWorkingSet.IsUnknown(Other))
                {
                    m_Assembler.AndVariableToX64reg(Rd32, &m_Reg.m_GPR[Other].UW[0], CRegName::GPR_Lo[Other]);
                }
                else
                {
                    m_Assembler.and_(Rd32, m_RegWorkingSet.GetMipsRegMap(Other).r32());
                }
            }
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, Source);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
            const asmjit::x86::Gp Rd64 = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64();
            if (OtherConst)
            {
                if (asmjit::Support::isInt32((int64_t)OtherValue))
                {
                    if (OtherValue != ~0ull)
                    {
                        m_Assembler.and_(Rd64, (int32_t)OtherValue);
                    }
                }
                else
                {
                    const asmjit::x86::Gp Temp64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                    m_Assembler.MoveConstToX64reg(Temp64, OtherValue);
                    m_Assembler.and_(Rd64, Temp64.r64());
                }
            }
            else if (!SameReg)
            {
                if (m_RegWorkingSet.IsUnknown(Other))
                {
                    m_Assembler.AndVariable64ToX64reg(Rd64, &m_Reg.m_GPR[Other].UDW, CRegName::GPR[Other]);
                }
                else if (m_RegWorkingSet.Is64Bit(Other))
                {
                    m_Assembler.and_(Rd64, m_RegWorkingSet.GetMipsRegMap(Other).r64());
                }
                else
                {
                    const asmjit::x86::Gp Other64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), Other, asmjit::RegType::kX86_Gpq);
                    m_Assembler.and_(Rd64, Other64.r64());
                }
            }
        }
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::SPECIAL_OR()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    CompileOr();
    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::CompileOr()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const uint64_t rtValue = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        const uint64_t rsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, rtValue | rsValue);
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
        // Load the non constant register into rd, if rd is also a source it has to be the one loaded
        uint32_t Source, Other;
        if (m_Opcode.rd == m_Opcode.rs && !m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            Source = m_Opcode.rs;
        }
        else if (m_Opcode.rd == m_Opcode.rt && !m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            Source = m_Opcode.rt;
        }
        else
        {
            Source = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;
        }
        Other = Source == m_Opcode.rs ? m_Opcode.rt : m_Opcode.rs;

        // Other can be rd when it is a constant, so read its value before rd gets mapped
        const bool OtherConst = m_RegWorkingSet.IsConst(Other);
        const uint64_t OtherValue = !OtherConst ? 0 : (m_RegWorkingSet.Is64Bit(Other) ? m_RegWorkingSet.GetMipsReg(Other) : (m_RegWorkingSet.IsSigned(Other) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(Other) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(Other)));
        // or with itself leaves the value unchanged
        const bool SameReg = m_Opcode.rs == m_Opcode.rt;

        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

        // Two 32-bit values extended the same way or to a value that is still extended that way
        if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs) &&
            m_RegWorkingSet.Is32Bit(m_Opcode.rt) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) &&
            m_RegWorkingSet.IsSigned(m_Opcode.rt) == m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(Source), Source);
            if (OtherConst)
            {
                if ((uint32_t)OtherValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), (uint32_t)OtherValue);
                }
            }
            else if (!SameReg)
            {
                m_Assembler.or_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegMap(Other).r32());
            }
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, Source);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
            const asmjit::x86::Gp Rd64 = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64();
            if (OtherConst)
            {
                if (asmjit::Support::isInt32((int64_t)OtherValue))
                {
                    if (OtherValue != 0)
                    {
                        m_Assembler.or_(Rd64, (int32_t)OtherValue);
                    }
                }
                else
                {
                    const asmjit::x86::Gp Temp64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                    m_Assembler.MoveConstToX64reg(Temp64, OtherValue);
                    m_Assembler.or_(Rd64, Temp64.r64());
                }
            }
            else if (!SameReg)
            {
                if (m_RegWorkingSet.IsUnknown(Other))
                {
                    m_Assembler.OrVariable64ToX64reg(Rd64, &m_Reg.m_GPR[Other].UDW, CRegName::GPR[Other]);
                }
                else if (m_RegWorkingSet.Is64Bit(Other))
                {
                    m_Assembler.or_(Rd64, m_RegWorkingSet.GetMipsRegMap(Other).r64());
                }
                else
                {
                    const asmjit::x86::Gp Other64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), Other, asmjit::RegType::kX86_Gpq);
                    m_Assembler.or_(Rd64, Other64.r64());
                }
            }
        }
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
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, 0);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const uint64_t rtValue = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        const uint64_t rsValue = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, rtValue ^ rsValue);
        if ((m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1) ||
            (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0))
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegBase::STATE_CONST_64);
        }
        return;
    }

    // Load the non constant register into rd, if rd is also a source it has to be the one loaded
    uint32_t Source, Other;
    if (m_Opcode.rd == m_Opcode.rs && !m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        Source = m_Opcode.rs;
    }
    else if (m_Opcode.rd == m_Opcode.rt && !m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        Source = m_Opcode.rt;
    }
    else
    {
        Source = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;
    }
    Other = Source == m_Opcode.rs ? m_Opcode.rt : m_Opcode.rs;

    // Other can be rd when it is a constant, so read its value before rd gets mapped
    const bool OtherConst = m_RegWorkingSet.IsConst(Other);
    const uint64_t OtherValue = !OtherConst ? 0 : (m_RegWorkingSet.Is64Bit(Other) ? m_RegWorkingSet.GetMipsReg(Other) : (m_RegWorkingSet.IsSigned(Other) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(Other) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(Other)));

    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

    // Two 32-bit values extended the same way xor to a value that is still extended that way
    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs) &&
        m_RegWorkingSet.Is32Bit(m_Opcode.rt) && m_RegWorkingSet.Is32Bit(m_Opcode.rs) &&
        m_RegWorkingSet.IsSigned(m_Opcode.rt) == m_RegWorkingSet.IsSigned(m_Opcode.rs))
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(Source), Source);
        if (OtherConst)
        {
            if ((uint32_t)OtherValue != 0)
            {
                m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), (uint32_t)OtherValue);
            }
        }
        else
        {
            m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), m_RegWorkingSet.GetMipsRegMap(Other).r32());
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, Source);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
        const asmjit::x86::Gp Rd64 = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64();
        if (OtherConst)
        {
            if (asmjit::Support::isInt32((int64_t)OtherValue))
            {
                if (OtherValue != 0)
                {
                    m_Assembler.xor_(Rd64, (int32_t)OtherValue);
                }
            }
            else
            {
                const asmjit::x86::Gp Temp64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
                m_Assembler.MoveConstToX64reg(Temp64, OtherValue);
                m_Assembler.xor_(Rd64, Temp64.r64());
            }
        }
        else if (m_RegWorkingSet.IsUnknown(Other))
        {
            m_Assembler.XorVariable64ToX64reg(Rd64, &m_Reg.m_GPR[Other].UDW, CRegName::GPR[Other]);
        }
        else if (m_RegWorkingSet.Is64Bit(Other))
        {
            m_Assembler.xor_(Rd64, m_RegWorkingSet.GetMipsRegMap(Other).r64());
        }
        else
        {
            const asmjit::x86::Gp Other64 = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), Other, asmjit::RegType::kX86_Gpq);
            m_Assembler.xor_(Rd64, Other64.r64());
        }
    }
}

void CX64RecompilerOps::SPECIAL_NOR()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    CompileOr();
    if (m_RegWorkingSet.IsConst(m_Opcode.rd))
    {
        const uint64_t Value = ~(m_RegWorkingSet.Is64Bit(m_Opcode.rd) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rd) : (m_RegWorkingSet.IsSigned(m_Opcode.rd) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rd)));
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value);
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
    else if (m_RegWorkingSet.Is32Bit(m_Opcode.rd) && m_RegWorkingSet.IsSigned(m_Opcode.rd))
    {
        // The complement of a sign extended value is still sign extended
        m_Assembler.not_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32());
    }
    else
    {
        // The complement of a zero extended value sets the high word, so it has to be 64-bit
        if (m_RegWorkingSet.Is32Bit(m_Opcode.rd))
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rd);
        }
        m_Assembler.not_(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64());
    }

    if (g_GameSettings.fastSP && m_Opcode.rd == 29)
    {
        m_RegWorkingSet.ResetRegisterProtection();
        ResetMemoryStack();
    }
}

void CX64RecompilerOps::SPECIAL_SLT()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const int64_t Rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        const int64_t Rt = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, Rs < Rt ? 1 : 0);
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

    // rd can only be written before the compare if it is not also a source register
    const bool preMap = (m_Opcode.rd != m_Opcode.rs && m_Opcode.rd != m_Opcode.rt);
    asmjit::x86::Gp Result;
    if (preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
        Result = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32();
    }
    else
    {
        Result = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    }
    m_Assembler.xor_(Result.r32(), Result.r32());

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt) &&
        !m_RegWorkingSet.Is64Bit(m_Opcode.rs) && !m_RegWorkingSet.Is64Bit(m_Opcode.rt) &&
        m_RegWorkingSet.IsSigned(m_Opcode.rs) == m_RegWorkingSet.IsSigned(m_Opcode.rt))
    {
        // Two sign extended 32-bit values compare the same signed as their 64-bit forms,
        // two zero extended 32-bit values are never negative so compare the same unsigned
        const bool Signed = m_RegWorkingSet.IsSigned(m_Opcode.rs);
        if (m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            if (Signed)
            {
                m_Assembler.setg(Result.r8Lo());
            }
            else
            {
                m_Assembler.seta(Result.r8Lo());
            }
        }
        else
        {
            if (m_RegWorkingSet.IsConst(m_Opcode.rt))
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
            }
            if (Signed)
            {
                m_Assembler.setl(Result.r8Lo());
            }
            else
            {
                m_Assembler.setb(Result.r8Lo());
            }
        }
    }
    else
    {
        const asmjit::x86::Gp Rs64 = m_RegWorkingSet.Is64BitMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        const asmjit::x86::Gp Rt64 = m_RegWorkingSet.Is64BitMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rt, asmjit::RegType::kX86_Gpq);
        m_Assembler.cmp(Rs64.r64(), Rt64.r64());
        m_Assembler.setl(Result.r8Lo());
    }

    if (!preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), Result.r32());
    }
}

void CX64RecompilerOps::SPECIAL_SLTU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        const uint64_t Rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (m_RegWorkingSet.IsSigned(m_Opcode.rs) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
        const uint64_t Rt = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (m_RegWorkingSet.IsSigned(m_Opcode.rt) ? (uint64_t)(int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) : (uint64_t)m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, Rs < Rt ? 1 : 0);
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rs);

    // rd can only be written before the compare if it is not also a source register
    const bool preMap = (m_Opcode.rd != m_Opcode.rs && m_Opcode.rd != m_Opcode.rt);
    asmjit::x86::Gp Result;
    if (preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
        Result = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32();
    }
    else
    {
        Result = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    }
    m_Assembler.xor_(Result.r32(), Result.r32());

    // Two 32-bit values extended the same way compare the same as their 64-bit forms
    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt) &&
        !m_RegWorkingSet.Is64Bit(m_Opcode.rs) && !m_RegWorkingSet.Is64Bit(m_Opcode.rt) &&
        m_RegWorkingSet.IsSigned(m_Opcode.rs) == m_RegWorkingSet.IsSigned(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            m_Assembler.seta(Result.r8Lo());
        }
        else if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
            m_Assembler.setb(Result.r8Lo());
        }
        else
        {
            m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs).r32(), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
            m_Assembler.setb(Result.r8Lo());
        }
    }
    else
    {
        const asmjit::x86::Gp Rs64 = m_RegWorkingSet.Is64BitMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rs, asmjit::RegType::kX86_Gpq);
        const asmjit::x86::Gp Rt64 = m_RegWorkingSet.Is64BitMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), m_Opcode.rt, asmjit::RegType::kX86_Gpq);
        m_Assembler.cmp(Rs64.r64(), Rt64.r64());
        m_Assembler.setb(Result.r8Lo());
    }

    if (!preMap)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r32(), Result.r32());
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
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.shl(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64(), (uint8_t)(m_Opcode.sa + 32));
    }
}

void CX64RecompilerOps::SPECIAL_DSRL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRA32()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.sar(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rd).r64(), (uint8_t)(m_Opcode.sa + 32));
    }
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
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveVariableToX64reg(asmjit::x86::edx, &m_Reg.INDEX_REGISTER, "INDEX_REGISTER", false);
    m_Assembler.and_(asmjit::x86::edx, 0x1F);
    m_Assembler.xor_(asmjit::x86::r8d, asmjit::x86::r8d);
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_TLB, MemberFuncAddress(&CTLB::WriteEntry), "CTLB::WriteEntry");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();
}

void CX64RecompilerOps::COP0_CO_TLBWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBP()
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_TLB, MemberFuncAddress(&CTLB::Probe), "CTLB::Probe");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();
}

void CX64RecompilerOps::COP0_CO_ERET()
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
    m_RegWorkingSet.WriteBackRegisters();
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_System.m_OpCodes, MemberFuncAddress(&R4300iOp::COP0_CO_ERET), "R4300iOp::COP0_CO_ERET");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();

    UpdateCounters(m_RegWorkingSet, true, true);
    CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, ExitReason_Eret);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX64RecompilerOps::COP1_MF()
{
    CompileCop1Test();

    if (m_Opcode.rt == 0)
    {
        return;
    }

    const asmjit::x86::Gp FprPtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(FprPtrReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.fs], stdstr_f("m_FPR_S[%d]", m_Opcode.fs).c_str());
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), asmjit::x86::dword_ptr(FprPtrReg));
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
    CompileCop1Test();

    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.fs], stdstr_f("m_FPR_S[%d]", m_Opcode.fs).c_str());

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32());
    }
    else
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt).r32());
    }
}

void CX64RecompilerOps::COP1_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_CT()
{
    CompileCop1Test();

    if (m_Opcode.fs != 31)
    {
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
    m_Assembler.and_(ValueReg.r32(), 0x183FFFFu);
    m_Assembler.MovDwordToVariable(&m_Reg.m_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], ValueReg);

    // Matches CRegisters::Cop1_CT, an exception is raised if a cause bit is set with its enable bit or unimplemented operation is set
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    m_Assembler.mov(TempReg.r32(), ValueReg.r32());
    m_Assembler.shl(TempReg.r32(), 5);
    m_Assembler.and_(TempReg.r32(), FPCSR_CV | FPCSR_CZ | FPCSR_CO | FPCSR_CU | FPCSR_CI);
    m_Assembler.or_(TempReg.r32(), FPCSR_CE);
    m_Assembler.test(TempReg.r32(), ValueReg.r32());
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint, &CX64Ops::JneLabel);
    m_RegWorkingSet.SetRoundingModel(CRegBase::RoundUnknown);
}

namespace
{
// Rounding control bits for MXCSR indexed by the FCSR rounding mode
const uint32_t MxcsrRoundingMode[4] = {0x0000, 0x6000, 0x4000, 0x2000};

// FCSR flag bits indexed by the MXCSR exception flags
struct MxcsrFlagMap
{
    uint32_t Value[64];
    MxcsrFlagMap()
    {
        for (uint32_t i = 0; i < 64; i++)
        {
            Value[i] = ((i & 0x01) ? FPCSR_FV : 0) | ((i & 0x04) ? FPCSR_FZ : 0) | ((i & 0x08) ? FPCSR_FO : 0) | ((i & 0x10) ? FPCSR_FU : 0) | ((i & 0x20) ? FPCSR_FI : 0);
        }
    }
};
const MxcsrFlagMap MxcsrToFcsrFlags;

// Flushed single subnormal result indexed by (rounding mode << 1) | sign
const uint32_t FlushSubnormal32[8] = {0x00000000, 0x80000000, 0x00000000, 0x80000000, 0x00800000, 0x80000000, 0x00000000, 0x80800000};

uint32_t FpuMxcsrSave[2];
} // namespace

// Saves the host MXCSR and loads one with the guest rounding mode (or the fixed RoundingMode when it is not -1),
// all exceptions masked and no flush to zero. PtrReg is left pointing at FpuMxcsrSave for CompileFpuLeaveMxcsr
void CX64RecompilerOps::CompileFpuEnterMxcsr(const asmjit::x86::Gp & FcsrPtr, const asmjit::x86::Gp & PtrReg, const asmjit::x86::Gp & TempReg, int32_t RoundingMode)
{
    if (RoundingMode >= 0)
    {
        m_Assembler.mov(TempReg.r32(), MxcsrRoundingMode[RoundingMode & FPCSR_RM_MASK]);
    }
    else
    {
        m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(FcsrPtr));
        m_Assembler.and_(TempReg.r32(), FPCSR_RM_MASK);
        m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)MxcsrRoundingMode, "MxcsrRoundingMode");
        m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(PtrReg, TempReg.r64(), 2));
    }
    m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)FpuMxcsrSave, "FpuMxcsrSave");
    m_Assembler.stmxcsr(asmjit::x86::dword_ptr(PtrReg));
    m_Assembler.stmxcsr(asmjit::x86::dword_ptr(PtrReg, 4));
    m_Assembler.and_(asmjit::x86::dword_ptr(PtrReg, 4), ~0xE07F); // clear FTZ, rounding, DAZ and exception flags
    m_Assembler.or_(asmjit::x86::dword_ptr(PtrReg, 4), 0x1F80);   // mask all exceptions
    m_Assembler.or_(asmjit::x86::dword_ptr(PtrReg, 4), TempReg.r32());
    m_Assembler.ldmxcsr(asmjit::x86::dword_ptr(PtrReg, 4));
}

// Stores the raised exception flags to FpuMxcsrSave[1] and restores the host MXCSR
void CX64RecompilerOps::CompileFpuLeaveMxcsr(const asmjit::x86::Gp & PtrReg)
{
    m_Assembler.stmxcsr(asmjit::x86::dword_ptr(PtrReg, 4));
    m_Assembler.ldmxcsr(asmjit::x86::dword_ptr(PtrReg));
}

// Matches R4300iOp::SetFPUException, Flags holds the raised exceptions as FCSR flag bits
void CX64RecompilerOps::CompileSetFpuException(const asmjit::x86::Gp & FcsrPtr, const asmjit::x86::Gp & Flags, const asmjit::x86::Gp & Temp, asmjit::Label & Unimplemented, asmjit::Label & Exception)
{
    asmjit::Label NoUnderflow = m_Assembler.newLabel();
    m_Assembler.test(Flags.r32(), FPCSR_FU);
    m_Assembler.JeLabel("NoUnderflow", NoUnderflow);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FS);
    m_Assembler.JeLabel("Unimplemented", Unimplemented);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EU | FPCSR_EI);
    m_Assembler.JneLabel("Unimplemented", Unimplemented);
    m_CodeBlock.Log("");
    m_Assembler.bind(NoUnderflow);

    // Cause bits are set for every raised exception, flag bits only for the ones that are not enabled
    m_Assembler.mov(Temp.r32(), Flags.r32());
    m_Assembler.shl(Temp.r32(), 10);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), Temp.r32());
    m_Assembler.mov(Temp.r32(), asmjit::x86::dword_ptr(FcsrPtr));
    m_Assembler.shr(Temp.r32(), 5);
    m_Assembler.not_(Temp.r32());
    m_Assembler.and_(Temp.r32(), Flags.r32());
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), Temp.r32());
    m_Assembler.mov(Temp.r32(), asmjit::x86::dword_ptr(FcsrPtr));
    m_Assembler.shr(Temp.r32(), 5);
    m_Assembler.and_(Temp.r32(), Flags.r32());
    m_Assembler.JneLabel("FpuException", Exception);
}

// Matches the interpreter: InitFpuOperation, CheckFPUInputs32 (CheckFPUInput32 when only fs is used),
// the operation, CheckFPUResult32
void CX64RecompilerOps::COP1_S_Arithmetic(uint32_t InstId, bool UseFt)
{
    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp FsReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp FtReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    asmjit::Label SpecialInput = m_Assembler.newLabel();
    asmjit::Label InputsDone = m_Assembler.newLabel();
    asmjit::Label InvalidInput = m_Assembler.newLabel();
    asmjit::Label Unimplemented = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();
    asmjit::Label ResultNaN = m_Assembler.newLabel();
    asmjit::Label ResultSubnormal = m_Assembler.newLabel();
    asmjit::Label ResultDone = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_S_L[m_Opcode.fs], stdstr_f("m_FPR_S_L[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(FsReg.r32(), asmjit::x86::dword_ptr(PtrReg));
    if (UseFt)
    {
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UW[m_Opcode.ft], stdstr_f("m_FPR_UW[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(FtReg.r32(), asmjit::x86::dword_ptr(PtrReg));
    }

    // Zero, infinity, subnormal and NaN inputs all have an exponent of 0 or 0xFF
    m_Assembler.mov(TempReg.r32(), FsReg.r32());
    m_Assembler.and_(TempReg.r32(), 0x7F800000);
    m_Assembler.JeLabel("SpecialInput", SpecialInput);
    m_Assembler.cmp(TempReg.r32(), 0x7F800000);
    m_Assembler.JeLabel("SpecialInput", SpecialInput);
    if (UseFt)
    {
        m_Assembler.mov(TempReg.r32(), FtReg.r32());
        m_Assembler.and_(TempReg.r32(), 0x7F800000);
        m_Assembler.JeLabel("SpecialInput", SpecialInput);
        m_Assembler.cmp(TempReg.r32(), 0x7F800000);
        m_Assembler.JeLabel("SpecialInput", SpecialInput);
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      InputsDone:");
    m_Assembler.bind(InputsDone);

    CompileFpuEnterMxcsr(FcsrPtr, PtrReg, TempReg);
    m_Assembler.movd(asmjit::x86::xmm0, FsReg.r32());
    if (UseFt)
    {
        m_Assembler.movd(asmjit::x86::xmm1, FtReg.r32());
        m_Assembler.emit(InstId, asmjit::x86::xmm0, asmjit::x86::xmm1);
    }
    else
    {
        m_Assembler.emit(InstId, asmjit::x86::xmm0, asmjit::x86::xmm0);
    }
    CompileFpuLeaveMxcsr(PtrReg);

    // FsReg now holds the result, FtReg the raised exceptions as FCSR flag bits
    m_Assembler.movd(FsReg.r32(), asmjit::x86::xmm0);
    m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(PtrReg, 4));
    m_Assembler.and_(TempReg.r32(), 0x3F);
    m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)MxcsrToFcsrFlags.Value, "MxcsrToFcsrFlags");
    m_Assembler.mov(FtReg.r32(), asmjit::x86::dword_ptr(PtrReg, TempReg.r64(), 2));

    // CheckFPUResult32
    m_Assembler.mov(TempReg.r32(), FsReg.r32());
    m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
    m_Assembler.cmp(TempReg.r32(), 0x7F800000);
    m_Assembler.JaLabel("ResultNaN", ResultNaN);
    m_Assembler.sub(TempReg.r32(), 1);
    m_Assembler.cmp(TempReg.r32(), 0x007FFFFF);
    m_Assembler.JbLabel("ResultSubnormal", ResultSubnormal);
    m_Assembler.test(FtReg.r32(), FtReg.r32());
    m_Assembler.JeLabel("ResultDone", ResultDone);
    CompileSetFpuException(FcsrPtr, FtReg, TempReg, Unimplemented, Exception);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultDone:");
    m_Assembler.bind(ResultDone);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), FsReg.r64());

    m_Assembler.EnterSecondarySection();

    // CheckFPUInputs32: subnormal on either input, then signalling NaN on either input, then quiet NaN
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      SpecialInput:");
    m_Assembler.bind(SpecialInput);
    const asmjit::x86::Gp Inputs[2] = {FsReg, FtReg};
    const size_t InputCount = UseFt ? 2 : 1;
    for (size_t i = 0; i < InputCount; i++)
    {
        const asmjit::x86::Gp & Input = Inputs[i];
        m_Assembler.mov(TempReg.r32(), Input.r32());
        m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
        m_Assembler.sub(TempReg.r32(), 1);
        m_Assembler.cmp(TempReg.r32(), 0x007FFFFF);
        m_Assembler.JbLabel("Unimplemented", Unimplemented);
    }
    for (size_t i = 0; i < InputCount; i++)
    {
        const asmjit::x86::Gp & Input = Inputs[i];
        m_Assembler.mov(TempReg.r32(), Input.r32());
        m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
        m_Assembler.sub(TempReg.r32(), 0x7F800001);
        m_Assembler.cmp(TempReg.r32(), 0x003FFFFF);
        m_Assembler.JbLabel("Unimplemented", Unimplemented);
    }
    for (size_t i = 0; i < InputCount; i++)
    {
        const asmjit::x86::Gp & Input = Inputs[i];
        m_Assembler.mov(TempReg.r32(), Input.r32());
        m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
        m_Assembler.cmp(TempReg.r32(), 0x7F800000);
        m_Assembler.JaLabel("InvalidInput", InvalidInput);
    }
    m_Assembler.JmpLabel("InputsDone", InputsDone);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      InvalidInput:");
    m_Assembler.bind(InvalidInput);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CV);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EV);
    m_Assembler.JneLabel("FpuException", Exception);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FV);
    m_Assembler.JmpLabel("InputsDone", InputsDone);

    // CheckFPUResult32 NaN result
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultNaN:");
    m_Assembler.bind(ResultNaN);
    asmjit::Label NaNNoFlags = m_Assembler.newLabel();
    m_Assembler.test(FtReg.r32(), FtReg.r32());
    m_Assembler.JeLabel("NaNNoFlags", NaNNoFlags);
    CompileSetFpuException(FcsrPtr, FtReg, TempReg, Unimplemented, Exception);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      NaNNoFlags:");
    m_Assembler.bind(NaNNoFlags);
    m_Assembler.mov(FsReg.r32(), 0x7FBFFFFF);
    m_Assembler.JmpLabel("ResultDone", ResultDone);

    // CheckFPUResult32 subnormal result
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultSubnormal:");
    m_Assembler.bind(ResultSubnormal);
    asmjit::Label SubnormalNoFlags = m_Assembler.newLabel();
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FS);
    m_Assembler.JeLabel("Unimplemented", Unimplemented);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EU | FPCSR_EI);
    m_Assembler.JneLabel("Unimplemented", Unimplemented);
    m_Assembler.test(FtReg.r32(), FtReg.r32());
    m_Assembler.JeLabel("SubnormalNoFlags", SubnormalNoFlags);
    CompileSetFpuException(FcsrPtr, FtReg, TempReg, Unimplemented, Exception);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      SubnormalNoFlags:");
    m_Assembler.bind(SubnormalNoFlags);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CU | FPCSR_FU | FPCSR_CI | FPCSR_FI);
    m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(FcsrPtr));
    m_Assembler.and_(TempReg.r32(), FPCSR_RM_MASK);
    m_Assembler.shl(TempReg.r32(), 1);
    m_Assembler.shr(FsReg.r32(), 31);
    m_Assembler.or_(TempReg.r32(), FsReg.r32());
    m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)FlushSubnormal32, "FlushSubnormal32");
    m_Assembler.mov(FsReg.r32(), asmjit::x86::dword_ptr(PtrReg, TempReg.r64(), 2));
    m_Assembler.JmpLabel("ResultDone", ResultDone);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unimplemented:");
    m_Assembler.bind(Unimplemented);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CE);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);

    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_S_ADD()
{
    COP1_S_Arithmetic(asmjit::x86::Inst::kIdAddss);
}

void CX64RecompilerOps::COP1_S_SUB()
{
    COP1_S_Arithmetic(asmjit::x86::Inst::kIdSubss);
}

void CX64RecompilerOps::COP1_S_MUL()
{
    COP1_S_Arithmetic(asmjit::x86::Inst::kIdMulss);
}

void CX64RecompilerOps::COP1_S_DIV()
{
    COP1_S_Arithmetic(asmjit::x86::Inst::kIdDivss);
}

void CX64RecompilerOps::COP1_S_ABS()
{
    COP1_S_ExactOp(ExactFpuOp_Abs);
}

void CX64RecompilerOps::COP1_S_NEG()
{
    COP1_S_ExactOp(ExactFpuOp_Neg);
}

void CX64RecompilerOps::COP1_S_ExactOp(ExactFpuOp Op)
{
    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);

    asmjit::Label NaNInput = m_Assembler.newLabel();
    asmjit::Label Unimplemented = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();
    asmjit::Label StoreResult = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_S_L[m_Opcode.fs], stdstr_f("m_FPR_S_L[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(ValueReg.r32(), asmjit::x86::dword_ptr(PtrReg));

    // CheckFPUInput32: a NaN goes to NaNInput with the absolute value in TempReg, a subnormal is unimplemented
    m_Assembler.mov(TempReg.r32(), ValueReg.r32());
    m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
    m_Assembler.cmp(TempReg.r32(), 0x7F800000);
    m_Assembler.JaLabel("NaNInput", NaNInput);
    m_Assembler.sub(TempReg.r32(), 1);
    m_Assembler.cmp(TempReg.r32(), 0x007FFFFF);
    m_Assembler.JbLabel("Unimplemented", Unimplemented);

    switch (Op)
    {
    case ExactFpuOp_Neg:
        m_Assembler.xor_(ValueReg.r32(), 0x80000000);
        break;
    case ExactFpuOp_Abs:
        m_Assembler.and_(ValueReg.r32(), 0x7FFFFFFF);
        break;
    case ExactFpuOp_CvtD:
        m_Assembler.movd(asmjit::x86::xmm0, ValueReg.r32());
        m_Assembler.cvtss2sd(asmjit::x86::xmm0, asmjit::x86::xmm0);
        m_Assembler.movq(ValueReg.r64(), asmjit::x86::xmm0);
        break;
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      StoreResult:");
    m_Assembler.bind(StoreResult);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), ValueReg.r64());

    m_Assembler.EnterSecondarySection();

    // A NaN in the 0x7F800001 - 0x7FBFFFFF range (either sign) is unimplemented
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      NaNInput:");
    m_Assembler.bind(NaNInput);
    m_Assembler.cmp(TempReg.r32(), 0x7FC00000);
    m_Assembler.JbLabel("Unimplemented", Unimplemented);

    // Quiet NaN: invalid operation, then the NaN result is replaced
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CV);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EV);
    m_Assembler.JneLabel("FpuException", Exception);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FV);
    if (Op == ExactFpuOp_CvtD)
    {
        m_Assembler.MoveConstToX64reg(ValueReg, 0x7FF7FFFFFFFFFFFFull);
    }
    else
    {
        m_Assembler.mov(ValueReg.r32(), 0x7FBFFFFF);
    }
    m_Assembler.JmpLabel("StoreResult", StoreResult);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unimplemented:");
    m_Assembler.bind(Unimplemented);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CE);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);

    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_S_SQRT()
{
    COP1_S_Arithmetic(asmjit::x86::Inst::kIdSqrtss, false);
}

void CX64RecompilerOps::COP1_S_MOV()
{
    COP1_Move();
}

void CX64RecompilerOps::COP1_S_ROUND_L()
{
    COP1_ToInteger(FPRoundingMode_RoundToNearest, false, true);
}

void CX64RecompilerOps::COP1_S_TRUNC_L()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardZero, false, true);
}

void CX64RecompilerOps::COP1_S_CEIL_L()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardPlusInfinity, false, true);
}

void CX64RecompilerOps::COP1_S_FLOOR_L()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardMinusInfinity, false, true);
}

void CX64RecompilerOps::COP1_S_ROUND_W()
{
    COP1_ToInteger(FPRoundingMode_RoundToNearest, false, false);
}

void CX64RecompilerOps::COP1_S_TRUNC_W()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardZero, false, false);
}

void CX64RecompilerOps::COP1_S_CEIL_W()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardPlusInfinity, false, false);
}

void CX64RecompilerOps::COP1_S_FLOOR_W()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardMinusInfinity, false, false);
}

void CX64RecompilerOps::COP1_S_CVT_D()
{
    COP1_S_ExactOp(ExactFpuOp_CvtD);
}

void CX64RecompilerOps::COP1_S_CVT_W()
{
    COP1_ToInteger(-1, false, false);
}

void CX64RecompilerOps::COP1_ToInteger(int32_t RoundingMode, bool DoubleInput, bool LongResult)
{
    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    asmjit::Label Unimplemented = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();
    asmjit::Label Exact = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    if (DoubleInput)
    {
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("m_FPR_D[%d]", m_Opcode.fs).c_str());
        m_Assembler.mov(ValueReg.r64(), asmjit::x86::qword_ptr(PtrReg));

        // CheckFPUInput64Conv: subnormal, infinity, NaN or a magnitude of 2^53 (exponent 0x434) or more is unimplemented
        asmjit::Label InputValid = m_Assembler.newLabel();
        m_Assembler.mov(TempReg.r64(), ValueReg.r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.JeLabel("InputValid", InputValid);
        m_Assembler.shr(TempReg.r64(), 53);
        m_Assembler.JeLabel("Unimplemented", Unimplemented);
        m_Assembler.cmp(TempReg.r32(), 0x434);
        m_Assembler.JaeLabel("Unimplemented", Unimplemented);
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      InputValid:");
        m_Assembler.bind(InputValid);
    }
    else
    {
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_S_L[m_Opcode.fs], stdstr_f("m_FPR_S_L[%d]", m_Opcode.fs).c_str());
        m_Assembler.mov(ValueReg.r32(), asmjit::x86::dword_ptr(PtrReg));

        // CheckFPUInput32Conv: subnormal, infinity, NaN or a magnitude of 2^53 or more is unimplemented
        m_Assembler.mov(TempReg.r32(), ValueReg.r32());
        m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
        m_Assembler.cmp(TempReg.r32(), 0x5A000000);
        m_Assembler.JaeLabel("Unimplemented", Unimplemented);
        m_Assembler.sub(TempReg.r32(), 1);
        m_Assembler.cmp(TempReg.r32(), 0x007FFFFF);
        m_Assembler.JbLabel("Unimplemented", Unimplemented);
    }

    // Rounds with the rounding mode, out of range sets invalid and returns 0x80000000 (a long result always fits
    // because the magnitude is below 2^53)
    const asmjit::x86::Gp ResultReg = LongResult ? asmjit::x86::Gp(ValueReg.r64()) : asmjit::x86::Gp(ValueReg.r32());
    CompileFpuEnterMxcsr(FcsrPtr, PtrReg, TempReg, RoundingMode);
    if (DoubleInput)
    {
        m_Assembler.movq(asmjit::x86::xmm0, ValueReg.r64());
        m_Assembler.cvtsd2si(ResultReg, asmjit::x86::xmm0);
    }
    else
    {
        m_Assembler.movd(asmjit::x86::xmm0, ValueReg.r32());
        m_Assembler.cvtss2si(ResultReg, asmjit::x86::xmm0);
    }
    CompileFpuLeaveMxcsr(PtrReg);

    // CheckFPUInvalidException: invalid is unimplemented, otherwise inexact sets the cause and flag or raises the exception
    m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(PtrReg, 4));
    m_Assembler.test(TempReg.r32(), 0x01);
    m_Assembler.JneLabel("Unimplemented", Unimplemented);
    m_Assembler.test(TempReg.r32(), 0x20);
    m_Assembler.JeLabel("Exact", Exact);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CI);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EI);
    m_Assembler.JneLabel("FpuException", Exception);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FI);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Exact:");
    m_Assembler.bind(Exact);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), ValueReg.r64());

    m_Assembler.EnterSecondarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unimplemented:");
    m_Assembler.bind(Unimplemented);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CE);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);
    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_S_CVT_L()
{
    COP1_ToInteger(-1, false, true);
}

void CX64RecompilerOps::COP1_S_CMP()
{
    COP1_Compare(false);
}

// Matches R4300iOp::COP1_S_CMP and R4300iOp::COP1_D_CMP
void CX64RecompilerOps::COP1_Compare(bool Double)
{
    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp FsReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp FtReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ResultReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);

    asmjit::Label Unordered = m_Assembler.newLabel();
    asmjit::Label ConditionReady = m_Assembler.newLabel();
    asmjit::Label Invalid = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    if (Double)
    {
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("m_FPR_D[%d]", m_Opcode.fs).c_str());
        m_Assembler.mov(FsReg.r64(), asmjit::x86::qword_ptr(PtrReg));
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.ft], stdstr_f("m_FPR_UDW[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(FtReg.r64(), asmjit::x86::qword_ptr(PtrReg));
    }
    else
    {
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_S_L[m_Opcode.fs], stdstr_f("m_FPR_S_L[%d]", m_Opcode.fs).c_str());
        m_Assembler.mov(FsReg.r32(), asmjit::x86::dword_ptr(PtrReg));
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UW[m_Opcode.ft], stdstr_f("m_FPR_UW[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(FtReg.r32(), asmjit::x86::dword_ptr(PtrReg));
    }

    // ucomiss/ucomisd: unordered sets ZF, PF and CF, less sets CF, equal sets ZF
    m_Assembler.xor_(ResultReg.r32(), ResultReg.r32());
    if (Double)
    {
        m_Assembler.movq(asmjit::x86::xmm0, FsReg.r64());
        m_Assembler.movq(asmjit::x86::xmm1, FtReg.r64());
        m_Assembler.ucomisd(asmjit::x86::xmm0, asmjit::x86::xmm1);
    }
    else
    {
        m_Assembler.movd(asmjit::x86::xmm0, FsReg.r32());
        m_Assembler.movd(asmjit::x86::xmm1, FtReg.r32());
        m_Assembler.ucomiss(asmjit::x86::xmm0, asmjit::x86::xmm1);
    }
    m_Assembler.JpLabel("Unordered", Unordered);
    const bool TestLess = (m_Opcode.funct & 4) != 0, TestEqual = (m_Opcode.funct & 2) != 0;
    if (TestLess && TestEqual)
    {
        m_Assembler.setbe(ResultReg.r8Lo());
    }
    else if (TestLess)
    {
        m_Assembler.setb(ResultReg.r8Lo());
    }
    else if (TestEqual)
    {
        m_Assembler.sete(ResultReg.r8Lo());
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ConditionReady:");
    m_Assembler.bind(ConditionReady);
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~FPCSR_C);
    m_Assembler.shl(ResultReg.r32(), 23);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), ResultReg.r32());

    m_Assembler.EnterSecondarySection();

    // At least one input is a NaN, invalid is raised for a signalling compare or when either input is
    // in the 0x7FC00000 - 0x7FFFFFFF range for singles or the 0x7FF8000000000000 - 0x7FFFFFFFFFFFFFFF
    // range for doubles (either sign), the same as the interpreter
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unordered:");
    m_Assembler.bind(Unordered);
    if ((m_Opcode.funct & 8) == 0)
    {
        asmjit::Label NotInvalid = m_Assembler.newLabel();
        const asmjit::x86::Gp Inputs[2] = {FsReg, FtReg};
        for (const asmjit::x86::Gp & Input : Inputs)
        {
            if (Double)
            {
                // Drop the sign, leaving the exponent and top mantissa bit: >= 0xFFF is >= 0x7FF8000000000000
                m_Assembler.shl(Input.r64(), 1);
                m_Assembler.shr(Input.r64(), 52);
                m_Assembler.cmp(Input.r32(), 0xFFF);
            }
            else
            {
                m_Assembler.and_(Input.r32(), 0x7FFFFFFF);
                m_Assembler.cmp(Input.r32(), 0x7FC00000);
            }
            m_Assembler.JaeLabel("Invalid", Invalid);
        }
        m_Assembler.JmpLabel("NotInvalid", NotInvalid);
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      Invalid:");
        m_Assembler.bind(Invalid);
        m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CV);
        m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EV);
        m_Assembler.JneLabel("FpuException", Exception);
        m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FV);
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      NotInvalid:");
        m_Assembler.bind(NotInvalid);
    }
    else
    {
        m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CV);
        m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EV);
        m_Assembler.JneLabel("FpuException", Exception);
        m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FV);
    }
    m_Assembler.mov(ResultReg.r32(), (m_Opcode.funct & 1) != 0 ? 1 : 0);
    m_Assembler.JmpLabel("ConditionReady", ConditionReady);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);

    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_D_Arithmetic(uint32_t InstId, bool UseFt, bool SingleResult)
{
    // Flushed double subnormal result indexed by (rounding mode << 1) | sign
    static const uint64_t FlushSubnormal64[8] = {0x0000000000000000ull, 0x8000000000000000ull, 0x0000000000000000ull, 0x8000000000000000ull, 0x0010000000000000ull, 0x8000000000000000ull, 0x0000000000000000ull, 0x8010000000000000ull};

    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp FsReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp FtReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    asmjit::Label SpecialInput = m_Assembler.newLabel();
    asmjit::Label InputsDone = m_Assembler.newLabel();
    asmjit::Label InvalidInput = m_Assembler.newLabel();
    asmjit::Label Unimplemented = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();
    asmjit::Label ResultExponentZero = m_Assembler.newLabel();
    asmjit::Label ResultExponentMax = m_Assembler.newLabel();
    asmjit::Label ResultNormal = m_Assembler.newLabel();
    asmjit::Label ResultDone = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("m_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(FsReg.r64(), asmjit::x86::qword_ptr(PtrReg));
    if (UseFt)
    {
        m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.ft], stdstr_f("m_FPR_UDW[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(FtReg.r64(), asmjit::x86::qword_ptr(PtrReg));
    }

    // Zero, infinity, subnormal and NaN inputs all have an exponent of 0 or 0x7FF
    const asmjit::x86::Gp Inputs[2] = {FsReg, FtReg};
    const size_t InputCount = UseFt ? 2 : 1;
    for (size_t i = 0; i < InputCount; i++)
    {
        m_Assembler.mov(TempReg.r64(), Inputs[i].r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.shr(TempReg.r64(), 53);
        m_Assembler.JeLabel("SpecialInput", SpecialInput);
        m_Assembler.cmp(TempReg.r32(), 0x7FF);
        m_Assembler.JeLabel("SpecialInput", SpecialInput);
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      InputsDone:");
    m_Assembler.bind(InputsDone);

    CompileFpuEnterMxcsr(FcsrPtr, PtrReg, TempReg);
    m_Assembler.movq(asmjit::x86::xmm0, FsReg.r64());
    if (UseFt)
    {
        m_Assembler.movq(asmjit::x86::xmm1, FtReg.r64());
        m_Assembler.emit(InstId, asmjit::x86::xmm0, asmjit::x86::xmm1);
    }
    else
    {
        m_Assembler.emit(InstId, asmjit::x86::xmm0, asmjit::x86::xmm0);
    }
    CompileFpuLeaveMxcsr(PtrReg);

    // FsReg now holds the result (a single is zero extended), FtReg the raised exceptions as FCSR flag bits
    if (SingleResult)
    {
        m_Assembler.movd(FsReg.r32(), asmjit::x86::xmm0);
    }
    else
    {
        m_Assembler.movq(FsReg.r64(), asmjit::x86::xmm0);
    }
    m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(PtrReg, 4));
    m_Assembler.and_(TempReg.r32(), 0x3F);
    m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)MxcsrToFcsrFlags.Value, "MxcsrToFcsrFlags");
    m_Assembler.mov(FtReg.r32(), asmjit::x86::dword_ptr(PtrReg, TempReg.r64(), 2));

    if (SingleResult)
    {
        // CheckFPUResult32: a single NaN goes to ResultExponentMax and a subnormal to ResultExponentZero
        m_Assembler.mov(TempReg.r32(), FsReg.r32());
        m_Assembler.and_(TempReg.r32(), 0x7FFFFFFF);
        m_Assembler.cmp(TempReg.r32(), 0x7F800000);
        m_Assembler.JaLabel("ResultExponentMax", ResultExponentMax);
        m_Assembler.sub(TempReg.r32(), 1);
        m_Assembler.cmp(TempReg.r32(), 0x007FFFFF);
        m_Assembler.JbLabel("ResultExponentZero", ResultExponentZero);
    }
    else
    {
        // CheckFPUResult64, zero and infinity results come back to ResultNormal
        m_Assembler.mov(TempReg.r64(), FsReg.r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.shr(TempReg.r64(), 53);
        m_Assembler.JeLabel("ResultExponentZero", ResultExponentZero);
        m_Assembler.cmp(TempReg.r32(), 0x7FF);
        m_Assembler.JeLabel("ResultExponentMax", ResultExponentMax);
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultNormal:");
    m_Assembler.bind(ResultNormal);
    m_Assembler.test(FtReg.r32(), FtReg.r32());
    m_Assembler.JeLabel("ResultDone", ResultDone);
    CompileSetFpuException(FcsrPtr, FtReg, TempReg, Unimplemented, Exception);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultDone:");
    m_Assembler.bind(ResultDone);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), FsReg.r64());

    m_Assembler.EnterSecondarySection();

    // CheckFPUInputs64: subnormal on either input, then signalling NaN (quiet bit clear) on either input, then quiet NaN
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      SpecialInput:");
    m_Assembler.bind(SpecialInput);
    for (size_t i = 0; i < InputCount; i++)
    {
        asmjit::Label NotSubnormal = m_Assembler.newLabel();
        m_Assembler.mov(TempReg.r64(), Inputs[i].r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.JeLabel("NotSubnormal", NotSubnormal);
        m_Assembler.shr(TempReg.r64(), 53);
        m_Assembler.JeLabel("Unimplemented", Unimplemented);
        m_CodeBlock.Log("");
        m_Assembler.bind(NotSubnormal);
    }
    for (size_t i = 0; i < InputCount; i++)
    {
        asmjit::Label NotSignalling = m_Assembler.newLabel();
        m_Assembler.mov(TempReg.r64(), Inputs[i].r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.shr(TempReg.r64(), 52);
        m_Assembler.cmp(TempReg.r32(), 0xFFE);
        m_Assembler.JneLabel("NotSignalling", NotSignalling);
        m_Assembler.mov(TempReg.r64(), Inputs[i].r64());
        m_Assembler.shl(TempReg.r64(), 12);
        m_Assembler.JneLabel("Unimplemented", Unimplemented);
        m_CodeBlock.Log("");
        m_Assembler.bind(NotSignalling);
    }
    for (size_t i = 0; i < InputCount; i++)
    {
        m_Assembler.mov(TempReg.r64(), Inputs[i].r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.shr(TempReg.r64(), 52);
        m_Assembler.cmp(TempReg.r32(), 0xFFF);
        m_Assembler.JeLabel("InvalidInput", InvalidInput);
    }
    m_Assembler.JmpLabel("InputsDone", InputsDone);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      InvalidInput:");
    m_Assembler.bind(InvalidInput);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CV);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EV);
    m_Assembler.JneLabel("FpuException", Exception);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FV);
    m_Assembler.JmpLabel("InputsDone", InputsDone);

    // Exponent 0x7FF: infinity is a normal result, anything else is a NaN (a single result only gets here for a NaN)
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultExponentMax:");
    m_Assembler.bind(ResultExponentMax);
    if (!SingleResult)
    {
        m_Assembler.mov(TempReg.r64(), FsReg.r64());
        m_Assembler.shl(TempReg.r64(), 12);
        m_Assembler.JeLabel("ResultNormal", ResultNormal);
    }
    asmjit::Label NaNNoFlags = m_Assembler.newLabel();
    m_Assembler.test(FtReg.r32(), FtReg.r32());
    m_Assembler.JeLabel("NaNNoFlags", NaNNoFlags);
    CompileSetFpuException(FcsrPtr, FtReg, TempReg, Unimplemented, Exception);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      NaNNoFlags:");
    m_Assembler.bind(NaNNoFlags);
    if (SingleResult)
    {
        m_Assembler.mov(FsReg.r32(), 0x7FBFFFFF);
    }
    else
    {
        m_Assembler.MoveConstToX64reg(FsReg, 0x7FF7FFFFFFFFFFFFull);
    }
    m_Assembler.JmpLabel("ResultDone", ResultDone);

    // Exponent 0: zero is a normal result, anything else is a subnormal (a single result only gets here for a subnormal)
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ResultExponentZero:");
    m_Assembler.bind(ResultExponentZero);
    if (!SingleResult)
    {
        m_Assembler.mov(TempReg.r64(), FsReg.r64());
        m_Assembler.shl(TempReg.r64(), 1);
        m_Assembler.JeLabel("ResultNormal", ResultNormal);
    }
    asmjit::Label SubnormalNoFlags = m_Assembler.newLabel();
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FS);
    m_Assembler.JeLabel("Unimplemented", Unimplemented);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EU | FPCSR_EI);
    m_Assembler.JneLabel("Unimplemented", Unimplemented);
    m_Assembler.test(FtReg.r32(), FtReg.r32());
    m_Assembler.JeLabel("SubnormalNoFlags", SubnormalNoFlags);
    CompileSetFpuException(FcsrPtr, FtReg, TempReg, Unimplemented, Exception);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      SubnormalNoFlags:");
    m_Assembler.bind(SubnormalNoFlags);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CU | FPCSR_FU | FPCSR_CI | FPCSR_FI);
    m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(FcsrPtr));
    m_Assembler.and_(TempReg.r32(), FPCSR_RM_MASK);
    m_Assembler.shl(TempReg.r32(), 1);
    if (SingleResult)
    {
        m_Assembler.shr(FsReg.r32(), 31);
        m_Assembler.or_(TempReg.r32(), FsReg.r32());
        m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)FlushSubnormal32, "FlushSubnormal32");
        m_Assembler.mov(FsReg.r32(), asmjit::x86::dword_ptr(PtrReg, TempReg.r64(), 2));
    }
    else
    {
        m_Assembler.shr(FsReg.r64(), 63);
        m_Assembler.or_(TempReg.r32(), FsReg.r32());
        m_Assembler.MoveConstToX64reg(PtrReg, (uintptr_t)FlushSubnormal64, "FlushSubnormal64");
        m_Assembler.mov(FsReg.r64(), asmjit::x86::qword_ptr(PtrReg, TempReg.r64(), 3));
    }
    m_Assembler.JmpLabel("ResultDone", ResultDone);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unimplemented:");
    m_Assembler.bind(Unimplemented);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CE);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);

    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_D_ADD()
{
    COP1_D_Arithmetic(asmjit::x86::Inst::kIdAddsd);
}

void CX64RecompilerOps::COP1_D_SUB()
{
    COP1_D_Arithmetic(asmjit::x86::Inst::kIdSubsd);
}

void CX64RecompilerOps::COP1_D_MUL()
{
    COP1_D_Arithmetic(asmjit::x86::Inst::kIdMulsd);
}

void CX64RecompilerOps::COP1_D_DIV()
{
    COP1_D_Arithmetic(asmjit::x86::Inst::kIdDivsd);
}

void CX64RecompilerOps::COP1_D_ABS()
{
    COP1_D_ExactOp(false);
}

void CX64RecompilerOps::COP1_D_NEG()
{
    COP1_D_ExactOp(true);
}

void CX64RecompilerOps::COP1_D_SQRT()
{
    COP1_D_Arithmetic(asmjit::x86::Inst::kIdSqrtsd, false);
}

void CX64RecompilerOps::COP1_D_MOV()
{
    COP1_Move();
}

void CX64RecompilerOps::COP1_D_ROUND_L()
{
    COP1_ToInteger(FPRoundingMode_RoundToNearest, true, true);
}

void CX64RecompilerOps::COP1_D_TRUNC_L()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardZero, true, true);
}

void CX64RecompilerOps::COP1_D_CEIL_L()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardPlusInfinity, true, true);
}

void CX64RecompilerOps::COP1_D_FLOOR_L()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardMinusInfinity, true, true);
}

void CX64RecompilerOps::COP1_D_ROUND_W()
{
    COP1_ToInteger(FPRoundingMode_RoundToNearest, true, false);
}

void CX64RecompilerOps::COP1_D_TRUNC_W()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardZero, true, false);
}

void CX64RecompilerOps::COP1_D_CEIL_W()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardPlusInfinity, true, false);
}

void CX64RecompilerOps::COP1_D_FLOOR_W()
{
    COP1_ToInteger(FPRoundingMode_RoundTowardMinusInfinity, true, false);
}

void CX64RecompilerOps::COP1_D_CVT_S()
{
    COP1_D_Arithmetic(asmjit::x86::Inst::kIdCvtsd2ss, false, true);
}

void CX64RecompilerOps::COP1_D_CVT_W()
{
    COP1_ToInteger(-1, true, false);
}

void CX64RecompilerOps::COP1_D_CVT_L()
{
    COP1_ToInteger(-1, true, true);
}

void CX64RecompilerOps::COP1_D_CMP()
{
    COP1_Compare(true);
}

void CX64RecompilerOps::COP1_W_CVT_S()
{
    static uint32_t MxcsrSave[2];
    static_assert(sizeof(softfloat_roundingMode) == 1 && sizeof(softfloat_exceptionFlags) == 1, "softfloat state is stored as a byte");

    CompileCop1Test();

    m_Assembler.AndConstToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", ~0x0003F000u);

    const asmjit::x86::Gp AddrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    m_Assembler.MoveVariableToX64reg(TempReg, &m_Reg.m_FPCR[31], "_FPCR[31]", false);
    m_Assembler.and_(TempReg.r32(), FPCSR_RM_MASK);
    m_Assembler.mov(ValueReg.r32(), TempReg.r32());
    m_Assembler.shr(ValueReg.r32(), 1);
    m_Assembler.xor_(ValueReg.r32(), TempReg.r32());
    m_Assembler.MoveConstToX64reg(AddrReg, (uintptr_t)&softfloat_roundingMode, "softfloat_roundingMode");
    m_Assembler.mov(asmjit::x86::byte_ptr(AddrReg), ValueReg.r8());
    m_Assembler.MoveConstByteToVariable(&softfloat_exceptionFlags, "softfloat_exceptionFlags", 0);

    m_Assembler.MoveVariable64ToX64reg(AddrReg, (uint8_t *)&m_Reg.m_FPR_S_L[m_Opcode.fs], stdstr_f("m_FPR_S_L[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(ValueReg.r32(), asmjit::x86::dword_ptr(AddrReg));

    m_Assembler.MoveConstToX64reg(AddrReg, (uintptr_t)MxcsrRoundingMode, "MxcsrRoundingMode");
    m_Assembler.mov(TempReg.r32(), asmjit::x86::dword_ptr(AddrReg, TempReg.r64(), 2));
    m_Assembler.MoveConstToX64reg(AddrReg, (uintptr_t)MxcsrSave, "MxcsrSave");
    m_Assembler.stmxcsr(asmjit::x86::dword_ptr(AddrReg));
    m_Assembler.stmxcsr(asmjit::x86::dword_ptr(AddrReg, 4));
    m_Assembler.and_(asmjit::x86::dword_ptr(AddrReg, 4), ~0x6000);
    m_Assembler.or_(asmjit::x86::dword_ptr(AddrReg, 4), TempReg.r32());
    m_Assembler.ldmxcsr(asmjit::x86::dword_ptr(AddrReg, 4));
    m_Assembler.cvtsi2ss(asmjit::x86::xmm0, ValueReg.r32());
    m_Assembler.ldmxcsr(asmjit::x86::dword_ptr(AddrReg));

    m_Assembler.cvttss2si(TempReg.r32(), asmjit::x86::xmm0);
    m_Assembler.cmp(TempReg.r32(), ValueReg.r32());
    asmjit::Label ExactLabel = m_Assembler.newLabel();
    m_Assembler.JeLabel("Exact", ExactLabel);
    m_Assembler.MoveConstByteToVariable(&softfloat_exceptionFlags, "softfloat_exceptionFlags", softfloat_flag_inexact);
    m_Assembler.MoveConstToX64reg(AddrReg, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.or_(asmjit::x86::dword_ptr(AddrReg), FPCSR_CI);
    m_Assembler.test(asmjit::x86::dword_ptr(AddrReg), FPCSR_EI);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint, &CX64Ops::JneLabel);
    m_Assembler.or_(asmjit::x86::dword_ptr(AddrReg), FPCSR_FI);
    m_Assembler.bind(ExactLabel);

    m_Assembler.movd(ValueReg.r32(), asmjit::x86::xmm0);
    m_Assembler.MoveVariable64ToX64reg(AddrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(AddrReg), ValueReg.r64());
}

void CX64RecompilerOps::COP1_W_CVT_D()
{
    CompileCop1Test();

    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    // InitFpuOperation clears the cause bits, every int32 converts to a double exactly so nothing else is raised
    m_Assembler.AndConstToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", ~0x0003F000u);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_S_L[m_Opcode.fs], stdstr_f("m_FPR_S_L[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(ValueReg.r32(), asmjit::x86::dword_ptr(PtrReg));
    m_Assembler.cvtsi2sd(asmjit::x86::xmm0, ValueReg.r32());
    m_Assembler.movq(ValueReg.r64(), asmjit::x86::xmm0);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), ValueReg.r64());
}

void CX64RecompilerOps::COP1_L_ToFloat(bool Double)
{
    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    asmjit::Label Unimplemented = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();
    asmjit::Label Exact = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("m_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(ValueReg.r64(), asmjit::x86::qword_ptr(PtrReg));

    // A value outside -2^55 to 2^55 - 1 is unimplemented, inside that range value >> 55 is 0 or -1
    m_Assembler.mov(TempReg.r64(), ValueReg.r64());
    m_Assembler.sar(TempReg.r64(), 55);
    m_Assembler.add(TempReg.r64(), 1);
    m_Assembler.cmp(TempReg.r64(), 1);
    m_Assembler.JaLabel("Unimplemented", Unimplemented);

    // i64_to_f32 / i64_to_f64 round with the guest rounding mode, the result can only be inexact
    CompileFpuEnterMxcsr(FcsrPtr, PtrReg, TempReg);
    if (Double)
    {
        m_Assembler.cvtsi2sd(asmjit::x86::xmm0, ValueReg.r64());
    }
    else
    {
        m_Assembler.cvtsi2ss(asmjit::x86::xmm0, ValueReg.r64());
    }
    CompileFpuLeaveMxcsr(PtrReg);
    if (Double)
    {
        m_Assembler.movq(ValueReg.r64(), asmjit::x86::xmm0);
    }
    else
    {
        m_Assembler.movd(ValueReg.r32(), asmjit::x86::xmm0);
    }

    // CheckFPUResult32/64 with only inexact possible: set the cause and flag or raise the exception
    m_Assembler.test(asmjit::x86::dword_ptr(PtrReg, 4), 0x20);
    m_Assembler.JeLabel("Exact", Exact);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CI);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EI);
    m_Assembler.JneLabel("FpuException", Exception);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FI);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Exact:");
    m_Assembler.bind(Exact);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), ValueReg.r64());

    m_Assembler.EnterSecondarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unimplemented:");
    m_Assembler.bind(Unimplemented);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CE);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);
    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_D_ExactOp(bool Negate)
{
    CompileCop1Test();

    const asmjit::x86::Gp FcsrPtr = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);

    asmjit::Label ExponentZero = m_Assembler.newLabel();
    asmjit::Label ExponentMax = m_Assembler.newLabel();
    asmjit::Label InputValid = m_Assembler.newLabel();
    asmjit::Label Unimplemented = m_Assembler.newLabel();
    asmjit::Label Exception = m_Assembler.newLabel();
    asmjit::Label StoreResult = m_Assembler.newLabel();

    // InitFpuOperation clears the cause bits
    m_Assembler.MoveConstToX64reg(FcsrPtr, (uintptr_t)&m_Reg.m_FPCR[31], "_FPCR[31]");
    m_Assembler.and_(asmjit::x86::dword_ptr(FcsrPtr), ~0x0003F000);

    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("m_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(ValueReg.r64(), asmjit::x86::qword_ptr(PtrReg));

    // CheckFPUInput64, values with an exponent of 0 or 0x7FF are checked in the secondary section
    m_Assembler.mov(TempReg.r64(), ValueReg.r64());
    m_Assembler.shl(TempReg.r64(), 1);
    m_Assembler.shr(TempReg.r64(), 53);
    m_Assembler.JeLabel("ExponentZero", ExponentZero);
    m_Assembler.cmp(TempReg.r32(), 0x7FF);
    m_Assembler.JeLabel("ExponentMax", ExponentMax);
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      InputValid:");
    m_Assembler.bind(InputValid);

    // Multiplying by -1.0 and fabs are exact and raise nothing, so the result is the input with the sign bit flipped or cleared
    if (Negate)
    {
        m_Assembler.btc(ValueReg.r64(), 63);
    }
    else
    {
        m_Assembler.btr(ValueReg.r64(), 63);
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      StoreResult:");
    m_Assembler.bind(StoreResult);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), ValueReg.r64());

    m_Assembler.EnterSecondarySection();

    // Exponent 0: zero is valid, anything else is a subnormal
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ExponentZero:");
    m_Assembler.bind(ExponentZero);
    m_Assembler.mov(TempReg.r64(), ValueReg.r64());
    m_Assembler.shl(TempReg.r64(), 1);
    m_Assembler.JeLabel("InputValid", InputValid);
    m_Assembler.JmpLabel("Unimplemented", Unimplemented);

    // Exponent 0x7FF: infinity is valid, a NaN with the quiet bit clear is unimplemented
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      ExponentMax:");
    m_Assembler.bind(ExponentMax);
    m_Assembler.mov(TempReg.r64(), ValueReg.r64());
    m_Assembler.shl(TempReg.r64(), 12);
    m_Assembler.JeLabel("InputValid", InputValid);
    m_Assembler.bt(ValueReg.r64(), 51);
    m_Assembler.JaeLabel("Unimplemented", Unimplemented);

    // Quiet NaN: invalid operation, then the NaN result is replaced
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CV);
    m_Assembler.test(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_EV);
    m_Assembler.JneLabel("FpuException", Exception);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_FV);
    m_Assembler.MoveConstToX64reg(ValueReg, 0x7FF7FFFFFFFFFFFFull);
    m_Assembler.JmpLabel("StoreResult", StoreResult);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      Unimplemented:");
    m_Assembler.bind(Unimplemented);
    m_Assembler.or_(asmjit::x86::dword_ptr(FcsrPtr), FPCSR_CE);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      FpuException:");
    m_Assembler.bind(Exception);
    CompileExit((uint32_t)-1, m_CompilePC, m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp), ExitReason_ExceptionFloatingPoint);

    m_Assembler.EnterPrimarySection();
}

void CX64RecompilerOps::COP1_Move()
{
    CompileCop1Test();

    const asmjit::x86::Gp PtrReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("m_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(ValueReg.r64(), asmjit::x86::qword_ptr(PtrReg));
    m_Assembler.MoveVariable64ToX64reg(PtrReg, (uint8_t *)&m_Reg.m_FPR_UDW[m_Opcode.fd], stdstr_f("m_FPR_UDW[%d]", m_Opcode.fd).c_str());
    m_Assembler.mov(asmjit::x86::qword_ptr(PtrReg), ValueReg.r64());
}

void CX64RecompilerOps::COP1_L_CVT_S()
{
    COP1_L_ToFloat(false);
}

void CX64RecompilerOps::COP1_L_CVT_D()
{
    COP1_L_ToFloat(true);
}

void CX64RecompilerOps::UnknownOpcode()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::EnterCodeBlock()
{
    m_Assembler.push(asmjit::x86::rbx);
    m_Assembler.push(asmjit::x86::rbp);
    m_Assembler.push(asmjit::x86::rsi);
    m_Assembler.push(asmjit::x86::rdi);
    m_Assembler.push(asmjit::x86::r12);
    m_Assembler.push(asmjit::x86::r13);
    m_Assembler.push(asmjit::x86::r14);
    m_Assembler.push(asmjit::x86::r15);
    m_Assembler.sub(asmjit::x86::rsp, FunctionStackSize);
}

void CX64RecompilerOps::CompileExitCode()
{
}

void CX64RecompilerOps::CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter)
{
    m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", ProgramCounter);
    RegSet.WriteBackRegisters();
    UpdateCounters(RegSet, false, true, false);

    // All registers have been written back, so no caller saved registers need preserving around the calls
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_System.m_OpCodes, MemberFuncAddress(&R4300iOp::InPermLoop), "R4300iOp::InPermLoop");
    m_Assembler.CallThis(g_SystemTimer, MemberFuncAddress(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_CodeBlock.Log("CompileSystemCheck 3");
    CompileSystemCheck((uint32_t)-1, RegSet);
    if (g_SyncSystem)
    {
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_BaseSystem, MemberFuncAddress(&CN64System::SyncSystem), "CN64System::SyncSystem");
        m_Assembler.add(asmjit::x86::rsp, 32);
    }
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
    case ExitReason_ExceptionFloatingPoint:
    {
        const bool InDelaySlot = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", InDelaySlot ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.mov(asmjit::x86::rdx, EXC_FPE);
        m_Assembler.xor_(asmjit::x86::r8d, asmjit::x86::r8d);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::TriggerException), "CRegisters::TriggerException");
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_Assembler.MoveVariableToX64reg(asmjit::x86::eax, &g_System->m_JumpToLocation, "System->m_JumpToLocation", false);
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

    case ExitReason_Eret:
        m_Assembler.MoveVariable64ToX64reg(asmjit::x86::rax, &m_System.m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MovQwordToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::rax);
        m_Assembler.MoveConstToVariable(&m_System.m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        CompileSystemCheck((uint32_t)-1, ExitRegSet);
        ExitCodeBlock();
        break;
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

void CX64RecompilerOps::OverflowDelaySlot(bool TestTimer)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT)
    {
        m_Assembler.MoveVariable64ToX64reg(asmjit::x86::rax, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
        m_Assembler.MovQwordToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::rax);
        m_Assembler.MoveVariable64ToX64reg(asmjit::x86::rax, &g_System->m_JumpDelayLocation, "System::JumpDelayLocation");
        m_Assembler.MovQwordToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", asmjit::x86::rax);
    }
    else
    {
        m_Assembler.MoveConst64ToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", (uint64_t)(int64_t)(int32_t)(m_CompilePC + 4));
    }
    m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_JUMP);
    if (g_SyncSystem)
    {
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_BaseSystem, MemberFuncAddress(&CN64System::SyncSystem), "CN64System::SyncSystem");
        m_Assembler.add(asmjit::x86::rsp, 32);
    }

    if (TestTimer)
    {
        m_Assembler.MoveConstByteToVariable(&g_System->m_TestTimer, "R4300iOp::m_TestTimer", TestTimer);
    }

    // The interpreter runs the delay slot instruction and then jumps to m_JumpToLocation
    m_Assembler.mov(asmjit::x86::edx, g_GameSettings.countPerOp);
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_System.m_OpCodes, MemberFuncAddress(&R4300iOp::ExecuteOps), "R4300iOp::ExecuteOps");
    m_Assembler.add(asmjit::x86::rsp, 32);

    if (g_GameSettings.fastSP && g_Recompiler)
    {
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_Recompiler, MemberFuncAddress(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos");
        m_Assembler.add(asmjit::x86::rsp, 32);
    }
    if (g_SyncSystem)
    {
        UpdateSyncCPU(m_RegWorkingSet, g_GameSettings.countPerOp);
    }

    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
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
    m_Assembler.pop(asmjit::x86::r15);
    m_Assembler.pop(asmjit::x86::r14);
    m_Assembler.pop(asmjit::x86::r13);
    m_Assembler.pop(asmjit::x86::r12);
    m_Assembler.pop(asmjit::x86::rdi);
    m_Assembler.pop(asmjit::x86::rsi);
    m_Assembler.pop(asmjit::x86::rbp);
    m_Assembler.pop(asmjit::x86::rbx);
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
        asmjit::Label ContinueFromTimerTest = m_Assembler.newLabel();
        const bool InSecondary = m_Assembler.InSecondarySection();

        if (InSecondary)
        {
            m_Assembler.JnsLabel("Continue_From_Timer_Test", ContinueFromTimerTest);
        }
        else
        {
            asmjit::Label TimerDonePath = m_Assembler.newLabel();
            m_Assembler.JsLabel("Timer_Done_Path", TimerDonePath);
            m_CodeBlock.Log("");
            m_Assembler.bind(ContinueFromTimerTest);
            m_Assembler.EnterSecondarySection();
            m_Assembler.bind(TimerDonePath);
        }

        RegSet.BeforeCallDirect();
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.CallThis(g_SystemTimer, MemberFuncAddress(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
        m_Assembler.add(asmjit::x86::rsp, 32);
        RegSet.AfterCallDirect();

        if (InSecondary)
        {
            m_Assembler.bind(ContinueFromTimerTest);
        }
        else
        {
            m_Assembler.JmpLabel("Continue_From_Timer_Test", ContinueFromTimerTest);
            m_Assembler.EnterPrimarySection();
        }
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

void CX64RecompilerOps::LB_KnownAddress(const asmjit::x86::Gp & Reg, uint32_t VAddr, bool SignExtend)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    uint32_t PAddr = 0;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_Assembler.xor_(Reg.r32(), Reg.r32());
        m_CodeBlock.Log("%s\nFailed to translate address %08X", __FUNCTION__, VAddr);
        if (g_DebugSettings.breakOnUnhandledMemory)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
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
        if (PAddr < m_MMU.RdramSize())
        {
            const uintptr_t src = reinterpret_cast<uintptr_t>(m_MMU.Rdram() + (PAddr ^ 3));
            m_Assembler.MoveConstToX64reg(asmjit::x86::r11, src, stdstr_f("RDRAM + (0x%X ^ 3)", PAddr).c_str());
            if (SignExtend)
            {
                m_Assembler.movsx(Reg.r32(), asmjit::x86::byte_ptr(asmjit::x86::r11));
            }
            else
            {
                m_Assembler.movzx(Reg.r32(), asmjit::x86::byte_ptr(asmjit::x86::r11));
            }
        }
        else
        {
            m_Assembler.xor_(Reg.r32(), Reg.r32());
        }
        break;
    default:
        m_Assembler.xor_(Reg.r32(), Reg.r32());
        if (g_DebugSettings.breakOnUnhandledMemory)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    }
}

void CX64RecompilerOps::LH_KnownAddress(const asmjit::x86::Gp & Reg, uint32_t VAddr, bool SignExtend)
{
    m_RegWorkingSet.SetX64Protected(Reg.id(), true);
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    uint32_t PAddr = 0;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_Assembler.xor_(Reg.r32(), Reg.r32());
        m_CodeBlock.Log("%s\nFailed to translate address %08X", __FUNCTION__, VAddr);
        if (g_DebugSettings.breakOnUnhandledMemory)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
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
        if (PAddr < m_MMU.RdramSize())
        {
            const uintptr_t src = reinterpret_cast<uintptr_t>(m_MMU.Rdram() + (PAddr ^ 2));
            m_Assembler.MoveConstToX64reg(asmjit::x86::r11, src, stdstr_f("RDRAM + (0x%X ^ 2)", PAddr).c_str());
            if (SignExtend)
            {
                m_Assembler.movsx(Reg.r32(), asmjit::x86::word_ptr(asmjit::x86::r11));
            }
            else
            {
                m_Assembler.movzx(Reg.r32(), asmjit::x86::word_ptr(asmjit::x86::r11));
            }
        }
        else
        {
            m_Assembler.xor_(Reg.r32(), Reg.r32());
        }
        break;
    default:
        m_Assembler.xor_(Reg.r32(), Reg.r32());
        if (g_DebugSettings.breakOnUnhandledMemory)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    }
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
    case 0x00000000u:
    case 0x00100000u:
    case 0x00200000u:
    case 0x00300000u:
    case 0x00400000u:
    case 0x00500000u:
    case 0x00600000u:
    case 0x00700000u:
        if (PAddr < m_MMU.RdramSize())
        {
            m_Assembler.MoveVariableToX64reg(Reg, m_MMU.Rdram() + PAddr, stdstr_f("RDRAM + 0x%X", PAddr).c_str(), ResultSigned);
        }
        else
        {
            LW_PhysicalAddressCall(Reg, PAddr, ResultSigned);
        }
        return false;
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
                LW_PhysicalAddressCall(Reg, PAddr, ResultSigned);
                break;
            }
        }
        return false;
    case 0x04300000u:
        switch (PAddr)
        {
        case 0x04300000u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.MI_MODE_REG, "MI_MODE_REG", ResultSigned); break;
        case 0x04300004u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.MI_VERSION_REG, "MI_VERSION_REG", ResultSigned); break;
        case 0x04300008u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.MI_INTR_REG, "MI_INTR_REG", ResultSigned); break;
        case 0x0430000Cu: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.MI_INTR_MASK_REG, "MI_INTR_MASK_REG", ResultSigned); break;
        default:
            m_Assembler.xor_(Reg, Reg);
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            break;
        }
        return false;
    case 0x04400000u:
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_VideoInterfaceHandler), "g_MMU->m_VideoInterfaceHandler");
        m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
        m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uintptr_t)&m_TempValue32, "m_TempValue32");
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
        m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11));
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_RegWorkingSet.AfterCallDirect();
        m_Assembler.MoveVariableToX64reg(Reg, &m_TempValue32, "m_TempValue32", ResultSigned);
        return false;
    case 0x04500000u:
        UpdateCounters(m_RegWorkingSet, false, true, false);
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_AudioInterfaceHandler), "g_MMU->m_AudioInterfaceHandler");
        m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
        m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uintptr_t)&m_TempValue32, "m_TempValue32");
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
        m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11));
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_RegWorkingSet.AfterCallDirect();
        m_Assembler.MoveVariableToX64reg(Reg, &m_TempValue32, "m_TempValue32", ResultSigned);
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
    case 0x04800000u:
        switch (PAddr)
        {
        case 0x04800018u: m_Assembler.MoveVariableToX64reg(Reg, &m_Reg.SI_STATUS_REG, "SI_STATUS_REG", ResultSigned); break;
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

void CX64RecompilerOps::LW_PhysicalAddressCall(const asmjit::x86::Gp & Reg, uint32_t PAddr, bool ResultSigned)
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr, stdstr_f("PAddr 0x%08X", PAddr).c_str());
    m_Assembler.MoveConstToX64reg(asmjit::x86::r8, reinterpret_cast<uintptr_t>(&m_TempValue32), "m_TempValue32");
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LW_PhysicalAddress), "CMipsMemoryVM::LW_PhysicalAddress");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.MoveVariableToX64reg(Reg, &m_TempValue32, "m_TempValue32", ResultSigned);
}

void CX64RecompilerOps::SB_KnownAddress(uint32_t VAddr, const asmjit::x86::Gp * ValueReg, uint32_t ValueConst)
{
    if (ValueReg != nullptr)
    {
        m_RegWorkingSet.SetX64Protected(ValueReg->id(), true);
    }
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        uint32_t PAddr = 0;
        if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
        {
            m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return;
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
                const uintptr_t dst = (uintptr_t)(m_MMU.Rdram() + (PAddr ^ 3));
                if (ValueReg != nullptr)
                {
                    m_Assembler.MovByteToAddress(dst, *ValueReg);
                }
                else
                {
                    m_Assembler.MovByteToAddress(dst, (uint8_t)ValueConst);
                }
            }
            else if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
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
}

void CX64RecompilerOps::SH_KnownAddress(uint32_t VAddr, const asmjit::x86::Gp * ValueReg, uint32_t ValueConst)
{
    if (ValueReg != nullptr)
    {
        m_RegWorkingSet.SetX64Protected(ValueReg->id(), true);
    }
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    else
    {
        uint32_t PAddr = 0;
        if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
        {
            m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
            if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            return;
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
                const uintptr_t dst = (uintptr_t)(m_MMU.Rdram() + (PAddr ^ 2));
                if (ValueReg != nullptr)
                {
                    m_Assembler.MovWordToAddress(dst, *ValueReg);
                }
                else
                {
                    m_Assembler.MovWordToAddress(dst, (uint16_t)ValueConst);
                }
            }
            else if (g_DebugSettings.breakOnUnhandledMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
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
            const uintptr_t dst = (uintptr_t)(m_MMU.Rdram() + PAddr);
            if (ValueReg != nullptr)
            {
                m_Assembler.MovDwordToAddress(dst, *ValueReg);
            }
            else
            {
                m_Assembler.MovDwordToAddress(dst, ValueConst);
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
                m_Assembler.MovDwordToAddress(dst, *ValueReg);
            }
            else
            {
                m_Assembler.MovDwordToAddress(dst, ValueConst);
            }
        }
        else if (PAddr < 0x04002000u)
        {
            const uintptr_t dst = (uintptr_t)(m_MMU.Imem() + (PAddr - 0x04001000u));
            if (ValueReg != nullptr)
            {
                m_Assembler.MovDwordToAddress(dst, *ValueReg);
            }
            else
            {
                m_Assembler.MovDwordToAddress(dst, ValueConst);
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
    case 0x04400000u:
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
        m_Assembler.MoveConstToX64reg(asmjit::x86::rcx, reinterpret_cast<uintptr_t>(&m_MMU.m_VideoInterfaceHandler), "g_MMU->m_VideoInterfaceHandler");
        m_Assembler.MoveConstToX64reg(asmjit::x86::rdx, PAddr & 0x1FFFFFFFu);
        m_Assembler.mov(asmjit::x86::r9d, 0xFFFFFFFFu);
        m_Assembler.sub(asmjit::x86::rsp, 32);
        m_Assembler.mov(asmjit::x86::r11, asmjit::x86::qword_ptr(asmjit::x86::rcx));
        m_Assembler.call(asmjit::x86::qword_ptr(asmjit::x86::r11, 8));
        m_Assembler.add(asmjit::x86::rsp, 32);
        m_RegWorkingSet.AfterCallDirect();
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

void CX64RecompilerOps::CompileLoadMemoryValue(asmjit::x86::Gp & AddressReg, const asmjit::x86::Gp & ValueReg, const asmjit::x86::Gp & /*ValueRegHi*/, uint8_t ValueSize, bool SignExtend)
{
    if (ValueReg.isValid())
    {
        m_RegWorkingSet.SetX64Protected(ValueReg.id(), true);
    }

    const bool CreatedAddressReg = !AddressReg.isValid();
    if (CreatedAddressReg)
    {
        AddressReg = BaseOffsetAddress();
    }
    else
    {
        m_RegWorkingSet.SetX64Protected(AddressReg.id(), true);
    }

    CRegInfo ExitRegSet = m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp);
    if (m_Instruction.WritesGPR() > 0 && ValueReg.isValid())
    {
        ExitRegSet.UnMap_GPR(m_Opcode.rt, false);
    }

    if (ValueSize == 16)
    {
        m_Assembler.test(AddressReg.r32(), 1);
        CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionRead32, &CX64Ops::JneLabel, &AddressReg);
    }
    else if (ValueSize == 32)
    {
        m_Assembler.test(AddressReg.r32(), 3);
        CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionRead32, &CX64Ops::JneLabel, &AddressReg);
    }
    else if (ValueSize == 64)
    {
        m_Assembler.test(AddressReg.r32(), 7);
        CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionRead32, &CX64Ops::JneLabel, &AddressReg);
    }

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

    asmjit::x86::Gp DestReg = ValueReg;
    if (!DestReg.isValid())
    {
        if (ValueSize == 64)
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rt == m_Opcode.base ? m_Opcode.base : -1);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, SignExtend, -1);
        }
        DestReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
    }

    if (ValueSize == 64)
    {
        m_Assembler.mov(DestReg.r64(), asmjit::x86::qword_ptr(AddressReg, HostOffsetReg));
        m_Assembler.ror(DestReg.r64(), 32);
    }
    else if (ValueSize == 32)
    {
        m_Assembler.mov(DestReg.r32(), asmjit::x86::dword_ptr(AddressReg, HostOffsetReg));
    }
    else
    {
        m_Assembler.xor_(AddressReg.r32(), ValueSize == 8 ? 3 : 2);
        if (ValueSize == 8)
        {
            if (SignExtend)
            {
                m_Assembler.movsx(DestReg.r32(), asmjit::x86::byte_ptr(AddressReg, HostOffsetReg));
            }
            else
            {
                m_Assembler.movzx(DestReg.r32(), asmjit::x86::byte_ptr(AddressReg, HostOffsetReg));
            }
        }
        else if (ValueSize == 16)
        {
            if (SignExtend)
            {
                m_Assembler.movsx(DestReg.r32(), asmjit::x86::word_ptr(AddressReg, HostOffsetReg));
            }
            else
            {
                m_Assembler.movzx(DestReg.r32(), asmjit::x86::word_ptr(AddressReg, HostOffsetReg));
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
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
    if (ValueSize == 64)
    {
        m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uintptr_t)&m_TempValue64, "TempValue64");
    }
    else
    {
        m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uintptr_t)&m_TempValue32, "TempValue32");
        m_Assembler.MoveConstToVariable(&m_TempValue32, "TempValue32", 0);
    }
    m_Assembler.sub(asmjit::x86::rsp, 32);
    if (ValueSize == 8)
    {
        m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LB_VAddr32), "CMipsMemoryVM::LB_VAddr32");
    }
    else if (ValueSize == 16)
    {
        m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LH_VAddr32), "CMipsMemoryVM::LH_VAddr32");
    }
    else if (ValueSize == 32)
    {
        m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LW_VAddr32), "CMipsMemoryVM::LW_VAddr32");
    }
    else
    {
        m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::LD_VAddr32), "CMipsMemoryVM::LD_VAddr32");
    }
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
    m_RegWorkingSet.AfterCallDirect();

    asmjit::Label SlowPathException = m_Assembler.newLabel();
    m_Assembler.JeLabel(stdstr_f("MemoryReadMap_%X_Exception", m_CompilePC).c_str(), SlowPathException);

    DestReg = ValueReg;
    if (!DestReg.isValid())
    {
        if (ValueSize == 64)
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rt == m_Opcode.base ? m_Opcode.base : -1);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, SignExtend, -1);
        }
        DestReg = m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt);
    }
    if (ValueSize == 64)
    {
        m_Assembler.MoveVariable64ToX64reg(DestReg, &m_TempValue64, "TempValue64");
    }
    else
    {
        m_Assembler.MoveVariableToX64reg(DestReg, &m_TempValue32, "TempValue32", false);
        if (SignExtend && ValueSize != 32)
        {
            if (ValueSize == 8)
            {
                m_Assembler.movsx(DestReg.r32(), DestReg.r8Lo());
            }
            else if (ValueSize == 16)
            {
                m_Assembler.movsx(DestReg.r32(), DestReg.r16());
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
    }
    m_Assembler.JmpLabel(AfterLoadLabel.c_str(), AfterLoad);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MemoryReadMap_%X_Exception:", m_CompilePC);
    m_Assembler.bind(SlowPathException);
    CompileExit(m_CompilePC, (uint32_t)-1, ValueSize == 64 ? PreMapRegSet.WithAddedCycles(g_GameSettings.countPerOp) : ExitRegSet, ExitReason_Exception);

    m_Assembler.EnterPrimarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      %s:", AfterLoadLabel.c_str());
    m_Assembler.bind(AfterLoad);

    if (CreatedAddressReg)
    {
        m_RegWorkingSet.SetX64Protected(AddressReg.id(), false);
    }
}

void CX64RecompilerOps::CompileStoreMemoryValue(asmjit::x86::Gp AddressReg, const asmjit::x86::Gp & ValueReg, const asmjit::x86::Gp & /*ValueRegHi*/, uint64_t Value, uint8_t ValueSize)
{
    if (ValueReg.isValid())
    {
        m_RegWorkingSet.SetX64Protected(ValueReg.id(), true);
    }

    if (!AddressReg.isValid())
    {
        AddressReg = BaseOffsetAddress();
    }
    else
    {
        m_RegWorkingSet.SetX64Protected(AddressReg.id(), true);
    }

    CRegInfo ExitRegSet = m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp);
    if (ValueSize == 16)
    {
        m_Assembler.test(AddressReg.r32(), 1);
        CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionWrite32, &CX64Ops::JneLabel, &AddressReg);
    }
    else if (ValueSize == 32)
    {
        m_Assembler.test(AddressReg.r32(), 3);
        CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionWrite32, &CX64Ops::JneLabel, &AddressReg);
    }
    else if (ValueSize == 64)
    {
        m_Assembler.test(AddressReg.r32(), 7);
        CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_AddressErrorExceptionWrite32, &CX64Ops::JneLabel, &AddressReg);
    }
    else if (ValueSize != 8)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    const asmjit::x86::Gp PageIndexReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    m_Assembler.mov(PageIndexReg.r32(), AddressReg.r32());
    m_Assembler.shr(PageIndexReg.r32(), 12);

    const asmjit::x86::Gp HostOffsetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(HostOffsetReg, &m_MMU.m_MemoryWriteMap, "MMU->m_MemoryWriteMap");
    m_Assembler.mov(HostOffsetReg.r64(), asmjit::x86::qword_ptr(HostOffsetReg, PageIndexReg, 3));
    m_Assembler.cmp(HostOffsetReg.r64(), (int64_t)-1);

    const stdstr AfterStoreLabel = stdstr_f("MemoryWriteMap_%X_AfterStore", m_CompilePC);
    const stdstr MapMissLabel = stdstr_f("MemoryWriteMap_%X_Miss", m_CompilePC);
    asmjit::Label AfterStore = m_Assembler.newLabel();
    asmjit::Label SlowPath = m_Assembler.newLabel();
    m_Assembler.JeLabel(MapMissLabel.c_str(), SlowPath);

    if (ValueSize == 8)
    {
        m_Assembler.xor_(AddressReg.r32(), 3);
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::byte_ptr(AddressReg, HostOffsetReg), (uint8_t)(Value & 0xFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::byte_ptr(AddressReg, HostOffsetReg), ValueReg.r8Lo());
        }
    }
    else if (ValueSize == 16)
    {
        m_Assembler.xor_(AddressReg.r32(), 2);
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::word_ptr(AddressReg, HostOffsetReg), (uint16_t)(Value & 0xFFFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::word_ptr(AddressReg, HostOffsetReg), ValueReg.r16());
        }
    }
    else if (ValueSize == 32)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, HostOffsetReg), (uint32_t)Value);
        }
        else
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, HostOffsetReg), ValueReg.r32());
        }
    }
    else if (ValueSize == 64)
    {
        const asmjit::x86::Gp StoreReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
        if (!ValueReg.isValid())
        {
            m_Assembler.MoveConstToX64reg(StoreReg, (Value << 32) | (Value >> 32));
        }
        else
        {
            m_Assembler.mov(StoreReg.r64(), ValueReg.r64());
            m_Assembler.ror(StoreReg.r64(), 32);
        }
        m_Assembler.mov(asmjit::x86::qword_ptr(AddressReg, HostOffsetReg), StoreReg.r64());
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    m_Assembler.EnterSecondarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      %s:", MapMissLabel.c_str());
    m_Assembler.bind(SlowPath);

    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_JUMP);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::edx, AddressReg.r32());
    uintptr_t FunctPtr = 0;
    const char * FunctName = nullptr;
    if (ValueSize == 8)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::r8d, (uint32_t)(Value & 0xFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::r8d, ValueReg.r32());
        }
        FunctPtr = MemberFuncAddress(&CMipsMemoryVM::SB_VAddr32);
        FunctName = "CMipsMemoryVM::SB_VAddr32";
    }
    else if (ValueSize == 16)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::r8d, (uint32_t)(Value & 0xFFFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::r8d, ValueReg.r32());
        }
        FunctPtr = MemberFuncAddress(&CMipsMemoryVM::SH_VAddr32);
        FunctName = "CMipsMemoryVM::SH_VAddr32";
    }
    else if (ValueSize == 32)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::r8d, (uint32_t)Value);
        }
        else
        {
            m_Assembler.mov(asmjit::x86::r8d, ValueReg.r32());
        }
        FunctPtr = MemberFuncAddress(&CMipsMemoryVM::SW_VAddr32);
        FunctName = "CMipsMemoryVM::SW_VAddr32";
    }
    else if (ValueSize == 64)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.MoveConstToX64reg(asmjit::x86::r8, Value);
        }
        else
        {
            m_Assembler.mov(asmjit::x86::r8, ValueReg.r64());
        }
        FunctPtr = MemberFuncAddress(&CMipsMemoryVM::SD_VAddr32);
        FunctName = "CMipsMemoryVM::SD_VAddr32";
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_MMU, FunctPtr, FunctName);
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

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MemoryWriteMap_%X_Exception:", m_CompilePC);
    m_Assembler.bind(SlowPathException);
    CompileExit(m_CompilePC, (uint32_t)-1, ExitRegSet, ExitReason_Exception);

    m_Assembler.EnterPrimarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      %s:", AfterStoreLabel.c_str());
    m_Assembler.bind(AfterStore);
}

void CX64RecompilerOps::StoreWordPartial(bool Left)
{
    // The variable shift count has to be in cl, so ecx is reserved before anything else is mapped
    m_RegWorkingSet.UnMap_X64reg(asmjit::x86::rcx);
    const asmjit::x86::Gp OffsetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1);

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    asmjit::x86::Gp AddressReg = BaseOffsetAddress();
    m_Assembler.mov(OffsetReg.r32(), AddressReg.r32());
    m_Assembler.and_(OffsetReg.r32(), 3);
    m_Assembler.and_(AddressReg.r32(), ~3u);

    // Read the aligned word, directly mapped memory uses the read map, everything else goes through
    // MemoryValue32 which raises a store TLB miss for the unaligned address when it fails
    const CRegInfo ExitRegSet = m_RegWorkingSet.WithAddedCycles(g_GameSettings.countPerOp);
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    const asmjit::x86::Gp PageIndexReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    m_Assembler.mov(PageIndexReg.r32(), AddressReg.r32());
    m_Assembler.shr(PageIndexReg.r32(), 12);
    const asmjit::x86::Gp HostOffsetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveVariable64ToX64reg(HostOffsetReg, &m_MMU.m_MemoryReadMap, "MMU->m_MemoryReadMap");
    m_Assembler.mov(HostOffsetReg.r64(), asmjit::x86::qword_ptr(HostOffsetReg, PageIndexReg, 3));
    m_Assembler.cmp(HostOffsetReg.r64(), (int64_t)-1);
    asmjit::Label ReadMiss = m_Assembler.newLabel();
    asmjit::Label ReadDone = m_Assembler.newLabel();
    asmjit::Label ReadException = m_Assembler.newLabel();
    m_Assembler.JeLabel(stdstr_f("MemoryReadMap_%X_Miss", m_CompilePC).c_str(), ReadMiss);
    m_Assembler.mov(ValueReg.r32(), asmjit::x86::dword_ptr(AddressReg, HostOffsetReg));

    // The slow path is compiled with the register state at the read, before anything else is mapped
    const CX64RegInfo ReadRegSet(m_RegWorkingSet);
    m_Assembler.EnterSecondarySection();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MemoryReadMap_%X_Miss:", m_CompilePC);
    m_Assembler.bind(ReadMiss);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::edx, AddressReg.r32());
    m_Assembler.MoveConstToX64reg(asmjit::x86::r8, (uintptr_t)&m_TempValue32, "TempValue32");
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(&m_MMU, MemberFuncAddress(&CMipsMemoryVM::MemoryValue32), "CMipsMemoryVM::MemoryValue32");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.JeLabel(stdstr_f("MemoryReadMap_%X_Exception", m_CompilePC).c_str(), ReadException);
    m_Assembler.MoveVariableToX64reg(ValueReg, &m_TempValue32, "TempValue32", false);
    m_Assembler.JmpLabel(stdstr_f("MemoryReadMap_%X_Done", m_CompilePC).c_str(), ReadDone);

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MemoryReadMap_%X_Exception:", m_CompilePC);
    m_Assembler.bind(ReadException);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (m_PipelineStage != PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_JUMP);
    }
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.mov(asmjit::x86::edx, AddressReg.r32());
    m_Assembler.or_(asmjit::x86::edx, OffsetReg.r32());
    m_Assembler.movsxd(asmjit::x86::rdx, asmjit::x86::edx);
    m_Assembler.mov(asmjit::x86::r8d, EXC_WMISS);
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(g_Reg, MemberFuncAddress(&CRegisters::TriggerAddressException), "CRegisters::TriggerAddressException");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();
    CompileExit(m_CompilePC, (uint32_t)-1, ExitRegSet, ExitReason_Exception);
    m_Assembler.EnterPrimarySection();
    m_RegWorkingSet = ReadRegSet;
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MemoryReadMap_%X_Done:", m_CompilePC);
    m_Assembler.bind(ReadDone);

    const asmjit::x86::Gp TableReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    m_Assembler.MoveConstToX64reg(TableReg, (uintptr_t)(Left ? R4300iOp::SWL_MASK : R4300iOp::SWR_MASK), Left ? "R4300iOp::SWL_MASK" : "R4300iOp::SWR_MASK");
    m_Assembler.and_(ValueReg.r32(), asmjit::x86::dword_ptr(TableReg, OffsetReg.r64(), 2));

    const asmjit::x86::Gp RtReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
    m_Assembler.MoveConstToX64reg(TableReg, (uintptr_t)(Left ? R4300iOp::SWL_SHIFT : R4300iOp::SWR_SHIFT), Left ? "R4300iOp::SWL_SHIFT" : "R4300iOp::SWR_SHIFT");
    m_Assembler.mov(OffsetReg.r32(), asmjit::x86::dword_ptr(TableReg, OffsetReg.r64(), 2));
    if (Left)
    {
        m_Assembler.shr(RtReg.r32(), asmjit::x86::cl);
    }
    else
    {
        m_Assembler.shl(RtReg.r32(), asmjit::x86::cl);
    }
    m_Assembler.add(ValueReg.r32(), RtReg.r32());

    CompileStoreMemoryValue(AddressReg, ValueReg, asmjit::x86::Gp(), 0, 32);
}

void CX64RecompilerOps::LoadWordPartial(bool Left)
{
    // The variable shift count has to be in cl, so ecx is reserved before anything else is mapped
    m_RegWorkingSet.UnMap_X64reg(asmjit::x86::rcx);
    const asmjit::x86::Gp OffsetReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1);

    m_RegWorkingSet.ProtectGPR(m_Opcode.base);
    m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
    if (m_RegWorkingSet.IsMapped(m_Opcode.base) && m_RegWorkingSet.Is32Bit(m_Opcode.base) && !m_RegWorkingSet.IsSigned(m_Opcode.base))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    asmjit::x86::Gp AddressReg = BaseOffsetAddress();
    m_Assembler.mov(OffsetReg.r32(), AddressReg.r32());
    m_Assembler.and_(OffsetReg.r32(), 3);
    m_Assembler.and_(AddressReg.r32(), ~3u);

    // rt is still read after the load, so the load goes to a temp and rt keeps its value if the load raises an exception
    const asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
    CompileLoadMemoryValue(AddressReg, ValueReg, asmjit::x86::Gp(), 32, false);

    const asmjit::x86::Gp TableReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpq(), -1, asmjit::RegType::kX86_Gpq);
    const asmjit::x86::Gp RtReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), m_Opcode.rt);
    m_Assembler.MoveConstToX64reg(TableReg, (uintptr_t)(Left ? R4300iOp::LWL_MASK : R4300iOp::LWR_MASK), Left ? "R4300iOp::LWL_MASK" : "R4300iOp::LWR_MASK");
    m_Assembler.and_(RtReg.r32(), asmjit::x86::dword_ptr(TableReg, OffsetReg.r64(), 2));
    m_Assembler.MoveConstToX64reg(TableReg, (uintptr_t)(Left ? R4300iOp::LWL_SHIFT : R4300iOp::LWR_SHIFT), Left ? "R4300iOp::LWL_SHIFT" : "R4300iOp::LWR_SHIFT");
    m_Assembler.mov(OffsetReg.r32(), asmjit::x86::dword_ptr(TableReg, OffsetReg.r64(), 2));
    if (Left)
    {
        m_Assembler.shl(ValueReg.r32(), asmjit::x86::cl);
    }
    else
    {
        m_Assembler.shr(ValueReg.r32(), asmjit::x86::cl);
    }
    m_Assembler.add(RtReg.r32(), ValueReg.r32());

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMap(m_Opcode.rt).r32(), RtReg.r32());
    }
}

asmjit::x86::Gp CX64RecompilerOps::BaseOffsetAddress()
{
    asmjit::x86::Gp AddressReg;
    if (m_RegWorkingSet.IsMapped(m_Opcode.base))
    {
        if (m_Opcode.offset != 0)
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.base);
            AddressReg = m_RegWorkingSet.Map_TempReg(asmjit::x86::Gpd(), -1);
            m_Assembler.lea(AddressReg, asmjit::x86::ptr(m_RegWorkingSet.GetMipsRegMap(m_Opcode.base).r32(), (int16_t)m_Opcode.offset));
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

void CX64RecompilerOps::ResetMemoryStack()
{
    constexpr int32_t MipsReg = 29;
    if (m_RegWorkingSet.IsConst(MipsReg))
    {
        m_Assembler.MoveConstToVariable(&m_Reg.m_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], m_RegWorkingSet.GetMipsRegLo(MipsReg));
    }
    else if (m_RegWorkingSet.IsMapped(MipsReg))
    {
        m_Assembler.MovDwordToVariable(&m_Reg.m_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], m_RegWorkingSet.GetMipsRegMap(MipsReg));
    }

    const asmjit::x86::Gp MemoryStackReg = m_RegWorkingSet.Get_MemoryStack();
    if (MemoryStackReg.isValid())
    {
        m_CodeBlock.Log("    regcache: unallocate memory stack");
        m_RegWorkingSet.SetX64Mapped(MemoryStackReg.id(), CX64RegInfo::NotMapped);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.sub(asmjit::x86::rsp, 32);
    m_Assembler.CallThis(m_Recompiler, MemberFuncAddress(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos");
    m_Assembler.add(asmjit::x86::rsp, 32);
    m_RegWorkingSet.AfterCallDirect();
}

#endif