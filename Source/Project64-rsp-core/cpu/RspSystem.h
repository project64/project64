#pragma once
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <stdint.h>

class CRSPSystem
{
public:
    CRSPSystem();

    uint32_t RunInterpreterCPU(uint32_t Cycles);

private:
    RSPOp m_OpCodes;
};

extern CRSPSystem RSPSystem;