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
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Mips/OpCode.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/x86CodeLog.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Recompiler/LoopAnalysis.h>
#include <Project64-core/N64System/Recompiler/SectionInfo.h>
#include <Project64-core/ExceptionHandler.h>

void InPermLoop();

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

static bool DelaySlotEffectsJump(uint32_t JumpPC)
{
    OPCODE Command;

    if (!g_MMU->LW_VAddr(JumpPC, Command.Hex))
	{
        return true;
	}

    switch (Command.op)
    {
    case R4300i_SPECIAL:
        switch (Command.funct)
        {
        case R4300i_SPECIAL_JR: return DelaySlotEffectsCompare(JumpPC, Command.rs, 0);
        case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC, Command.rs, 31);
        }
        break;
    case R4300i_REGIMM:
        switch (Command.rt)
        {
        case R4300i_REGIMM_BLTZ:
        case R4300i_REGIMM_BGEZ:
        case R4300i_REGIMM_BLTZL:
        case R4300i_REGIMM_BGEZL:
        case R4300i_REGIMM_BLTZAL:
        case R4300i_REGIMM_BGEZAL:
            return DelaySlotEffectsCompare(JumpPC, Command.rs, 0);
        }
        break;
    case R4300i_JAL:
    case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC, 31, 0); break;
    case R4300i_J: return false;
    case R4300i_BEQ:
    case R4300i_BNE:
    case R4300i_BLEZ:
    case R4300i_BGTZ:
        return DelaySlotEffectsCompare(JumpPC, Command.rs, Command.rt);
    case R4300i_CP1:
        switch (Command.fmt)
        {
        case R4300i_COP1_BC:
            switch (Command.ft)
            {
            case R4300i_COP1_BC_BCF:
            case R4300i_COP1_BC_BCT:
            case R4300i_COP1_BC_BCFL:
            case R4300i_COP1_BC_BCTL:
                {
                    bool EffectDelaySlot = false;
                    OPCODE NewCommand;

                    if (!g_MMU->LW_VAddr(JumpPC + 4, NewCommand.Hex))
                    {
                        return true;
                    }

                    if (NewCommand.op == R4300i_CP1)
                    {
                        if (NewCommand.fmt == R4300i_COP1_S && (NewCommand.funct & 0x30) == 0x30)
                        {
                            EffectDelaySlot = true;
                        }
                        if (NewCommand.fmt == R4300i_COP1_D && (NewCommand.funct & 0x30) == 0x30)
                        {
                            EffectDelaySlot = true;
                        }
                    }
                    return EffectDelaySlot;
                }
                break;
            }
            break;
        }
        break;
    case R4300i_BEQL:
    case R4300i_BNEL:
    case R4300i_BLEZL:
    case R4300i_BGTZL:
        return DelaySlotEffectsCompare(JumpPC, Command.rs, Command.rt);
    }
    return true;
}

CCodeSection::CCodeSection(CCodeBlock * CodeBlock, uint32_t EnterPC, uint32_t ID, bool LinkAllowed) :
    m_BlockInfo(CodeBlock),
    m_SectionID(ID),
    m_EnterPC(EnterPC),
    m_EndPC((uint32_t)-1),
    m_ContinueSection(NULL),
    m_JumpSection(NULL),
    m_EndSection(false),
    m_LinkAllowed(LinkAllowed),
    m_Test(0),
    m_Test2(0),
    m_CompiledLocation(NULL),
    m_InLoop(false),
    m_DelaySlot(false)
{
    CPU_Message("%s: ID %d EnterPC 0x%08X", __FUNCTION__, ID, EnterPC);
}

CCodeSection::~CCodeSection()
{
}

