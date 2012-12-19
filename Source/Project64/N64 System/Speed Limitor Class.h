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
#pragma once

class CSpeedLimitor {
	CNotification * const g_Notify;
	DWORD                 m_Speed, m_BaseSpeed, m_Frames, m_LastTime;
	double                m_Ratio;

	void FixSpeedRatio     ( void );
	
public:
	     CSpeedLimitor     ( CNotification * const g_Notify );
	    ~CSpeedLimitor     ( void );
	void SetHertz          ( const DWORD Hertz );
	bool Timer_Process     ( DWORD * const FrameRate );
	void IncreaeSpeed      ( int Percent );
	void DecreaeSpeed      ( int Percent );

};
