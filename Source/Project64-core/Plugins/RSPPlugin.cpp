#include "stdafx.h"

#include "GFXPlugin.h"
#include "RSPPlugin.h"
#include <Common/Util.h>
#include <Project64-core/Debugger.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/N64Disk.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/Plugins/AudioPlugin.h>

CRSP_Plugin::CRSP_Plugin(void) :
    m_DoRspCycles(nullptr),
    m_EnableDebugging(nullptr),
    m_GetDebugInfo(nullptr),
    m_InitiateDebugger(nullptr),
    m_Thread(stRspThread),
    m_CycleCount(0),
    m_Plugins(nullptr),
    m_System(nullptr),
    m_AlistCount(0),
    m_DlistCount(0),
    m_UnknownCount(0),
    m_RomOpened(false)
{
    memset(&m_RSPDebug, 0, sizeof(m_RSPDebug));
}

CRSP_Plugin::~CRSP_Plugin()
{
    Close(nullptr);
    UnloadPlugin();
}

bool CRSP_Plugin::LoadFunctions(void)
{
    void(CALL * InitiateRSP)(void) = (void(CALL *)(void))DynamicLibraryGetProc(m_LibHandle, "InitiateRSP");
    m_DoRspCycles = (uint32_t(CALL *)(uint32_t))DynamicLibraryGetProc(m_LibHandle, "DoRspCycles");
    m_GetDebugInfo = (void(CALL *)(RSPDEBUG_INFO * GFXDebugInfo)) DynamicLibraryGetProc(m_LibHandle, "GetRspDebugInfo");
    m_InitiateDebugger = (void(CALL *)(DEBUG_INFO DebugInfo))DynamicLibraryGetProc(m_LibHandle, "InitiateRSPDebugger");
    m_EnableDebugging = (void(CALL *)(int32_t Enable))DynamicLibraryGetProc(m_LibHandle, "EnableDebugging");

    if (m_DoRspCycles == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (InitiateRSP == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (RomClosed == nullptr)
    {
        UnloadPlugin();
        return false;
    }
    if (CloseDLL == nullptr)
    {
        UnloadPlugin();
        return false;
    }

    if (m_PluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == nullptr)
        {
            UnloadPlugin();
            return false;
        }
    }

    if (m_GetDebugInfo != nullptr)
    {
        m_GetDebugInfo(&m_RSPDebug);
    }
    return true;
}

void CRSP_Plugin::RomOpened(RenderWindow * Render)
{
    CPlugin::RomOpened(Render);
    m_AlistCount = 0;
    m_DlistCount = 0;
    m_UnknownCount = 0;
    m_RomOpened = true;
    m_Thread.Start(this);
}

void CRSP_Plugin::RomClose(RenderWindow * Render)
{
    m_RomOpened = false;
    m_RunEvent.Trigger();
    for (uint32_t i = 0; i < 300; i++)
    {
        if (!m_Thread.isRunning())
        {
            break;
        }
        pjutil::Sleep(10);
    }
    if (m_Thread.isRunning())
    {
        m_Thread.Terminate();
    }
    CPlugin::RomClose(Render);
    m_Plugins = nullptr;
    m_System = nullptr;
}

bool CRSP_Plugin::Initiate(CPlugins * Plugins, CN64System * System)
{
    WriteTrace(TraceRSPPlugin, TraceDebug, "Starting");
    m_Plugins = Plugins;
    m_System = System;
    if (m_PluginInfo.Version == 1 || m_PluginInfo.Version == 0x100)
    {
        WriteTrace(TraceRSPPlugin, TraceDebug, "Invalid version: %X", m_PluginInfo.Version);
        WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: false)");
        return false;
    }

    if (m_PluginInfo.Version >= 0x103)
    {
        typedef struct
        {
            void * hInst;
            int MemoryBswaped; // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary
            uint8_t * HEADER;  // This is the ROM header (first 40h bytes of the ROM)
            // This will be in the same memory format as the rest of the memory
            uint8_t * RDRAM;
            uint8_t * DMEM;
            uint8_t * IMEM;

            uint32_t * MI__INTR_REG;

            uint32_t * SP__MEM_ADDR_REG;
            uint32_t * SP__DRAM_ADDR_REG;
            uint32_t * SP__RD_LEN_REG;
            uint32_t * SP__WR_LEN_REG;
            uint32_t * SP__STATUS_REG;
            uint32_t * SP__DMA_FULL_REG;
            uint32_t * SP__DMA_BUSY_REG;
            uint32_t * SP__PC_REG;
            uint32_t * SP__SEMAPHORE_REG;

            uint32_t * DPC__START_REG;
            uint32_t * DPC__END_REG;
            uint32_t * DPC__CURRENT_REG;
            uint32_t * DPC__STATUS_REG;
            uint32_t * DPC__CLOCK_REG;
            uint32_t * DPC__BUFBUSY_REG;
            uint32_t * DPC__PIPEBUSY_REG;
            uint32_t * DPC__TMEM_REG;

            void(CALL * CheckInterrupts)(void);
            void(CALL * ProcessDlist)(void);
            void(CALL * ProcessAlist)(void);
            void(CALL * ProcessRdpList)(void);
            void(CALL * ShowCFB)(void);
        } RSP_INFO_1_3;

        RSP_INFO_1_3 Info = {0};

#ifdef _WIN32
        Info.hInst = (Plugins != nullptr && Plugins->MainWindow() != nullptr) ? Plugins->MainWindow()->GetModuleInstance() : nullptr;
#else
        Info.hInst = nullptr;
#endif
        Info.CheckInterrupts = DummyCheckInterrupts;
        Info.MemoryBswaped = (System == nullptr); // Only true when the system's not yet loaded

        // Get function from DLL
        void(CALL * InitiateRSP)(RSP_INFO_1_3 RSP_Info, uint32_t * Cycles);
        LoadFunction(InitiateRSP);
        if (InitiateRSP == nullptr)
        {
            WriteTrace(TraceRSPPlugin, TraceDebug, "Failed to find InitiateRSP");
            WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: false)");
            return false;
        }

        // We are initializing the plugin before any ROM is loaded so we do not have any correct
        // parameters here, just needed to we can config the DLL
        if (System == nullptr)
        {
            static uint8_t Buffer[100];
            static uint32_t Value = 0;

            Info.ProcessDlist = DummyCheckInterrupts;
            Info.ProcessRdpList = DummyCheckInterrupts;
            Info.ShowCFB = DummyCheckInterrupts;
            Info.ProcessAlist = DummyCheckInterrupts;

            Info.HEADER = Buffer;
            Info.RDRAM = Buffer;
            Info.DMEM = Buffer;
            Info.IMEM = Buffer;

            Info.MI__INTR_REG = &Value;

            Info.SP__MEM_ADDR_REG = &Value;
            Info.SP__DRAM_ADDR_REG = &Value;
            Info.SP__RD_LEN_REG = &Value;
            Info.SP__WR_LEN_REG = &Value;
            Info.SP__STATUS_REG = &Value;
            Info.SP__DMA_FULL_REG = &Value;
            Info.SP__DMA_BUSY_REG = &Value;
            Info.SP__PC_REG = &Value;
            Info.SP__SEMAPHORE_REG = &Value;

            Info.DPC__START_REG = &Value;
            Info.DPC__END_REG = &Value;
            Info.DPC__CURRENT_REG = &Value;
            Info.DPC__STATUS_REG = &Value;
            Info.DPC__CLOCK_REG = &Value;
            Info.DPC__BUFBUSY_REG = &Value;
            Info.DPC__PIPEBUSY_REG = &Value;
            Info.DPC__TMEM_REG = &Value;
        }
        else
        {
            CMipsMemoryVM & MMU = System->m_MMU_VM;
            CRegisters & Reg = System->m_Reg;

            Info.ProcessDlist = Plugins->Gfx()->ProcessDList;
            Info.ProcessRdpList = Plugins->Gfx()->ProcessRDPList;
            Info.ShowCFB = Plugins->Gfx()->ShowCFB;
            Info.ProcessAlist = Plugins->Audio()->ProcessAList;

            Info.HEADER = g_Rom->IsLoadedRomDDIPL() && g_Disk != nullptr ? g_Disk->GetDiskHeader() : g_Rom->GetRomAddress();
            Info.RDRAM = MMU.Rdram();
            Info.DMEM = MMU.Dmem();
            Info.IMEM = MMU.Imem();

            Info.MI__INTR_REG = &Reg.m_RspIntrReg;

            Info.SP__MEM_ADDR_REG = &Reg.SP_MEM_ADDR_REG;
            Info.SP__DRAM_ADDR_REG = &Reg.SP_DRAM_ADDR_REG;
            Info.SP__RD_LEN_REG = &Reg.SP_RD_LEN_REG;
            Info.SP__WR_LEN_REG = &Reg.SP_WR_LEN_REG;
            Info.SP__STATUS_REG = &Reg.SP_STATUS_REG;
            Info.SP__DMA_FULL_REG = &Reg.SP_DMA_FULL_REG;
            Info.SP__DMA_BUSY_REG = &Reg.SP_DMA_BUSY_REG;
            Info.SP__PC_REG = &Reg.SP_PC_REG;
            Info.SP__SEMAPHORE_REG = &Reg.SP_SEMAPHORE_REG;

            Info.DPC__START_REG = &Reg.DPC_START_REG;
            Info.DPC__END_REG = &Reg.DPC_END_REG;
            Info.DPC__CURRENT_REG = &Reg.DPC_CURRENT_REG;
            Info.DPC__STATUS_REG = &Reg.DPC_STATUS_REG;
            Info.DPC__CLOCK_REG = &Reg.DPC_CLOCK_REG;
            Info.DPC__BUFBUSY_REG = &Reg.DPC_BUFBUSY_REG;
            Info.DPC__PIPEBUSY_REG = &Reg.DPC_PIPEBUSY_REG;
            Info.DPC__TMEM_REG = &Reg.DPC_TMEM_REG;
        }

        InitiateRSP(Info, &m_CycleCount);
    }
    else
    {
        typedef struct
        {
            void * hInst;
            int MemoryBswaped; // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary
            uint8_t * RDRAM;
            uint8_t * DMEM;
            uint8_t * IMEM;

            uint32_t * MI__INTR_REG;

            uint32_t * SP__MEM_ADDR_REG;
            uint32_t * SP__DRAM_ADDR_REG;
            uint32_t * SP__RD_LEN_REG;
            uint32_t * SP__WR_LEN_REG;
            uint32_t * SP__STATUS_REG;
            uint32_t * SP__DMA_FULL_REG;
            uint32_t * SP__DMA_BUSY_REG;
            uint32_t * SP__PC_REG;
            uint32_t * SP__SEMAPHORE_REG;

            uint32_t * DPC__START_REG;
            uint32_t * DPC__END_REG;
            uint32_t * DPC__CURRENT_REG;
            uint32_t * DPC__STATUS_REG;
            uint32_t * DPC__CLOCK_REG;
            uint32_t * DPC__BUFBUSY_REG;
            uint32_t * DPC__PIPEBUSY_REG;
            uint32_t * DPC__TMEM_REG;

            void(CALL * CheckInterrupts)(void);
            void(CALL * ProcessDlist)(void);
            void(CALL * ProcessAlist)(void);
            void(CALL * ProcessRdpList)(void);
            void(CALL * ShowCFB)(void);
        } RSP_INFO_1_1;

        RSP_INFO_1_1 Info = {0};

#ifdef _WIN32
        Info.hInst = (Plugins != nullptr && Plugins->MainWindow() != nullptr) ? Plugins->MainWindow()->GetModuleInstance() : nullptr;
#else
        Info.hInst = nullptr;
#endif
        Info.CheckInterrupts = DummyCheckInterrupts;
        Info.MemoryBswaped = (System == nullptr); // Only true when the system's not yet loaded

        // Get function from DLL
        void(CALL * InitiateRSP)(RSP_INFO_1_1 RSP_Info, uint32_t * Cycles);
        LoadFunction(InitiateRSP);
        if (InitiateRSP == nullptr)
        {
            WriteTrace(TraceRSPPlugin, TraceDebug, "Failed to find InitiateRSP");
            WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: false)");
            return false;
        }

        // We are initializing the plugin before any ROM is loaded so we do not have any correct
        // parameters here, just needed to we can config the DLL
        if (System == nullptr)
        {
            static uint8_t Buffer[100];
            static uint32_t Value = 0;

            Info.ProcessDlist = DummyCheckInterrupts;
            Info.ProcessRdpList = DummyCheckInterrupts;
            Info.ShowCFB = DummyCheckInterrupts;
            Info.ProcessAlist = DummyCheckInterrupts;

            Info.RDRAM = Buffer;
            Info.DMEM = Buffer;
            Info.IMEM = Buffer;

            Info.MI__INTR_REG = &Value;

            Info.SP__MEM_ADDR_REG = &Value;
            Info.SP__DRAM_ADDR_REG = &Value;
            Info.SP__RD_LEN_REG = &Value;
            Info.SP__WR_LEN_REG = &Value;
            Info.SP__STATUS_REG = &Value;
            Info.SP__DMA_FULL_REG = &Value;
            Info.SP__DMA_BUSY_REG = &Value;
            Info.SP__PC_REG = &Value;
            Info.SP__SEMAPHORE_REG = &Value;

            Info.DPC__START_REG = &Value;
            Info.DPC__END_REG = &Value;
            Info.DPC__CURRENT_REG = &Value;
            Info.DPC__STATUS_REG = &Value;
            Info.DPC__CLOCK_REG = &Value;
            Info.DPC__BUFBUSY_REG = &Value;
            Info.DPC__PIPEBUSY_REG = &Value;
            Info.DPC__TMEM_REG = &Value;
        }
        // Send initialization information to the DLL
        else
        {
            Info.ProcessDlist = Plugins->Gfx()->ProcessDList;
            Info.ProcessRdpList = Plugins->Gfx()->ProcessRDPList;
            Info.ShowCFB = Plugins->Gfx()->ShowCFB;
            Info.ProcessAlist = Plugins->Audio()->ProcessAList;

            CMipsMemoryVM & MMU = System->m_MMU_VM;
            CRegisters & Reg = System->m_Reg;

            Info.RDRAM = MMU.Rdram();
            Info.DMEM = MMU.Dmem();
            Info.IMEM = MMU.Imem();

            Info.MI__INTR_REG = &Reg.m_RspIntrReg;

            Info.SP__MEM_ADDR_REG = &Reg.SP_MEM_ADDR_REG;
            Info.SP__DRAM_ADDR_REG = &Reg.SP_DRAM_ADDR_REG;
            Info.SP__RD_LEN_REG = &Reg.SP_RD_LEN_REG;
            Info.SP__WR_LEN_REG = &Reg.SP_WR_LEN_REG;
            Info.SP__STATUS_REG = &Reg.SP_STATUS_REG;
            Info.SP__DMA_FULL_REG = &Reg.SP_DMA_FULL_REG;
            Info.SP__DMA_BUSY_REG = &Reg.SP_DMA_BUSY_REG;
            Info.SP__PC_REG = &Reg.SP_PC_REG;
            Info.SP__SEMAPHORE_REG = &Reg.SP_SEMAPHORE_REG;

            Info.DPC__START_REG = &Reg.DPC_START_REG;
            Info.DPC__END_REG = &Reg.DPC_END_REG;
            Info.DPC__CURRENT_REG = &Reg.DPC_CURRENT_REG;
            Info.DPC__STATUS_REG = &Reg.DPC_STATUS_REG;
            Info.DPC__CLOCK_REG = &Reg.DPC_CLOCK_REG;
            Info.DPC__BUFBUSY_REG = &Reg.DPC_BUFBUSY_REG;
            Info.DPC__PIPEBUSY_REG = &Reg.DPC_PIPEBUSY_REG;
            Info.DPC__TMEM_REG = &Reg.DPC_TMEM_REG;
        }

        InitiateRSP(Info, &m_CycleCount);
    }

    m_Initialized = true;

    WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: %s)", m_Initialized ? "true" : "false");
    return m_Initialized;
}

