/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>
#include <Project64-core/N64System/N64Class.h>
#include <stdio.h>
#include "x86CodeLog.h"

CCodeSection * CRecompilerOps::m_Section = NULL;
CRegInfo       CRecompilerOps::m_RegWorkingSet;
STEP_TYPE      CRecompilerOps::m_NextInstruction;
uint32_t       CRecompilerOps::m_CompilePC;
OPCODE         CRecompilerOps::m_Opcode;
uint32_t       CRecompilerOps::m_BranchCompare = 0;

void CRecompilerOps::CompileReadTLBMiss(uint32_t VirtualAddress, x86Reg LookUpReg)
{
    MoveConstToVariable(VirtualAddress, g_TLBLoadAddress, "TLBLoadAddress");
    TestX86RegToX86Reg(LookUpReg, LookUpReg);
    m_Section->CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBReadMiss, false, JeLabel32);
}

void CRecompilerOps::CompileReadTLBMiss(x86Reg AddressReg, x86Reg LookUpReg)
{
    MoveX86regToVariable(AddressReg, g_TLBLoadAddress, "TLBLoadAddress");
    TestX86RegToX86Reg(LookUpReg, LookUpReg);
    m_Section->CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBReadMiss, false, JeLabel32);
}

void CRecompilerOps::CompileWriteTLBMiss(x86Reg AddressReg, x86Reg LookUpReg)
{
    MoveX86regToVariable(AddressReg, &g_TLBStoreAddress, "g_TLBStoreAddress");
    TestX86RegToX86Reg(LookUpReg, LookUpReg);
    m_Section->CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBWriteMiss, false, JeLabel32);
}

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

/************************** Branch functions  ************************/
void CRecompilerOps::Compile_Branch(CRecompilerOps::BranchFunction CompareFunc, BRANCH_TYPE BranchType, bool Link)
{
    static CRegInfo RegBeforeDelay;
    static bool EffectDelaySlot;

    if (m_NextInstruction == NORMAL)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8)
        {
            return;
        }

        if ((m_CompilePC & 0xFFC) != 0xFFC)
        {
            switch (BranchType)
            {
            case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0); break;
            case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, m_Opcode.rt); break;
            case BranchTypeCop1:
            {
                OPCODE Command;

                if (!g_MMU->LW_VAddr(m_CompilePC + 4, Command.Hex))
                {
                    g_Notify->FatalError(GS(MSG_FAIL_LOAD_WORD));
                }

                EffectDelaySlot = false;
                if (Command.op == R4300i_CP1)
                {
                    if ((Command.fmt == R4300i_COP1_S && (Command.funct & 0x30) == 0x30) ||
                        (Command.fmt == R4300i_COP1_D && (Command.funct & 0x30) == 0x30))
                    {
                        EffectDelaySlot = true;
                    }
                }
            }
            break;
            default:
                if (bHaveDebugger()) { g_Notify->DisplayError("Unknown branch type"); }
            }
        }
        else
        {
            EffectDelaySlot = true;
        }
        m_Section->m_Jump.JumpPC = m_CompilePC;
        m_Section->m_Jump.TargetPC = m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4;
        if (m_Section->m_JumpSection != NULL)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", m_Section->m_JumpSection->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel.Format("Exit_%X_jump_%X", m_Section->m_EnterPC, m_Section->m_Jump.TargetPC);
        }
        m_Section->m_Jump.LinkLocation = NULL;
        m_Section->m_Jump.LinkLocation2 = NULL;
        m_Section->m_Jump.DoneDelaySlot = false;
        m_Section->m_Cont.JumpPC = m_CompilePC;
        m_Section->m_Cont.TargetPC = m_CompilePC + 8;
        if (m_Section->m_ContinueSection != NULL)
        {
            m_Section->m_Cont.BranchLabel.Format("Section_%d", m_Section->m_ContinueSection->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel.Format("Exit_%X_continue_%X", m_Section->m_EnterPC, m_Section->m_Cont.TargetPC);
        }
        m_Section->m_Cont.LinkLocation = NULL;
        m_Section->m_Cont.LinkLocation2 = NULL;
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
            UnMap_GPR(31, false);
            m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
            m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
        }
        if (EffectDelaySlot)
        {
            if ((m_CompilePC & 0xFFC) != 0xFFC)
            {
                m_Section->m_Cont.BranchLabel = m_Section->m_ContinueSection != NULL ? "Continue" : "ExitBlock";
                m_Section->m_Jump.BranchLabel = m_Section->m_JumpSection != NULL ? "Jump" : "ExitBlock";
            }
            else
            {
                m_Section->m_Cont.BranchLabel = "Continue";
                m_Section->m_Jump.BranchLabel = "Jump";
            }
            if (m_Section->m_Jump.TargetPC != m_Section->m_Cont.TargetPC)
            {
                CompareFunc();
            }
            if (!m_Section->m_Jump.FallThrough && !m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != NULL)
                {
                    CPU_Message("");
                    CPU_Message("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    SetJump32((uint32_t *)m_Section->m_Jump.LinkLocation, (uint32_t *)m_RecompPos);
                    m_Section->m_Jump.LinkLocation = NULL;
                    if (m_Section->m_Jump.LinkLocation2 != NULL)
                    {
                        SetJump32((uint32_t *)m_Section->m_Jump.LinkLocation2, (uint32_t *)m_RecompPos);
                        m_Section->m_Jump.LinkLocation2 = NULL;
                    }
                    m_Section->m_Jump.FallThrough = true;
                }
                else if (m_Section->m_Cont.LinkLocation != NULL)
                {
                    CPU_Message("");
                    CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    SetJump32((uint32_t *)m_Section->m_Cont.LinkLocation, (uint32_t *)m_RecompPos);
                    m_Section->m_Cont.LinkLocation = NULL;
                    if (m_Section->m_Cont.LinkLocation2 != NULL)
                    {
                        SetJump32((uint32_t *)m_Section->m_Cont.LinkLocation2, (uint32_t *)m_RecompPos);
                        m_Section->m_Cont.LinkLocation2 = NULL;
                    }
                    m_Section->m_Cont.FallThrough = true;
                }
            }
            if ((m_CompilePC & 0xFFC) == 0xFFC)
            {
                uint8_t * DelayLinkLocation = NULL;
                if (m_Section->m_Jump.FallThrough)
                {
                    if (m_Section->m_Jump.LinkLocation != NULL || m_Section->m_Jump.LinkLocation2 != NULL)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    MoveConstToVariable(m_Section->m_Jump.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                }
                else if (m_Section->m_Cont.FallThrough)
                {
                    if (m_Section->m_Cont.LinkLocation != NULL || m_Section->m_Cont.LinkLocation2 != NULL)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    MoveConstToVariable(m_Section->m_Cont.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                }

                if (m_Section->m_Jump.LinkLocation != NULL || m_Section->m_Jump.LinkLocation2 != NULL)
                {
                    JmpLabel8("DoDelaySlot", 0);
                    if (DelayLinkLocation != NULL) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                    DelayLinkLocation = (uint8_t *)(m_RecompPos - 1);

                    CPU_Message("      ");
                    CPU_Message("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    SetJump32(m_Section->m_Jump.LinkLocation, (uint32_t *)m_RecompPos);
                    m_Section->m_Jump.LinkLocation = NULL;
                    if (m_Section->m_Jump.LinkLocation2 != NULL)
                    {
                        SetJump32(m_Section->m_Jump.LinkLocation2, (uint32_t *)m_RecompPos);
                        m_Section->m_Jump.LinkLocation2 = NULL;
                    }
                    MoveConstToVariable(m_Section->m_Jump.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                }
                if (m_Section->m_Cont.LinkLocation != NULL || m_Section->m_Cont.LinkLocation2 != NULL)
                {
                    JmpLabel8("DoDelaySlot", 0);
                    if (DelayLinkLocation != NULL) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                    DelayLinkLocation = (uint8_t *)(m_RecompPos - 1);

                    CPU_Message("      ");
                    CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    SetJump32(m_Section->m_Cont.LinkLocation, (uint32_t *)m_RecompPos);
                    m_Section->m_Cont.LinkLocation = NULL;
                    if (m_Section->m_Cont.LinkLocation2 != NULL)
                    {
                        SetJump32(m_Section->m_Cont.LinkLocation2, (uint32_t *)m_RecompPos);
                        m_Section->m_Cont.LinkLocation2 = NULL;
                    }
                    MoveConstToVariable(m_Section->m_Cont.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                }
                if (DelayLinkLocation)
                {
                    CPU_Message("");
                    CPU_Message("      DoDelaySlot:");
                    SetJump8(DelayLinkLocation, m_RecompPos);
                }
                OverflowDelaySlot(false);
                return;
            }
            ResetX86Protection();
            RegBeforeDelay = m_RegWorkingSet;
        }
        m_NextInstruction = DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        if (EffectDelaySlot)
        {
            CJumpInfo * FallInfo = m_Section->m_Jump.FallThrough ? &m_Section->m_Jump : &m_Section->m_Cont;
            CJumpInfo * JumpInfo = m_Section->m_Jump.FallThrough ? &m_Section->m_Cont : &m_Section->m_Jump;

            if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot)
            {
                ResetX86Protection();
                FallInfo->RegSet = m_RegWorkingSet;
                if (FallInfo == &m_Section->m_Jump)
                {
                    if (m_Section->m_JumpSection != NULL)
                    {
                        m_Section->m_Jump.BranchLabel.Format("Section_%d", m_Section->m_JumpSection->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Jump.BranchLabel = "ExitBlock";
                    }
                    if (FallInfo->TargetPC <= m_CompilePC)
                    {
                        UpdateCounters(m_Section->m_Jump.RegSet, true, true);
                        CPU_Message("CompileSystemCheck 12");
                        CompileSystemCheck(FallInfo->TargetPC, m_Section->m_Jump.RegSet);
                        ResetX86Protection();
                        FallInfo->ExitReason = CExitInfo::Normal_NoSysCheck;
                        FallInfo->JumpPC = (uint32_t)-1;
                    }
                }
                else
                {
                    if (m_Section->m_ContinueSection != NULL)
                    {
                        m_Section->m_Cont.BranchLabel.Format("Section_%d", m_Section->m_ContinueSection->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Cont.BranchLabel = "ExitBlock";
                    }
                }
                FallInfo->DoneDelaySlot = true;
                if (!JumpInfo->DoneDelaySlot)
                {
                    FallInfo->FallThrough = false;
                    JmpLabel32(FallInfo->BranchLabel.c_str(), 0);
                    FallInfo->LinkLocation = (uint32_t *)(m_RecompPos - 4);

                    if (JumpInfo->LinkLocation != NULL)
                    {
                        CPU_Message("      %s:", JumpInfo->BranchLabel.c_str());
                        SetJump32((uint32_t *)JumpInfo->LinkLocation, (uint32_t *)m_RecompPos);
                        JumpInfo->LinkLocation = NULL;
                        if (JumpInfo->LinkLocation2 != NULL)
                        {
                            SetJump32((uint32_t *)JumpInfo->LinkLocation2, (uint32_t *)m_RecompPos);
                            JumpInfo->LinkLocation2 = NULL;
                        }
                        JumpInfo->FallThrough = true;
                        m_NextInstruction = DO_DELAY_SLOT;
                        m_RegWorkingSet = RegBeforeDelay;
                        return;
                    }
                }
            }
        }
        else
        {
            if (m_Section->m_Jump.TargetPC != m_Section->m_Cont.TargetPC)
            {
                CompareFunc();
                ResetX86Protection();
                m_Section->m_Cont.RegSet = m_RegWorkingSet;
                m_Section->m_Jump.RegSet = m_RegWorkingSet;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
                m_Section->m_Cont.RegSet = m_RegWorkingSet;
                if (m_Section->m_ContinueSection == NULL && m_Section->m_JumpSection != NULL)
                {
                    m_Section->m_ContinueSection = m_Section->m_JumpSection;
                    m_Section->m_JumpSection = NULL;
                }
                if (m_Section->m_ContinueSection != NULL)
                {
                    m_Section->m_Cont.BranchLabel.Format("Section_%d", m_Section->m_ContinueSection->m_SectionID);
                }
                else
                {
                    m_Section->m_Cont.BranchLabel = "ExitBlock";
                }
            }
        }
        m_Section->GenerateSectionLinkage();
        m_NextInstruction = END_BLOCK;
    }
    else
    {
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction).c_str());
        }
    }
}

void CRecompilerOps::Compile_BranchLikely(BranchFunction CompareFunc, bool Link)
{
    if (m_NextInstruction == NORMAL)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

        if (!g_System->bLinkBlocks() || (m_CompilePC & 0xFFC) == 0xFFC)
        {
            m_Section->m_Jump.JumpPC = m_CompilePC;
            m_Section->m_Jump.TargetPC = m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4;
            m_Section->m_Cont.JumpPC = m_CompilePC;
            m_Section->m_Cont.TargetPC = m_CompilePC + 8;
        }
        else
        {
            if (m_Section->m_Jump.JumpPC != m_CompilePC)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_Section->m_Cont.JumpPC != m_CompilePC)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_Section->m_Cont.TargetPC != m_CompilePC + 8)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }

        if (m_Section->m_JumpSection != NULL)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }

        if (m_Section->m_ContinueSection != NULL)
        {
            m_Section->m_Cont.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel = "ExitBlock";
        }

        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = NULL;
        m_Section->m_Jump.LinkLocation2 = NULL;
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = NULL;
        m_Section->m_Cont.LinkLocation2 = NULL;

        if (Link)
        {
            UnMap_GPR(31, false);
            m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
            m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
        }

        CompareFunc();
        ResetX86Protection();

        m_Section->m_Cont.RegSet = m_RegWorkingSet;
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != NULL)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }

            if (m_Section->m_Jump.LinkLocation != NULL || m_Section->m_Jump.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != NULL)
                {
                    SetJump32(m_Section->m_Jump.LinkLocation, (uint32_t *)m_RecompPos);
                    m_Section->m_Jump.LinkLocation = NULL;
                    if (m_Section->m_Jump.LinkLocation2 != NULL)
                    {
                        SetJump32(m_Section->m_Jump.LinkLocation2, (uint32_t *)m_RecompPos);
                        m_Section->m_Jump.LinkLocation2 = NULL;
                    }
                }

                MoveConstToVariable(m_Section->m_Jump.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                OverflowDelaySlot(false);
                CPU_Message("      ");
                CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
            }
            else if (!m_Section->m_Cont.FallThrough)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            if (m_Section->m_Cont.LinkLocation != NULL)
            {
                SetJump32(m_Section->m_Cont.LinkLocation, (uint32_t *)m_RecompPos);
                m_Section->m_Cont.LinkLocation = NULL;
                if (m_Section->m_Cont.LinkLocation2 != NULL)
                {
                    SetJump32(m_Section->m_Cont.LinkLocation2, (uint32_t *)m_RecompPos);
                    m_Section->m_Cont.LinkLocation2 = NULL;
                }
            }
            m_Section->CompileExit(m_CompilePC, m_CompilePC + 8, m_Section->m_Cont.RegSet, CExitInfo::Normal, true, NULL);
            return;
        }
        else
        {
            m_NextInstruction = DO_DELAY_SLOT;
        }

        if (g_System->bLinkBlocks())
        {
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->GenerateSectionLinkage();
            m_NextInstruction = END_BLOCK;
        }
        else
        {
            if (m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != NULL)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                m_Section->GenerateSectionLinkage();
                m_NextInstruction = END_BLOCK;
            }
        }
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        ResetX86Protection();
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->GenerateSectionLinkage();
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nBranchLikely\nNextInstruction = %X", m_NextInstruction).c_str());
    }
}

