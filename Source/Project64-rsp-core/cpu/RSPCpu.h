#pragma once

#include "RSPOpcode.h"
#include "RSPRegisterHandlerPlugin.h"
#include "RspTypes.h"
#include <memory>

enum RSPCpuType
{
    InterpreterCPU = 0,
    RecompilerCPU = 1,
};

extern UDWORD EleSpec[16], Indx[16];

extern uint32_t *PrgCount, RSP_Running;

void SetCPU(RSPCpuType core);
void Build_RSP(void);

extern uint32_t Mfc0Count, SemaphoreExit;
extern RSPCpuType g_CPUCore;
