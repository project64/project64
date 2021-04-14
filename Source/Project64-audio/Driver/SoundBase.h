// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2000-2015 Azimer
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#pragma once
#include <Common/SyncEvent.h>
#include <Common/CriticalSection.h>

class SoundDriverBase
{
public:
    SoundDriverBase();

    void AI_SetFrequency(uint32_t Frequency, uint32_t BufferSize);
    void AI_LenChanged(uint8_t *start, uint32_t length);
    void AI_Startup();
    void AI_Shutdown();
    void AI_Update(bool Wait);
    uint32_t AI_ReadLength();

    virtual void SetFrequency(uint32_t Frequency, uint32_t BufferSize);
    virtual void StartAudio();
    virtual void StopAudio();

protected:
    enum { MAX_SIZE = 48000 * 2 * 2 }; // Max buffer size (44100Hz * 16-bit * stereo)

    virtual bool Initialize();
    void LoadAiBuffer(uint8_t *start, uint32_t length); // Reads in length amount of audio bytes
    uint32_t m_MaxBufferSize;   // Variable size determined by playback rate
    CriticalSection m_CS;

private:
    void BufferAudio();

    SyncEvent m_AiUpdateEvent;
    uint8_t *m_AI_DMAPrimaryBuffer, *m_AI_DMASecondaryBuffer;
    uint32_t m_AI_DMAPrimaryBytes, m_AI_DMASecondaryBytes;
    uint32_t m_BufferRemaining; // Buffer remaining
    uint32_t m_CurrentReadLoc;  // Currently playing buffer
    uint32_t m_CurrentWriteLoc; // Currently writing buffer
    uint8_t m_Buffer[MAX_SIZE]; // Emulated buffers
};