void CRecompilerOps::BNE_Compare()
{
    uint8_t *Jump = NULL;

    if (IsKnown(m_Opcode.rs) && IsKnown(m_Opcode.rt))
    {
        if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt))
        {
            if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt))
            {
                CRecompilerOps::UnknownOpcode();
            }
            else if (GetMipsRegLo(m_Opcode.rs) != GetMipsRegLo(m_Opcode.rt))
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rs);
            ProtectGPR(m_Opcode.rt);
            if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt))
            {
                CompX86RegToX86Reg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(x86_Any, m_Opcode.rs, true) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, true) : GetMipsRegMapHi(m_Opcode.rt)
                    );

                if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel8("continue", 0);
                    Jump = m_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, m_RecompPos);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
            }
            else
            {
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(ConstReg) || Is64Bit(MappedReg))
            {
                if (Is32Bit(ConstReg) || Is32Bit(MappedReg))
                {
                    ProtectGPR(MappedReg);
                    if (Is32Bit(MappedReg))
                    {
                        CompConstToX86reg(Map_TempReg(x86_Any, MappedReg, true), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel8("continue", 0);
                    Jump = m_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, m_RecompPos);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
            }
            else
            {
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rs) || IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!g_System->b32BitCore())
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    CompConstToVariable((GetMipsRegLo_S(KnownReg) >> 31), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    ProtectGPR(KnownReg);
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel8("continue", 0);
                Jump = m_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        if (IsConst(KnownReg))
        {
            CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        else
        {
            CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);

            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");

                SetJump8(Jump, m_RecompPos);
            }
        }
        else
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
    }
    else
    {
        x86Reg Reg = x86_Any;

        if (!g_System->b32BitCore())
        {
            Reg = Map_TempReg(x86_Any, m_Opcode.rt, true);
            CompX86regToVariable(Reg, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel8("continue", 0);
                Jump = m_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }

        Reg = Map_TempReg(Reg, m_Opcode.rt, false);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        if (m_Section->m_Cont.FallThrough)
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, m_RecompPos);
            }
        }
        else
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
    }
}

