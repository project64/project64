#include "stdafx.h"

#include "InterpreterCPU.h"

#include <Project64-core/Debugger.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/R4300iInstruction.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/GFXPlugin.h>
#include <Project64-core/Plugins/Plugin.h>

R4300iOp::Func * CInterpreterCPU::m_R4300i_Opcode = nullptr;

void ExecuteInterpreterOps(uint32_t /*Cycles*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CInterpreterCPU::BuildCPU()
{
    R4300iOp::m_TestTimer = false;
    m_R4300i_Opcode = R4300iOp::BuildInterpreter();
}

void CInterpreterCPU::InPermLoop()
{
    if (EndOnPermLoop() &&
        ((g_Reg->STATUS_REGISTER.InterruptEnable) == 0 ||
         (g_Reg->STATUS_REGISTER.ExceptionLevel) != 0 ||
         (g_Reg->STATUS_REGISTER.ErrorLevel) != 0 ||
         (g_Reg->STATUS_REGISTER.InterruptMask) == 0))
    {
        if (g_Plugins->Gfx()->UpdateScreen != nullptr)
        {
            g_Plugins->Gfx()->UpdateScreen();
        }
        g_Notify->DisplayError(GS(MSG_PERM_LOOP));
        g_System->CloseCpu();
    }
    else if (*g_NextTimer > 0)
    {
        g_SystemTimer->UpdateTimers();
        *g_NextTimer = 0 - g_System->CountPerOp();
        g_SystemTimer->UpdateTimers();
    }
}

void CInterpreterCPU::ExecuteCPU()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    bool & Done = g_System->m_EndEmulation;
    PIPELINE_STAGE & PipelineStage = g_System->m_PipelineStage;
    uint32_t & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
    R4300iOpcode & Opcode = R4300iOp::m_Opcode;
    uint32_t & JumpToLocation = g_System->m_JumpToLocation;
    uint32_t & JumpDelayLocation = g_System->m_JumpDelayLocation;
    bool & TestTimer = R4300iOp::m_TestTimer;
    const int32_t & bDoSomething = g_SystemEvents->DoSomething();
    uint32_t CountPerOp = g_System->CountPerOp();
    int32_t & NextTimer = *g_NextTimer;
    bool CheckTimer = false;

    __except_try()
    {
        while (!Done)
        {
            if (!g_MMU->MemoryValue32(PROGRAM_COUNTER, Opcode.Value))
            {
                g_Reg->DoTLBReadMiss(PipelineStage == PIPELINE_STAGE_JUMP, PROGRAM_COUNTER);
                PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                continue;
            }

            if (HaveDebugger())
            {
                if (HaveExecutionBP() && g_Debugger->ExecutionBP(PROGRAM_COUNTER))
                {
                    g_Settings->SaveBool(Debugger_SteppingOps, true);
                }

                g_Debugger->CPUStepStarted(); // May set stepping ops/skip op

                if (isStepping())
                {
                    g_Debugger->WaitForStep();
                }

                if (SkipOp())
                {
                    // Skip command if instructed by the debugger
                    g_Settings->SaveBool(Debugger_SkipOp, false);
                    PROGRAM_COUNTER += 4;
                    continue;
                }

                g_Debugger->CPUStep();
            }

            /* if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER < 0x80380000)
            {
            WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
            } */

            m_R4300i_Opcode[Opcode.op]();
            _GPR[0].DW = 0; // MIPS $zero hard-wired to 0
            NextTimer -= CountPerOp;

            if (CDebugSettings::HaveDebugger())
            {
                g_Debugger->CPUStepEnded();
            }

            PROGRAM_COUNTER += 4;
            switch (PipelineStage)
            {
            case PIPELINE_STAGE_NORMAL:
                break;
            case PIPELINE_STAGE_DELAY_SLOT:
                PipelineStage = PIPELINE_STAGE_JUMP;
                break;
            case PIPELINE_STAGE_PERMLOOP_DO_DELAY:
                PipelineStage = PIPELINE_STAGE_PERMLOOP_DELAY_DONE;
                break;
            case PIPELINE_STAGE_JUMP:
                CheckTimer = (JumpToLocation < PROGRAM_COUNTER - 4 || TestTimer);
                PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                if ((PROGRAM_COUNTER & 0x3) != 0)
                {
                    GenerateAddressErrorException((int32_t)JumpToLocation, true);
                }
                else if (CheckTimer)
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
            case PIPELINE_STAGE_JUMP_DELAY_SLOT:
                PipelineStage = PIPELINE_STAGE_JUMP;
                PROGRAM_COUNTER = JumpToLocation;
                JumpToLocation = JumpDelayLocation;
                break;
            case PIPELINE_STAGE_PERMLOOP_DELAY_DONE:
                PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
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
    bool & Done = g_System->m_EndEmulation;
    uint32_t & PROGRAM_COUNTER = *_PROGRAM_COUNTER;
    R4300iOpcode & Opcode = R4300iOp::m_Opcode;
    PIPELINE_STAGE & PipelineStage = g_System->m_PipelineStage;
    uint32_t & JumpDelayLocation = g_System->m_JumpDelayLocation;
    uint32_t & JumpToLocation = g_System->m_JumpToLocation;
    bool & TestTimer = R4300iOp::m_TestTimer;
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

            if (g_MMU->MemoryValue32(PROGRAM_COUNTER, Opcode.Value))
            {
                /*if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                /*if (PROGRAM_COUNTER > 0x80323000 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",*_PROGRAM_COUNTER,R4300iInstruction(*_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),_GPR[0x19].UW[0],_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",*_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                m_R4300i_Opcode[Opcode.op]();
                _GPR[0].DW = 0; /* MIPS $zero hard-wired to 0 */

                Cycles -= CountPerOp;
                *g_NextTimer -= CountPerOp;

                /*static uint32_t TestAddress = 0x80077B0C, TestValue = 0, CurrentValue = 0;
                if (g_MMU->MemoryValue32(TestAddress, TestValue))
                {
                if (TestValue != CurrentValue)
                {
                WriteTraceF(TraceError,"%X: %X changed (%s)",PROGRAM_COUNTER,TestAddress,R4300iInstruction(PROGRAM_COUNTER, m_Opcode.Value).NameAndParam().c_str());
                CurrentValue = TestValue;
                }
                }*/

                switch (PipelineStage)
                {
                case PIPELINE_STAGE_NORMAL:
                    PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_DELAY_SLOT:
                    PipelineStage = PIPELINE_STAGE_JUMP;
                    PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_PERMLOOP_DO_DELAY:
                    PipelineStage = PIPELINE_STAGE_PERMLOOP_DELAY_DONE;
                    PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_JUMP:
                    CheckTimer = (JumpToLocation < PROGRAM_COUNTER || TestTimer);
                    PROGRAM_COUNTER = JumpToLocation;
                    PipelineStage = PIPELINE_STAGE_NORMAL;
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
                case PIPELINE_STAGE_JUMP_DELAY_SLOT:
                    PipelineStage = PIPELINE_STAGE_JUMP;
                    PROGRAM_COUNTER = JumpToLocation;
                    JumpToLocation = JumpDelayLocation;
                    break;
                case PIPELINE_STAGE_PERMLOOP_DELAY_DONE:
                    PROGRAM_COUNTER = JumpToLocation;
                    PipelineStage = PIPELINE_STAGE_NORMAL;
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
                g_Reg->DoTLBReadMiss(PipelineStage == PIPELINE_STAGE_JUMP, PROGRAM_COUNTER);
                PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
            }
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
}