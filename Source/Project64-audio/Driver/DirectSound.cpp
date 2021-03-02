// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64.
// Copyright(C) 2000-2015 Azimer
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#include "DirectSound.h"
#include "AudioMain.h"
#include "trace.h"
#include "AudioSettings.h"

DirectSoundDriver::DirectSoundDriver() :
    m_AudioIsDone(true),
    m_LOCK_SIZE(0),
    m_lpds(NULL),
    m_lpdsb(NULL),
    m_lpdsbuf(NULL),
    m_handleAudioThread(NULL),
    m_dwAudioThreadId(0)
{
}

bool DirectSoundDriver::Initialize()
{
    WriteTrace(TraceAudioDriver, TraceDebug, "Start");
    if (!SoundDriverBase::Initialize())
    {
        WriteTrace(TraceAudioDriver, TraceDebug, "Done (res: false)");
        return false;
    }

    CGuard guard(m_CS);
    LPDIRECTSOUND8  & lpds = (LPDIRECTSOUND8 &)m_lpds;
    HRESULT hr = DirectSoundCreate8(NULL, &lpds, NULL);
    if (FAILED(hr))
    {
        WriteTrace(TraceAudioDriver, TraceWarning, "Unable to DirectSoundCreate (hr: 0x%08X)", hr);
        WriteTrace(TraceAudioDriver, TraceDebug, "Done (res: false)");
        return false;
    }

    hr = lpds->SetCooperativeLevel((HWND)g_AudioInfo.hwnd, DSSCL_PRIORITY);
    if (FAILED(hr))
    {
        WriteTrace(TraceAudioDriver, TraceWarning, "Failed to SetCooperativeLevel (hr: 0x%08X)", hr);
        WriteTrace(TraceAudioDriver, TraceDebug, "Done (res: false)");
        return false;
    }

    LPDIRECTSOUNDBUFFER & lpdsbuf = (LPDIRECTSOUNDBUFFER &)m_lpdsbuf;
    if (lpdsbuf)
    {
        IDirectSoundBuffer_Release(lpdsbuf);
        lpdsbuf = NULL;
    }
    DSBUFFERDESC dsPrimaryBuff = { 0 };
    dsPrimaryBuff.dwSize = sizeof(DSBUFFERDESC);
    dsPrimaryBuff.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    dsPrimaryBuff.dwBufferBytes = 0;
    dsPrimaryBuff.lpwfxFormat = NULL;

    WAVEFORMATEX wfm = { 0 };
    wfm.wFormatTag = WAVE_FORMAT_PCM;
    wfm.nChannels = 2;
    wfm.nSamplesPerSec = 48000;
    wfm.wBitsPerSample = 16;
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

    LPDIRECTSOUNDBUFFER & lpdsb = (LPDIRECTSOUNDBUFFER &)m_lpdsb;
    hr = lpds->CreateSoundBuffer(&dsPrimaryBuff, &lpdsb, NULL);
    if (SUCCEEDED(hr))
    {
        lpdsb->SetFormat(&wfm);
        lpdsb->Play(0, 0, DSBPLAY_LOOPING);
    }
    WriteTrace(TraceAudioDriver, TraceDebug, "Done (res: true)");
    return true;
}

void DirectSoundDriver::StopAudio()
{
    if (m_handleAudioThread != NULL)
    {
        m_AudioIsDone = true;
        if (WaitForSingleObject((HANDLE)m_handleAudioThread, 5000) == WAIT_TIMEOUT)
        {
            WriteTrace(TraceAudioDriver, TraceError, "time out on close");

            TerminateThread((HANDLE)m_handleAudioThread, 1);
        }
        CloseHandle((HANDLE)m_handleAudioThread);
        m_handleAudioThread = NULL;
    }
}

void DirectSoundDriver::StartAudio()
{
    WriteTrace(TraceAudioDriver, TraceDebug, "Start");
    if (m_handleAudioThread == NULL)
    {
        m_AudioIsDone = false;
        m_handleAudioThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)stAudioThreadProc, this, 0, (LPDWORD)&m_dwAudioThreadId);

        LPDIRECTSOUNDBUFFER & lpdsbuf = (LPDIRECTSOUNDBUFFER &)m_lpdsbuf;
        if (lpdsbuf != NULL)
        {
            lpdsbuf->Play(0, 0, DSBPLAY_LOOPING);
        }
    }
    WriteTrace(TraceAudioDriver, TraceDebug, "Done");
}

void DirectSoundDriver::SetFrequency(uint32_t Frequency, uint32_t BufferSize)
{
    WriteTrace(TraceAudioDriver, TraceDebug, "Start (Frequency: 0x%08X)", Frequency);
    StopAudio();
    m_LOCK_SIZE = (BufferSize * 2);
    SetSegmentSize(m_LOCK_SIZE, Frequency);

    StartAudio();
    WriteTrace(TraceAudioDriver, TraceDebug, "Done");
}

void DirectSoundDriver::SetVolume(uint32_t Volume)
{
    LPDIRECTSOUNDBUFFER & lpdsb = (LPDIRECTSOUNDBUFFER &)m_lpdsb;
    int32_t dsVolume = -((100 - (int32_t)Volume) * 25);
    if (Volume == 0)
    {
        dsVolume = DSBVOLUME_MIN;
    }
    if (lpdsb != NULL)
    {
        lpdsb->SetVolume(dsVolume);
    }
}