void CRecompilerOps::BEQ_Compare()
{
    uint8_t *Jump = NULL;

    if (IsKnown(m_Opcode.rs) && IsKnown(m_Opcode.rt))
    {
        if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt))
        {
            if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt))
            {
                CRecompilerOps::UnknownOpcode();
            }
            else if (GetMipsRegLo(m_Opcode.rs) == GetMipsRegLo(m_Opcode.rt))
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt))
        {
            if ((Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)) && !g_System->b32BitCore())
            {
                ProtectGPR(m_Opcode.rs);
                ProtectGPR(m_Opcode.rt);

                CompX86RegToX86Reg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(x86_Any, m_Opcode.rs, true) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, true) : GetMipsRegMapHi(m_Opcode.rt)
                    );
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel8("continue", 0);
                    Jump = m_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, m_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
            }
            else
            {
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(ConstReg) || Is64Bit(MappedReg))
            {
                if (Is32Bit(ConstReg) || Is32Bit(MappedReg))
                {
                    if (Is32Bit(MappedReg))
                    {
                        ProtectGPR(MappedReg);
                        CompConstToX86reg(Map_TempReg(x86_Any, MappedReg, true), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Cont.FallThrough) {
                    JneLabel8("continue", 0);
                    Jump = m_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, m_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
            }
            else
            {
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rs) || IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!g_System->b32BitCore())
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    CompConstToVariable(GetMipsRegLo_S(KnownReg) >> 31, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                ProtectGPR(KnownReg);
                if (Is64Bit(KnownReg))
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            if (m_Section->m_Cont.FallThrough)
            {
                JneLabel8("continue", 0);
                Jump = m_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        if (IsConst(KnownReg))
        {
            CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        else
        {
            CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, m_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else
        {
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }
    else
    {
        x86Reg Reg = x86_Any;
        if (!g_System->b32BitCore())
        {
            Reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
            CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
            if (m_Section->m_Cont.FallThrough)
            {
                JneLabel8("continue", 0);
                Jump = m_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (m_Section->m_Cont.FallThrough)
        {
            JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, m_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else
        {
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }
}

void CRecompilerOps::BGTZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            if (GetMipsReg_S(m_Opcode.rs) > 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            if (GetMipsRegLo_S(m_Opcode.rs) > 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
    }
    else if (IsMapped(m_Opcode.rs) && Is32Bit(m_Opcode.rs))
    {
        CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
        if (m_Section->m_Jump.FallThrough)
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }
    else if (IsUnknown(m_Opcode.rs) && g_System->b32BitCore())
    {
        CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        if (m_Section->m_Jump.FallThrough)
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }
    else
    {
        uint8_t *Jump = NULL;

        if (IsMapped(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JgLabel8("continue", 0);
            Jump = m_RecompPos - 1;
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JlLabel8("continue", 0);
            Jump = m_RecompPos - 1;
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }

        if (IsMapped(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            CPU_Message("      continue:");
            SetJump8(Jump, m_RecompPos);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            CPU_Message("      continue:");
            SetJump8(Jump, m_RecompPos);
        }
        else
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
        }
    }
}

void CRecompilerOps::BLEZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            if (GetMipsReg_S(m_Opcode.rs) <= 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            if (GetMipsRegLo_S(m_Opcode.rs) <= 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            if (GetMipsRegLo(m_Opcode.rs) == 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is32Bit(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JleLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else
        {
            uint8_t *Jump = NULL;

            if (IsMapped(m_Opcode.rs))
            {
                CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            }
            else
            {
                CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JlLabel8("Continue", 0);
                Jump = m_RecompPos - 1;
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JgLabel8("Continue", 0);
                Jump = m_RecompPos - 1;
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }

            if (IsMapped(m_Opcode.rs))
            {
                CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            }
            else
            {
                CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                CPU_Message("      continue:");
                SetJump8(Jump, m_RecompPos);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                CPU_Message("      continue:");
                SetJump8(Jump, m_RecompPos);
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32("BranchToJump", 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
    }
    else {
        uint8_t *Jump = NULL;

        if (!g_System->b32BitCore())
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JlLabel8("Continue", 0);
                Jump = m_RecompPos - 1;
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JgLabel8("Continue", 0);
                Jump = m_RecompPos - 1;
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                if (g_System->b32BitCore())
                {
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                }
                if (Jump)
                {
                    CPU_Message("      continue:");
                    SetJump8(Jump, m_RecompPos);
                }
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                if (Jump)
                {
                    CPU_Message("      continue:");
                    SetJump8(Jump, m_RecompPos);
                }
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32("BranchToJump", 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JleLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
    }
}

void CRecompilerOps::BLTZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            if (GetMipsReg_S(m_Opcode.rs) < 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            if (GetMipsRegLo_S(m_Opcode.rs) < 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
        }
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
        }
    }
    else if (IsUnknown(m_Opcode.rs))
    {
        if (g_System->b32BitCore())
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else
        {
            JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }
}

void CRecompilerOps::BGEZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            CRecompilerOps::UnknownOpcode();
        }
        else if (IsSigned(m_Opcode.rs))
        {
            if (GetMipsRegLo_S(m_Opcode.rs) >= 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = true;
            m_Section->m_Cont.FallThrough = false;
        }
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
            else
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = true;
            m_Section->m_Cont.FallThrough = false;
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
        else
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }
}

void CRecompilerOps::COP1_BCF_Compare()
{
    TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
    }
    else
    {
        JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
    }
}

void CRecompilerOps::COP1_BCT_Compare()
{
    TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
    }
    else
    {
        JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(m_RecompPos - 4);
        JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(m_RecompPos - 4);
    }
}

/*************************  OpCode functions *************************/
void CRecompilerOps::J()
{
    if (m_NextInstruction == NORMAL)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            MoveConstToVariable((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
            OverflowDelaySlot(false);
            return;
        }

        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);;
        m_Section->m_Jump.JumpPC = m_CompilePC;
        if (m_Section->m_JumpSection != NULL)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }
        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = NULL;
        m_Section->m_Jump.LinkLocation2 = NULL;
        m_NextInstruction = DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->GenerateSectionLinkage();
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nJ\nNextInstruction = %X", m_NextInstruction).c_str());
    }
}

void CRecompilerOps::JAL()
{
    if (m_NextInstruction == NORMAL)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
        Map_GPR_32bit(31, true, -1);
        MoveVariableToX86reg(_PROGRAM_COUNTER, "_PROGRAM_COUNTER", GetMipsRegMapLo(31));
        AndConstToX86Reg(GetMipsRegMapLo(31), 0xF0000000);
        AddConstToX86Reg(GetMipsRegMapLo(31), (m_CompilePC + 8) & ~0xF0000000);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            MoveConstToVariable((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
            OverflowDelaySlot(false);
            return;
        }
        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
        m_Section->m_Jump.JumpPC = m_CompilePC;
        if (m_Section->m_JumpSection != NULL)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }
        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = NULL;
        m_Section->m_Jump.LinkLocation2 = NULL;
        m_NextInstruction = DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        if (m_Section->m_JumpSection)
        {
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->GenerateSectionLinkage();
        }
        else
        {
            m_RegWorkingSet.WriteBackRegisters();

            x86Reg pc_reg = Map_TempReg(x86_Any, -1, false);
            MoveVariableToX86reg(_PROGRAM_COUNTER, "_PROGRAM_COUNTER", pc_reg);
            AndConstToX86Reg(pc_reg, 0xF0000000);
            AddConstToX86Reg(pc_reg, (m_Opcode.target << 2));
            MoveX86regToVariable(pc_reg, _PROGRAM_COUNTER, "_PROGRAM_COUNTER");

            uint32_t TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
            bool bCheck = TargetPC <= m_CompilePC;
            UpdateCounters(m_RegWorkingSet, bCheck, true);

            m_Section->CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, bCheck ? CExitInfo::Normal : CExitInfo::Normal_NoSysCheck, true, NULL);
        }
        m_NextInstruction = END_BLOCK;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return;
}

void CRecompilerOps::ADDI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rt == 0) { return; }

    if (g_System->bFastSP() && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        AddConstToX86Reg(Map_MemoryStack(x86_Any, true), (int16_t)m_Opcode.immediate);
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) + (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        ResetX86Protection();
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::ADDIU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rt == 0 || (m_Opcode.immediate == 0 && m_Opcode.rs == m_Opcode.rt))
    {
        return;
    }

    if (g_System->bFastSP())
    {
        if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
        {
            AddConstToX86Reg(Map_MemoryStack(x86_Any, true), (int16_t)m_Opcode.immediate);
        }
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) + (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        ResetX86Protection();
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::SLTIU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Result = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) < ((unsigned)((int64_t)((int16_t)m_Opcode.immediate))) ? 1 : 0 :
            GetMipsRegLo(m_Opcode.rs) < ((unsigned)((int16_t)m_Opcode.immediate)) ? 1 : 0;
        UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            uint8_t * Jump[2];

            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            JeLabel8("Low Compare", 0);
            Jump[0] = m_RecompPos - 1;
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            JmpLabel8("Continue", 0);
            Jump[1] = m_RecompPos - 1;
            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], m_RecompPos);
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], m_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
    }
    else if (g_System->b32BitCore())
    {
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        uint8_t * Jump = NULL;

        CompConstToVariable(((int16_t)m_Opcode.immediate >> 31), &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        JneLabel8("CompareSet", 0);
        Jump = m_RecompPos - 1;
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        CPU_Message("");
        CPU_Message("      CompareSet:");
        SetJump8(Jump, m_RecompPos);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
    }
}

void CRecompilerOps::SLTI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Result = Is64Bit(m_Opcode.rs) ?
            ((int64_t)GetMipsReg(m_Opcode.rs) < (int64_t)((int16_t)m_Opcode.immediate) ? 1 : 0) :
            (GetMipsRegLo_S(m_Opcode.rs) < (int16_t)m_Opcode.immediate ? 1 : 0);

        UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            uint8_t * Jump[2];

            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            JeLabel8("Low Compare", 0);
            Jump[0] = m_RecompPos - 1;
            SetlVariable(&m_BranchCompare, "m_BranchCompare");
            JmpLabel8("Continue", 0);
            Jump[1] = m_RecompPos - 1;
            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], m_RecompPos);
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], m_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            /*	CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs),(int16_t)m_Opcode.immediate);
            SetlVariable(&m_BranchCompare,"m_BranchCompare");
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",GetMipsRegMapLo(m_Opcode.rt));
            */
            ProtectGPR(m_Opcode.rs);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);

            if (GetMipsRegMapLo(m_Opcode.rt) > x86_EBX)
            {
                SetlVariable(&m_BranchCompare, "m_BranchCompare");
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
            }
            else
            {
                Setl(GetMipsRegMapLo(m_Opcode.rt));
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 1);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);

        if (GetMipsRegMapLo(m_Opcode.rt) > x86_EBX)
        {
            SetlVariable(&m_BranchCompare, "m_BranchCompare");
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            Setl(GetMipsRegMapLo(m_Opcode.rt));
            AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 1);
        }
    }
    else
    {
        uint8_t * Jump[2] = { NULL, NULL };
        CompConstToVariable(((int16_t)m_Opcode.immediate >> 31), &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        JeLabel8("Low Compare", 0);
        Jump[0] = m_RecompPos - 1;
        SetlVariable(&m_BranchCompare, "m_BranchCompare");
        JmpLabel8("Continue", 0);
        Jump[1] = m_RecompPos - 1;
        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(Jump[0], m_RecompPos);
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], m_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
    }
}

void CRecompilerOps::ANDI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) & m_Opcode.immediate);
    }
    else if (m_Opcode.immediate != 0)
    {
        Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.rs);
        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rt, false, 0);
    }
}

void CRecompilerOps::ORI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_System->bFastSP() && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        OrConstToX86Reg(m_Opcode.immediate, Map_MemoryStack(x86_Any, true));
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) | m_Opcode.immediate);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            if (Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rt, IsSigned(m_Opcode.rs), m_Opcode.rs);
            }
        }
        OrConstToX86Reg(m_Opcode.immediate, GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        }
        OrConstToX86Reg(m_Opcode.immediate, GetMipsRegMapLo(m_Opcode.rt));
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        ResetX86Protection();
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::XORI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        if (m_Opcode.rs != m_Opcode.rt)
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) ^ m_Opcode.immediate);
    }
    else
    {
        if (IsMapped(m_Opcode.rs) && Is32Bit(m_Opcode.rs))
        {
            Map_GPR_32bit(m_Opcode.rt, IsSigned(m_Opcode.rs), m_Opcode.rs);
        }
        else if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        }
        if (m_Opcode.immediate != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate); }
    }
}

void CRecompilerOps::LUI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        x86Reg Reg = Map_MemoryStack(x86_Any, true, false);
        uint32_t Address;

        g_TransVaddr->TranslateVaddr(((int16_t)m_Opcode.offset << 16), Address);
        if (Reg < 0)
        {
            MoveConstToVariable((uint32_t)(Address + g_MMU->Rdram()), &(g_Recompiler->MemoryStackPos()), "MemoryStack");
        }
        else
        {
            MoveConstToX86reg((uint32_t)(Address + g_MMU->Rdram()), Reg);
        }
    }

    UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, ((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
}

void CRecompilerOps::DADDIU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rs != 0)
    {
        UnMap_GPR(m_Opcode.rs, true);
    }

    if (m_Opcode.rs != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::DADDIU, "R4300iOp::DADDIU");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::CACHE()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (g_Settings->LoadDword(Game_SMM_Cache) == 0)
    {
        return;
    }

    switch (m_Opcode.rt)
    {
    case 0:
    case 16:
        BeforeCallDirect(m_RegWorkingSet);
        PushImm32("CRecompiler::Remove_Cache", CRecompiler::Remove_Cache);
        PushImm32("0x20", 0x20);
        if (IsConst(m_Opcode.base))
        {
            uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            PushImm32("Address", Address);
        }
        else if (IsMapped(m_Opcode.base))
        {
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            Push(GetMipsRegMapLo(m_Opcode.base));
        }
        else
        {
            MoveVariableToX86reg(&_GPR[m_Opcode.base].UW[0], CRegName::GPR_Lo[m_Opcode.base], x86_EAX);
            AddConstToX86Reg(x86_EAX, (int16_t)m_Opcode.offset);
            Push(x86_EAX);
        }
        MoveConstToX86reg((uint32_t)g_Recompiler, x86_ECX);
        Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
        AfterCallDirect(m_RegWorkingSet);
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
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("cache: %d", m_Opcode.rt).c_str());
        }
    }
}

/********************** R4300i OpCodes: Special **********************/
void CRecompilerOps::SPECIAL_SLL()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rd == 0)
    {
        return;
    }
    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    if (m_Opcode.rd != m_Opcode.rt && IsMapped(m_Opcode.rt))
    {
        switch (m_Opcode.sa)
        {
        case 0:
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            break;
        case 1:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, Multip_x2);
            break;
        case 2:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, Multip_x4);
            break;
        case 3:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, Multip_x8);
            break;
        default:
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    }
}

