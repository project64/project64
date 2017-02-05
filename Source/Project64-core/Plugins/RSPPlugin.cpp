/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/N64Class.h>
#include "RSPPlugin.h"
#include "GFXPlugin.h"
#include <Project64-core/Plugins/AudioPlugin.h>

void DummyFunc1(int a) { a += 1; }

CRSP_Plugin::CRSP_Plugin(void) :
    DoRspCycles(NULL),
    EnableDebugging(NULL),
    m_CycleCount(0),
    GetDebugInfo(NULL),
    InitiateDebugger(NULL)
{
    memset(&m_RSPDebug, 0, sizeof(m_RSPDebug));
}

CRSP_Plugin::~CRSP_Plugin()
{
    Close(NULL);
    UnloadPlugin();
}

bool CRSP_Plugin::LoadFunctions(void)
{
    // Find entries for functions in DLL
    void(CALL *InitiateRSP)(void);
    LoadFunction(InitiateRSP);
    LoadFunction(DoRspCycles);
    _LoadFunction("GetRspDebugInfo", GetDebugInfo);
    _LoadFunction("InitiateRSPDebugger", InitiateDebugger);
    LoadFunction(EnableDebugging);
    if (EnableDebugging == NULL) { EnableDebugging = DummyFunc1; }

    //Make sure dll had all needed functions
    if (DoRspCycles == NULL) { UnloadPlugin(); return false; }
    if (InitiateRSP == NULL) { UnloadPlugin(); return false; }
    if (RomClosed == NULL) { UnloadPlugin(); return false; }
    if (CloseDLL == NULL) { UnloadPlugin(); return false; }

    if (m_PluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == NULL) { UnloadPlugin(); return false; }
    }

    // Get debug info if able
    if (GetDebugInfo != NULL)
    {
        GetDebugInfo(&m_RSPDebug);
    }
    return true;
}

bool CRSP_Plugin::Initiate(CPlugins * Plugins, CN64System * System)
{
    WriteTrace(TraceRSPPlugin, TraceDebug, "Starting");
    if (m_PluginInfo.Version == 1 || m_PluginInfo.Version == 0x100)
    {
        WriteTrace(TraceRSPPlugin, TraceDebug, "Invalid Version: %X", m_PluginInfo.Version);
        WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: false)");
        return false;
    }

    if (m_PluginInfo.Version >= 0x103)
    {
        typedef struct
        {
            void * hInst;
            int MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
                                  bswap on a dword (32 bits) boundry */
            uint8_t * HEADER;	// This is the rom header (first 40h bytes of the rom
            // This will be in the same memory format as the rest of the memory.
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

            void(CALL *CheckInterrupts)(void);
            void(CALL *ProcessDlist)(void);
            void(CALL *ProcessAlist)(void);
            void(CALL *ProcessRdpList)(void);
            void(CALL *ShowCFB)(void);
        } RSP_INFO_1_3;

        RSP_INFO_1_3 Info = { 0 };

#ifdef _WIN32
        Info.hInst = (Plugins != NULL && Plugins->MainWindow() != NULL) ? Plugins->MainWindow()->GetModuleInstance() : NULL;
#else
        Info.hInst = NULL;
#endif
        Info.CheckInterrupts = DummyCheckInterrupts;
        Info.MemoryBswaped = (System == NULL); // only true when the system's not yet loaded
    
        //Get Function from DLL
        void(CALL *InitiateRSP) (RSP_INFO_1_3 RSP_Info, uint32_t * Cycles);
        LoadFunction(InitiateRSP);
        if (InitiateRSP == NULL)
        {
            WriteTrace(TraceRSPPlugin, TraceDebug, "Failed to find InitiateRSP");
            WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: false)");
            return false;
        }

        // We are initializing the plugin before any rom is loaded so we do not have any correct
        // parameters here.. just needed to we can config the DLL.
        if (System == NULL)
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
        // Send initialization information to the DLL
        else
        {
            Info.ProcessDlist = Plugins->Gfx()->ProcessDList;
            Info.ProcessRdpList = Plugins->Gfx()->ProcessRDPList;
            Info.ShowCFB = Plugins->Gfx()->ShowCFB;
            Info.ProcessAlist = Plugins->Audio()->ProcessAList;

            CMipsMemoryVM & MMU = System->m_MMU_VM;
            CRegisters & Reg = System->m_Reg;

            Info.HEADER = g_Rom->GetRomAddress();
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
            int MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
                                  bswap on a dword (32 bits) boundry */
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

            void(CALL *CheckInterrupts)(void);
            void(CALL *ProcessDlist)(void);
            void(CALL *ProcessAlist)(void);
            void(CALL *ProcessRdpList)(void);
            void(CALL *ShowCFB)(void);
        } RSP_INFO_1_1;

        RSP_INFO_1_1 Info = { 0 };

#ifdef _WIN32
        Info.hInst = (Plugins != NULL && Plugins->MainWindow() != NULL) ? Plugins->MainWindow()->GetModuleInstance() : NULL;
#else
        Info.hInst = NULL;
#endif
        Info.CheckInterrupts = DummyCheckInterrupts;
        Info.MemoryBswaped = (System == NULL); // only true when the system's not yet loaded
    
        //Get Function from DLL
        void(CALL *InitiateRSP) (RSP_INFO_1_1 RSP_Info, uint32_t * Cycles);
        LoadFunction(InitiateRSP);
        if (InitiateRSP == NULL)
        {
            WriteTrace(TraceRSPPlugin, TraceDebug, "Failed to find InitiateRSP");
            WriteTrace(TraceRSPPlugin, TraceDebug, "Done (res: false)");
            return false;
        }

        // We are initializing the plugin before any rom is loaded so we do not have any correct
        // parameters here.. just needed to we can config the DLL.
        if (System == NULL)
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

void CRSP_Plugin::UnloadPluginDetails(void)
{
    memset(&m_RSPDebug, 0, sizeof(m_RSPDebug));
    DoRspCycles = NULL;
    EnableDebugging = NULL;
    GetDebugInfo = NULL;
    InitiateDebugger = NULL;
}

void CRSP_Plugin::ProcessMenuItem(int id)
{
    if (m_RSPDebug.ProcessMenuItem)
    {
        m_RSPDebug.ProcessMenuItem(id);
    }
}
