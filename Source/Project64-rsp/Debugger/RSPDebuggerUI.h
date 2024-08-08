#pragma once
#include <Project64-rsp-core/RSPDebugger.h>
#include <Project64-rsp-core/cpu/RSPOpcode.h>

class CRSPSystem;

class RSPDebuggerUI :
    public RSPDebugger
{
public:
    RSPDebuggerUI(CRSPSystem & System);

    void ResetTimerList(void);
    void StartingCPU(void);
    void RspCyclesStart(void);
    void RspCyclesStop(void);
    void BeforeExecuteOp(void);
    void UnknownOpcode(void);
    void RDP_LogMF0(uint32_t PC, uint32_t Reg);

private:
    CRSPSystem & m_System;
    RSPOpcode & m_OpCode;
};
