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

#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps32.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Recompiler/SectionInfo.h>
#include <Project64-core/N64System/Recompiler/LoopAnalysis.h>
#include <Project64-core/N64System/Recompiler/Arm/ArmRecompilerOps.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/ExceptionHandler.h>

uint32_t CArmRecompilerOps::m_TempValue = 0;

/*uint32_t TestValue = 0;
void TestFunc()
{
CPU_Message("%s: %X t2: %X", __FUNCTION__,TestValue, g_Reg->m_GPR[10].UW[0]);
}*/
void CArmRecompilerOps::PreCompileOpcode(void)
{
    if (m_NextInstruction != DELAY_SLOT_DONE)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    }

    /*FlushPopArmReg();
    ArmNop();
    m_RegWorkingSet.BeforeCallDirect();

    MoveConstToArmReg(Arm_R1,m_CompilePC);
    MoveConstToArmReg(Arm_R2,(uint32_t)&TestValue, "TestValue");
    StoreArmRegToArmRegPointer(Arm_R1,Arm_R2,0);
    CallFunction(AddressOf(&TestFunc), "TestFunc");
    m_RegWorkingSet.AfterCallDirect();*/

    /*if ((m_CompilePC == 0x8027F564 || m_CompilePC == 0x8027F574) && m_NextInstruction == NORMAL)
    {
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToArmReg(Arm_R0,(uint32_t)&TestValue, "TestValue");
    StoreArmRegToArmRegPointer(Arm_R1,Arm_R0,0);
    CallFunction(AddressOf(&TestFunc), "TestFunc");
    m_RegWorkingSet.AfterCallDirect();

    for (int32_t i = 1; i < 32; i++)
    {
    m_RegWorkingSet.WriteBack_GPR(i,false);
    }
    UpdateCounters(m_RegWorkingSet,false,true);
    if (g_SyncSystem)
    {
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToArmReg(Arm_R1, m_CompilePC);
    MoveConstToArmReg(Arm_R2, (uint32_t)&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    StoreArmRegToArmRegPointer(Arm_R1, Arm_R2, 0);
    MoveConstToArmReg(Arm_R0, (uint32_t)g_BaseSystem, "g_BaseSystem");
    CallFunction(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_RegWorkingSet.AfterCallDirect();
    }
    }*/

    /*if (m_CompilePC == 0x8027F564 && m_NextInstruction == NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem)
    {
    MoveConstToArmReg(Arm_R0, (uint32_t)g_BaseSystem, "g_BaseSystem");
    CallFunction(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }
    }*/
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    m_RegWorkingSet.ResetRegProtection();
}

void CArmRecompilerOps::PostCompileOpcode(void)
{
    if (!g_System->bRegCaching())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_RegWorkingSet.ResetRegProtection();
}

CArmRecompilerOps::CArmRecompilerOps() :
    m_NextInstruction(NORMAL)
{
    memset(&m_Opcode, 0, sizeof(m_Opcode));
}

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

/************************** Branch functions  ************************/
void CArmRecompilerOps::Compile_BranchCompare(BRANCH_COMPARE CompareType)
{
    switch (CompareType)
    {
    case CompareTypeBEQ: BEQ_Compare(); break;
    case CompareTypeBNE: BNE_Compare(); break;
    case CompareTypeBLTZ: BLTZ_Compare(); break;
    case CompareTypeBLEZ: BLEZ_Compare(); break;
    case CompareTypeBGTZ: BGTZ_Compare(); break;
    case CompareTypeBGEZ: BGEZ_Compare(); break;
    case CompareTypeCOP1BCF: COP1_BCF_Compare(); break;
    case CompareTypeCOP1BCT: COP1_BCT_Compare(); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmRecompilerOps::Compile_Branch(BRANCH_COMPARE CompareType, BRANCH_TYPE BranchType, bool Link)
{
    static CRegInfo RegBeforeDelay;
    static bool EffectDelaySlot;
    OPCODE Command = { 0 };

    if (m_NextInstruction == NORMAL)
    {
        if (CompareType == CompareTypeCOP1BCF || CompareType == CompareTypeCOP1BCT)
        {
            CompileCop1Test();
        }
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
            if (!g_System->b32BitCore())
            {
                MoveConstToVariable((m_CompilePC & 0x80000000) != 0 ? 0xFFFFFFFF : 0, &_GPR[31].UW[1], CRegName::GPR_Hi[31]);
            }
            MoveConstToVariable(m_CompilePC + 8, &_GPR[31].UW[0], CRegName::GPR_Lo[31]);
        }
        if (EffectDelaySlot)
        {
            if ((m_CompilePC & 0xFFC) != 0xFFC)
            {
                m_Section->m_Cont.BranchLabel = m_Section->m_ContinueSection != NULL ? "Continue" : "ContinueExitBlock";
                m_Section->m_Jump.BranchLabel = m_Section->m_JumpSection != NULL ? "Jump" : "JumpExitBlock";
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
                if (m_Section->m_Jump.LinkLocation != NULL)
                {
                    CPU_Message("");
                    CPU_Message("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    LinkJump(m_Section->m_Jump);
                    m_Section->m_Jump.FallThrough = true;
                }
                else if (m_Section->m_Cont.LinkLocation != NULL)
                {
                    CPU_Message("");
                    CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    LinkJump(m_Section->m_Cont);
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
                    if (DelayLinkLocation != NULL) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                    DelayLinkLocation = *g_RecompPos;
                    BranchLabel8(ArmBranch_Always, "DoDelaySlot");

                    CPU_Message("      ");
                    CPU_Message("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    LinkJump(m_Section->m_Jump);
                    MoveConstToVariable(m_Section->m_Jump.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                }
                if (m_Section->m_Cont.LinkLocation != NULL || m_Section->m_Cont.LinkLocation2 != NULL)
                {
                    if (DelayLinkLocation != NULL) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                    DelayLinkLocation = *g_RecompPos;
                    BranchLabel8(ArmBranch_Always, "DoDelaySlot");

                    CPU_Message("      ");
                    CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    LinkJump(m_Section->m_Cont);
                    MoveConstToVariable(m_Section->m_Cont.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                }
                if (DelayLinkLocation)
                {
                    CPU_Message("");
                    CPU_Message("      DoDelaySlot:");
                    SetJump8(DelayLinkLocation, *g_RecompPos);
                }
                ResetRegProtection();
                OverflowDelaySlot(false);
                return;
            }
            ResetRegProtection();
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
                ResetRegProtection();
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
                        ResetRegProtection();
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
                    BranchLabel20(ArmBranch_Always, FallInfo->BranchLabel.c_str());
                    FallInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);

                    if (JumpInfo->LinkLocation != NULL)
                    {
                        CPU_Message("      %s:", JumpInfo->BranchLabel.c_str());
                        LinkJump(*JumpInfo);
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
                Compile_BranchCompare(CompareType);
                ResetRegProtection();
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
            g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_NextInstruction).c_str());
        }
    }
}

void CArmRecompilerOps::Compile_BranchLikely(BRANCH_COMPARE CompareType, bool Link)
{
    if (m_NextInstruction == NORMAL)
    {
        if (CompareType == CompareTypeCOP1BCF || CompareType == CompareTypeCOP1BCT)
        {
            CompileCop1Test();
        }
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
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
            m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
#endif
        }

        Compile_BranchCompare(CompareType);
        ResetRegProtection();

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
                LinkJump(m_Section->m_Jump);

                MoveConstToVariable(m_Section->m_Jump.TargetPC, &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                OverflowDelaySlot(false);
                CPU_Message("      ");
                CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
            }
            else if (!m_Section->m_Cont.FallThrough)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            LinkJump(m_Section->m_Cont);
            CompileExit(m_CompilePC, m_CompilePC + 8, m_Section->m_Cont.RegSet, CExitInfo::Normal);
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
        ResetRegProtection();
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->GenerateSectionLinkage();
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_NextInstruction).c_str());
    }
}

void CArmRecompilerOps::BNE_Compare()
{
    uint8_t * Jump = NULL;

    if (IsKnown(m_Opcode.rs) && IsKnown(m_Opcode.rt))
    {
        if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt))
        {
            if (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                CArmRecompilerOps::UnknownOpcode();
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
            if (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)))
            {
                CompareArmRegToArmReg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(Arm_Any, m_Opcode.rs, true) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(Arm_Any, m_Opcode.rt, true) : GetMipsRegMapHi(m_Opcode.rt)
                );

                if (m_Section->m_Jump.FallThrough)
                {
                    Jump = *g_RecompPos;
                    BranchLabel8(ArmBranch_Notequal, "continue");
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompareArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompareArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (!g_System->b32BitCore() && (Is64Bit(ConstReg) || Is64Bit(MappedReg)))
            {
                if (Is32Bit(ConstReg) || Is32Bit(MappedReg))
                {
                    ProtectGPR(MappedReg);
                    if (Is32Bit(MappedReg))
                    {
                        CompareArmRegToConst(Map_TempReg(Arm_Any, MappedReg, true), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        CompareArmRegToConst(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    //CompareArmRegToConst(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Jump.FallThrough)
                {
                    Jump = *g_RecompPos;
                    BranchLabel8(ArmBranch_Notequal, "continue");
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompareArmRegToConst(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompareArmRegToConst(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rs) || IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsMapped(KnownReg))
        {
            ProtectGPR(KnownReg);
        }

        ArmReg TempRegUnknown = Arm_Any;
        if (!g_System->b32BitCore())
        {
            TempRegUnknown = Map_TempReg(Arm_Any, UnknownReg, true);
            if (IsConst(KnownReg))
            {
                if (Is32Bit(KnownReg) && IsSigned(KnownReg))
                {
                    CompareArmRegToConst(TempRegUnknown, (GetMipsRegLo_S(KnownReg) >> 31));
                }
                else if (Is32Bit(KnownReg))
                {
                    CompareArmRegToConst(TempRegUnknown, 0);
                }
                else
                {
                    CompareArmRegToConst(TempRegUnknown, GetMipsRegHi(KnownReg));
                }
            }
            else
            {
                ProtectGPR(KnownReg);
                CompareArmRegToArmReg(TempRegUnknown, Is32Bit(KnownReg) ? Map_TempReg(Arm_Any, KnownReg, true) : GetMipsRegMapHi(KnownReg));
            }
            if (m_Section->m_Jump.FallThrough)
            {
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Notequal, "continue");
            }
            else
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            m_RegWorkingSet.SetArmRegProtected(TempRegUnknown, false);
        }
        TempRegUnknown = Map_TempReg(TempRegUnknown, UnknownReg, false);
        if (IsConst(KnownReg))
        {
            CompareArmRegToConst(TempRegUnknown, GetMipsRegLo(KnownReg));
        }
        else
        {
            CompareArmRegToArmReg(TempRegUnknown, GetMipsRegMapLo(KnownReg));
        }
        m_RegWorkingSet.SetArmRegProtected(TempRegUnknown, false);

        if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);

            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");

                SetJump8(Jump, *g_RecompPos);
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            CArmRecompilerOps::UnknownOpcode();
            /*JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
            m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }*/
        }
    }
    else
    {
        if (!g_System->b32BitCore())
        {
            ArmReg TempRegRs = Map_TempReg(Arm_Any, m_Opcode.rs, true);
            ArmReg TempRegRt = Map_TempReg(Arm_Any, m_Opcode.rt, true);
            CompareArmRegToArmReg(TempRegRs, TempRegRt);
            m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
            m_RegWorkingSet.SetArmRegProtected(TempRegRt, false);

            if (m_Section->m_Jump.FallThrough)
            {
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Notequal, "continue");
            }
            else
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }

        ArmReg TempRegRs = Map_TempReg(Arm_Any, m_Opcode.rs, false);
        ArmReg TempRegRt = Map_TempReg(Arm_Any, m_Opcode.rt, false);
        CompareArmRegToArmReg(TempRegRs, TempRegRt);
        m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
        m_RegWorkingSet.SetArmRegProtected(TempRegRt, false);

        if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
#endif
        }
    }
}

void CArmRecompilerOps::BEQ_Compare()
{
    uint8_t *Jump = NULL;

    if (IsKnown(m_Opcode.rs) && IsKnown(m_Opcode.rt))
    {
        if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt))
        {
            if (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)))
            {
                CArmRecompilerOps::UnknownOpcode();
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

                CompareArmRegToArmReg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(Arm_Any, m_Opcode.rs, true) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(Arm_Any, m_Opcode.rt, true) : GetMipsRegMapHi(m_Opcode.rt)
                );
                if (m_Section->m_Cont.FallThrough)
                {
                    Jump = *g_RecompPos;
                    BranchLabel8(ArmBranch_Notequal, "continue");
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompareArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompareArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (!g_System->b32BitCore() && (Is64Bit(ConstReg) || Is64Bit(MappedReg)))
            {
                if (Is32Bit(ConstReg) || Is32Bit(MappedReg))
                {
                    if (Is32Bit(MappedReg))
                    {
                        ProtectGPR(MappedReg);
                        CompareArmRegToConst(Map_TempReg(Arm_Any, MappedReg, true), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        CompareArmRegToConst(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    CompareArmRegToConst(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Cont.FallThrough)
                {
                    Jump = *g_RecompPos;
                    BranchLabel8(ArmBranch_Notequal, "continue");
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompareArmRegToConst(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompareArmRegToConst(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rs) || IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        ArmReg TempReg = Arm_Any;
        if (!g_System->b32BitCore())
        {
            TempReg = Map_TempReg(Arm_Any, UnknownReg, true);
            if (IsConst(KnownReg))
            {
                if (Is32Bit(KnownReg) && IsSigned(KnownReg))
                {
                    CompareArmRegToConst(TempReg, (GetMipsRegLo_S(KnownReg) >> 31));
                }
                else if (Is32Bit(KnownReg))
                {
                    CompareArmRegToConst(TempReg, 0);
                }
                else
                {
                    CompareArmRegToConst(TempReg, GetMipsRegHi(KnownReg));
                }
            }
            else
            {
                ProtectGPR(KnownReg);
                CompareArmRegToArmReg(TempReg, Is32Bit(KnownReg) ? Map_TempReg(Arm_Any, KnownReg, true) : GetMipsRegMapHi(KnownReg));
            }
            if (m_Section->m_Cont.FallThrough)
            {
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Notequal, "continue");
            }
            else
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        }
        TempReg = Map_TempReg(TempReg, UnknownReg, false);
        if (IsConst(KnownReg))
        {
            CompareArmRegToConst(TempReg, GetMipsRegLo(KnownReg));
        }
        else
        {
            CompareArmRegToArmReg(TempReg, GetMipsRegMapLo(KnownReg));
        }
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else
    {
        if (!g_System->b32BitCore())
        {
            ArmReg TempRegRs = Map_TempReg(Arm_Any, m_Opcode.rs, true);
            ArmReg TempRegRt = Map_TempReg(Arm_Any, m_Opcode.rt, true);
            CompareArmRegToArmReg(TempRegRs, TempRegRt);
            m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
            m_RegWorkingSet.SetArmRegProtected(TempRegRt, false);

            if (m_Section->m_Cont.FallThrough)
            {
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Notequal, "continue");
            }
            else
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        ArmReg TempRegRs = Map_TempReg(Arm_Any, m_Opcode.rs, false);
        ArmReg TempRegRt = Map_TempReg(Arm_Any, m_Opcode.rt, false);
        CompareArmRegToArmReg(TempRegRs, TempRegRt);
        m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
        m_RegWorkingSet.SetArmRegProtected(TempRegRt, false);
        if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
            BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CArmRecompilerOps::BGTZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            CArmRecompilerOps::UnknownOpcode();
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
    else if ((IsMapped(m_Opcode.rs) && Is32Bit(m_Opcode.rs)) || (IsUnknown(m_Opcode.rs) && g_System->b32BitCore()))
    {
        if (IsMapped(m_Opcode.rs))
        {
            CompareArmRegToConst(GetMipsRegMapLo(m_Opcode.rs), 0);
        }
        else
        {
            ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, false);
            CompareArmRegToConst(TempReg, 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_LessThanOrEqual, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            BranchLabel20(ArmBranch_LessThanOrEqual, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else
    {
        uint8_t *Jump = NULL;

        if (IsMapped(m_Opcode.rs))
        {
            CompareArmRegToConst(GetMipsRegMapHi(m_Opcode.rs), 0);
        }
        else
        {
            ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, true);
            CompareArmRegToConst(TempReg, 0);
            m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            Jump = *g_RecompPos;
            BranchLabel8(ArmBranch_GreaterThan, "continue");
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            Jump = *g_RecompPos;
            BranchLabel8(ArmBranch_LessThan, "continue");
            BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }

        if (IsMapped(m_Opcode.rs))
        {
            CompareArmRegToConst(GetMipsRegMapLo(m_Opcode.rs), 0);
        }
        else
        {
            ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, false);
            CompareArmRegToConst(TempReg, 0);
            m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            CPU_Message("      continue:");
            SetJump8(Jump, *g_RecompPos);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            CPU_Message("      continue:");
            SetJump8(Jump, *g_RecompPos);
        }
        else
        {
            BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CArmRecompilerOps::BLEZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            /*if (GetMipsReg_S(m_Opcode.rs) <= 0)
            {
            m_Section->m_Jump.FallThrough = true;
            m_Section->m_Cont.FallThrough = false;
            }
            else
            {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
            }*/
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
        else if (GetMipsRegLo(m_Opcode.rs) == 0)
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
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is32Bit(m_Opcode.rs))
        {
            CompareArmRegToConst(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_LessThanOrEqual, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            uint8_t *Jump = NULL;

            ArmReg TempRegRs = Arm_Any;
            if (IsMapped(m_Opcode.rs))
            {
                CompareArmRegToConst(GetMipsRegMapHi(m_Opcode.rs), 0);
            }
            else
            {
                TempRegRs = Map_TempReg(Arm_Any, m_Opcode.rs, true);
                CompareArmRegToConst(TempRegRs, 0);
                m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_LessThan, "Continue");
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_GreaterThan, "Continue");
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }

            if (IsMapped(m_Opcode.rs))
            {
                CompareArmRegToConst(GetMipsRegMapLo(m_Opcode.rs), 0);
            }
            else
            {
                TempRegRs = Map_TempReg(TempRegRs, m_Opcode.rs, false);
                CompareArmRegToConst(TempRegRs, 0);
                m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            else
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, "BranchToJump");
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
    else
    {
        uint8_t *Jump = NULL;

        if (!g_System->b32BitCore())
        {
            ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, true);
            CompareArmRegToConst(TempReg, 0);
            m_RegWorkingSet.SetArmRegProtected(TempReg, false);
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_LessThan, "Continue");
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_GreaterThan, "Continue");
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, false);
            CompareArmRegToConst(TempReg, 0);
            m_RegWorkingSet.SetArmRegProtected(TempReg, false);
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                if (Jump)
                {
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                if (Jump)
                {
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
            }
            else
            {
                BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, "BranchToJump");
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, false);
            CompareArmRegToConst(TempReg, 0);
            m_RegWorkingSet.SetArmRegProtected(TempReg, false);
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_LessThanOrEqual, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_GreaterThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
}

void CArmRecompilerOps::BLTZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
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
        if (Is64Bit(m_Opcode.rs) || IsSigned(m_Opcode.rs))
        {
            CompareArmRegToConst(Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : GetMipsRegMapLo(m_Opcode.rs), (uint32_t)0);
            if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThanOrEqual, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_GreaterThanOrEqual, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
        }
    }
    else
    {
        ArmReg TempReg1 = Map_TempReg(Arm_Any, m_Opcode.rs, !g_System->b32BitCore());
        ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg2, (uint32_t)0);
        CompareArmRegToArmReg(TempReg1, TempReg2);
        if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_GreaterThanOrEqual, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CArmRecompilerOps::BGEZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            CArmRecompilerOps::UnknownOpcode();
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
            CompareArmRegToConst(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThanOrEqual, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            CompareArmRegToConst(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                BranchLabel20(ArmBranch_GreaterThanOrEqual, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
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
        ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, !g_System->b32BitCore());
        CompareArmRegToConst(TempReg, 0);
        if (m_Section->m_Cont.FallThrough)
        {
            BranchLabel20(ArmBranch_GreaterThanOrEqual, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            BranchLabel20(ArmBranch_LessThan, m_Section->m_Cont.BranchLabel.c_str());
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CArmRecompilerOps::COP1_BCF_Compare()
{
    TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        BranchLabel20(ArmBranch_Notequal, m_Section->m_Jump.BranchLabel.c_str());
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else
    {
        BranchLabel20(ArmBranch_Equal, m_Section->m_Cont.BranchLabel.c_str());
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
}

void CArmRecompilerOps::COP1_BCT_Compare()
{
    TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        BranchLabel20(ArmBranch_Equal, m_Section->m_Jump.BranchLabel.c_str());
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else
    {
        BranchLabel20(ArmBranch_Notequal, m_Section->m_Cont.BranchLabel.c_str());
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        BranchLabel20(ArmBranch_Always, m_Section->m_Jump.BranchLabel.c_str());
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
}

/*************************  OpCode functions *************************/
void CArmRecompilerOps::J()
{
    if (m_NextInstruction == NORMAL)
    {
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
        g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_NextInstruction).c_str());
    }
}

void CArmRecompilerOps::JAL()
{
    if (m_NextInstruction == NORMAL)
    {
        Map_GPR_32bit(31, true, -1);
        MoveVariableToArmReg(_PROGRAM_COUNTER, "_PROGRAM_COUNTER", GetMipsRegMapLo(31));
        ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, 0xF0000000);
        AndArmRegToArmReg(GetMipsRegMapLo(31), GetMipsRegMapLo(31), TempReg);
        MoveConstToArmReg(TempReg, (m_CompilePC + 8) & ~0xF0000000);
        OrArmRegToArmReg(GetMipsRegMapLo(31), GetMipsRegMapLo(31), TempReg, 0);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);

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

            MoveConstToArmReg(Arm_R0, (uint32_t)_PROGRAM_COUNTER, "_PROGRAM_COUNTER");
            LoadArmRegPointerToArmReg(Arm_R1, Arm_R0, 0);
            MoveConstToArmReg(Arm_R2, 0xF0000000);
            MoveConstToArmReg(Arm_R3, (uint32_t)(m_Opcode.target << 2));
            AndArmRegToArmReg(Arm_R1, Arm_R1, Arm_R2);
            AddArmRegToArmReg(Arm_R1, Arm_R3, Arm_R1);
            StoreArmRegToArmRegPointer(Arm_R1, Arm_R0, 0);

            uint32_t TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
            bool bCheck = TargetPC <= m_CompilePC;
            UpdateCounters(m_RegWorkingSet, bCheck, true);

            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, bCheck ? CExitInfo::Normal : CExitInfo::Normal_NoSysCheck);
        }
        m_NextInstruction = END_BLOCK;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmRecompilerOps::ADDI()
{
    if (m_Opcode.rt == 0 || (m_Opcode.immediate == 0 && m_Opcode.rs == m_Opcode.rt))
    {
        return;
    }

    if (g_System->bFastSP())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
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
    else if (IsMapped(m_Opcode.rs))
    {
        ProtectGPR(m_Opcode.rs);
        Map_GPR_32bit(m_Opcode.rt, true, -1);
        AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmRecompilerOps::ADDIU()
{
    ADDI();
}

void CArmRecompilerOps::SLTI()
{
    UnMap_GPR(m_Opcode.rt, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SLTI, "R4300iOp32::SLTI");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SLTI, "R4300iOp::SLTI");
    }
}

void CArmRecompilerOps::SLTIU()
{
    UnMap_GPR(m_Opcode.rt, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SLTIU, "R4300iOp32::SLTIU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SLTIU, "R4300iOp::SLTIU");
    }
}

void CArmRecompilerOps::ANDI()
{
    UnMap_GPR(m_Opcode.rt, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::ANDI, "R4300iOp32::ANDI");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::ANDI, "R4300iOp::ANDI");
    }
}

void CArmRecompilerOps::ORI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_System->bFastSP() && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        //OrConstToX86Reg(m_Opcode.immediate, Map_MemoryStack(x86_Any, true));
        g_Notify->BreakPoint(__FILE__, __LINE__);
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
        ProtectGPR(m_Opcode.rs);
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, -1);
        }
        else
        {
            if (Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rt, IsSigned(m_Opcode.rs), -1);
            }
        }
        OrConstToArmReg(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rs), m_Opcode.immediate);
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
        OrConstToArmReg(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        //ResetX86Protection();
        //ResetMemoryStack();
    }
}

void CArmRecompilerOps::XORI()
{
    UnMap_GPR(m_Opcode.rt, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::XORI, "R4300iOp32::XORI");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::XORI, "R4300iOp::XORI");
    }
}

