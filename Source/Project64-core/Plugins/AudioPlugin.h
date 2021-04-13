#pragma once
#include <Project64-core/Plugins/PluginBase.h>

class CAudioPlugin : public CPlugin
{
public:
    CAudioPlugin(void);
    ~CAudioPlugin();

    void DacrateChanged(SYSTEM_TYPE Type);
    bool Initiate(CN64System * System, RenderWindow * Window);

    void(CALL *AiLenChanged)(void);
    uint32_t(CALL *AiReadLength)(void);
    void(CALL *ProcessAList)(void);

private:
    CAudioPlugin(const CAudioPlugin&);
    CAudioPlugin& operator=(const CAudioPlugin&);

    virtual int32_t GetDefaultSettingStartRange() const { return FirstAudioDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstAudioSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_AUDIO; }

    void * m_hAudioThread;

    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    void(CALL *AiUpdate) (int32_t Wait);
    void(CALL *AiDacrateChanged)(SYSTEM_TYPE Type);

    static void AudioThread(CAudioPlugin * _this);
};
