#include <Project64-rsp-core/Hle/HleTask.h>
#include <Project64-rsp-core/cpu/RSPRegisterHandlerPlugin.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <zlib/zlib.h>

CHleTask::CHleTask(CRSPSystem & System) :
    CGPRRegisters(System.m_Reg.m_GPR),
    m_hle(System),
    m_System(System),
    m_Recompiler(System.m_Recompiler),
    m_RSPRegisterHandler(System.m_RSPRegisterHandler),
    m_MI_INTR_REG(System.m_MI_INTR_REG),
    m_SP_STATUS_REG(System.m_SP_STATUS_REG),
    m_SP_DMA_FULL_REG(System.m_SP_DMA_FULL_REG),
    m_SP_DMA_BUSY_REG(System.m_SP_DMA_BUSY_REG),
    m_SP_PC_REG(System.m_SP_PC_REG),
    m_SP_SEMAPHORE_REG(System.m_SP_SEMAPHORE_REG),
    m_DPC_STATUS_REG(System.m_DPC_STATUS_REG),
    m_DMEM(System.m_DMEM),
    m_IMEM(System.m_IMEM),
    m_UcodeCRC(0),
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

void CHleTask::SetupCommandList(TASK_INFO & TaskInfo)
{
    uint32_t JumpTableLength = 0x7E, JumpTablePos = 0x10;
    if ((HLETaskType)(TaskInfo.Type) == HLETaskType::Audio)
    {
        if (*((uint32_t *)&m_DMEM[0]) == 0x00000001 && *((uint32_t *)&m_DMEM[0x30]) == 0xf0000f00)
        {
            JumpTableLength = 0x10;
            JumpTablePos = 0x10;
        }
    }

    uint32_t JumpTableCRC = crc32(0L, m_IMEM + JumpTablePos, JumpTableLength << 1);
    TaskFunctionMap::iterator itr = m_FunctionMap.find(JumpTableCRC);
    if (itr != m_FunctionMap.end())
    {
        m_TaskFunctions = &itr->second;
        return;
    }

    if (m_FunctionMap.size() > 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_TaskFunctions = nullptr;

    memset(&m_Recompiler.m_CurrentBlock, 0, sizeof(m_Recompiler.m_CurrentBlock));
    m_Recompiler.BuildBranchLabels();
    TaskFunctions JumpFunctions;
    for (uint32_t i = 0, n = JumpTableLength; i < n; i++)
    {
        uint16_t FuncAddress = *((uint16_t *)(m_DMEM + (((i << 1) + JumpTablePos) ^ 2)));
        if (FuncAddress != 0x1118)
        {
            m_Recompiler.CompileHLETask(FuncAddress);
            void * FuncPtr = *(JumpTable + ((FuncAddress & 0xFFF) >> 2));
            JumpFunctions.emplace_back(TaskFunctionAddress(FuncAddress, FuncPtr));
        }
        else
        {
            JumpFunctions.emplace_back(TaskFunctionAddress(FuncAddress, nullptr));
        }
    }
    m_Recompiler.LinkBranches(&m_Recompiler.m_CurrentBlock);
    m_FunctionMap[JumpTableCRC] = JumpFunctions;
    itr = m_FunctionMap.find(JumpTableCRC);
    if (itr == m_FunctionMap.end())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_TaskFunctions = &itr->second;
}

void CHleTask::ExecuteTask_1a13a51a(TASK_INFO & TaskInfo)
{
    *((uint32_t *)(m_DMEM + 0x320)) = 0;
    GPR_T8 = 0x360;
    GPR_S7 = 0xF90;
    if ((*m_DPC_STATUS_REG & 1) != 0 || *m_SP_SEMAPHORE_REG != 0 || *m_SP_DMA_FULL_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, 0x380);
    m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, TaskInfo.DataPtr);
    m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, 0x13F);
    if (*m_SP_DMA_BUSY_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    *m_SP_SEMAPHORE_REG = 0;
    if (SyncCPU)
    {
        *m_SP_PC_REG = 0x0E4;
        RSPSystem.SyncSystem()->ExecuteOps(200, *m_SP_PC_REG);
        RSPSystem.BasicSyncCheck();
    }

    GPR_GP = TaskInfo.DataPtr;
    GPR_K1 = TaskInfo.DataSize;
    GPR_SP = 0x380;
    GPR_S8 = 0x140;
    static uint32_t TaskCount = 0;
    for (;;)
    {
        GPR_K0 = *((uint32_t *)(m_DMEM + GPR_SP));
        GPR_T9 = *((uint32_t *)(m_DMEM + GPR_SP + 4));

        uint32_t Index = (GPR_K0 >> 0x18) & 0x7F;
        GPR_GP += 8;
        GPR_K1 -= 8;
        GPR_SP += 8;
        GPR_S8 -= 8;
        if (Index >= m_TaskFunctions->size())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        TaskFunctionAddress FunctionAddress = (*m_TaskFunctions)[Index];
        *m_SP_PC_REG = FunctionAddress.first;
        if (SyncCPU)
        {
            RSPSystem.SyncSystem()->ExecuteOps(0x10000, 0x118);
        }
#if defined(_M_IX86) && defined(_MSC_VER)
        void * Block = FunctionAddress.second;
        _asm {
            pushad
            call Block
            popad
        }
#else
        g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
        if (SyncCPU)
        {
            RSPSystem.BasicSyncCheck();
            RSPSystem.SyncSystem()->ExecuteOps(2, (uint32_t)-1);
        }
        if (GPR_S8 == 0)
        {
            if (GPR_K1 <= 0)
            {
                m_RSPRegisterHandler->WriteReg(RSPRegister_STATUS, 0x4000);
                RSPSystem.m_Op.Special_BREAK();
                if (SyncCPU)
                {
                    *m_SP_PC_REG = 0x144;
                    RSPSystem.SyncSystem()->ExecuteOps(100, 0x144);
                    RSPSystem.BasicSyncCheck();
                }
                break;
            }
            uint32_t ReadLen = (GPR_K1 > 0x140) ? 0x140 : GPR_K1;
            GPR_S8 = ReadLen;
            if (*m_SP_SEMAPHORE_REG != 0 || *m_SP_DMA_FULL_REG != 0)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, 0x380);
            m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, GPR_GP);
            m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, ReadLen - 1);
            GPR_SP = 0x380;
            if (*m_SP_DMA_BUSY_REG != 0)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            *m_SP_SEMAPHORE_REG = 0;
            if (SyncCPU)
            {
                *m_SP_PC_REG = 0x0E4;
                RSPSystem.SyncSystem()->ExecuteOps(400, 0x0E4);
                RSPSystem.BasicSyncCheck();
            }
        }
        TaskCount += 1;
    }
}

