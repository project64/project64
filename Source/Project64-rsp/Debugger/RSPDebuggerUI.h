#pragma once
#include <Project64-rsp-core\RSPDebugger.h>

class RSPDebuggerUI :
    public RSPDebugger
{
public:
    void ResetTimerList(void);
    void StartingCPU(void);
    void RspCyclesStart(void);
    void RspCyclesStop(void);
    void BeforeExecuteOp(void);
    void UnknownOpcode(void);
    void RDP_LogMF0(uint32_t PC, uint32_t Reg);
};
