#include "RSPInterpreterCPU.h"
#include "RSPCpu.h"
#include "RSPInterpreterOps.h"
#include "RSPRegisters.h"
#include <Project64-rsp-core/RSPDebugger.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>

RSPPIPELINE_STAGE RSP_NextInstruction;
uint32_t RSP_JumpTo;