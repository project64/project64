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
    CAudioPlugin(const CAudioPlugin&);				// Disable copy constructor
    CAudioPlugin& operator=(const CAudioPlugin&);	// Disable assignment

    virtual int32_t GetDefaultSettingStartRange() const { return FirstAudioDefaultSet; }
    virtual int32_t GetSettingStartRange() const { return FirstAudioSettings; }
    PLUGIN_TYPE type() { return PLUGIN_TYPE_AUDIO; }

    void * m_hAudioThread;

    bool LoadFunctions(void);
    void UnloadPluginDetails(void);

    void(CALL *AiUpdate)        (int32_t Wait);
    void(CALL *AiDacrateChanged)(SYSTEM_TYPE Type);

    // Function used in a thread for using audio
    static void AudioThread(CAudioPlugin * _this);
};
