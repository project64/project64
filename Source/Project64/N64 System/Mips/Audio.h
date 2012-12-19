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

class CAudio
{	
public:
	CAudio (void);
	~CAudio (void);

	DWORD GetLength         ( void );
	DWORD GetStatus         ( void );
	void  LenChanged        ( void );
	void  TimerDone         ( void );
	void  Reset             ( void );
	void  SetViIntr         ( DWORD VI_INTR_TIME );
	void  SetFrequency      ( DWORD Dacrate, DWORD System );

private:
	DWORD  m_SecondBuff;
	DWORD  m_Status;
	DWORD  m_BytesPerSecond;
	int    m_CountsPerByte;
	int    m_FramesPerSecond;
};
