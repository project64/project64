#pragma once
#include <stdint.h>

__interface RSPDebugger
{
    void ResetTimerList(void) = 0;
    void StartingCPU(void) = 0;
    void RspCyclesStart(void) = 0;
    void RspCyclesStop(void) = 0;
    void BeforeExecuteOp(void) = 0;
    void UnknownOpcode(void) = 0;
    void RDP_LogMF0(uint32_t PC, uint32_t Reg) = 0;
};

extern RSPDebugger * g_RSPDebugger;
