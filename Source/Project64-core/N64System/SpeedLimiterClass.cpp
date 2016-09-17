/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#include "stdafx.h"
#include "Project64-Core/N64System/SpeedLimiterClass.h"

#include <Common/Util.h>

// ---------------------------------------------------

const uint32_t CSpeedLimiter::m_HighSpeed = 60;
const uint32_t CSpeedLimiter::m_MediumSpeed = 15;

// ---------------------------------------------------

CSpeedLimiter::CSpeedLimiter() :
m_Frames(0),
m_Speed(m_HighSpeed),
m_BaseSpeed(m_HighSpeed)
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

void CSpeedLimiter::AlterSpeed( const ESpeedChange SpeedChange )
{
	int32_t SpeedFactor = 1;
	if (SpeedChange == DECREASE_SPEED) { SpeedFactor = -1; }

	if (m_Speed >= m_HighSpeed)
	{
		m_Speed += 10 * SpeedFactor;
	}
	else if (m_Speed >= m_MediumSpeed)
	{
		m_Speed += 5 * SpeedFactor;
	}
	else if (m_Speed > 1 && SpeedChange == DECREASE_SPEED || SpeedChange == INCREASE_SPEED)
	{
		m_Speed += 1 * SpeedFactor;
	}

	SpeedChanged(m_Speed);
	FixSpeedRatio();
}