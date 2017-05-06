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
#include <Project64-core/N64System/N64RomClass.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/Plugins/AudioPlugin.h>
#ifdef _WIN32
#include <Windows.h>
#endif

CAudioPlugin::CAudioPlugin() :
    AiLenChanged(NULL),
    AiReadLength(NULL),
    ProcessAList(NULL),
    m_hAudioThread(NULL),
    AiUpdate(NULL),
    AiDacrateChanged(NULL)
{
}

CAudioPlugin::~CAudioPlugin()
{
    Close(NULL);
    UnloadPlugin();
}

bool CAudioPlugin::LoadFunctions(void)
{
    // Find entries for functions in DLL
    void(CALL *InitiateAudio)(void);
    LoadFunction(InitiateAudio);
    LoadFunction(AiDacrateChanged);
    LoadFunction(AiLenChanged);
    LoadFunction(AiReadLength);
    LoadFunction(AiUpdate);
    LoadFunction(ProcessAList);

    // Make sure dll has all needed functions
    if (AiDacrateChanged == NULL) { UnloadPlugin(); return false; }
    if (AiLenChanged == NULL) { UnloadPlugin(); return false; }
    if (AiReadLength == NULL) { UnloadPlugin(); return false; }
    if (InitiateAudio == NULL) { UnloadPlugin(); return false; }
    if (ProcessAList == NULL) { UnloadPlugin(); return false; }

    if (m_PluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == NULL) { UnloadPlugin(); return false; }
    }
    return true;
}

bool CAudioPlugin::Initiate(CN64System * System, RenderWindow * Window)
{
    struct AUDIO_INFO
    {
        void * hwnd;
        void * hinst;

        int32_t MemoryBswaped;    // If this is set to TRUE, then the memory has been pre

        //   bswap on a dword (32 bits) boundry
        //	eg. the first 8 bytes are stored like this:
        //        4 3 2 1   8 7 6 5
        uint8_t * HEADER;	// This is the rom header (first 40h bytes of the rom
        // This will be in the same memory format as the rest of the memory.
        uint8_t * RDRAM;
        uint8_t * DMEM;
        uint8_t * IMEM;

        uint32_t * MI__INTR_REG;

        uint32_t * AI__DRAM_ADDR_REG;
        uint32_t * AI__LEN_REG;
        uint32_t * AI__CONTROL_REG;
        uint32_t * AI__STATUS_REG;
        uint32_t * AI__DACRATE_REG;
        uint32_t * AI__BITRATE_REG;

        void(CALL *CheckInterrupts)(void);
    };

    //Get Function from DLL
    int32_t(CALL *InitiateAudio)(AUDIO_INFO Audio_Info);
    LoadFunction(InitiateAudio);
    if (InitiateAudio == NULL) { return false; }

    AUDIO_INFO Info = { 0 };

#ifdef _WIN32
    Info.hwnd = Window ? Window->GetWindowHandle() : NULL;
    Info.hinst = Window ? Window->GetModuleInstance() : NULL;
#else
    Info.hwnd = NULL;
    Info.hinst = NULL;
#endif
    Info.MemoryBswaped = true;
    Info.CheckInterrupts = DummyCheckInterrupts;

    // We are initializing the plugin before any rom is loaded so we do not have any correct
    // parameters here.. just needed to we can config the DLL.
    if (System == NULL)
    {
        static uint8_t Buffer[100];
        static uint32_t Value = 0;

        Info.HEADER = Buffer;
        Info.RDRAM = Buffer;
        Info.DMEM = Buffer;
        Info.IMEM = Buffer;
        Info.MI__INTR_REG = &Value;
        Info.AI__DRAM_ADDR_REG = &Value;
        Info.AI__LEN_REG = &Value;
        Info.AI__CONTROL_REG = &Value;
        Info.AI__STATUS_REG = &Value;
        Info.AI__DACRATE_REG = &Value;
        Info.AI__BITRATE_REG = &Value;
    }
    // Send initialization information to the DLL
    else
    {
        CMipsMemoryVM & MMU = System->m_MMU_VM;
        CRegisters & Reg = System->m_Reg;

        Info.HEADER = g_Rom->GetRomAddress();
        Info.RDRAM = MMU.Rdram();
        Info.DMEM = MMU.Dmem();
        Info.IMEM = MMU.Imem();
        Info.MI__INTR_REG = &Reg.m_AudioIntrReg;
        Info.AI__DRAM_ADDR_REG = &Reg.AI_DRAM_ADDR_REG;
        Info.AI__LEN_REG = &Reg.AI_LEN_REG;
        Info.AI__CONTROL_REG = &Reg.AI_CONTROL_REG;
        Info.AI__STATUS_REG = &Reg.AI_STATUS_REG;
        Info.AI__DACRATE_REG = &Reg.AI_DACRATE_REG;
        Info.AI__BITRATE_REG = &Reg.AI_BITRATE_REG;
    }

    m_Initialized = InitiateAudio(Info) != 0;

#ifdef _WIN32
    if (System != NULL)
    {
        if (AiUpdate)
        {
            if (m_hAudioThread)
            {
                WriteTrace(TraceAudioPlugin, TraceDebug, "Terminate Audio Thread");
                TerminateThread(m_hAudioThread, 0);
            }
            DWORD ThreadID;
            m_hAudioThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AudioThread, (LPVOID)this, 0, &ThreadID);
        }

        if (System->m_Reg.AI_DACRATE_REG != 0)
        {
            DacrateChanged(System->SystemType());
        }
    }
#endif
    return m_Initialized;
}

void CAudioPlugin::UnloadPluginDetails(void)
{
#ifdef _WIN32
    if (m_hAudioThread)
    {
        WriteTrace(TraceAudioPlugin, TraceDebug, "Terminate Audio Thread");
        TerminateThread(m_hAudioThread, 0);
        m_hAudioThread = NULL;
    }
#endif
    AiDacrateChanged = NULL;
    AiLenChanged = NULL;
    AiReadLength = NULL;
    AiUpdate = NULL;
    ProcessAList = NULL;
}

void CAudioPlugin::DacrateChanged(SYSTEM_TYPE Type)
{
    if (!Initialized()) { return; }
    WriteTrace(TraceAudioPlugin, TraceDebug, "SystemType: %s", Type == SYSTEM_NTSC ? "SYSTEM_NTSC" : "SYSTEM_PAL");

    //uint32_t Frequency = g_Reg->AI_DACRATE_REG * 30;
    //uint32_t CountsPerSecond = (g_Reg->VI_V_SYNC_REG != 0 ? (g_Reg->VI_V_SYNC_REG + 1) * g_Settings->LoadDword(Game_ViRefreshRate) : 500000) * 60;
    AiDacrateChanged(Type);
}

#ifdef _WIN32
void CAudioPlugin::AudioThread(CAudioPlugin * _this)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (g_Settings->LoadBool(Setting_CN64TimeCritical))
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    }
    for (;;)
    {
        _this->AiUpdate(true);
    }
}
#endif