void CRecompilerOps::SPECIAL_SRL()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_SRA()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_SLLV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) << Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        }
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftLeftSign(GetMipsRegMapLo(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_SRLV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) >> Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }

    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_SRAV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo_S(m_Opcode.rt) >> Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftRightSign(GetMipsRegMapLo(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_JR()
{
    if (m_NextInstruction == NORMAL)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
            }
            OverflowDelaySlot(true);
            return;
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = NULL;
        m_Section->m_Jump.LinkLocation2 = NULL;
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = NULL;
        m_Section->m_Cont.LinkLocation2 = NULL;

        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
        }
        m_NextInstruction = DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            m_Section->CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            m_Section->CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
            if (m_Section->m_JumpSection)
            {
                m_Section->GenerateSectionLinkage();
            }
        }
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction).c_str());
    }
}

void CRecompilerOps::SPECIAL_JALR()
{
    if (m_NextInstruction == NORMAL)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0) && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
        }
        UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_CompilePC + 8);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
            }
            OverflowDelaySlot(true);
            return;
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = NULL;
        m_Section->m_Jump.LinkLocation2 = NULL;
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = NULL;
        m_Section->m_Cont.LinkLocation2 = NULL;

        m_NextInstruction = DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            m_Section->CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            m_Section->CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
            if (m_Section->m_JumpSection)
            {
                m_Section->GenerateSectionLinkage();
            }
        }
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction).c_str());
    }
}

void CRecompilerOps::SPECIAL_SYSCALL()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::DoSysCall, true, NULL);
    m_NextInstruction = END_BLOCK;
}

void CRecompilerOps::SPECIAL_MFLO()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0) { return; }

    Map_GPR_64bit(m_Opcode.rd, -1);
    MoveVariableToX86reg(&_RegLO->UW[0], "_RegLO->UW[0]", GetMipsRegMapLo(m_Opcode.rd));
    MoveVariableToX86reg(&_RegLO->UW[1], "_RegLO->UW[1]", GetMipsRegMapHi(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_MTLO()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveConstToVariable(GetMipsRegHi(m_Opcode.rs), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs) && ((GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            MoveConstToVariable(0xFFFFFFFF, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
    else if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.rs), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs))
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, true), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
    else
    {
        x86Reg reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        MoveX86regToVariable(reg, &_RegLO->UW[1], "_RegLO->UW[1]");
        MoveX86regToVariable(Map_TempReg(reg, m_Opcode.rs, false), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
}

void CRecompilerOps::SPECIAL_MFHI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0) { return; }

    Map_GPR_64bit(m_Opcode.rd, -1);
    MoveVariableToX86reg(&_RegHI->UW[0], "_RegHI->UW[0]", GetMipsRegMapLo(m_Opcode.rd));
    MoveVariableToX86reg(&_RegHI->UW[1], "_RegHI->UW[1]", GetMipsRegMapHi(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_MTHI()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveConstToVariable(GetMipsRegHi(m_Opcode.rs), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs) && ((GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            MoveConstToVariable(0xFFFFFFFF, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
    else if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.rs), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs))
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, true), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
    else
    {
        x86Reg reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        MoveX86regToVariable(reg, &_RegHI->UW[1], "_RegHI->UW[1]");
        MoveX86regToVariable(Map_TempReg(reg, m_Opcode.rs, false), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
}

void CRecompilerOps::SPECIAL_DSLLV()
{
    uint8_t * Jump[2];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        //uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x3F);
        CRecompilerOps::UnknownOpcode();
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x3F);
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    CompConstToX86reg(x86_ECX, 0x20);
    JaeLabel8("MORE32", 0);
    Jump[0] = m_RecompPos - 1;
    ShiftLeftDouble(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    ShiftLeftSign(GetMipsRegMapLo(m_Opcode.rd));
    JmpLabel8("continue", 0);
    Jump[1] = m_RecompPos - 1;

    //MORE32:
    CPU_Message("");
    CPU_Message("      MORE32:");
    SetJump8(Jump[0], m_RecompPos);
    MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
    XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    AndConstToX86Reg(x86_ECX, 0x1F);
    ShiftLeftSign(GetMipsRegMapHi(m_Opcode.rd));

    //continue:
    CPU_Message("");
    CPU_Message("      continue:");
    SetJump8(Jump[1], m_RecompPos);
}

void CRecompilerOps::SPECIAL_DSRLV()
{
    uint8_t * Jump[2];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x3F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt));
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, GetMipsReg(m_Opcode.rd) >> Shift);
            if ((GetMipsRegHi(m_Opcode.rd) == 0) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) == 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if ((GetMipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) != 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
            }
            return;
        }
        if (m_Opcode.rd == m_Opcode.rt)
        {
            CRecompilerOps::UnknownOpcode();
            return;
        }

        Map_TempReg(x86_ECX, -1, false);
        MoveConstToX86reg(Shift, x86_ECX);
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        if ((Shift & 0x20) == 0x20)
        {
            MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
            XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            AndConstToX86Reg(x86_ECX, 0x1F);
            ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            ShiftRightUnsign(GetMipsRegMapHi(m_Opcode.rd));
        }
    }
    else
    {
        Map_TempReg(x86_ECX, m_Opcode.rs, false);
        AndConstToX86Reg(x86_ECX, 0x3F);
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        CompConstToX86reg(x86_ECX, 0x20);
        JaeLabel8("MORE32", 0);
        Jump[0] = m_RecompPos - 1;
        ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        ShiftRightUnsign(GetMipsRegMapHi(m_Opcode.rd));
        JmpLabel8("continue", 0);
        Jump[1] = m_RecompPos - 1;

        //MORE32:
        CPU_Message("");
        CPU_Message("      MORE32:");
        SetJump8(Jump[0], m_RecompPos);
        MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
        XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        AndConstToX86Reg(x86_ECX, 0x1F);
        ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));

        //continue:
        CPU_Message("");
        CPU_Message("      continue:");
        SetJump8(Jump[1], m_RecompPos);
    }
}

void CRecompilerOps::SPECIAL_DSRAV()
{
    uint8_t * Jump[2];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        //uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x3F);
        CRecompilerOps::UnknownOpcode();
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x3F);
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    CompConstToX86reg(x86_ECX, 0x20);
    JaeLabel8("MORE32", 0);
    Jump[0] = m_RecompPos - 1;
    ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
    ShiftRightSign(GetMipsRegMapHi(m_Opcode.rd));
    JmpLabel8("continue", 0);
    Jump[1] = m_RecompPos - 1;

    //MORE32:
    CPU_Message("");
    CPU_Message("      MORE32:");
    SetJump8(Jump[0], m_RecompPos);
    MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    ShiftRightSignImmed(GetMipsRegMapHi(m_Opcode.rd), 0x1F);
    AndConstToX86Reg(x86_ECX, 0x1F);
    ShiftRightSign(GetMipsRegMapLo(m_Opcode.rd));

    //continue:
    CPU_Message("");
    CPU_Message("      continue:");
    SetJump8(Jump[1], m_RecompPos);
}

void CRecompilerOps::SPECIAL_MULT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_RegWorkingSet.SetX86Protected(x86_EDX, true);
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    m_RegWorkingSet.SetX86Protected(x86_EDX, false);
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    imulX86reg(x86_EDX);

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    ShiftRightSignImmed(x86_EAX, 31);	/* paired */
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CRecompilerOps::SPECIAL_MULTU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_RegWorkingSet.SetX86Protected(x86_EDX, true);
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    m_RegWorkingSet.SetX86Protected(x86_EDX, false);
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    MulX86reg(x86_EDX);

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    ShiftRightSignImmed(x86_EAX, 31);	/* paired */
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CRecompilerOps::SPECIAL_DIV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsConst(m_Opcode.rt))
    {
        if (GetMipsRegLo(m_Opcode.rt) == 0)
        {
            MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
            MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
            return;
        }
    }
    else
    {
        if (IsMapped(m_Opcode.rt))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rt), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_Section->CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::DivByZero, false, JeLabel32);
    }
    /*	lo = (SD)rs / (SD)rt;
    hi = (SD)rs % (SD)rt; */

    m_RegWorkingSet.SetX86Protected(x86_EDX, true);
    Map_TempReg(x86_EAX, m_Opcode.rs, false);

    /* edx is the signed portion to eax */
    m_RegWorkingSet.SetX86Protected(x86_EDX, false);
    Map_TempReg(x86_EDX, -1, false);

    MoveX86RegToX86Reg(x86_EAX, x86_EDX);
    ShiftRightSignImmed(x86_EDX, 31);

    if (IsMapped(m_Opcode.rt))
    {
        idivX86reg(GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        idivX86reg(Map_TempReg(x86_Any, m_Opcode.rt, false));
    }

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    ShiftRightSignImmed(x86_EAX, 31);	/* paired */
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CRecompilerOps::SPECIAL_DIVU()
{
    uint8_t *Jump[2];
    x86Reg Reg;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsConst(m_Opcode.rt))
    {
        if (GetMipsRegLo(m_Opcode.rt) == 0)
        {
            MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
            MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
            return;
        }
        Jump[1] = NULL;
    }
    else
    {
        if (IsMapped(m_Opcode.rt))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rt), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        JneLabel8("NoExcept", 0);
        Jump[0] = m_RecompPos - 1;

        MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
        MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
        MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

        JmpLabel8("EndDivu", 0);
        Jump[1] = m_RecompPos - 1;

        CPU_Message("");
        CPU_Message("      NoExcept:");
        SetJump8(Jump[0], m_RecompPos);
    }

    /*	lo = (UD)rs / (UD)rt;
    hi = (UD)rs % (UD)rt; */

    m_RegWorkingSet.SetX86Protected(x86_EAX, true);
    Map_TempReg(x86_EDX, 0, false);
    m_RegWorkingSet.SetX86Protected(x86_EAX, false);

    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);

    DivX86reg(Reg);

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");

    /* wouldnt these be zero (???) */

    ShiftRightSignImmed(x86_EAX, 31);	/* paired */
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

    if (Jump[1] != NULL)
    {
        CPU_Message("");
        CPU_Message("      EndDivu:");
        SetJump8(Jump[1], m_RecompPos);
    }
}

void CRecompilerOps::SPECIAL_DMULT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rs != 0)
    {
        UnMap_GPR(m_Opcode.rs, true);
    }

    if (m_Opcode.rs != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::SPECIAL_DMULTU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DMULTU, "R4300iOp::SPECIAL_DMULTU");
    AfterCallDirect(m_RegWorkingSet);

