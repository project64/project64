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
#include "stdafx.h"
#include "Audio.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>

CAudio::CAudio()
{
    Reset();
}

CAudio::~CAudio()
{
}

void CAudio::Reset()
{
    m_SecondBuff = 0;
    m_Status = 0;
    m_BytesPerSecond = 0;
    m_CountsPerByte = g_System->AiCountPerBytes(); // should be calculated ... see below, instead allow from user settings
    if (m_CountsPerByte == 0) m_CountsPerByte = 500; // If the user has no defined value, grant a default and we will calculate
    m_FramesPerSecond = 60;
}

uint32_t CAudio::GetLength()
{
    WriteTraceF(TraceAudio, __FUNCTION__ ": Start (m_SecondBuff = %d)", m_SecondBuff);
    uint32_t TimeLeft = g_SystemTimer->GetTimer(CSystemTimer::AiTimerInterrupt), Res = 0;
    if (TimeLeft > 0)
    {
        Res = (TimeLeft / m_CountsPerByte);
    }
    WriteTraceF(TraceAudio, __FUNCTION__ ": Done (res = %d, TimeLeft = %d)", Res, TimeLeft);
    return (Res + 3)&~3;
}

uint32_t CAudio::GetStatus()
{
    WriteTraceF(TraceAudio, __FUNCTION__ ": m_Status = %X", m_Status);
    return m_Status;
}

void CAudio::LenChanged()
{
    WriteTraceF(TraceAudio, __FUNCTION__ ": Start (g_Reg->AI_LEN_REG = %d)", g_Reg->AI_LEN_REG);
    if (g_Reg->AI_LEN_REG != 0)
    {
        if (g_Reg->AI_LEN_REG >= 0x40000)
        {
            WriteTraceF(TraceAudio, __FUNCTION__ ": *** Ignoring Write, To Large (%X)", g_Reg->AI_LEN_REG);
        }
        else
        {
            m_Status |= ai_busy;
            uint32_t AudioLeft = g_SystemTimer->GetTimer(CSystemTimer::AiTimerInterrupt);
            if (m_SecondBuff == 0)
            {
                if (AudioLeft == 0)
                {
                    WriteTraceF(TraceAudio, __FUNCTION__ ": Set Timer  AI_LEN_REG: %d m_CountsPerByte: %d", g_Reg->AI_LEN_REG, m_CountsPerByte);
                    g_SystemTimer->SetTimer(CSystemTimer::AiTimerInterrupt, g_Reg->AI_LEN_REG * m_CountsPerByte, false);
                }
                else
                {
                    WriteTraceF(TraceAudio, __FUNCTION__ ": Increasing Second Buffer (m_SecondBuff %d Increase: %d)", m_SecondBuff, g_Reg->AI_LEN_REG);
                    m_SecondBuff += g_Reg->AI_LEN_REG;
                    m_Status |= ai_full;
                }
            }
            else
            {
                g_Notify->BreakPoint(__FILEW__, __LINE__);
            }
        }
    }
    else
    {
        WriteTraceF(TraceAudio, __FUNCTION__ ": *** Reset Timer to 0");
        g_SystemTimer->StopTimer(CSystemTimer::AiTimerBusy);
        g_SystemTimer->StopTimer(CSystemTimer::AiTimerInterrupt);
        m_SecondBuff = 0;
        m_Status = 0;
    }

    if (g_Plugins->Audio()->AiLenChanged != NULL)
    {
        g_Plugins->Audio()->AiLenChanged();
    }
    WriteTraceF(TraceAudio, __FUNCTION__ ": Done");
}

void CAudio::InterruptTimerDone()
{
    WriteTraceF(TraceAudio, __FUNCTION__ ": Start (m_SecondBuff = %d)", m_SecondBuff);
    m_Status &= ~ai_full;
    g_Reg->MI_INTR_REG |= MI_INTR_AI;
    g_Reg->CheckInterrupts();
    if (m_SecondBuff != 0)
    {
        g_SystemTimer->SetTimer(CSystemTimer::AiTimerInterrupt, m_SecondBuff * m_CountsPerByte, false);
        m_SecondBuff = 0;
    }
    else
    {
        m_Status &= ~ai_busy;
    }
    if (g_Reg->m_AudioIntrReg == 0)
    {
        g_System->SyncToAudio();
    }
    WriteTrace(TraceAudio, __FUNCTION__ ": Done");
}

void CAudio::BusyTimerDone()
{
    WriteTraceF(TraceAudio, __FUNCTION__ ": Start (m_SecondBuff = %d)", m_SecondBuff);
    g_Notify->BreakPoint(__FILEW__, __LINE__);
    m_Status &= ~ai_busy;
}

void CAudio::SetViIntr(uint32_t VI_INTR_TIME)
{
    double CountsPerSecond = (uint32_t)((double)VI_INTR_TIME * m_FramesPerSecond);
    if (m_BytesPerSecond != 0 && (g_System->AiCountPerBytes() == 0))
    {
        m_CountsPerByte = (int32_t)((double)CountsPerSecond / (double)m_BytesPerSecond);
    }
}

void CAudio::SetFrequency(uint32_t Dacrate, uint32_t System)
{
    WriteTraceF(TraceAudio, __FUNCTION__ "(Dacrate: %X System: %d): AI_BITRATE_REG = %X", Dacrate, System, g_Reg->AI_BITRATE_REG);
    uint32_t Frequency;

    switch (System)
    {
    case SYSTEM_PAL:  Frequency = 49656530 / (Dacrate + 1); break;
    case SYSTEM_MPAL: Frequency = 48628316 / (Dacrate + 1); break;
    default:          Frequency = 48681812 / (Dacrate + 1); break;
    }

    //nBlockAlign = 16 / 8 * 2;
    m_BytesPerSecond = Frequency * 4;
    //m_BytesPerSecond = 194532;
    //m_BytesPerSecond = 128024;

    m_FramesPerSecond = System == SYSTEM_PAL ? 50 : 60;
}