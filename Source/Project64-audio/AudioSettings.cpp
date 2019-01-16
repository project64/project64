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
	m_Set_SyncViaAudioEnabled(0),
    m_Set_EnableAudio(0),
    m_Set_FixedAudio(0),
    m_Set_SyncAudio(0),
    m_Set_FullSpeed(0),
    m_Set_LimitFPS(0),
    m_Set_basic_mode(0),
    m_Set_debugger(0),
    m_Set_log_dir(0),
    m_Set_log_flush(0),
    m_FlushLogs(false),
    m_AudioEnabled(true),
    m_advanced_options(false),
    m_debugger_enabled(false),
    m_Volume(100),
    m_TinyBuffer(true),
    m_FPSBuffer(true),
    m_FixedAudio(false),
    m_SyncAudio(false),
    m_FullSpeed(true)
{
    memset(m_log_dir, 0, sizeof(m_log_dir));
    RegisterSettings();
    ReadSettings();

    if (m_Set_EnableAudio != 0) { SettingsRegisterChange(true, m_Set_EnableAudio, this, stSettingsChanged); }
    if (m_Set_basic_mode != 0) { SettingsRegisterChange(true, m_Set_basic_mode, this, stSettingsChanged); }
    if (m_Set_debugger != 0) { SettingsRegisterChange(true, m_Set_debugger, this, stSettingsChanged); }
    if (m_Set_log_flush != 0) { SettingsRegisterChange(true, m_Set_log_flush, this, stSettingsChanged); }
    if (m_Set_FixedAudio != 0) { SettingsRegisterChange(true, m_Set_FixedAudio, this, stSettingsChanged); }
    if (m_Set_SyncAudio != 0) { SettingsRegisterChange(true, m_Set_SyncAudio, this, stSettingsChanged); }
    if (m_Set_FullSpeed != 0) { SettingsRegisterChange(true, m_Set_FullSpeed, this, stSettingsChanged); }
    if (m_Set_LimitFPS != 0) { SettingsRegisterChange(true, m_Set_LimitFPS, this, stSettingsChanged); }
    SettingsRegisterChange(false, Set_Volume, this, stSettingsChanged);

    SettingsRegisterChange(false, Set_Logging_MD5, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Thread, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Path, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_InitShutdown, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Interface, this, stLogLevelChanged);
    SettingsRegisterChange(false, Set_Logging_Driver, this, stLogLevelChanged);
}

CSettings::~CSettings()
{
    if (m_Set_EnableAudio != 0) { SettingsUnregisterChange(true, m_Set_EnableAudio, this, stSettingsChanged); }
    if (m_Set_basic_mode != 0) { SettingsUnregisterChange(true, m_Set_basic_mode, this, stSettingsChanged); }
    if (m_Set_debugger != 0) { SettingsUnregisterChange(true, m_Set_debugger, this, stSettingsChanged); }
    if (m_Set_log_flush != 0) { SettingsUnregisterChange(true, m_Set_log_flush, this, stSettingsChanged); }
    if (m_Set_FixedAudio != 0) { SettingsUnregisterChange(true, m_Set_FixedAudio, this, stSettingsChanged); }
    if (m_Set_SyncAudio != 0) { SettingsUnregisterChange(true, m_Set_SyncAudio, this, stSettingsChanged); }
    if (m_Set_FullSpeed != 0) { SettingsUnregisterChange(true, m_Set_FullSpeed, this, stSettingsChanged); }
    if (m_Set_LimitFPS != 0) { SettingsUnregisterChange(true, m_Set_LimitFPS, this, stSettingsChanged); }
    SettingsUnregisterChange(false, Set_Volume, this, stSettingsChanged);

    SettingsUnregisterChange(false, Set_Logging_MD5, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Thread, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Path, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_InitShutdown, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Interface, this, stLogLevelChanged);
    SettingsUnregisterChange(false, Set_Logging_Driver, this, stLogLevelChanged);
}

