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
#include "InterpreterCPU.h"

#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps32.h>
#include <Project64-core/Plugins/PluginClass.h>
#include <Project64-core/Plugins/GFXPlugin.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/Debugger.h>

R4300iOp::Func * CInterpreterCPU::m_R4300i_Opcode = NULL;

void ExecuteInterpreterOps(uint32_t /*Cycles*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2)
{
    OPCODE Command;

    if (!g_MMU->LW_VAddr(PC + 4, Command.Hex))
    {
        //g_Notify->DisplayError("Failed to load word 2");
        //ExitThread(0);
        return true;
    }

    switch (Command.op)
    {
    case R4300i_SPECIAL:
        switch (Command.funct)
        {
        case R4300i_SPECIAL_SLL:
        case R4300i_SPECIAL_SRL:
        case R4300i_SPECIAL_SRA:
        case R4300i_SPECIAL_SLLV:
        case R4300i_SPECIAL_SRLV:
        case R4300i_SPECIAL_SRAV:
        case R4300i_SPECIAL_MFHI:
        case R4300i_SPECIAL_MTHI:
        case R4300i_SPECIAL_MFLO:
        case R4300i_SPECIAL_MTLO:
        case R4300i_SPECIAL_DSLLV:
        case R4300i_SPECIAL_DSRLV:
        case R4300i_SPECIAL_DSRAV:
        case R4300i_SPECIAL_ADD:
        case R4300i_SPECIAL_ADDU:
        case R4300i_SPECIAL_SUB:
        case R4300i_SPECIAL_SUBU:
        case R4300i_SPECIAL_AND:
        case R4300i_SPECIAL_OR:
        case R4300i_SPECIAL_XOR:
        case R4300i_SPECIAL_NOR:
        case R4300i_SPECIAL_SLT:
        case R4300i_SPECIAL_SLTU:
        case R4300i_SPECIAL_DADD:
        case R4300i_SPECIAL_DADDU:
        case R4300i_SPECIAL_DSUB:
        case R4300i_SPECIAL_DSUBU:
        case R4300i_SPECIAL_DSLL:
        case R4300i_SPECIAL_DSRL:
        case R4300i_SPECIAL_DSRA:
        case R4300i_SPECIAL_DSLL32:
        case R4300i_SPECIAL_DSRL32:
        case R4300i_SPECIAL_DSRA32:
            if (Command.rd == 0)
            {
                return false;
            }
            if (Command.rd == Reg1 || Command.rd == Reg2)
            {
                return true;
            }
            break;
        case R4300i_SPECIAL_MULT:
        case R4300i_SPECIAL_MULTU:
        case R4300i_SPECIAL_DIV:
        case R4300i_SPECIAL_DIVU:
        case R4300i_SPECIAL_DMULT:
        case R4300i_SPECIAL_DMULTU:
        case R4300i_SPECIAL_DDIV:
        case R4300i_SPECIAL_DDIVU:
            break;
        default:
            if (CDebugSettings::bHaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("Does %s effect Delay slot at %X?", R4300iOpcodeName(Command.Hex, PC + 4), PC).c_str());
            }
            return true;
        }
        break;
    case R4300i_CP0:
        switch (Command.rs)
        {
        case R4300i_COP0_MT: break;
        case R4300i_COP0_MF:
            if (Command.rt == 0)
            {
                return false;
            }
            if (Command.rt == Reg1 || Command.rt == Reg2)
            {
                return true;
            }
            break;
        default:
            if ((Command.rs & 0x10) != 0)
            {
                switch (Command.funct)
                {
                case R4300i_COP0_CO_TLBR: break;
                case R4300i_COP0_CO_TLBWI: break;
                case R4300i_COP0_CO_TLBWR: break;
                case R4300i_COP0_CO_TLBP: break;
                default:
                    if (CDebugSettings::bHaveDebugger())
                    {
                        g_Notify->DisplayError(stdstr_f("Does %s effect Delay slot at %X?\n6", R4300iOpcodeName(Command.Hex, PC + 4), PC).c_str());
                    }
                    return true;
                }
            }
            else
            {
                if (CDebugSettings::bHaveDebugger())
                {
                    g_Notify->DisplayError(stdstr_f("Does %s effect Delay slot at %X?\n7", R4300iOpcodeName(Command.Hex, PC + 4), PC).c_str());
                }
                return true;
            }
        }
        break;
    case R4300i_CP1:
        switch (Command.fmt)
        {
        case R4300i_COP1_MF:
            if (Command.rt == 0)
            {
                return false;
            }
            if (Command.rt == Reg1 || Command.rt == Reg2)
            {
                return true;
            }
            break;
        case R4300i_COP1_CF: break;
        case R4300i_COP1_MT: break;
        case R4300i_COP1_CT: break;
        case R4300i_COP1_S: break;
        case R4300i_COP1_D: break;
        case R4300i_COP1_W: break;
        case R4300i_COP1_L: break;
        default:
            if (CDebugSettings::bHaveDebugger())
            {
                g_Notify->DisplayError(stdstr_f("Does %s effect Delay slot at %X?", R4300iOpcodeName(Command.Hex, PC + 4), PC).c_str());
            }
            return true;
        }
        break;
    case R4300i_ANDI:
    case R4300i_ORI:
    case R4300i_XORI:
    case R4300i_LUI:
    case R4300i_ADDI:
    case R4300i_ADDIU:
    case R4300i_SLTI:
    case R4300i_SLTIU:
    case R4300i_DADDI:
    case R4300i_DADDIU:
    case R4300i_LB:
    case R4300i_LH:
    case R4300i_LW:
    case R4300i_LWL:
    case R4300i_LWR:
    case R4300i_LDL:
    case R4300i_LDR:
    case R4300i_LBU:
    case R4300i_LHU:
    case R4300i_LD:
    case R4300i_LWC1:
    case R4300i_LDC1:
        if (Command.rt == 0)
        {
            return false;
        }
        if (Command.rt == Reg1 || Command.rt == Reg2)
        {
            return true;
        }
        break;
    case R4300i_CACHE: break;
    case R4300i_SB: break;
    case R4300i_SH: break;
    case R4300i_SW: break;
    case R4300i_SWR: break;
    case R4300i_SWL: break;
    case R4300i_SWC1: break;
    case R4300i_SDC1: break;
    case R4300i_SD: break;
    default:
        if (CDebugSettings::bHaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("Does %s effect Delay slot at %X?", R4300iOpcodeName(Command.Hex, PC + 4), PC).c_str());
        }
        return true;
    }
    return false;
}

void CInterpreterCPU::BuildCPU()
{
    R4300iOp::m_TestTimer = false;
    R4300iOp::m_NextInstruction = NORMAL;
    R4300iOp::m_JumpToLocation = 0;

    if (g_Settings->LoadBool(Game_32Bit))
    {
        m_R4300i_Opcode = R4300iOp32::BuildInterpreter();
    }
    else
    {
        m_R4300i_Opcode = R4300iOp::BuildInterpreter();
    }
}

void CInterpreterCPU::InPermLoop()
{
    // *** Changed ***/
    //if (CPU_Type == CPU_SyncCores)
    //{
    //	SyncRegisters.CP0[9] +=5;
    //}

    /* Interrupts enabled */
    if ((g_Reg->STATUS_REGISTER & STATUS_IE) == 0 ||
        (g_Reg->STATUS_REGISTER & STATUS_EXL) != 0 ||
        (g_Reg->STATUS_REGISTER & STATUS_ERL) != 0 ||
        (g_Reg->STATUS_REGISTER & 0xFF00) == 0)
    {
        if (g_Plugins->Gfx()->UpdateScreen != NULL)
        {
            g_Plugins->Gfx()->UpdateScreen();
        }
        //CurrentFrame = 0;
        //CurrentPercent = 0;
        //DisplayFPS();
        g_Notify->DisplayError(GS(MSG_PERM_LOOP));
        g_System->CloseCpu();
    }
    else
    {
        if (*g_NextTimer > 0)
        {
            *g_NextTimer = 0 - g_System->CountPerOp();
            g_SystemTimer->UpdateTimers();
        }
    }
}

