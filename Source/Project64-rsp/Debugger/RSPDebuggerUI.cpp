#include "RSPDebuggerUI.h"
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Recompiler/RspProfiling.h>
#include <Project64-rsp-core/Recompiler/RspRecompilerCPU.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPInstruction.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Project64-rsp/RSP Command.h>
#include <Project64-rsp/breakpoint.h>

void UpdateRSPRegistersScreen(void);

RSPDebuggerUI::RSPDebuggerUI(CRSPSystem & System) :
    m_System(System),
    m_OpCode(System.m_OpCode)
{
}

void RSPDebuggerUI::ResetTimerList(void)
{
    ::ResetTimerList();
}

void RSPDebuggerUI::StartingCPU(void)
{
    Enable_RSP_Commands_Window();
}

void RSPDebuggerUI::RspCyclesStart(void)
{
    uint32_t TaskType = *(uint32_t *)(RSPInfo.DMEM + 0xFC0);
    Compiler.bAudioUcode = (TaskType == 2) ? true : false;
    if (Profiling && !IndvidualBlock)
    {
        StartTimer((uint32_t)Timer_RSP_Running);
    }
    if (BreakOnStart)
    {
        Enter_RSP_Commands_Window();
    }
}

void RSPDebuggerUI::RspCyclesStop(void)
{
    if (Profiling && !IndvidualBlock)
    {
        StartTimer((DWORD)Timer_R4300_Running);
    }
}

void RSPDebuggerUI::BeforeExecuteOp(void)
{
    if (NoOfBpoints != 0)
    {
        if (CheckForRSPBPoint(*RSPInfo.SP_PC_REG))
        {
            if (InRSPCommandsWindow)
            {
                Enter_RSP_Commands_Window();
                if (Stepping_Commands)
                {
                    DisplayError("Encountered an R4300i breakpoint");
                }
                else
                {
                    DisplayError("Encountered an R4300i breakpoint\n\nNow stepping");
                    SetRSPCommandViewto(*RSPInfo.SP_PC_REG);
                    SetRSPCommandToStepping();
                }
            }
            else
            {
                DisplayError("Encountered an RSP breakpoint\n\nEntering command window");
                Enter_RSP_Commands_Window();
            }
        }
    }

    if (Stepping_Commands)
    {
        WaitingForStep = true;
        SetRSPCommandViewto(*RSPInfo.SP_PC_REG);
        UpdateRSPRegistersScreen();
        while (WaitingForStep != 0)
        {
            Sleep(20);
            if (!Stepping_Commands)
            {
                WaitingForStep = false;
            }
        }
    }
}

void RSPDebuggerUI::UnknownOpcode(void)
{
    char Message[200];
    int response;

    if (InRSPCommandsWindow)
    {
        SetRSPCommandViewto(*RSPInfo.SP_PC_REG);
        DisplayError("Unhandled Opcode\n%s\n\nStopping emulation", RSPInstruction(*RSPInfo.SP_PC_REG, m_OpCode.Value).NameAndParam().c_str());
    }
    else
    {
        sprintf(Message, "Unhandled Opcode\n%s\n\nStopping emulation.\n\nWould you like to open the debugger?",
                RSPInstruction(*RSPInfo.SP_PC_REG, m_OpCode.Value).NameAndParam().c_str());
        response = MessageBoxA(NULL, Message, "Error", MB_YESNO | MB_ICONERROR);
        if (response == IDYES)
        {
            Enter_RSP_Commands_Window();
        }
    }
    ExitThread(0);
}

void RSPDebuggerUI::RDP_LogMF0(uint32_t PC, uint32_t Reg)
{
    if (LogRDP && CRSPSettings::CPUMethod() == RSPCpuMethod::Interpreter)
    {
        RDPLog.LogMF0(PC, Reg);
    }
}