void CSettings::RegisterSettings(void)
{
    SetModuleName("default");
	m_Set_SyncViaAudioEnabled = FindSystemSettingId("SyncViaAudioEnabled");
	m_Set_EnableAudio = FindSystemSettingId("Enable Audio");
    m_Set_FixedAudio = FindSystemSettingId("Fixed Audio");
    m_Set_SyncAudio = FindSystemSettingId("Sync Audio");
    m_Set_FullSpeed = FindSystemSettingId("Full Speed");
    m_Set_LimitFPS = FindSystemSettingId("Limit FPS");
    m_Set_basic_mode = FindSystemSettingId("Basic Mode");
    m_Set_debugger = FindSystemSettingId("Debugger");
    m_Set_log_flush = FindSystemSettingId("Log Auto Flush");
    m_Set_log_dir = FindSystemSettingId("Dir:Log");

    SetModuleName("Audio");
    RegisterSetting(Set_Volume, Data_DWORD_General, "Volume", "Settings", 100, NULL);
    RegisterSetting(Set_Logging_MD5, Data_DWORD_General, "MD5", "Logging", g_ModuleLogLevel[TraceMD5], NULL);
    RegisterSetting(Set_Logging_Thread, Data_DWORD_General, "Thread", "Logging", g_ModuleLogLevel[TraceThread], NULL);
    RegisterSetting(Set_Logging_Path, Data_DWORD_General, "Path", "Logging", g_ModuleLogLevel[TracePath], NULL);
    RegisterSetting(Set_Logging_InitShutdown, Data_DWORD_General, "InitShutdown", "Logging", g_ModuleLogLevel[TraceAudioInitShutdown], NULL);
    RegisterSetting(Set_Logging_Interface, Data_DWORD_General, "Interface", "Logging", g_ModuleLogLevel[TraceAudioInterface], NULL);
    RegisterSetting(Set_Logging_Driver, Data_DWORD_General, "Driver", "Logging", g_ModuleLogLevel[TraceAudioDriver], NULL);
    RegisterSetting(Set_TinyBuffer, Data_DWORD_Game, "TinyBuffer", "", (bool)true, NULL);
    RegisterSetting(Set_FPSBuffer, Data_DWORD_Game, "FPSBuffer", "", (bool)true, NULL);
    LogLevelChanged();
}

void CSettings::SetSyncViaAudioEnabled(bool Enabled)
{
	if (m_Set_SyncViaAudioEnabled != 0)
	{
		SetSystemSetting(m_Set_SyncViaAudioEnabled, Enabled ? 1 : 0);
	}
}

void CSettings::SetAudioEnabled(bool Enabled)
{
    if (m_Set_EnableAudio != 0)
    {
        SetSystemSetting(m_Set_EnableAudio, Enabled ? 1 : 0);
    }
}

void CSettings::SetVolume(uint32_t Volume)
{
    if (m_Set_EnableAudio != 0)
    {
        SetSetting(Set_Volume, Volume);
    }
}

void CSettings::SetTinyBuffer(bool TinyBuffer)
{
    SetSetting(Set_TinyBuffer, TinyBuffer ? 1 : 0);
}

void CSettings::SetFPSBuffer(bool FPSBuffer)
{
    SetSetting(Set_FPSBuffer, FPSBuffer ? 1 : 0);
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

void CSettings::ReadSettings(void)
{
    bool SyncAudio = m_Set_SyncAudio ? GetSystemSetting(m_Set_SyncAudio) != 0 : false;
    bool bLimitFPS = m_Set_LimitFPS ? GetSystemSetting(m_Set_LimitFPS) != 0 : true;

    m_Volume = GetSetting(Set_Volume);
    m_AudioEnabled = m_Set_EnableAudio ? GetSystemSetting(m_Set_EnableAudio) != 0 : true;
    m_advanced_options = m_Set_basic_mode ? GetSystemSetting(m_Set_basic_mode) == 0 : false;
    m_debugger_enabled = m_advanced_options && m_Set_debugger ? GetSystemSetting(m_Set_debugger) == 1 : false;
    m_TinyBuffer = GetSetting(Set_TinyBuffer) != 0;
    m_FPSBuffer = GetSetting(Set_FPSBuffer) != 0;
    m_FullSpeed = m_Set_FullSpeed ? GetSystemSetting(m_Set_FullSpeed) != 0 : false;

    m_SyncAudio = ((!m_advanced_options || bLimitFPS);

    if (m_Set_log_dir != 0)
    {
        GetSystemSettingSz(m_Set_log_dir, m_log_dir, sizeof(m_log_dir));
    }
    m_FlushLogs = m_Set_log_flush != 0 ? GetSystemSetting(m_Set_log_flush) != 0 : false;
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