#ifdef toremove
    /* _RegLO->UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
    X86Protected(x86_EDX) = true;
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    X86Protected(x86_EDX) = false;
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    MulX86reg(x86_EDX);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

    /* _RegHI->UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
    Map_TempReg(x86_EAX, m_Opcode.rs, true);
    Map_TempReg(x86_EDX, m_Opcode.rt, true);

    MulX86reg(x86_EDX);
    MoveX86regToVariable(x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

    /* Tmp[0].UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
    Map_TempReg(x86_EAX, m_Opcode.rs, true);
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    Map_TempReg(x86_EBX, -1, false);
    Map_TempReg(x86_ECX, -1, false);

    MulX86reg(x86_EDX);
    MoveX86RegToX86Reg(x86_EAX, x86_EBX); /* EDX:EAX -> ECX:EBX */
    MoveX86RegToX86Reg(x86_EDX, x86_ECX);

    /* Tmp[1].UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    Map_TempReg(x86_EDX, m_Opcode.rt, true);

    MulX86reg(x86_EDX);
    Map_TempReg(x86_ESI, -1, false);
    Map_TempReg(x86_EDI, -1, false);
    MoveX86RegToX86Reg(x86_EAX, x86_ESI); /* EDX:EAX -> EDI:ESI */
    MoveX86RegToX86Reg(x86_EDX, x86_EDI);

    /* Tmp[2].UDW = (uint64)_RegLO->UW[1] + (uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]; */
    XorX86RegToX86Reg(x86_EDX, x86_EDX);
    MoveVariableToX86reg(&_RegLO->UW[1], "_RegLO->UW[1]", x86_EAX);
    AddX86RegToX86Reg(x86_EAX, x86_EBX);
    AddConstToX86Reg(x86_EDX, 0);
    AddX86RegToX86Reg(x86_EAX, x86_ESI);
    AddConstToX86Reg(x86_EDX, 0);			/* EDX:EAX */

    /* _RegLO->UDW += ((uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]) << 32; */
    /* [low+4] += ebx + esi */

    AddX86regToVariable(x86_EBX, &_RegLO->UW[1], "_RegLO->UW[1]");
    AddX86regToVariable(x86_ESI, &_RegLO->UW[1], "_RegLO->UW[1]");

    /* _RegHI->UDW += (uint64)Tmp[0].UW[1] + (uint64)Tmp[1].UW[1] + Tmp[2].UW[1]; */
    /* [hi] += ecx + edi + edx */

    AddX86regToVariable(x86_ECX, &_RegHI->UW[0], "_RegHI->UW[0]");
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

    AddX86regToVariable(x86_EDI, &_RegHI->UW[0], "_RegHI->UW[0]");
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

    AddX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
#endif
}

void CRecompilerOps::SPECIAL_DDIV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::SPECIAL_DDIVU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::SPECIAL_ADD()
{
    int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(source1) && IsConst(source2))
    {
        uint32_t temp = GetMipsRegLo(source1) + GetMipsRegLo(source2);
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }

    Map_GPR_32bit(m_Opcode.rd, true, source1);
    if (IsConst(source2))
    {
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
    }
    else if (IsKnown(source2) && IsMapped(source2))
    {
        AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
    }
    else
    {
        AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::SPECIAL_ADDU()
{
    int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(source1) && IsConst(source2))
    {
        uint32_t temp = GetMipsRegLo(source1) + GetMipsRegLo(source2);
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }

    Map_GPR_32bit(m_Opcode.rd, true, source1);
    if (IsConst(source2))
    {
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
    }
    else if (IsKnown(source2) && IsMapped(source2))
    {
        AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
    }
    else
    {
        AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::SPECIAL_SUB()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        uint32_t temp = GetMipsRegLo(m_Opcode.rs) - GetMipsRegLo(m_Opcode.rt);

        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::SPECIAL_SUBU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        uint32_t temp = GetMipsRegLo(m_Opcode.rs) - GetMipsRegLo(m_Opcode.rt);

        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
    }

    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::SPECIAL_AND()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                    (Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)) &
                    (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs))
                    );

                if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
                }
            }
            else
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) & GetMipsReg(m_Opcode.rs));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(source1);
            ProtectGPR(source2);
            if (Is32Bit(source1) && Is32Bit(source2))
            {
                bool Sign = (IsSigned(m_Opcode.rt) && IsSigned(m_Opcode.rs));
                Map_GPR_32bit(m_Opcode.rd, Sign, source1);
                AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
            else if (Is32Bit(source1) || Is32Bit(source2))
            {
                if (IsUnsigned(Is32Bit(source1) ? source1 : source2))
                {
                    Map_GPR_32bit(m_Opcode.rd, false, source1);
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
                else
                {
                    Map_GPR_64bit(m_Opcode.rd, source1);
                    if (Is32Bit(source2))
                    {
                        AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                    }
                    else
                    {
                        AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                    }
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
        }
        else
        {
            int ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            int MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(ConstReg))
            {
                if (Is32Bit(MappedReg) && IsUnsigned(MappedReg))
                {
                    if (GetMipsRegLo(ConstReg) == 0)
                    {
                        Map_GPR_32bit(m_Opcode.rd, false, 0);
                    }
                    else
                    {
                        uint32_t Value = GetMipsRegLo(ConstReg);
                        Map_GPR_32bit(m_Opcode.rd, false, MappedReg);
                        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                    }
                }
                else
                {
                    int64_t Value = GetMipsReg(ConstReg);
                    Map_GPR_64bit(m_Opcode.rd, MappedReg);
                    AndConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
                }
            }
            else if (Is64Bit(MappedReg))
            {
                uint32_t Value = GetMipsRegLo(ConstReg);
                if (Value != 0)
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(ConstReg), MappedReg);
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(ConstReg), 0);
                }
            }
            else
            {
                uint32_t Value = GetMipsRegLo(ConstReg);
                bool Sign = false;

                if (IsSigned(ConstReg) && IsSigned(MappedReg))
                {
                    Sign = true;
                }

                if (Value != 0)
                {
                    Map_GPR_32bit(m_Opcode.rd, Sign, MappedReg);
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, false, 0);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            if (Is64Bit(KnownReg))
            {
                uint64_t Value = GetMipsReg(KnownReg);
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                AndConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
            }
            else
            {
                uint32_t Value = GetMipsRegLo(KnownReg);
                Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), UnknownReg);
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
            }
        }
        else
        {
            ProtectGPR(KnownReg);
            if (KnownReg == m_Opcode.rd)
            {
                if (Is64Bit(KnownReg) || !g_System->b32BitCore())
                {
                    Map_GPR_64bit(m_Opcode.rd, KnownReg);
                    AndVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                    AndVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), KnownReg);
                    AndVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                    AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(KnownReg));
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), UnknownReg);
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
                }
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
        }
        AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CRecompilerOps::SPECIAL_OR()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                    (Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)) |
                    (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs))
                    );
                if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
                }
            }
            else
            {
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) | GetMipsRegLo(m_Opcode.rs));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                if (Is64Bit(source2))
                {
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else
                {
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                }
            }
            else
            {
                ProtectGPR(source2);
                Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            OrX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint64_t Value;

                if (Is64Bit(ConstReg))
                {
                    Value = GetMipsReg(ConstReg);
                }
                else
                {
                    Value = IsSigned(ConstReg) ? (int64_t)GetMipsRegLo_S(ConstReg) : GetMipsRegLo(ConstReg);
                }
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if ((Value >> 32) != 0)
                {
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0) { OrConstToX86Reg(Value, GetMipsRegMapLo(m_Opcode.rd)); }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        int KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            uint64_t Value = Is64Bit(KnownReg) ? GetMipsReg(KnownReg) : GetMipsRegLo_S(KnownReg);
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);

            if (g_System->b32BitCore() && Is32Bit(KnownReg))
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetX86Protection();
        g_MMU->ResetMemoryStack();
    }
}

