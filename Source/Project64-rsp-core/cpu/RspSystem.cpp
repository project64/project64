#include <Project64-rsp-core/RSPDebugger.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Recompiler/RspRecompilerCPU.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>

CRSPSystem RSPSystem;

CRSPSystem::CRSPSystem() :
    CHleTask(*this),
    m_SyncSystem(nullptr),
    m_BaseSystem(nullptr),
    m_Recompiler(*this),
    m_RSPRegisterHandler(nullptr),
    m_Op(*this),
    m_NextInstruction(RSPPIPELINE_NORMAL),
    m_JumpTo(0),
    m_HEADER(nullptr),
    m_RDRAM(nullptr),
    m_DMEM(nullptr),
    m_IMEM(nullptr),
    m_MI_INTR_REG(nullptr),
    m_SP_MEM_ADDR_REG(nullptr),
    m_SP_DRAM_ADDR_REG(nullptr),
    m_SP_RD_LEN_REG(nullptr),
    m_SP_WR_LEN_REG(nullptr),
    m_SP_STATUS_REG(nullptr),
    m_SP_DMA_FULL_REG(nullptr),
    m_SP_DMA_BUSY_REG(nullptr),
    m_SP_PC_REG(nullptr),
    m_SP_SEMAPHORE_REG(nullptr),
    m_DPC_START_REG(nullptr),
    m_DPC_END_REG(nullptr),
    m_DPC_CURRENT_REG(nullptr),
    m_DPC_STATUS_REG(nullptr),
    m_DPC_CLOCK_REG(nullptr),
    m_DPC_BUFBUSY_REG(nullptr),
    m_DPC_PIPEBUSY_REG(nullptr),
    m_DPC_TMEM_REG(nullptr),
    CheckInterrupts(NullCheckInterrupts),
    ProcessDList(NullProcessDList),
    ProcessRdpList(NullProcessRdpList),
    m_SyncReg(nullptr),
    m_RdramSize(0)
{
    m_OpCode.Value = 0;
}

CRSPSystem::~CRSPSystem()
{
    if (m_SyncSystem != nullptr && m_SyncSystem->m_BaseSystem != nullptr)
    {
        if (m_IMEM != nullptr)
        {
            delete[] m_IMEM;
            m_IMEM = nullptr;
        }
        if (m_DMEM != nullptr)
        {
            delete[] m_DMEM;
            m_DMEM = nullptr;
        }
    }
    if (m_SyncReg != nullptr)
    {
        delete m_SyncReg;
        m_SyncReg = nullptr;
    }
    if (m_SyncSystem)
    {
        delete m_SyncSystem;
        m_SyncSystem = nullptr;
    }
    if (m_RSPRegisterHandler != nullptr)
    {
        delete m_RSPRegisterHandler;
        m_RSPRegisterHandler = nullptr;
    }
}

void CRSPSystem::Reset(RSP_INFO & Info)
{
    m_Reg.Reset();

    m_HEADER = Info.HEADER;
    m_RDRAM = Info.RDRAM;

    if (m_BaseSystem == nullptr)
    {
        m_DMEM = Info.DMEM;
        m_IMEM = Info.IMEM;
        m_MI_INTR_REG = Info.MI_INTR_REG;
        m_SP_MEM_ADDR_REG = Info.SP_MEM_ADDR_REG;
        m_SP_DRAM_ADDR_REG = Info.SP_DRAM_ADDR_REG;
        m_SP_RD_LEN_REG = Info.SP_RD_LEN_REG;
        m_SP_WR_LEN_REG = Info.SP_WR_LEN_REG;
        m_SP_STATUS_REG = Info.SP_STATUS_REG;
        m_SP_DMA_FULL_REG = Info.SP_DMA_FULL_REG;
        m_SP_DMA_BUSY_REG = Info.SP_DMA_BUSY_REG;
        m_SP_PC_REG = Info.SP_PC_REG;
        m_SP_SEMAPHORE_REG = Info.SP_SEMAPHORE_REG;
        m_DPC_START_REG = Info.DPC_START_REG;
        m_DPC_END_REG = Info.DPC_END_REG;
        m_DPC_CURRENT_REG = Info.DPC_CURRENT_REG;
        m_DPC_STATUS_REG = Info.DPC_STATUS_REG;
        m_DPC_CLOCK_REG = Info.DPC_CLOCK_REG;
        m_DPC_BUFBUSY_REG = Info.DPC_BUFBUSY_REG;
        m_DPC_PIPEBUSY_REG = Info.DPC_PIPEBUSY_REG;
        m_DPC_TMEM_REG = Info.DPC_TMEM_REG;
        CheckInterrupts = Info.CheckInterrupts;
        ProcessDList = Info.ProcessDList;
        ProcessRdpList = Info.ProcessRdpList;
    }

    m_RdramSize = Set_AllocatedRdramSize != 0 ? GetSystemSetting(Set_AllocatedRdramSize) : 0;
    if (m_RdramSize == 0)
    {
        m_RdramSize = 0x00400000;
    }
    m_RSPRegisterHandler = new RSPRegisterHandlerPlugin(*this);

    if (m_SyncSystem != nullptr)
    {
        m_SyncSystem->Reset(Info);
    }
}

