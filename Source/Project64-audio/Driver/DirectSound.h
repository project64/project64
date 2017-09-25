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
#include "SoundBase.h"

class DirectSoundDriver :
    public SoundDriverBase
{
    enum { DS_SEGMENTS = 4 };

public:
    DirectSoundDriver();
    bool Initialize();
    void StopAudio();							// Stops the Audio PlayBack (as if paused)
    void StartAudio();							// Starts the Audio PlayBack (as if unpaused)
    void SetFrequency(uint32_t Frequency);
    void SetVolume(uint32_t Volume);

private:
    static uint32_t __stdcall stAudioThreadProc(DirectSoundDriver * _this) { _this->AudioThreadProc(); return 0; }

    void SetSegmentSize(uint32_t length, uint32_t SampleRate);
    void AudioThreadProc();

    bool m_AudioIsDone;
    uint32_t m_LOCK_SIZE;
    void * m_lpds;
    void * m_lpdsb;
    void * m_lpdsbuf;
    void * m_handleAudioThread;
    uint32_t m_dwAudioThreadId;
};