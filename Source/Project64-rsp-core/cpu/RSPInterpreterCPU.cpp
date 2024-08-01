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

unsigned int RSP_branch_if(int condition)
{
    unsigned int new_PC;

    if (condition)
    {
        new_PC = *PrgCount + 4 + ((short)RSPOpC.offset << 2);
    }
    else
    {
        new_PC = *PrgCount + 4 + 4;
    }
    return (new_PC & 0xFFC);
}
