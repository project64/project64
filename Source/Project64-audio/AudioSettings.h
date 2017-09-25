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
#pragma once

class CSettings
{
public:
    CSettings();
    ~CSettings();

    inline bool AudioEnabled(void) const { return m_AudioEnabled; }
    inline bool debugger_enabled(void) const { return m_debugger_enabled; }
    inline uint32_t GetVolume(void) const { return m_Volume; }
    inline bool FlushLogs(void) const { return m_FlushLogs; }
    inline const char * log_dir(void) const { return m_log_dir; }

    void SetAudioEnabled(bool Enabled);
    void SetVolume(uint32_t Volume);
    void ReadSettings();

private:
    static void stLogLevelChanged(void * _this)
    {
        ((CSettings *)_this)->LogLevelChanged();
    }
    static void stSettingsChanged(void * _this)
    {
        ((CSettings *)_this)->SettingsChanged();
    }

    void RegisterSettings(void);
    void LogLevelChanged(void);
    void SettingsChanged(void);

    short m_Set_EnableAudio;
    short m_Set_basic_mode;
    short m_Set_debugger;
    short m_Set_log_dir;
    short m_Set_log_flush;
    char m_log_dir[260];
    bool m_FlushLogs;
    bool m_AudioEnabled;
    bool m_advanced_options;
    bool m_debugger_enabled;
    uint32_t m_Volume;
};

extern CSettings * g_settings;

void SetupAudioSettings(void);
void CleanupAudioSettings(void);
