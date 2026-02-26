#include <Project64-rsp-core/Hle/HleTask.h>
#include <Project64-rsp-core/Recompiler/RspCodeBlock.h>
#include <Project64-rsp-core/Recompiler/RspProfiling.h>
#include <Project64-rsp-core/cpu/RSPRegisterHandlerPlugin.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>
#include <zlib/zlib.h>

CHleTask::CHleTask(CRSPSystem & System) :
    CGPRRegisters(System.m_Reg.m_GPR),
    m_hle(System),
    m_TaskEnter(nullptr),
    m_TaskLeave(nullptr),
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

HLETaskBooter CHleTask::IsHleTask(void)
{
    if ((*m_SP_PC_REG) != 0)
    {
        return HLETaskBooter::unknown;
    }
    uint32_t ImemCrc = crc32(0L, m_IMEM, 0xCC);
    if (ImemCrc == 0xf40b72dc || ImemCrc == 0xc4fa99b9) // The Legend of Zelda - Ocarina of Time
    {
        return HLETaskBooter::unknown;
    }
    if (ImemCrc == 0xcab15710 || // Super Mario
        ImemCrc == 0x6f849879)   // pokemon puzzle league
    {
        return HLETaskBooter::Boot_CAB15710;
    }
    if (ImemCrc == 0xb4c62bfc) // The Legend of Zelda - Ocarina of Time
    {
        return HLETaskBooter::Boot_B4C62BFC;
    }
    return HLETaskBooter::unknown;
}

#if defined(__amd64__) || defined(_M_X64)
void CHleTask::SetupCommandList(const TASK_INFO & TaskInfo, HLETaskBooter bootType)
{
    uint32_t JumpTableLength = 0x7E, JumpTablePos = 0x10;
    if ((HLETaskType)(TaskInfo.Type) == HLETaskType::Audio)
    {
        if (*((uint32_t *)&m_DMEM[0]) == 0x00000001 && *((uint32_t *)&m_DMEM[0x30]) == 0xf0000f00)
        {
            JumpTableLength = 0x10;
        }
        else if (*((uint32_t *)&m_DMEM[0]) == 0x00000001 && *((uint32_t *)&m_DMEM[0x30]) != 0xf0000f00)
        {
            if (*((uint32_t *)&m_DMEM[0x10]) == 0x1f681230) // Zelda Ocarina of Time / Zelda Majora's Mask (J, J Rev A)
            {
                JumpTableLength = 0x18;
            }
        }
    }

    uint32_t EndBlockAddress = 0x1118;
    switch (bootType)
    {
    case HLETaskBooter::Boot_B4C62BFC:
        EndBlockAddress = 0x108C;
        break;
    case HLETaskBooter::Boot_CAB15710:
        EndBlockAddress = 0x1118;
        break;
    }

    uint32_t JumpTableCRC = crc32(0L, m_IMEM + JumpTablePos, JumpTableLength << 1);
    TaskFunctionMap::iterator itr = m_FunctionMap.find(JumpTableCRC);
    if (itr != m_FunctionMap.end())
    {
        m_TaskFunctions = &itr->second;
        return;
    }

    if (Profiling)
    {
        StartTimer((uint32_t)Timer_Compiling);
    }
    if (m_FunctionMap.size() > 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_TaskFunctions = nullptr;

    if (m_TaskEnter == nullptr)
    {
        m_TaskEnter = m_Recompiler.CompileTaskEnter();
        m_TaskLeave = m_Recompiler.CompileTaskLeave();
        if (m_TaskEnter == nullptr || m_TaskLeave == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
    }

    TaskFunctions JumpFunctions;
    RspCodeBlocks Functions;
    for (uint32_t i = 0, n = JumpTableLength; i < n; i++)
    {
        uint16_t FuncAddress = *((uint16_t *)(m_DMEM + (((i << 1) + JumpTablePos) ^ 2)));
        if (FuncAddress != EndBlockAddress)
        {
            void * FuncPtr = m_Recompiler.CompileHLETask(FuncAddress, Functions, EndBlockAddress);
            JumpFunctions.emplace_back(TaskFunctionAddress(FuncAddress, FuncPtr));
        }
        else
        {
            JumpFunctions.emplace_back(TaskFunctionAddress(FuncAddress, nullptr));
        }
    }
    m_FunctionMap[JumpTableCRC] = JumpFunctions;
    itr = m_FunctionMap.find(JumpTableCRC);
    if (itr == m_FunctionMap.end())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_TaskFunctions = &itr->second;
    if (Profiling)
    {
        StopTimer();
    }
}

void CHleTask::ExecuteTask_1a13a51a(const TASK_INFO & TaskInfo)
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
        typedef void (*FuncPtr)();
        FuncPtr func = (FuncPtr)FunctionAddress.second;
        if (func == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        func();
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
    }
}

void CHleTask::ExecuteTask_c2193700(const TASK_INFO & TaskInfo)
{
    if (SyncCPU)
    {
        RSPSystem.BasicSyncCheck();
    }
    typedef void (*FuncPtr)();
    ((FuncPtr)m_TaskEnter)();

    do
    {
        GPR_K0 = *((uint32_t *)&m_DMEM[GPR_SP & 0xFFF]);
        GPR_T9 = *((uint32_t *)&m_DMEM[(GPR_SP + 4) & 0xFFF]);
        GPR_GP += 8;
        GPR_K1 -= 8;
        GPR_SP += 8;
        GPR_S8 -= 8;
        uint32_t funcIndex = ((GPR_K0 >> 0x17) & 0x00FE) >> 1;
        if (funcIndex >= m_TaskFunctions->size())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        TaskFunctionAddress FunctionAddress = (*m_TaskFunctions)[funcIndex];
        *m_SP_PC_REG = FunctionAddress.first;

        FuncPtr func = (FuncPtr)FunctionAddress.second;
        if (func == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        func();
        if (SyncCPU)
        {
            RSPSystem.SyncSystem()->ExecuteOps(0x10000, 0x08C);
            RSPSystem.BasicSyncCheck();
            RSPSystem.SyncSystem()->ExecuteOps(2, (uint32_t)-1);
        }
        if (GPR_S8 <= 0 && GPR_K1 > 0)
        {
            GPR_V0 = GPR_GP;
            GPR_V1 = GPR_K1;
            GPR_A0 = GPR_V1 - 0x40;
            GPR_AT = 0x2f0;
            if (GPR_K1 > 0x040)
            {
                GPR_V1 = 0x40;
            }
            GPR_S8 = GPR_V1;
            GPR_V1 -= 1;
            *m_SP_SEMAPHORE_REG = 0;

            if (*m_SP_SEMAPHORE_REG != 0 || *m_SP_DMA_FULL_REG != 0)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, GPR_AT);
            m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, GPR_V0);
            m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, GPR_V1);
            if (*m_SP_DMA_BUSY_REG != 0)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            *m_SP_SEMAPHORE_REG = 0;
            GPR_SP = 0x02F0;
            if (SyncCPU)
            {
                *m_SP_PC_REG = 0x10A4;
                RSPSystem.SyncSystem()->ExecuteOps(0x10000, 0x0A4);
                RSPSystem.BasicSyncCheck();
            }
        }
    } while (GPR_S8 > 0);
    m_RSPRegisterHandler->WriteReg(RSPRegister_STATUS, 0x4000);
    *m_SP_STATUS_REG |= (SP_STATUS_HALT | SP_STATUS_BROKE);
    if ((*m_SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
    {
        *m_MI_INTR_REG |= MI_INTR_SP;
        CheckInterrupts();
    }
    ((FuncPtr)m_TaskLeave)();
}

void CHleTask::SetupTask_B4C62BFC(const TASK_INFO & TaskInfo)
{
    *m_SP_SEMAPHORE_REG = 0;
    if (*m_SP_SEMAPHORE_REG != 0 || *m_SP_DMA_FULL_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, 0);
    m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, TaskInfo.UcodeData);
    m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, TaskInfo.UcodeDataSize);
    if (*m_SP_DMA_BUSY_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    *m_SP_SEMAPHORE_REG = 0;

    GPR_T8 = 0x02E0;
    GPR_S7 = 0x0FB0;
    GPR_GP = TaskInfo.DataPtr;
    GPR_K1 = TaskInfo.DataSize;
    GPR_S8 = 0x40;
    if (*m_DPC_STATUS_REG != 0 || *m_SP_SEMAPHORE_REG != 0 || *m_SP_DMA_FULL_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    uint32_t DataSize = TaskInfo.DataSize;
    if (DataSize > 0x40)
    {
        DataSize = 0x40;
    }
    m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, 0x2F0);
    m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, TaskInfo.DataPtr);
    m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, DataSize - 1);
    if (*m_SP_DMA_BUSY_REG != 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    *m_SP_SEMAPHORE_REG = 0;
    GPR_SP = 0x2F0;
    GPR_AT = 0x2F0;
    if (SyncCPU)
    {
        *m_SP_PC_REG = 0x58;
        RSPSystem.SyncSystem()->ExecuteOps(200, *m_SP_PC_REG);
        RSPSystem.BasicSyncCheck();
    }

    SetupCommandList(TaskInfo, HLETaskBooter::Boot_B4C62BFC);
}