void CArmRecompilerOps::LUI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, ((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
}

void CArmRecompilerOps::DADDIU()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::DADDIU, "R4300iOp32::DADDIU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::DADDIU, "R4300iOp::DADDIU");
    }
}

void CArmRecompilerOps::LDL()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LDL, "R4300iOp32::LDL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LDL, "R4300iOp::LDL");
    }
}

void CArmRecompilerOps::LDR()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LDR, "R4300iOp32::LDR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LDR, "R4300iOp::LDR");
    }
}

void CArmRecompilerOps::LB()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset) ^ 3;
        Map_GPR_32bit(m_Opcode.rt, true, -1);
        LB_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address, true);
        return;
    }
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }

    ArmReg TempRegAddress;
    if (IsMapped(m_Opcode.base))
    {
        ProtectGPR(m_Opcode.base);
        TempRegAddress = Map_TempReg(Arm_Any, -1, false);
        AddConstToArmReg(TempRegAddress, GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.immediate);
    }
    else
    {
        TempRegAddress = Map_TempReg(Arm_Any, m_Opcode.base, false);
        AddConstToArmReg(TempRegAddress, (int16_t)m_Opcode.immediate);
    }
    if (g_System->bUseTlb())
    {
        ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
        ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
        ArmReg ReadMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_READMAP);
        LoadArmRegPointerToArmReg(TempReg, ReadMapReg, TempReg, 2);
        CompileReadTLBMiss(TempRegAddress, TempReg);
        XorConstToArmReg(TempRegAddress, 3);
        Map_GPR_32bit(m_Opcode.rt, true, -1);
        LoadArmRegPointerByteToArmReg(GetMipsRegMapLo(m_Opcode.rt), TempReg, TempRegAddress, 0);
        SignExtendByte(GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmRecompilerOps::LH()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LH, "R4300iOp32::LH");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LH, "R4300iOp::LH");
    }
}

void CArmRecompilerOps::LWL()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LWL, "R4300iOp32::LWL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LWL, "R4300iOp::LWL");
    }
}

void CArmRecompilerOps::LW()
{
    LW(true, false);
}

void CArmRecompilerOps::LW(bool ResultSigned, bool bRecordLLBit)
{
    if (m_Opcode.rt == 0) return;

    if (m_Opcode.base == 29 && g_System->bFastSP())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        /*Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        TempReg1 = Map_MemoryStack(x86_Any, true);
        MoveVariableDispToX86Reg((void *)((uint32_t)(int16_t)m_Opcode.offset), stdstr_f("%Xh", (int16_t)m_Opcode.offset).c_str(), GetMipsRegMapLo(m_Opcode.rt), TempReg1, 1);
        if (bRecordLLBit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }*/
    }
    else if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        LW_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address);
        if (bRecordLLBit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (g_System->bUseTlb())
    {
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }

        ArmReg TempRegAddress;
        if (IsMapped(m_Opcode.base))
        {
            ProtectGPR(m_Opcode.base);
            TempRegAddress = Map_TempReg(Arm_Any, -1, false);
            AddConstToArmReg(TempRegAddress, GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.immediate);
        }
        else
        {
            TempRegAddress = Map_TempReg(Arm_Any, m_Opcode.base, false);
            AddConstToArmReg(TempRegAddress, (int16_t)m_Opcode.immediate);
        }

        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        ArmReg TempRegValue = Arm_Unknown;
        ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
        ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
        ArmReg ReadMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_READMAP);
        LoadArmRegPointerToArmReg(TempReg, ReadMapReg, TempReg, 2);
        CompileReadTLBMiss(TempRegAddress, TempReg);
        Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        LoadArmRegPointerToArmReg(GetMipsRegMapLo(m_Opcode.rt), TempReg, TempRegAddress, 0);
        if (bRecordLLBit)
        {
            MoveConstToVariable(1, _LLBit, "LLBit");
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        /*if (IsMapped(m_Opcode.base))
        {
            ProtectGPR(m_Opcode.base);
            if (m_Opcode.offset != 0)
            {
                Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
                LeaSourceAndOffset(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rt, ResultSigned, m_Opcode.base);
            }
        }
        else
        {
            Map_GPR_32bit(m_Opcode.rt, ResultSigned, m_Opcode.base);
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
        }
        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 0x1FFFFFFF);
        MoveN64MemToX86reg(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rt));
        if (bRecordLLBit)
        {
            MoveConstToVariable(1, _LLBit, "LLBit");
        }*/
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        /*ResetX86Protection();
        ResetMemoryStack();*/
    }
}

void CArmRecompilerOps::LBU()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LBU, "R4300iOp32::LBU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LBU, "R4300iOp::LBU");
    }
}

void CArmRecompilerOps::LHU()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LHU, "R4300iOp32::LHU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LHU, "R4300iOp::LHU");
    }
}

void CArmRecompilerOps::LWR()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LWR, "R4300iOp32::LWR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LWR, "R4300iOp::LWR");
    }
}

void CArmRecompilerOps::LWU()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LWU, "R4300iOp32::LWU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LWU, "R4300iOp::LWU");
    }
}

void CArmRecompilerOps::SB()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SB, "R4300iOp32::SB");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SB, "R4300iOp::SB");
    }
}

void CArmRecompilerOps::SH()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SH, "R4300iOp32::SH");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SH, "R4300iOp::SH");
    }
}

void CArmRecompilerOps::SWL()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SWL, "R4300iOp32::SWL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SWL, "R4300iOp::SWL");
    }
}

void CArmRecompilerOps::SW()
{
    SW(false);
}

void CArmRecompilerOps::SW(bool bCheckLLbit)
{
    if (m_Opcode.base == 29 && g_System->bFastSP())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;

        if (bCheckLLbit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (IsConst(m_Opcode.rt))
        {
            SW_Const(GetMipsRegLo(m_Opcode.rt), Address);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SW_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SW_Register(Map_TempReg(Arm_Any, m_Opcode.rt, false), Address);
        }
        return;
    }

    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }

    if (g_System->bDelaySI() || g_System->bDelayDP())
    {
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    }

    if (g_System->bUseTlb())
    {
        if (IsMapped(m_Opcode.base)) { ProtectGPR(m_Opcode.base); }
        ArmReg TempRegAddress = Map_TempReg(Arm_Any, IsMapped(m_Opcode.base) ? -1 : m_Opcode.base, false);
        if (IsMapped(m_Opcode.base))
        {
            AddConstToArmReg(TempRegAddress, GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.immediate);
        }
        else
        {
            AddConstToArmReg(TempRegAddress, (int16_t)m_Opcode.immediate);
        }

        ArmReg TempRegValue = Arm_Unknown;
        if (g_System->bUseTlb())
        {
            ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
            ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
            ArmReg WriteMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_WRITEMAP);
            LoadArmRegPointerToArmReg(TempReg, WriteMapReg, TempReg, 2);
            CompileWriteTLBMiss(TempRegAddress, TempReg);
            if (bCheckLLbit)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            StoreArmRegToArmRegPointer(IsMapped(m_Opcode.rt) ? GetMipsRegMapLo(m_Opcode.rt) : Map_TempReg(Arm_Any, m_Opcode.rt, false), TempReg, TempRegAddress, 0);
            if (bCheckLLbit)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        if (bHaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        UnProtectGPR(m_Opcode.rt);

        if (m_Opcode.base != 0) { UnMap_GPR(m_Opcode.base, true); }
        if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }
        if (g_Settings->LoadBool(Game_32Bit))
        {
            CompileInterpterCall((void *)R4300iOp32::SW, "R4300iOp32::SW");
        }
        else
        {
            CompileInterpterCall((void *)R4300iOp::SW, "R4300iOp::SW");
        }
    }
}

void CArmRecompilerOps::SWR()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SWR, "R4300iOp32::SWR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SWR, "R4300iOp::SWR");
    }
}

void CArmRecompilerOps::SDL()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SDL, "R4300iOp32::SDL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SDL, "R4300iOp::SDL");
    }
}

void CArmRecompilerOps::SDR()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SDR, "R4300iOp32::SDR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SDR, "R4300iOp::SDR");
    }
}

void CArmRecompilerOps::CACHE()
{
    if (g_Settings->LoadDword(Game_SMM_Cache) == 0)
    {
        return;
    }
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }

    switch (m_Opcode.rt)
    {
    case 0:
    case 16:
        m_RegWorkingSet.BeforeCallDirect();
        MoveConstToArmReg(Arm_R3, (uint32_t)CRecompiler::Remove_Cache, "CRecompiler::Remove_Cache");
        MoveConstToArmReg(Arm_R2, (uint32_t)0x20);
        MoveVariableToArmReg(&_GPR[m_Opcode.base].UW[0], CRegName::GPR_Lo[m_Opcode.base], Arm_R1);
        MoveConstToArmReg(Arm_R0, (uint32_t)((int16_t)m_Opcode.offset));
        AddArmRegToArmReg(Arm_R1, Arm_R0, Arm_R1);
        MoveConstToArmReg(Arm_R0, (uint32_t)g_Recompiler, "g_Recompiler");
        CallFunction((void *)AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
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
        if (bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("cache: %d", m_Opcode.rt).c_str());
        }
    }
}

void CArmRecompilerOps::LL()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LL, "R4300iOp32::LL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LL, "R4300iOp::LL");
    }
}

void CArmRecompilerOps::LWC1()
{
    CompileCop1Test();
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;

        ArmReg TempRegValue = Map_TempReg(Arm_Any, -1, false);
        LW_KnownAddress(TempRegValue, Address);

        ArmReg FprReg = Map_Variable(CArmRegInfo::VARIABLE_FPR);
        ArmReg TempRegAddress = Map_TempReg(Arm_Any, -1, false);
        LoadArmRegPointerToArmReg(TempRegAddress, FprReg, (uint8_t)(m_Opcode.ft << 2));
        StoreArmRegToArmRegPointer(TempRegValue, TempRegAddress, 0);
        return;
    }
    ArmReg TempRegAddress;
    if (IsMapped(m_Opcode.base))
    {
        ProtectGPR(m_Opcode.base);
        TempRegAddress = Map_TempReg(Arm_Any, -1, false);
        AddConstToArmReg(TempRegAddress, GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.immediate);
    }
    else
    {
        TempRegAddress = Map_TempReg(Arm_Any, m_Opcode.base, false);
        AddConstToArmReg(TempRegAddress, (int16_t)m_Opcode.immediate);
    }

    ArmReg TempRegValue = Arm_Unknown;
    if (g_System->bUseTlb())
    {
        ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
        ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
        ArmReg ReadMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_READMAP);
        LoadArmRegPointerToArmReg(TempReg, ReadMapReg, TempReg, 2);
        CompileReadTLBMiss(TempRegAddress, TempReg);

        //12:	4408      	add	r0, r1
        //14:	ed90 7a00 	vldr	s14, [r0]

        TempRegValue = TempReg;
        LoadArmRegPointerToArmReg(TempRegValue, TempReg, TempRegAddress, 0);
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        AndConstToX86Reg(TempReg1, 0x1FFFFFFF);
        TempReg3 = Map_TempReg(x86_Any, -1, false);
        MoveN64MemToX86reg(TempReg3, TempReg1);
#endif
    }
    ArmReg FprReg = Map_Variable(CArmRegInfo::VARIABLE_FPR);
    LoadArmRegPointerToArmReg(TempRegAddress, FprReg, (uint8_t)(m_Opcode.ft << 2));
    StoreArmRegToArmRegPointer(TempRegValue, TempRegAddress, 0);
}

void CArmRecompilerOps::LDC1()
{
    CompileCop1Test();
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LDC1, "R4300iOp32::LDC1");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LDC1, "R4300iOp::LDC1");
    }
}

void CArmRecompilerOps::LD()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::LD, "R4300iOp32::LD");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::LD, "R4300iOp::LD");
    }
}

void CArmRecompilerOps::SC()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SC, "R4300iOp32::SC");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SC, "R4300iOp::SC");
    }
}

void CArmRecompilerOps::SWC1()
{
    CompileCop1Test();
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SWC1, "R4300iOp32::SWC1");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SWC1, "R4300iOp::SWC1");
    }
}

void CArmRecompilerOps::SDC1()
{
    CompileCop1Test();
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SDC1, "R4300iOp32::SDC1");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SDC1, "R4300iOp::SDC1");
    }
}

void CArmRecompilerOps::SD()
{
    if (m_Opcode.base != 0) { WriteBack_GPR(m_Opcode.base, false); }
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SD, "R4300iOp32::SD");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SD, "R4300iOp::SD");
    }
}

void CArmRecompilerOps::SPECIAL_SLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SLL, "R4300iOp32::SPECIAL_SLL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SLL, "R4300iOp::SPECIAL_SLL");
    }
}

void CArmRecompilerOps::SPECIAL_SRL()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SRL, "R4300iOp32::SPECIAL_SRL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SRL, "R4300iOp::SPECIAL_SRL");
    }
}

void CArmRecompilerOps::SPECIAL_SRA()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SRA, "R4300iOp32::SPECIAL_SRA");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SRA, "R4300iOp::SPECIAL_SRA");
    }
}

void CArmRecompilerOps::SPECIAL_SLLV()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SLLV, "R4300iOp32::SPECIAL_SLLV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SLLV, "R4300iOp::SPECIAL_SLLV");
    }
}

void CArmRecompilerOps::SPECIAL_SRLV()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SRLV, "R4300iOp32::SPECIAL_SRLV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SRLV, "R4300iOp::SPECIAL_SRLV");
    }
}

void CArmRecompilerOps::SPECIAL_SRAV()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SRAV, "R4300iOp32::SPECIAL_SRAV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SRAV, "R4300iOp::SPECIAL_SRAV");
    }
}

void CArmRecompilerOps::SPECIAL_JR()
{
    if (m_NextInstruction == NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
            {
                MoveArmRegToVariable(GetMipsRegMapLo(m_Opcode.rs), &R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                MoveVariableToArmReg(&_GPR[m_Opcode.rs].UW[0], CRegName::GPR_Lo[m_Opcode.rs], Arm_R0);
                MoveConstToArmReg(Arm_R1, (uint32_t)&R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");
                StoreArmRegToArmRegPointer(Arm_R0, Arm_R1, 0);
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
            ArmReg PCTempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
            MoveConstToArmReg(PCTempReg, (uint32_t)_PROGRAM_COUNTER, "PROGRAM_COUNTER");
            if (IsConst(m_Opcode.rs))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            else if (IsMapped(m_Opcode.rs))
            {
                StoreArmRegToArmRegPointer(GetMipsRegMapLo(m_Opcode.rs), PCTempReg, 0);
            }
            else
            {
                ArmReg ValueTempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, m_Opcode.rs, false);
                StoreArmRegToArmRegPointer(ValueTempReg, PCTempReg, 0);
                m_RegWorkingSet.SetArmRegProtected(ValueTempReg, false);
            }
            m_RegWorkingSet.SetArmRegProtected(PCTempReg, false);
        }
        m_NextInstruction = DO_DELAY_SLOT;
    }
    else if (m_NextInstruction == DELAY_SLOT_DONE)
    {
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            ArmReg PCTempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
            MoveConstToArmReg(PCTempReg, (uint32_t)_PROGRAM_COUNTER, "PROGRAM_COUNTER");
            if (IsConst(m_Opcode.rs))
            {
                ArmReg ValueTempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
                MoveConstToArmReg(ValueTempReg, GetMipsRegLo(m_Opcode.rs));
                StoreArmRegToArmRegPointer(ValueTempReg, PCTempReg, 0);
                m_RegWorkingSet.SetArmRegProtected(ValueTempReg, false);
            }
            else if (IsMapped(m_Opcode.rs))
            {
                StoreArmRegToArmRegPointer(GetMipsRegMapLo(m_Opcode.rs), PCTempReg, 0);
            }
            else
            {
                ArmReg ValueTempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, m_Opcode.rs, false);
                StoreArmRegToArmRegPointer(ValueTempReg, PCTempReg, 0);
                m_RegWorkingSet.SetArmRegProtected(ValueTempReg, false);
            }
            m_RegWorkingSet.SetArmRegProtected(PCTempReg, false);
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal);
            if (m_Section->m_JumpSection)
            {
                m_Section->GenerateSectionLinkage();
            }
        }
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_NextInstruction).c_str());
    }
}