void CInterpreterCPU::ExecuteCPU()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    bool & Done = g_System->m_EndEmulation;
    uint32_t & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
    OPCODE & Opcode = R4300iOp::m_Opcode;
    uint32_t & JumpToLocation = R4300iOp::m_JumpToLocation;
    bool & TestTimer = R4300iOp::m_TestTimer;
    const int32_t & bDoSomething = g_SystemEvents->DoSomething();
    uint32_t CountPerOp = g_System->CountPerOp();
    int32_t & NextTimer = *g_NextTimer;
    bool CheckTimer = false;

    __except_try()
    {
        while (!Done)
        {
            if (!g_MMU->LW_VAddr(PROGRAM_COUNTER, Opcode.Hex))
            {
                g_Reg->DoTLBReadMiss(R4300iOp::m_NextInstruction == JUMP, PROGRAM_COUNTER);
                R4300iOp::m_NextInstruction = NORMAL;
                continue;
            }

            if (CDebugSettings::bHaveDebugger() && !g_Debugger->CPUStepStarted())
            {
                // Skip command if instructed by the debugger
                PROGRAM_COUNTER += 4;
                continue;
            }

            /* if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER < 0x80380000)
            {
            WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER));
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
            } */

            m_R4300i_Opcode[Opcode.op]();
            NextTimer -= CountPerOp;

            if (CDebugSettings::bHaveDebugger()) { g_Debugger->CPUStep(); }

            PROGRAM_COUNTER += 4;
            switch (R4300iOp::m_NextInstruction)
            {
            case NORMAL:
                break;
            case DELAY_SLOT:
                R4300iOp::m_NextInstruction = JUMP;
                break;
            case PERMLOOP_DO_DELAY:
                R4300iOp::m_NextInstruction = PERMLOOP_DELAY_DONE;
                break;
            case JUMP:
                CheckTimer = (JumpToLocation < PROGRAM_COUNTER - 4 || TestTimer);
                PROGRAM_COUNTER = JumpToLocation;
                R4300iOp::m_NextInstruction = NORMAL;
                if (CheckTimer)
                {
                    TestTimer = false;
                    if (NextTimer < 0)
                    {
                        g_SystemTimer->TimerDone();
                    }
                    if (bDoSomething)
                    {
                        g_SystemEvents->ExecuteEvents();
                    }
                }
                break;
            case PERMLOOP_DELAY_DONE:
                PROGRAM_COUNTER = JumpToLocation;
                R4300iOp::m_NextInstruction = NORMAL;
                CInterpreterCPU::InPermLoop();
                g_SystemTimer->TimerDone();
                if (bDoSomething)
                {
                    g_SystemEvents->ExecuteEvents();
                }
                break;
            default:
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
    WriteTrace(TraceN64System, TraceDebug, "Done");
}

void CInterpreterCPU::ExecuteOps(int32_t Cycles)
{
    bool   & Done = g_System->m_EndEmulation;
    uint32_t  & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
    OPCODE & Opcode = R4300iOp::m_Opcode;
    uint32_t  & JumpToLocation = R4300iOp::m_JumpToLocation;
    bool   & TestTimer = R4300iOp::m_TestTimer;
    const int32_t & DoSomething = g_SystemEvents->DoSomething();
    uint32_t CountPerOp = g_System->CountPerOp();
    bool CheckTimer = false;

    __except_try()
    {
        while (!Done)
        {
            if (Cycles <= 0)
            {
                g_SystemTimer->UpdateTimers();
                return;
            }

            if (g_MMU->LW_VAddr(PROGRAM_COUNTER, Opcode.Hex))
            {
                /*if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER));
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                /*if (PROGRAM_COUNTER > 0x80323000 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER));
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iOpcodeName(Opcode.Hex,*_PROGRAM_COUNTER),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                m_R4300i_Opcode[Opcode.op]();
                _GPR[0].DW = 0; /* MIPS $zero hard-wired to 0 */

                Cycles -= CountPerOp;
                *g_NextTimer -= CountPerOp;

                /*static uint32_t TestAddress = 0x80077B0C, TestValue = 0, CurrentValue = 0;
                if (g_MMU->LW_VAddr(TestAddress, TestValue))
                {
                if (TestValue != CurrentValue)
                {
                WriteTraceF(TraceError,"%X: %X changed (%s)",PROGRAM_COUNTER,TestAddress,R4300iOpcodeName(m_Opcode.Hex,PROGRAM_COUNTER) );
                CurrentValue = TestValue;
                }
                }*/

                switch (R4300iOp::m_NextInstruction)
                {
                case NORMAL:
                    PROGRAM_COUNTER += 4;
                    break;
                case DELAY_SLOT:
                    R4300iOp::m_NextInstruction = JUMP;
                    PROGRAM_COUNTER += 4;
                    break;
                case PERMLOOP_DO_DELAY:
                    R4300iOp::m_NextInstruction = PERMLOOP_DELAY_DONE;
                    PROGRAM_COUNTER += 4;
                    break;
                case JUMP:
                    CheckTimer = (JumpToLocation < PROGRAM_COUNTER || TestTimer);
                    PROGRAM_COUNTER = JumpToLocation;
                    R4300iOp::m_NextInstruction = NORMAL;
                    if (CheckTimer)
                    {
                        TestTimer = false;
                        if (*g_NextTimer < 0)
                        {
                            g_SystemTimer->TimerDone();
                        }
                        if (DoSomething)
                        {
                            g_SystemEvents->ExecuteEvents();
                        }
                    }
                    break;
                case PERMLOOP_DELAY_DONE:
                    PROGRAM_COUNTER = JumpToLocation;
                    R4300iOp::m_NextInstruction = NORMAL;
                    CInterpreterCPU::InPermLoop();
                    g_SystemTimer->TimerDone();
                    if (DoSomething)
                    {
                        g_SystemEvents->ExecuteEvents();
                    }
                    break;
                default:
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            else
            {
                g_Reg->DoTLBReadMiss(R4300iOp::m_NextInstruction == JUMP, PROGRAM_COUNTER);
                R4300iOp::m_NextInstruction = NORMAL;
            }
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
}