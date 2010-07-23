#include "stdafx.h"
#pragma comment(lib, "winmm.lib") 

CSpeedLimitor::CSpeedLimitor(CNotification * const _Notify ) :
	_Notify(_Notify)
{
	m_Frames    = 0;
	m_LastTime  = 0;
	m_Speed     = 60;
	m_BaseSpeed = 60;
	m_Ratio     = 1000.0F / (float)m_Speed;

	TIMECAPS Caps;
	timeGetDevCaps(&Caps, sizeof(Caps));
	if (timeBeginPeriod(Caps.wPeriodMin) == TIMERR_NOCANDO) {
		_Notify->DisplayError("Error during timer begin");
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

void CSpeedLimitor::IncreaeSpeed ( int Percent )
{
	m_Speed += m_BaseSpeed * ((float)Percent / 100);
	FixSpeedRatio();
}

void CSpeedLimitor::DecreaeSpeed ( int Percent )
{
	ULONG Unit = m_BaseSpeed * ((float)Percent / 100);
	if (m_Speed > Unit)
	{
		m_Speed -= Unit; 
		FixSpeedRatio();		
	}
}