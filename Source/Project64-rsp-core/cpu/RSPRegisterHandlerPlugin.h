#pragma once
#include "RSPRegisterHandler.h"
#include <Project64-rsp-core/Settings/RspSettings.h>

class CRSPSystem;

class RSPRegisterHandlerPlugin :
    public RSPRegisterHandler,
    private CRSPSettings
{
public:
    RSPRegisterHandlerPlugin(CRSPSystem & System);

    uint32_t & PendingSPMemAddr();
    uint32_t & PendingSPDramAddr();

private:
    void ClearSPInterrupt(void);
    void SetSPInterrupt(void);
    void SetHalt(void);
    void DmaReadDone(uint32_t End);

    CRSPSystem & m_System;
};
