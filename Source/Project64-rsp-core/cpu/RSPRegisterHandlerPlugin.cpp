#include "RSPRegisterHandlerPlugin.h"
#include "RSPCpu.h"
#include "RSPRegisters.h"
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Recompiler/RspRecompilerCPU.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>

RSPRegisterHandlerPlugin::RSPRegisterHandlerPlugin(CRSPSystem & System) :
    RSPRegisterHandler(System),
    m_System(System)
{
}

uint32_t & RSPRegisterHandlerPlugin::PendingSPMemAddr()
{
    return m_PendingSPMemAddr;
}

uint32_t & RSPRegisterHandlerPlugin::PendingSPDramAddr()
{
    return m_PendingSPDramAddr;
}

void RSPRegisterHandlerPlugin::ClearSPInterrupt(void)
{
    *RSPInfo.MI_INTR_REG &= ~MI_INTR_SP;
}

void RSPRegisterHandlerPlugin::SetSPInterrupt(void)
{
    *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
    RSPInfo.CheckInterrupts();
    RSP_Running = false;
}

void RSPRegisterHandlerPlugin::SetHalt(void)
{
    RSP_Running = false;
}

void RSPRegisterHandlerPlugin::DmaReadDone(uint32_t End)
{
    if (CPUMethod() == RSPCpuMethod::Recompiler && (*RSPInfo.SP_MEM_ADDR_REG & 0x1000) != 0)
    {
        m_System.m_Recompiler.SetJumpTable(End);
    }
}
