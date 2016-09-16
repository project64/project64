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
#include <Common/Util.h>

CSpeedLimiter::CSpeedLimiter() :
m_Frames(0),
m_Speed(60),
m_BaseSpeed(60)
{
}

CSpeedLimiter::~CSpeedLimiter()
{
}

void CSpeedLimiter::SetHertz(uint32_t Hertz)
{
    m_Speed = Hertz;
    m_BaseSpeed = Hertz;
    FixSpeedRatio();
}

void CSpeedLimiter::FixSpeedRatio()
{
    m_MicroSecondsPerFrame = 1000000 / m_Speed;
    m_Frames = 0;
}

bool CSpeedLimiter::Timer_Process(uint32_t * FrameRate)
{
    m_Frames += 1;
    HighResTimeStamp CurrentTime;
    CurrentTime.SetToNow();

    /* Calculate time that should of elapsed for this frame */
    uint64_t LastTime = m_LastTime.GetMicroSeconds(), CurrentTimeValue = CurrentTime.GetMicroSeconds();
    if (LastTime == 0)
    {
        m_Frames = 0;
        m_LastTime = CurrentTime;
        return true;
    }
    uint64_t CalculatedTime = LastTime + (m_MicroSecondsPerFrame * m_Frames);
    if (CurrentTimeValue < CalculatedTime)
    {
        int32_t time = (int)(CalculatedTime - CurrentTimeValue);
        if (time > 0)
        {
            pjutil::Sleep((time / 1000) + 1);
        }
        /* Refresh current time */
        CurrentTime.SetToNow();
        CurrentTimeValue = CurrentTime.GetMicroSeconds();
    }
    if (CurrentTimeValue - LastTime >= 1000000)
    {
        /* Output FPS */
        if (FrameRate != NULL) { *FrameRate = m_Frames; }
        m_Frames = 0;
        m_LastTime = CurrentTime;
        return true;
    }
    return false;
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
