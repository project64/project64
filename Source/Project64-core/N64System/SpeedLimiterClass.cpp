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
#include "SpeedLimiterClass.h"
#include <common/util.h>
#include <Windows.h>
#include <Mmsystem.h>

#pragma comment(lib, "winmm.lib")

CSpeedLimiter::CSpeedLimiter()
{
    m_Frames = 0;
    m_LastTime = 0;
    m_Speed = 60;
    m_BaseSpeed = 60;
    m_Ratio = 1000.0F / (float)m_Speed;

    TIMECAPS Caps;
    timeGetDevCaps(&Caps, sizeof(Caps));
    if (timeBeginPeriod(Caps.wPeriodMin) == TIMERR_NOCANDO)
    {
        g_Notify->DisplayError(L"Error during timer begin");
    }
}

CSpeedLimiter::~CSpeedLimiter()
{
    TIMECAPS Caps;
    timeGetDevCaps(&Caps, sizeof(Caps));
    timeEndPeriod(Caps.wPeriodMin);
}

void CSpeedLimiter::SetHertz(uint32_t Hertz)
{
    m_Speed = Hertz;
    m_BaseSpeed = Hertz;
    FixSpeedRatio();
}

void CSpeedLimiter::FixSpeedRatio()
{
    m_Ratio = 1000.0f / static_cast<double>(m_Speed);
    m_Frames = 0;
    m_LastTime = timeGetTime();
}

bool CSpeedLimiter::Timer_Process(uint32_t * FrameRate)
{
    m_Frames += 1;
    uint32_t CurrentTime = timeGetTime();

    /* Calculate time that should of elapsed for this frame */
    double CalculatedTime = (double)m_LastTime + (m_Ratio * (double)m_Frames);
    if ((double)CurrentTime < CalculatedTime)
    {
        int32_t time = (int)(CalculatedTime - (double)CurrentTime);
        if (time > 0)
        {
            pjutil::Sleep(time);
        }

        /* Refresh current time */
        CurrentTime = timeGetTime();
    }

    if (CurrentTime - m_LastTime >= 1000)
    {
        /* Output FPS */
        if (FrameRate != NULL) { *FrameRate = m_Frames; }
        m_Frames = 0;
        m_LastTime = CurrentTime;

        return true;
    }
    else
    {
        return false;
    }
}

void CSpeedLimiter::IncreaseSpeed()
{
    if (m_Speed >= 60)
    {
        m_Speed += 10;
    }
    else if (m_Speed >= 15)
    {
        m_Speed += 5;
    }
    else
    {
        m_Speed += 1;
    }
    SpeedChanged(m_Speed);
    FixSpeedRatio();
}

void CSpeedLimiter::DecreaseSpeed()
{
    if (m_Speed > 60)
    {
        m_Speed -= 10;
    }
    else if (m_Speed > 15)
    {
        m_Speed -= 5;
    }
    else if (m_Speed > 1)
    {
        m_Speed -= 1;
    }
    SpeedChanged(m_Speed);
    FixSpeedRatio();
}