void CRecompilerOps::SPECIAL_XOR()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
        return;

    if (m_Opcode.rt == m_Opcode.rs)
    {
        UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
        return;
    }
    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                if (bHaveDebugger()) { g_Notify->DisplayError("XOR 1"); }
                CRecompilerOps::UnknownOpcode();
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) ^ GetMipsRegLo(m_Opcode.rs));
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(source1);
            ProtectGPR(source2);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                if (Is64Bit(source2))
                {
                    XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else if (IsSigned(source2))
                {
                    XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                }
                XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
            else
            {
                if (IsSigned(m_Opcode.rt) != IsSigned(m_Opcode.rs))
                {
                    Map_GPR_32bit(m_Opcode.rd, true, source1);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(m_Opcode.rt), source1);
                }
                XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo;

                ConstHi = Is32Bit(ConstReg) ? (uint32_t)(GetMipsRegLo_S(ConstReg) >> 31) : GetMipsRegHi(ConstReg);
                ConstLo = GetMipsRegLo(ConstReg);
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if (ConstHi != 0) { XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), ConstHi); }
                if (ConstLo != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), ConstLo); }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                if (IsSigned(m_Opcode.rt) != IsSigned(m_Opcode.rs))
                {
                    Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(MappedReg), MappedReg);
                }
                if (Value != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value); }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        int KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            uint64_t Value;

            if (Is64Bit(KnownReg))
            {
                Value = GetMipsReg(KnownReg);
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (IsSigned(KnownReg))
                {
                    Value = (int)GetMipsRegLo(KnownReg);
                }
                else
                {
                    Value = GetMipsRegLo(KnownReg);
                }
            }
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
            if (dwValue != 0)
            {
                XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), dwValue);
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                XorVariableToX86reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                XorVariableToX86reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                XorVariableToX86reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        XorVariableToX86reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
        XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CRecompilerOps::SPECIAL_NOR()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (IsMapped(m_Opcode.rd))
                UnMap_GPR(m_Opcode.rd, false);

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                    ~((Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)) |
                    (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs)))
                    );
                if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
                }
            }
            else
            {
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, ~(GetMipsRegLo(m_Opcode.rt) | GetMipsRegLo(m_Opcode.rs)));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                if (Is64Bit(source2))
                {
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else
                {
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                }
            }
            else
            {
                ProtectGPR(source2);
                Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            OrX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint64_t Value;

                if (Is64Bit(ConstReg))
                {
                    Value = GetMipsReg(ConstReg);
                }
                else
                {
                    Value = IsSigned(ConstReg) ? (int64_t)GetMipsRegLo_S(ConstReg) : GetMipsRegLo(ConstReg);
                }
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if ((Value >> 32) != 0)
                {
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0) {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0) { OrConstToX86Reg(Value, GetMipsRegMapLo(m_Opcode.rd)); }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        int KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            uint64_t Value = Is64Bit(KnownReg) ? GetMipsReg(KnownReg) : GetMipsRegLo_S(KnownReg);
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);

            if (g_System->b32BitCore() && Is32Bit(KnownReg))
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
    }

    if (IsMapped(m_Opcode.rd))
    {
        if (Is64Bit(m_Opcode.rd))
        {
            NotX86Reg(GetMipsRegMapHi(m_Opcode.rd));
        }
        NotX86Reg(GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CRecompilerOps::SPECIAL_SLT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                g_Notify->DisplayError("1");
                CRecompilerOps::UnknownOpcode();
            }
            else
            {
                if (IsMapped(m_Opcode.rd))
                {
                    UnMap_GPR(m_Opcode.rd, false);
                }

                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                if (GetMipsRegLo_S(m_Opcode.rs) < GetMipsRegLo_S(m_Opcode.rt))
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 1);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
                }
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if ((Is64Bit(m_Opcode.rt) && Is64Bit(m_Opcode.rs)) ||
                (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))))
            {
                uint8_t *Jump[2];

                CompX86RegToX86Reg(
                    Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(x86_Any, m_Opcode.rs, true),
                    Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true)
                    );
                JeLabel8("Low Compare", 0);
                Jump[0] = m_RecompPos - 1;
                SetlVariable(&m_BranchCompare, "m_BranchCompare");
                JmpLabel8("Continue", 0);
                Jump[1] = m_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], m_RecompPos);
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], m_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));

                if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
                {
                    SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    Setl(GetMipsRegMapLo(m_Opcode.rd));
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rs) ? m_Opcode.rs : m_Opcode.rt;
            uint32_t MappedReg = IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;

            ProtectGPR(MappedReg);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint8_t *Jump[2];

                CompConstToX86reg(
                    Is64Bit(MappedReg) ? GetMipsRegMapHi(MappedReg) : Map_TempReg(x86_Any, MappedReg, true),
                    Is64Bit(ConstReg) ? GetMipsRegHi(ConstReg) : (GetMipsRegLo_S(ConstReg) >> 31)
                    );
                JeLabel8("Low Compare", 0);
                Jump[0] = m_RecompPos - 1;
                if (MappedReg == m_Opcode.rs)
                {
                    SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                JmpLabel8("Continue", 0);
                Jump[1] = m_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], m_RecompPos);
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], m_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                uint32_t Constant = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), Constant);

                if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    else
                    {
                        SetgVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        Setl(GetMipsRegMapLo(m_Opcode.rd));
                    }
                    else
                    {
                        Setg(GetMipsRegMapLo(m_Opcode.rd));
                    }
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        uint8_t *Jump[2];

        if (!g_System->b32BitCore())
        {
            if (Is64Bit(KnownReg))
            {
                if (IsConst(KnownReg))
                {
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (IsConst(KnownReg))
                {
                    CompConstToVariable((GetMipsRegLo_S(KnownReg) >> 31), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    ProtectGPR(KnownReg);
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            JeLabel8("Low Compare", 0);
            Jump[0] = m_RecompPos - 1;
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                SetgVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetlVariable(&m_BranchCompare, "m_BranchCompare");
            }
            JmpLabel8("Continue", 0);
            Jump[1] = m_RecompPos - 1;

            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], m_RecompPos);
            if (IsConst(KnownReg))
            {
                CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt)) {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], m_RecompPos);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            if (IsMapped(KnownReg))
            {
                ProtectGPR(KnownReg);
            }
            bool bConstant = IsConst(KnownReg);
            uint32_t Value = IsConst(KnownReg) ? GetMipsRegLo(KnownReg) : 0;

            Map_GPR_32bit(m_Opcode.rd, true, -1);
            if (bConstant)
            {
                CompConstToVariable(Value, &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    Setg(GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    Setl(GetMipsRegMapLo(m_Opcode.rd));
                }
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, false);
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
        {
            SetlVariable(&m_BranchCompare, "m_BranchCompare");
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Setl(GetMipsRegMapLo(m_Opcode.rd));
            AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
        }
    }
    else
    {
        uint8_t *Jump[2] = { NULL, NULL };

        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        JeLabel8("Low Compare", 0);
        Jump[0] = m_RecompPos - 1;
        SetlVariable(&m_BranchCompare, "m_BranchCompare");
        JmpLabel8("Continue", 0);
        Jump[1] = m_RecompPos - 1;

        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(Jump[0], m_RecompPos);
        CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], m_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CRecompilerOps::SPECIAL_SLTU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                g_Notify->DisplayError("1");
                CRecompilerOps::UnknownOpcode();
            }
            else
            {
                if (IsMapped(m_Opcode.rd))
                {
                    UnMap_GPR(m_Opcode.rd, false);
                }

                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                if (GetMipsRegLo(m_Opcode.rs) < GetMipsRegLo(m_Opcode.rt))
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 1);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
                }
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if ((Is64Bit(m_Opcode.rt) && Is64Bit(m_Opcode.rs)) ||
                (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))))
            {
                uint8_t *Jump[2];

                CompX86RegToX86Reg(
                    Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(x86_Any, m_Opcode.rs, true),
                    Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true)
                    );
                JeLabel8("Low Compare", 0);
                Jump[0] = m_RecompPos - 1;
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                JmpLabel8("Continue", 0);
                Jump[1] = m_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], m_RecompPos);
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], m_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
        }
        else
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo, ConstReg, MappedReg;
                x86Reg MappedRegHi, MappedRegLo;
                uint8_t *Jump[2];

                ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
                MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                ConstLo = GetMipsRegLo_S(ConstReg);
                ConstHi = GetMipsRegLo_S(ConstReg) >> 31;
                if (Is64Bit(ConstReg)) { ConstHi = GetMipsRegHi(ConstReg); }

                ProtectGPR(MappedReg);
                MappedRegLo = GetMipsRegMapLo(MappedReg);
                MappedRegHi = GetMipsRegMapHi(MappedReg);
                if (Is32Bit(MappedReg))
                {
                    MappedRegHi = Map_TempReg(x86_Any, MappedReg, true);
                }

                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompConstToX86reg(MappedRegHi, ConstHi);
                JeLabel8("Low Compare", 0);
                Jump[0] = m_RecompPos - 1;
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                JmpLabel8("Continue", 0);
                Jump[1] = m_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], m_RecompPos);
                CompConstToX86reg(MappedRegLo, ConstLo);
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], m_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                uint32_t Const = IsConst(m_Opcode.rs) ? GetMipsRegLo(m_Opcode.rs) : GetMipsRegLo(m_Opcode.rt);
                uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                CompConstToX86reg(GetMipsRegMapLo(MappedReg), Const);
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        uint8_t *Jump[2] = { NULL, NULL };

        ProtectGPR(KnownReg);
        if (g_System->b32BitCore())
        {
            uint32_t TestReg = IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt;
            if (IsConst(KnownReg))
            {
                uint32_t Value = GetMipsRegLo(KnownReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompConstToVariable(Value, &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == TestReg)
            {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
        }
        else
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable((GetMipsRegLo_S(KnownReg) >> 31), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    ProtectGPR(KnownReg);
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            JeLabel8("Low Compare", 0);
            Jump[0] = m_RecompPos - 1;

            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            JmpLabel8("Continue", 0);
            Jump[1] = m_RecompPos - 1;

            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], m_RecompPos);
            if (IsConst(KnownReg))
            {
                CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            if (Jump[1])
            {
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], m_RecompPos);
            }
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
    else if (g_System->b32BitCore())
    {
        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, false);
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        uint8_t *Jump[2] = { NULL, NULL };

        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        JeLabel8("Low Compare", 0);
        Jump[0] = m_RecompPos - 1;
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        JmpLabel8("Continue", 0);
        Jump[1] = m_RecompPos - 1;

        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(Jump[0], m_RecompPos);
        CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], m_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CRecompilerOps::SPECIAL_DADD()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
            Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) +
            Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)
            );
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        if (IsMapped(source2)) { ProtectGPR(source2); }
        Map_GPR_64bit(m_Opcode.rd, source1);
        if (IsConst(source2))
        {
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
            AddConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
        }
        else if (IsMapped(source2))
        {
            x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(x86_Any, source2, true);
            AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            AdcX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            AdcVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
    }
}

void CRecompilerOps::SPECIAL_DADDU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
        return;

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        int64_t ValRs = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs);
        int64_t ValRt = Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        if (IsMapped(m_Opcode.rd))
            UnMap_GPR(m_Opcode.rd, false);

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, ValRs + ValRt);
        if ((GetMipsRegHi(m_Opcode.rd) == 0) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if ((GetMipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) != 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        if (IsMapped(source2)) { ProtectGPR(source2); }
        Map_GPR_64bit(m_Opcode.rd, source1);
        if (IsConst(source2))
        {
            uint32_t LoReg = GetMipsRegLo(source2);
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            if (LoReg != 0)
            {
                AdcConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
            }
            else
            {
                AddConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
            }
        }
        else if (IsMapped(source2))
        {
            x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(x86_Any, source2, true);
            AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            AdcX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            AdcVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
    }
}

void CRecompilerOps::SPECIAL_DSUB()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
            Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) -
            Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)
            );
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            x86Reg HiReg = Map_TempReg(x86_Any, m_Opcode.rt, true);
            x86Reg LoReg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
            return;
        }

        if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
            SbbConstFromX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            x86Reg HiReg = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
            SbbVariableFromX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        }
    }
}

void CRecompilerOps::SPECIAL_DSUBU()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
            Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) -
            Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)
            );
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            x86Reg HiReg = Map_TempReg(x86_Any, m_Opcode.rt, true);
            x86Reg LoReg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
            return;
        }
        if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
            SbbConstFromX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            x86Reg HiReg = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
            SbbVariableFromX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        }
    }
}

void CRecompilerOps::SPECIAL_DSLL()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        int64_t Value = Is64Bit(m_Opcode.rt) ? GetMipsReg_S(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value << m_Opcode.sa);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }

    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    ShiftLeftDoubleImmed(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_DSRL()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }
        int64_t Value = Is64Bit(m_Opcode.rt) ? GetMipsReg_S(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value >> m_Opcode.sa);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    ShiftRightDoubleImmed(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    ShiftRightUnsignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_DSRA()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        int64_t Value = Is64Bit(m_Opcode.rt) ? GetMipsReg_S(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg_S(m_Opcode.rd, Value >> m_Opcode.sa);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }

    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    ShiftRightDoubleImmed(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    ShiftRightSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_DSLL32()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
        Map_GPR_64bit(m_Opcode.rd, -1);
        if (m_Opcode.rt != m_Opcode.rd)
        {
            MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapHi(m_Opcode.rd));
        }
        else
        {
            CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s", x86_Name(GetMipsRegMapHi(m_Opcode.rt)), x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
            x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
            m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
            m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
        }
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftLeftSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, -1);
        MoveVariableToX86reg(&_GPR[m_Opcode.rt], CRegName::GPR_Hi[m_Opcode.rt], GetMipsRegMapHi(m_Opcode.rd));
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftLeftSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CRecompilerOps::SPECIAL_DSRL32()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, (uint32_t)(GetMipsRegHi(m_Opcode.rt) >> m_Opcode.sa));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
        if (Is64Bit(m_Opcode.rt))
        {
            if (m_Opcode.rt == m_Opcode.rd)
            {
                CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s", x86_Name(GetMipsRegMapHi(m_Opcode.rt)), x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                Map_GPR_32bit(m_Opcode.rd, false, -1);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
                MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rd));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CRecompilerOps::UnknownOpcode();
        }
    }
    else {
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt], GetMipsRegMapLo(m_Opcode.rd));
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

void CRecompilerOps::SPECIAL_DSRA32()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)(GetMipsReg_S(m_Opcode.rt) >> (m_Opcode.sa + 32)));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
        if (Is64Bit(m_Opcode.rt))
        {
            if (m_Opcode.rt == m_Opcode.rd)
            {
                CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s", x86_Name(GetMipsRegMapHi(m_Opcode.rt)), x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rd));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CRecompilerOps::UnknownOpcode();
        }
    }
    else {
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Lo[m_Opcode.rt], GetMipsRegMapLo(m_Opcode.rd));
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

/************************** COP0 functions **************************/
void CRecompilerOps::COP0_MF()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    switch (m_Opcode.rd)
    {
    case 9: //Count
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AfterCallDirect(m_RegWorkingSet);
    }
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    MoveVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd], GetMipsRegMapLo(m_Opcode.rt));
}