void CArmRecompilerOps::SPECIAL_JALR()
{
    if (m_NextInstruction == NORMAL)
    {
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0) && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            if (IsKnown(m_Opcode.rs))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                return;
            }
            g_Notify->BreakPoint(__FILE__, __LINE__);
            MoveVariableToArmReg(&_GPR[m_Opcode.rs].UW[0], CRegName::GPR_Lo[m_Opcode.rs], Arm_R0);
            MoveConstToArmReg(Arm_R1, (uint32_t)_PROGRAM_COUNTER, "PROGRAM_COUNTER");
            StoreArmRegToArmRegPointer(Arm_R0, Arm_R1, 0);
        }
        UnMap_GPR(m_Opcode.rd, false);
        MoveConstToVariable(m_CompilePC + 8, &_GPR[m_Opcode.rd].UW[0], CRegName::GPR_Lo[m_Opcode.rd]);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            ArmReg TempRegVar = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
            MoveConstToArmReg(TempRegVar, (uint32_t)&R4300iOp::m_JumpToLocation, "R4300iOp::m_JumpToLocation");

            ArmReg TempRegRs = Arm_Unknown;
            if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
            {
                StoreArmRegToArmRegPointer(GetMipsRegMapLo(m_Opcode.rs), TempRegVar, 0);
            }
            else
            {
                TempRegRs = m_RegWorkingSet.Map_TempReg(Arm_Any, m_Opcode.rs, false);
                StoreArmRegToArmRegPointer(TempRegRs, TempRegVar, 0);
                m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
            }
            m_RegWorkingSet.SetArmRegProtected(TempRegVar, false);
            m_RegWorkingSet.WriteBackRegisters();
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
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            ArmReg ArmRegRs = ArmRegRs = IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs) ? GetMipsRegMapLo(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(Arm_Any, m_Opcode.rs, false);
            m_RegWorkingSet.SetArmRegProtected(ArmRegRs, true);
            ArmReg TempRegPC = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
            MoveConstToArmReg(TempRegPC, (uint32_t)_PROGRAM_COUNTER, "PROGRAM_COUNTER");
            StoreArmRegToArmRegPointer(ArmRegRs, TempRegPC, 0);
            m_RegWorkingSet.SetArmRegProtected(ArmRegRs, false);
            m_RegWorkingSet.SetArmRegProtected(TempRegPC, false);
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal);
            if (m_Section->m_JumpSection)
            {
                m_Section->GenerateSectionLinkage();
            }
        }
        m_NextInstruction = END_BLOCK;
    }
    else if (bHaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_NextInstruction).c_str());
    }
}

void CArmRecompilerOps::SPECIAL_SYSCALL()
{
    CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::DoSysCall);
    m_NextInstruction = END_BLOCK;
}

void CArmRecompilerOps::SPECIAL_MFLO()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_MFLO, "R4300iOp32::SPECIAL_MFLO");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_MFLO, "R4300iOp::SPECIAL_MFLO");
    }
}

void CArmRecompilerOps::SPECIAL_MTLO()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_MTLO, "R4300iOp32::SPECIAL_MTLO");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_MTLO, "R4300iOp::SPECIAL_MTLO");
    }
}

void CArmRecompilerOps::SPECIAL_MFHI()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_MFHI, "R4300iOp32::SPECIAL_MFHI");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_MFHI, "R4300iOp::SPECIAL_MFHI");
    }
}

void CArmRecompilerOps::SPECIAL_MTHI()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_MTHI, "R4300iOp32::SPECIAL_MTHI");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_MTHI, "R4300iOp::SPECIAL_MTHI");
    }
}

void CArmRecompilerOps::SPECIAL_DSLLV()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSLLV, "R4300iOp32::SPECIAL_DSLLV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSLLV, "R4300iOp::SPECIAL_DSLLV");
    }
}

void CArmRecompilerOps::SPECIAL_DSRLV()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSRLV, "R4300iOp32::SPECIAL_DSRLV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSRLV, "R4300iOp::SPECIAL_DSRLV");
    }
}

void CArmRecompilerOps::SPECIAL_DSRAV()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSRAV, "R4300iOp32::SPECIAL_DSRAV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSRAV, "R4300iOp::SPECIAL_DSRAV");
    }
}

void CArmRecompilerOps::SPECIAL_MULT()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_MULT, "R4300iOp32::SPECIAL_MULT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_MULT, "R4300iOp::SPECIAL_MULT");
    }
}

void CArmRecompilerOps::SPECIAL_MULTU()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_MULTU, "R4300iOp32::SPECIAL_MULTU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_MULTU, "R4300iOp::SPECIAL_MULTU");
    }
}

void CArmRecompilerOps::SPECIAL_DIV()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DIV, "R4300iOp32::SPECIAL_DIV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DIV, "R4300iOp::SPECIAL_DIV");
    }
}

void CArmRecompilerOps::SPECIAL_DIVU()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DIVU, "R4300iOp32::SPECIAL_DIVU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DIVU, "R4300iOp::SPECIAL_DIVU");
    }
}

void CArmRecompilerOps::SPECIAL_DMULT()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DMULT, "R4300iOp32::SPECIAL_DMULT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
    }
}

void CArmRecompilerOps::SPECIAL_DMULTU()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DMULTU, "R4300iOp32::SPECIAL_DMULTU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DMULTU, "R4300iOp::SPECIAL_DMULTU");
    }
}

void CArmRecompilerOps::SPECIAL_DDIV()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DDIV, "R4300iOp32::SPECIAL_DDIV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
    }
}

void CArmRecompilerOps::SPECIAL_DDIVU()
{
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DDIVU, "R4300iOp32::SPECIAL_DDIVU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
    }
}

void CArmRecompilerOps::SPECIAL_ADD()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_ADD, "R4300iOp32::SPECIAL_ADD");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_ADD, "R4300iOp::SPECIAL_ADD");
    }
}

void CArmRecompilerOps::SPECIAL_ADDU()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_ADDU, "R4300iOp32::SPECIAL_ADDU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_ADDU, "R4300iOp::SPECIAL_ADDU");
    }
}

void CArmRecompilerOps::SPECIAL_SUB()
{
    SPECIAL_SUBU();
}

void CArmRecompilerOps::SPECIAL_SUBU()
{
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
    else if (m_Opcode.rd == m_Opcode.rt)
    {
        ArmReg Reg = Map_TempReg(Arm_Any, m_Opcode.rt, false);
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        SubArmRegFromArmReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd), Reg);
    }
    else
    {
        bool rsMapped = IsMapped(m_Opcode.rs);
        ProtectGPR(m_Opcode.rs);

        Map_GPR_32bit(m_Opcode.rd, true, rsMapped ? -1 : m_Opcode.rs);
        ArmReg SouceReg = rsMapped ? GetMipsRegMapLo(m_Opcode.rs) : GetMipsRegMapLo(m_Opcode.rd);

        if (IsConst(m_Opcode.rt))
        {
            SubConstFromArmReg(GetMipsRegMapLo(m_Opcode.rd), SouceReg, GetMipsRegLo(m_Opcode.rt));
        }
        else
        {
            SubArmRegFromArmReg(GetMipsRegMapLo(m_Opcode.rd), SouceReg, IsMapped(m_Opcode.rt) ? GetMipsRegMapLo(m_Opcode.rt) : Map_TempReg(Arm_Any, m_Opcode.rt, false));
        }
    }

    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
        ResetMemoryStack();
#endif
    }
}

void CArmRecompilerOps::SPECIAL_AND()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_AND, "R4300iOp32::SPECIAL_AND");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_AND, "R4300iOp::SPECIAL_AND");
    }
}

void CArmRecompilerOps::SPECIAL_OR()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_OR, "R4300iOp32::SPECIAL_OR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_OR, "R4300iOp::SPECIAL_OR");
    }
}

void CArmRecompilerOps::SPECIAL_XOR()
{
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
                g_Notify->BreakPoint(__FILE__, __LINE__);
                CArmRecompilerOps::UnknownOpcode();
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
                if (m_Opcode.rt != m_Opcode.rd && m_Opcode.rs != m_Opcode.rd)
                {
                    Map_GPR_64bit(m_Opcode.rd, -1);
                    XorArmRegToArmReg(GetMipsRegMapHi(m_Opcode.rd),
                        Is32Bit(source1) ? Map_TempReg(Arm_Any, source1, true) : GetMipsRegMapHi(source1),
                        Is32Bit(source2) ? Map_TempReg(Arm_Any, source2, true) : GetMipsRegMapHi(source2));
                    XorArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source1), GetMipsRegMapLo(source2));
                }
                else
                {
                    Map_GPR_64bit(m_Opcode.rd, source1);
                    XorArmRegToArmReg(GetMipsRegMapHi(m_Opcode.rd), Is32Bit(source2) ? Map_TempReg(Arm_Any, source2, true) : GetMipsRegMapHi(source2));
                    XorArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, IsSigned(m_Opcode.rt) != IsSigned(m_Opcode.rs) ? true : IsSigned(m_Opcode.rt), -1);
                XorArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source1), GetMipsRegMapLo(source2));
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(MappedReg);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                CArmRecompilerOps::UnknownOpcode();
                /*uint32_t ConstHi, ConstLo;

                ConstHi = Is32Bit(ConstReg) ? (uint32_t)(GetMipsRegLo_S(ConstReg) >> 31) : GetMipsRegHi(ConstReg);
                ConstLo = GetMipsRegLo(ConstReg);
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if (ConstHi != 0) { XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), ConstHi); }
                if (ConstLo != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), ConstLo); }
                */
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, IsSigned(m_Opcode.rt) != IsSigned(m_Opcode.rs) ? true : IsSigned(MappedReg), MappedReg);
                XorConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), Value);
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
                g_Notify->BreakPoint(__FILE__, __LINE__);
                CArmRecompilerOps::UnknownOpcode();
                /*Value = GetMipsReg(KnownReg);
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }*/
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                Value = IsSigned(KnownReg) ? GetMipsRegLo_S(KnownReg) : GetMipsRegLo(KnownReg);
            }
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
            XorConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), dwValue);
        }
        else
        {
            ProtectGPR(KnownReg);
            if (m_Opcode.rd == KnownReg)
            {
                ArmReg TempReg = Arm_Any;
                if (Is64Bit(KnownReg))
                {
                    TempReg = Map_TempReg(Arm_Any, UnknownReg, true);
                    XorArmRegToArmReg(GetMipsRegMapHi(m_Opcode.rd), TempReg);
                    m_RegWorkingSet.SetArmRegProtected(TempReg, false);
                }
                XorArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rd), Map_TempReg(TempReg, UnknownReg, false));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                XorArmRegToArmReg(GetMipsRegMapHi(m_Opcode.rd), Is32Bit(KnownReg) ? Map_TempReg(Arm_Any, KnownReg, true) : GetMipsRegMapHi(KnownReg));
                XorArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
            }
        }
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs == m_Opcode.rd ? m_Opcode.rs : m_Opcode.rt);
        ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs == m_Opcode.rd ? m_Opcode.rt : m_Opcode.rs, true);
        XorArmRegToArmReg(GetMipsRegMapHi(m_Opcode.rd), TempReg);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        Map_TempReg(TempReg, m_Opcode.rs == m_Opcode.rd ? m_Opcode.rt : m_Opcode.rs, false);
        XorArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rd), TempReg);
    }
}

void CArmRecompilerOps::SPECIAL_NOR()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_NOR, "R4300iOp32::SPECIAL_NOR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_NOR, "R4300iOp::SPECIAL_NOR");
    }
}

void CArmRecompilerOps::SPECIAL_SLT()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    bool useRdReg = m_Opcode.rd != m_Opcode.rt && m_Opcode.rd != m_Opcode.rs;
    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                if (bHaveDebugger())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                UnMap_GPR(m_Opcode.rd, true);
                if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
                if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
                if (g_Settings->LoadBool(Game_32Bit))
                {
                    CompileInterpterCall((void *)R4300iOp32::SPECIAL_SLT, "R4300iOp32::SPECIAL_SLT");
                }
                else
                {
                    CompileInterpterCall((void *)R4300iOp::SPECIAL_SLT, "R4300iOp::SPECIAL_SLT");
                }
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
            ProtectGPR(m_Opcode.rs);
            ProtectGPR(m_Opcode.rt);
            if (useRdReg)
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
            }
            ArmReg TempResult = useRdReg ? Arm_Unknown : Map_TempReg(Arm_Any, -1, false);
            CompareArmRegToArmReg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
            IfBlock(ItMask_E, ArmBranch_LessThan);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempResult, (uint16_t)1);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempResult, (uint16_t)0);
            if (!useRdReg)
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
                AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), TempResult, 0);
                m_RegWorkingSet.SetArmRegProtected(TempResult, false);
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(MappedReg);
            Map_GPR_32bit(m_Opcode.rd, false, -1);
            CompareArmRegToConst(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
            IfBlock(ItMask_E, m_Opcode.rt == MappedReg ? ArmBranch_GreaterThan : ArmBranch_LessThan);
            MoveConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), (uint16_t)1);
            MoveConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), (uint16_t)0);
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsMapped(KnownReg))
        {
            ProtectGPR(KnownReg);
        }

        if (!g_System->b32BitCore())
        {
            if (useRdReg)
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
            }
            ArmReg UnknownArmReg = Map_TempReg(Arm_Any, UnknownReg, true);
            if (IsConst(KnownReg))
            {
                CompareArmRegToConst(UnknownArmReg, Is32Bit(KnownReg) ? (IsSigned(KnownReg) ? (GetMipsRegLo_S(KnownReg) >> 31) : 0) : GetMipsRegHi(KnownReg));
            }
            else
            {
                CompareArmRegToArmReg(UnknownArmReg, Is32Bit(KnownReg) ? Map_TempReg(Arm_Any, KnownReg, true) : GetMipsRegMapHi(KnownReg));
            }
            uint8_t * JumpLow = *g_RecompPos;
            BranchLabel8(ArmBranch_Equal, "Low Compare");

            IfBlock(ItMask_E, KnownReg == m_Opcode.rt ? ArmBranch_LessThan : ArmBranch_GreaterThan);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : UnknownArmReg, (uint16_t)1);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : UnknownArmReg, (uint16_t)0);

            uint8_t * JumpContinue = *g_RecompPos;
            BranchLabel8(ArmBranch_Always, "Continue");

            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(JumpLow, *g_RecompPos);
            m_RegWorkingSet.SetArmRegProtected(UnknownArmReg, false);
            Map_TempReg(UnknownArmReg, UnknownReg, false);
            if (IsConst(KnownReg))
            {
                CompareArmRegToConst(UnknownArmReg, GetMipsRegLo(KnownReg));
            }
            else
            {
                CompareArmRegToArmReg(UnknownArmReg, GetMipsRegMapLo(KnownReg));
            }
            IfBlock(ItMask_E, KnownReg == m_Opcode.rt ? ArmBranch_LessThan : ArmBranch_GreaterThan);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : UnknownArmReg, (uint16_t)1);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : UnknownArmReg, (uint16_t)0);

            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(JumpContinue, *g_RecompPos);
            if (!useRdReg)
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
                AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), UnknownArmReg, 0);
            }
        }
        else
        {
            ArmReg TempResult = useRdReg ? Arm_Unknown : Map_TempReg(Arm_Any, -1, false);
            if (useRdReg)
            {
                Map_GPR_32bit(m_Opcode.rd, false, UnknownReg);
            }
            if (IsConst(KnownReg))
            {
                ArmReg UnknownArmReg = useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : Map_TempReg(Arm_Any, UnknownReg, false);
                CompareArmRegToConst(UnknownArmReg, GetMipsRegLo(KnownReg));
                IfBlock(ItMask_E, KnownReg == m_Opcode.rt ? ArmBranch_LessThan : ArmBranch_GreaterThan);
            }
            else
            {
                ArmReg UnknownArmReg = useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : Map_TempReg(Arm_Any, UnknownReg, false);
                CompareArmRegToArmReg(KnownReg == m_Opcode.rs ? GetMipsRegMapLo(KnownReg) : UnknownArmReg, KnownReg == m_Opcode.rs ? UnknownArmReg : GetMipsRegMapLo(KnownReg));
                IfBlock(ItMask_E, ArmBranch_LessThan);
            }
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempResult, (uint16_t)1);
            MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempResult, (uint16_t)0);
            if (!useRdReg)
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
                AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), TempResult, 0);
                m_RegWorkingSet.SetArmRegProtected(TempResult, false);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        if (useRdReg)
        {
            Map_GPR_32bit(m_Opcode.rd, false, m_Opcode.rt);
        }
        ArmReg TempReg = Map_TempReg(Arm_Any, m_Opcode.rs, false);
        CompareArmRegToArmReg(TempReg, useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : Map_TempReg(Arm_Any, m_Opcode.rt, false));
        IfBlock(ItMask_E, ArmBranch_LessThan);
        MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempReg, (uint16_t)1);
        MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempReg, (uint16_t)0);
        if (!useRdReg)
        {
            Map_GPR_32bit(m_Opcode.rd, false, -1);
            AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), TempReg, 0);
        }
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    }
    else
    {
        if (useRdReg)
        {
            Map_GPR_32bit(m_Opcode.rd, false, -1);
        }
        ArmReg TempRegRt = Map_TempReg(Arm_Any, m_Opcode.rt, true);
        ArmReg TempRegRs = Map_TempReg(Arm_Any, m_Opcode.rs, true);
        CompareArmRegToArmReg(TempRegRs, TempRegRt);
        uint8_t * JumpLow = *g_RecompPos;
        BranchLabel8(ArmBranch_Equal, "Low Compare");
        IfBlock(ItMask_E, ArmBranch_LessThan);
        MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempRegRt, (uint16_t)1);
        MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempRegRt, (uint16_t)0);
        uint8_t * JumpContinue = *g_RecompPos;
        BranchLabel8(ArmBranch_Always, "Continue");
        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(JumpLow, *g_RecompPos);
        m_RegWorkingSet.SetArmRegProtected(TempRegRt, false);
        Map_TempReg(TempRegRt, m_Opcode.rt, false);
        m_RegWorkingSet.SetArmRegProtected(TempRegRs, false);
        Map_TempReg(TempRegRs, m_Opcode.rs, false);
        CompareArmRegToArmReg(TempRegRs, TempRegRt);
        IfBlock(ItMask_E, ArmBranch_LessThan);
        MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempRegRt, (uint16_t)1);
        MoveConstToArmReg(useRdReg ? GetMipsRegMapLo(m_Opcode.rd) : TempRegRt, (uint16_t)0);
        CPU_Message("");
        CPU_Message("      Continue:");
        SetJump8(JumpContinue, *g_RecompPos);
        if (!useRdReg)
        {
            Map_GPR_32bit(m_Opcode.rd, false, -1);
            AddConstToArmReg(GetMipsRegMapLo(m_Opcode.rd), TempRegRt, 0);
        }
    }
}

void CArmRecompilerOps::SPECIAL_SLTU()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_SLTU, "R4300iOp32::SPECIAL_SLTU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_SLTU, "R4300iOp::SPECIAL_SLTU");
    }
}

void CArmRecompilerOps::SPECIAL_DADD()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DADD, "R4300iOp32::SPECIAL_DADD");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DADD, "R4300iOp::SPECIAL_DADD");
    }
}

void CArmRecompilerOps::SPECIAL_DADDU()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DADDU, "R4300iOp32::SPECIAL_DADDU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DADDU, "R4300iOp::SPECIAL_DADDU");
    }
}

void CArmRecompilerOps::SPECIAL_DSUB()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSUB, "R4300iOp32::SPECIAL_DSUB");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSUB, "R4300iOp::SPECIAL_DSUB");
    }
}

void CArmRecompilerOps::SPECIAL_DSUBU()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rs != 0) { WriteBack_GPR(m_Opcode.rs, false); }
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSUBU, "R4300iOp32::SPECIAL_DSUBU");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSUBU, "R4300iOp::SPECIAL_DSUBU");
    }
}

void CArmRecompilerOps::SPECIAL_DSLL()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSLL, "R4300iOp32::SPECIAL_DSLL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSLL, "R4300iOp::SPECIAL_DSLL");
    }
}

void CArmRecompilerOps::SPECIAL_DSRL()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSRL, "R4300iOp32::SPECIAL_DSRL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSRL, "R4300iOp::SPECIAL_DSRL");
    }
}

void CArmRecompilerOps::SPECIAL_DSRA()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSRA, "R4300iOp32::SPECIAL_DSRA");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSRA, "R4300iOp::SPECIAL_DSRA");
    }
}

void CArmRecompilerOps::SPECIAL_DSLL32()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSLL32, "R4300iOp32::SPECIAL_DSLL32");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSLL32, "R4300iOp::SPECIAL_DSLL32");
    }
}

void CArmRecompilerOps::SPECIAL_DSRL32()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSRL32, "R4300iOp32::SPECIAL_DSRL32");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSRL32, "R4300iOp::SPECIAL_DSRL32");
    }
}

void CArmRecompilerOps::SPECIAL_DSRA32()
{
    UnMap_GPR(m_Opcode.rd, true);
    if (m_Opcode.rt != 0) { WriteBack_GPR(m_Opcode.rt, false); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::SPECIAL_DSRA32, "R4300iOp32::SPECIAL_DSRA32");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::SPECIAL_DSRA32, "R4300iOp::SPECIAL_DSRA32");
    }
}

