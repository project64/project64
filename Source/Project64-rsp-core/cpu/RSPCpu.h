#pragma once

#include "RSPOpcode.h"
#include "RSPRegisterHandlerPlugin.h"
#include "RspTypes.h"
#include <memory>

extern UDWORD EleSpec[16], Indx[16];

extern uint32_t RSP_Running;

void Build_RSP(void);

extern uint32_t Mfc0Count, SemaphoreExit;