void CCodeSection::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason, bool CompileNow, void(*x86Jmp)(const char * Label, uint32_t Value))
{
    if (!CompileNow)
    {
        char String[100];
        sprintf(String, "Exit_%d", m_BlockInfo->m_ExitInfo.size());
        if (x86Jmp == NULL)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        x86Jmp(String, 0);

        CExitInfo ExitInfo;
        ExitInfo.ID = m_BlockInfo->m_ExitInfo.size();
        ExitInfo.TargetPC = TargetPC;
        ExitInfo.ExitRegSet = ExitRegSet;
        ExitInfo.reason = reason;
        ExitInfo.NextInstruction = m_NextInstruction;
        ExitInfo.JumpLoc = (uint32_t *)(m_RecompPos - 4);
        m_BlockInfo->m_ExitInfo.push_back(ExitInfo);
        return;
    }

    //CPU_Message("CompileExit: %d",reason);
    ExitRegSet.WriteBackRegisters();

    if (TargetPC != (uint32_t)-1)
    {
        MoveConstToVariable(TargetPC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
        UpdateCounters(ExitRegSet, TargetPC <= JumpPC && JumpPC != -1, reason == CExitInfo::Normal);
    }
    else
    {
        UpdateCounters(ExitRegSet, false, reason == CExitInfo::Normal);
    }

    switch (reason)
    {
    case CExitInfo::Normal: case CExitInfo::Normal_NoSysCheck:
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
        if (g_SyncSystem)
        {
            MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
#ifdef LinkBlocks
        if (bSMM_ValidFunc == false)
        {
            if (LookUpMode() == FuncFind_ChangeMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                //			uint8_t * Jump, * Jump2;
                //			if (TargetPC >= 0x80000000 && TargetPC < 0xC0000000) {
                //				uint32_t pAddr = TargetPC & 0x1FFFFFFF;
                //
                //				MoveVariableToX86reg((uint8_t *)RDRAM + pAddr,"RDRAM + pAddr",x86_EAX);
                //				Jump2 = NULL;
                //			} else {
                //				MoveConstToX86reg((TargetPC >> 12),x86_ECX);
                //				MoveConstToX86reg(TargetPC,x86_EBX);
                //				MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",x86_ECX,x86_ECX,4);
                //				TestX86RegToX86Reg(x86_ECX,x86_ECX);
                //				JeLabel8("NoTlbEntry",0);
                //				Jump2 = m_RecompPos - 1;
                //				MoveX86regPointerToX86reg(x86_ECX, x86_EBX,x86_EAX);
                //			}
                //			MoveX86RegToX86Reg(x86_EAX,x86_ECX);
                //			AndConstToX86Reg(x86_ECX,0xFFFF0000);
                //			CompConstToX86reg(x86_ECX,0x7C7C0000);
                //			JneLabel8("NoCode",0);
                //			Jump = m_RecompPos - 1;
                //			AndConstToX86Reg(x86_EAX,0xFFFF);
                //			ShiftLeftSignImmed(x86_EAX,4);
                //			AddConstToX86Reg(x86_EAX,0xC);
                //			MoveVariableDispToX86Reg(OrigMem,"OrigMem",x86_ECX,x86_EAX,1);
                //			JmpDirectReg(x86_ECX);
                //			CPU_Message("      NoCode:");
                //			*((uint8_t *)(Jump))=(uint8_t)(m_RecompPos - Jump - 1);
                //			if (Jump2 != NULL) {
                //				CPU_Message("      NoTlbEntry:");
                //				*((uint8_t *)(Jump2))=(uint8_t)(m_RecompPos - Jump2 - 1);
                //			}
            }
            else if (LookUpMode() == FuncFind_VirtualLookup)
            {
                MoveConstToX86reg(TargetPC,x86_EDX);
                MoveConstToX86reg((uint32_t)&m_Functions,x86_ECX);
                Call_Direct(AddressOf(&CFunctionMap::CompilerFindFunction), "CFunctionMap::CompilerFindFunction");
                MoveX86RegToX86Reg(x86_EAX,x86_ECX);
                JecxzLabel8("NullPointer",0);
                uint8_t * Jump = m_RecompPos - 1;
                MoveX86PointerToX86regDisp(x86_EBX,x86_ECX,0xC);
                JmpDirectReg(x86_EBX);
                CPU_Message("      NullPointer:");
                *((uint8_t *)(Jump))=(uint8_t)(m_RecompPos - Jump - 1);
            }
            else if (LookUpMode() == FuncFind_PhysicalLookup)
            {
                uint8_t * Jump2 = NULL;
                if (TargetPC >= 0x80000000 && TargetPC < 0x90000000)
                {
                    uint32_t pAddr = TargetPC & 0x1FFFFFFF;
                    MoveVariableToX86reg((uint8_t *)JumpTable + pAddr,"JumpTable + pAddr",x86_ECX);
                }
                else if (TargetPC >= 0x90000000 && TargetPC < 0xC0000000)
                {
                }
                else
                {
                    MoveConstToX86reg((TargetPC >> 12),x86_ECX);
                    MoveConstToX86reg(TargetPC,x86_EBX);
                    MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",x86_ECX,x86_ECX,4);
                    TestX86RegToX86Reg(x86_ECX,x86_ECX);
                    JeLabel8("NoTlbEntry",0);
                    Jump2 = m_RecompPos - 1;
                    AddConstToX86Reg(x86_ECX,(uint32_t)JumpTable - (uint32_t)RDRAM);
                    MoveX86regPointerToX86reg(x86_ECX, x86_EBX,x86_ECX);
                }
                if (TargetPC < 0x90000000 || TargetPC >= 0xC0000000)
                {
                    JecxzLabel8("NullPointer",0);
                    uint8_t * Jump = m_RecompPos - 1;
                    MoveX86PointerToX86regDisp(x86_EAX,x86_ECX,0xC);
                    JmpDirectReg(x86_EAX);
                    CPU_Message("      NullPointer:");
                    *((uint8_t *)(Jump))=(uint8_t)(m_RecompPos - Jump - 1);
                    if (Jump2 != NULL)
                    {
                        CPU_Message("      NoTlbEntry:");
                        *((uint8_t *)(Jump2))=(uint8_t)(m_RecompPos - Jump2 - 1);
                    }
                }
            }
        }
        ExitCodeBlock();
#else
        ExitCodeBlock();
#endif
        break;
    case CExitInfo::DoCPU_Action:
        MoveConstToX86reg((uint32_t)g_SystemEvents, x86_ECX);
        Call_Direct(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
        if (g_SyncSystem)
        {
            MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        //g_System->SyncCPU(g_SyncSystem);
        ExitCodeBlock();
        break;
    case CExitInfo::DoSysCall:
        {
            bool bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
            PushImm32(bDelay ? "true" : "false", bDelay);
            MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
            Call_Direct(AddressOf(&CRegisters::DoSysCallException), "CRegisters::DoSysCallException");
            if (g_SyncSystem)
            {
                MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
                Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
            }
            ExitCodeBlock();
        }
        break;
    case CExitInfo::COP1_Unuseable:
        {
            bool bDelay = m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT;
            PushImm32("1", 1);
            PushImm32(bDelay ? "true" : "false", bDelay);
            MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
            Call_Direct(AddressOf(&CRegisters::DoCopUnusableException), "CRegisters::DoCopUnusableException");
            if (g_SyncSystem)
            {
                MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
                Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
            }
            ExitCodeBlock();
        }
        break;
    case CExitInfo::ExitResetRecompCode:
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
        if (m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT)
        {
            X86BreakPoint(__FILEW__,__LINE__);
        }
        if (g_SyncSystem)
        {
            MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        X86BreakPoint(__FILEW__,__LINE__);
        MoveVariableToX86reg(this,"this",x86_ECX);
        Call_Direct(AddressOf(ResetRecompCode), "ResetRecompCode");
#endif
        ExitCodeBlock();
        break;
    case CExitInfo::TLBReadMiss:
        MoveVariableToX86reg(g_TLBLoadAddress, "g_TLBLoadAddress", x86_EDX);
        Push(x86_EDX);
        PushImm32(m_NextInstruction == JUMP || m_NextInstruction == DELAY_SLOT);
        MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
        Call_Direct(AddressOf(&CRegisters::DoTLBReadMiss), "CRegisters::DoTLBReadMiss");
        if (g_SyncSystem)
        {
            MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        ExitCodeBlock();
        break;
    case CExitInfo::TLBWriteMiss:
        X86BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case CExitInfo::DivByZero:
        AddConstToVariable(4, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
        if (!g_System->b32BitCore())
        {
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
        MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
        if (g_SyncSystem)
        {
            MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        ExitCodeBlock();
        break;
    default:
        WriteTrace(TraceRecompiler, TraceError, "how did you want to exit on reason (%d) ???", reason);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CCodeSection::GenerateSectionLinkage()
{
    CCodeSection * TargetSection[] = { m_ContinueSection, m_JumpSection };
    CJumpInfo * JumpInfo[] = { &m_Cont, &m_Jump };
    int i;

    for (i = 0; i < 2; i++)
    {
        if (JumpInfo[i]->LinkLocation == NULL &&
            JumpInfo[i]->FallThrough == false)
        {
            JumpInfo[i]->TargetPC = (uint32_t)-1;
        }
    }

    if ((CompilePC() & 0xFFC) == 0xFFC)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
        //Handle Fall througth
        uint8_t * Jump = NULL;
        for (i = 0; i < 2; i ++)
        {
            if (!JumpInfo[i]->FallThrough) { continue; }
            JumpInfo[i]->FallThrough = false;
            if (JumpInfo[i]->LinkLocation != NULL)
            {
                SetJump32(JumpInfo[i]->LinkLocation,(uint32_t *)m_RecompPos);
                JumpInfo[i]->LinkLocation = NULL;
                if (JumpInfo[i]->LinkLocation2 != NULL)
                {
                    SetJump32(JumpInfo[i]->LinkLocation2,(uint32_t *)m_RecompPos);
                    JumpInfo[i]->LinkLocation2 = NULL;
                }
            }
            PushImm32(stdstr_f("0x%08X",JumpInfo[i]->TargetPC).c_str(),JumpInfo[i]->TargetPC);
            if (JumpInfo[(i + 1) & 1]->LinkLocation == NULL) { break; }
            JmpLabel8("FinishBlock",0);
            Jump = m_RecompPos - 1;
        }
        for (i = 0; i < 2; i ++)
        {
            if (JumpInfo[i]->LinkLocation == NULL) { continue; }
            JumpInfo[i]->FallThrough = false;
            if (JumpInfo[i]->LinkLocation != NULL)
            {
                SetJump32(JumpInfo[i]->LinkLocation,(uint32_t *)m_RecompPos);
                JumpInfo[i]->LinkLocation = NULL;
                if (JumpInfo[i]->LinkLocation2 != NULL)
                {
                    SetJump32(JumpInfo[i]->LinkLocation2,(uint32_t *)m_RecompPos);
                    JumpInfo[i]->LinkLocation2 = NULL;
                }
            }
            PushImm32(stdstr_f("0x%08X",JumpInfo[i]->TargetPC).c_str(),JumpInfo[i]->TargetPC);
            if (JumpInfo[(i + 1) & 1]->LinkLocation == NULL) { break; }
            JmpLabel8("FinishBlock",0);
            Jump = m_RecompPos - 1;
        }
        if (Jump != NULL)
        {
            CPU_Message("      $FinishBlock:");
            SetJump8(Jump,m_RecompPos);
        }
        //MoveConstToVariable(CompilePC() + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet,false,true);
        //		WriteBackRegisters(Section);
        //		if (g_SyncSystem) {
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        //}
        //	MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
        PushImm32(stdstr_f("0x%08X",CompilePC() + 4).c_str(),CompilePC() + 4);

        // check if there is an existing section

        MoveConstToX86reg((uint32_t)g_Recompiler,x86_ECX);
        Call_Direct(AddressOf(&CRecompiler::CompileDelaySlot), "CRecompiler::CompileDelaySlot");
        JmpDirectReg(x86_EAX);
        ExitCodeBlock();
        return;
#endif
    }

    // Handle Perm Loop
    if (CRecompilerOps::m_CompilePC == m_Jump.TargetPC && (m_Cont.FallThrough == false))
    {
        if (!DelaySlotEffectsJump(CompilePC()))
        {
            MoveConstToVariable(CompilePC(), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            m_Jump.RegSet.WriteBackRegisters();
            UpdateCounters(m_Jump.RegSet, false, true);
            Call_Direct(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
            MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
            Call_Direct(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
            CPU_Message("CompileSystemCheck 3");
            CompileSystemCheck((uint32_t)-1, m_Jump.RegSet);
        }
    }
    if (TargetSection[0] != TargetSection[1] || TargetSection[0] == NULL)
    {
        for (i = 0; i < 2; i++)
        {
            if (JumpInfo[i]->LinkLocation == NULL && JumpInfo[i]->FallThrough == false)
            {
                if (TargetSection[i])
                {
                    TargetSection[i]->UnlinkParent(this, i == 0);
                    TargetSection[i] = NULL;
                }
            }
            else if (TargetSection[i] == NULL && JumpInfo[i]->FallThrough)
            {
                if (JumpInfo[i]->LinkLocation != NULL)
                {
                    SetJump32(JumpInfo[i]->LinkLocation, (uint32_t *)m_RecompPos);
                    JumpInfo[i]->LinkLocation = NULL;
                    if (JumpInfo[i]->LinkLocation2 != NULL)
                    {
                        SetJump32(JumpInfo[i]->LinkLocation2, (uint32_t *)m_RecompPos);
                        JumpInfo[i]->LinkLocation2 = NULL;
                    }
                }
                CompileExit(JumpInfo[i]->JumpPC, JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet, JumpInfo[i]->ExitReason, true, NULL);
                JumpInfo[i]->FallThrough = false;
            }
            else if (TargetSection[i] != NULL && JumpInfo[i] != NULL)
            {
                if (!JumpInfo[i]->FallThrough) { continue; }
                if (JumpInfo[i]->TargetPC == TargetSection[i]->m_EnterPC) { continue; }
                if (JumpInfo[i]->LinkLocation != NULL)
                {
                    SetJump32(JumpInfo[i]->LinkLocation, (uint32_t *)m_RecompPos);
                    JumpInfo[i]->LinkLocation = NULL;
                    if (JumpInfo[i]->LinkLocation2 != NULL)
                    {
                        SetJump32(JumpInfo[i]->LinkLocation2, (uint32_t *)m_RecompPos);
                        JumpInfo[i]->LinkLocation2 = NULL;
                    }
                }
                CompileExit(JumpInfo[i]->JumpPC, JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet, JumpInfo[i]->ExitReason, true, NULL);
                //FreeSection(TargetSection[i],Section);
            }
        }
    }
    else
    {
        if (m_Cont.LinkLocation == NULL && m_Cont.FallThrough == false) { m_ContinueSection = NULL; }
        if (m_Jump.LinkLocation == NULL && m_Jump.FallThrough == false) { m_JumpSection = NULL; }
        if (m_JumpSection == NULL &&  m_ContinueSection == NULL)
        {
            //FreeSection(TargetSection[0],Section);
        }
    }

    TargetSection[0] = m_ContinueSection;
    TargetSection[1] = m_JumpSection;

    for (i = 0; i < 2; i++) {
        if (TargetSection[i] == NULL) { continue; }
        if (!JumpInfo[i]->FallThrough) { continue; }

        if (TargetSection[i]->m_CompiledLocation != NULL)
        {
            char Label[100];
            sprintf(Label, "Section_%d", TargetSection[i]->m_SectionID);
            JumpInfo[i]->FallThrough = false;
            if (JumpInfo[i]->LinkLocation != NULL)
            {
                SetJump32(JumpInfo[i]->LinkLocation, (uint32_t *)m_RecompPos);
                JumpInfo[i]->LinkLocation = NULL;
                if (JumpInfo[i]->LinkLocation2 != NULL)
                {
                    SetJump32(JumpInfo[i]->LinkLocation2, (uint32_t *)m_RecompPos);
                    JumpInfo[i]->LinkLocation2 = NULL;
                }
            }
            if (JumpInfo[i]->TargetPC <= CompilePC())
            {
                if (JumpInfo[i]->PermLoop)
                {
                    CPU_Message("PermLoop *** 1");
                    MoveConstToVariable(JumpInfo[i]->TargetPC, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
                    UpdateCounters(JumpInfo[i]->RegSet, false, true);
                    if (g_SyncSystem)
                    {
                        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
                        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
                    }

                    //JumpInfo[i]->RegSet.BlockCycleCount() -= g_System->CountPerOp();
                    Call_Direct(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
                    //JumpInfo[i]->RegSet.BlockCycleCount() += g_System->CountPerOp();
                    UpdateCounters(JumpInfo[i]->RegSet, true, true);
                    CPU_Message("CompileSystemCheck 4");
                    CompileSystemCheck((uint32_t)-1, JumpInfo[i]->RegSet);
                }
                else
                {
                    UpdateCounters(JumpInfo[i]->RegSet, true, true);
                    CPU_Message("CompileSystemCheck 5");
                    CompileSystemCheck(JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet);
                }
            }
            else
            {
                UpdateCounters(JumpInfo[i]->RegSet, false, true);
            }

            JumpInfo[i]->RegSet.SetBlockCycleCount(0);
            m_RegWorkingSet = JumpInfo[i]->RegSet;
            SyncRegState(TargetSection[i]->m_RegEnter);
            JmpLabel32(Label, 0);
            SetJump32((uint32_t *)m_RecompPos - 1, (uint32_t *)(TargetSection[i]->m_CompiledLocation));
        }
    }

    for (i = 0; i < 2; i++)
    {
        if (TargetSection[i] == NULL) { continue; }
        if (TargetSection[i]->m_ParentSection.empty()) { continue; }
        for (SECTION_LIST::iterator iter = TargetSection[i]->m_ParentSection.begin(); iter != TargetSection[i]->m_ParentSection.end(); iter++)
        {
            CCodeSection * Parent = *iter;

            if (Parent->m_CompiledLocation != NULL) { continue; }
            if (Parent->m_InLoop) { continue; }
            if (JumpInfo[i]->PermLoop)
            {
                CPU_Message("PermLoop *** 2");
                MoveConstToVariable(JumpInfo[i]->TargetPC, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
                UpdateCounters(JumpInfo[i]->RegSet, false, true);
                Call_Direct(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
                UpdateCounters(JumpInfo[i]->RegSet, true, true);
                CPU_Message("CompileSystemCheck 6");
                CompileSystemCheck((uint32_t)-1, JumpInfo[i]->RegSet);
            }
            if (JumpInfo[i]->FallThrough)
            {
                JumpInfo[i]->FallThrough = false;
                JmpLabel32(JumpInfo[i]->BranchLabel.c_str(), 0);
                JumpInfo[i]->LinkLocation = (uint32_t*)(m_RecompPos - 4);
            }
        }
    }

    for (i = 0; i < 2; i++)
    {
        if (JumpInfo[i]->FallThrough)
        {
            if (JumpInfo[i]->TargetPC < CompilePC())
            {
                UpdateCounters(JumpInfo[i]->RegSet, true, true);
                CPU_Message("CompileSystemCheck 7");
                CompileSystemCheck(JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet);
            }
        }
    }

    CPU_Message("====== End of Section %d ======", m_SectionID);

    for (i = 0; i < 2; i++)
    {
        if (JumpInfo[i]->FallThrough && !TargetSection[i]->GenerateX86Code(m_BlockInfo->NextTest()))
        {
            JumpInfo[i]->FallThrough = false;
            JmpLabel32(JumpInfo[i]->BranchLabel.c_str(), 0);
            JumpInfo[i]->LinkLocation = (uint32_t *)(m_RecompPos - 4);
        }
    }

    //CPU_Message("Section %d",m_SectionID);
    for (i = 0; i < 2; i++)
    {
        if (JumpInfo[i]->LinkLocation == NULL) { continue; }
        if (TargetSection[i] == NULL)
        {
            CPU_Message("ExitBlock (from %d):", m_SectionID);
            SetJump32(JumpInfo[i]->LinkLocation, (uint32_t *)m_RecompPos);
            JumpInfo[i]->LinkLocation = NULL;
            if (JumpInfo[i]->LinkLocation2 != NULL)
            {
                SetJump32(JumpInfo[i]->LinkLocation2, (uint32_t *)m_RecompPos);
                JumpInfo[i]->LinkLocation2 = NULL;
            }
            CompileExit(JumpInfo[i]->JumpPC, JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet, JumpInfo[i]->ExitReason, true, NULL);
            continue;
        }
        if (JumpInfo[i]->TargetPC != TargetSection[i]->m_EnterPC)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (TargetSection[i]->m_CompiledLocation == NULL)
        {
            TargetSection[i]->GenerateX86Code(m_BlockInfo->NextTest());
        }
        else
        {
            stdstr_f Label("Section_%d (from %d):", TargetSection[i]->m_SectionID, m_SectionID);

            CPU_Message(Label.c_str());
            SetJump32(JumpInfo[i]->LinkLocation, (uint32_t *)m_RecompPos);
            JumpInfo[i]->LinkLocation = NULL;
            if (JumpInfo[i]->LinkLocation2 != NULL)
            {
                SetJump32(JumpInfo[i]->LinkLocation2, (uint32_t *)m_RecompPos);
                JumpInfo[i]->LinkLocation2 = NULL;
            }
            m_RegWorkingSet = JumpInfo[i]->RegSet;
            if (JumpInfo[i]->TargetPC <= JumpInfo[i]->JumpPC)
            {
                UpdateCounters(JumpInfo[i]->RegSet, true, true);
                if (JumpInfo[i]->PermLoop)
                {
                    CPU_Message("PermLoop *** 3");
                    MoveConstToVariable(JumpInfo[i]->TargetPC, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
                    Call_Direct(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
                    CPU_Message("CompileSystemCheck 8");
                    CompileSystemCheck((uint32_t)-1, JumpInfo[i]->RegSet);
                }
                else
                {
                    CPU_Message("CompileSystemCheck 9");
                    CompileSystemCheck(JumpInfo[i]->TargetPC, JumpInfo[i]->RegSet);
                }
            }
            else
            {
                UpdateCounters(m_RegWorkingSet, false, true);
            }
            m_RegWorkingSet = JumpInfo[i]->RegSet;
            SyncRegState(TargetSection[i]->m_RegEnter);
            JmpLabel32(Label.c_str(), 0);
            SetJump32((uint32_t *)m_RecompPos - 1, (uint32_t *)(TargetSection[i]->m_CompiledLocation));
        }
    }
}

void CCodeSection::SyncRegState(const CRegInfo & SyncTo)
{
    ResetX86Protection();

    bool changed = false;
    UnMap_AllFPRs();
    if (m_RegWorkingSet.GetRoundingModel() != SyncTo.GetRoundingModel()) { m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown); }
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

    for (int i = 1; i < 32; i++)
    {
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

        switch (SyncTo.GetMipsRegState(i))
        {
        case CRegInfo::STATE_UNKNOWN: UnMap_GPR(i, true);  break;
        case CRegInfo::STATE_MAPPED_64:
            {
                x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
                x86Reg x86RegHi = SyncTo.GetMipsRegMapHi(i);
                UnMap_X86reg(Reg);
                UnMap_X86reg(x86RegHi);
                switch (GetMipsRegState(i))
                {
                case CRegInfo::STATE_UNKNOWN:
                    MoveVariableToX86reg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg);
                    MoveVariableToX86reg(&_GPR[i].UW[1], CRegName::GPR_Hi[i], x86RegHi);
                    break;
                case CRegInfo::STATE_MAPPED_64:
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    MoveX86RegToX86Reg(GetMipsRegMapHi(i), x86RegHi);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapHi(i), CRegInfo::NotMapped);
                    break;
                case CRegInfo::STATE_MAPPED_32_SIGN:
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), x86RegHi);
                    ShiftRightSignImmed(x86RegHi, 31);
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    break;
                case CRegInfo::STATE_MAPPED_32_ZERO:
                    XorX86RegToX86Reg(x86RegHi, x86RegHi);
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    break;
                case CRegInfo::STATE_CONST_64:
                    MoveConstToX86reg(GetMipsRegHi(i), x86RegHi);
                    MoveConstToX86reg(GetMipsRegLo(i), Reg);
                    break;
                case CRegInfo::STATE_CONST_32_SIGN:
                    MoveConstToX86reg(GetMipsRegLo_S(i) >> 31, x86RegHi);
                    MoveConstToX86reg(GetMipsRegLo(i), Reg);
                    break;
                default:
                    CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d", GetMipsRegState(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    continue;
                }
                m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
                m_RegWorkingSet.SetMipsRegMapHi(i, x86RegHi);
                m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_64);
                m_RegWorkingSet.SetX86Mapped(Reg, CRegInfo::GPR_Mapped);
                m_RegWorkingSet.SetX86Mapped(x86RegHi, CRegInfo::GPR_Mapped);
                m_RegWorkingSet.SetX86MapOrder(Reg, 1);
                m_RegWorkingSet.SetX86MapOrder(x86RegHi, 1);
            }
            break;
        case CRegInfo::STATE_MAPPED_32_SIGN:
            {
                x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
                UnMap_X86reg(Reg);
                switch (GetMipsRegState(i))
                {
                case CRegInfo::STATE_UNKNOWN: MoveVariableToX86reg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg); break;
                case CRegInfo::STATE_CONST_32_SIGN: MoveConstToX86reg(GetMipsRegLo(i), Reg); break;
                case CRegInfo::STATE_MAPPED_32_SIGN:
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    break;
                case CRegInfo::STATE_MAPPED_32_ZERO:
                    if (GetMipsRegMapLo(i) != Reg)
                    {
                        MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                        m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    }
                    break;
                case CRegInfo::STATE_MAPPED_64:
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapHi(i), CRegInfo::NotMapped);
                    break;
                case CRegInfo::STATE_CONST_64:
                    CPU_Message("hi %X\nLo %X", GetMipsRegHi(i), GetMipsRegLo(i));
                default:
                    CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d", GetMipsRegState(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
                m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_32_SIGN);
                m_RegWorkingSet.SetX86Mapped(Reg, CRegInfo::GPR_Mapped);
                m_RegWorkingSet.SetX86MapOrder(Reg, 1);
            }
            break;
        case CRegInfo::STATE_MAPPED_32_ZERO:
            {
                x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
                UnMap_X86reg(Reg);
                switch (GetMipsRegState(i))
                {
                case CRegInfo::STATE_MAPPED_64:
                case CRegInfo::STATE_UNKNOWN:
                    MoveVariableToX86reg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg);
                    break;
                case CRegInfo::STATE_MAPPED_32_ZERO:
                    MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                    m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                    break;
                case CRegInfo::STATE_MAPPED_32_SIGN:
                    if (g_System->b32BitCore())
                    {
                        MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                        m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
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
                    MoveConstToX86reg(GetMipsRegLo(i), Reg);
                    break;
                default:
                    CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
                m_RegWorkingSet.SetMipsRegState(i, SyncTo.GetMipsRegState(i));
                m_RegWorkingSet.SetX86Mapped(Reg, CRegInfo::GPR_Mapped);
                m_RegWorkingSet.SetX86MapOrder(Reg, 1);
            }
            break;
        default:
            CPU_Message("%d - %d reg: %s (%d)", SyncTo.GetMipsRegState(i), GetMipsRegState(i), CRegName::GPR[i], i);
            g_Notify->BreakPoint(__FILE__, __LINE__);
            changed = false;
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
    m_Jump.BranchLabel.Format("0x%08X", TargetPC);
    m_Jump.PermLoop = PermLoop;
}

void CCodeSection::SetContinueAddress(uint32_t JumpPC, uint32_t TargetPC)
{
    m_Cont.JumpPC = JumpPC;
    m_Cont.TargetPC = TargetPC;
    m_Cont.BranchLabel.Format("0x%08X", TargetPC);
}

void CCodeSection::CompileCop1Test()
{
    if (m_RegWorkingSet.FpuBeenUsed())
        return;

    TestVariable(STATUS_CU1, &g_Reg->STATUS_REGISTER, "STATUS_REGISTER");
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::COP1_Unuseable, false, JeLabel32);
    m_RegWorkingSet.FpuBeenUsed() = true;
}

bool CCodeSection::ParentContinue()
{
    if (m_ParentSection.size() > 0)
    {
        for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
        {
            CCodeSection * Parent = *iter;
            if (Parent->m_CompiledLocation != NULL) { continue; }
            if (IsAllParentLoops(Parent, true, m_BlockInfo->NextTest())) { continue; }
            return false;
        }
        if (!InheritParentInfo())
        {
            return false;
        }
    }
    return true;
}

/*int TestValue = 0;
void TestFunc()
{
TestValue += 1;
if (TestValue >= 4)
{
g_Notify->BreakPoint(__FILE__, __LINE__);
}
}*/

bool CCodeSection::GenerateX86Code(uint32_t Test)
{
    if (this == NULL) { return false; }

    if (m_CompiledLocation != NULL)
    {
        if (m_Test == Test)
        {
            return false;
        }
        m_Test = Test;
        if (m_ContinueSection->GenerateX86Code(Test)) { return true; }
        if (m_JumpSection->GenerateX86Code(Test)) { return true; }
        return false;
    }

    if (!ParentContinue())
    {
        return false;
    }
    m_CompiledLocation = m_RecompPos;
    m_RegWorkingSet = m_RegEnter;
    m_CompilePC = m_EnterPC;
    m_NextInstruction = m_DelaySlot ? JUMP : NORMAL;
    m_Section = this;

    if (m_CompilePC < m_BlockInfo->VAddrFirst())
    {
        m_BlockInfo->SetVAddrFirst(m_CompilePC);
    }

    uint32_t ContinueSectionPC = m_ContinueSection ? m_ContinueSection->m_EnterPC : (uint32_t)-1;

    do
    {
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

        if (m_CompilePC > m_BlockInfo->VAddrLast())
        {
            m_BlockInfo->SetVAddrLast(m_CompilePC);
        }
        /*if (m_CompilePC == 0x803245C4)
        {
        X86BreakPoint(__FILEW__,__LINE__);
        //m_RegWorkingSet.UnMap_AllFPRs();
        }*/

        /*if (m_CompilePC >= 0x80000000 && m_CompilePC <= 0x80400000 && m_NextInstruction == NORMAL)
        {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet,false,true);
        MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        if (g_SyncSystem) {
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        }*/

        /*if ((m_CompilePC == 0x8031C0E4 || m_CompilePC == 0x8031C118 ||
        m_CompilePC == 0x8031CD88 ||  m_CompilePC == 0x8031CE24 ||
        m_CompilePC == 0x8031CE30 || m_CompilePC == 0x8031CE40) && m_NextInstruction == NORMAL)
        {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet,false,true);
        MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        if (g_SyncSystem) {
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        }*/

        /*if (m_CompilePC == 0x801C1B88)
        {
        BeforeCallDirect(m_RegWorkingSet);
        Call_Direct(AddressOf(TestFunc), "TestFunc");
        AfterCallDirect(m_RegWorkingSet);
        }*/

        /*if (m_CompilePC >= 0x801C1AF8 && m_CompilePC <= 0x801C1C00 && m_NextInstruction == NORMAL)
        {
        UpdateCounters(m_RegWorkingSet,false,true);
        MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        if (g_SyncSystem) {
        BeforeCallDirect(m_RegWorkingSet);
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystemPC), "CN64System::SyncSystemPC");
        AfterCallDirect(m_RegWorkingSet);
        }
        }*/

        /*if ((m_CompilePC == 0x80263900) && m_NextInstruction == NORMAL)
        {
        X86BreakPoint(__FILEW__,__LINE__);
        }*/

        /*if ((m_CompilePC >= 0x80325D80 && m_CompilePC <= 0x80325DF0) && m_NextInstruction == NORMAL)
        {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet,false,true);
        MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        if (g_SyncSystem) {
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        }*/
        /*if ((m_CompilePC == 0x80324E14) && m_NextInstruction == NORMAL)
        {
        X86BreakPoint(__FILEW__,__LINE__);
        }*/

        /*if (m_CompilePC == 0x80324E18 && m_NextInstruction == NORMAL)
        {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet,false,true);
        MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        if (g_SyncSystem) {
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        }*/
        /*if (m_CompilePC >= 0x80324E00 && m_CompilePC <= 0x80324E18 && m_NextInstruction == NORMAL)
        {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet,false,true);
        MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
        if (g_SyncSystem) {
        MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        }
        }*/
        /*		if (m_CompilePC == 0x803245CC && m_NextInstruction == NORMAL)
        {
        //m_RegWorkingSet.UnMap_AllFPRs();
        g_Notify->BreakPoint(__FILE__, __LINE__);
        //X86HardBreakPoint();
        //X86BreakPoint(__FILEW__,__LINE__);
        //m_RegWorkingSet.UnMap_AllFPRs();
        }*/
        /*if (m_CompilePC >= 0x80179DC4 && m_CompilePC <= 0x80179DF0 && m_NextInstruction == NORMAL)
        {
        m_RegWorkingSet.UnMap_AllFPRs();
        }*/

        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        m_RegWorkingSet.ResetX86Protection();

        switch (m_Opcode.op)
        {
        case R4300i_SPECIAL:
            switch (m_Opcode.funct)
            {
            case R4300i_SPECIAL_SLL: SPECIAL_SLL(); break;
            case R4300i_SPECIAL_SRL: SPECIAL_SRL(); break;
            case R4300i_SPECIAL_SRA: SPECIAL_SRA(); break;
            case R4300i_SPECIAL_SLLV: SPECIAL_SLLV(); break;
            case R4300i_SPECIAL_SRLV: SPECIAL_SRLV(); break;
            case R4300i_SPECIAL_SRAV: SPECIAL_SRAV(); break;
            case R4300i_SPECIAL_JR: SPECIAL_JR(); break;
            case R4300i_SPECIAL_JALR: SPECIAL_JALR(); break;
            case R4300i_SPECIAL_MFLO: SPECIAL_MFLO(); break;
            case R4300i_SPECIAL_SYSCALL: SPECIAL_SYSCALL(); break;
            case R4300i_SPECIAL_MTLO: SPECIAL_MTLO(); break;
            case R4300i_SPECIAL_MFHI: SPECIAL_MFHI(); break;
            case R4300i_SPECIAL_MTHI: SPECIAL_MTHI(); break;
            case R4300i_SPECIAL_DSLLV: SPECIAL_DSLLV(); break;
            case R4300i_SPECIAL_DSRLV: SPECIAL_DSRLV(); break;
            case R4300i_SPECIAL_DSRAV: SPECIAL_DSRAV(); break;
            case R4300i_SPECIAL_MULT: SPECIAL_MULT(); break;
            case R4300i_SPECIAL_DIV: SPECIAL_DIV(); break;
            case R4300i_SPECIAL_DIVU: SPECIAL_DIVU(); break;
            case R4300i_SPECIAL_MULTU: SPECIAL_MULTU(); break;
            case R4300i_SPECIAL_DMULT: SPECIAL_DMULT(); break;
            case R4300i_SPECIAL_DMULTU: SPECIAL_DMULTU(); break;
            case R4300i_SPECIAL_DDIV: SPECIAL_DDIV(); break;
            case R4300i_SPECIAL_DDIVU: SPECIAL_DDIVU(); break;
            case R4300i_SPECIAL_ADD: SPECIAL_ADD(); break;
            case R4300i_SPECIAL_ADDU: SPECIAL_ADDU(); break;
            case R4300i_SPECIAL_SUB: SPECIAL_SUB(); break;
            case R4300i_SPECIAL_SUBU: SPECIAL_SUBU(); break;
            case R4300i_SPECIAL_AND: SPECIAL_AND(); break;
            case R4300i_SPECIAL_OR: SPECIAL_OR(); break;
            case R4300i_SPECIAL_XOR: SPECIAL_XOR(); break;
            case R4300i_SPECIAL_NOR: SPECIAL_NOR(); break;
            case R4300i_SPECIAL_SLT: SPECIAL_SLT(); break;
            case R4300i_SPECIAL_SLTU: SPECIAL_SLTU(); break;
            case R4300i_SPECIAL_DADD: SPECIAL_DADD(); break;
            case R4300i_SPECIAL_DADDU: SPECIAL_DADDU(); break;
            case R4300i_SPECIAL_DSUB: SPECIAL_DSUB(); break;
            case R4300i_SPECIAL_DSUBU: SPECIAL_DSUBU(); break;
            case R4300i_SPECIAL_DSLL: SPECIAL_DSLL(); break;
            case R4300i_SPECIAL_DSRL: SPECIAL_DSRL(); break;
            case R4300i_SPECIAL_DSRA: SPECIAL_DSRA(); break;
            case R4300i_SPECIAL_DSLL32: SPECIAL_DSLL32(); break;
            case R4300i_SPECIAL_DSRL32: SPECIAL_DSRL32(); break;
            case R4300i_SPECIAL_DSRA32: SPECIAL_DSRA32(); break;
            default:
                UnknownOpcode(); break;
            }
            break;
        case R4300i_REGIMM:
            switch (m_Opcode.rt)
            {
            case R4300i_REGIMM_BLTZ:Compile_Branch(BLTZ_Compare, BranchTypeRs, false); break;
            case R4300i_REGIMM_BGEZ:Compile_Branch(BGEZ_Compare, BranchTypeRs, false); break;
            case R4300i_REGIMM_BLTZL:Compile_BranchLikely(BLTZ_Compare, false); break;
            case R4300i_REGIMM_BGEZL:Compile_BranchLikely(BGEZ_Compare, false); break;
            case R4300i_REGIMM_BLTZAL:Compile_Branch(BLTZ_Compare, BranchTypeRs, true); break;
            case R4300i_REGIMM_BGEZAL:Compile_Branch(BGEZ_Compare, BranchTypeRs, true); break;
            default:
                UnknownOpcode(); break;
            }
            break;
        case R4300i_BEQ: Compile_Branch(BEQ_Compare, BranchTypeRsRt, false); break;
        case R4300i_BNE: Compile_Branch(BNE_Compare, BranchTypeRsRt, false); break;
        case R4300i_BGTZ:Compile_Branch(BGTZ_Compare, BranchTypeRs, false); break;
        case R4300i_BLEZ:Compile_Branch(BLEZ_Compare, BranchTypeRs, false); break;
        case R4300i_J: J(); break;
        case R4300i_JAL: JAL(); break;
        case R4300i_ADDI: ADDI(); break;
        case R4300i_ADDIU: ADDIU(); break;
        case R4300i_SLTI: SLTI(); break;
        case R4300i_SLTIU: SLTIU(); break;
        case R4300i_ANDI: ANDI(); break;
        case R4300i_ORI: ORI(); break;
        case R4300i_XORI: XORI(); break;
        case R4300i_LUI: LUI(); break;
        case R4300i_CP0:
            switch (m_Opcode.rs)
            {
            case R4300i_COP0_MF: COP0_MF(); break;
            case R4300i_COP0_MT: COP0_MT(); break;
            default:
                if ((m_Opcode.rs & 0x10) != 0)
                {
                    switch (m_Opcode.funct)
                    {
                    case R4300i_COP0_CO_TLBR: COP0_CO_TLBR(); break;
                    case R4300i_COP0_CO_TLBWI: COP0_CO_TLBWI(); break;
                    case R4300i_COP0_CO_TLBWR: COP0_CO_TLBWR(); break;
                    case R4300i_COP0_CO_TLBP: COP0_CO_TLBP(); break;
                    case R4300i_COP0_CO_ERET: COP0_CO_ERET(); break;
                    default: UnknownOpcode(); break;
                    }
                }
                else
                {
                    UnknownOpcode();
                }
            }
            break;
        case R4300i_CP1:
            switch (m_Opcode.rs)
            {
            case R4300i_COP1_MF: COP1_MF(); break;
            case R4300i_COP1_DMF: COP1_DMF(); break;
            case R4300i_COP1_CF: COP1_CF(); break;
            case R4300i_COP1_MT: COP1_MT(); break;
            case R4300i_COP1_DMT: COP1_DMT(); break;
            case R4300i_COP1_CT: COP1_CT(); break;
            case R4300i_COP1_BC:
                switch (m_Opcode.ft)
                {
                case R4300i_COP1_BC_BCF: Compile_Branch(COP1_BCF_Compare, BranchTypeCop1, false); break;
                case R4300i_COP1_BC_BCT: Compile_Branch(COP1_BCT_Compare, BranchTypeCop1, false); break;
                case R4300i_COP1_BC_BCFL: Compile_BranchLikely(COP1_BCF_Compare, false); break;
                case R4300i_COP1_BC_BCTL: Compile_BranchLikely(COP1_BCT_Compare, false); break;
                default:
                    UnknownOpcode(); break;
                }
                break;
            case R4300i_COP1_S:
                switch (m_Opcode.funct)
                {
                case R4300i_COP1_FUNCT_ADD: COP1_S_ADD(); break;
                case R4300i_COP1_FUNCT_SUB: COP1_S_SUB(); break;
                case R4300i_COP1_FUNCT_MUL: COP1_S_MUL(); break;
                case R4300i_COP1_FUNCT_DIV: COP1_S_DIV(); break;
                case R4300i_COP1_FUNCT_ABS: COP1_S_ABS(); break;
                case R4300i_COP1_FUNCT_NEG: COP1_S_NEG(); break;
                case R4300i_COP1_FUNCT_SQRT: COP1_S_SQRT(); break;
                case R4300i_COP1_FUNCT_MOV: COP1_S_MOV(); break;
                case R4300i_COP1_FUNCT_TRUNC_L: COP1_S_TRUNC_L(); break;
                case R4300i_COP1_FUNCT_CEIL_L: COP1_S_CEIL_L(); break;	//added by Witten
                case R4300i_COP1_FUNCT_FLOOR_L: COP1_S_FLOOR_L(); break;	//added by Witten
                case R4300i_COP1_FUNCT_ROUND_W: COP1_S_ROUND_W(); break;
                case R4300i_COP1_FUNCT_TRUNC_W: COP1_S_TRUNC_W(); break;
                case R4300i_COP1_FUNCT_CEIL_W: COP1_S_CEIL_W(); break;	//added by Witten
                case R4300i_COP1_FUNCT_FLOOR_W: COP1_S_FLOOR_W(); break;
                case R4300i_COP1_FUNCT_CVT_D: COP1_S_CVT_D(); break;
                case R4300i_COP1_FUNCT_CVT_W: COP1_S_CVT_W(); break;
                case R4300i_COP1_FUNCT_CVT_L: COP1_S_CVT_L(); break;
                case R4300i_COP1_FUNCT_C_F:   case R4300i_COP1_FUNCT_C_UN:
                case R4300i_COP1_FUNCT_C_EQ:  case R4300i_COP1_FUNCT_C_UEQ:
                case R4300i_COP1_FUNCT_C_OLT: case R4300i_COP1_FUNCT_C_ULT:
                case R4300i_COP1_FUNCT_C_OLE: case R4300i_COP1_FUNCT_C_ULE:
                case R4300i_COP1_FUNCT_C_SF:  case R4300i_COP1_FUNCT_C_NGLE:
                case R4300i_COP1_FUNCT_C_SEQ: case R4300i_COP1_FUNCT_C_NGL:
                case R4300i_COP1_FUNCT_C_LT:  case R4300i_COP1_FUNCT_C_NGE:
                case R4300i_COP1_FUNCT_C_LE:  case R4300i_COP1_FUNCT_C_NGT:
                    COP1_S_CMP(); break;
                default:
                    UnknownOpcode(); break;
                }
                break;
            case R4300i_COP1_D:
                switch (m_Opcode.funct)
                {
                case R4300i_COP1_FUNCT_ADD: COP1_D_ADD(); break;
                case R4300i_COP1_FUNCT_SUB: COP1_D_SUB(); break;
                case R4300i_COP1_FUNCT_MUL: COP1_D_MUL(); break;
                case R4300i_COP1_FUNCT_DIV: COP1_D_DIV(); break;
                case R4300i_COP1_FUNCT_ABS: COP1_D_ABS(); break;
                case R4300i_COP1_FUNCT_NEG: COP1_D_NEG(); break;
                case R4300i_COP1_FUNCT_SQRT: COP1_D_SQRT(); break;
                case R4300i_COP1_FUNCT_MOV: COP1_D_MOV(); break;
                case R4300i_COP1_FUNCT_TRUNC_L: COP1_D_TRUNC_L(); break;	//added by Witten
                case R4300i_COP1_FUNCT_CEIL_L: COP1_D_CEIL_L(); break;	//added by Witten
                case R4300i_COP1_FUNCT_FLOOR_L: COP1_D_FLOOR_L(); break;	//added by Witten
                case R4300i_COP1_FUNCT_ROUND_W: COP1_D_ROUND_W(); break;
                case R4300i_COP1_FUNCT_TRUNC_W: COP1_D_TRUNC_W(); break;
                case R4300i_COP1_FUNCT_CEIL_W: COP1_D_CEIL_W(); break;	//added by Witten
                case R4300i_COP1_FUNCT_FLOOR_W: COP1_D_FLOOR_W(); break;	//added by Witten
                case R4300i_COP1_FUNCT_CVT_S: COP1_D_CVT_S(); break;
                case R4300i_COP1_FUNCT_CVT_W: COP1_D_CVT_W(); break;
                case R4300i_COP1_FUNCT_CVT_L: COP1_D_CVT_L(); break;
                case R4300i_COP1_FUNCT_C_F:   case R4300i_COP1_FUNCT_C_UN:
                case R4300i_COP1_FUNCT_C_EQ:  case R4300i_COP1_FUNCT_C_UEQ:
                case R4300i_COP1_FUNCT_C_OLT: case R4300i_COP1_FUNCT_C_ULT:
                case R4300i_COP1_FUNCT_C_OLE: case R4300i_COP1_FUNCT_C_ULE:
                case R4300i_COP1_FUNCT_C_SF:  case R4300i_COP1_FUNCT_C_NGLE:
                case R4300i_COP1_FUNCT_C_SEQ: case R4300i_COP1_FUNCT_C_NGL:
                case R4300i_COP1_FUNCT_C_LT:  case R4300i_COP1_FUNCT_C_NGE:
                case R4300i_COP1_FUNCT_C_LE:  case R4300i_COP1_FUNCT_C_NGT:
                    COP1_D_CMP(); break;
                default:
                    UnknownOpcode(); break;
                }
                break;
            case R4300i_COP1_W:
                switch (m_Opcode.funct)
                {
                case R4300i_COP1_FUNCT_CVT_S: COP1_W_CVT_S(); break;
                case R4300i_COP1_FUNCT_CVT_D: COP1_W_CVT_D(); break;
                default:
                    UnknownOpcode(); break;
                }
                break;
            case R4300i_COP1_L:
                switch (m_Opcode.funct)
                {
                case R4300i_COP1_FUNCT_CVT_S: COP1_L_CVT_S(); break;
                case R4300i_COP1_FUNCT_CVT_D: COP1_L_CVT_D(); break;
                default:
                    UnknownOpcode(); break;
                }
                break;
            default:
                UnknownOpcode(); break;
            }
            break;
        case R4300i_BEQL: Compile_BranchLikely(BEQ_Compare, false); break;
        case R4300i_BNEL: Compile_BranchLikely(BNE_Compare, false); break;
        case R4300i_BGTZL:Compile_BranchLikely(BGTZ_Compare, false); break;
        case R4300i_BLEZL:Compile_BranchLikely(BLEZ_Compare, false); break;
        case R4300i_DADDIU: DADDIU(); break;
        case R4300i_LDL: g_MMU->Compile_LDL(); break;
        case R4300i_LDR: g_MMU->Compile_LDR(); break;
        case R4300i_LB: g_MMU->Compile_LB(); break;
        case R4300i_LH: g_MMU->Compile_LH(); break;
        case R4300i_LWL: g_MMU->Compile_LWL(); break;
        case R4300i_LW: g_MMU->Compile_LW(); break;
        case R4300i_LBU: g_MMU->Compile_LBU(); break;
        case R4300i_LHU: g_MMU->Compile_LHU(); break;
        case R4300i_LWR: g_MMU->Compile_LWR(); break;
        case R4300i_LWU: g_MMU->Compile_LWU(); break;	//added by Witten
        case R4300i_SB: g_MMU->Compile_SB(); break;
        case R4300i_SH: g_MMU->Compile_SH(); break;
        case R4300i_SWL: g_MMU->Compile_SWL(); break;
        case R4300i_SW: g_MMU->Compile_SW(); break;
        case R4300i_SWR: g_MMU->Compile_SWR(); break;
        case R4300i_SDL: g_MMU->Compile_SDL(); break;
        case R4300i_SDR: g_MMU->Compile_SDR(); break;
        case R4300i_CACHE: CACHE(); break;
        case R4300i_LL: g_MMU->Compile_LL(); break;
        case R4300i_LWC1: g_MMU->Compile_LWC1(); break;
        case R4300i_LDC1: g_MMU->Compile_LDC1(); break;
        case R4300i_SC: g_MMU->Compile_SC(); break;
        case R4300i_LD: g_MMU->Compile_LD(); break;
        case R4300i_SWC1: g_MMU->Compile_SWC1(); break;
        case R4300i_SDC1: g_MMU->Compile_SDC1(); break;
        case R4300i_SD: g_MMU->Compile_SD(); break;
        default:
            UnknownOpcode(); break;
        }

        if (!g_System->bRegCaching()) { m_RegWorkingSet.WriteBackRegisters(); }
        m_RegWorkingSet.UnMap_AllFPRs();

        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (m_NextInstruction == DO_DELAY_SLOT)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_NextInstruction == NORMAL)
            {
                if (m_DelaySlot)
                {
                    CompileExit(m_CompilePC, m_Jump.TargetPC, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
                }
                else
                {
                    CompileExit(m_CompilePC, m_CompilePC + 4, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
                }
                m_NextInstruction = END_BLOCK;
            }
        }

        switch (m_NextInstruction)
        {
        case NORMAL:
            m_CompilePC += 4;
            break;
        case DO_DELAY_SLOT:
            m_NextInstruction = DELAY_SLOT;
            m_CompilePC += 4;
            break;
        case DELAY_SLOT:
            m_NextInstruction = DELAY_SLOT_DONE;
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            m_CompilePC -= 4;
            break;
        }

        if (m_DelaySlot)
        {
            if ((CompilePC() & 0xFFC) != 0xFFC && m_Jump.JumpPC != (uint32_t)-1)
            {
                m_CompilePC = m_Jump.JumpPC;
                m_Jump.RegSet = m_RegWorkingSet;
                m_Jump.FallThrough = true;
                GenerateSectionLinkage();
            }
            else
            {
                CompileExit(m_Jump.JumpPC, m_Jump.TargetPC, m_RegWorkingSet, CExitInfo::Normal, true, NULL);
            }
            m_NextInstruction = END_BLOCK;
        }
        else if (m_NextInstruction != END_BLOCK && m_CompilePC == ContinueSectionPC)
        {
            if (m_NextInstruction != NORMAL)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_CompilePC -= 4;
            m_Cont.RegSet = m_RegWorkingSet;
            m_Cont.FallThrough = true;
            m_Cont.JumpPC = m_CompilePC;
            GenerateSectionLinkage();
            m_NextInstruction = END_BLOCK;
        }
    } while (m_NextInstruction != END_BLOCK);
    return true;
}

void CCodeSection::AddParent(CCodeSection * Parent)
{
    if (this == NULL) { return; }
    if (Parent == NULL)
    {
        m_RegWorkingSet = m_RegEnter;
        return;
    }

    // check to see if we already have the parent in the list
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
            g_Notify->DisplayError("How are these sections joined?????");
        }
        m_RegWorkingSet = m_RegEnter;
    }
    else {
        if (Parent->m_ContinueSection == this)
        {
            TestRegConstantStates(Parent->m_Cont.RegSet, m_RegEnter);
        }
        if (Parent->m_JumpSection == this)
        {
            TestRegConstantStates(Parent->m_Jump.RegSet, m_RegEnter);
        }
        m_RegWorkingSet = m_RegEnter;
    }
}

void CCodeSection::SwitchParent(CCodeSection * OldParent, CCodeSection * NewParent)
{
    if (this == NULL) { return; }

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
    if (this == NULL) { return; }

    if (m_SectionID == TestID)
    {
        if (m_Test2 != Test2)
        {
            m_Test2 = Test2;
            m_ContinueSection->DetermineLoop(Test, Test2, TestID);
            m_JumpSection->DetermineLoop(Test, Test2, TestID);

            if (m_Test != Test)
            {
                m_Test = Test;
                if (m_ContinueSection != NULL)
                {
                    m_ContinueSection->DetermineLoop(Test, m_BlockInfo->NextTest(), m_ContinueSection->m_SectionID);
                }
                if (m_JumpSection != NULL)
                {
                    m_JumpSection->DetermineLoop(Test, m_BlockInfo->NextTest(), m_JumpSection->m_SectionID);
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
            m_ContinueSection->DetermineLoop(Test, Test2, TestID);
            m_JumpSection->DetermineLoop(Test, Test2, TestID);
        }
    }
}

CCodeSection * CCodeSection::ExistingSection(uint32_t Addr, uint32_t Test)
{
    if (this == NULL) { return NULL; }
    if (m_EnterPC == Addr && m_LinkAllowed)
    {
        return this;
    }
    if (m_Test == Test) { return NULL; }
    m_Test = Test;

    CCodeSection * Section = m_JumpSection->ExistingSection(Addr, Test);
    if (Section != NULL) { return Section; }
    Section = m_ContinueSection->ExistingSection(Addr, Test);
    if (Section != NULL) { return Section; }

    return NULL;
}

bool CCodeSection::SectionAccessible(uint32_t SectionId, uint32_t Test)
{
    if (this == NULL) { return false; }
    if (m_SectionID == SectionId)
    {
        return true;
    }

    if (m_Test == Test) { return false; }
    m_Test = Test;

    if (m_ContinueSection->SectionAccessible(SectionId, Test))
    {
        return true;
    }
    return m_JumpSection->SectionAccessible(SectionId, Test);
}

void CCodeSection::UnlinkParent(CCodeSection * Parent, bool ContinueSection)
{
    if (this == NULL)
    {
        return;
    }

    CPU_Message("%s: Section %d Parent: %d ContinueSection = %s", __FUNCTION__, m_SectionID, Parent->m_SectionID, ContinueSection ? "Yes" : "No");
    if (Parent->m_ContinueSection == this && Parent->m_JumpSection == this)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    SECTION_LIST::iterator iter = m_ParentSection.begin();
    while (iter != m_ParentSection.end())
    {
        CCodeSection * ParentIter = *iter;
        if (ParentIter == Parent && (Parent->m_ContinueSection != this || Parent->m_JumpSection != this))
        {
            m_ParentSection.erase(iter);
            iter = m_ParentSection.begin();
        }
        else
        {
            iter++;
        }
    }

    if (ContinueSection && Parent->m_ContinueSection == this)
    {
        Parent->m_ContinueSection = NULL;
    }

    if (!ContinueSection && Parent->m_JumpSection == this)
    {
        Parent->m_JumpSection = NULL;
    }

    bool bRemove = false;
    if (m_ParentSection.size() > 0)
    {
        if (!m_BlockInfo->SectionAccessible(m_SectionID))
        {
            for (SECTION_LIST::iterator iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
            {
                CCodeSection * ParentIter = *iter;
                if (ParentIter->m_ContinueSection == this)
                {
                    if (ParentIter->m_CompiledLocation)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    ParentIter->m_ContinueSection = NULL;
                }

                if (ParentIter->m_JumpSection == this)
                {
                    if (ParentIter->m_CompiledLocation)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    ParentIter->m_JumpSection = NULL;
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
        if (m_JumpSection != NULL)
        {
            m_JumpSection->UnlinkParent(this, false);
        }
        if (m_ContinueSection != NULL)
        {
            m_ContinueSection->UnlinkParent(this, true);
        }
    }
}

bool CCodeSection::IsAllParentLoops(CCodeSection * Parent, bool IgnoreIfCompiled, uint32_t Test)
{
    if (IgnoreIfCompiled && Parent->m_CompiledLocation != NULL) { return true; }
    if (!m_InLoop) { return false; }
    if (!Parent->m_InLoop) { return false; }
    if (Parent->m_ParentSection.empty()) { return false; }
    if (this == Parent) { return true; }
    if (Parent->m_Test == Test) { return true; }
    Parent->m_Test = Test;

    for (SECTION_LIST::iterator iter = Parent->m_ParentSection.begin(); iter != Parent->m_ParentSection.end(); iter++)
    {
        CCodeSection * ParentSection = *iter;
        if (!IsAllParentLoops(ParentSection, IgnoreIfCompiled, Test)) { return false; }
    }
    return true;
}

bool CCodeSection::SetupRegisterForLoop()
{
    CRegInfo OriginalReg = m_RegWorkingSet;
    if (!LoopAnalysis(m_BlockInfo, this).SetupRegisterForLoop())
    {
        return false;
    }
    for (int i = 1; i < 32; i++)
    {
        if (OriginalReg.GetMipsRegState(i) != m_RegEnter.GetMipsRegState(i))
        {
            UnMap_GPR(i, true);
        }
    }
    return true;
}

bool CCodeSection::InheritParentInfo()
{
    if (m_CompiledLocation == NULL)
    {
        m_CompiledLocation = m_RecompPos;
        DisplaySectionInformation();
        m_CompiledLocation = NULL;
    }
    else
    {
        DisplaySectionInformation();
    }

    if (m_ParentSection.empty())
    {
        m_RegWorkingSet = m_RegEnter;
        return true;
    }

    if (m_ParentSection.size() == 1)
    {
        CCodeSection * Parent = *(m_ParentSection.begin());
        if (Parent->m_CompiledLocation == NULL)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        CJumpInfo * JumpInfo = this == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;

        m_RegEnter = JumpInfo->RegSet;
        if (JumpInfo->LinkLocation != NULL)
        {
            CPU_Message("   Section_%d:", m_SectionID);
            SetJump32(JumpInfo->LinkLocation, (uint32_t *)m_RecompPos);
            if (JumpInfo->LinkLocation2 != NULL)
            {
                SetJump32(JumpInfo->LinkLocation2, (uint32_t *)m_RecompPos);
            }
        }
        m_RegWorkingSet = m_RegEnter;
        return true;
    }

    //Multiple Parents
    BLOCK_PARENT_LIST ParentList;
    SECTION_LIST::iterator iter;
    for (iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (Parent->m_CompiledLocation == NULL) { continue; }
        if (Parent->m_JumpSection != Parent->m_ContinueSection)
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = this == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;
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
    for (iter = m_ParentSection.begin(); iter != m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (Parent->m_CompiledLocation != NULL) { continue; }
        if (Parent->m_JumpSection != Parent->m_ContinueSection)
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = this == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;
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

    m_RegWorkingSet = JumpInfo->RegSet;
    m_RegWorkingSet.ResetX86Protection();

    if (JumpInfo->LinkLocation != NULL)
    {
        CPU_Message("   Section_%d (from %d):", m_SectionID, Parent->m_SectionID);
        SetJump32(JumpInfo->LinkLocation, (uint32_t *)m_RecompPos);
        JumpInfo->LinkLocation = NULL;
        if (JumpInfo->LinkLocation2 != NULL)
        {
            SetJump32(JumpInfo->LinkLocation2, (uint32_t *)m_RecompPos);
            JumpInfo->LinkLocation2 = NULL;
        }
    }

    if (JumpInfo->ExitReason == CExitInfo::Normal_NoSysCheck)
    {
        if (JumpInfo->RegSet.GetBlockCycleCount() != 0)
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
        UpdateCounters(m_RegWorkingSet, m_EnterPC < JumpInfo->JumpPC, true);
        if (JumpInfo->JumpPC == (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (m_EnterPC <= JumpInfo->JumpPC)
        {
            CPU_Message("CompileSystemCheck 10");
            CompileSystemCheck(m_EnterPC, m_RegWorkingSet);
        }
    }
    JumpInfo->FallThrough = false;

    //Fix up initial state
    UnMap_AllFPRs();

    //determine loop reg usage
    if (m_InLoop && ParentList.size() > 1)
    {
        if (!SetupRegisterForLoop()) { return false; }
        m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
    }

    for (size_t i = 0; i < ParentList.size(); i++)
    {
        x86Reg MemoryStackPos;
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
        MemoryStackPos = x86_Unknown;
        for (i2 = 0; i2 < sizeof(x86_Registers) / sizeof(x86_Registers[0]); i2++)
        {
            if (RegSet->GetX86Mapped(x86_Registers[i2]) == CRegInfo::Stack_Mapped)
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
        }

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
            ResetX86Protection();
        }

        if (MemoryStackPos > 0)
        {
            Map_MemoryStack(MemoryStackPos, true);
        }
    }
    m_RegEnter = m_RegWorkingSet;

    //Sync registers for different blocks
    stdstr_f Label("Section_%d", m_SectionID);
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

        for (i2 = 0; !NeedSync && i2 < 8; i2++)
        {
            if (m_RegWorkingSet.FpuMappedTo(i2) == (uint32_t)-1)
            {
                NeedSync = true;
            }
        }

        for (i2 = 0; !NeedSync && i2 < sizeof(x86_Registers) / sizeof(x86_Registers[0]); i2++)
        {
            if (m_RegWorkingSet.GetX86Mapped(x86_Registers[i2]) == CRegInfo::Stack_Mapped)
            {
                if (m_RegWorkingSet.GetX86Mapped(x86_Registers[i2]) != RegSet->GetX86Mapped(x86_Registers[i2]))
                {
                    NeedSync = true;
                }
                break;
            }
        }
        for (i2 = 0; !NeedSync && i2 < 32; i2++)
        {
            if (NeedSync == true)  { break; }
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
        JmpLabel32(Label.c_str(), 0);
        JumpInfo->LinkLocation = (uint32_t *)(m_RecompPos - 4);
        JumpInfo->LinkLocation2 = NULL;

        CurrentParent = i;
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        CPU_Message("   Section_%d (from %d):", m_SectionID, Parent->m_SectionID);
        if (JumpInfo->LinkLocation != NULL)
        {
            SetJump32(JumpInfo->LinkLocation, (uint32_t *)m_RecompPos);
            JumpInfo->LinkLocation = NULL;
            if (JumpInfo->LinkLocation2 != NULL)
            {
                SetJump32(JumpInfo->LinkLocation2, (uint32_t *)m_RecompPos);
                JumpInfo->LinkLocation2 = NULL;
            }
        }
        //if (m_EnterPC == 0x8031CE44 && m_SectionID == 6)
        //{
        //	g_Notify->BreakPoint(__FILE__, __LINE__);
        //}
        m_RegWorkingSet = JumpInfo->RegSet;
        if (m_EnterPC < JumpInfo->JumpPC)
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            CPU_Message("CompileSystemCheck 11");
            CompileSystemCheck(m_EnterPC, m_RegWorkingSet);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, false, true);
        }
        SyncRegState(m_RegEnter); 		//Sync
        m_RegEnter = m_RegWorkingSet;
    }

    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;

        if (JumpInfo->LinkLocation != NULL)
        {
            SetJump32(JumpInfo->LinkLocation, (uint32_t *)m_RecompPos);
            JumpInfo->LinkLocation = NULL;
            if (JumpInfo->LinkLocation2 != NULL)
            {
                SetJump32(JumpInfo->LinkLocation2, (uint32_t *)m_RecompPos);
                JumpInfo->LinkLocation2 = NULL;
            }
        }
    }

    CPU_Message("   Section_%d:", m_SectionID);
    m_RegWorkingSet.SetBlockCycleCount(0);
    return true;
}

bool CCodeSection::DisplaySectionInformation(uint32_t ID, uint32_t Test)
{
    if (!bX86Logging)
    {
        return false;
    }
    if (this == NULL) { return false; }
    if (m_Test == Test) { return false; }
    m_Test = Test;
    if (m_SectionID != ID)
    {
        if (m_ContinueSection->DisplaySectionInformation(ID, Test)) { return true; }
        if (m_JumpSection->DisplaySectionInformation(ID, Test)) { return true; }
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

    CPU_Message("====== Section %d ======", m_SectionID);
    CPU_Message("Start PC: %X", m_EnterPC);
    CPU_Message("End PC: %X", m_EndPC);
    CPU_Message("CompiledLocation: %X", m_CompiledLocation);
    if (!m_ParentSection.empty())
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
        CPU_Message("Number of parents: %d (%s)", m_ParentSection.size(), ParentList.c_str());
    }

    CPU_Message("Jump Address: 0x%08X", m_Jump.JumpPC);
    CPU_Message("Jump Target Address: 0x%08X", m_Jump.TargetPC);
    if (m_JumpSection != NULL)
    {
        CPU_Message("Jump Section: %d", m_JumpSection->m_SectionID);
    }
    else
    {
        CPU_Message("Jump Section: None");
    }
    CPU_Message("Continue Address: 0x%08X", m_Cont.JumpPC);
    CPU_Message("Continue Target Address: 0x%08X", m_Cont.TargetPC);
    if (m_ContinueSection != NULL) {
        CPU_Message("Continue Section: %d", m_ContinueSection->m_SectionID);
    }
    else
    {
        CPU_Message("Continue Section: None");
    }
    CPU_Message("In Loop: %s", m_InLoop ? "Yes" : "No");
    CPU_Message("=======================");
}