/************************** COP0 functions **************************/
void CArmRecompilerOps::COP0_MF()
{
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }

    switch (m_Opcode.rd)
    {
    case 9: //Count
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP0_MF, "R4300iOp32::COP0_MF");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP0_MF, "R4300iOp::COP0_MF");
    }
}

void CArmRecompilerOps::COP0_MT()
{
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }

    switch (m_Opcode.rd)
    {
    case 0: //Index
    case 2: //EntryLo0
    case 3: //EntryLo1
    case 4: //Context
    case 5: //PageMask
    case 10: //Entry Hi
    case 12: //Status
    case 13: //cause
    case 14: //EPC
    case 16: //Config
    case 18: //WatchLo
    case 19: //WatchHi
    case 28: //Tag lo
    case 29: //Tag Hi
    case 30: //ErrEPC
        if (g_Settings->LoadBool(Game_32Bit))
        {
            CompileInterpterCall((void *)R4300iOp32::COP0_MT, "R4300iOp32::COP0_MT");
        }
        else
        {
            CompileInterpterCall((void *)R4300iOp::COP0_MT, "R4300iOp::COP0_MT");
        }
        break;
    case 6: //Wired
    case 9: //Count
    case 11: //Compare
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        if (g_Settings->LoadBool(Game_32Bit))
        {
            CompileInterpterCall((void *)R4300iOp32::COP0_MT, "R4300iOp32::COP0_MT");
        }
        else
        {
            CompileInterpterCall((void *)R4300iOp::COP0_MT, "R4300iOp::COP0_MT");
        }
        break;
    default:
        UnknownOpcode();
    }
}

void CArmRecompilerOps::COP0_CO_TLBR()
{
    if (!g_System->bUseTlb()) { return; }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP0_CO_TLBR, "R4300iOp32::COP0_CO_TLBR");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP0_CO_TLBR, "R4300iOp::COP0_CO_TLBR");
    }
}

void CArmRecompilerOps::COP0_CO_TLBWI()
{
    if (!g_System->bUseTlb()) { return; }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP0_CO_TLBWI, "R4300iOp32::COP0_CO_TLBWI");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP0_CO_TLBWI, "R4300iOp::COP0_CO_TLBWI");
    }
}

void CArmRecompilerOps::COP0_CO_TLBWR()
{
    if (!g_System->bUseTlb()) { return; }

    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    UpdateCounters(m_RegWorkingSet, false, true);
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToArmReg(Arm_R0, (uint32_t)g_SystemTimer, "g_SystemTimer");
    CallFunction((void *)AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");

    MoveConstToArmReg(Arm_R2, (uint32_t)true, "true");
    MoveVariableToArmReg(&g_Reg->RANDOM_REGISTER, "RANDOM_REGISTER", Arm_R1);
    AndConstToArmReg(Arm_R1, Arm_R1, 0x1F);
    MoveConstToArmReg(Arm_R0, (uint32_t)g_TLB, "g_TLB");
    CallFunction((void *)AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
    m_RegWorkingSet.AfterCallDirect();
}

void CArmRecompilerOps::COP0_CO_TLBP()
{
    if (!g_System->bUseTlb()) { return; }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP0_CO_TLBP, "R4300iOp32::COP0_CO_TLBP");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP0_CO_TLBP, "R4300iOp::COP0_CO_TLBP");
    }
}

void arm_compiler_COP0_CO_ERET()
{
    if ((g_Reg->STATUS_REGISTER & STATUS_ERL) != 0)
    {
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

void CArmRecompilerOps::COP0_CO_ERET()
{
    m_RegWorkingSet.WriteBackRegisters();
    CallFunction((void *)arm_compiler_COP0_CO_ERET, "arm_compiler_COP0_CO_ERET");

    UpdateCounters(m_RegWorkingSet, true, true);
    CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal);
    m_NextInstruction = END_BLOCK;
}

/************************** COP1 functions **************************/
void CArmRecompilerOps::COP1_MF()
{
    CompileCop1Test();
    UnMap_GPR(m_Opcode.rt, false);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_MF, "R4300iOp32::COP1_MF");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_MF, "R4300iOp::COP1_MF");
    }
}

void CArmRecompilerOps::COP1_DMF()
{
    CompileCop1Test();
    UnMap_GPR(m_Opcode.rt, false);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_DMF, "R4300iOp32::COP1_DMF");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_DMF, "R4300iOp::COP1_DMF");
    }
}

void CArmRecompilerOps::COP1_CF()
{
    CompileCop1Test();

    UnMap_GPR(m_Opcode.rt, false);
    if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
    {
        UnknownOpcode();
        return;
    }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_CF, "R4300iOp32::COP1_CF");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_CF, "R4300iOp::COP1_CF");
    }
}

void CArmRecompilerOps::COP1_MT()
{
    CompileCop1Test();
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_MT, "R4300iOp32::COP1_MT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_MT, "R4300iOp::COP1_MT");
    }
}

void CArmRecompilerOps::COP1_DMT()
{
    CompileCop1Test();
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }

    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_DMT, "R4300iOp32::COP1_DMT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_DMT, "R4300iOp::COP1_DMT");
    }
}

void CArmRecompilerOps::COP1_CT()
{
    CompileCop1Test();
    if (m_Opcode.rt != 0) { UnMap_GPR(m_Opcode.rt, true); }

    if (m_Opcode.fs != 31)
    {
        UnknownOpcode();
        return;
    }
    UnMap_GPR(m_Opcode.rt, true);
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_CT, "R4300iOp32::COP1_CT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_CT, "R4300iOp::COP1_CT");
    }
    m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
}

void CArmRecompilerOps::COP1_S_ADD()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_ADD, "R4300iOp32::COP1_S_ADD");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_ADD, "R4300iOp::COP1_S_ADD");
    }
}

void CArmRecompilerOps::COP1_S_SUB()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_SUB, "R4300iOp32::COP1_S_SUB");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_SUB, "R4300iOp::COP1_S_SUB");
    }
}

void CArmRecompilerOps::COP1_S_MUL()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    ArmReg FprReg = Map_Variable(CArmRegInfo::VARIABLE_FPR);
    ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    LoadArmRegPointerToArmReg(TempReg, FprReg, (uint8_t)(m_Opcode.fs << 2));
    LoadArmRegPointerToFloatReg(TempReg, Arm_S14, 0);
    LoadArmRegPointerToArmReg(TempReg, FprReg, (uint8_t)(m_Opcode.ft << 2));
    LoadArmRegPointerToFloatReg(TempReg, Arm_S15, 0);
    MulF32(Arm_S0, Arm_S14, Arm_S15);
    LoadArmRegPointerToArmReg(TempReg, FprReg, (uint8_t)(m_Opcode.fd << 2));
    StoreFloatRegToArmRegPointer(Arm_S0, TempReg, 0);
}

void CArmRecompilerOps::COP1_S_DIV()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_DIV, "R4300iOp32::COP1_S_DIV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_DIV, "R4300iOp::COP1_S_DIV");
    }
}

void CArmRecompilerOps::COP1_S_ABS()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_ABS, "R4300iOp32::COP1_S_ABS");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_ABS, "R4300iOp::COP1_S_ABS");
    }
}

void CArmRecompilerOps::COP1_S_NEG()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_NEG, "R4300iOp32::COP1_S_NEG");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_NEG, "R4300iOp::COP1_S_NEG");
    }
}

void CArmRecompilerOps::COP1_S_SQRT()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_SQRT, "R4300iOp32::COP1_S_SQRT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_SQRT, "R4300iOp::COP1_S_SQRT");
    }
}

void CArmRecompilerOps::COP1_S_MOV()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_MOV, "R4300iOp32::COP1_S_MOV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_MOV, "R4300iOp::COP1_S_MOV");
    }
}

void CArmRecompilerOps::COP1_S_ROUND_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_ROUND_L, "R4300iOp32::COP1_S_ROUND_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_ROUND_L, "R4300iOp::COP1_S_ROUND_L");
    }
}

void CArmRecompilerOps::COP1_S_TRUNC_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_TRUNC_L, "R4300iOp32::COP1_S_TRUNC_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_TRUNC_L, "R4300iOp::COP1_S_TRUNC_L");
    }
}

void CArmRecompilerOps::COP1_S_CEIL_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_CEIL_L, "R4300iOp32::COP1_S_CEIL_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_CEIL_L, "R4300iOp::COP1_S_CEIL_L");
    }
}

void CArmRecompilerOps::COP1_S_FLOOR_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_FLOOR_L, "R4300iOp32::COP1_S_FLOOR_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_FLOOR_L, "R4300iOp::COP1_S_FLOOR_L");
    }
}

void CArmRecompilerOps::COP1_S_ROUND_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_ROUND_W, "R4300iOp32::COP1_S_ROUND_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_ROUND_W, "R4300iOp::COP1_S_ROUND_W");
    }
}

void CArmRecompilerOps::COP1_S_TRUNC_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_TRUNC_W, "R4300iOp32::COP1_S_TRUNC_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_TRUNC_W, "R4300iOp::COP1_S_TRUNC_W");
    }
}

void CArmRecompilerOps::COP1_S_CEIL_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_CEIL_W, "R4300iOp32::COP1_S_CEIL_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_CEIL_W, "R4300iOp::COP1_S_CEIL_W");
    }
}

void CArmRecompilerOps::COP1_S_FLOOR_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_FLOOR_W, "R4300iOp32::COP1_S_FLOOR_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_FLOOR_W, "R4300iOp::COP1_S_FLOOR_W");
    }
}

void CArmRecompilerOps::COP1_S_CVT_D()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_CVT_D, "R4300iOp32::COP1_S_CVT_D");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_CVT_D, "R4300iOp::COP1_S_CVT_D");
    }
}

void CArmRecompilerOps::COP1_S_CVT_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_CVT_W, "R4300iOp32::COP1_S_CVT_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_CVT_W, "R4300iOp::COP1_S_CVT_W");
    }
}

void CArmRecompilerOps::COP1_S_CVT_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_CVT_L, "R4300iOp32::COP1_S_CVT_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_CVT_L, "R4300iOp::COP1_S_CVT_L");
    }
}

void CArmRecompilerOps::COP1_S_CMP()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_S_CMP, "R4300iOp32::COP1_S_CMP");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_S_CMP, "R4300iOp::COP1_S_CMP");
    }
}

void CArmRecompilerOps::COP1_D_ADD()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_ADD, "R4300iOp32::COP1_D_ADD");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_ADD, "R4300iOp::COP1_D_ADD");
    }
}

void CArmRecompilerOps::COP1_D_SUB()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_SUB, "R4300iOp32::COP1_D_SUB");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_SUB, "R4300iOp::COP1_D_SUB");
    }
}

void CArmRecompilerOps::COP1_D_MUL()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_MUL, "R4300iOp32::COP1_D_MUL");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_MUL, "R4300iOp::COP1_D_MUL");
    }
}

void CArmRecompilerOps::COP1_D_DIV()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_DIV, "R4300iOp32::COP1_D_DIV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_DIV, "R4300iOp::COP1_D_DIV");
    }
}

void CArmRecompilerOps::COP1_D_ABS()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_ABS, "R4300iOp32::COP1_D_ABS");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_ABS, "R4300iOp::COP1_D_ABS");
    }
}

void CArmRecompilerOps::COP1_D_NEG()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_NEG, "R4300iOp32::COP1_D_NEG");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_NEG, "R4300iOp::COP1_D_NEG");
    }
}

void CArmRecompilerOps::COP1_D_SQRT()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_SQRT, "R4300iOp32::COP1_D_SQRT");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_SQRT, "R4300iOp::COP1_D_SQRT");
    }
}

void CArmRecompilerOps::COP1_D_MOV()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_MOV, "R4300iOp32::COP1_D_MOV");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_MOV, "R4300iOp::COP1_D_MOV");
    }
}

void CArmRecompilerOps::COP1_D_ROUND_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_ROUND_L, "R4300iOp32::COP1_D_ROUND_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_ROUND_L, "R4300iOp::COP1_D_ROUND_L");
    }
}

void CArmRecompilerOps::COP1_D_TRUNC_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_TRUNC_L, "R4300iOp32::COP1_D_TRUNC_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_TRUNC_L, "R4300iOp::COP1_D_TRUNC_L");
    }
}

void CArmRecompilerOps::COP1_D_CEIL_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_CEIL_L, "R4300iOp32::COP1_D_CEIL_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_CEIL_L, "R4300iOp::COP1_D_CEIL_L");
    }
}

void CArmRecompilerOps::COP1_D_FLOOR_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_FLOOR_L, "R4300iOp32::COP1_D_FLOOR_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_FLOOR_L, "R4300iOp::COP1_D_FLOOR_L");
    }
}

void CArmRecompilerOps::COP1_D_ROUND_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_ROUND_W, "R4300iOp32::COP1_D_ROUND_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_ROUND_W, "R4300iOp::COP1_D_ROUND_W");
    }
}

void CArmRecompilerOps::COP1_D_TRUNC_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_TRUNC_W, "R4300iOp32::COP1_D_TRUNC_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_TRUNC_W, "R4300iOp::COP1_D_TRUNC_W");
    }
}

void CArmRecompilerOps::COP1_D_CEIL_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_CEIL_W, "R4300iOp32::COP1_D_CEIL_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_CEIL_W, "R4300iOp::COP1_D_CEIL_W");
    }
}

void CArmRecompilerOps::COP1_D_FLOOR_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_FLOOR_W, "R4300iOp32::COP1_D_FLOOR_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_FLOOR_W, "R4300iOp::COP1_D_FLOOR_W");
    }
}

void CArmRecompilerOps::COP1_D_CVT_S()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_CVT_S, "R4300iOp32::COP1_D_CVT_S");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_CVT_S, "R4300iOp::COP1_D_CVT_S");
    }
}

void CArmRecompilerOps::COP1_D_CVT_W()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_CVT_W, "R4300iOp32::COP1_D_CVT_W");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_CVT_W, "R4300iOp::COP1_D_CVT_W");
    }
}

void CArmRecompilerOps::COP1_D_CVT_L()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_CVT_L, "R4300iOp32::COP1_D_CVT_L");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_CVT_L, "R4300iOp::COP1_D_CVT_L");
    }
}

void CArmRecompilerOps::COP1_D_CMP()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_D_CMP, "R4300iOp32::COP1_D_CMP");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_D_CMP, "R4300iOp::COP1_D_CMP");
    }
}

void CArmRecompilerOps::COP1_W_CVT_S()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_W_CVT_S, "R4300iOp32::COP1_W_CVT_S");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_W_CVT_S, "R4300iOp::COP1_W_CVT_S");
    }
}

void CArmRecompilerOps::COP1_W_CVT_D()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_W_CVT_D, "R4300iOp32::COP1_W_CVT_D");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_W_CVT_D, "R4300iOp::COP1_W_CVT_D");
    }
}

void CArmRecompilerOps::COP1_L_CVT_S()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_L_CVT_S, "R4300iOp32::COP1_L_CVT_S");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_L_CVT_S, "R4300iOp::COP1_L_CVT_S");
    }
}

void CArmRecompilerOps::COP1_L_CVT_D()
{
    CompileCop1Test();
    if (g_Settings->LoadBool(Game_32Bit))
    {
        CompileInterpterCall((void *)R4300iOp32::COP1_L_CVT_D, "R4300iOp32::COP1_L_CVT_D");
    }
    else
    {
        CompileInterpterCall((void *)R4300iOp::COP1_L_CVT_D, "R4300iOp::COP1_L_CVT_D");
    }
}

