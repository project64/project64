#include "stdafx.h"

#include <Project64-core/Debugger.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/Logging.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/R4300iInstruction.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Mips/TLB.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <fenv.h>
#include <float.h>
#include <math.h>

const uint32_t R4300iOp::SWL_MASK[4] = {0x00000000, 0xFF000000, 0xFFFF0000, 0xFFFFFF00};
const uint32_t R4300iOp::SWR_MASK[4] = {0x00FFFFFF, 0x0000FFFF, 0x000000FF, 0x00000000};
const uint32_t R4300iOp::LWL_MASK[4] = {0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF};
const uint32_t R4300iOp::LWR_MASK[4] = {0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0x0000000};

const int32_t R4300iOp::SWL_SHIFT[4] = {0, 8, 16, 24};
const int32_t R4300iOp::SWR_SHIFT[4] = {24, 16, 8, 0};
const int32_t R4300iOp::LWL_SHIFT[4] = {0, 8, 16, 24};
const int32_t R4300iOp::LWR_SHIFT[4] = {24, 16, 8, 0};

R4300iOp::R4300iOp(CN64System & System) :
    m_System(System),
    m_Reg(System.m_Reg),
    m_MMU(System.m_MMU_VM),
    m_PROGRAM_COUNTER(System.m_Reg.m_PROGRAM_COUNTER),
    m_GPR(System.m_Reg.m_GPR),
    m_FPR(System.m_Reg.m_FPR),
    m_CP0(System.m_Reg.m_CP0),
    m_RegHI(System.m_Reg.m_HI),
    m_RegLO(System.m_Reg.m_LO),
    m_FPR_UW(System.m_Reg.m_FPR_UW),
    m_FPR_UDW(System.m_Reg.m_FPR_UDW),
    m_FPR_S(System.m_Reg.m_FPR_S),
    m_FPR_S_L(System.m_Reg.m_FPR_S_L),
    m_FPR_D(System.m_Reg.m_FPR_D),
    m_FPCR(System.m_Reg.m_FPCR),
    m_LLBit(System.m_Reg.m_LLBit)
{
    m_Opcode.Value = 0;
    BuildInterpreter();
}

R4300iOp::~R4300iOp()
{
}

void R4300iOp::InPermLoop()
{
    if (EndOnPermLoop() &&
        ((m_Reg.STATUS_REGISTER.InterruptEnable) == 0 ||
         (m_Reg.STATUS_REGISTER.ExceptionLevel) != 0 ||
         (m_Reg.STATUS_REGISTER.ErrorLevel) != 0 ||
         (m_Reg.STATUS_REGISTER.InterruptMask) == 0))
    {
        if (g_Plugins->Gfx()->UpdateScreen != nullptr)
        {
            g_Plugins->Gfx()->UpdateScreen();
        }
        g_Notify->DisplayError(GS(MSG_PERM_LOOP));
        m_System.CloseCpu();
    }
    else if (*g_NextTimer > 0)
    {
        g_SystemTimer->UpdateTimers();
        *g_NextTimer = 0 - m_System.CountPerOp();
        g_SystemTimer->UpdateTimers();
    }
}

