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
#pragma once

class CAudio
{	
	enum
	{
		ai_full = 0x80000000,
		ai_busy = 0x40000000,
	};
public:
	CAudio();
	~CAudio();

	DWORD GetLength         ();
	DWORD GetStatus         ();
	void  LenChanged        ();
	void  InterruptTimerDone();
	void  BusyTimerDone     ();
	void  Reset             ();
	void  SetViIntr         ( DWORD VI_INTR_TIME );
	void  SetFrequency      ( DWORD Dacrate, DWORD System );

private:
	DWORD  m_SecondBuff;
	DWORD  m_Status;
	DWORD  m_BytesPerSecond;
	int    m_CountsPerByte;
	int    m_FramesPerSecond;
};
