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

class CFramePerSecond {
	CNotification * const g_Notify;
	int  m_iFrameRateType, m_ScreenHertz;
	
	enum { NoOfFrames = 7 };

	__int64 Frequency, Frames[NoOfFrames], LastFrame;
	int CurrentFrame;

	static void FrameRateTypeChanged (CFramePerSecond * _this);
	static void ScreenHertzChanged   (CFramePerSecond * _this);


public:
         CFramePerSecond ( CNotification * Notification );
        ~CFramePerSecond ( void );

	void Reset           ( bool ClearDisplay );

	void UpdateDlCounter  ( void );
	void UpdateViCounter  ( void );
	void DisplayDlCounter ( DWORD FrameRate );
	void DisplayViCounter ( DWORD FrameRate );
//	void ClearDisplay     ( void );
};
