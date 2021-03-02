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
    void SetFrequency(uint32_t Frequency, uint32_t BufferSize);
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