void CRecompilerOps::COP0_MT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    uint8_t *Jump;

    switch (m_Opcode.rd)
    {
    case 0: //Index
    case 2: //EntryLo0
    case 3: //EntryLo1
    case 4: //Context
    case 5: //PageMask
    case 10: //Entry Hi
    case 14: //EPC
    case 16: //Config
    case 18: //WatchLo
    case 19: //WatchHi
    case 28: //Tag lo
    case 29: //Tag Hi
    case 30: //ErrEPC
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        if (m_Opcode.rd == 4) //Context
        {
            AndConstToVariable(0xFF800000, &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        break;
    case 11: //Compare
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AfterCallDirect(m_RegWorkingSet);
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        AndConstToVariable((uint32_t)~CAUSE_IP7, &g_Reg->FAKE_CAUSE_REGISTER, "FAKE_CAUSE_REGISTER");
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
        AfterCallDirect(m_RegWorkingSet);
        break;
    case 9: //Count
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AfterCallDirect(m_RegWorkingSet);
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
        AfterCallDirect(m_RegWorkingSet);
        break;
    case 12: //Status
    {
        x86Reg OldStatusReg = Map_TempReg(x86_Any, -1, false);
        MoveVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd], OldStatusReg);
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        XorVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd], OldStatusReg);
        TestConstToX86Reg(STATUS_FR, OldStatusReg);
        JeLabel8("FpuFlagFine", 0);
        Jump = m_RecompPos - 1;
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
        Call_Direct(AddressOf(&CRegisters::FixFpuLocations), "CRegisters::FixFpuLocations");

        AfterCallDirect(m_RegWorkingSet);
        SetJump8(Jump, m_RecompPos);

        //TestConstToX86Reg(STATUS_FR,OldStatusReg);
        //BreakPoint(__FILEW__,__LINE__); //m_Section->CompileExit(m_CompilePC+4,m_RegWorkingSet,ExitResetRecompCode,false,JneLabel32);
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
        Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
        AfterCallDirect(m_RegWorkingSet);
    }
    break;
    case 6: //Wired
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AfterCallDirect(m_RegWorkingSet);
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        break;
    case 13: //cause
        AndConstToVariable(0xFFFFCFF, &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        if (IsConst(m_Opcode.rt))
        {
            if ((GetMipsRegLo(m_Opcode.rt) & 0x300) != 0 && bHaveDebugger()){ g_Notify->DisplayError("Set IP0 or IP1"); }
        }
        else if (bHaveDebugger())
        {
            UnknownOpcode();
            return;
        }
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
        Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
        AfterCallDirect(m_RegWorkingSet);
        break;
    default:
        UnknownOpcode();
    }
}

/************************** COP0 CO functions ***********************/
void CRecompilerOps::COP0_CO_TLBR(void)
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (!g_System->bUseTlb()) { return; }
    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::ReadEntry), "CTLB::ReadEntry");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::COP0_CO_TLBWI(void)
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (!g_System->bUseTlb()) { return; }
    BeforeCallDirect(m_RegWorkingSet);
    PushImm32("false", 0);
    MoveVariableToX86reg(&g_Reg->INDEX_REGISTER, "INDEX_REGISTER", x86_ECX);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Push(x86_ECX);
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::COP0_CO_TLBWR(void)
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    if (!g_System->bUseTlb()) { return; }

    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    UpdateCounters(m_RegWorkingSet, false, true);
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
    Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");

    PushImm32("true", true);
    MoveVariableToX86reg(&g_Reg->RANDOM_REGISTER, "RANDOM_REGISTER", x86_ECX);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Push(x86_ECX);
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
    AfterCallDirect(m_RegWorkingSet);
}

void CRecompilerOps::COP0_CO_TLBP(void)
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    if (!g_System->bUseTlb()) { return; }
    BeforeCallDirect(m_RegWorkingSet);
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::Probe), "CTLB::TLB_Probe");
    AfterCallDirect(m_RegWorkingSet);
}

void compiler_COP0_CO_ERET()
{
    if ((g_Reg->STATUS_REGISTER & STATUS_ERL) != 0) {
        g_Reg->m_PROGRAM_COUNTER = g_Reg->ERROREPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_ERL;
    }
    else
    {
        g_Reg->m_PROGRAM_COUNTER = g_Reg->EPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_EXL;
    }
    g_Reg->m_LLBit = 0;
    g_Reg->CheckInterrupts();
}

void CRecompilerOps::COP0_CO_ERET(void)
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_RegWorkingSet.WriteBackRegisters();
    Call_Direct((void *)compiler_COP0_CO_ERET, "compiler_COP0_CO_ERET");

    UpdateCounters(m_RegWorkingSet, true, true);
    m_Section->CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
    m_NextInstruction = END_BLOCK;
}

/************************** FPU Options **************************/
void CRecompilerOps::ChangeDefaultRoundingModel()
{
    switch ((_FPCR[31] & 3))
    {
    case 0: *_RoundingModel = FE_TONEAREST; break;
    case 1: *_RoundingModel = FE_TOWARDZERO; break;
    case 2: *_RoundingModel = FE_UPWARD;   break;
    case 3: *_RoundingModel = FE_DOWNWARD; break;
    }
}

/************************** COP1 functions **************************/
void CRecompilerOps::COP1_MF()
{
    x86Reg TempReg;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();

    UnMap_FPR(m_Opcode.fs, true);
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    TempReg = Map_TempReg(x86_Any, -1, false);
    char Name[100];
    sprintf(Name, "_FPR_S[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.fs], Name, TempReg);
    MoveX86PointerToX86reg(GetMipsRegMapLo(m_Opcode.rt), TempReg);
}

void CRecompilerOps::COP1_DMF()
{
    x86Reg TempReg;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();

    UnMap_FPR(m_Opcode.fs, true);
    Map_GPR_64bit(m_Opcode.rt, -1);
    TempReg = Map_TempReg(x86_Any, -1, false);
    sprintf(Name, "_FPR_D[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.fs], Name, TempReg);
    AddConstToX86Reg(TempReg, 4);
    MoveX86PointerToX86reg(GetMipsRegMapHi(m_Opcode.rt), TempReg);
    sprintf(Name, "_FPR_D[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.fs], Name, TempReg);
    MoveX86PointerToX86reg(GetMipsRegMapLo(m_Opcode.rt), TempReg);
}

void CRecompilerOps::COP1_CF()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();

    if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
    {
        UnknownOpcode();
        return;
    }

    Map_GPR_32bit(m_Opcode.rt, true, -1);
    MoveVariableToX86reg(&_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], GetMipsRegMapLo(m_Opcode.rt));
}

void CRecompilerOps::COP1_MT()
{
    x86Reg TempReg;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();

    if ((m_Opcode.fs & 1) != 0)
    {
        if (RegInStack(m_Opcode.fs - 1, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs - 1, CRegInfo::FPU_Qword))
        {
            UnMap_FPR(m_Opcode.fs - 1, true);
        }
    }
    UnMap_FPR(m_Opcode.fs, true);
    TempReg = Map_TempReg(x86_Any, -1, false);
    char Name[50];
    sprintf(Name, "_FPR_S[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.fs], Name, TempReg);

    if (IsConst(m_Opcode.rt))
    {
        MoveConstToX86Pointer(GetMipsRegLo(m_Opcode.rt), TempReg);
    }
    else if (IsMapped(m_Opcode.rt))
    {
        MoveX86regToX86Pointer(GetMipsRegMapLo(m_Opcode.rt), TempReg);
    }
    else
    {
        MoveX86regToX86Pointer(Map_TempReg(x86_Any, m_Opcode.rt, false), TempReg);
    }
}

void CRecompilerOps::COP1_DMT()
{
    x86Reg TempReg;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();

    if ((m_Opcode.fs & 1) == 0)
    {
        if (RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Float) || RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Dword)) {
            UnMap_FPR(m_Opcode.fs + 1, true);
        }
    }
    UnMap_FPR(m_Opcode.fs, true);
    TempReg = Map_TempReg(x86_Any, -1, false);
    char Name[50];
    sprintf(Name, "_FPR_D[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.fs], Name, TempReg);

    if (IsConst(m_Opcode.rt))
    {
        MoveConstToX86Pointer(GetMipsRegLo(m_Opcode.rt), TempReg);
        AddConstToX86Reg(TempReg, 4);
        if (Is64Bit(m_Opcode.rt))
        {
            MoveConstToX86Pointer(GetMipsRegHi(m_Opcode.rt), TempReg);
        }
        else
        {
            MoveConstToX86Pointer(GetMipsRegLo_S(m_Opcode.rt) >> 31, TempReg);
        }
    }
    else if (IsMapped(m_Opcode.rt))
    {
        MoveX86regToX86Pointer(GetMipsRegMapLo(m_Opcode.rt), TempReg);
        AddConstToX86Reg(TempReg, 4);
        if (Is64Bit(m_Opcode.rt))
        {
            MoveX86regToX86Pointer(GetMipsRegMapHi(m_Opcode.rt), TempReg);
        }
        else
        {
            MoveX86regToX86Pointer(Map_TempReg(x86_Any, m_Opcode.rt, true), TempReg);
        }
    }
    else
    {
        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);
        MoveX86regToX86Pointer(Reg, TempReg);
        AddConstToX86Reg(TempReg, 4);
        MoveX86regToX86Pointer(Map_TempReg(Reg, m_Opcode.rt, true), TempReg);
    }
}

void CRecompilerOps::COP1_CT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();

    if (m_Opcode.fs != 31)
    {
        UnknownOpcode();
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    else if (IsMapped(m_Opcode.rt))
    {
        MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    else
    {
        MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    BeforeCallDirect(m_RegWorkingSet);
    Call_Direct((void *)ChangeDefaultRoundingModel, "ChangeDefaultRoundingModel");
    AfterCallDirect(m_RegWorkingSet);
    m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
}

/************************** COP1: S functions ************************/
void CRecompilerOps::COP1_S_ADD()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        fpuAddReg(StackPosition(Reg2));
    }
    else
    {
        x86Reg TempReg;

        UnMap_FPR(Reg2, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        char Name[50];
        sprintf(Name, "_FPR_S[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);
        fpuAddDwordRegPointer(TempReg);
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.ft], Name, TempReg);
        fpuSubDwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            fpuSubReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_S[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
            fpuSubDwordRegPointer(TempReg);
        }
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        fpuMulReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

        TempReg = Map_TempReg(x86_Any, -1, false);
        char Name[50];
        sprintf(Name, "_FPR_S[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
        fpuMulDwordRegPointer(TempReg);
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.ft], Name, TempReg);
        fpuDivDwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            fpuDivReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_S[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
            fpuDivDwordRegPointer(TempReg);
        }
    }

    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_ABS()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    fpuAbs();
    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_NEG()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    fpuNeg();
    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_SQRT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    fpuSqrt();
    UnMap_FPR(m_Opcode.fd, true);
}

void CRecompilerOps::COP1_S_MOV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
}

void CRecompilerOps::COP1_S_ROUND_L()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundNearest);
}

void CRecompilerOps::COP1_S_TRUNC_L()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundTruncate);
}

void CRecompilerOps::COP1_S_CEIL_L() //added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundUp);
}

