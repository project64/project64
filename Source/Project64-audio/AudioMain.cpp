/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
* Copyright (C) 2015 Gilles Siberlin                                        *
* Copyright (C) 2007-2009 Richard Goedeken                                  *
* Copyright (C) 2007-2008 Ebenblues                                         *
* Copyright (C) 2003 JttL                                                   *
* Copyright (C) 2002 Hacktarux                                              *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include <Project64-audio/Driver/OpenSLES.h>
#include "audio_1.1.h"
#include "Version.h"
#include <stdio.h>
#include <string.h>
#include "AudioSettings.h"
#include "trace.h"
#include "AudioMain.h"
#include "SettingsID.h"

bool g_AudioEnabled = true;

/* Read header for type definition */
AUDIO_INFO g_AudioInfo;

bool g_PluginInit = false;
uint32_t g_Dacrate = 0;

OpenSLESDriver * g_SoundDriver = NULL;

void PluginInit(void)
{
    if (g_PluginInit)
    {
        return;
    }
    SetupTrace();
    SetupAudioSettings();
    StartTrace();
    g_PluginInit = true;
}

EXPORT void CALL PluginLoaded(void)
{
    PluginInit();
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

EXPORT void CALL AiDacrateChanged(int SystemType)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Start (SystemType: %d)", SystemType);
    if (!g_PluginInit)
    {
        WriteTrace(TraceAudioInterface, TraceNotice, "Plugin has not been initilized");
        WriteTrace(TraceAudioInterface, TraceDebug, "Done");
        return;
    }

    if (g_SoundDriver && g_Dacrate != *g_AudioInfo.AI_DACRATE_REG)
    {
        g_Dacrate = *g_AudioInfo.AI_DACRATE_REG & 0x00003FFF;
        if (g_Dacrate != *g_AudioInfo.AI_DACRATE_REG)
        {
            WriteTrace(TraceAudioInterface, TraceNotice, "Unknown/reserved bits in AI_DACRATE_REG set. 0x%08X", *g_AudioInfo.AI_DACRATE_REG);
        }

        uint32_t video_clock = 0;
        switch (SystemType)
        {
        case SYSTEM_NTSC: video_clock = 48681812; break;
        case SYSTEM_PAL: video_clock = 49656530; break;
        case SYSTEM_MPAL: video_clock = 48628316; break;
        }
        uint32_t Frequency = video_clock / (g_Dacrate + 1);
        g_SoundDriver->AI_SetFrequency(Frequency);
    }
    WriteTrace(TraceAudioInterface, TraceDebug, "Done");
}

EXPORT void CALL AiLenChanged(void)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Start (DRAM_ADDR = 0x%X Len = 0x%X)", *g_AudioInfo.AI_DRAM_ADDR_REG, *g_AudioInfo.AI_LEN_REG);
    if (g_SoundDriver && g_AudioEnabled)
    {
        uint32_t Len = *g_AudioInfo.AI_LEN_REG & 0x3FFF8;
        uint8_t * Buffer = (g_AudioInfo.RDRAM + (*g_AudioInfo.AI_DRAM_ADDR_REG & 0x00FFFFF8));

        g_SoundDriver->AI_LenChanged(Buffer, Len);
    }
    WriteTrace(TraceAudioInterface, TraceDebug, "Done");
}

EXPORT uint32_t CALL AiReadLength(void)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
    return 0;
}

EXPORT void CALL AiUpdate(int32_t /*Wait*/)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

EXPORT void CALL CloseDLL(void)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

EXPORT void CALL DllAbout(void * /*hParent*/)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

EXPORT void CALL DllConfig(void * /*hParent*/)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

EXPORT void CALL DllTest(void * /*hParent*/)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0101;
    PluginInfo->Type = PLUGIN_TYPE_AUDIO;
#ifdef _DEBUG
    sprintf(PluginInfo->Name, "Project64 Audio Plugin (Debug): %s", VER_FILE_VERSION_STR);
#else
    sprintf(PluginInfo->Name, "Project64 Audio Plugin: %s", VER_FILE_VERSION_STR);
#endif
    PluginInfo->MemoryBswaped = true;
    PluginInfo->NormalMemory = false;
}

EXPORT int32_t CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Start");
    if (g_SoundDriver != NULL)
    {
        delete g_SoundDriver;
    }
    g_AudioInfo = Audio_Info;
#ifdef ANDROID
    g_SoundDriver = new OpenSLESDriver;
#endif
    WriteTrace(TraceAudioInterface, TraceDebug, "Done (res: true)");
    return true;
}

EXPORT void CALL RomOpen()
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Start");
    if (g_SoundDriver)
    {
        g_SoundDriver->AI_SetFrequency(DEFAULT_FREQUENCY);
    }
    WriteTrace(TraceAudioInterface, TraceDebug, "Done");
}

EXPORT void CALL RomClosed(void)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Start");
    g_Dacrate = 0;
    if (g_SoundDriver)
    {
        g_SoundDriver->AI_Shutdown();
    }
    WriteTrace(TraceAudioInterface, TraceDebug, "Done");
}

EXPORT void CALL ProcessAList(void)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
}

extern "C" void UseUnregisteredSetting(int /*SettingID*/)
{
    WriteTrace(TraceAudioInterface, TraceDebug, "Called");
#ifdef _WIN32
    DebugBreak();
#endif
}