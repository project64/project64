/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#pragma comment(lib, "winmm.lib") 

CSpeedLimitor::CSpeedLimitor(void)
{
	m_Frames    = 0;
	m_LastTime  = 0;
	m_Speed     = 60;
	m_BaseSpeed = 60;
	m_Ratio     = 1000.0F / (float)m_Speed;

	TIMECAPS Caps;
	timeGetDevCaps(&Caps, sizeof(Caps));
	if (timeBeginPeriod(Caps.wPeriodMin) == TIMERR_NOCANDO) {
		g_Notify->DisplayError("Error during timer begin");
	}
}

CSpeedLimitor::~CSpeedLimitor(void) {
	TIMECAPS Caps;
	timeGetDevCaps(&Caps, sizeof(Caps));
	timeEndPeriod(Caps.wPeriodMin);
}

void CSpeedLimitor::SetHertz (DWORD Hertz ) {
	m_Speed = Hertz;
	m_BaseSpeed = Hertz;
	FixSpeedRatio();
}

void CSpeedLimitor::FixSpeedRatio ( void )
{
	m_Ratio = 1000.0f / static_cast<double>(m_Speed);
	m_Frames = 0;
	m_LastTime = timeGetTime();
}

bool CSpeedLimitor::Timer_Process (DWORD * FrameRate ) {
	m_Frames += 1;
	DWORD CurrentTime = timeGetTime();
	
	/* Calculate time that should of elapsed for this frame */
	double CalculatedTime = (double)m_LastTime + (m_Ratio * (double)m_Frames);
	if ((double)CurrentTime < CalculatedTime) {
		long time = (int)(CalculatedTime - (double)CurrentTime);
		if (time > 0) {
			Sleep(time);
		}

		/* Refresh current time */
		CurrentTime = timeGetTime();
	}

	if (CurrentTime - m_LastTime >= 1000) {
		/* Output FPS */
		if (FrameRate != NULL) { *FrameRate = m_Frames; }
		m_Frames = 0;
		m_LastTime = CurrentTime;

		return true;
	} else {
		return false;
	}
}

void CSpeedLimitor::IncreaeSpeed ( void )
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

void CSpeedLimitor::DecreaeSpeed ( void )
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