void CHleTask::SetupTask(TASK_INFO & TaskInfo)
{
    if (TaskInfo.Flags != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    if (*m_SP_DMA_FULL_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, 0);
    m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, TaskInfo.UcodeData);
    m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, TaskInfo.UcodeDataSize);
    if (*m_SP_DMA_BUSY_REG != 0 || (*m_SP_STATUS_REG & 0x80) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, 0x1080);
    m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, TaskInfo.Ucode);
    m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, 0x0F7F);
    if (*m_SP_DMA_BUSY_REG != 0 || (*m_SP_STATUS_REG & 0x80) != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    *m_SP_SEMAPHORE_REG = 0;
    if (SyncCPU)
    {
        *m_SP_PC_REG = 0x80;
        RSPSystem.SyncSystem()->ExecuteOps(200, 0x080);
        RSPSystem.BasicSyncCheck();
    }
    SetupCommandList(TaskInfo);
}

bool CHleTask::ProcessHleTask(void)
{
    TASK_INFO & TaskInfo = *((TASK_INFO *)(m_DMEM + 0xFC0));
    extern bool AudioHle, GraphicsHle;

    if (((HLETaskType)TaskInfo.Type) == HLETaskType::Video && GraphicsHle && TaskInfo.DataPtr != 0)
    {
        if (ProcessDList == nullptr)
        {
            return false;
        }
        ProcessDList();
        *m_SP_STATUS_REG |= (0x0203);
        if ((*m_SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            *m_MI_INTR_REG |= MI_INTR_SP;
            CheckInterrupts();
        }
        *m_DPC_STATUS_REG &= ~0x0002;
        return true;
    }
    else if (TaskInfo.Type == 7)
    {
        RSPInfo.ShowCFB();
    }

    if (CRSPSettings::CPUMethod() == RSPCpuMethod::RecompilerTasks)
    {
        if (SyncCPU)
        {
            RSPSystem.SetupSyncCPU();
        }
        SetupTask(TaskInfo);
        uint32_t UcodeSize = TaskInfo.UcodeSize;
        if (UcodeSize < 0x4 || TaskInfo.UcodeSize > 0x0F80)
        {
            UcodeSize = 0x0F80;
        }
        m_UcodeCRC = crc32(0L, m_IMEM + 0x80, UcodeSize);
        if (m_UcodeCRC == 0x1a13a51a)
        {
            ExecuteTask_1a13a51a(TaskInfo);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return true;
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
