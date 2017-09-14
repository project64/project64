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
#include <Settings/Settings.h>
#include <Project64-audio/trace.h>
#include "SettingsID.h"
#include "AudioSettings.h"

CSettings * g_settings = NULL;

CSettings::CSettings() :
    m_Set_EnableAudio(0),
    m_Set_basic_mode(0),
    m_Set_debugger(0),
    m_Set_log_dir(0),
    m_Set_log_flush(0),
    m_FlushLogs(false),
    m_AudioEnabled(true),
    m_advanced_options(false),
    m_debugger_enabled(false)
{
    memset(m_log_dir, 0, sizeof(m_log_dir));
    RegisterSettings();
    ReadSettings();
}

CSettings::~CSettings()
{
}

void CSettings::RegisterSettings(void)
{
    SetModuleName("default");
    m_Set_EnableAudio = FindSystemSettingId("Enable Audio");
    m_Set_basic_mode = FindSystemSettingId("Basic Mode");
    m_Set_debugger = FindSystemSettingId("Debugger");
    m_Set_log_flush = FindSystemSettingId("Log Auto Flush");
    m_Set_log_dir = FindSystemSettingId("Dir:Log");

    SetModuleName("Project64-Audio");
    RegisterSetting(Set_Logging_MD5, Data_DWORD_General, "MD5", "Logging", g_ModuleLogLevel[TraceMD5], NULL);
    RegisterSetting(Set_Logging_Thread, Data_DWORD_General, "Thread", "Logging", g_ModuleLogLevel[TraceThread], NULL);
    RegisterSetting(Set_Logging_Path, Data_DWORD_General, "Path", "Logging", g_ModuleLogLevel[TracePath], NULL);
    RegisterSetting(Set_Logging_InitShutdown, Data_DWORD_General, "InitShutdown", "Logging", g_ModuleLogLevel[TraceAudioInitShutdown], NULL);
    RegisterSetting(Set_Logging_Interface, Data_DWORD_General, "Interface", "Logging", g_ModuleLogLevel[TraceAudioInterface], NULL);
    RegisterSetting(Set_Logging_Driver, Data_DWORD_General, "Driver", "Logging", g_ModuleLogLevel[TraceAudioDriver], NULL);
    LogLevelChanged();
}

void CSettings::ReadSettings()
{
    m_AudioEnabled = m_Set_EnableAudio ? GetSystemSetting(m_Set_EnableAudio) != 0 : true;
    m_advanced_options = m_Set_basic_mode ? GetSystemSetting(m_Set_basic_mode) == 0 : false;
    m_debugger_enabled = m_advanced_options && m_Set_debugger ? GetSystemSetting(m_Set_debugger) == 1 : false;

    if (m_Set_log_dir != 0)
    {
        GetSystemSettingSz(m_Set_log_dir, m_log_dir, sizeof(m_log_dir));
    }
    m_FlushLogs = m_Set_log_flush != 0 ? GetSystemSetting(m_Set_log_flush) != 0 : false;
}

void CSettings::LogLevelChanged(void)
{
    g_ModuleLogLevel[TraceMD5] = GetSetting(Set_Logging_MD5);
    g_ModuleLogLevel[TraceThread] = GetSetting(Set_Logging_Thread);
    g_ModuleLogLevel[TracePath] = GetSetting(Set_Logging_Path);
    g_ModuleLogLevel[TraceAudioInitShutdown] = GetSetting(Set_Logging_InitShutdown);
    g_ModuleLogLevel[TraceAudioInterface] = GetSetting(Set_Logging_Interface);
    g_ModuleLogLevel[TraceAudioDriver] = GetSetting(Set_Logging_Driver);
}

void SetupAudioSettings(void)
{
    if (g_settings == NULL)
    {
        g_settings = new CSettings;
    }
}

void CleanupAudioSettings(void)
{
    if (g_settings)
    {
        delete g_settings;
        g_settings = NULL;
    }
}