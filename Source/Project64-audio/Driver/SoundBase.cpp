// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2000-2015 Azimer
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

#include "SoundBase.h"
#include <Common/Util.h>
#include <Project64-audio/AudioSettings.h>
#include <Project64-audio/AudioMain.h>
#include <Project64-audio/trace.h>
#include <string.h>

SoundDriverBase::SoundDriverBase() :
    m_MaxBufferSize(MAX_SIZE),
    m_AI_DMAPrimaryBuffer(NULL),
    m_AI_DMASecondaryBuffer(NULL),
    m_AI_DMAPrimaryBytes(0),
    m_AI_DMASecondaryBytes(0),
    m_CurrentReadLoc(0),
    m_CurrentWriteLoc(0),
    m_BufferRemaining(0)
{
    memset(&m_Buffer, 0, sizeof(m_Buffer));
}

bool SoundDriverBase::Initialize()
{
    return true;
}

void SoundDriverBase::AI_SetFrequency(uint32_t Frequency, uint32_t BufferSize)
{
    SetFrequency(Frequency, BufferSize);
    m_MaxBufferSize = (BufferSize * 8);
    m_CurrentReadLoc = m_CurrentWriteLoc = m_BufferRemaining = 0;
}

void SoundDriverBase::AI_LenChanged(uint8_t *start, uint32_t length)
{
    WriteTrace(TraceAudioDriver, TraceDebug, "Start");

    // Bleed off some of this buffer to smooth out audio
    if (g_settings->SyncAudio() || !g_settings->FullSpeed())
    {
        while ((m_BufferRemaining) == m_MaxBufferSize)
        {
            pjutil::Sleep(1);
        }
    }

    CGuard guard(m_CS);
    BufferAudio();

    if (m_AI_DMASecondaryBuffer != NULL)
    {
        WriteTrace(TraceAudioDriver, TraceDebug, "Discarding previous secondary buffer");
    }
    m_AI_DMASecondaryBuffer = start;
    m_AI_DMASecondaryBytes = length;
    if (m_AI_DMAPrimaryBytes == 0)
    {
        m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer;
        m_AI_DMASecondaryBuffer = NULL;
        m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes;
        m_AI_DMASecondaryBytes = 0;
    }

    *g_AudioInfo.AI_STATUS_REG = AI_STATUS_DMA_BUSY;
    if (m_AI_DMAPrimaryBytes > 0 && m_AI_DMASecondaryBytes > 0)
    {
        *g_AudioInfo.AI_STATUS_REG = (uint32_t)(AI_STATUS_DMA_BUSY | AI_STATUS_FIFO_FULL);
    }
    BufferAudio();
    WriteTrace(TraceAudioDriver, TraceDebug, "Done");
}

void SoundDriverBase::AI_Startup()
{
    WriteTrace(TraceAudioDriver, TraceDebug, "Start");
    m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes = 0;
    m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer = NULL;
    m_MaxBufferSize = MAX_SIZE;
    m_CurrentReadLoc = m_CurrentWriteLoc = m_BufferRemaining = 0;
    if (Initialize())
    {
        StartAudio();
    }
    WriteTrace(TraceAudioDriver, TraceDebug, "Start");
}

void SoundDriverBase::AI_Shutdown()
{
    StopAudio();
}

void SoundDriverBase::AI_Update(bool Wait)
{
    m_AiUpdateEvent.IsTriggered(Wait ? SyncEvent::INFINITE_TIMEOUT : 0);
}

uint32_t SoundDriverBase::AI_ReadLength()
{
    CGuard guard(m_CS);
    return m_AI_DMAPrimaryBytes & ~0x7;
}

