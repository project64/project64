#pragma once
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <stdint.h>

class CRSPSystem
{
    friend class RSPOp;
    friend class CRSPRecompilerOps;
    friend void UpdateRSPRegistersScreen(void);

public:
    CRSPSystem();

    void Reset();

    uint32_t RunInterpreterCPU(uint32_t Cycles);

private:
    CRSPRegisters m_Reg;
    RSPOp m_OpCodes;
};

extern CRSPSystem RSPSystem;