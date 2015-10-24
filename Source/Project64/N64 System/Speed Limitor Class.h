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

class CSpeedLimitor :
	private CGameSettings
{	
public:
	CSpeedLimitor();
	~CSpeedLimitor();

	void SetHertz(const DWORD Hertz);
	bool Timer_Process(DWORD* const FrameRate);
	void IncreaseSpeed();
	void DecreaseSpeed();

private:
	CSpeedLimitor(const CSpeedLimitor&);            // Disable copy constructor
	CSpeedLimitor& operator=(const CSpeedLimitor&); // Disable assignment

	void FixSpeedRatio();

	DWORD  m_Speed, m_BaseSpeed, m_Frames, m_LastTime;
	double m_Ratio;
};
