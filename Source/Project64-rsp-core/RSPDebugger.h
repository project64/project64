#pragma once
#include <stdint.h>

#if !defined(_WIN32) && !defined(__interface)
#define __interface struct
#endif

__interface RSPDebugger
{
    virtual void ResetTimerList(void) = 0;
    virtual void StartingCPU(void) = 0;
    virtual void RspCyclesStart(void) = 0;
    virtual void RspCyclesStop(void) = 0;
    virtual void BeforeExecuteOp(void) = 0;
    virtual void UnknownOpcode(void) = 0;
    virtual void RDP_LogMF0(uint32_t PC, uint32_t Reg) = 0;
};

extern RSPDebugger * g_RSPDebugger;
