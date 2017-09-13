/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "SettingsID.h"
#include "AudioSettings.h"
#include "trace.h"
#include "AudioMain.h"

short Set_EnableAudio = 0;
extern bool g_AudioEnabled;

void SettingsChanged(void *)
{
    g_AudioEnabled = GetSystemSetting(Set_EnableAudio) != 0;
}

void SetupAudioSettings(void)
{
    SetModuleName("AndroidAudio");
    RegisterSetting(Output_SwapChannels, Data_DWORD_General, "SwapChannels", "", 0, NULL);
    RegisterSetting(Output_DefaultFrequency, Data_DWORD_General, "DefaultFrequency", "", DEFAULT_FREQUENCY, NULL);
    RegisterSetting(Buffer_PrimarySize, Data_DWORD_General, "BufferPrimarySize", "", PRIMARY_BUFFER_SIZE, NULL);
    RegisterSetting(Buffer_SecondarySize, Data_DWORD_General, "BufferSecondarySize", "", SECONDARY_BUFFER_SIZE, NULL);
    RegisterSetting(Buffer_SecondaryNbr, Data_DWORD_General, "BufferSecondaryNbr", "", SECONDARY_BUFFER_NBR, NULL);
    RegisterSetting(Logging_LogAudioInitShutdown, Data_DWORD_General, "AudioInitShutdown", "Logging", g_ModuleLogLevel[TraceAudioInitShutdown], NULL);
    RegisterSetting(Logging_LogAudioInterface, Data_DWORD_General, "AudioInterface", "Logging", g_ModuleLogLevel[TraceAudioInterface], NULL);

    g_SwapChannels = GetSetting(Output_SwapChannels) != 0;
    g_GameFreq = GetSetting(Output_DefaultFrequency);

    g_ModuleLogLevel[TraceAudioInitShutdown] = GetSetting(Logging_LogAudioInitShutdown);
    g_ModuleLogLevel[TraceAudioInterface] = GetSetting(Logging_LogAudioInterface);

    Set_EnableAudio = FindSystemSettingId("Enable Audio");
    if (Set_EnableAudio != 0)
    {
        SettingsRegisterChange(true, Set_EnableAudio, NULL, SettingsChanged);
        g_AudioEnabled = GetSystemSetting(Set_EnableAudio) != 0;
    }
}