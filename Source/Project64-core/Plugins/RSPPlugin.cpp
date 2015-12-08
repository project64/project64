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
#include <Project64-core/N64System/Mips/MemoryClass.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include "RSPPlugin.h"
#include "GFXplugin.h"
#include "AudioPlugin.h"
#include <Windows.h>

void DummyFunc1(int a) { a += 1;}

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
    Close();
    UnloadPlugin();
}

bool CRSP_Plugin::LoadFunctions(void)
{
    // Find entries for functions in DLL
    void(__cdecl *InitiateRSP)(void);
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
        GetDebugInfo(&m_RSPDebug);

    return true;
}

bool CRSP_Plugin::Initiate(CPlugins * Plugins, CN64System * System)
{
    if (m_PluginInfo.Version == 1 || m_PluginInfo.Version == 0x100)
    {
        return false;
    }

    typedef struct
    {
        HINSTANCE hInst;
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

        void(__cdecl *CheckInterrupts)(void);
        void(__cdecl *ProcessDlist)(void);
        void(__cdecl *ProcessAlist)(void);
        void(__cdecl *ProcessRdpList)(void);
        void(__cdecl *ShowCFB)(void);
    } RSP_INFO_1_1;

    RSP_INFO_1_1 Info = { 0 };

    Info.hInst = GetModuleHandle(NULL);
    Info.CheckInterrupts = DummyCheckInterrupts;
    Info.MemoryBswaped = (System == NULL); // only true when the system's not yet loaded

    //Get Function from DLL
    void(__cdecl *InitiateRSP)    (RSP_INFO_1_1 Audio_Info, uint32_t * Cycles);
    LoadFunction(InitiateRSP);
    if (InitiateRSP == NULL) { return false; }

    // We are initializing the plugin before any rom is loaded so we do not have any correct
    // parameters here.. just needed to we can config the DLL.
    if (System == NULL)
    {
        uint8_t Buffer[100];
        uint32_t Value = 0;

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

        Info.RDRAM = g_MMU->Rdram();
        Info.DMEM = g_MMU->Dmem();
        Info.IMEM = g_MMU->Imem();

        Info.MI__INTR_REG = &g_Reg->m_RspIntrReg;

        Info.SP__MEM_ADDR_REG = &g_Reg->SP_MEM_ADDR_REG;
        Info.SP__DRAM_ADDR_REG = &g_Reg->SP_DRAM_ADDR_REG;
        Info.SP__RD_LEN_REG = &g_Reg->SP_RD_LEN_REG;
        Info.SP__WR_LEN_REG = &g_Reg->SP_WR_LEN_REG;
        Info.SP__STATUS_REG = &g_Reg->SP_STATUS_REG;
        Info.SP__DMA_FULL_REG = &g_Reg->SP_DMA_FULL_REG;
        Info.SP__DMA_BUSY_REG = &g_Reg->SP_DMA_BUSY_REG;
        Info.SP__PC_REG = &g_Reg->SP_PC_REG;
        Info.SP__SEMAPHORE_REG = &g_Reg->SP_SEMAPHORE_REG;

        Info.DPC__START_REG = &g_Reg->DPC_START_REG;
        Info.DPC__END_REG = &g_Reg->DPC_END_REG;
        Info.DPC__CURRENT_REG = &g_Reg->DPC_CURRENT_REG;
        Info.DPC__STATUS_REG = &g_Reg->DPC_STATUS_REG;
        Info.DPC__CLOCK_REG = &g_Reg->DPC_CLOCK_REG;
        Info.DPC__BUFBUSY_REG = &g_Reg->DPC_BUFBUSY_REG;
        Info.DPC__PIPEBUSY_REG = &g_Reg->DPC_PIPEBUSY_REG;
        Info.DPC__TMEM_REG = &g_Reg->DPC_TMEM_REG;
    }

    InitiateRSP(Info, &m_CycleCount);
    m_Initialized = true;

    //jabo had a bug so I call CreateThread so his dllmain gets called again
    DWORD ThreadID;
    HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyFunction, NULL, 0, &ThreadID);
    CloseHandle(hthread);
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
