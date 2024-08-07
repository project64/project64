#include <Project64-rsp-core/RSPDebugger.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPInterpreterCPU.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspSystem.h>

CRSPSystem RSPSystem;

CRSPSystem::CRSPSystem() :
    m_OpCodes(*this)
{
}

void CRSPSystem::Reset()
{
    m_Reg.Reset();
}

uint32_t CRSPSystem::RunInterpreterCPU(uint32_t Cycles)
{
    uint32_t CycleCount;
    RSP_Running = true;
    if (g_RSPDebugger != nullptr)
    {
        g_RSPDebugger->StartingCPU();
    }
    CycleCount = 0;
    uint32_t & GprR0 = m_Reg.m_GPR[0].UW;

    while (RSP_Running)
    {
        if (g_RSPDebugger != nullptr)
        {
            g_RSPDebugger->BeforeExecuteOp();
        }
        RSPOpC.Value = *(uint32_t *)(RSPInfo.IMEM + (*PrgCount & 0xFFC));
        (m_OpCodes.*(m_OpCodes.Jump_Opcode[RSPOpC.op]))();
        GprR0 = 0x00000000; // MIPS $zero hard-wired to 0

        switch (RSP_NextInstruction)
        {
        case RSPPIPELINE_NORMAL:
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            break;
        case RSPPIPELINE_DELAY_SLOT:
            RSP_NextInstruction = RSPPIPELINE_JUMP;
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            break;
        case RSPPIPELINE_JUMP:
            RSP_NextInstruction = RSPPIPELINE_NORMAL;
            *PrgCount = RSP_JumpTo;
            break;
        case RSPPIPELINE_SINGLE_STEP:
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            RSP_NextInstruction = RSPPIPELINE_SINGLE_STEP_DONE;
            break;
        case RSPPIPELINE_SINGLE_STEP_DONE:
            *PrgCount = (*PrgCount + 4) & 0xFFC;
            *RSPInfo.SP_STATUS_REG |= SP_STATUS_HALT;
            RSP_Running = false;
            break;
        }
    }
    return Cycles;
}