void CHleTask::SetupTask_CAB15710(const TASK_INFO & TaskInfo)
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
    SetupCommandList(TaskInfo, HLETaskBooter::Boot_CAB15710);
}
#endif

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

    if (((HLETaskType)TaskInfo.Type) == HLETaskType::Audio && m_hle.try_fast_audio_dispatching())
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

#if defined(__amd64__) || defined(_M_X64)
bool CHleTask::HleTaskRecompiler(HLETaskBooter booter)
{
    const TASK_INFO & TaskInfo = *((TASK_INFO *)(m_DMEM + 0xFC0));

    if (SyncCPU)
    {
        RSPSystem.SetupSyncCPU();
    }

    switch (booter)
    {
    case HLETaskBooter::Boot_CAB15710:
        SetupTask_CAB15710(TaskInfo);
        break;
    case HLETaskBooter::Boot_B4C62BFC:
        SetupTask_B4C62BFC(TaskInfo);
        break;
    default:
        return false;
    }

    uint32_t UcodeSize = TaskInfo.UcodeSize;
    if (UcodeSize < 0x4 || TaskInfo.UcodeSize > 0x0F80)
    {
        UcodeSize = 0x0F80;
    }
    m_UcodeCRC = crc32(0L, m_IMEM + 0x80, UcodeSize);

    switch (m_UcodeCRC)
    {
    case 0x1a13a51a: ExecuteTask_1a13a51a(TaskInfo); break;
    case 0xc2193700: ExecuteTask_c2193700(TaskInfo); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return true;
}
#endif