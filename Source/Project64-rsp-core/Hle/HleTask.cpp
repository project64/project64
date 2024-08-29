#include <Project64-rsp-core/Hle/HleTask.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <zlib/zlib.h>

CHleTask::CHleTask(CRSPSystem & System) :
    m_hle(System),
    m_System(System),
    m_SP_STATUS_REG(System.m_SP_STATUS_REG),
    m_SP_PC_REG(System.m_SP_PC_REG),
    m_DPC_STATUS_REG(System.m_DPC_STATUS_REG),
    m_DMEM(System.m_DMEM),
    m_IMEM(System.m_IMEM),
    CheckInterrupts(System.CheckInterrupts),
    ProcessDList(System.ProcessDList)
{
}

bool CHleTask::IsHleTask(void)
{
    if ((*m_SP_PC_REG) != 0)
    {
        return false;
    }
    uint32_t ImemCrc = crc32(0L, m_IMEM, 0xCC);
    if (ImemCrc == 0xcab15710 || // Super Mario
        ImemCrc == 0x6f849879)   // pokemon puzzle league
    {
        return true;
    }
    return false;
}

bool CHleTask::ProcessHleTask(void)
{
    TASK_INFO & TaskInfo = *((TASK_INFO *)(m_DMEM + 0xFC0));
    extern bool AudioHle, GraphicsHle;

    if (((HLETaskType)TaskInfo.Type) == HLETaskType::Video && GraphicsHle && TaskInfo.DataPtr != 0)
    {
        if (ProcessDList != nullptr)
        {
            ProcessDList();
        }
        *m_SP_STATUS_REG |= (0x0203);
        if ((*m_SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
            RSPInfo.CheckInterrupts();
        }

        *RSPInfo.DPC_STATUS_REG &= ~0x0002;
        return true;
    }
    else if (TaskInfo.Type == 7)
    {
        RSPInfo.ShowCFB();
    }

    if (CRSPSettings::CPUMethod() == RSPCpuMethod::HighLevelEmulation && m_hle.try_fast_audio_dispatching())
    {
        *m_SP_STATUS_REG |= SP_STATUS_SIG2 | SP_STATUS_BROKE | SP_STATUS_HALT;
        if ((*m_SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            *RSPInfo.MI_INTR_REG |= MI_INTR_SP;
            RSPInfo.CheckInterrupts();
        }
        return true;
    }
    return false;
}