void CRecompilerOps::COP1_S_FLOOR_L()  //added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundDown);
}

void CRecompilerOps::COP1_S_ROUND_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundNearest);
}

void CRecompilerOps::COP1_S_TRUNC_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundTruncate);
}

void CRecompilerOps::COP1_S_CEIL_W() // added by Witten
{			// added by Witten
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundUp);
}

void CRecompilerOps::COP1_S_FLOOR_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundDown);
}

void CRecompilerOps::COP1_S_CVT_D()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_S_CVT_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_S_CVT_L()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_S_CMP()
{
    uint32_t Reg1 = m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft;
    uint32_t cmp = 0;

    if ((m_Opcode.funct & 4) == 0)
    {
        Reg1 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float) ? m_Opcode.ft : m_Opcode.fs;
        Reg2 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float) ? m_Opcode.fs : m_Opcode.ft;
    }

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if ((m_Opcode.funct & 7) == 0) { CRecompilerOps::UnknownOpcode(); }
    if ((m_Opcode.funct & 2) != 0) { cmp |= 0x4000; }
    if ((m_Opcode.funct & 4) != 0) { cmp |= 0x0100; }

    Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);
    Map_TempReg(x86_EAX, 0, false);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        fpuComReg(StackPosition(Reg2), false);
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);

        x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
        char Name[50];
        sprintf(Name, "_FPR_S[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
        fpuComDwordRegPointer(TempReg, false);
    }
    AndConstToVariable((uint32_t)~FPCSR_C, &_FPCR[31], "_FPCR[31]");
    fpuStoreStatus();
    x86Reg Reg = Map_TempReg(x86_Any8Bit, 0, false);
    TestConstToX86Reg(cmp, x86_EAX);
    Setnz(Reg);

    if (cmp != 0)
    {
        TestConstToX86Reg(cmp, x86_EAX);
        Setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            x86Reg Reg2 = Map_TempReg(x86_Any8Bit, 0, false);
            AndConstToX86Reg(x86_EAX, 0x4300);
            CompConstToX86reg(x86_EAX, 0x4300);
            Setz(Reg2);

            OrX86RegToX86Reg(Reg, Reg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        AndConstToX86Reg(x86_EAX, 0x4300);
        CompConstToX86reg(x86_EAX, 0x4300);
        Setz(Reg);
    }
    ShiftLeftSignImmed(Reg, 23);
    OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", Reg);
}

/************************** COP1: D functions ************************/
void CRecompilerOps::COP1_D_ADD()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        fpuAddReg(StackPosition(Reg2));
    }
    else
    {
        x86Reg TempReg;

        UnMap_FPR(Reg2, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        fpuAddQwordRegPointer(TempReg);
    }
}

void CRecompilerOps::COP1_D_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.ft], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        fpuSubQwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            fpuSubReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);

            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_D[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            fpuSubQwordRegPointer(TempReg);
        }
    }
}

void CRecompilerOps::COP1_D_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        fpuMulReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
        fpuMulQwordRegPointer(TempReg);
    }
}

void CRecompilerOps::COP1_D_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.ft], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        fpuDivQwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            fpuDivReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_D[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            fpuDivQwordRegPointer(TempReg);
        }
    }
}

void CRecompilerOps::COP1_D_ABS()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    fpuAbs();
}

void CRecompilerOps::COP1_D_NEG()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    fpuNeg();
}

void CRecompilerOps::COP1_D_SQRT()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    fpuSqrt();
}

void CRecompilerOps::COP1_D_MOV()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
}

void CRecompilerOps::COP1_D_TRUNC_L() //added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundTruncate);
}

void CRecompilerOps::COP1_D_CEIL_L() //added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundUp);
}

void CRecompilerOps::COP1_D_FLOOR_L() //added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundDown);
}

void CRecompilerOps::COP1_D_ROUND_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundNearest);
}

void CRecompilerOps::COP1_D_TRUNC_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fd, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fd, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundTruncate);
}

void CRecompilerOps::COP1_D_CEIL_W() // added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundUp);
}

void CRecompilerOps::COP1_D_FLOOR_W() //added by Witten
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundDown);
}

void CRecompilerOps::COP1_D_CVT_S()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fd, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fd, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_D_CVT_W()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_D_CVT_L()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_D_CMP()
{
    uint32_t Reg1 = m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft;
    uint32_t cmp = 0;

    if ((m_Opcode.funct & 4) == 0)
    {
        Reg1 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) ? m_Opcode.ft : m_Opcode.fs;
        Reg2 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) ? m_Opcode.fs : m_Opcode.ft;
    }
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if ((m_Opcode.funct & 7) == 0) { CRecompilerOps::UnknownOpcode(); }
    if ((m_Opcode.funct & 2) != 0) { cmp |= 0x4000; }
    if ((m_Opcode.funct & 4) != 0) { cmp |= 0x0100; }

    Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
    Map_TempReg(x86_EAX, 0, false);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        fpuComReg(StackPosition(Reg2), false);
    }
    else
    {
        char Name[50];

        UnMap_FPR(Reg2, true);
        x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
        Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
        fpuComQwordRegPointer(TempReg, false);
    }
    AndConstToVariable((uint32_t)~FPCSR_C, &_FPCR[31], "_FPCR[31]");
    fpuStoreStatus();
    x86Reg Reg = Map_TempReg(x86_Any8Bit, 0, false);
    TestConstToX86Reg(cmp, x86_EAX);
    Setnz(Reg);
    if (cmp != 0)
    {
        TestConstToX86Reg(cmp, x86_EAX);
        Setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            x86Reg Reg2 = Map_TempReg(x86_Any8Bit, 0, false);
            AndConstToX86Reg(x86_EAX, 0x4300);
            CompConstToX86reg(x86_EAX, 0x4300);
            Setz(Reg2);

            OrX86RegToX86Reg(Reg, Reg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        AndConstToX86Reg(x86_EAX, 0x4300);
        CompConstToX86reg(x86_EAX, 0x4300);
        Setz(Reg);
    }
    ShiftLeftSignImmed(Reg, 23);
    OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", Reg);
}

/************************** COP1: W functions ************************/
void CRecompilerOps::COP1_W_CVT_S()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Dword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Dword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Dword, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_W_CVT_D()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Dword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Dword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Dword, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

/************************** COP1: L functions ************************/
void CRecompilerOps::COP1_L_CVT_S()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Qword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Qword, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_L_CVT_D()
{
    CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_Section->CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Qword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Qword, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

/************************** Other functions **************************/
void CRecompilerOps::UnknownOpcode()
{
    CPU_Message("  %X Unhandled Opcode: %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem)
    {
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());

    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
    Ret();
    if (m_NextInstruction == NORMAL) { m_NextInstruction = END_BLOCK; }
}

void CRecompilerOps::BeforeCallDirect(CRegInfo  & RegSet)
{
    RegSet.UnMap_AllFPRs();
    Pushad();
}

void CRecompilerOps::AfterCallDirect(CRegInfo  & RegSet)
{
    Popad();
    RegSet.SetRoundingModel(CRegInfo::RoundUnknown);
}

void CRecompilerOps::EnterCodeBlock()
{
#ifdef _DEBUG
    Push(x86_ESI);
#else
    Push(x86_EDI);
    Push(x86_ESI);
    Push(x86_EBX);
#endif
}

void CRecompilerOps::ExitCodeBlock()
{
#ifdef _DEBUG
    Pop(x86_ESI);
#else
    Pop(x86_EBX);
    Pop(x86_ESI);
    Pop(x86_EDI);
#endif
    Ret();
}

void CRecompilerOps::UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles)
{
    if (!g_SyncSystem)
    {
        return;
    }

    WriteX86Comment("Updating Sync CPU");
    BeforeCallDirect(RegSet);
    PushImm32(stdstr_f("%d", Cycles).c_str(), Cycles);
    PushImm32("g_SyncSystem", (uint32_t)g_SyncSystem);
    MoveConstToX86reg((uint32_t)g_System, x86_ECX);
    Call_Direct(AddressOf(&CN64System::UpdateSyncCPU), "CN64System::UpdateSyncCPU");
    AfterCallDirect(RegSet);
}

void CRecompilerOps::UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues)
{
    if (RegSet.GetBlockCycleCount() != 0)
    {
        UpdateSyncCPU(RegSet, RegSet.GetBlockCycleCount());
        WriteX86Comment("Update Counter");
        SubConstFromVariable(RegSet.GetBlockCycleCount(), g_NextTimer, "g_NextTimer"); // updates compare flag
        if (ClearValues)
        {
            RegSet.SetBlockCycleCount(0);
        }
    }
    else if (CheckTimer)
    {
        CompConstToVariable(0, g_NextTimer, "g_NextTimer");
    }

    if (CheckTimer)
    {
        JnsLabel8("Continue_From_Timer_Test", 0);
        uint8_t * Jump = m_RecompPos - 1;
        Pushad();
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
        Popad();

        CPU_Message("");
        CPU_Message("      $Continue_From_Timer_Test:");
        SetJump8(Jump, m_RecompPos);
    }
}

void CRecompilerOps::CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet)
{
    CompConstToVariable(0, (void *)&g_SystemEvents->DoSomething(), "g_SystemEvents->DoSomething()");
    JeLabel32("Continue_From_Interrupt_Test", 0);
    uint32_t * Jump = (uint32_t *)(m_RecompPos - 4);
    if (TargetPC != (uint32_t)-1)
    {
        MoveConstToVariable(TargetPC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    }

    CRegInfo RegSetCopy(RegSet);
    RegSetCopy.WriteBackRegisters();

    MoveConstToX86reg((uint32_t)g_SystemEvents, x86_ECX);
    Call_Direct(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
    if (g_SyncSystem)
    {
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }
    ExitCodeBlock();
    CPU_Message("");
    CPU_Message("      $Continue_From_Interrupt_Test:");
    SetJump32(Jump, (uint32_t *)m_RecompPos);
}

void CRecompilerOps::OverflowDelaySlot(bool TestTimer)
{
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(CompilePC() + 4, _PROGRAM_COUNTER, "PROGRAM_COUNTER");

    if (g_SyncSystem)
    {
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }

    MoveConstToVariable(JUMP, &R4300iOp::m_NextInstruction, "R4300iOp::m_NextInstruction");

    if (TestTimer)
    {
        MoveConstToVariable(TestTimer, &R4300iOp::m_TestTimer, "R4300iOp::m_TestTimer");
    }

    PushImm32("g_System->CountPerOp()", g_System->CountPerOp());
    Call_Direct((void *)CInterpreterCPU::ExecuteOps, "CInterpreterCPU::ExecuteOps");
    AddConstToX86Reg(x86_ESP, 4);

    if (g_System->bFastSP() && g_Recompiler)
    {
        MoveConstToX86reg((uint32_t)g_Recompiler, x86_ECX);
        Call_Direct(AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos");
    }

    if (g_SyncSystem)
    {
        UpdateSyncCPU(m_RegWorkingSet, g_System->CountPerOp());
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }

    ExitCodeBlock();
    m_NextInstruction = END_BLOCK;
}