void DirectSoundDriver::SetSegmentSize(uint32_t length, uint32_t SampleRate)
{
    if (SampleRate == 0) { return; }
    CGuard guard(m_CS);

    WAVEFORMATEX wfm = { 0 };
    wfm.wFormatTag = WAVE_FORMAT_PCM;
    wfm.nChannels = 2;
    wfm.nSamplesPerSec = SampleRate;
    wfm.wBitsPerSample = 16;
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

    DSBUFFERDESC dsbdesc = { 0 };
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;
    dsbdesc.dwBufferBytes = length * DS_SEGMENTS;
    dsbdesc.lpwfxFormat = &wfm;

    LPDIRECTSOUND8 & lpds = (LPDIRECTSOUND8 &)m_lpds;
    LPDIRECTSOUNDBUFFER & lpdsbuf = (LPDIRECTSOUNDBUFFER &)m_lpdsbuf;
    if (lpds != NULL)
    {
        HRESULT hr = lpds->CreateSoundBuffer(&dsbdesc, &lpdsbuf, NULL);
        if (FAILED(hr))
        {
            WriteTrace(TraceAudioDriver, TraceWarning, "CreateSoundBuffer failed (hr: 0x%08X)", hr);
        }
    }

    if (lpdsbuf != NULL)
    {
        lpdsbuf->Play(0, 0, DSBPLAY_LOOPING);
    }
}

void DirectSoundDriver::AudioThreadProc()
{
    LPDIRECTSOUNDBUFFER & lpdsbuff = (LPDIRECTSOUNDBUFFER &)m_lpdsbuf;
    while (lpdsbuff == NULL && !m_AudioIsDone)
    {
        Sleep(10);
    }

    if (!m_AudioIsDone)
    {
        WriteTrace(TraceAudioDriver, TraceDebug, "Audio Thread Started...");
        DWORD dwStatus;
        lpdsbuff->GetStatus(&dwStatus);
        if ((dwStatus & DSBSTATUS_PLAYING) == 0)
        {
            lpdsbuff->Play(0, 0, 0);
        }

        SetThreadPriority(m_handleAudioThread, THREAD_PRIORITY_HIGHEST);
    }

    uint32_t next_pos = 0, write_pos = 0, last_pos = 0;
    while (lpdsbuff != NULL && !m_AudioIsDone)
    {
        WriteTrace(TraceAudioDriver, TraceVerbose, "last_pos: 0x%08X write_pos: 0x%08X next_pos: 0x%08X", last_pos, write_pos, next_pos);
        while (last_pos == write_pos)
        { // Cycle around until a new buffer position is available
            if (lpdsbuff == NULL)
            {
                break;
            }
            uint32_t play_pos = 0;
            if (lpdsbuff == NULL || FAILED(lpdsbuff->GetCurrentPosition((unsigned long*)&play_pos, NULL)))
            {
                WriteTrace(TraceAudioDriver, TraceDebug, "Error getting audio position...");
                m_AudioIsDone = true;
                break;
            }
            write_pos = play_pos < m_LOCK_SIZE ? (m_LOCK_SIZE * DS_SEGMENTS) - m_LOCK_SIZE : ((play_pos / m_LOCK_SIZE) * m_LOCK_SIZE) - m_LOCK_SIZE;
            WriteTrace(TraceAudioDriver, TraceVerbose, "play_pos: 0x%08X m_write_pos: 0x%08X next_pos: 0x%08X m_LOCK_SIZE: 0x%08X", play_pos, write_pos, next_pos, m_LOCK_SIZE);
            if (last_pos == write_pos)
            {
                WriteTrace(TraceAudioDriver, TraceVerbose, "Sleep");
                Sleep(1);
            }
        }
        // This means we had a buffer segment skipped
        if (next_pos != write_pos)
        {
            WriteTrace(TraceAudioDriver, TraceDebug, "segment skipped");
        }

        // Store our last position
        last_pos = write_pos;

        // Set out next anticipated segment
        next_pos = write_pos + m_LOCK_SIZE;
        if (next_pos >= (m_LOCK_SIZE*DS_SEGMENTS))
        {
            next_pos -= (m_LOCK_SIZE*DS_SEGMENTS);
        }

        if (m_AudioIsDone)
        {
            break;
        }

        // Time to write out to the buffer
        LPVOID lpvPtr1, lpvPtr2;
        DWORD dwBytes1, dwBytes2;
        WriteTrace(TraceAudioDriver, TraceVerbose, "Lock Buffer");
        if (lpdsbuff->Lock(write_pos, m_LOCK_SIZE, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0) != DS_OK)
        {
            WriteTrace(TraceAudioDriver, TraceError, "Error locking sound buffer");
            break;
        }
        WriteTrace(TraceAudioDriver, TraceVerbose, "dwBytes1: 0x%08X dwBytes2: 0x%08X", dwBytes1, dwBytes2);

        {
            CGuard guard(m_CS);
            LoadAiBuffer((uint8_t *)lpvPtr1, dwBytes1);
            if (dwBytes2)
            {
                WriteTrace(TraceAudioDriver, TraceDebug, "Loading second buffer");
                LoadAiBuffer((BYTE *)lpvPtr2, dwBytes2);
            }
        }

        // Fills dwBytes to the Sound Buffer
        WriteTrace(TraceAudioDriver, TraceVerbose, "Unlock Buffer");
        if (FAILED(lpdsbuff->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2)))
        {
            WriteTrace(TraceAudioDriver, TraceError, "Error unlocking sound buffer");
            break;
        }
    }

    LPDIRECTSOUNDBUFFER & lpdsbuf = (LPDIRECTSOUNDBUFFER &)m_lpdsbuf;
    if (lpdsbuf)
    {
        lpdsbuf->Stop();
    }
    WriteTrace(TraceAudioDriver, TraceDebug, "Audio Thread Terminated...");
}