void SoundDriverBase::LoadAiBuffer(uint8_t *start, uint32_t length)
{
    static uint8_t nullBuff[MAX_SIZE];
    uint8_t *ptrStart = start != NULL ? start : nullBuff;
    uint32_t writePtr = 0, bytesToMove = length;

    if (bytesToMove > m_MaxBufferSize)
    {
        memset(ptrStart, 0, 100);
        return;
    }
    bool DMAEnabled = (*g_AudioInfo.AI_CONTROL_REG & AI_CONTROL_DMA_ON) == AI_CONTROL_DMA_ON;
    if (!DMAEnabled)
    {
        WriteTrace(TraceAudioDriver, TraceVerbose, "Return silence -- DMA is disabled");
        memset(ptrStart, 0, bytesToMove);
        return;
    }

    CGuard guard(m_CS);
    WriteTrace(TraceAudioDriver, TraceVerbose, "Step 0: Replace depleted stored buffer for next run");
    BufferAudio();

    WriteTrace(TraceAudioDriver, TraceVerbose, "Step 1: Deplete stored buffer (bytesToMove: 0x%08X m_BufferRemaining: 0x%08X)", bytesToMove, m_BufferRemaining);
    while (bytesToMove > 0 && m_BufferRemaining > 0)
    {
        *(uint32_t *)(ptrStart + writePtr) = *(uint32_t *)(m_Buffer + m_CurrentReadLoc);
        m_CurrentReadLoc += 4;
        writePtr += 4;
        m_CurrentReadLoc %= m_MaxBufferSize;
        m_BufferRemaining -= 4;
        bytesToMove -= 4;
    }

    WriteTrace(TraceAudioDriver, TraceVerbose, "Step 2: Fill bytesToMove (0x%08X) with silence", bytesToMove);
    while (bytesToMove > 0)
    {
        *(uint8_t *)(ptrStart + writePtr) = 0;
        writePtr += 1;
        bytesToMove -= 1;
    }

    WriteTrace(TraceAudioDriver, TraceVerbose, "Step 3: Replace depleted stored buffer for next run");
    BufferAudio();
}

void SoundDriverBase::BufferAudio()
{
    WriteTrace(TraceAudioDriver, TraceVerbose, "Start (m_BufferRemaining: 0x%08X m_MaxBufferSize: 0x%08X m_AI_DMAPrimaryBytes: 0x%08X m_AI_DMASecondaryBytes: 0x%08X)", m_BufferRemaining, m_MaxBufferSize, m_AI_DMAPrimaryBytes, m_AI_DMASecondaryBytes);
    while ((m_BufferRemaining < m_MaxBufferSize) && (m_AI_DMAPrimaryBytes > 0 || m_AI_DMASecondaryBytes > 0))
    {
        *(uint16_t *)(m_Buffer + m_CurrentWriteLoc) = *(uint16_t *)(m_AI_DMAPrimaryBuffer + 2);
        *(uint16_t *)(m_Buffer + m_CurrentWriteLoc + 2) = *(uint16_t *)m_AI_DMAPrimaryBuffer;
        m_CurrentWriteLoc += 4;
        m_AI_DMAPrimaryBuffer += 4;
        m_CurrentWriteLoc %= m_MaxBufferSize;
        m_BufferRemaining += 4;
        m_AI_DMAPrimaryBytes -= 4;
        if (m_AI_DMAPrimaryBytes == 0)
        {
            WriteTrace(TraceAudioDriver, TraceVerbose, "Emptied primary buffer");
            m_AI_DMAPrimaryBytes = m_AI_DMASecondaryBytes; m_AI_DMAPrimaryBuffer = m_AI_DMASecondaryBuffer; // Switch
            m_AI_DMASecondaryBytes = 0; m_AI_DMASecondaryBuffer = NULL;
            *g_AudioInfo.AI_STATUS_REG = AI_STATUS_DMA_BUSY;
            *g_AudioInfo.AI_STATUS_REG &= ~AI_STATUS_FIFO_FULL;
            *g_AudioInfo.MI_INTR_REG |= MI_INTR_AI;
            g_AudioInfo.CheckInterrupts();
            if (m_AI_DMAPrimaryBytes == 0)
            {
                *g_AudioInfo.AI_STATUS_REG = 0;
            }
        }
    }
    WriteTrace(TraceAudioDriver, TraceVerbose, "Done (m_BufferRemaining: 0x%08X)", m_BufferRemaining);
}

void SoundDriverBase::SetFrequency(uint32_t /*Frequency*/, uint32_t /*Divider*/)
{
}

void SoundDriverBase::StartAudio()
{
}

void SoundDriverBase::StopAudio()
{
}