void CRSP_Plugin::EnableDebugging(int32_t Enable)
{
    if (m_EnableDebugging != nullptr)
    {
        WriteTrace(TraceRSPPlugin, TraceInfo, "EnableDebugging starting");
        m_EnableDebugging(Enable);
        WriteTrace(TraceRSPPlugin, TraceInfo, "EnableDebugging done");
    }
}

void CRSP_Plugin::RunRSP()
{
    CRegisters & Reg = m_System->m_Reg;
    WriteTrace(TraceRSP, TraceDebug, "Start (SP Status %X)", Reg.SP_STATUS_REG);
    if ((Reg.SP_STATUS_REG & SP_STATUS_HALT) != 0)
    {
        WriteTrace(TraceRSP, TraceDebug, "Done (SP Status %X)", Reg.SP_STATUS_REG);
        return;
    }
    CProfiling & CPU_Usage = m_System->m_CPU_Usage;
    PROFILE_TIMERS CPU_UsageAddr = CPU_Usage.StopTimer();
    CMipsMemoryVM & Memory = m_System->m_MMU_VM;
    HighResTimeStamp StartTime;
    uint32_t TaskType = 0;

    Memory.MemoryValue32(0xA4000FC0, TaskType);
    if (TaskType == 1 && UseHleGfx() && (Reg.DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0)
    {
        WriteTrace(TraceRSP, TraceDebug, "Dlist that is frozen");
        WriteTrace(TraceRSP, TraceDebug, "Done (SP Status %X)", Reg.SP_STATUS_REG);
        return;
    }

    if (g_Debugger != NULL && HaveDebugger())
    {
        g_Debugger->RSPReceivedTask();
    }

    switch (TaskType)
    {
    case 1:
        WriteTrace(TraceRSP, TraceDebug, "*** Display list ***");
        m_DlistCount += 1;
        m_System->m_FPS.UpdateDlCounter();
        break;
    case 2:
        WriteTrace(TraceRSP, TraceDebug, "*** Audio list ***");
        m_AlistCount += 1;
        break;
    default:
        WriteTrace(TraceRSP, TraceDebug, "*** Unknown list ***");
        m_UnknownCount += 1;
        break;
    }

    if (bShowDListAListCount())
    {
        g_Notify->DisplayMessage(0, stdstr_f("Dlist: %d   Alist: %d   Unknown: %d", m_DlistCount, m_AlistCount, m_UnknownCount).c_str());
    }
    if (bRecordExecutionTimes() || bShowCPUPer())
    {
        StartTime.SetToNow();
    }

    uint32_t DataPtr = 0;
    Memory.MemoryValue32(0xA4000FC0, DataPtr);
    bool ExecuteCycles = true;
    if (TaskType == 1 && UseHleGfx() && DataPtr != 0)
    {
        Memory.MemoryValue32(0xA4000FF0, TaskType);
        if (m_Plugins->Gfx()->ProcessDList != nullptr)
        {
            m_Plugins->Gfx()->ProcessDList();
        }
        Reg.SP_STATUS_REG |= (0x0203);
        if ((Reg.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            Reg.MI_INTR_REG |= MI_INTR_SP;
        }

        Reg.DPC_STATUS_REG &= ~0x0002;
        if (bDelayDP() && ((Reg.m_GfxIntrReg & MI_INTR_DP) != 0))
        {
            g_SystemTimer->SetTimer(CSystemTimer::RSPTimerDlist, 0x1000, false);
            Reg.m_GfxIntrReg &= ~MI_INTR_DP;
        }
        ExecuteCycles = false;
    }
    else if (TaskType == 2 && UseHleAudio())
    {
        if (m_Plugins->Audio()->ProcessAList != nullptr)
        {
            m_Plugins->Audio()->ProcessAList();
        }
        Reg.SP_STATUS_REG |= (0x0203);
        if ((Reg.SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
        {
            Reg.MI_INTR_REG |= MI_INTR_SP;
        }
        ExecuteCycles = false;
    }
    else if (TaskType == 7 && UseHleGfx() && m_Plugins->Gfx()->ShowCFB != nullptr)
    {
        m_Plugins->Gfx()->ShowCFB();
    }

    if (ExecuteCycles)
    {
        if (RspMultiThreaded())
        {
            m_RunEvent.Trigger();
        }
        else
        {
            WriteTrace(TraceRSP, TraceDebug, "Do cycles - starting");
            m_DoRspCycles(100);
            WriteTrace(TraceRSP, TraceDebug, "Do cycles - done");
        }
    }
    if (bRecordExecutionTimes() || bShowCPUPer())
    {
        HighResTimeStamp EndTime;
        EndTime.SetToNow();
        uint32_t TimeTaken = (uint32_t)(EndTime.GetMicroSeconds() - StartTime.GetMicroSeconds());

        switch (TaskType)
        {
        case 1: CPU_Usage.RecordTime(Timer_RSP_Dlist, TimeTaken); break;
        case 2: CPU_Usage.RecordTime(Timer_RSP_Alist, TimeTaken); break;
        default: CPU_Usage.RecordTime(Timer_RSP_Unknown, TimeTaken); break;
        }
    }

    if (ExecuteCycles && (Reg.SP_STATUS_REG & SP_STATUS_HALT) == 0 && Reg.m_RspIntrReg == 0)
    {
        g_SystemTimer->SetTimer(CSystemTimer::RspTimer, 0x200, false);
    }
    WriteTrace(TraceRSP, TraceDebug, "Check interrupts");
    g_Reg->CheckInterrupts();
    if (bShowCPUPer())
    {
        CPU_Usage.StartTimer(CPU_UsageAddr);
    }
    WriteTrace(TraceRSP, TraceDebug, "Done (SP Status %X)", Reg.SP_STATUS_REG);
}

void CRSP_Plugin::UnloadPluginDetails(void)
{
    memset(&m_RSPDebug, 0, sizeof(m_RSPDebug));
    m_DoRspCycles = nullptr;
    m_EnableDebugging = nullptr;
    m_GetDebugInfo = nullptr;
    m_InitiateDebugger = nullptr;
}

uint32_t CRSP_Plugin::RspThread(void)
{
    CRegisters & Reg = m_System->m_Reg;
    for (;;)
    {
        m_RunEvent.IsTriggered(SyncEvent::INFINITE_TIMEOUT);
        if (!m_RomOpened)
        {
            break;
        }
        m_DoRspCycles(100);
        if ((Reg.SP_STATUS_REG & SP_STATUS_HALT) != 0)
        {
            m_RunEvent.Reset();
        }
    }
    return 0;
}

uint32_t CRSP_Plugin::stRspThread(void * lpThreadParameter)
{
    return ((CRSP_Plugin *)lpThreadParameter)->RspThread();
}

void * CRSP_Plugin::GetDebugMenu(void)
{
    return m_RSPDebug.hRSPMenu;
}

void CRSP_Plugin::ProcessMenuItem(int32_t id)
{
    if (m_RSPDebug.ProcessMenuItem)
    {
        m_RSPDebug.ProcessMenuItem(id);
    }
}

PLUGIN_TYPE CRSP_Plugin::type()
{
    return PLUGIN_TYPE_RSP;
}

int32_t CRSP_Plugin::GetDefaultSettingStartRange() const
{
    return FirstRSPDefaultSet;
}

int32_t CRSP_Plugin::GetSettingStartRange() const
{
    return FirstRSPSettings;
}
