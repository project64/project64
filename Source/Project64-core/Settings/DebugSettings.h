#pragma once

#include <Project64-core/N64System/N64Types.h>

struct DebugSettings
{
    bool haveDebugger;
    bool debugging;
    bool stepping;
    bool skipOp;
    bool waitingForStep;
    bool recordRecompilerAsm;
    bool recordExecutionTimes;
    bool haveExecutionBP;
    bool haveWriteBP;
    bool haveReadBP;
    bool showPifRamErrors;
    bool cpuLoggingEnabled;
    uint32_t exceptionBreakpoints;
    uint32_t fpExceptionBreakpoints;
    uint32_t intrBreakpoints;
    uint32_t rcpIntrBreakpoints;
    bool endOnPermLoop;
    bool fpuExceptionInRecompiler;
    bool breakOnUnhandledMemory;
    bool breakOnAddressError;
    bool stepOnBreakOpCode;
    bool trackCPUStepStarted;
    bool trackCPUStepEnded;
};

extern DebugSettings g_DebugSettings;

void SetupDebugSettings(void);
void ShutdownDebugSettings(void);