void R4300iOp::ExecuteCPU()
{
    WriteTrace(TraceN64System, TraceDebug, "Start");

    bool & Done = m_System.m_EndEmulation;
    PIPELINE_STAGE & PipelineStage = m_System.m_PipelineStage;
    uint32_t & JumpToLocation = m_System.m_JumpToLocation;
    uint32_t & JumpDelayLocation = m_System.m_JumpDelayLocation;
    bool & TestTimer = m_System.m_TestTimer;
    CSystemEvents & SystemEvents = m_System.m_SystemEvents;
    const bool & DoSomething = SystemEvents.DoSomething();
    uint32_t CountPerOp = m_System.CountPerOp();
    int32_t & NextTimer = *g_NextTimer;
    bool CheckTimer = false;

    __except_try()
    {
        while (!Done)
        {
            if (!m_MMU.MemoryValue32(m_PROGRAM_COUNTER, m_Opcode.Value))
            {
                m_Reg.TriggerAddressException((int32_t)m_PROGRAM_COUNTER, EXC_RMISS);
                m_PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                continue;
            }

            if (HaveDebugger())
            {
                if (HaveExecutionBP() && g_Debugger->ExecutionBP(m_PROGRAM_COUNTER))
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
                    m_PROGRAM_COUNTER += 4;
                    continue;
                }

                g_Debugger->CPUStep();
            }

            /* if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER < 0x80380000)
            {
            WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",m_PROGRAM_COUNTER,R4300iInstruction(m_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",m_PROGRAM_COUNTER,R4300iInstruction(m_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),m_GPR[0x19].UW[0],m_GPR[0x03].UW[0]);
            // WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",m_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
            } */

            (this->*Jump_Opcode[m_Opcode.op])();
            m_GPR[0].DW = 0; // MIPS $zero hard-wired to 0
            NextTimer -= CountPerOp;

            if (CDebugSettings::HaveDebugger())
            {
                g_Debugger->CPUStepEnded();
            }

            m_PROGRAM_COUNTER += 4;
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
                CheckTimer = (JumpToLocation < m_PROGRAM_COUNTER - 4 || TestTimer);
                m_PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                if ((m_PROGRAM_COUNTER & 0x3) != 0)
                {
                    m_Reg.DoAddressError((int32_t)JumpToLocation, true);
                    m_PROGRAM_COUNTER = JumpToLocation;
                    PipelineStage = PIPELINE_STAGE_NORMAL;
                }
                else if (CheckTimer)
                {
                    TestTimer = false;
                    if (NextTimer < 0)
                    {
                        g_SystemTimer->TimerDone();
                    }
                    if (DoSomething)
                    {
                        SystemEvents.ExecuteEvents();
                    }
                }
                break;
            case PIPELINE_STAGE_JUMP_DELAY_SLOT:
                PipelineStage = PIPELINE_STAGE_JUMP;
                m_PROGRAM_COUNTER = JumpToLocation;
                JumpToLocation = JumpDelayLocation;
                break;
            case PIPELINE_STAGE_PERMLOOP_DELAY_DONE:
                m_PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
                InPermLoop();
                g_SystemTimer->TimerDone();
                if (DoSomething)
                {
                    SystemEvents.ExecuteEvents();
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

void R4300iOp::ExecuteOps(int32_t Cycles)
{
    bool & Done = m_System.m_EndEmulation;
    PIPELINE_STAGE & PipelineStage = m_System.m_PipelineStage;
    uint32_t & JumpDelayLocation = m_System.m_JumpDelayLocation;
    uint32_t & JumpToLocation = m_System.m_JumpToLocation;
    bool & TestTimer = m_System.m_TestTimer;
    CSystemEvents & SystemEvents = m_System.m_SystemEvents;
    const bool & DoSomething = SystemEvents.DoSomething();
    uint32_t CountPerOp = m_System.CountPerOp();
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

            if (m_MMU.MemoryValue32(m_PROGRAM_COUNTER, m_Opcode.Value))
            {
                /*if (PROGRAM_COUNTER > 0x80000300 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",m_PROGRAM_COUNTER,R4300iInstruction(m_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",m_PROGRAM_COUNTER,R4300iInstruction(m_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),m_GPR[0x19].UW[0],m_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",m_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                /*if (PROGRAM_COUNTER > 0x80323000 && PROGRAM_COUNTER< 0x80380000)
                {
                WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s",m_PROGRAM_COUNTER,R4300iInstruction(m_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str());
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %s t9: %08X v1: %08X",m_PROGRAM_COUNTER,R4300iInstruction(m_PROGRAM_COUNTER, Opcode.Value).NameAndParam().c_str(),m_GPR[0x19].UW[0],m_GPR[0x03].UW[0]);
                //WriteTraceF((TraceType)(TraceError | TraceNoHeader),"%X: %d %d",m_PROGRAM_COUNTER,*g_NextTimer,g_SystemTimer->CurrentType());
                }*/
                (this->*Jump_Opcode[m_Opcode.op])();
                m_GPR[0].DW = 0; /* MIPS $zero hard-wired to 0 */

                Cycles -= CountPerOp;
                *g_NextTimer -= CountPerOp;

                /*static uint32_t TestAddress = 0x80077B0C, TestValue = 0, CurrentValue = 0;
                if (m_MMU.MemoryValue32(TestAddress, TestValue))
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
                    m_PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_DELAY_SLOT:
                    PipelineStage = PIPELINE_STAGE_JUMP;
                    m_PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_PERMLOOP_DO_DELAY:
                    PipelineStage = PIPELINE_STAGE_PERMLOOP_DELAY_DONE;
                    m_PROGRAM_COUNTER += 4;
                    break;
                case PIPELINE_STAGE_JUMP:
                    CheckTimer = (JumpToLocation < m_PROGRAM_COUNTER || TestTimer);
                    m_PROGRAM_COUNTER = JumpToLocation;
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
                            SystemEvents.ExecuteEvents();
                        }
                    }
                    break;
                case PIPELINE_STAGE_JUMP_DELAY_SLOT:
                    PipelineStage = PIPELINE_STAGE_JUMP;
                    m_PROGRAM_COUNTER = JumpToLocation;
                    JumpToLocation = JumpDelayLocation;
                    break;
                case PIPELINE_STAGE_PERMLOOP_DELAY_DONE:
                    m_PROGRAM_COUNTER = JumpToLocation;
                    PipelineStage = PIPELINE_STAGE_NORMAL;
                    InPermLoop();
                    g_SystemTimer->TimerDone();
                    if (DoSomething)
                    {
                        SystemEvents.ExecuteEvents();
                    }
                    break;
                default:
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            else
            {
                m_Reg.TriggerAddressException((int32_t)m_PROGRAM_COUNTER, EXC_RMISS);
                m_PROGRAM_COUNTER = JumpToLocation;
                PipelineStage = PIPELINE_STAGE_NORMAL;
            }
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
}

void R4300iOp::SPECIAL()
{
    (this->*Jump_Special[m_Opcode.funct])();
}

void R4300iOp::REGIMM()
{
    (this->*Jump_Regimm[m_Opcode.rt])();
}

void R4300iOp::COP0()
{
    (this->*Jump_CoP0[m_Opcode.rs])();
}

void R4300iOp::COP0_CO()
{
    (this->*Jump_CoP0_Function[m_Opcode.funct])();
}

void R4300iOp::COP1()
{
    (this->*Jump_CoP1[m_Opcode.fmt])();
}

void R4300iOp::COP2()
{
    (this->*Jump_CoP2[m_Opcode.fmt])();
}

void R4300iOp::COP3()
{
    m_Reg.TriggerException(EXC_II);
}

void R4300iOp::COP1_BC()
{
    (this->*Jump_CoP1_BC[m_Opcode.ft])();
}

void R4300iOp::COP1_S()
{
    (this->*Jump_CoP1_S[m_Opcode.funct])();
}

void R4300iOp::COP1_D()
{
    (this->*Jump_CoP1_D[m_Opcode.funct])();
}

void R4300iOp::COP1_W()
{
    (this->*Jump_CoP1_W[m_Opcode.funct])();
}

void R4300iOp::COP1_L()
{
    (this->*Jump_CoP1_L[m_Opcode.funct])();
}

void R4300iOp::BuildInterpreter()
{
    Jump_Opcode[0] = &R4300iOp::SPECIAL;
    Jump_Opcode[1] = &R4300iOp::REGIMM;
    Jump_Opcode[2] = &R4300iOp::J;
    Jump_Opcode[3] = &R4300iOp::JAL;
    Jump_Opcode[4] = &R4300iOp::BEQ;
    Jump_Opcode[5] = &R4300iOp::BNE;
    Jump_Opcode[6] = &R4300iOp::BLEZ;
    Jump_Opcode[7] = &R4300iOp::BGTZ;
    Jump_Opcode[8] = &R4300iOp::ADDI;
    Jump_Opcode[9] = &R4300iOp::ADDIU;
    Jump_Opcode[10] = &R4300iOp::SLTI;
    Jump_Opcode[11] = &R4300iOp::SLTIU;
    Jump_Opcode[12] = &R4300iOp::ANDI;
    Jump_Opcode[13] = &R4300iOp::ORI;
    Jump_Opcode[14] = &R4300iOp::XORI;
    Jump_Opcode[15] = &R4300iOp::LUI;
    Jump_Opcode[16] = &R4300iOp::COP0;
    Jump_Opcode[17] = &R4300iOp::COP1;
    Jump_Opcode[18] = &R4300iOp::COP2;
    Jump_Opcode[19] = &R4300iOp::COP3;
    Jump_Opcode[20] = &R4300iOp::BEQL;
    Jump_Opcode[21] = &R4300iOp::BNEL;
    Jump_Opcode[22] = &R4300iOp::BLEZL;
    Jump_Opcode[23] = &R4300iOp::BGTZL;
    Jump_Opcode[24] = &R4300iOp::DADDI;
    Jump_Opcode[25] = &R4300iOp::DADDIU;
    Jump_Opcode[26] = &R4300iOp::LDL;
    Jump_Opcode[27] = &R4300iOp::LDR;
    Jump_Opcode[28] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[29] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[30] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[31] = &R4300iOp::ReservedInstruction;
    Jump_Opcode[32] = &R4300iOp::LB;
    Jump_Opcode[33] = &R4300iOp::LH;
    Jump_Opcode[34] = &R4300iOp::LWL;
    Jump_Opcode[35] = &R4300iOp::LW;
    Jump_Opcode[36] = &R4300iOp::LBU;
    Jump_Opcode[37] = &R4300iOp::LHU;
    Jump_Opcode[38] = &R4300iOp::LWR;
    Jump_Opcode[39] = &R4300iOp::LWU;
    Jump_Opcode[40] = &R4300iOp::SB;
    Jump_Opcode[41] = &R4300iOp::SH;
    Jump_Opcode[42] = &R4300iOp::SWL;
    Jump_Opcode[43] = &R4300iOp::SW;
    Jump_Opcode[44] = &R4300iOp::SDL;
    Jump_Opcode[45] = &R4300iOp::SDR;
    Jump_Opcode[46] = &R4300iOp::SWR;
    Jump_Opcode[47] = &R4300iOp::CACHE;
    Jump_Opcode[48] = &R4300iOp::LL;
    Jump_Opcode[49] = &R4300iOp::LWC1;
    Jump_Opcode[50] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[51] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[52] = &R4300iOp::LLD;
    Jump_Opcode[53] = &R4300iOp::LDC1;
    Jump_Opcode[54] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[55] = &R4300iOp::LD;
    Jump_Opcode[56] = &R4300iOp::SC;
    Jump_Opcode[57] = &R4300iOp::SWC1;
    Jump_Opcode[58] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[59] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[60] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[61] = &R4300iOp::SDC1;
    Jump_Opcode[62] = &R4300iOp::UnknownOpcode;
    Jump_Opcode[63] = &R4300iOp::SD;

    Jump_Special[0] = &R4300iOp::SPECIAL_SLL;
    Jump_Special[1] = &R4300iOp::UnknownOpcode;
    Jump_Special[2] = &R4300iOp::SPECIAL_SRL;
    Jump_Special[3] = &R4300iOp::SPECIAL_SRA;
    Jump_Special[4] = &R4300iOp::SPECIAL_SLLV;
    Jump_Special[5] = &R4300iOp::UnknownOpcode;
    Jump_Special[6] = &R4300iOp::SPECIAL_SRLV;
    Jump_Special[7] = &R4300iOp::SPECIAL_SRAV;
    Jump_Special[8] = &R4300iOp::SPECIAL_JR;
    Jump_Special[9] = &R4300iOp::SPECIAL_JALR;
    Jump_Special[10] = &R4300iOp::UnknownOpcode;
    Jump_Special[11] = &R4300iOp::UnknownOpcode;
    Jump_Special[12] = &R4300iOp::SPECIAL_SYSCALL;
    Jump_Special[13] = &R4300iOp::SPECIAL_BREAK;
    Jump_Special[14] = &R4300iOp::UnknownOpcode;
    Jump_Special[15] = &R4300iOp::SPECIAL_SYNC;
    Jump_Special[16] = &R4300iOp::SPECIAL_MFHI;
    Jump_Special[17] = &R4300iOp::SPECIAL_MTHI;
    Jump_Special[18] = &R4300iOp::SPECIAL_MFLO;
    Jump_Special[19] = &R4300iOp::SPECIAL_MTLO;
    Jump_Special[20] = &R4300iOp::SPECIAL_DSLLV;
    Jump_Special[21] = &R4300iOp::UnknownOpcode;
    Jump_Special[22] = &R4300iOp::SPECIAL_DSRLV;
    Jump_Special[23] = &R4300iOp::SPECIAL_DSRAV;
    Jump_Special[24] = &R4300iOp::SPECIAL_MULT;
    Jump_Special[25] = &R4300iOp::SPECIAL_MULTU;
    Jump_Special[26] = &R4300iOp::SPECIAL_DIV;
    Jump_Special[27] = &R4300iOp::SPECIAL_DIVU;
    Jump_Special[28] = &R4300iOp::SPECIAL_DMULT;
    Jump_Special[29] = &R4300iOp::SPECIAL_DMULTU;
    Jump_Special[30] = &R4300iOp::SPECIAL_DDIV;
    Jump_Special[31] = &R4300iOp::SPECIAL_DDIVU;
    Jump_Special[32] = &R4300iOp::SPECIAL_ADD;
    Jump_Special[33] = &R4300iOp::SPECIAL_ADDU;
    Jump_Special[34] = &R4300iOp::SPECIAL_SUB;
    Jump_Special[35] = &R4300iOp::SPECIAL_SUBU;
    Jump_Special[36] = &R4300iOp::SPECIAL_AND;
    Jump_Special[37] = &R4300iOp::SPECIAL_OR;
    Jump_Special[38] = &R4300iOp::SPECIAL_XOR;
    Jump_Special[39] = &R4300iOp::SPECIAL_NOR;
    Jump_Special[40] = &R4300iOp::UnknownOpcode;
    Jump_Special[41] = &R4300iOp::UnknownOpcode;
    Jump_Special[42] = &R4300iOp::SPECIAL_SLT;
    Jump_Special[43] = &R4300iOp::SPECIAL_SLTU;
    Jump_Special[44] = &R4300iOp::SPECIAL_DADD;
    Jump_Special[45] = &R4300iOp::SPECIAL_DADDU;
    Jump_Special[46] = &R4300iOp::SPECIAL_DSUB;
    Jump_Special[47] = &R4300iOp::SPECIAL_DSUBU;
    Jump_Special[48] = &R4300iOp::SPECIAL_TGE;
    Jump_Special[49] = &R4300iOp::SPECIAL_TGEU;
    Jump_Special[50] = &R4300iOp::SPECIAL_TLT;
    Jump_Special[51] = &R4300iOp::SPECIAL_TLTU;
    Jump_Special[52] = &R4300iOp::SPECIAL_TEQ;
    Jump_Special[53] = &R4300iOp::UnknownOpcode;
    Jump_Special[54] = &R4300iOp::SPECIAL_TNE;
    Jump_Special[55] = &R4300iOp::UnknownOpcode;
    Jump_Special[56] = &R4300iOp::SPECIAL_DSLL;
    Jump_Special[57] = &R4300iOp::UnknownOpcode;
    Jump_Special[58] = &R4300iOp::SPECIAL_DSRL;
    Jump_Special[59] = &R4300iOp::SPECIAL_DSRA;
    Jump_Special[60] = &R4300iOp::SPECIAL_DSLL32;
    Jump_Special[61] = &R4300iOp::UnknownOpcode;
    Jump_Special[62] = &R4300iOp::SPECIAL_DSRL32;
    Jump_Special[63] = &R4300iOp::SPECIAL_DSRA32;

    Jump_Regimm[0] = &R4300iOp::REGIMM_BLTZ;
    Jump_Regimm[1] = &R4300iOp::REGIMM_BGEZ;
    Jump_Regimm[2] = &R4300iOp::REGIMM_BLTZL;
    Jump_Regimm[3] = &R4300iOp::REGIMM_BGEZL;
    Jump_Regimm[4] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[5] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[6] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[7] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[8] = &R4300iOp::REGIMM_TGEI;
    Jump_Regimm[9] = &R4300iOp::REGIMM_TGEIU;
    Jump_Regimm[10] = &R4300iOp::REGIMM_TLTI;
    Jump_Regimm[11] = &R4300iOp::REGIMM_TLTIU;
    Jump_Regimm[12] = &R4300iOp::REGIMM_TEQI;
    Jump_Regimm[13] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[14] = &R4300iOp::REGIMM_TNEI;
    Jump_Regimm[15] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[16] = &R4300iOp::REGIMM_BLTZAL;
    Jump_Regimm[17] = &R4300iOp::REGIMM_BGEZAL;
    Jump_Regimm[18] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[19] = &R4300iOp::REGIMM_BGEZALL;
    Jump_Regimm[20] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[21] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[22] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[23] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[24] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[25] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[26] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[27] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[28] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[29] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[30] = &R4300iOp::UnknownOpcode;
    Jump_Regimm[31] = &R4300iOp::UnknownOpcode;

    Jump_CoP0[0] = &R4300iOp::COP0_MF;
    Jump_CoP0[1] = &R4300iOp::COP0_DMF;
    Jump_CoP0[2] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[3] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[4] = &R4300iOp::COP0_MT;
    Jump_CoP0[5] = &R4300iOp::COP0_DMT;
    Jump_CoP0[6] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[7] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[8] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[9] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[10] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[11] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[12] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[13] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[14] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[15] = &R4300iOp::UnknownOpcode;
    Jump_CoP0[16] = &R4300iOp::COP0_CO;
    Jump_CoP0[17] = &R4300iOp::COP0_CO;
    Jump_CoP0[18] = &R4300iOp::COP0_CO;
    Jump_CoP0[19] = &R4300iOp::COP0_CO;
    Jump_CoP0[20] = &R4300iOp::COP0_CO;
    Jump_CoP0[21] = &R4300iOp::COP0_CO;
    Jump_CoP0[22] = &R4300iOp::COP0_CO;
    Jump_CoP0[23] = &R4300iOp::COP0_CO;
    Jump_CoP0[24] = &R4300iOp::COP0_CO;
    Jump_CoP0[25] = &R4300iOp::COP0_CO;
    Jump_CoP0[26] = &R4300iOp::COP0_CO;
    Jump_CoP0[27] = &R4300iOp::COP0_CO;
    Jump_CoP0[28] = &R4300iOp::COP0_CO;
    Jump_CoP0[29] = &R4300iOp::COP0_CO;
    Jump_CoP0[30] = &R4300iOp::COP0_CO;
    Jump_CoP0[31] = &R4300iOp::COP0_CO;

    Jump_CoP0_Function[0] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[1] = &R4300iOp::COP0_CO_TLBR;
    Jump_CoP0_Function[2] = &R4300iOp::COP0_CO_TLBWI;
    Jump_CoP0_Function[3] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[4] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[5] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[6] = &R4300iOp::COP0_CO_TLBWR;
    Jump_CoP0_Function[7] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[8] = &R4300iOp::COP0_CO_TLBP;
    Jump_CoP0_Function[9] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[10] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[11] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[12] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[13] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[14] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[15] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[16] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[17] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[20] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[21] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[24] = &R4300iOp::COP0_CO_ERET;
    Jump_CoP0_Function[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[31] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[32] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[33] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[34] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[35] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[36] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[37] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[38] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[39] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[40] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[41] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[42] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[43] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[44] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[45] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[46] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[47] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[48] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[49] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[50] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[51] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[52] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[53] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[54] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[55] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[56] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[57] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[58] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[59] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[60] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[61] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[62] = &R4300iOp::UnknownOpcode;
    Jump_CoP0_Function[63] = &R4300iOp::UnknownOpcode;

    Jump_CoP1[0] = &R4300iOp::COP1_MF;
    Jump_CoP1[1] = &R4300iOp::COP1_DMF;
    Jump_CoP1[2] = &R4300iOp::COP1_CF;
    Jump_CoP1[3] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1[4] = &R4300iOp::COP1_MT;
    Jump_CoP1[5] = &R4300iOp::COP1_DMT;
    Jump_CoP1[6] = &R4300iOp::COP1_CT;
    Jump_CoP1[7] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1[8] = &R4300iOp::COP1_BC;
    Jump_CoP1[9] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[10] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[11] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[12] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[13] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[14] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[15] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[16] = &R4300iOp::COP1_S;
    Jump_CoP1[17] = &R4300iOp::COP1_D;
    Jump_CoP1[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[20] = &R4300iOp::COP1_W;
    Jump_CoP1[21] = &R4300iOp::COP1_L;
    Jump_CoP1[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[24] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP1[31] = &R4300iOp::UnknownOpcode;

    Jump_CoP1_BC[0] = &R4300iOp::COP1_BCF;
    Jump_CoP1_BC[1] = &R4300iOp::COP1_BCT;
    Jump_CoP1_BC[2] = &R4300iOp::COP1_BCFL;
    Jump_CoP1_BC[3] = &R4300iOp::COP1_BCTL;
    Jump_CoP1_BC[4] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[5] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[6] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[7] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[8] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[9] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[10] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[11] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[12] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[13] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[14] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[15] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[16] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[17] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[20] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[21] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[24] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_BC[31] = &R4300iOp::UnknownOpcode;

    Jump_CoP1_S[0] = &R4300iOp::COP1_S_ADD;
    Jump_CoP1_S[1] = &R4300iOp::COP1_S_SUB;
    Jump_CoP1_S[2] = &R4300iOp::COP1_S_MUL;
    Jump_CoP1_S[3] = &R4300iOp::COP1_S_DIV;
    Jump_CoP1_S[4] = &R4300iOp::COP1_S_SQRT;
    Jump_CoP1_S[5] = &R4300iOp::COP1_S_ABS;
    Jump_CoP1_S[6] = &R4300iOp::COP1_S_MOV;
    Jump_CoP1_S[7] = &R4300iOp::COP1_S_NEG;
    Jump_CoP1_S[8] = &R4300iOp::COP1_S_ROUND_L;
    Jump_CoP1_S[9] = &R4300iOp::COP1_S_TRUNC_L;
    Jump_CoP1_S[10] = &R4300iOp::COP1_S_CEIL_L;
    Jump_CoP1_S[11] = &R4300iOp::COP1_S_FLOOR_L;
    Jump_CoP1_S[12] = &R4300iOp::COP1_S_ROUND_W;
    Jump_CoP1_S[13] = &R4300iOp::COP1_S_TRUNC_W;
    Jump_CoP1_S[14] = &R4300iOp::COP1_S_CEIL_W;
    Jump_CoP1_S[15] = &R4300iOp::COP1_S_FLOOR_W;
    Jump_CoP1_S[16] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[17] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[20] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[21] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[24] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[31] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[32] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_S[33] = &R4300iOp::COP1_S_CVT_D;
    Jump_CoP1_S[34] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[35] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[36] = &R4300iOp::COP1_S_CVT_W;
    Jump_CoP1_S[37] = &R4300iOp::COP1_S_CVT_L;
    Jump_CoP1_S[38] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[39] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[40] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[41] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[42] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[43] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[44] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[45] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[46] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[47] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_S[48] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[49] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[50] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[51] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[52] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[53] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[54] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[55] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[56] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[57] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[58] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[59] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[60] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[61] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[62] = &R4300iOp::COP1_S_CMP;
    Jump_CoP1_S[63] = &R4300iOp::COP1_S_CMP;

    Jump_CoP1_D[0] = &R4300iOp::COP1_D_ADD;
    Jump_CoP1_D[1] = &R4300iOp::COP1_D_SUB;
    Jump_CoP1_D[2] = &R4300iOp::COP1_D_MUL;
    Jump_CoP1_D[3] = &R4300iOp::COP1_D_DIV;
    Jump_CoP1_D[4] = &R4300iOp::COP1_D_SQRT;
    Jump_CoP1_D[5] = &R4300iOp::COP1_D_ABS;
    Jump_CoP1_D[6] = &R4300iOp::COP1_D_MOV;
    Jump_CoP1_D[7] = &R4300iOp::COP1_D_NEG;
    Jump_CoP1_D[8] = &R4300iOp::COP1_D_ROUND_L;
    Jump_CoP1_D[9] = &R4300iOp::COP1_D_TRUNC_L;
    Jump_CoP1_D[10] = &R4300iOp::COP1_D_CEIL_L;
    Jump_CoP1_D[11] = &R4300iOp::COP1_D_FLOOR_L;
    Jump_CoP1_D[12] = &R4300iOp::COP1_D_ROUND_W;
    Jump_CoP1_D[13] = &R4300iOp::COP1_D_TRUNC_W;
    Jump_CoP1_D[14] = &R4300iOp::COP1_D_CEIL_W;
    Jump_CoP1_D[15] = &R4300iOp::COP1_D_FLOOR_W;
    Jump_CoP1_D[16] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[17] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[20] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[21] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[24] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[31] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[32] = &R4300iOp::COP1_D_CVT_S;
    Jump_CoP1_D[33] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_D[34] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[35] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[36] = &R4300iOp::COP1_D_CVT_W;
    Jump_CoP1_D[37] = &R4300iOp::COP1_D_CVT_L;
    Jump_CoP1_D[38] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[39] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[40] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[41] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[42] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[43] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[44] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[45] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[46] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[47] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_D[48] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[49] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[50] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[51] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[52] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[53] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[54] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[55] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[56] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[57] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[58] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[59] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[60] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[61] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[62] = &R4300iOp::COP1_D_CMP;
    Jump_CoP1_D[63] = &R4300iOp::COP1_D_CMP;

    Jump_CoP1_W[0] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[1] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[2] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[3] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[4] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[5] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[6] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[7] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[8] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[9] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[10] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[11] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[12] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[13] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[14] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[15] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[16] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[17] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[20] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[21] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[24] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[31] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[32] = &R4300iOp::COP1_W_CVT_S;
    Jump_CoP1_W[33] = &R4300iOp::COP1_W_CVT_D;
    Jump_CoP1_W[34] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[35] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[36] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[37] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_W[38] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[39] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[40] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[41] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[42] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[43] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[44] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[45] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[46] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[47] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[48] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[49] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[50] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[51] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[52] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[53] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[54] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[55] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[56] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[57] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[58] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[59] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[60] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[61] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[62] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_W[63] = &R4300iOp::UnknownOpcode;

    Jump_CoP1_L[0] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[1] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[2] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[3] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[4] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[5] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[6] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[7] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[8] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[9] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[10] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[11] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[12] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[13] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[14] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[15] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[16] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[17] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[18] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[19] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[20] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[21] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[22] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[23] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[24] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[25] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[26] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[27] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[28] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[29] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[30] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[31] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[32] = &R4300iOp::COP1_L_CVT_S;
    Jump_CoP1_L[33] = &R4300iOp::COP1_L_CVT_D;
    Jump_CoP1_L[34] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[35] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[36] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[37] = &R4300iOp::CPO1_UNIMPLEMENTED_OP;
    Jump_CoP1_L[38] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[39] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[40] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[41] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[42] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[43] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[44] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[45] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[46] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[47] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[48] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[49] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[50] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[51] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[52] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[53] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[54] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[55] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[56] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[57] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[58] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[59] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[60] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[61] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[62] = &R4300iOp::UnknownOpcode;
    Jump_CoP1_L[63] = &R4300iOp::UnknownOpcode;

    Jump_CoP2[0] = &R4300iOp::COP2_MF;
    Jump_CoP2[1] = &R4300iOp::COP2_DMF;
    Jump_CoP2[2] = &R4300iOp::COP2_CF;
    Jump_CoP2[3] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[4] = &R4300iOp::COP2_MT;
    Jump_CoP2[5] = &R4300iOp::COP2_DMT;
    Jump_CoP2[6] = &R4300iOp::COP2_CT;
    Jump_CoP2[7] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[8] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[10] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[11] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[12] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[13] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[14] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[15] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[16] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[17] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[18] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[19] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[20] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[21] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[22] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[23] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[24] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[25] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[26] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[27] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[28] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[29] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[30] = &R4300iOp::CPO2_INVALID_OP;
    Jump_CoP2[31] = &R4300iOp::CPO2_INVALID_OP;
}

// Opcode functions

void R4300iOp::J()
{
    m_System.DelayedJump((m_PROGRAM_COUNTER & 0xF0000000) + (m_Opcode.target << 2));
}

void R4300iOp::JAL()
{
    m_System.DelayedJump((m_PROGRAM_COUNTER & 0xF0000000) + (m_Opcode.target << 2));
    m_GPR[31].DW = (int32_t)(m_System.m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? m_System.m_JumpToLocation + 4 : m_PROGRAM_COUNTER + 8);
}

void R4300iOp::BEQ()
{
    if (m_GPR[m_Opcode.rs].DW == m_GPR[m_Opcode.rt].DW)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::BNE()
{
    if (m_GPR[m_Opcode.rs].DW != m_GPR[m_Opcode.rt].DW)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::BLEZ()
{
    if (m_GPR[m_Opcode.rs].DW <= 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::BGTZ()
{
    if (m_GPR[m_Opcode.rs].DW > 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::ADDI()
{
#ifdef Interpreter_StackTest
    if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        StackValue += (int16_t)m_Opcode.immediate;
    }
#endif
    int32_t rs = m_GPR[m_Opcode.rs].W[0];
    int32_t imm = (int16_t)m_Opcode.immediate;
    int32_t sum = rs + imm;
    if ((~(rs ^ imm) & (rs ^ sum)) & 0x80000000)
    {
        m_Reg.TriggerException(EXC_OV);
        return;
    }
    m_GPR[m_Opcode.rt].DW = sum;
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        StackValue = m_GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::ADDIU()
{
#ifdef Interpreter_StackTest
    if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        StackValue += (int16_t)m_Opcode.immediate;
    }
#endif
    m_GPR[m_Opcode.rt].DW = (m_GPR[m_Opcode.rs].W[0] + ((int16_t)m_Opcode.immediate));
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        StackValue = m_GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::SLTI()
{
    if (m_GPR[m_Opcode.rs].DW < (int64_t)((int16_t)m_Opcode.immediate))
    {
        m_GPR[m_Opcode.rt].DW = 1;
    }
    else
    {
        m_GPR[m_Opcode.rt].DW = 0;
    }
}

void R4300iOp::SLTIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rs].UDW < (uint64_t)imm64 ? 1 : 0;
}

void R4300iOp::ANDI()
{
    m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rs].DW & m_Opcode.immediate;
}

void R4300iOp::ORI()
{
    m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rs].DW | m_Opcode.immediate;
}

void R4300iOp::XORI()
{
    m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rs].DW ^ m_Opcode.immediate;
}

void R4300iOp::LUI()
{
    m_GPR[m_Opcode.rt].DW = (int32_t)((int16_t)m_Opcode.offset << 16);
#ifdef Interpreter_StackTest
    if (m_Opcode.rt == 29)
    {
        StackValue = m_GPR[m_Opcode.rt].W[0];
    }
#endif
}

void R4300iOp::BEQL()
{
    if (m_GPR[m_Opcode.rs].DW == m_GPR[m_Opcode.rt].DW)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BNEL()
{
    if (m_GPR[m_Opcode.rs].DW != m_GPR[m_Opcode.rt].DW)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BLEZL()
{
    if (m_GPR[m_Opcode.rs].DW <= 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::BGTZL()
{
    if (m_GPR[m_Opcode.rs].DW > 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::DADDI()
{
    int64_t rs = m_GPR[m_Opcode.rs].DW;
    int64_t imm = (int64_t)((int16_t)m_Opcode.immediate);
    int64_t sum = rs + imm;
    if ((~(rs ^ imm) & (rs ^ sum)) & 0x8000000000000000)
    {
        m_Reg.TriggerException(EXC_OV);
        return;
    }
    m_GPR[m_Opcode.rt].DW = sum;
}

void R4300iOp::DADDIU()
{
    m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rs].DW + (int64_t)((int16_t)m_Opcode.immediate);
}

uint64_t LDL_MASK[8] = {0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFF, 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF};
int32_t LDL_SHIFT[8] = {0, 8, 16, 24, 32, 40, 48, 56};

void R4300iOp::LDL()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (m_MMU.LD_Memory((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rt].DW & LDL_MASK[Offset];
        m_GPR[m_Opcode.rt].DW += MemoryValue << LDL_SHIFT[Offset];
    }
}

uint64_t LDR_MASK[8] = {0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000,
                        0xFFFFFFFFFF000000, 0xFFFFFFFF00000000,
                        0xFFFFFF0000000000, 0xFFFF000000000000,
                        0xFF00000000000000, 0};
int32_t LDR_SHIFT[8] = {56, 48, 40, 32, 24, 16, 8, 0};

void R4300iOp::LDR()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (m_MMU.LD_Memory((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        m_GPR[m_Opcode.rt].DW = m_GPR[m_Opcode.rt].DW & LDR_MASK[Offset];
        m_GPR[m_Opcode.rt].DW += MemoryValue >> LDR_SHIFT[Offset];
    }
}

void R4300iOp::LB()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint8_t MemoryValue;
    if (m_MMU.LB_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].DW = (int8_t)MemoryValue;
    }
}

void R4300iOp::LH()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint16_t MemoryValue;
    if (m_MMU.LH_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].DW = (int16_t)MemoryValue;
    }
}

void R4300iOp::LWL()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.LW_Memory((Address & ~3), MemoryValue))
    {
        uint32_t Offset = Address & 3;
        m_GPR[m_Opcode.rt].DW = (int32_t)(m_GPR[m_Opcode.rt].W[0] & LWL_MASK[Offset]);
        m_GPR[m_Opcode.rt].DW += (int32_t)(MemoryValue << LWL_SHIFT[Offset]);
    }
}

void R4300iOp::LW()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.LW_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].DW = (int32_t)MemoryValue;
    }
}

void R4300iOp::LBU()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint8_t MemoryValue;
    if (m_MMU.LB_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].UDW = MemoryValue;
    }
}

void R4300iOp::LHU()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint16_t MemoryValue;
    if (m_MMU.LH_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].UDW = MemoryValue;
    }
}

void R4300iOp::LWR()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.LW_Memory((Address & ~3), MemoryValue))
    {
        uint32_t Offset = Address & 3;
        m_GPR[m_Opcode.rt].DW = (int32_t)(m_GPR[m_Opcode.rt].W[0] & LWR_MASK[Offset]);
        m_GPR[m_Opcode.rt].DW += (int32_t)(MemoryValue >> LWR_SHIFT[Offset]);
    }
}

void R4300iOp::LWU()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.LW_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].UDW = MemoryValue;
    }
}

void R4300iOp::SB()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.SB_Memory(Address, m_GPR[m_Opcode.rt].UW[0]);
}

void R4300iOp::SH()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.SH_Memory(Address, m_GPR[m_Opcode.rt].UW[0]);
}

void R4300iOp::SWL()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.MemoryValue32(Address & ~3, MemoryValue))
    {
        uint32_t Offset = Address & 3;
        MemoryValue &= SWL_MASK[Offset];
        MemoryValue += m_GPR[m_Opcode.rt].UW[0] >> SWL_SHIFT[Offset];
        m_MMU.SW_Memory(Address & ~3, MemoryValue);
    }
    else
    {
        m_Reg.TriggerAddressException(Address, EXC_WMISS);
    }
}

void R4300iOp::SW()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.SW_Memory(Address, m_GPR[m_Opcode.rt].UW[0]);
}

uint64_t SDL_MASK[8] = {0, 0xFF00000000000000,
                        0xFFFF000000000000,
                        0xFFFFFF0000000000,
                        0xFFFFFFFF00000000,
                        0xFFFFFFFFFF000000,
                        0xFFFFFFFFFFFF0000,
                        0xFFFFFFFFFFFFFF00};

int32_t SDL_SHIFT[8] = {0, 8, 16, 24, 32, 40, 48, 56};

void R4300iOp::SDL()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (m_MMU.MemoryValue64((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        MemoryValue &= SDL_MASK[Offset];
        MemoryValue += m_GPR[m_Opcode.rt].UDW >> SDL_SHIFT[Offset];
        m_MMU.SD_Memory((Address & ~7), MemoryValue);
    }
    else
    {
        m_Reg.TriggerAddressException(Address, EXC_WMISS);
    }
}

uint64_t SDR_MASK[8] = {0x00FFFFFFFFFFFFFF,
                        0x0000FFFFFFFFFFFF,
                        0x000000FFFFFFFFFF,
                        0x00000000FFFFFFFF,
                        0x0000000000FFFFFF,
                        0x000000000000FFFF,
                        0x00000000000000FF,
                        0x0000000000000000};

int32_t SDR_SHIFT[8] = {56, 48, 40, 32, 24, 16, 8, 0};

void R4300iOp::SDR()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint64_t MemoryValue;
    if (m_MMU.MemoryValue64((Address & ~7), MemoryValue))
    {
        uint32_t Offset = Address & 7;
        MemoryValue &= SDR_MASK[Offset];
        MemoryValue += m_GPR[m_Opcode.rt].UDW << SDR_SHIFT[Offset];
        m_MMU.SD_Memory((Address & ~7), MemoryValue);
    }
    else
    {
        m_Reg.TriggerAddressException(Address, EXC_WMISS);
    }
}

void R4300iOp::SWR()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.MemoryValue32((Address & ~3), MemoryValue))
    {
        uint32_t Offset = Address & 3;
        MemoryValue &= SWR_MASK[Offset];
        MemoryValue += m_GPR[m_Opcode.rt].UW[0] << SWR_SHIFT[Offset];
        m_MMU.SW_Memory((Address & ~0x03), MemoryValue);
    }
    else
    {
        m_Reg.TriggerAddressException(Address, EXC_WMISS);
    }
}

void R4300iOp::CACHE()
{
    if (!LogCache())
    {
        return;
    }
    LogMessage("%08X: Cache operation %d, 0x%08X", (m_PROGRAM_COUNTER), m_Opcode.rt, m_GPR[m_Opcode.base].UW[0] + (int16_t)m_Opcode.offset);
}

void R4300iOp::LL()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    uint32_t MemoryValue;

    if (m_MMU.LW_Memory(Address, MemoryValue))
    {
        m_GPR[m_Opcode.rt].DW = (int32_t)MemoryValue;
        m_LLBit = 1;
        uint32_t PhysicalAddr;
        bool MemoryUsed;
        g_TLB->VAddrToPAddr(Address, PhysicalAddr, MemoryUsed);
        m_CP0[17] = PhysicalAddr >> 4;
    }
}

void R4300iOp::LWC1()
{
    if (TestCop1UsableException())
    {
        return;
    }
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.LW_Memory(Address, *(uint32_t *)m_FPR_S[m_Opcode.ft]);
}

void R4300iOp::SC()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    if (m_LLBit != 1 || m_MMU.SW_Memory(Address, m_GPR[m_Opcode.rt].UW[0]))
    {
        m_GPR[m_Opcode.rt].UW[0] = m_LLBit;
    }
}

void R4300iOp::LD()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    if (m_MMU.LD_Memory(Address, m_GPR[m_Opcode.rt].UDW))
    {
#ifdef Interpreter_StackTest
        if (m_Opcode.rt == 29)
        {
            StackValue = m_GPR[m_Opcode.rt].W[0];
        }
#endif
    }
}

void R4300iOp::LLD()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    if (m_MMU.LD_Memory(Address, m_GPR[m_Opcode.rt].UDW))
    {
        m_LLBit = 1;
        uint32_t PhysicalAddr;
        bool MemoryUsed;
        g_TLB->VAddrToPAddr(Address, PhysicalAddr, MemoryUsed);
        m_CP0[17] = PhysicalAddr >> 4;
    }
}

void R4300iOp::LDC1()
{
    if (TestCop1UsableException())
    {
        return;
    }
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.LD_Memory(Address, *(uint64_t *)m_FPR_D[m_Opcode.ft]);
}

void R4300iOp::SWC1()
{
    if (TestCop1UsableException())
    {
        return;
    }

    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.SW_Memory(Address, *(uint32_t *)m_FPR_S[m_Opcode.ft]);
}

void R4300iOp::SDC1()
{
    if (TestCop1UsableException())
    {
        return;
    }
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.SD_Memory(Address, *((uint64_t *)m_FPR_D[m_Opcode.ft]));
}

void R4300iOp::SD()
{
    uint64_t Address = m_GPR[m_Opcode.base].DW + (int16_t)m_Opcode.offset;
    m_MMU.SD_Memory(Address, m_GPR[m_Opcode.rt].UDW);
}

// R4300i opcodes: Special

void R4300iOp::SPECIAL_SLL()
{
    m_GPR[m_Opcode.rd].DW = (m_GPR[m_Opcode.rt].W[0] << m_Opcode.sa);
}

void R4300iOp::SPECIAL_SRL()
{
    m_GPR[m_Opcode.rd].DW = (int32_t)(m_GPR[m_Opcode.rt].UW[0] >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_SRA()
{
    m_GPR[m_Opcode.rd].DW = (int32_t)(m_GPR[m_Opcode.rt].DW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_SLLV()
{
    m_GPR[m_Opcode.rd].DW = (m_GPR[m_Opcode.rt].W[0] << (m_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_SRLV()
{
    m_GPR[m_Opcode.rd].DW = (int32_t)(m_GPR[m_Opcode.rt].UW[0] >> (m_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_SRAV()
{
    m_GPR[m_Opcode.rd].DW = (int32_t)(m_GPR[m_Opcode.rt].DW >> (m_GPR[m_Opcode.rs].UW[0] & 0x1F));
}

void R4300iOp::SPECIAL_JR()
{
    m_System.DelayedJump(m_GPR[m_Opcode.rs].UW[0]);
    m_System.m_TestTimer = true;
}

void R4300iOp::SPECIAL_JALR()
{
    m_System.DelayedJump(m_GPR[m_Opcode.rs].UW[0]);
    m_GPR[m_Opcode.rd].DW = (int32_t)(m_System.m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? m_System.m_JumpToLocation + 4 : m_PROGRAM_COUNTER + 8);
    m_System.m_TestTimer = true;
}

void R4300iOp::SPECIAL_SYSCALL()
{
    m_Reg.TriggerException(EXC_SYSCALL);
}

void R4300iOp::SPECIAL_BREAK()
{
    if (StepOnBreakOpCode())
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
        g_Debugger->WaitForStep();
    }
    else
    {
        m_Reg.TriggerException(EXC_BREAK);
    }
}

void R4300iOp::SPECIAL_SYNC()
{
}

void R4300iOp::SPECIAL_MFHI()
{
    m_GPR[m_Opcode.rd].DW = m_RegHI.DW;
}

void R4300iOp::SPECIAL_MTHI()
{
    m_RegHI.DW = m_GPR[m_Opcode.rs].DW;
}

void R4300iOp::SPECIAL_MFLO()
{
    m_GPR[m_Opcode.rd].DW = m_RegLO.DW;
}

void R4300iOp::SPECIAL_MTLO()
{
    m_RegLO.DW = m_GPR[m_Opcode.rs].DW;
}

void R4300iOp::SPECIAL_DSLLV()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rt].DW << (m_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_DSRLV()
{
    m_GPR[m_Opcode.rd].UDW = m_GPR[m_Opcode.rt].UDW >> (m_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_DSRAV()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rt].DW >> (m_GPR[m_Opcode.rs].UW[0] & 0x3F);
}

void R4300iOp::SPECIAL_MULT()
{
    m_RegHI.DW = (int64_t)(m_GPR[m_Opcode.rs].W[0]) * (int64_t)(m_GPR[m_Opcode.rt].W[0]);
    m_RegLO.DW = m_RegHI.W[0];
    m_RegHI.DW = m_RegHI.W[1];
}

void R4300iOp::SPECIAL_MULTU()
{
    m_RegHI.DW = (uint64_t)(m_GPR[m_Opcode.rs].UW[0]) * (uint64_t)(m_GPR[m_Opcode.rt].UW[0]);
    m_RegLO.DW = m_RegHI.W[0];
    m_RegHI.DW = m_RegHI.W[1];
}

void R4300iOp::SPECIAL_DIV()
{
    if (m_GPR[m_Opcode.rt].W[0] != 0)
    {
        if (m_GPR[m_Opcode.rs].W[0] != 0x80000000 || m_GPR[m_Opcode.rt].W[0] != -1)
        {
            m_RegLO.DW = m_GPR[m_Opcode.rs].W[0] / m_GPR[m_Opcode.rt].W[0];
            m_RegHI.DW = m_GPR[m_Opcode.rs].W[0] % m_GPR[m_Opcode.rt].W[0];
        }
        else
        {
            m_RegLO.DW = 0xFFFFFFFF80000000;
            m_RegHI.DW = 0x0000000000000000;
        }
    }
    else
    {
        m_RegLO.DW = m_GPR[m_Opcode.rs].W[0] < 0 ? 0x0000000000000001 : 0xFFFFFFFFFFFFFFFF;
        m_RegHI.DW = m_GPR[m_Opcode.rs].W[0];
    }
}

void R4300iOp::SPECIAL_DIVU()
{
    if (m_GPR[m_Opcode.rt].UW[0] != 0)
    {
        m_RegLO.DW = (int32_t)(m_GPR[m_Opcode.rs].UW[0] / m_GPR[m_Opcode.rt].UW[0]);
        m_RegHI.DW = (int32_t)(m_GPR[m_Opcode.rs].UW[0] % m_GPR[m_Opcode.rt].UW[0]);
    }
    else
    {
        m_RegLO.DW = 0xFFFFFFFFFFFFFFFF;
        m_RegHI.DW = m_GPR[m_Opcode.rs].W[0];
    }
}

void R4300iOp::SPECIAL_DMULT()
{
    MIPS_DWORD Tmp[3];

    m_RegLO.UDW = (uint64_t)m_GPR[m_Opcode.rs].UW[0] * (uint64_t)m_GPR[m_Opcode.rt].UW[0];
    Tmp[0].UDW = (int64_t)m_GPR[m_Opcode.rs].W[1] * (int64_t)(uint64_t)m_GPR[m_Opcode.rt].UW[0];
    Tmp[1].UDW = (int64_t)(uint64_t)m_GPR[m_Opcode.rs].UW[0] * (int64_t)m_GPR[m_Opcode.rt].W[1];
    m_RegHI.UDW = (int64_t)m_GPR[m_Opcode.rs].W[1] * (int64_t)m_GPR[m_Opcode.rt].W[1];

    Tmp[2].UDW = (uint64_t)m_RegLO.UW[1] + (uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0];
    m_RegLO.UDW += ((uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0]) << 32;
    m_RegHI.UDW += (uint64_t)Tmp[0].W[1] + (uint64_t)Tmp[1].W[1] + Tmp[2].UW[1];
}

void R4300iOp::SPECIAL_DMULTU()
{
    MIPS_DWORD Tmp[3];

    m_RegLO.UDW = (uint64_t)m_GPR[m_Opcode.rs].UW[0] * (uint64_t)m_GPR[m_Opcode.rt].UW[0];
    Tmp[0].UDW = (uint64_t)m_GPR[m_Opcode.rs].UW[1] * (uint64_t)m_GPR[m_Opcode.rt].UW[0];
    Tmp[1].UDW = (uint64_t)m_GPR[m_Opcode.rs].UW[0] * (uint64_t)m_GPR[m_Opcode.rt].UW[1];
    m_RegHI.UDW = (uint64_t)m_GPR[m_Opcode.rs].UW[1] * (uint64_t)m_GPR[m_Opcode.rt].UW[1];

    Tmp[2].UDW = (uint64_t)m_RegLO.UW[1] + (uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0];
    m_RegLO.UDW += ((uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0]) << 32;
    m_RegHI.UDW += (uint64_t)Tmp[0].UW[1] + (uint64_t)Tmp[1].UW[1] + Tmp[2].UW[1];
}

void R4300iOp::SPECIAL_DDIV()
{
    if (m_GPR[m_Opcode.rt].UDW != 0)
    {
        m_RegLO.DW = m_GPR[m_Opcode.rs].DW / m_GPR[m_Opcode.rt].DW;
        m_RegHI.DW = m_GPR[m_Opcode.rs].DW % m_GPR[m_Opcode.rt].DW;
    }
    else
    {
        m_RegLO.DW = m_GPR[m_Opcode.rs].DW < 0 ? 0x0000000000000001 : 0xFFFFFFFFFFFFFFFF;
        m_RegHI.DW = m_GPR[m_Opcode.rs].DW;
    }
}

void R4300iOp::SPECIAL_DDIVU()
{
    if (m_GPR[m_Opcode.rt].UDW != 0)
    {
        m_RegLO.UDW = m_GPR[m_Opcode.rs].UDW / m_GPR[m_Opcode.rt].UDW;
        m_RegHI.UDW = m_GPR[m_Opcode.rs].UDW % m_GPR[m_Opcode.rt].UDW;
    }
    else
    {
        m_RegLO.DW = 0xFFFFFFFFFFFFFFFF;
        m_RegHI.DW = m_GPR[m_Opcode.rs].DW;
    }
}

void R4300iOp::SPECIAL_ADD()
{
    int32_t rs = m_GPR[m_Opcode.rs].W[0];
    int32_t rt = m_GPR[m_Opcode.rt].W[0];
    int32_t sum = rs + rt;
    if ((~(rs ^ rt) & (rs ^ sum)) & 0x80000000)
    {
        m_Reg.TriggerException(EXC_OV);
        return;
    }
    m_GPR[m_Opcode.rd].DW = sum;
}

void R4300iOp::SPECIAL_ADDU()
{
    m_GPR[m_Opcode.rd].DW = (int32_t)(m_GPR[m_Opcode.rs].UW[0] + m_GPR[m_Opcode.rt].UW[0]);
}

void R4300iOp::SPECIAL_SUB()
{
    int32_t rs = m_GPR[m_Opcode.rs].W[0];
    int32_t rt = m_GPR[m_Opcode.rt].W[0];
    int32_t sub = rs - rt;

    if (((rs ^ rt) & (rs ^ sub)) & 0x80000000)
    {
        m_Reg.TriggerException(EXC_OV);
        return;
    }
    m_GPR[m_Opcode.rd].DW = sub;
}

void R4300iOp::SPECIAL_SUBU()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rs].W[0] - m_GPR[m_Opcode.rt].W[0];
}

void R4300iOp::SPECIAL_AND()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rs].DW & m_GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_OR()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rs].DW | m_GPR[m_Opcode.rt].DW;
#ifdef Interpreter_StackTest
    if (m_Opcode.rd == 29)
    {
        StackValue = m_GPR[m_Opcode.rd].W[0];
    }
#endif
}

void R4300iOp::SPECIAL_XOR()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rs].DW ^ m_GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_NOR()
{
    m_GPR[m_Opcode.rd].DW = ~(m_GPR[m_Opcode.rs].DW | m_GPR[m_Opcode.rt].DW);
}

void R4300iOp::SPECIAL_SLT()
{
    if (m_GPR[m_Opcode.rs].DW < m_GPR[m_Opcode.rt].DW)
    {
        m_GPR[m_Opcode.rd].DW = 1;
    }
    else
    {
        m_GPR[m_Opcode.rd].DW = 0;
    }
}

void R4300iOp::SPECIAL_SLTU()
{
    if (m_GPR[m_Opcode.rs].UDW < m_GPR[m_Opcode.rt].UDW)
    {
        m_GPR[m_Opcode.rd].DW = 1;
    }
    else
    {
        m_GPR[m_Opcode.rd].DW = 0;
    }
}

void R4300iOp::SPECIAL_DADD()
{
    int64_t rs = m_GPR[m_Opcode.rs].DW;
    int64_t rt = m_GPR[m_Opcode.rt].DW;
    int64_t sum = rs + rt;
    if ((~(rs ^ rt) & (rs ^ sum)) & 0x8000000000000000)
    {
        m_Reg.TriggerException(EXC_OV);
        return;
    }
    m_GPR[m_Opcode.rd].DW = sum;
}

void R4300iOp::SPECIAL_DADDU()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rs].DW + m_GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_DSUB()
{
    int64_t rs = m_GPR[m_Opcode.rs].DW;
    int64_t rt = m_GPR[m_Opcode.rt].DW;
    int64_t sub = rs - rt;

    if (((rs ^ rt) & (rs ^ sub)) & 0x8000000000000000)
    {
        m_Reg.TriggerException(EXC_OV);
        return;
    }
    m_GPR[m_Opcode.rd].DW = sub;
}

void R4300iOp::SPECIAL_DSUBU()
{
    m_GPR[m_Opcode.rd].DW = m_GPR[m_Opcode.rs].DW - m_GPR[m_Opcode.rt].DW;
}

void R4300iOp::SPECIAL_TEQ()
{
    if (m_GPR[m_Opcode.rs].DW == m_GPR[m_Opcode.rt].DW)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TGE()
{
    if (m_GPR[m_Opcode.rs].DW >= m_GPR[m_Opcode.rt].DW)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TGEU()
{
    if (m_GPR[m_Opcode.rs].UDW >= m_GPR[m_Opcode.rt].UDW)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TLT()
{
    if (m_GPR[m_Opcode.rs].DW < m_GPR[m_Opcode.rt].DW)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TLTU()
{
    if (m_GPR[m_Opcode.rs].UDW < m_GPR[m_Opcode.rt].UDW)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_TNE()
{
    if (m_GPR[m_Opcode.rs].DW != m_GPR[m_Opcode.rt].DW)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::SPECIAL_DSLL()
{
    m_GPR[m_Opcode.rd].DW = (m_GPR[m_Opcode.rt].DW << m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSRL()
{
    m_GPR[m_Opcode.rd].UDW = (m_GPR[m_Opcode.rt].UDW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSRA()
{
    m_GPR[m_Opcode.rd].DW = (m_GPR[m_Opcode.rt].DW >> m_Opcode.sa);
}

void R4300iOp::SPECIAL_DSLL32()
{
    m_GPR[m_Opcode.rd].DW = (m_GPR[m_Opcode.rt].DW << (m_Opcode.sa + 32));
}

void R4300iOp::SPECIAL_DSRL32()
{
    m_GPR[m_Opcode.rd].UDW = (m_GPR[m_Opcode.rt].UDW >> (m_Opcode.sa + 32));
}

void R4300iOp::SPECIAL_DSRA32()
{
    m_GPR[m_Opcode.rd].DW = (m_GPR[m_Opcode.rt].DW >> (m_Opcode.sa + 32));
}

// R4300i opcodes: RegImm

void R4300iOp::REGIMM_BLTZ()
{
    if (m_GPR[m_Opcode.rs].DW < 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::REGIMM_BGEZ()
{
    if (m_GPR[m_Opcode.rs].DW >= 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
}

void R4300iOp::REGIMM_BLTZL()
{
    if (m_GPR[m_Opcode.rs].DW < 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BGEZL()
{
    if (m_GPR[m_Opcode.rs].DW >= 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::REGIMM_BLTZAL()
{
    if (m_GPR[m_Opcode.rs].DW < 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
    m_GPR[31].DW = (int32_t)(m_System.m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? m_System.m_JumpToLocation + 4 : m_PROGRAM_COUNTER + 8);
}

void R4300iOp::REGIMM_BGEZAL()
{
    if (m_GPR[m_Opcode.rs].DW >= 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.DelayedJump(m_PROGRAM_COUNTER + 8);
    }
    m_GPR[31].DW = (int32_t)(m_System.m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? m_System.m_JumpToLocation + 4 : m_PROGRAM_COUNTER + 8);
}

void R4300iOp::REGIMM_BGEZALL()
{
    if (m_GPR[m_Opcode.rs].DW >= 0)
    {
        m_System.DelayedRelativeJump(((int16_t)m_Opcode.offset << 2) + 4);
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
    m_GPR[31].DW = (int32_t)(m_System.m_PipelineStage == PIPELINE_STAGE_JUMP_DELAY_SLOT ? m_System.m_JumpToLocation + 4 : m_PROGRAM_COUNTER + 8);
}

void R4300iOp::REGIMM_TEQI()
{
    if (m_GPR[m_Opcode.rs].DW == (int64_t)((int16_t)m_Opcode.immediate))
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TGEI()
{
    if (m_GPR[m_Opcode.rs].DW >= (int64_t)((int16_t)m_Opcode.immediate))
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TGEIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    if (m_GPR[m_Opcode.rs].UDW >= (uint64_t)imm64)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TLTI()
{
    if (m_GPR[m_Opcode.rs].DW < (int64_t)((int16_t)m_Opcode.immediate))
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TLTIU()
{
    int32_t imm32 = (int16_t)m_Opcode.immediate;
    int64_t imm64;

    imm64 = imm32;
    if (m_GPR[m_Opcode.rs].UDW < (uint64_t)imm64)
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

void R4300iOp::REGIMM_TNEI()
{
    if (m_GPR[m_Opcode.rs].DW != (int64_t)((int16_t)m_Opcode.immediate))
    {
        m_Reg.TriggerException(EXC_TRAP);
    }
}

// COP0 functions

void R4300iOp::COP0_MF()
{
    m_GPR[m_Opcode.rt].DW = (int32_t)m_Reg.Cop0_MF((CRegisters::COP0Reg)m_Opcode.rd);
}

void R4300iOp::COP0_DMF()
{
    m_GPR[m_Opcode.rt].DW = m_Reg.Cop0_MF((CRegisters::COP0Reg)m_Opcode.rd);
}

void R4300iOp::COP0_MT()
{
    m_Reg.Cop0_MT((CRegisters::COP0Reg)m_Opcode.rd, (int64_t)m_GPR[m_Opcode.rt].W[0]);
}

void R4300iOp::COP0_DMT()
{
    m_Reg.Cop0_MT((CRegisters::COP0Reg)m_Opcode.rd, m_GPR[m_Opcode.rt].UDW);
}
// COP0 CO functions

void R4300iOp::COP0_CO_TLBR()
{
    g_TLB->ReadEntry();
}

void R4300iOp::COP0_CO_TLBWI()
{
    g_TLB->WriteEntry(m_Reg.INDEX_REGISTER & 0x1F, false);
}

void R4300iOp::COP0_CO_TLBWR()
{
    g_TLB->WriteEntry(m_Reg.RANDOM_REGISTER & 0x1F, true);
}

void R4300iOp::COP0_CO_TLBP()
{
    g_TLB->Probe();
}

void R4300iOp::COP0_CO_ERET()
{
    m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
    if ((m_Reg.STATUS_REGISTER.ErrorLevel) != 0)
    {
        m_System.m_JumpToLocation = (uint32_t)m_Reg.ERROREPC_REGISTER;
        m_Reg.STATUS_REGISTER.ErrorLevel = 0;
    }
    else
    {
        m_System.m_JumpToLocation = (uint32_t)m_Reg.EPC_REGISTER;
        m_Reg.STATUS_REGISTER.ExceptionLevel = 0;
    }
    m_LLBit = 0;
    m_Reg.CheckInterrupts();
    m_System.m_TestTimer = true;
}

// COP1 functions
void R4300iOp::CPO1_UNIMPLEMENTED_OP()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
    StatusReg.Cause.UnimplementedOperation = 1;
    m_Reg.TriggerException(EXC_FPE);
}

void R4300iOp::COP1_MF()
{
    if (TestCop1UsableException())
    {
        return;
    }
    m_GPR[m_Opcode.rt].DW = *(int32_t *)m_FPR_S[m_Opcode.fs];
}

void R4300iOp::COP1_DMF()
{
    if (TestCop1UsableException())
    {
        return;
    }
    m_GPR[m_Opcode.rt].DW = *(int64_t *)m_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_CF()
{
    if (TestCop1UsableException())
    {
        return;
    }
    m_GPR[m_Opcode.rt].DW = (int32_t)m_FPCR[m_Opcode.fs];
}

void R4300iOp::COP1_MT()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *(int32_t *)m_FPR_S[m_Opcode.fs] = m_GPR[m_Opcode.rt].W[0];
}

void R4300iOp::COP1_DMT()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *(int64_t *)m_FPR_D[m_Opcode.fs] = m_GPR[m_Opcode.rt].DW;
}

void R4300iOp::COP1_CT()
{
    if (TestCop1UsableException())
    {
        return;
    }
    m_Reg.Cop1_CT(m_Opcode.fs, m_GPR[m_Opcode.rt].W[0]);
}

// COP1: BC1 functions

void R4300iOp::COP1_BCF()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    m_System.m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
    if ((m_FPCR[31] & FPCSR_C) == 0)
    {
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCT()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    m_System.m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
    if ((m_FPCR[31] & FPCSR_C) != 0)
    {
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCFL()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if ((m_FPCR[31] & FPCSR_C) == 0)
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

void R4300iOp::COP1_BCTL()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if ((m_FPCR[31] & FPCSR_C) != 0)
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_DELAY_SLOT;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + ((int16_t)m_Opcode.offset << 2) + 4;
    }
    else
    {
        m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
        m_System.m_JumpToLocation = (m_PROGRAM_COUNTER) + 8;
    }
}

// COP1: S functions
void R4300iOp::COP1_S_ADD()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)m_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)m_FPR_S_L[m_Opcode.fs] + *(float *)m_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_SUB()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)m_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)m_FPR_S_L[m_Opcode.fs] - *(float *)m_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_MUL()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)m_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)m_FPR_S_L[m_Opcode.fs] * *(float *)m_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_DIV()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]) || CheckFPUInput32(*(float *)m_FPR_UW[m_Opcode.ft]))
    {
        return;
    }
    float Result = (*(float *)m_FPR_S_L[m_Opcode.fs] / *(float *)m_FPR_UW[m_Opcode.ft]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_SQRT()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    float Result = sqrtf(*(float *)(m_FPR_S_L[m_Opcode.fs]));
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_ABS()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    float Result = (float)fabs(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_MOV()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = (*(uint64_t *)m_FPR_D[m_Opcode.fs] & 0xFFFFFFFF00000000ll) | *(uint32_t *)m_FPR_S_L[m_Opcode.fs];
}

void R4300iOp::COP1_S_NEG()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    float Result = (*(float *)m_FPR_S_L[m_Opcode.fs] * -1.0f);
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_ROUND_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }

    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_TRUNC_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_CEIL_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_FLOOR_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_ROUND_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_TRUNC_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_CEIL_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_FLOOR_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_CVT_D()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    double Result = (double)(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_CVT_W()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_S_CVT_L()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput32Conv(*(float *)m_FPR_S_L[m_Opcode.fs]))
    {
        return;
    }
    int64_t Result = (int64_t)rint(*(float *)m_FPR_S_L[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_S_CMP()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    float Temp0 = *(float *)m_FPR_S_L[m_Opcode.fs];
    float Temp1 = *(float *)m_FPR_UW[m_Opcode.ft];

    bool less, equal, unorded;
    if (_isnan(Temp0) || _isnan(Temp1))
    {
        less = false;
        equal = false;
        unorded = true;

        bool QuietNan = false;
        if ((*(uint32_t *)m_FPR_S_L[m_Opcode.fs] >= 0x7FC00000 && *(uint32_t *)m_FPR_S_L[m_Opcode.fs] <= 0x7FFFFFFF) ||
            (*(uint32_t *)m_FPR_S_L[m_Opcode.fs] >= 0xFFC00000 && *(uint32_t *)m_FPR_S_L[m_Opcode.fs] <= 0xFFFFFFFF))
        {
            QuietNan = true;
        }
        else if ((*(uint32_t *)m_FPR_UW[m_Opcode.ft] >= 0x7FC00000 && *(uint32_t *)m_FPR_UW[m_Opcode.ft] <= 0x7FFFFFFF) ||
                 (*(uint32_t *)m_FPR_UW[m_Opcode.ft] >= 0xFFC00000 && *(uint32_t *)m_FPR_UW[m_Opcode.ft] <= 0xFFFFFFFF))
        {
            QuietNan = true;
        }
        if ((m_Opcode.funct & 8) != 0 || QuietNan)
        {
            FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                m_Reg.TriggerException(EXC_FPE);
                return;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    else
    {
        less = Temp0 < Temp1;
        equal = Temp0 == Temp1;
        unorded = false;
    }

    int32_t condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) |
                        ((m_Opcode.funct & 1) && unorded);

    if (condition != 0)
    {
        m_FPCR[31] |= FPCSR_C;
    }
    else
    {
        m_FPCR[31] &= ~FPCSR_C;
    }
}

// COP1: D functions
void R4300iOp::COP1_D_ADD()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)m_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)m_FPR_D[m_Opcode.fs] + *(double *)m_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_SUB()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)m_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)m_FPR_D[m_Opcode.fs] - *(double *)m_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_MUL()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)m_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)m_FPR_D[m_Opcode.fs] * *(double *)m_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_DIV()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]) || CheckFPUInput64(*(double *)m_FPR_UDW[m_Opcode.ft]))
    {
        return;
    }
    double Result = (*(double *)m_FPR_D[m_Opcode.fs] / *(double *)m_FPR_UDW[m_Opcode.ft]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

#if defined(_MSC_VER) && (defined(__i386__) || defined(_M_IX86))
static double correct_sqrt(double a)
{
    __asm
    {
        fld QWORD PTR [a]
        fsqrt
    }
}
#endif

void R4300iOp::COP1_D_SQRT()
{
#if defined(_MSC_VER) && (defined(__i386__) || defined(_M_IX86))
    _controlfp(_PC_53, _MCW_PC);
#endif
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
#if defined(_MSC_VER) && (defined(__i386__) || defined(_M_IX86))
    double Result = (double)correct_sqrt(*(double *)m_FPR_D[m_Opcode.fs]);
#else
    double Result = (double)sqrt(*(double *)m_FPR_D[m_Opcode.fs]);
#endif
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_ABS()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    double Result = fabs(*(double *)m_FPR_D[m_Opcode.fs]);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_MOV()
{
    if (TestCop1UsableException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(int64_t *)m_FPR_D[m_Opcode.fs];
}

void R4300iOp::COP1_D_NEG()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    double Result = (*(double *)m_FPR_D[m_Opcode.fs] * -1.0);
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

void R4300iOp::COP1_D_ROUND_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }
    const double & fs = *(double *)m_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_TRUNC_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    const double & fs = *(double *)m_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_CEIL_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    const double & fs = *(double *)m_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_FLOOR_L()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    const double & fs = *(double *)m_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_ROUND_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundToNearest))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)m_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_TRUNC_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardZero))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)m_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CEIL_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardPlusInfinity))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)m_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_FLOOR_W()
{
    if (InitFpuOperation(FPRoundingMode_RoundTowardMinusInfinity))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)m_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CVT_S()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput64(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    float Result = (float)*(double *)m_FPR_D[m_Opcode.fs];
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CVT_W()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    if (CheckFPUInput64Conv(*(double *)m_FPR_D[m_Opcode.fs]))
    {
        return;
    }
    int32_t Result = (int32_t)rint(*(double *)m_FPR_D[m_Opcode.fs]);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_D_CVT_L()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    const double & fs = *(double *)m_FPR_D[m_Opcode.fs];
    if (CheckFPUInput64Conv(fs))
    {
        return;
    }
    double Result = rint(fs);
    if (CheckFPUInvalidException())
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = (uint64_t)Result;
}

void R4300iOp::COP1_D_CMP()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }

    MIPS_DWORD Temp0, Temp1;
    Temp0.DW = *(int64_t *)m_FPR_D[m_Opcode.fs];
    Temp1.DW = *(int64_t *)m_FPR_UDW[m_Opcode.ft];

    bool less, equal, unorded;
    if (_isnan(Temp0.D) || _isnan(Temp1.D))
    {
        less = false;
        equal = false;
        unorded = true;

        bool QuietNan = false;
        if ((*(uint64_t *)m_FPR_D[m_Opcode.fs] >= 0x7FF8000000000000 && *(uint64_t *)m_FPR_D[m_Opcode.fs] <= 0x7FFFFFFFFFFFFFFF) ||
            (*(uint64_t *)m_FPR_D[m_Opcode.fs] >= 0xFFF8000000000000 && *(uint64_t *)m_FPR_D[m_Opcode.fs] <= 0xFFFFFFFFFFFFFFFF))
        {
            QuietNan = true;
        }
        else if ((*(uint64_t *)m_FPR_UDW[m_Opcode.ft] >= 0x7FF8000000000000 && *(uint64_t *)m_FPR_UDW[m_Opcode.ft] <= 0x7FFFFFFFFFFFFFFF) ||
                 (*(uint64_t *)m_FPR_UDW[m_Opcode.ft] >= 0xFFF8000000000000 && *(uint64_t *)m_FPR_UDW[m_Opcode.ft] <= 0xFFFFFFFFFFFFFFFF))
        {
            QuietNan = true;
        }

        if ((m_Opcode.funct & 8) != 0 || QuietNan)
        {
            FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                m_Reg.TriggerException(EXC_FPE);
                return;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    else
    {
        less = Temp0.D < Temp1.D;
        equal = Temp0.D == Temp1.D;
        unorded = false;
    }

    int32_t condition = ((m_Opcode.funct & 4) && less) | ((m_Opcode.funct & 2) && equal) |
                        ((m_Opcode.funct & 1) && unorded);

    if (condition != 0)
    {
        m_FPCR[31] |= FPCSR_C;
    }
    else
    {
        m_FPCR[31] &= ~FPCSR_C;
    }
}

// COP1: W functions
void R4300iOp::COP1_W_CVT_S()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    float Result = (float)*(int32_t *)m_FPR_S_L[m_Opcode.fs];
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_W_CVT_D()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    double Result = (double)*(int32_t *)m_FPR_S_L[m_Opcode.fs];
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

// COP1: L functions

void R4300iOp::COP1_L_CVT_S()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    int64_t fs = *(int64_t *)m_FPR_D[m_Opcode.fs];
    if (fs >= (int64_t)0x0080000000000000ull || fs < (int64_t)0xff80000000000000ull)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        m_Reg.TriggerException(EXC_FPE);
        return;
    }
    float Result = (float)fs;
    if (CheckFPUResult32(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint32_t *)&Result;
}

void R4300iOp::COP1_L_CVT_D()
{
    if (InitFpuOperation(((FPStatusReg &)m_FPCR[31]).RoundingMode))
    {
        return;
    }
    int64_t fs = *(int64_t *)m_FPR_D[m_Opcode.fs];
    if (fs >= (int64_t)0x0080000000000000ull || fs < (int64_t)0xff80000000000000ull)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        m_Reg.TriggerException(EXC_FPE);
        return;
    }
    double Result = (double)fs;
    if (CheckFPUResult64(Result))
    {
        return;
    }
    *m_FPR_UDW[m_Opcode.fd] = *(uint64_t *)&Result;
}

// COP2 functions
void R4300iOp::CPO2_INVALID_OP(void)
{
    m_Reg.TriggerException(m_Reg.STATUS_REGISTER.CU2 == 0 ? EXC_CPU : EXC_II, 2);
}

void R4300iOp::COP2_MF()
{
    if (m_Reg.STATUS_REGISTER.CU2 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 2);
    }
    else
    {
        m_GPR[m_Opcode.rt].DW = (int32_t)m_Reg.Cop2_MF(m_Opcode.rd);
    }
}

void R4300iOp::COP2_DMF()
{
    if (m_Reg.STATUS_REGISTER.CU2 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 2);
    }
    else
    {
        m_GPR[m_Opcode.rt].DW = m_Reg.Cop2_MF(m_Opcode.rd);
    }
}

void R4300iOp::COP2_CF()
{
    if (m_Reg.STATUS_REGISTER.CU2 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 2);
    }
    else
    {
        m_GPR[m_Opcode.rt].DW = (int32_t)m_Reg.Cop2_MF(m_Opcode.rd);
    }
}

void R4300iOp::COP2_MT()
{
    if (m_Reg.STATUS_REGISTER.CU2 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 2);
    }
    else
    {
        m_Reg.Cop2_MT((CRegisters::COP0Reg)m_Opcode.rd, m_GPR[m_Opcode.rt].DW);
    }
}

void R4300iOp::COP2_DMT()
{
    if (m_Reg.STATUS_REGISTER.CU2 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 2);
    }
    else
    {
        m_Reg.Cop2_MT((CRegisters::COP0Reg)m_Opcode.rd, m_GPR[m_Opcode.rt].DW);
    }
}

void R4300iOp::COP2_CT()
{
    if (m_Reg.STATUS_REGISTER.CU2 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 2);
    }
    else
    {
        m_Reg.Cop2_MT((CRegisters::COP0Reg)m_Opcode.rd, m_GPR[m_Opcode.rt].DW);
    }
}

// Other functions
void R4300iOp::ReservedInstruction()
{
    m_Reg.TriggerException(EXC_II);
}

void R4300iOp::UnknownOpcode()
{
    if (HaveDebugger())
    {
        g_Settings->SaveBool(Debugger_SteppingOps, true);
        g_Debugger->WaitForStep();
    }
    else
    {
        R4300iInstruction Opcode(m_PROGRAM_COUNTER, m_Opcode.Value);
        g_Notify->DisplayError(stdstr_f("%s: %08X\n%s %s\n\nStopping emulation", GS(MSG_UNHANDLED_OP), (m_PROGRAM_COUNTER), Opcode.Name(), Opcode.Param()).c_str());
        m_System.m_EndEmulation = true;

        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

bool R4300iOp::TestCop1UsableException(void)
{
    if (m_Reg.STATUS_REGISTER.CU1 == 0)
    {
        m_Reg.TriggerException(EXC_CPU, 1);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput32(const float & Value)
{
    bool Exception = false;
    if ((*((uint32_t *)&Value) & 0x7F800000) == 0x00000000 && (*((uint32_t *)&Value) & 0x007FFFFF) != 0x00000000) // Sub Normal
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        Exception = true;
    }
    else if ((*((uint32_t *)&Value) & 0x7F800000) == 0x7F800000 && (*((uint32_t *)&Value) & 0x007FFFFF) != 0x00000000) // Nan
    {
        uint32_t Value32 = *(uint32_t *)&Value;
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        if ((Value32 >= 0x7F800001 && Value32 < 0x7FC00000) ||
            (Value32 >= 0xFF800001 && Value32 < 0xFFC00000))
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            Exception = true;
        }
        else
        {
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                Exception = true;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    if (Exception)
    {
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput32Conv(const float & Value)
{
    uint32_t InvalidValueMax = 0x5a000000, InvalidMinValue = 0xda000000;

    int Type = fpclassify(Value);
    if (Type == FP_SUBNORMAL || Type == FP_INFINITE || Type == FP_NAN ||
        Value >= *(float *)&InvalidValueMax || Value <= *(float *)&InvalidMinValue)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput64(const double & Value)
{
    int Type = fpclassify(Value);
    bool Exception = false;
    if (Type == FP_SUBNORMAL)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        Exception = true;
    }
    else if (Type == FP_NAN)
    {
        uint64_t Value64 = *(uint64_t *)&Value;
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        if ((Value64 >= 0x7FF0000000000001 && Value64 <= 0x7FF7FFFFFFFFFFFF) ||
            (Value64 >= 0xFFF0000000000001 && Value64 <= 0xFFF7FFFFFFFFFFFF))
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            Exception = true;
        }
        else
        {
            StatusReg.Cause.InvalidOperation = 1;
            if (StatusReg.Enable.InvalidOperation)
            {
                Exception = true;
            }
            else
            {
                StatusReg.Flags.InvalidOperation = 1;
            }
        }
    }
    if (Exception)
    {
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInput64Conv(const double & Value)
{
    uint64_t InvalidValueMax = 0x4340000000000000, InvalidMinValue = 0xc340000000000000;

    int Type = fpclassify(Value);
    if (Type == FP_SUBNORMAL || Type == FP_INFINITE || Type == FP_NAN ||
        Value >= *(double *)&InvalidValueMax || Value <= *(double *)&InvalidMinValue)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        StatusReg.Cause.UnimplementedOperation = 1;
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUResult32(float & Result)
{
    int Except = fetestexcept(FE_ALL_EXCEPT);
    bool DoException = false;

    if ((*((uint32_t *)&Result) & 0x7F800000) == 0x7F800000 && (*((uint32_t *)&Result) & 0x007FFFFF) != 0x00000000) // Nan
    {
        if (Except == 0 || !SetFPUException())
        {
            *((uint32_t *)&Result) = 0x7fbfffff;
        }
        else
        {
            DoException = true;
        }
    }
    else if ((*((uint32_t *)&Result) & 0x7F800000) == 0x00000000 && (*((uint32_t *)&Result) & 0x007FFFFF) != 0x00000000) // Sub Normal
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        if (!StatusReg.FlushSubnormals || StatusReg.Enable.Underflow || StatusReg.Enable.Inexact)
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            DoException = true;
        }
        else if (Except == 0 || !SetFPUException())
        {
            StatusReg.Cause.Underflow = 1;
            StatusReg.Flags.Underflow = 1;

            StatusReg.Cause.Inexact = 1;
            StatusReg.Flags.Inexact = 1;

            switch (StatusReg.RoundingMode)
            {
            case FPRoundingMode_RoundToNearest:
            case FPRoundingMode_RoundTowardZero:
                Result = Result >= 0.0f ? 0.0f : -0.0f;
                break;
            case FPRoundingMode_RoundTowardPlusInfinity:
                Result = Result >= 0.0f ? 1.175494351e-38F : -0.0f;
                break;
            case FPRoundingMode_RoundTowardMinusInfinity:
                Result = Result >= 0.0f ? 0.0f : -1.175494351e-38F;
                break;
            }
        }
        else
        {
            DoException = true;
        }
    }
    else if (Except != 0 && SetFPUException())
    {
        DoException = true;
    }
    if (DoException)
    {
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUResult64(double & Result)
{
    int Except = fetestexcept(FE_ALL_EXCEPT);
    bool DoException = false;
    int fptype = fpclassify(Result);
    if (fptype == FP_NAN)
    {
        if (Except == 0 || !SetFPUException())
        {
            *((uint64_t *)&Result) = 0x7FF7FFFFFFFFFFFF;
        }
        else
        {
            DoException = true;
        }
    }
    else if (fptype == FP_SUBNORMAL)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
        if (!StatusReg.FlushSubnormals || StatusReg.Enable.Underflow || StatusReg.Enable.Inexact)
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            DoException = true;
        }
        else if (Except == 0 || !SetFPUException())
        {
            StatusReg.Cause.Underflow = 1;
            StatusReg.Flags.Underflow = 1;

            StatusReg.Cause.Inexact = 1;
            StatusReg.Flags.Inexact = 1;

            switch (StatusReg.RoundingMode)
            {
            case FPRoundingMode_RoundToNearest:
            case FPRoundingMode_RoundTowardZero:
                Result = Result >= 0.0 ? 0.0 : -0.0;
                break;
            case FPRoundingMode_RoundTowardPlusInfinity:
                Result = Result >= 0.0 ? 2.2250738585072014e-308 : -0.0;
                break;
            case FPRoundingMode_RoundTowardMinusInfinity:
                Result = Result >= 0.0 ? 0.0 : -2.2250738585072014e-308;
                break;
            }
        }
        else
        {
            DoException = true;
        }
    }
    else if (Except != 0 && SetFPUException())
    {
        DoException = true;
    }
    if (DoException)
    {
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::CheckFPUInvalidException(void)
{
    int Except = fetestexcept(FE_ALL_EXCEPT);
    if (Except == 0)
    {
        return false;
    }

    FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
    bool Res = false;
    if ((Except & FE_INVALID) != 0)
    {
        StatusReg.Cause.UnimplementedOperation = 1;
        Res = true;
    }
    else if ((Except & FE_INEXACT) != 0)
    {
        StatusReg.Cause.Inexact = 1;
        if (StatusReg.Enable.Inexact)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Inexact = 1;
        }
    }

    if (Res)
    {
        m_Reg.TriggerException(EXC_FPE);
        return true;
    }
    return false;
}

bool R4300iOp::InitFpuOperation(FPRoundingMode RoundingModel)
{
    if (TestCop1UsableException())
    {
        return true;
    }
    m_FPCR[31] &= ~0x0003F000;
    switch (RoundingModel)
    {
    case FPRoundingMode_RoundToNearest: fesetround(FE_TONEAREST); break;
    case FPRoundingMode_RoundTowardZero: fesetround(FE_TOWARDZERO); break;
    case FPRoundingMode_RoundTowardPlusInfinity: fesetround(FE_UPWARD); break;
    case FPRoundingMode_RoundTowardMinusInfinity: fesetround(FE_DOWNWARD); break;
    }
    feclearexcept(FE_ALL_EXCEPT);
    return false;
}

bool R4300iOp::SetFPUException(void)
{
    FPStatusReg & StatusReg = (FPStatusReg &)m_FPCR[31];
    int Except = fetestexcept(FE_ALL_EXCEPT);
    if ((Except & FE_UNDERFLOW) != 0)
    {
        if (StatusReg.FlushSubnormals == 0 || StatusReg.Enable.Underflow || StatusReg.Enable.Inexact)
        {
            StatusReg.Cause.UnimplementedOperation = 1;
            return true;
        }
    }

    bool Res = false;
    if ((Except & FE_INEXACT) != 0)
    {
        StatusReg.Cause.Inexact = 1;
        if (StatusReg.Enable.Inexact)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Inexact = 1;
        }
    }
    if ((Except & FE_UNDERFLOW) != 0)
    {
        StatusReg.Cause.Underflow = 1;
        if (StatusReg.Enable.Underflow)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Underflow = 1;
        }
    }
    if ((Except & FE_OVERFLOW) != 0)
    {
        StatusReg.Cause.Overflow = 1;
        if (StatusReg.Enable.Overflow)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.Overflow = 1;
        }
    }
    if ((Except & FE_DIVBYZERO) != 0)
    {
        StatusReg.Cause.DivisionByZero = 1;
        if (StatusReg.Enable.DivisionByZero)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.DivisionByZero = 1;
        }
    }
    if ((Except & FE_INVALID) != 0)
    {
        StatusReg.Cause.InvalidOperation = 1;
        if (StatusReg.Enable.InvalidOperation)
        {
            Res = true;
        }
        else
        {
            StatusReg.Flags.InvalidOperation = 1;
        }
    }
    return Res;
}
