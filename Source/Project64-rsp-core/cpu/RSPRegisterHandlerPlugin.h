#pragma once
#include "RSPRegisterHandler.h"

class RSPRegisterHandlerPlugin :
    public RSPRegisterHandler
{
public:
    RSPRegisterHandlerPlugin(_RSP_INFO & RSPInfo, const uint32_t & RdramSize);

    uint32_t & PendingSPMemAddr();
    uint32_t & PendingSPDramAddr();

private:
    void ClearSPInterrupt(void);
    void SetSPInterrupt(void);
    void SetHalt(void);
    void DmaReadDone(uint32_t End);
};
