#pragma once

class CSettings
{
public:
    CSettings();
    ~CSettings();

    inline bool AudioEnabled(void) const { return m_AudioEnabled; }
    inline bool debugger_enabled(void) const { return m_debugger_enabled; }
    inline uint32_t GetVolume(void) const { return m_Volume; }
    inline uint32_t GetBuffer(void) const { return m_Buffer; }
    inline bool SyncAudio(void) const { return m_SyncAudio; }
    inline bool FullSpeed(void) const { return m_FullSpeed; }
    inline bool FlushLogs(void) const { return m_FlushLogs; }
    inline const char * log_dir(void) const { return m_log_dir; }

    void SetSyncViaAudioEnabled(bool Enabled);
    void SetAudioEnabled(bool Enabled);
    void SetVolume(uint32_t Volume);
    void SetBuffer(uint32_t Buffer);
    void ReadSettings();

private:
    static void stLogLevelChanged(void * _this)
    {
        ((CSettings *)_this)->LogLevelChanged();
    }
    static void stSettingsChanged(void * _this)
    {
        ((CSettings *)_this)->ReadSettings();
    }

    void RegisterSettings(void);
    void LogLevelChanged(void);

	short m_Set_SyncViaAudioEnabled;
	short m_Set_EnableAudio;
    short m_Set_SyncAudio;
    short m_Set_FullSpeed;
    short m_Set_LimitFPS;
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
    uint32_t m_Buffer;
    bool m_SyncAudio;
    bool m_FullSpeed;
};

extern CSettings * g_settings;

void SetupAudioSettings(void);
void CleanupAudioSettings(void);