void CRSPSystem::RomClosed(void)
{
    if (m_RSPRegisterHandler != nullptr)
    {
        delete m_RSPRegisterHandler;
        m_RSPRegisterHandler = nullptr;
    }
}

void CRSPSystem::RunRecompiler(void)
{
    m_Recompiler.RunCPU();
}

void CRSPSystem::ExecuteOps(uint32_t Cycles, uint32_t TargetPC)
{
    RSP_Running = true;
    if (g_RSPDebugger != nullptr)
    {
        g_RSPDebugger->StartingCPU();
    }
    uint32_t & GprR0 = m_Reg.m_GPR[0].UW;
    uint32_t & ProgramCounter = *m_SP_PC_REG;
    while (RSP_Running && Cycles > 0)
    {
        if (g_RSPDebugger != nullptr)
        {
            g_RSPDebugger->BeforeExecuteOp();
        }
        if (TargetPC != -1 && (ProgramCounter & 0xFFC) == TargetPC)
        {
            break;
        }
        m_OpCode.Value = *(uint32_t *)(m_IMEM + (ProgramCounter & 0xFFC));
        (m_Op.*(m_Op.Jump_Opcode[m_OpCode.op]))();
        GprR0 = 0x00000000; // MIPS $zero hard-wired to 0
        if (Cycles != (uint32_t)-1)
        {
            Cycles -= 1;
        }

        switch (m_NextInstruction)
        {
        case RSPPIPELINE_NORMAL:
            ProgramCounter = (ProgramCounter + 4) & 0xFFC;
            break;
        case RSPPIPELINE_DELAY_SLOT:
            m_NextInstruction = RSPPIPELINE_JUMP;
            ProgramCounter = (ProgramCounter + 4) & 0xFFC;
            break;
        case RSPPIPELINE_JUMP:
            m_NextInstruction = RSPPIPELINE_NORMAL;
            ProgramCounter = m_JumpTo;
            break;
        case RSPPIPELINE_SINGLE_STEP:
            ProgramCounter = (ProgramCounter + 4) & 0xFFC;
            m_NextInstruction = RSPPIPELINE_SINGLE_STEP_DONE;
            break;
        case RSPPIPELINE_SINGLE_STEP_DONE:
            ProgramCounter = (ProgramCounter + 4) & 0xFFC;
            *m_SP_STATUS_REG |= SP_STATUS_HALT;
            RSP_Running = false;
            break;
        }
    }
}