void CArmRecompilerOps::UnknownOpcode()
{
    m_RegWorkingSet.WriteBackRegisters();
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem)
    {
        MoveConstToArmReg(Arm_R0, (uint32_t)g_BaseSystem, "g_BaseSystem");
        CallFunction(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }

    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    CallFunction((void *)R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
    ExitCodeBlock();
    if (m_NextInstruction == NORMAL) { m_NextInstruction = END_BLOCK; }
}

void CArmRecompilerOps::EnterCodeBlock()
{
    PushArmReg(ArmPushPop_R2 | ArmPushPop_R3 | ArmPushPop_R4 | ArmPushPop_R5 | ArmPushPop_R6 | ArmPushPop_R7 | ArmPushPop_R8 | ArmPushPop_R9 | ArmPushPop_R10 | ArmPushPop_R11 | ArmPushPop_R12 | ArmPushPop_LR);
}

void CArmRecompilerOps::ExitCodeBlock()
{
    if (g_SyncSystem)
    {
        MoveConstToArmReg(Arm_R0, (uint32_t)g_BaseSystem, "g_BaseSystem");
        CallFunction(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }
    PopArmReg(ArmPushPop_R2 | ArmPushPop_R3 | ArmPushPop_R4 | ArmPushPop_R5 | ArmPushPop_R6 | ArmPushPop_R7 | ArmPushPop_R8 | ArmPushPop_R9 | ArmPushPop_R10 | ArmPushPop_R11 | ArmPushPop_R12 | ArmPushPop_PC);
}

void CArmRecompilerOps::CompileExitCode()
{
    for (EXIT_LIST::iterator ExitIter = m_ExitInfo.begin(); ExitIter != m_ExitInfo.end(); ExitIter++)
    {
        CPU_Message("");
        CPU_Message("      $Exit_%d", ExitIter->ID);
        SetJump20(ExitIter->JumpLoc, (uint32_t *)*g_RecompPos);
        m_NextInstruction = ExitIter->NextInstruction;
        CompileExit((uint32_t)-1, ExitIter->TargetPC, ExitIter->ExitRegSet, ExitIter->reason);
    }
}

void CArmRecompilerOps::CompileCop1Test()
{
    if (m_RegWorkingSet.GetFpuBeenUsed())
        return;

    ArmReg TempReg1 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    ArmReg TempReg2 = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    MoveVariableToArmReg(&g_Reg->STATUS_REGISTER, "STATUS_REGISTER", TempReg1);
    MoveConstToArmReg(TempReg2, STATUS_CU1, "STATUS_CU1");
    AndArmRegToArmReg(TempReg1, TempReg1, TempReg2);
    CompareArmRegToConst(TempReg1, 0);
    m_RegWorkingSet.SetArmRegProtected(TempReg1, false);
    m_RegWorkingSet.SetArmRegProtected(TempReg2, false);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::COP1_Unuseable, ArmBranch_Equal);
    m_RegWorkingSet.SetFpuBeenUsed(true);
}

void CArmRecompilerOps::CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter)
{
    MoveConstToVariable(ProgramCounter, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
    RegSet.WriteBackRegisters();
    UpdateCounters(RegSet, false, true);
    CallFunction(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
    MoveConstToArmReg(Arm_R0, (uint32_t)g_SystemTimer);
    CallFunction(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
    CPU_Message("CompileSystemCheck 3");
    CompileSystemCheck((uint32_t)-1, RegSet);
    if (g_SyncSystem)
    {
        MoveConstToArmReg(Arm_R0, (uint32_t)g_BaseSystem);
        CallFunction(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }
}

bool CArmRecompilerOps::SetupRegisterForLoop(CCodeBlock * BlockInfo, const CRegInfo & RegSet)
{
    CRegInfo OriginalReg = m_RegWorkingSet;
    if (!LoopAnalysis(BlockInfo, m_Section).SetupRegisterForLoop())
    {
        return false;
    }
    for (int i = 1; i < 32; i++)
    {
        if (OriginalReg.GetMipsRegState(i) != RegSet.GetMipsRegState(i))
        {
            UnMap_GPR(i, true);
        }
    }
    return true;
}

void CArmRecompilerOps::OutputRegisterState(const CRegInfo & SyncTo, const CRegInfo & CurrentSet) const
{
    if (!CDebugSettings::bRecordRecompilerAsm())
    {
        return;
    }

    for (uint32_t i = 0; i < 16; i++)
    {
        stdstr synctoreg, currentreg;

        if (SyncTo.GetArmRegMapped((ArmReg)i) == CArmRegInfo::GPR_Mapped)
        {
            for (uint32_t count = 1; count < 32; count++)
            {
                if (!SyncTo.IsMapped(count))
                {
                    continue;
                }

                if (SyncTo.Is64Bit(count) && SyncTo.GetMipsRegMapHi(count) == (ArmReg)i)
                {
                    synctoreg = CRegName::GPR_Hi[count];
                    break;
                }
                if (SyncTo.GetMipsRegMapLo(count) == (ArmReg)i)
                {
                    synctoreg = CRegName::GPR_Lo[count];
                    break;
                }
            }
        }

        if (CurrentSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::GPR_Mapped)
        {
            for (uint32_t count = 1; count < 32; count++)
            {
                if (!CurrentSet.IsMapped(count))
                {
                    continue;
                }

                if (CurrentSet.Is64Bit(count) && CurrentSet.GetMipsRegMapHi(count) == (ArmReg)i)
                {
                    currentreg = CRegName::GPR_Hi[count];
                    break;
                }
                if (CurrentSet.GetMipsRegMapLo(count) == (ArmReg)i)
                {
                    currentreg = CRegName::GPR_Lo[count];
                    break;
                }
            }
        }

        CPU_Message("SyncTo.GetArmRegMapped(%s) = %X%s%s CurrentSet.GetArmRegMapped(%s) = %X%s%s",
            ArmRegName((ArmReg)i),
            SyncTo.GetArmRegMapped((ArmReg)i),
            SyncTo.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped ? stdstr_f(" (%s)", CArmRegInfo::VariableMapName(SyncTo.GetVariableMappedTo((ArmReg)i))).c_str() : "",
            synctoreg.length() > 0 ? stdstr_f(" (%s)", synctoreg.c_str()).c_str() : "",
            ArmRegName((ArmReg)i),
            CurrentSet.GetArmRegMapped((ArmReg)i),
            CurrentSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped ? stdstr_f(" (%s)", CArmRegInfo::VariableMapName(CurrentSet.GetVariableMappedTo((ArmReg)i))).c_str() : "",
            currentreg.length() > 0 ? stdstr_f(" (%s)", currentreg.c_str()).c_str() : ""
        );
    }
}

void CArmRecompilerOps::SyncRegState(const CRegInfo & SyncTo)
{
    ResetRegProtection();

    bool changed = false;
#ifdef tofix
    UnMap_AllFPRs();
#endif
    if (m_RegWorkingSet.GetRoundingModel() != SyncTo.GetRoundingModel()) { m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown); }

    CPU_Message("Before:");
    OutputRegisterState(SyncTo, m_RegWorkingSet);

    for (uint32_t i = 0; i < 16; i++)
    {
        if (SyncTo.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped &&
            m_RegWorkingSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped &&
            SyncTo.GetVariableMappedTo((ArmReg)i) == m_RegWorkingSet.GetVariableMappedTo((ArmReg)i))
        {
            continue;
        }

        if (SyncTo.GetArmRegMapped((ArmReg)i) == CArmRegInfo::NotMapped ||
            SyncTo.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Temp_Mapped)
        {
            m_RegWorkingSet.UnMap_ArmReg((ArmReg)i);
            continue;
        }

        if (SyncTo.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped)
        {
            if (m_RegWorkingSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::GPR_Mapped)
            {
                bool moved_gpr_mapping = false;
                //See if mapped, if so move it
                for (uint32_t z = 0; z < 16; z++)
                {
                    if (SyncTo.GetArmRegMapped((ArmReg)z) != CArmRegInfo::GPR_Mapped)
                    {
                        continue;
                    }
                    for (uint32_t count = 1; count < 32; count++)
                    {
                        if (!SyncTo.IsMapped(count))
                        {
                            continue;
                        }

                        if (SyncTo.Is64Bit(count) && SyncTo.GetMipsRegMapHi(count) == (ArmReg)i)
                        {
                            CPU_Message("    regcache: move %s to %s", ArmRegName((ArmReg)i), ArmRegName((ArmReg)z));
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        if (SyncTo.GetMipsRegMapLo(count) == (ArmReg)i)
                        {
                            CPU_Message("    regcache: move %s to %s", ArmRegName((ArmReg)i), ArmRegName((ArmReg)z));
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                    }
                }
                if (!moved_gpr_mapping)
                {
                    m_RegWorkingSet.UnMap_ArmReg((ArmReg)i);
                }
            }
            bool moved = false;
            //See if mapped, if so move it
            for (uint32_t z = i + 1; z < 16; z++)
            {
                if (m_RegWorkingSet.GetArmRegMapped((ArmReg)z) == CArmRegInfo::Variable_Mapped &&
                    m_RegWorkingSet.GetVariableMappedTo((ArmReg)z) == SyncTo.GetVariableMappedTo((ArmReg)i))
                {
                    m_RegWorkingSet.UnMap_ArmReg((ArmReg)i);

                    CPU_Message("    regcache: move %s to %s for variable mapping (%s)", ArmRegName((ArmReg)z), ArmRegName((ArmReg)i), CArmRegInfo::VariableMapName(m_RegWorkingSet.GetVariableMappedTo((ArmReg)z)));
                    AddConstToArmReg((ArmReg)i, (ArmReg)z, 0);
                    m_RegWorkingSet.SetArmRegMapped((ArmReg)i, m_RegWorkingSet.GetArmRegMapped((ArmReg)z));
                    m_RegWorkingSet.SetVariableMappedTo((ArmReg)i, m_RegWorkingSet.GetVariableMappedTo((ArmReg)z));
                    m_RegWorkingSet.SetArmRegMapped((ArmReg)z, CArmRegInfo::NotMapped);
                    m_RegWorkingSet.SetVariableMappedTo((ArmReg)z, CArmRegInfo::VARIABLE_UNKNOWN);
                    moved = true;
                }
            }
            if (!moved)
            {
                Map_Variable(SyncTo.GetVariableMappedTo((ArmReg)i), (ArmReg)i);
            }
        }

        if (m_RegWorkingSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped &&
            m_RegWorkingSet.GetVariableMappedTo((ArmReg)i) != SyncTo.GetVariableMappedTo((ArmReg)i))
        {
            //See if mapped, if so move it
            for (uint32_t z = i + 1; z < 16; z++)
            {
                if (SyncTo.GetArmRegMapped((ArmReg)z) != CArmRegInfo::Variable_Mapped ||
                    SyncTo.GetVariableMappedTo((ArmReg)z) != m_RegWorkingSet.GetVariableMappedTo((ArmReg)i))
                {
                    continue;
                }
                m_RegWorkingSet.UnMap_ArmReg((ArmReg)z);

                CPU_Message("    regcache: move %s to %s for variable mapping (%s)", ArmRegName((ArmReg)i), ArmRegName((ArmReg)z), CArmRegInfo::VariableMapName(m_RegWorkingSet.GetVariableMappedTo((ArmReg)i)));
                AddConstToArmReg((ArmReg)z, (ArmReg)i, 0);
                m_RegWorkingSet.SetArmRegMapped((ArmReg)z, m_RegWorkingSet.GetArmRegMapped((ArmReg)i));
                m_RegWorkingSet.SetVariableMappedTo((ArmReg)z, m_RegWorkingSet.GetVariableMappedTo((ArmReg)i));
                m_RegWorkingSet.SetArmRegMapped((ArmReg)i, CArmRegInfo::NotMapped);
                m_RegWorkingSet.SetVariableMappedTo((ArmReg)i, CArmRegInfo::VARIABLE_UNKNOWN);
                break;
            }

            m_RegWorkingSet.UnMap_ArmReg((ArmReg)i);
        }
        if (m_RegWorkingSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped &&
            m_RegWorkingSet.GetVariableMappedTo((ArmReg)i) != CArmRegInfo::VARIABLE_GPR &&
            SyncTo.GetArmRegMapped((ArmReg)i) != CArmRegInfo::Variable_Mapped)
        {
            m_RegWorkingSet.UnMap_ArmReg((ArmReg)i);
        }
    }

#ifdef tofix
    x86Reg MemStackReg = Get_MemoryStack();
    x86Reg TargetStackReg = SyncTo.Get_MemoryStack();

    //CPU_Message("MemoryStack for Original State = %s",MemStackReg > 0?x86_Name(MemStackReg):"Not Mapped");
    if (MemStackReg != TargetStackReg)
    {
        if (TargetStackReg == x86_Unknown)
        {
            UnMap_X86reg(MemStackReg);
        }
        else if (MemStackReg == x86_Unknown)
        {
            UnMap_X86reg(TargetStackReg);
            CPU_Message("    regcache: allocate %s as Memory Stack", x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(TargetStackReg, CRegInfo::Stack_Mapped);
            MoveVariableToX86reg(&g_Recompiler->MemoryStackPos(), "MemoryStack", TargetStackReg);
        }
        else
        {
            UnMap_X86reg(TargetStackReg);
            CPU_Message("    regcache: change allocation of Memory Stack from %s to %s", x86_Name(MemStackReg), x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(TargetStackReg, CRegInfo::Stack_Mapped);
            m_RegWorkingSet.SetX86Mapped(MemStackReg, CRegInfo::NotMapped);
            MoveX86RegToX86Reg(MemStackReg, TargetStackReg);
        }
    }
#endif

    for (uint32_t i = 1; i < 32; i++)
    {
        CPU_Message("SyncTo.GetMipsRegState(%d: %s) = %X GetMipsRegState(%d: %s) = %X", i, CRegName::GPR[i], SyncTo.GetMipsRegState(i), i, CRegName::GPR[i], GetMipsRegState(i));
        if (IsMapped(i) && Is64Bit(i)) { CPU_Message("GetMipsRegMapHi(%d: %s) = %X", i, CRegName::GPR[i], GetMipsRegMapHi(i)); }
        if (IsMapped(i)) { CPU_Message("GetMipsRegMapLo(%d: %s) = %X", i, CRegName::GPR[i], GetMipsRegMapLo(i)); }

        if (GetMipsRegState(i) == SyncTo.GetMipsRegState(i) ||
            (g_System->b32BitCore() && GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN) ||
            (g_System->b32BitCore() && GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO))
        {
            switch (GetMipsRegState(i)) {
            case CRegInfo::STATE_UNKNOWN: continue;
            case CRegInfo::STATE_MAPPED_64:
                if (GetMipsRegMapHi(i) == SyncTo.GetMipsRegMapHi(i) &&
                    GetMipsRegMapLo(i) == SyncTo.GetMipsRegMapLo(i))
                {
                    continue;
                }
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (GetMipsRegMapLo(i) == SyncTo.GetMipsRegMapLo(i))
                {
                    continue;
                }
                break;
            case CRegInfo::STATE_CONST_64:
                if (GetMipsReg(i) != SyncTo.GetMipsReg(i))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (GetMipsRegLo(i) != SyncTo.GetMipsRegLo(i))
                {
                    CPU_Message("Value of const is different Reg %d (%s) Value: 0x%08X to 0x%08X", i, CRegName::GPR[i], GetMipsRegLo(i), SyncTo.GetMipsRegLo(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            default:
                CPU_Message("Unhandled Reg state %d\nin SyncRegState", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        changed = true;

        ArmReg Reg = Arm_Unknown, RegHi = Arm_Unknown, GprReg = Arm_Unknown;
        switch (SyncTo.GetMipsRegState(i))
        {
        case CRegInfo::STATE_UNKNOWN: UnMap_GPR(i, true);  break;
        case CRegInfo::STATE_MAPPED_64:
            Reg = SyncTo.GetMipsRegMapLo(i);
            RegHi = SyncTo.GetMipsRegMapHi(i);
            UnMap_ArmReg(Reg);
            UnMap_ArmReg(RegHi);
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN:
                GprReg = m_RegWorkingSet.GetVariableReg(CArmRegInfo::VARIABLE_GPR);
                if (GprReg == Arm_Unknown)
                {
                    MoveVariableToArmReg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg);
                    MoveVariableToArmReg(&_GPR[i].UW[1], CRegName::GPR_Hi[i], RegHi);
                }
                else
                {
                    LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(i << 3), CRegName::GPR_Lo[i]);
                    LoadArmRegPointerToArmReg(RegHi, GprReg, (uint8_t)(i << 3) + 4, CRegName::GPR_Hi[i]);
                }
                break;
            case CRegInfo::STATE_MAPPED_64:
                AddConstToArmReg(Reg, GetMipsRegMapLo(i), 0);
                m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                AddConstToArmReg(RegHi, GetMipsRegMapHi(i), 0);
                m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapHi(i), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                ShiftRightSignImmed(RegHi, GetMipsRegMapLo(i), 31);
                AddConstToArmReg(Reg, GetMipsRegMapLo(i), 0);
                m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                MoveConstToArmReg(RegHi, (uint32_t)0);
                AddConstToArmReg(Reg, GetMipsRegMapLo(i), 0);
                m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                break;
#ifdef tofix
            case CRegInfo::STATE_CONST_64:
                MoveConstToX86reg(GetMipsRegHi(i), x86RegHi);
                MoveConstToX86reg(GetMipsRegLo(i), Reg);
                break;
#endif
            case CRegInfo::STATE_CONST_32_SIGN:
                MoveConstToArmReg(RegHi, (uint32_t)(GetMipsRegLo_S(i) >> 31));
                MoveConstToArmReg(Reg, (uint32_t)(GetMipsRegLo(i)));
                break;
            default:
                CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
                continue;
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegMapHi(i, RegHi);
            m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_64);
            m_RegWorkingSet.SetArmRegMapped(Reg, CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetArmRegMapped(RegHi, CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetArmRegMapOrder(Reg, 1);
            m_RegWorkingSet.SetArmRegMapOrder(RegHi, 1);
            m_RegWorkingSet.SetArmRegProtected(Reg, true);
            m_RegWorkingSet.SetArmRegProtected(RegHi, true);
            break;
        case CRegInfo::STATE_MAPPED_32_SIGN:
            Reg = SyncTo.GetMipsRegMapLo(i);
            UnMap_ArmReg(Reg);
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN:
                GprReg = m_RegWorkingSet.GetVariableReg(CArmRegInfo::VARIABLE_GPR);
                if (GprReg == Arm_Unknown)
                {
                    MoveVariableToArmReg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg);
                }
                else
                {
                    LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(i << 3), CRegName::GPR_Lo[i]);
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                MoveConstToArmReg(Reg, GetMipsRegLo(i));
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
            case CRegInfo::STATE_MAPPED_32_ZERO:
                if (GetMipsRegMapLo(i) != Reg)
                {
                    AddConstToArmReg(Reg, GetMipsRegMapLo(i), 0);
                    m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                }
                break;
            case CRegInfo::STATE_MAPPED_64:
                g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef tofix
                MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                m_RegWorkingSet.SetX86Mapped(GetMipsRegMapHi(i), CRegInfo::NotMapped);
#endif
                break;
            case CRegInfo::STATE_CONST_64:
                CPU_Message("hi %X\nLo %X", GetMipsRegHi(i), GetMipsRegLo(i));
                CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
                break;
            default:
                CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_32_SIGN);
            m_RegWorkingSet.SetArmRegMapped(Reg, CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetArmRegMapOrder(Reg, 1);
            m_RegWorkingSet.SetArmRegProtected(Reg, true);
            break;
        case CRegInfo::STATE_MAPPED_32_ZERO:
            Reg = SyncTo.GetMipsRegMapLo(i);
            UnMap_ArmReg(Reg);
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_MAPPED_64:
            case CRegInfo::STATE_UNKNOWN:
                GprReg = m_RegWorkingSet.GetVariableReg(CArmRegInfo::VARIABLE_GPR);
                LoadArmRegPointerToArmReg(Reg, GprReg, (uint8_t)(i << 3), CRegName::GPR_Lo[i]);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                AddConstToArmReg(Reg, GetMipsRegMapLo(i), 0);
                m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (g_System->b32BitCore())
                {
                    AddConstToArmReg(Reg, GetMipsRegMapLo(i), 0);
                    m_RegWorkingSet.SetArmRegMapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                }
                else
                {
                    CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (!g_System->b32BitCore() && GetMipsRegLo_S(i) < 0)
                {
                    CPU_Message("Sign Problems in SyncRegState\nSTATE_MAPPED_32_ZERO");
                    CPU_Message("%s: %X", CRegName::GPR[i], GetMipsRegLo_S(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                MoveConstToArmReg(Reg, GetMipsRegLo(i));
                break;
            default:
                CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegState(i, SyncTo.GetMipsRegState(i));
            m_RegWorkingSet.SetArmRegMapped(Reg, CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetArmRegMapOrder(Reg, 1);
            m_RegWorkingSet.SetArmRegProtected(Reg, true);
            break;
        default:
            CPU_Message("%d - %d reg: %s (%d)", SyncTo.GetMipsRegState(i), GetMipsRegState(i), CRegName::GPR[i], i);
            g_Notify->BreakPoint(__FILE__, __LINE__);
            changed = false;
        }
    }

    for (uint32_t i = 0; i < 16; i++)
    {
        if (m_RegWorkingSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped &&
            m_RegWorkingSet.GetVariableMappedTo((ArmReg)i) == CArmRegInfo::VARIABLE_GPR &&
            SyncTo.GetArmRegMapped((ArmReg)i) != CArmRegInfo::Variable_Mapped)
        {
            m_RegWorkingSet.UnMap_ArmReg((ArmReg)i);
        }

        if (m_RegWorkingSet.GetArmRegMapped((ArmReg)i) == CArmRegInfo::Variable_Mapped && SyncTo.GetArmRegMapped((ArmReg)i) != CArmRegInfo::Variable_Mapped)
        {
            CPU_Message("Invalid SyncTo.GetArmRegMapped(%s) = %X m_RegWorkingSet.GetArmRegMapped(%s) = %X", ArmRegName((ArmReg)i), SyncTo.GetArmRegMapped((ArmReg)i), ArmRegName((ArmReg)i), m_RegWorkingSet.GetArmRegMapped((ArmReg)i));
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    CPU_Message("after:");
    OutputRegisterState(SyncTo, m_RegWorkingSet);
    for (int32_t i = 0; i < 16; i++)
    {
        m_RegWorkingSet.SetArmRegProtected((ArmReg)i, false);
    }
}

void CArmRecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason)
{
    m_RegWorkingSet = ExitRegSet;
    for (int32_t i = 0; i < 16; i++)
    {
        m_RegWorkingSet.SetArmRegProtected((ArmReg)i, false);
    }
    m_RegWorkingSet.WriteBackRegisters();
    ExitRegSet = m_RegWorkingSet;

    if (TargetPC != (uint32_t)-1)
    {
        MoveConstToArmReg(Arm_R1, TargetPC);
        MoveConstToArmReg(Arm_R2, (uint32_t)&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
        StoreArmRegToArmRegPointer(Arm_R1, Arm_R2, 0);

        UpdateCounters(ExitRegSet, TargetPC <= JumpPC && JumpPC != -1, reason == CExitInfo::Normal);
    }
    else
    {
        UpdateCounters(ExitRegSet, false, reason == CExitInfo::Normal);
    }

    bool bDelay;
    switch (reason)
    {
    case CExitInfo::Normal:
    case CExitInfo::Normal_NoSysCheck:
        ExitRegSet.SetBlockCycleCount(0);
        if (TargetPC != (uint32_t)-1)
        {
            if (TargetPC <= JumpPC && reason == CExitInfo::Normal)
            {
                CPU_Message("CompileSystemCheck 1");
                CompileSystemCheck((uint32_t)-1, ExitRegSet);
            }
        }
        else
        {
            if (reason == CExitInfo::Normal)
            {
                CPU_Message("CompileSystemCheck 2");
                CompileSystemCheck((uint32_t)-1, ExitRegSet);
            }
        }
        ExitCodeBlock();
        break;
    case CExitInfo::DoSysCall:
        bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
        MoveConstToArmReg(Arm_R1, (uint32_t)bDelay, bDelay ? "true" : "false");
        MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg);
        CallFunction(AddressOf(&CRegisters::DoSysCallException), "CRegisters::DoSysCallException");
        ExitCodeBlock();
        break;
    case CExitInfo::COP1_Unuseable:
        bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
        MoveConstToArmReg(Arm_R2, (uint32_t)1, "1");
        MoveConstToArmReg(Arm_R1, (uint32_t)bDelay, bDelay ? "true" : "false");
        MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg);
        CallFunction(AddressOf(&CRegisters::DoCopUnusableException), "CRegisters::DoCopUnusableException");
        ExitCodeBlock();
        break;
    case CExitInfo::TLBReadMiss:
        bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
        MoveVariableToArmReg(g_TLBLoadAddress, "g_TLBLoadAddress", Arm_R2);
        MoveConstToArmReg(Arm_R1, (uint32_t)bDelay, bDelay ? "true" : "false");
        MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg);
        CallFunction(AddressOf(&CRegisters::DoTLBReadMiss), "CRegisters::DoTLBReadMiss");
        ExitCodeBlock();
        break;
    case CExitInfo::TLBWriteMiss:
        ArmBreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CArmRecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason, CArmOps::ArmCompareType CompareType)
{
    BranchLabel20(CompareType, stdstr_f("Exit_%d", m_ExitInfo.size()).c_str());

    CExitInfo ExitInfo;
    ExitInfo.ID = m_ExitInfo.size();
    ExitInfo.TargetPC = TargetPC;
    ExitInfo.ExitRegSet = ExitRegSet;
    ExitInfo.reason = reason;
    ExitInfo.NextInstruction = m_NextInstruction;
    ExitInfo.JumpLoc = (uint32_t *)(*g_RecompPos - 4);
    m_ExitInfo.push_back(ExitInfo);
}

void CArmRecompilerOps::CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet)
{
    CRegInfo OriginalWorkingRegSet = GetRegWorkingSet();
    SetRegWorkingSet(RegSet);

    ArmReg TempReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
    MoveVariableToArmReg((void *)&g_SystemEvents->DoSomething(), "g_SystemEvents->DoSomething()", TempReg);
    CompareArmRegToConst(TempReg, 0);
    m_RegWorkingSet.SetArmRegProtected(TempReg, false);
    BranchLabel20(ArmBranch_Equal, "Continue_From_Interrupt_Test");
    uint32_t * Jump = (uint32_t *)(*g_RecompPos - 4);

    if (TargetPC != (uint32_t)-1)
    {
        MoveConstToVariable(TargetPC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    }
    m_RegWorkingSet.WriteBackRegisters();

    MoveConstToArmReg(Arm_R0, (uint32_t)g_SystemEvents, "g_SystemEvents");
    CallFunction(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");

    ExitCodeBlock();
    CPU_Message("");
    CPU_Message("      $Continue_From_Interrupt_Test:");
    SetJump20(Jump, (uint32_t *)*g_RecompPos);
    SetRegWorkingSet(OriginalWorkingRegSet);
}

void CArmRecompilerOps::CompileReadTLBMiss(ArmReg AddressReg, ArmReg LookUpReg)
{
    m_RegWorkingSet.SetArmRegProtected(AddressReg, true);
    m_RegWorkingSet.SetArmRegProtected(LookUpReg, true);

    ArmReg TlbLoadReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_LOAD_ADDRESS);
    StoreArmRegToArmRegPointer(AddressReg, TlbLoadReg, 0);
    CompareArmRegToConst(LookUpReg, 0);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBReadMiss, ArmBranch_Equal);
    m_RegWorkingSet.SetArmRegProtected(TlbLoadReg, false);
}

void CArmRecompilerOps::CompileWriteTLBMiss(ArmReg AddressReg, ArmReg LookUpReg)
{
    m_RegWorkingSet.SetArmRegProtected(AddressReg, true);
    m_RegWorkingSet.SetArmRegProtected(LookUpReg, true);

    ArmReg TlbStoreReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_STORE_ADDRESS);
    StoreArmRegToArmRegPointer(AddressReg, TlbStoreReg, 0);
    CompareArmRegToConst(LookUpReg, 0);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBWriteMiss, ArmBranch_Equal);
    m_RegWorkingSet.SetArmRegProtected(TlbStoreReg, false);
}

CRegInfo & CArmRecompilerOps::GetRegWorkingSet(void)
{
    return m_RegWorkingSet;
}

void CArmRecompilerOps::SetRegWorkingSet(const CRegInfo & RegInfo)
{
    m_RegWorkingSet = RegInfo;
}

bool CArmRecompilerOps::InheritParentInfo()
{
    if (m_Section->m_CompiledLocation == NULL)
    {
        m_Section->m_CompiledLocation = *g_RecompPos;
        m_Section->DisplaySectionInformation();
        m_Section->m_CompiledLocation = NULL;
    }
    else
    {
        m_Section->DisplaySectionInformation();
    }

    if (m_Section->m_ParentSection.empty())
    {
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    if (m_Section->m_ParentSection.size() == 1)
    {
        CCodeSection * Parent = *(m_Section->m_ParentSection.begin());
        if (Parent->m_CompiledLocation == NULL)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        CJumpInfo * JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;

        m_Section->m_RegEnter = JumpInfo->RegSet;
        LinkJump(*JumpInfo, m_Section->m_SectionID);
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    //Multiple Parents
    BLOCK_PARENT_LIST ParentList;
    CCodeSection::SECTION_LIST::iterator iter;
    for (iter = m_Section->m_ParentSection.begin(); iter != m_Section->m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (Parent->m_CompiledLocation == NULL) { continue; }
        if (Parent->m_JumpSection != Parent->m_ContinueSection)
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
        else
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Cont;
            ParentList.push_back(BlockParent);
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
    }
    size_t NoOfCompiledParents = ParentList.size();
    if (NoOfCompiledParents == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }

    // Add all the uncompiled blocks to the end of the list
    for (iter = m_Section->m_ParentSection.begin(); iter != m_Section->m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (Parent->m_CompiledLocation != NULL) { continue; }
        if (Parent->m_JumpSection != Parent->m_ContinueSection)
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
        else
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Cont;
            ParentList.push_back(BlockParent);
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
    }
    int FirstParent = -1;
    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        if (!ParentList[i].JumpInfo->FallThrough)
        {
            continue;
        }
        if (FirstParent != -1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        FirstParent = i;
    }
    if (FirstParent == -1)
    {
        FirstParent = 0;
    }

    //Link First Parent to start
    CCodeSection * Parent = ParentList[FirstParent].Parent;
    CJumpInfo * JumpInfo = ParentList[FirstParent].JumpInfo;

    SetRegWorkingSet(JumpInfo->RegSet);
    m_RegWorkingSet.ResetRegProtection();
    LinkJump(*JumpInfo, m_Section->m_SectionID, Parent->m_SectionID);

    if (JumpInfo->ExitReason == CExitInfo::Normal_NoSysCheck)
    {
        if (m_RegWorkingSet.GetBlockCycleCount() != 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (JumpInfo->JumpPC != (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        UpdateCounters(m_RegWorkingSet, m_Section->m_EnterPC < JumpInfo->JumpPC, true);
        if (JumpInfo->JumpPC == (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (m_Section->m_EnterPC <= JumpInfo->JumpPC)
        {
            CPU_Message("CompileSystemCheck 10");
            CompileSystemCheck(m_Section->m_EnterPC, GetRegWorkingSet());
        }
    }
    JumpInfo->FallThrough = false;

    //determine loop reg usage
    if (m_Section->m_InLoop && ParentList.size() > 1)
    {
        if (!SetupRegisterForLoop(m_Section->m_BlockInfo, m_Section->m_RegEnter)) { return false; }
        m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
    }

    for (size_t i = 0; i < ParentList.size(); i++)
    {
        //x86Reg MemoryStackPos;
        int i2;

        if (i == (size_t)FirstParent) { continue; }
        Parent = ParentList[i].Parent;
        if (Parent->m_CompiledLocation == NULL)
        {
            continue;
        }
        CRegInfo * RegSet = &ParentList[i].JumpInfo->RegSet;

        if (m_RegWorkingSet.GetRoundingModel() != RegSet->GetRoundingModel()) { m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown); }

        //Find Parent MapRegState
        /*MemoryStackPos = x86_Unknown;
        for (i2 = 0; i2 < sizeof(x86_Registers) / sizeof(x86_Registers[0]); i2++)
        {
        if (RegSet->GetArmRegMapped(x86_Registers[i2]) == CRegInfo::Stack_Mapped)
        {
        MemoryStackPos = x86_Registers[i2];
        break;
        }
        }
        if (MemoryStackPos == x86_Unknown)
        {
        // if the memory stack position is not mapped then unmap it
        x86Reg MemStackReg = Get_MemoryStack();
        if (MemStackReg != x86_Unknown)
        {
        UnMap_X86reg(MemStackReg);
        }
        }*/

        for (i2 = 1; i2 < 32; i2++)
        {
            if (Is32BitMapped(i2))
            {
                switch (RegSet->GetMipsRegState(i2))
                {
                case CRegInfo::STATE_MAPPED_64: Map_GPR_64bit(i2, i2); break;
                case CRegInfo::STATE_MAPPED_32_ZERO: break;
                case CRegInfo::STATE_MAPPED_32_SIGN:
                    if (IsUnsigned(i2))
                    {
                        m_RegWorkingSet.SetMipsRegState(i2, CRegInfo::STATE_MAPPED_32_SIGN);
                    }
                    break;
                case CRegInfo::STATE_CONST_64: Map_GPR_64bit(i2, i2); break;
                case CRegInfo::STATE_CONST_32_SIGN:
                    if ((RegSet->GetMipsRegLo_S(i2) < 0) && IsUnsigned(i2))
                    {
                        m_RegWorkingSet.SetMipsRegState(i2, CRegInfo::STATE_MAPPED_32_SIGN);
                    }
                    break;
                case CRegInfo::STATE_UNKNOWN:
                    if (g_System->b32BitCore())
                    {
                        Map_GPR_32bit(i2, true, i2);
                    }
                    else
                    {
                        //Map_GPR_32bit(i2,true,i2);
                        Map_GPR_64bit(i2, i2); //??
                        //UnMap_GPR(Section,i2,true); ??
                    }
                    break;
                default:
                    CPU_Message("Unknown CPU State(%d) in InheritParentInfo", GetMipsRegState(i2));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            if (IsConst(i2)) {
                if (GetMipsRegState(i2) != RegSet->GetMipsRegState(i2))
                {
                    switch (RegSet->GetMipsRegState(i2))
                    {
                    case CRegInfo::STATE_MAPPED_64:
                        Map_GPR_64bit(i2, i2);
                        break;
                    case CRegInfo::STATE_MAPPED_32_ZERO:
                        if (Is32Bit(i2))
                        {
                            Map_GPR_32bit(i2, (GetMipsRegLo(i2) & 0x80000000) != 0, i2);
                        }
                        else
                        {
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        break;
                    case CRegInfo::STATE_MAPPED_32_SIGN:
                        if (Is32Bit(i2))
                        {
                            Map_GPR_32bit(i2, true, i2);
                        }
                        else
                        {
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        break;
                    case CRegInfo::STATE_UNKNOWN:
                        if (g_System->b32BitCore())
                        {
                            Map_GPR_32bit(i2, true, i2);
                        }
                        else
                        {
                            Map_GPR_64bit(i2, i2);
                        }
                        break;
                    default:
                        CPU_Message("Unknown CPU State(%d) in InheritParentInfo", RegSet->GetMipsRegState(i2));
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                        break;
                    }
                }
                else if (Is32Bit(i2) && GetMipsRegLo(i2) != RegSet->GetMipsRegLo(i2))
                {
                    Map_GPR_32bit(i2, true, i2);
                }
                else if (Is64Bit(i2) && GetMipsReg(i2) != RegSet->GetMipsReg(i2))
                {
                    Map_GPR_32bit(i2, true, i2);
                }
            }
            ResetRegProtection();
        }

#ifdef tofix
        if (MemoryStackPos > 0)
        {
            Map_MemoryStack(MemoryStackPos, true);
        }
#endif
    }
    m_Section->m_RegEnter = m_RegWorkingSet;

    //Sync registers for different blocks
    stdstr_f Label("Section_%d", m_Section->m_SectionID);
    int CurrentParent = FirstParent;
    bool NeedSync = false;
    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        CRegInfo * RegSet;
        int i2;

        if (i == (size_t)FirstParent) { continue; }
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;
        RegSet = &ParentList[i].JumpInfo->RegSet;

        if (JumpInfo->RegSet.GetBlockCycleCount() != 0) { NeedSync = true; }

#ifdef tofix
        for (i2 = 0; !NeedSync && i2 < 8; i2++)
        {
            if (m_RegWorkingSet.FpuMappedTo(i2) == (uint32_t)-1)
            {
                NeedSync = true;
            }
        }
#endif

#ifdef tofix
        for (i2 = 0; !NeedSync && i2 < sizeof(x86_Registers) / sizeof(x86_Registers[0]); i2++)
        {
            if (m_RegWorkingSet.GetArmRegMapped(x86_Registers[i2]) == CRegInfo::Stack_Mapped)
            {
                if (m_RegWorkingSet.GetArmRegMapped(x86_Registers[i2]) != RegSet->GetArmRegMapped(x86_Registers[i2]))
                {
                    NeedSync = true;
                }
                break;
            }
        }
#endif
        for (i2 = 0; !NeedSync && i2 < 32; i2++)
        {
            if (NeedSync == true) { break; }
            if (m_RegWorkingSet.GetMipsRegState(i2) != RegSet->GetMipsRegState(i2))
            {
                NeedSync = true;
                continue;
            }
            switch (m_RegWorkingSet.GetMipsRegState(i2))
            {
            case CRegInfo::STATE_UNKNOWN: break;
            case CRegInfo::STATE_MAPPED_64:
                if (GetMipsRegMapHi(i2) != RegSet->GetMipsRegMapHi(i2) ||
                    GetMipsRegMapLo(i2) != RegSet->GetMipsRegMapLo(i2))
                {
                    NeedSync = true;
                }
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (GetMipsRegMapLo(i2) != RegSet->GetMipsRegMapLo(i2))
                {
                    //DisplayError(L"Parent: %d",Parent->SectionID);
                    NeedSync = true;
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (GetMipsRegLo(i2) != RegSet->GetMipsRegLo(i2))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    NeedSync = true;
                }
                break;
            default:
                WriteTrace(TraceRecompiler, TraceError, "Unhandled Reg state %d\nin InheritParentInfo", GetMipsRegState(i2));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        if (NeedSync == false) { continue; }
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        BranchLabel20(ArmBranch_Always, Label.c_str());
        JumpInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        JumpInfo->LinkLocation2 = NULL;

        CurrentParent = i;
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        CPU_Message("   Section_%d (from %d):", m_Section->m_SectionID, Parent->m_SectionID);
        if (JumpInfo->LinkLocation != NULL)
        {
            SetJump20(JumpInfo->LinkLocation, (uint32_t *)*g_RecompPos);
            JumpInfo->LinkLocation = NULL;
            if (JumpInfo->LinkLocation2 != NULL)
            {
                SetJump20(JumpInfo->LinkLocation2, (uint32_t *)*g_RecompPos);
                JumpInfo->LinkLocation2 = NULL;
            }
        }

        m_RegWorkingSet = JumpInfo->RegSet;
        if (m_Section->m_EnterPC < JumpInfo->JumpPC)
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            CPU_Message("CompileSystemCheck 11");
            CompileSystemCheck(m_Section->m_EnterPC, m_RegWorkingSet);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, false, true);
        }
        SyncRegState(m_Section->m_RegEnter);         //Sync
        m_Section->m_RegEnter = m_RegWorkingSet;
    }

    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;
        LinkJump(*JumpInfo);
    }

    CPU_Message("   Section_%d:", m_Section->m_SectionID);
    m_Section->m_RegEnter.SetBlockCycleCount(0);
    return true;
}

void CArmRecompilerOps::LinkJump(CJumpInfo & JumpInfo, uint32_t SectionID, uint32_t FromSectionID)
{
    if (JumpInfo.LinkLocation != NULL)
    {
        if (SectionID != -1)
        {
            if (FromSectionID != -1)
            {
                CPU_Message("   Section_%d (from %d):", SectionID, FromSectionID);
            }
            else
            {
                CPU_Message("   Section_%d:", SectionID);
            }
        }
        SetJump20(JumpInfo.LinkLocation, (uint32_t *)*g_RecompPos);
        JumpInfo.LinkLocation = NULL;
        if (JumpInfo.LinkLocation2 != NULL)
        {
            SetJump20(JumpInfo.LinkLocation2, (uint32_t *)*g_RecompPos);
            JumpInfo.LinkLocation2 = NULL;
        }
    }
}

void CArmRecompilerOps::JumpToSection(CCodeSection * Section)
{
    BranchLabel20(ArmBranch_Always, stdstr_f("Section_%d", Section->m_SectionID).c_str());
    SetJump20(((uint32_t *)*g_RecompPos) - 1, (uint32_t *)(Section->m_CompiledLocation));
}

void CArmRecompilerOps::JumpToUnknown(CJumpInfo * JumpInfo)
{
    BranchLabel20(ArmBranch_Always, JumpInfo->BranchLabel.c_str());
    JumpInfo->LinkLocation = (uint32_t*)(*g_RecompPos - 4);
}

void CArmRecompilerOps::SetCurrentPC(uint32_t ProgramCounter)
{
    m_CompilePC = ProgramCounter;
    __except_try()
    {
        if (!g_MMU->LW_VAddr(m_CompilePC, m_Opcode.Hex))
        {
            g_Notify->FatalError(GS(MSG_FAIL_LOAD_WORD));
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
}

uint32_t CArmRecompilerOps::GetCurrentPC(void)
{
    return m_CompilePC;
}

void CArmRecompilerOps::SetCurrentSection(CCodeSection * section)
{
    m_Section = section;
}

void CArmRecompilerOps::SetNextStepType(STEP_TYPE StepType)
{
    m_NextInstruction = StepType;
}

STEP_TYPE CArmRecompilerOps::GetNextStepType(void)
{
    return m_NextInstruction;
}

const OPCODE &CArmRecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles)
{
    if (!g_SyncSystem)
    {
        return;
    }
    WriteArmComment("Updating Sync CPU");
    RegSet.BeforeCallDirect();
    MoveConstToArmReg(Arm_R2, Cycles);
    MoveConstToArmReg(Arm_R1, (uint32_t)g_SyncSystem, "g_SyncSystem");
    MoveConstToArmReg(Arm_R0, (uint32_t)g_System);
    CallFunction((void *)AddressOf(&CN64System::UpdateSyncCPU), "CN64System::UpdateSyncCPU");
    RegSet.AfterCallDirect();
}

void CArmRecompilerOps::UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues)
{
    if (RegSet.GetBlockCycleCount() != 0)
    {
        UpdateSyncCPU(RegSet, RegSet.GetBlockCycleCount());
        WriteArmComment("Update Counter");

        ArmReg NextTimerReg = RegSet.Map_Variable(CArmRegInfo::VARIABLE_NEXT_TIMER);
        ArmReg TempReg = RegSet.Map_TempReg(Arm_Any, -1, false);
        LoadArmRegPointerToArmReg(TempReg, NextTimerReg, 0);
        SubConstFromArmReg(TempReg, TempReg, RegSet.GetBlockCycleCount());
        StoreArmRegToArmRegPointer(TempReg, NextTimerReg, 0);
        if (ClearValues)
        {
            RegSet.SetBlockCycleCount(0);
        }
        if (CheckTimer)
        {
            CompareArmRegToConst(TempReg, 0);
        }
        RegSet.SetArmRegProtected(TempReg, false);
        RegSet.SetArmRegProtected(NextTimerReg, false);
    }
    else if (CheckTimer)
    {
        ArmReg NextTimerReg = RegSet.Map_Variable(CArmRegInfo::VARIABLE_NEXT_TIMER);
        ArmReg TempReg = RegSet.Map_TempReg(Arm_Any, -1, false);
        LoadArmRegPointerToArmReg(TempReg, NextTimerReg, 0);
        CompareArmRegToConst(TempReg, 0);
        RegSet.SetArmRegProtected(TempReg, false);
        RegSet.SetArmRegProtected(NextTimerReg, false);
    }

    if (CheckTimer)
    {
        uint8_t * Jump = *g_RecompPos;
        BranchLabel8(ArmBranch_GreaterThanOrEqual, "Continue_From_Timer_Test");
        RegSet.BeforeCallDirect();
        MoveConstToArmReg(Arm_R0, (uint32_t)g_SystemTimer, "g_SystemTimer");
        CallFunction(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
        RegSet.AfterCallDirect();
        FlushPopArmReg();

        CPU_Message("");
        CPU_Message("      $Continue_From_Timer_Test:");
        SetJump8(Jump, *g_RecompPos);
    }
}

void CArmRecompilerOps::CompileInterpterCall(void * Function, const char * FunctionName)
{
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToArmReg(Arm_R1, m_Opcode.Hex);
    MoveConstToArmReg(Arm_R2, (uint32_t)(void *)&R4300iOp::m_Opcode.Hex, "&R4300iOp::m_Opcode.Hex");
    StoreArmRegToArmRegPointer(Arm_R1, Arm_R2, 0);
    CallFunction(Function, FunctionName);
    m_RegWorkingSet.AfterCallDirect();
}

void CArmRecompilerOps::OverflowDelaySlot(bool TestTimer)
{
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(m_CompilePC + 4, _PROGRAM_COUNTER, "PROGRAM_COUNTER");

    if (g_SyncSystem)
    {
        MoveConstToArmReg(Arm_R0, (uint32_t)g_BaseSystem, "g_BaseSystem");
        CallFunction(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    }

    MoveConstToVariable(JUMP, &R4300iOp::m_NextInstruction, "R4300iOp::m_NextInstruction");

    if (TestTimer)
    {
        MoveConstToVariable(TestTimer, &R4300iOp::m_TestTimer, "R4300iOp::m_TestTimer");
    }

    MoveConstToArmReg(Arm_R0, g_System->CountPerOp());
    CallFunction((void *)CInterpreterCPU::ExecuteOps, "CInterpreterCPU::ExecuteOps");

    if (g_System->bFastSP() && g_Recompiler)
    {
        MoveConstToArmReg(Arm_R0, (uint32_t)g_Recompiler);
        CallFunction(AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos");
    }
    if (g_SyncSystem)
    {
        UpdateSyncCPU(m_RegWorkingSet, g_System->CountPerOp());
    }

    ExitCodeBlock();
    m_NextInstruction = END_BLOCK;
}

void CArmRecompilerOps::SW_Const(uint32_t Value, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        ArmReg TempRegAddress = Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempRegAddress, VAddr);
        ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
        ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
        ArmReg WriteMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_WRITEMAP);
        LoadArmRegPointerToArmReg(TempReg, WriteMapReg, TempReg, 2);
        CompileWriteTLBMiss(TempRegAddress, TempReg);
        ArmReg TempValueReg = Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempValueReg, Value);
        StoreArmRegToArmRegPointer(TempValueReg, TempReg, TempRegAddress, 0);
        return;
    }

    uint32_t PAddr;
    if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
        }
        return;
    }

    uint32_t ModValue;
    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        MoveConstToVariable(Value, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
        break;
    case 0x03F00000:
        switch (PAddr)
        {
        case 0x03F00000: MoveConstToVariable(Value, &g_Reg->RDRAM_CONFIG_REG, "RDRAM_CONFIG_REG"); break;
        case 0x03F00004: MoveConstToVariable(Value, &g_Reg->RDRAM_DEVICE_ID_REG, "RDRAM_DEVICE_ID_REG"); break;
        case 0x03F00008: MoveConstToVariable(Value, &g_Reg->RDRAM_DELAY_REG, "RDRAM_DELAY_REG"); break;
        case 0x03F0000C: MoveConstToVariable(Value, &g_Reg->RDRAM_MODE_REG, "RDRAM_MODE_REG"); break;
        case 0x03F00010: MoveConstToVariable(Value, &g_Reg->RDRAM_REF_INTERVAL_REG, "RDRAM_REF_INTERVAL_REG"); break;
        case 0x03F00014: MoveConstToVariable(Value, &g_Reg->RDRAM_REF_ROW_REG, "RDRAM_REF_ROW_REG"); break;
        case 0x03F00018: MoveConstToVariable(Value, &g_Reg->RDRAM_RAS_INTERVAL_REG, "RDRAM_RAS_INTERVAL_REG"); break;
        case 0x03F0001C: MoveConstToVariable(Value, &g_Reg->RDRAM_MIN_INTERVAL_REG, "RDRAM_MIN_INTERVAL_REG"); break;
        case 0x03F00020: MoveConstToVariable(Value, &g_Reg->RDRAM_ADDR_SELECT_REG, "RDRAM_ADDR_SELECT_REG"); break;
        case 0x03F00024: MoveConstToVariable(Value, &g_Reg->RDRAM_DEVICE_MANUF_REG, "RDRAM_DEVICE_MANUF_REG"); break;
        case 0x03F04004: break;
        case 0x03F08004: break;
        case 0x03F80004: break;
        case 0x03F80008: break;
        case 0x03F8000C: break;
        case 0x03F80014: break;
        default:
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
        }
        break;
    case 0x04000000:
        if (PAddr < 0x04002000)
        {
            MoveConstToVariable(Value, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
            break;
        }
        switch (PAddr)
        {
        case 0x04040000: MoveConstToVariable(Value, &g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG"); break;
        case 0x04040004: MoveConstToVariable(Value, &g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG"); break;
        case 0x04040008:
            MoveConstToVariable(Value, &g_Reg->SP_RD_LEN_REG, "SP_RD_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::SP_DMA_READ), "CDMA::SP_DMA_READ");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04040010:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R2, Value);
            MoveConstToArmReg(Arm_R1, PAddr);
            MoveConstToArmReg(Arm_R0, (uint32_t)(g_MMU), "g_MMU");
            CallFunction(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0404001C: MoveConstToVariable(0, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG"); break;
        case 0x04080000: MoveConstToVariable(Value & 0xFFC, &g_Reg->SP_PC_REG, "SP_PC_REG"); break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04100000:
        switch (PAddr)
        {
        case 0x0410000C:
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R2, Value);
            MoveConstToArmReg(Arm_R1, PAddr);
            MoveConstToArmReg(Arm_R0, (uint32_t)(g_MMU), "g_MMU");
            CallFunction(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04300000:
        switch (PAddr)
        {
        case 0x04300000:
            ModValue = 0x7F;
            if ((Value & MI_CLR_INIT) != 0)
            {
                ModValue |= MI_MODE_INIT;
            }
            if ((Value & MI_CLR_EBUS) != 0)
            {
                ModValue |= MI_MODE_EBUS;
            }
            if ((Value & MI_CLR_RDRAM) != 0)
            {
                ModValue |= MI_MODE_RDRAM;
            }
            if (ModValue != 0)
            {
                AndConstToVariable(&g_Reg->MI_MODE_REG, "MI_MODE_REG", ~ModValue);
            }

            ModValue = (Value & 0x7F);
            if ((Value & MI_SET_INIT) != 0)
            {
                ModValue |= MI_MODE_INIT;
            }
            if ((Value & MI_SET_EBUS) != 0)
            {
                ModValue |= MI_MODE_EBUS;
            }
            if ((Value & MI_SET_RDRAM) != 0)
            {
                ModValue |= MI_MODE_RDRAM;
            }
            if (ModValue != 0)
            {
                OrConstToVariable(&g_Reg->MI_MODE_REG, "MI_MODE_REG", ModValue);
            }
            if ((Value & MI_CLR_DP_INTR) != 0)
            {
                AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_DP);
                AndConstToVariable(&g_Reg->m_GfxIntrReg, "m_GfxIntrReg", (uint32_t)~MI_INTR_DP);
            }
            break;
        case 0x0430000C:
            ModValue = 0;
            if ((Value & MI_INTR_MASK_CLR_SP) != 0)
            {
                ModValue |= MI_INTR_MASK_SP;
            }
            if ((Value & MI_INTR_MASK_CLR_SI) != 0)
            {
                ModValue |= MI_INTR_MASK_SI;
            }
            if ((Value & MI_INTR_MASK_CLR_AI) != 0)
            {
                ModValue |= MI_INTR_MASK_AI;
            }
            if ((Value & MI_INTR_MASK_CLR_VI) != 0)
            {
                ModValue |= MI_INTR_MASK_VI;
            }
            if ((Value & MI_INTR_MASK_CLR_PI) != 0)
            {
                ModValue |= MI_INTR_MASK_PI;
            }
            if ((Value & MI_INTR_MASK_CLR_DP) != 0)
            {
                ModValue |= MI_INTR_MASK_DP;
            }
            if (ModValue != 0)
            {
                AndConstToVariable(&g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG", ~ModValue);
            }

            ModValue = 0;
            if ((Value & MI_INTR_MASK_SET_SP) != 0)
            {
                ModValue |= MI_INTR_MASK_SP;
            }
            if ((Value & MI_INTR_MASK_SET_SI) != 0)
            {
                ModValue |= MI_INTR_MASK_SI;
            }
            if ((Value & MI_INTR_MASK_SET_AI) != 0)
            {
                ModValue |= MI_INTR_MASK_AI;
            }
            if ((Value & MI_INTR_MASK_SET_VI) != 0)
            {
                ModValue |= MI_INTR_MASK_VI;
            }
            if ((Value & MI_INTR_MASK_SET_PI) != 0)
            {
                ModValue |= MI_INTR_MASK_PI;
            }
            if ((Value & MI_INTR_MASK_SET_DP) != 0)
            {
                ModValue |= MI_INTR_MASK_DP;
            }
            if (ModValue != 0)
            {
                OrConstToVariable(&g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG", ModValue);
            }
            break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04400000:
        switch (PAddr)
        {
        case 0x04400000:
            if (g_Plugins->Gfx()->ViStatusChanged != NULL)
            {
                ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
                MoveVariableToArmReg(&g_Reg->VI_STATUS_REG, "VI_STATUS_REG", TempReg);
                ArmReg TempValueReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
                MoveConstToArmReg(TempValueReg, Value);
                CompareArmRegToArmReg(TempReg, TempValueReg);

                uint8_t * Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Equal, "continue");

                m_RegWorkingSet.BeforeCallDirect();
                ArmReg VariableReg = TempValueReg != Arm_R1 ? Arm_R1 : Arm_R2;
                MoveConstToArmReg(VariableReg, (uint32_t)&g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                StoreArmRegToArmRegPointer(TempValueReg, VariableReg, 0);
                CallFunction((void *)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                m_RegWorkingSet.AfterCallDirect();
                FlushPopArmReg();
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            break;
        case 0x04400004: MoveConstToVariable((Value & 0xFFFFFF), &g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG"); break;
        case 0x04400008:
            if (g_Plugins->Gfx()->ViWidthChanged != NULL)
            {
                ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
                MoveVariableToArmReg(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", TempReg);
                ArmReg TempValueReg = m_RegWorkingSet.Map_TempReg(Arm_Any, -1, false);
                MoveConstToArmReg(TempValueReg, Value);
                CompareArmRegToArmReg(TempReg, TempValueReg);

                uint8_t * Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Equal, "continue");

                MoveArmRegToVariable(TempValueReg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction((void *)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                m_RegWorkingSet.AfterCallDirect();
                FlushPopArmReg();
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            break;
        case 0x0440000C: MoveConstToVariable(Value, &g_Reg->VI_INTR_REG, "VI_INTR_REG"); break;
        case 0x04400010:
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_VI);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04400014: MoveConstToVariable(Value, &g_Reg->VI_BURST_REG, "VI_BURST_REG"); break;
        case 0x04400018: MoveConstToVariable(Value, &g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG"); break;
        case 0x0440001C: MoveConstToVariable(Value, &g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG"); break;
        case 0x04400020: MoveConstToVariable(Value, &g_Reg->VI_LEAP_REG, "VI_LEAP_REG"); break;
        case 0x04400024: MoveConstToVariable(Value, &g_Reg->VI_H_START_REG, "VI_H_START_REG"); break;
        case 0x04400028: MoveConstToVariable(Value, &g_Reg->VI_V_START_REG, "VI_V_START_REG"); break;
        case 0x0440002C: MoveConstToVariable(Value, &g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG"); break;
        case 0x04400030: MoveConstToVariable(Value, &g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG"); break;
        case 0x04400034: MoveConstToVariable(Value, &g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG"); break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04500000: /* AI registers */
        switch (PAddr)
        {
        case 0x04500000: MoveConstToVariable(Value, &g_Reg->AI_DRAM_ADDR_REG, "AI_DRAM_ADDR_REG"); break;
        case 0x04500004:
            MoveConstToVariable(Value, &g_Reg->AI_LEN_REG, "AI_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            if (g_System->bFixedAudio())
            {
                ArmBreakPoint(__FILE__, __LINE__);
                MoveConstToArmReg(Arm_R0, (uint32_t)g_Audio, "g_Audio");
                CallFunction(AddressOf(&CAudio::LenChanged), "LenChanged");
            }
            else
            {
                CallFunction((void *)g_Plugins->Audio()->AiLenChanged, "AiLenChanged");
            }
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04500008: MoveConstToVariable((Value & 1), &g_Reg->AI_CONTROL_REG, "AI_CONTROL_REG"); break;
        case 0x0450000C:
            /* Clear Interrupt */;
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_AI);
            AndConstToVariable(&g_Reg->m_AudioIntrReg, "m_AudioIntrReg", (uint32_t)~MI_INTR_AI);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04500010:
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R2, Value);
            MoveConstToArmReg(Arm_R1, PAddr);
            MoveConstToArmReg(Arm_R0, (uint32_t)(g_MMU), "g_MMU");
            CallFunction(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04500014: MoveConstToVariable(Value, &g_Reg->AI_BITRATE_REG, "AI_BITRATE_REG"); break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04600000:
        switch (PAddr)
        {
        case 0x04600000: MoveConstToVariable(Value, &g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG"); break;
        case 0x04600004: MoveConstToVariable(Value, &g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG"); break;
        case 0x04600008:
            MoveConstToVariable(Value, &g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::PI_DMA_READ), "CDMA::PI_DMA_READ");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0460000C:
            MoveConstToVariable(Value, &g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::PI_DMA_WRITE), "CDMA::PI_DMA_WRITE");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600010:
            if ((Value & PI_CLR_INTR) != 0)
            {
                AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_PI);
                m_RegWorkingSet.BeforeCallDirect();
                MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
                CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
                m_RegWorkingSet.AfterCallDirect();
            }
            break;
        case 0x04600014: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG"); break;
        case 0x04600018: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG"); break;
        case 0x0460001C: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG"); break;
        case 0x04600020: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG"); break;
        case 0x04600024: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG"); break;
        case 0x04600028: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG"); break;
        case 0x0460002C: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG"); break;
        case 0x04600030: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG"); break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04700000:
        switch (PAddr)
        {
        case 0x04700000: MoveConstToVariable(Value, &g_Reg->RI_MODE_REG, "RI_MODE_REG"); break;
        case 0x04700004: MoveConstToVariable(Value, &g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG"); break;
        case 0x04700008: MoveConstToVariable(Value, &g_Reg->RI_CURRENT_LOAD_REG, "RI_CURRENT_LOAD_REG"); break;
        case 0x0470000C: MoveConstToVariable(Value, &g_Reg->RI_SELECT_REG, "RI_SELECT_REG"); break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04800000:
        switch (PAddr)
        {
        case 0x04800000: MoveConstToVariable(Value, &g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG"); break;
        case 0x04800004:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveConstToVariable(Value, &g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CPifRam *)g_MMU), "CPifRam *)g_MMU");
            CallFunction(AddressOf(&CPifRam::SI_DMA_READ), "CPifRam::SI_DMA_READ");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveConstToVariable(Value, &g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CPifRam *)g_MMU), "CPifRam *)g_MMU");
            CallFunction(AddressOf(&CPifRam::SI_DMA_WRITE), "CPifRam::SI_DMA_WRITE");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800018:
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_SI);
            AndConstToVariable(&g_Reg->SI_STATUS_REG, "SI_STATUS_REG", (uint32_t)~SI_STATUS_INTERRUPT);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x05000000:
        //64DD Registers
        if (g_Settings->LoadBool(Setting_EnableDisk))
        {
            switch (PAddr)
            {
            case 0x05000520:
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction(AddressOf(&DiskReset), "DiskReset");
                m_RegWorkingSet.AfterCallDirect();
                break;
            default:
                CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
                }
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        }
    case 0x1fc00000:
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        m_RegWorkingSet.BeforeCallDirect();
        MoveConstToArmReg(Arm_R2, Value);
        MoveConstToArmReg(Arm_R1, PAddr);
        MoveConstToArmReg(Arm_R0, (uint32_t)(g_MMU), "g_MMU");
        CallFunction(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
        m_RegWorkingSet.AfterCallDirect();
        break;
    default:
        CPU_Message("    Should be moving %X in to %08X ?!?", Value, VAddr);
        if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
        {
            g_Notify->DisplayError(stdstr_f("%s\ntrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
        }
        if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    }
}

void CArmRecompilerOps::SW_Register(ArmReg Reg, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetArmRegProtected(Reg, true);

        if (!g_System->bUseTlb())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
        ArmReg TempRegAddress = Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempRegAddress, VAddr);
        ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
        ArmReg WriteMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_WRITEMAP);
        LoadArmRegPointerToArmReg(TempReg, WriteMapReg, TempReg, 2);
        CompileWriteTLBMiss(TempRegAddress, TempReg);
        StoreArmRegToArmRegPointer(Reg, TempReg, TempRegAddress, 0);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        m_RegWorkingSet.SetArmRegProtected(TempRegAddress, false);
        return;
    }

    uint32_t PAddr;
    if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        MoveArmRegToVariable(Reg, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
        break;
    case 0x03F00000:
        switch (PAddr)
        {
        case 0x03F04004: break;
        case 0x03F08004: break;
        case 0x03F80004: break;
        case 0x03F80008: break;
        case 0x03F8000C: break;
        case 0x03F80014: break;
        default:
            CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04000000:
        switch (PAddr)
        {
        case 0x04040000: MoveArmRegToVariable(Reg, &g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG"); break;
        case 0x04040004: MoveArmRegToVariable(Reg, &g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG"); break;
        case 0x04040008:
            MoveArmRegToVariable(Reg, &g_Reg->SP_RD_LEN_REG, "SP_RD_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::SP_DMA_READ), "CDMA::SP_DMA_READ");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0404000C:
            MoveArmRegToVariable(Reg, &g_Reg->SP_WR_LEN_REG, "SP_WR_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::SP_DMA_WRITE), "CDMA::SP_DMA_WRITE");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04040010:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveArmRegToVariable(Reg, &CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue");
            m_RegWorkingSet.BeforeCallDirect();
            CallFunction(AddressOf(&CMipsMemoryVM::ChangeSpStatus), "CMipsMemoryVM::ChangeSpStatus");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0404001C: MoveConstToVariable(0, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG"); break;
        case 0x04080000:
            MoveArmRegToVariable(Reg, &g_Reg->SP_PC_REG, "SP_PC_REG");
            AndConstToVariable(&g_Reg->SP_PC_REG, "SP_PC_REG", 0xFFC);
            break;
        default:
            if (PAddr < 0x04002000)
            {
                MoveArmRegToVariable(Reg, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
            }
            else
            {
                CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
                }
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
        }
        break;
    case 0x04100000:
        if (PAddr == 0x0410000C)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        }
        m_RegWorkingSet.BeforeCallDirect();
        if (Reg != Arm_R2)
        {
            AddConstToArmReg(Arm_R2, Reg, 0);
        }
        MoveConstToArmReg(Arm_R1, PAddr);
        MoveConstToArmReg(Arm_R0, (uint32_t)(g_MMU), "g_MMU");
        CallFunction(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04300000:
        switch (PAddr)
        {
        case 0x04300000:
            MoveArmRegToVariable(Reg, &CMipsMemoryVM::m_MemLookupValue.UW[0], "CMipsMemoryVM::m_MemLookupValue.UW[0]");
            MoveConstToVariable(PAddr, &CMipsMemoryVM::m_MemLookupAddress, "m_MemLookupAddress");
            m_RegWorkingSet.BeforeCallDirect();
            CallFunction((void *)CMipsMemoryVM::Write32MIPSInterface, "CMipsMemoryVM::Write32MIPSInterface");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0430000C:
            MoveArmRegToVariable(Reg, &CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue");
            m_RegWorkingSet.BeforeCallDirect();
            CallFunction((void *)CMipsMemoryVM::ChangeMiIntrMask, "CMipsMemoryVM::ChangeMiIntrMask");
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04400000:
        switch (PAddr) {
        case 0x04400000:
            if (g_Plugins->Gfx()->ViStatusChanged != NULL)
            {
                ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
                MoveVariableToArmReg(&g_Reg->VI_STATUS_REG, "VI_STATUS_REG", TempReg);
                CompareArmRegToArmReg(TempReg, Reg);

                uint8_t * Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Equal, "continue");

                MoveArmRegToVariable(Reg, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction((void *)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                m_RegWorkingSet.AfterCallDirect();
                FlushPopArmReg();
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            break;
        case 0x04400004:
            MoveArmRegToVariable(Reg, &g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG");
            AndConstToVariable(&g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG", 0xFFFFFF);
            break;
        case 0x04400008:
            if (g_Plugins->Gfx()->ViWidthChanged != NULL)
            {
                ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
                MoveVariableToArmReg(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", TempReg);
                CompareArmRegToArmReg(TempReg, Reg);

                uint8_t * Jump = *g_RecompPos;
                BranchLabel8(ArmBranch_Equal, "continue");

                MoveArmRegToVariable(Reg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction((void *)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                m_RegWorkingSet.AfterCallDirect();
                FlushPopArmReg();
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            break;
        case 0x0440000C: MoveArmRegToVariable(Reg, &g_Reg->VI_INTR_REG, "VI_INTR_REG"); break;
        case 0x04400010:
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_VI);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04400014: MoveArmRegToVariable(Reg, &g_Reg->VI_BURST_REG, "VI_BURST_REG"); break;
        case 0x04400018: MoveArmRegToVariable(Reg, &g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG"); break;
        case 0x0440001C: MoveArmRegToVariable(Reg, &g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG"); break;
        case 0x04400020: MoveArmRegToVariable(Reg, &g_Reg->VI_LEAP_REG, "VI_LEAP_REG"); break;
        case 0x04400024: MoveArmRegToVariable(Reg, &g_Reg->VI_H_START_REG, "VI_H_START_REG"); break;
        case 0x04400028: MoveArmRegToVariable(Reg, &g_Reg->VI_V_START_REG, "VI_V_START_REG"); break;
        case 0x0440002C: MoveArmRegToVariable(Reg, &g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG"); break;
        case 0x04400030: MoveArmRegToVariable(Reg, &g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG"); break;
        case 0x04400034: MoveArmRegToVariable(Reg, &g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG"); break;
        default:
            CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
        }
        break;
    case 0x04500000: /* AI registers */
        switch (PAddr) {
        case 0x04500000: MoveArmRegToVariable(Reg, &g_Reg->AI_DRAM_ADDR_REG, "AI_DRAM_ADDR_REG"); break;
        case 0x04500004:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveArmRegToVariable(Reg, &g_Reg->AI_LEN_REG, "AI_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            if (g_System->bFixedAudio())
            {
                MoveConstToArmReg(Arm_R0, (uint32_t)g_Audio, "g_Audio");
                CallFunction(AddressOf(&CAudio::LenChanged), "LenChanged");
            }
            else
            {
                CallFunction((void *)g_Plugins->Audio()->AiLenChanged, "g_Plugins->Audio()->LenChanged");
            }
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04500008:
            MoveArmRegToVariable(Reg, &g_Reg->AI_CONTROL_REG, "AI_CONTROL_REG");
            AndConstToVariable(&g_Reg->AI_CONTROL_REG, "AI_CONTROL_REG", 1);
        case 0x0450000C:
            /* Clear Interrupt */;
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_AI);
            AndConstToVariable(&g_Reg->m_AudioIntrReg, "m_AudioIntrReg", (uint32_t)~MI_INTR_AI);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04500010:
            m_RegWorkingSet.BeforeCallDirect();
            if (Reg != Arm_R2)
            {
                AddConstToArmReg(Arm_R2, Reg, 0);
            }
            MoveConstToArmReg(Arm_R1, PAddr);
            MoveConstToArmReg(Arm_R0, (uint32_t)(g_MMU), "g_MMU");
            CallFunction(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04500014: MoveArmRegToVariable(Reg, &g_Reg->AI_BITRATE_REG, "AI_BITRATE_REG"); break;
        default:
            MoveArmRegToVariable(Reg, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
        }
        break;
    case 0x04600000:
        switch (PAddr)
        {
        case 0x04600000: MoveArmRegToVariable(Reg, &g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG"); break;
        case 0x04600004:
            MoveArmRegToVariable(Reg, &g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG");
            if (g_Settings->LoadBool(Setting_EnableDisk))
            {
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction(AddressOf(&DiskDMACheck), "DiskDMACheck");
                m_RegWorkingSet.AfterCallDirect();
            }
            break;
        case 0x04600008:
            MoveArmRegToVariable(Reg, &g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::PI_DMA_READ), "CDMA::PI_DMA_READ");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0460000C:
            MoveArmRegToVariable(Reg, &g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CDMA *)g_MMU), "(CDMA *)g_MMU");
            CallFunction(AddressOf(&CDMA::PI_DMA_WRITE), "CDMA::PI_DMA_WRITE");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600010:
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_PI);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600014:
            MoveArmRegToVariable(Reg, &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG");
            AndConstToVariable(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", 0xFF);
            break;
        case 0x04600018:
            MoveArmRegToVariable(Reg, &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG");
            AndConstToVariable(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", 0xFF);
            break;
        case 0x0460001C:
            MoveArmRegToVariable(Reg, &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG");
            AndConstToVariable(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", 0xFF);
            break;
        case 0x04600020:
            MoveArmRegToVariable(Reg, &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG");
            AndConstToVariable(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", 0xFF);
            break;
        case 0x04600024:
            MoveArmRegToVariable(Reg, &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG");
            AndConstToVariable(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", 0xFF);
            break;
        case 0x04600028:
            MoveArmRegToVariable(Reg, &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG");
            AndConstToVariable(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", 0xFF);
            break;
        case 0x0460002C:
            MoveArmRegToVariable(Reg, &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG");
            AndConstToVariable(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", 0xFF);
            break;
        case 0x04600030:
            MoveArmRegToVariable(Reg, &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG");
            AndConstToVariable(&g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", 0xFF);
            break;
        default:
            CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04700000:
        switch (PAddr)
        {
        case 0x04700000: MoveArmRegToVariable(Reg, &g_Reg->RI_MODE_REG, "RI_MODE_REG"); break;
        case 0x04700004: MoveArmRegToVariable(Reg, &g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG"); break;
        case 0x0470000C: MoveArmRegToVariable(Reg, &g_Reg->RI_SELECT_REG, "RI_SELECT_REG"); break;
        case 0x04700010: MoveArmRegToVariable(Reg, &g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG"); break;
        default:
            CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x04800000:
        switch (PAddr)
        {
        case 0x04800000: MoveArmRegToVariable(Reg, &g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG"); break;
        case 0x04800004:
            MoveArmRegToVariable(Reg, &g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CPifRam *)g_MMU), "CPifRam *)g_MMU");
            CallFunction(AddressOf(&CPifRam::SI_DMA_READ), "CPifRam::SI_DMA_READ");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            MoveArmRegToVariable(Reg, &g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)((CPifRam *)g_MMU), "CPifRam *)g_MMU");
            CallFunction(AddressOf(&CPifRam::SI_DMA_WRITE), "CPifRam::SI_DMA_WRITE");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800018:
            AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_SI);
            AndConstToVariable(&g_Reg->SI_STATUS_REG, "SI_STATUS_REG", (uint32_t)~SI_STATUS_INTERRUPT);
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
            CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
            if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
            {
                g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        }
        break;
    case 0x05000000:
        //64DD Registers
        if (g_Settings->LoadBool(Setting_EnableDisk))
        {
            switch (PAddr)
            {
            case 0x05000500: MoveArmRegToVariable(Reg, &g_Reg->ASIC_DATA, "ASIC_DATA"); break;
            case 0x05000508:
                //ASIC_CMD
                MoveArmRegToVariable(Reg, &g_Reg->ASIC_CMD, "ASIC_CMD");
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction(AddressOf(&DiskCommand), "DiskCommand");
                m_RegWorkingSet.AfterCallDirect();
                OrConstToVariable(&g_Reg->ASIC_STATUS, "ASIC_STATUS", (uint32_t)DD_STATUS_MECHA_INT);
                OrConstToVariable(&g_Reg->FAKE_CAUSE_REGISTER, "FAKE_CAUSE_REGISTER", (uint32_t)CAUSE_IP3);
                m_RegWorkingSet.BeforeCallDirect();
                MoveConstToArmReg(Arm_R0, (uint32_t)g_Reg, "g_Reg");
                CallFunction(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x05000510:
                //ASIC_BM_CTL
                MoveArmRegToVariable(Reg, &g_Reg->ASIC_BM_CTL, "ASIC_BM_CTL");
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction(AddressOf(&DiskBMControl), "DiskBMControl");
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x05000518:
                break;
            case 0x05000520:
                m_RegWorkingSet.BeforeCallDirect();
                CallFunction(AddressOf(&DiskReset), "DiskReset");
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x05000528: MoveArmRegToVariable(Reg, &g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE"); break;
            case 0x05000530: MoveArmRegToVariable(Reg, &g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE"); break;
            case 0x05000548: MoveArmRegToVariable(Reg, &g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL"); break;
            }
            break;
        }
    case 0x1FC00000:
        MoveArmRegToVariable(Reg, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
        break;
    default:
        CPU_Message("    Should be moving %s in to %08X ?!?", ArmRegName(Reg), VAddr);
        if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
        {
            g_Notify->DisplayError(stdstr_f("%s\ntrying to store in %08X?", __FUNCTION__, VAddr).c_str());
        }
        if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    }
}

void CArmRecompilerOps::LB_KnownAddress(ArmReg Reg, uint32_t VAddr, bool SignExtend)
{
    m_RegWorkingSet.SetArmRegProtected(Reg, true);

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
        return;
    }

    uint32_t PAddr;
    if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    ArmReg TempReg = Arm_Unknown;
    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
    case 0x10000000:
        TempReg = Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempReg, (uint32_t)PAddr + (uint32_t)g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
        LoadArmRegPointerByteToArmReg(Reg, TempReg, 0);
        SignExtendByte(Reg);
        break;
    default:
        CPU_Message("    Should be loading from %08X ?!?", VAddr);
        if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
    }
}

void CArmRecompilerOps::LW_KnownAddress(ArmReg Reg, uint32_t VAddr)
{
    m_RegWorkingSet.SetArmRegProtected(Reg, true);

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        if (!g_System->bUseTlb())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }

        ArmReg TempReg = Map_TempReg(Arm_Any, -1, false);
        ArmReg TempRegAddress = Map_TempReg(Arm_Any, -1, false);
        MoveConstToArmReg(TempRegAddress, VAddr);
        ShiftRightUnsignImmed(TempReg, TempRegAddress, 12);
        ArmReg ReadMapReg = Map_Variable(CArmRegInfo::VARIABLE_TLB_READMAP);
        LoadArmRegPointerToArmReg(TempReg, ReadMapReg, TempReg, 2);
        CompileReadTLBMiss(TempRegAddress, TempReg);
        LoadArmRegPointerToArmReg(Reg, TempReg, TempRegAddress, 0);
        m_RegWorkingSet.SetArmRegProtected(TempReg, false);
        m_RegWorkingSet.SetArmRegProtected(TempRegAddress, false);
    }
    else
    {
        uint32_t PAddr;
        if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        ArmReg TempReg;
        switch (PAddr & 0xFFF00000)
        {
        case 0x00000000:
        case 0x00100000:
        case 0x00200000:
        case 0x00300000:
        case 0x00400000:
        case 0x00500000:
        case 0x00600000:
        case 0x00700000:
            TempReg = Map_TempReg(Arm_Any, -1, false);
            MoveConstToArmReg(TempReg, (uint32_t)PAddr + (uint32_t)g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str());
            LoadArmRegPointerToArmReg(Reg, TempReg, 0);
            break;
        case 0x04000000:
            if (PAddr < 0x04002000)
            {
                MoveVariableToArmReg(PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str(), Reg);
                break;
            }
            switch (PAddr)
            {
            case 0x04040010: MoveVariableToArmReg(&g_Reg->SP_STATUS_REG, "SP_STATUS_REG", Reg); break;
            case 0x04040014: MoveVariableToArmReg(&g_Reg->SP_DMA_FULL_REG, "SP_DMA_FULL_REG", Reg); break;
            case 0x04040018: MoveVariableToArmReg(&g_Reg->SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG", Reg); break;
            case 0x0404001C:
                MoveVariableToArmReg(&g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", Reg);
                MoveConstToVariable(1, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
                break;
            case 0x04080000: MoveVariableToArmReg(&g_Reg->SP_PC_REG, "SP_PC_REG", Reg); break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x04100000:
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R1, PAddr);
            MoveConstToArmReg(Arm_R2, (uint32_t)&CMipsMemoryVM::m_MemLookupAddress, "m_MemLookupAddress");
            StoreArmRegToArmRegPointer(Arm_R1, Arm_R2, 0);
            CallFunction((void *)CMipsMemoryVM::Load32DPCommand, "CMipsMemoryVM::Load32DPCommand");
            m_RegWorkingSet.AfterCallDirect();
            MoveVariableToArmReg(&CMipsMemoryVM::m_MemLookupValue.UW[0], "CMipsMemoryVM::m_MemLookupValue.UW[0]", Reg);
            break;
        case 0x04300000:
            switch (PAddr)
            {
            case 0x04300000: MoveVariableToArmReg(&g_Reg->MI_MODE_REG, "MI_MODE_REG", Reg); break;
            case 0x04300004: MoveVariableToArmReg(&g_Reg->MI_VERSION_REG, "MI_VERSION_REG", Reg); break;
            case 0x04300008: MoveVariableToArmReg(&g_Reg->MI_INTR_REG, "MI_INTR_REG", Reg); break;
            case 0x0430000C: MoveVariableToArmReg(&g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG", Reg); break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str()); }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x04400000:
            switch (PAddr)
            {
            case 0x04400010:
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
                UpdateCounters(m_RegWorkingSet, false, true);
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
                m_RegWorkingSet.BeforeCallDirect();
                MoveConstToArmReg(Arm_R0, (uint32_t)g_MMU);
                CallFunction(AddressOf(&CMipsMemoryVM::UpdateHalfLine), "CMipsMemoryVM::UpdateHalfLine");
                m_RegWorkingSet.AfterCallDirect();
                MoveVariableToArmReg((void *)&g_MMU->m_HalfLine, "MMU->m_HalfLine", Reg);
                break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str()); }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x04500000: /* AI registers */
            switch (PAddr)
            {
            case 0x04500004:
                if (g_System->bFixedAudio())
                {
                    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
                    UpdateCounters(m_RegWorkingSet, false, true);
                    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
                    m_RegWorkingSet.BeforeCallDirect();
                    MoveConstToArmReg(Arm_R0, (uint32_t)g_Audio, "g_Audio");
                    CallFunction(AddressOf(&CAudio::GetLength), "CAudio::GetLength");
                    MoveConstToArmReg(Arm_R1, (uint32_t)&m_TempValue, "m_TempValue");
                    StoreArmRegToArmRegPointer(Arm_R0, Arm_R1, 0);
                    m_RegWorkingSet.AfterCallDirect();
                    MoveVariableToArmReg(&m_TempValue, "m_TempValue", Reg);
                }
                else
                {
                    if (g_Plugins->Audio()->AiReadLength != NULL)
                    {
                        m_RegWorkingSet.BeforeCallDirect();
                        CallFunction((void *)g_Plugins->Audio()->AiReadLength, "AiReadLength");
                        MoveConstToArmReg(Arm_R1, (uint32_t)&m_TempValue, "m_TempValue");
                        StoreArmRegToArmRegPointer(Arm_R0, Arm_R1, 0);
                        m_RegWorkingSet.AfterCallDirect();
                        MoveVariableToArmReg(&m_TempValue, "m_TempValue", Reg);
                    }
                    else
                    {
                        MoveConstToArmReg(Reg, (uint32_t)0);
                    }
                }
                break;
            case 0x0450000C:
                if (g_System->bFixedAudio())
                {
                    m_RegWorkingSet.BeforeCallDirect();
                    MoveConstToArmReg(Arm_R0, (uint32_t)g_Audio, "g_Audio");
                    CallFunction(AddressOf(&CAudio::GetStatus), "CAudio::GetStatus");
                    MoveConstToArmReg(Arm_R1, (uint32_t)&m_TempValue, "m_TempValue");
                    StoreArmRegToArmRegPointer(Arm_R0, Arm_R1, 0);
                    m_RegWorkingSet.AfterCallDirect();
                    MoveVariableToArmReg(&m_TempValue, "m_TempValue", Reg);
                }
                else
                {
                    MoveVariableToArmReg(&g_Reg->AI_STATUS_REG, "AI_STATUS_REG", Reg);
                }
                break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str()); }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x04600000:
            switch (PAddr)
            {
            case 0x04600000: MoveVariableToArmReg(&g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG", Reg); break;
            case 0x04600004: MoveVariableToArmReg(&g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG", Reg); break;
            case 0x04600008: MoveVariableToArmReg(&g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG", Reg); break;
            case 0x0460000C: MoveVariableToArmReg(&g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG", Reg); break;
            case 0x04600010: MoveVariableToArmReg(&g_Reg->PI_STATUS_REG, "PI_STATUS_REG", Reg); break;
            case 0x04600014: MoveVariableToArmReg(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", Reg); break;
            case 0x04600018: MoveVariableToArmReg(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", Reg); break;
            case 0x0460001C: MoveVariableToArmReg(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", Reg); break;
            case 0x04600020: MoveVariableToArmReg(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", Reg); break;
            case 0x04600024: MoveVariableToArmReg(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", Reg); break;
            case 0x04600028: MoveVariableToArmReg(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", Reg); break;
            case 0x0460002C: MoveVariableToArmReg(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", Reg); break;
            case 0x04600030: MoveVariableToArmReg(&g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", Reg); break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x04700000:
            switch (PAddr)
            {
            case 0x0470000C: MoveVariableToArmReg(&g_Reg->RI_SELECT_REG, "RI_SELECT_REG", Reg); break;
            case 0x04700010: MoveVariableToArmReg(&g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG", Reg); break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x04800000:
            switch (PAddr)
            {
            case 0x04800000: MoveVariableToArmReg(&g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG", Reg); break;
            case 0x04800018: MoveVariableToArmReg(&g_Reg->SI_STATUS_REG, "SI_STATUS_REG", Reg); break;
            default:
                MoveConstToArmReg(Reg, (uint32_t)0);
                if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
            break;
        case 0x05000000:
            //64DD Registers
            if (g_Settings->LoadBool(Setting_EnableDisk))
            {
                switch (PAddr)
                {
                case 0x05000500: MoveVariableToArmReg(&g_Reg->ASIC_DATA, "ASIC_DATA", Reg); break;
                case 0x05000504: MoveVariableToArmReg(&g_Reg->ASIC_MISC_REG, "ASIC_MISC_REG", Reg); break;
                case 0x05000508:
                    MoveVariableToArmReg(&g_Reg->ASIC_STATUS, "ASIC_STATUS", Reg);
                    m_RegWorkingSet.BeforeCallDirect();
                    CallFunction(AddressOf(&DiskGapSectorCheck), "DiskGapSectorCheck");
                    m_RegWorkingSet.AfterCallDirect();
                    break;
                case 0x0500050C: MoveVariableToArmReg(&g_Reg->ASIC_CUR_TK, "ASIC_CUR_TK", Reg); break;
                case 0x05000510: MoveVariableToArmReg(&g_Reg->ASIC_BM_STATUS, "ASIC_BM_STATUS", Reg); break;
                case 0x05000514: MoveVariableToArmReg(&g_Reg->ASIC_ERR_SECTOR, "ASIC_ERR_SECTOR", Reg); break;
                case 0x05000518: MoveVariableToArmReg(&g_Reg->ASIC_SEQ_STATUS, "ASIC_SEQ_STATUS", Reg); break;
                case 0x0500051C: MoveVariableToArmReg(&g_Reg->ASIC_CUR_SECTOR, "ASIC_CUR_SECTOR", Reg); break;
                case 0x05000520: MoveVariableToArmReg(&g_Reg->ASIC_HARD_RESET, "ASIC_HARD_RESET", Reg); break;
                case 0x05000524: MoveVariableToArmReg(&g_Reg->ASIC_C1_S0, "ASIC_C1_S0", Reg); break;
                case 0x05000528: MoveVariableToArmReg(&g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE", Reg); break;
                case 0x0500052C: MoveVariableToArmReg(&g_Reg->ASIC_C1_S2, "ASIC_C1_S2", Reg); break;
                case 0x05000530: MoveVariableToArmReg(&g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE", Reg); break;
                case 0x05000534: MoveVariableToArmReg(&g_Reg->ASIC_C1_S4, "ASIC_C1_S4", Reg); break;
                case 0x05000538: MoveVariableToArmReg(&g_Reg->ASIC_C1_S6, "ASIC_C1_S6", Reg); break;
                case 0x0500053C: MoveVariableToArmReg(&g_Reg->ASIC_CUR_ADDR, "ASIC_CUR_ADDR", Reg); break;
                case 0x05000540: MoveVariableToArmReg(&g_Reg->ASIC_ID_REG, "ASIC_ID_REG", Reg); break;
                case 0x05000544: MoveVariableToArmReg(&g_Reg->ASIC_TEST_REG, "ASIC_TEST_REG", Reg); break;
                case 0x05000548: MoveVariableToArmReg(&g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL", Reg); break;
                default:
                    MoveConstToArmReg(Reg, (uint32_t)0);
                    if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
                    {
                        g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                    }
                    CPU_Message("    Should be loading from %08X ?!?", VAddr);
                    if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                }
            }
            else
            {
                MoveConstToArmReg(Reg, (uint32_t)((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF));
            }
            break;
        case 0x06000000:
            m_RegWorkingSet.BeforeCallDirect();
            MoveConstToArmReg(Arm_R1, PAddr);
            MoveConstToArmReg(Arm_R2, (uint32_t)&CMipsMemoryVM::m_MemLookupAddress, "m_MemLookupAddress");
            StoreArmRegToArmRegPointer(Arm_R1, Arm_R2, 0);
            CallFunction((void *)CMipsMemoryVM::Load32CartridgeDomain1Address1, "CMipsMemoryVM::Load32CartridgeDomain1Address1");
            m_RegWorkingSet.AfterCallDirect();
            MoveVariableToArmReg(&CMipsMemoryVM::m_MemLookupValue.UW[0], "CMipsMemoryVM::m_MemLookupValue.UW[0]", Reg);
            break;
        default:
            if ((PAddr & 0xF0000000) == 0x10000000 && (PAddr - 0x10000000) < g_Rom->GetRomSize())
            {
                uint32_t RomOffset = PAddr - 0x10000000;
                MoveVariableToArmReg(RomOffset + g_Rom->GetRomAddress(), stdstr_f("ROM + %X", RomOffset).c_str(), Reg); // read from rom
            }
            else
            {
                CPU_Message("    Should be loading from %08X ?!?", VAddr);
                if (bHaveDebugger()) { g_Notify->BreakPoint(__FILE__, __LINE__); }
            }
        }
    }
}

#endif