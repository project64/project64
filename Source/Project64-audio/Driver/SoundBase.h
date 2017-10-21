/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
* Copyright (C) 2000-2015 Azimer. All rights reserved.                      *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
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
    enum { MAX_SIZE = 48000 * 2 * 2 }; // Max Buffer Size (44100Hz * 16bit * Stereo)

    virtual bool Initialize();
    void LoadAiBuffer(uint8_t *start, uint32_t length); // Reads in length amount of audio bytes
    uint32_t m_MaxBufferSize;   // Variable size determined by Playback rate
    CriticalSection m_CS;

private:
    void BufferAudio();

    SyncEvent m_AiUpdateEvent;
    uint8_t *m_AI_DMAPrimaryBuffer, *m_AI_DMASecondaryBuffer;
    uint32_t m_AI_DMAPrimaryBytes, m_AI_DMASecondaryBytes;
    uint32_t m_BufferRemaining; // Buffer remaining
    uint32_t m_CurrentReadLoc;  // Currently playing Buffer
    uint32_t m_CurrentWriteLoc; // Currently writing Buffer
    uint8_t m_Buffer[MAX_SIZE]; // Emulated buffers
};