void CRSPSystem::SetupSyncCPU()
{
    if (m_SyncSystem == nullptr)
    {
        m_SyncSystem = new CRSPSystem();
        m_SyncSystem->m_BaseSystem = this;
        m_SyncSystem->m_HEADER = m_HEADER;
        m_SyncSystem->m_RDRAM = m_RDRAM;
        m_SyncSystem->m_IMEM = new uint8_t[0x1000];
        m_SyncSystem->m_DMEM = new uint8_t[0x1000];
        m_SyncSystem->m_SyncReg = new uint32_t[18];
        m_SyncSystem->m_MI_INTR_REG = &m_SyncSystem->m_SyncReg[0];
        m_SyncSystem->m_SP_MEM_ADDR_REG = &m_SyncSystem->m_SyncReg[1];
        m_SyncSystem->m_SP_DRAM_ADDR_REG = &m_SyncSystem->m_SyncReg[2];
        m_SyncSystem->m_SP_RD_LEN_REG = &m_SyncSystem->m_SyncReg[3];
        m_SyncSystem->m_SP_WR_LEN_REG = &m_SyncSystem->m_SyncReg[4];
        m_SyncSystem->m_SP_STATUS_REG = &m_SyncSystem->m_SyncReg[5];
        m_SyncSystem->m_SP_DMA_FULL_REG = &m_SyncSystem->m_SyncReg[6];
        m_SyncSystem->m_SP_DMA_BUSY_REG = &m_SyncSystem->m_SyncReg[7];
        m_SyncSystem->m_SP_PC_REG = &m_SyncSystem->m_SyncReg[8];
        m_SyncSystem->m_SP_SEMAPHORE_REG = &m_SyncSystem->m_SyncReg[9];
        m_SyncSystem->m_DPC_START_REG = &m_SyncSystem->m_SyncReg[10];
        m_SyncSystem->m_DPC_END_REG = &m_SyncSystem->m_SyncReg[11];
        m_SyncSystem->m_DPC_CURRENT_REG = &m_SyncSystem->m_SyncReg[12];
        m_SyncSystem->m_DPC_STATUS_REG = &m_SyncSystem->m_SyncReg[13];
        m_SyncSystem->m_DPC_CLOCK_REG = &m_SyncSystem->m_SyncReg[14];
        m_SyncSystem->m_DPC_BUFBUSY_REG = &m_SyncSystem->m_SyncReg[15];
        m_SyncSystem->m_DPC_PIPEBUSY_REG = &m_SyncSystem->m_SyncReg[16];
        m_SyncSystem->m_DPC_TMEM_REG = &m_SyncSystem->m_SyncReg[17];

        m_SyncSystem->m_Reg.Reset();
        m_SyncSystem->m_RdramSize = m_RdramSize;
        m_SyncSystem->m_RSPRegisterHandler = new RSPRegisterHandlerPlugin(*m_SyncSystem);
    }
    if (m_IMEM != nullptr)
    {
        memcpy(m_SyncSystem->m_IMEM, m_IMEM, 0x1000);
    }
    if (m_DMEM != nullptr)
    {
        memcpy(m_SyncSystem->m_DMEM, m_DMEM, 0x1000);
    }
    *m_SyncSystem->m_MI_INTR_REG = *m_MI_INTR_REG;
    *m_SyncSystem->m_SP_MEM_ADDR_REG = *m_SP_MEM_ADDR_REG;
    *m_SyncSystem->m_SP_DRAM_ADDR_REG = *m_SP_DRAM_ADDR_REG;
    *m_SyncSystem->m_SP_RD_LEN_REG = *m_SP_RD_LEN_REG;
    *m_SyncSystem->m_SP_WR_LEN_REG = *m_SP_WR_LEN_REG;
    *m_SyncSystem->m_SP_STATUS_REG = *m_SP_STATUS_REG;
    *m_SyncSystem->m_SP_DMA_FULL_REG = *m_SP_DMA_FULL_REG;
    *m_SyncSystem->m_SP_DMA_BUSY_REG = *m_SP_DMA_BUSY_REG;
    *m_SyncSystem->m_SP_PC_REG = *m_SP_PC_REG;
    *m_SyncSystem->m_SP_SEMAPHORE_REG = *m_SP_SEMAPHORE_REG;
    *m_SyncSystem->m_DPC_START_REG = *m_DPC_START_REG;
    *m_SyncSystem->m_DPC_END_REG = *m_DPC_END_REG;
    *m_SyncSystem->m_DPC_CURRENT_REG = *m_DPC_CURRENT_REG;
    *m_SyncSystem->m_DPC_STATUS_REG = *m_DPC_STATUS_REG;
    *m_SyncSystem->m_DPC_CLOCK_REG = *m_DPC_CLOCK_REG;
    *m_SyncSystem->m_DPC_BUFBUSY_REG = *m_DPC_BUFBUSY_REG;
    *m_SyncSystem->m_DPC_PIPEBUSY_REG = *m_DPC_PIPEBUSY_REG;
    *m_SyncSystem->m_DPC_TMEM_REG = *m_DPC_TMEM_REG;
}

bool CRSPSystem::IsSyncSystem(void)
{
    return m_BaseSystem != nullptr;
}

CRSPSystem * CRSPSystem::SyncSystem(void)
{
    if (m_SyncSystem == nullptr)
    {
        SetupSyncCPU();
    }
    return m_SyncSystem;
}

void CRSPSystem::BasicSyncCheck(void)
{
    bool SyncFailed = false;
    if (memcmp(m_IMEM, m_SyncSystem->m_IMEM, 0x1000) != 0)
    {
        SyncFailed = true;
    }
    if (memcmp(m_DMEM, m_SyncSystem->m_DMEM, 0x1000) != 0)
    {
        SyncFailed = true;
        for (uint32_t i = 0, n = 0x1000; i < n; i++)
        {
            if (m_DMEM[i] != m_SyncSystem->m_DMEM[i])
            {
                SyncFailed = true;
                break;
            }
        }
    }
    if (*m_MI_INTR_REG != *m_SyncSystem->m_MI_INTR_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_MEM_ADDR_REG != *m_SyncSystem->m_SP_MEM_ADDR_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_DRAM_ADDR_REG != *m_SyncSystem->m_SP_DRAM_ADDR_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_RD_LEN_REG != *m_SyncSystem->m_SP_RD_LEN_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_WR_LEN_REG != *m_SyncSystem->m_SP_WR_LEN_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_STATUS_REG != *m_SyncSystem->m_SP_STATUS_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_DMA_FULL_REG != *m_SyncSystem->m_SP_DMA_FULL_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_DMA_BUSY_REG != *m_SyncSystem->m_SP_DMA_BUSY_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_PC_REG != *m_SyncSystem->m_SP_PC_REG)
    {
        SyncFailed = true;
    }
    if (*m_SP_SEMAPHORE_REG != *m_SyncSystem->m_SP_SEMAPHORE_REG)
    {
        SyncFailed = true;
    }
    if (SyncFailed)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void * CRSPSystem::operator new(size_t size)
{
    return _aligned_malloc(size, 16);
}

void CRSPSystem::operator delete(void * ptr)
{
    _aligned_free(ptr);
}

void CRSPSystem::NullProcessDList(void)
{
}

void CRSPSystem::NullProcessRdpList(void)
{
}

void CRSPSystem::NullCheckInterrupts(void)
{
}
