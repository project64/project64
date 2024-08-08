#pragma once
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <stdint.h>

class CRSPSystem
{
    friend class RSPOp;
    friend class CRSPRecompilerOps;
    friend class CRSPRecompiler;
    friend class RSPDebuggerUI;
    friend void UpdateRSPRegistersScreen(void);

public:
    CRSPSystem();

    void Reset();

    uint32_t RunInterpreterCPU(uint32_t Cycles);

private:
    CRSPRegisters m_Reg;
    RSPOp m_Op;
    RSPOpcode m_OpCode;
};

extern CRSPSystem RSPSystem;