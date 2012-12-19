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

#include "..\\N64 Types.h"

class CSystemTimer
{
public:
	enum TimerType {
		UnknownTimer, 
		CompareTimer, 
		SoftResetTimer, 
		ViTimer, 
		AiTimer, 
		AiTimerDMA, 
		SiTimer, 
		PiTimer, 
		RspTimer, 
		RSPTimerDlist, 
		MaxTimer
	};

	typedef struct {
		bool    Active;
		__int64 CyclesToTimer;
	} TIMER_DETAILS;

public:
	          CSystemTimer         ( int & NextTimer );
	void      SetTimer             ( TimerType Type, DWORD Cycles, bool bRelative );
	DWORD     GetTimer             ( TimerType Type );
	void      StopTimer            ( TimerType Type );
	void      UpdateTimers         ( void ); 
	void      TimerDone            ( void ); 
	void      Reset                ( void );
	void      UpdateCompareTimer   ( void );
	bool      SaveAllowed          ( void );
	
	void      SaveData             ( void * file ) const;
	void      LoadData             ( void * file );

	void RecordDifference( CLog &LogFile, const CSystemTimer& rSystemTimer);

	inline TimerType CurrentType ( void ) const { return m_Current; }

    bool operator == (const CSystemTimer& rSystemTimer) const;
    bool operator != (const CSystemTimer& rSystemTimer) const;

private:	
	TIMER_DETAILS m_TimerDetatils[MaxTimer];
	int           m_LastUpdate; //Timer at last update
	int         & m_NextTimer;  
	TimerType     m_Current;
	bool          m_inFixTimer;

	void SetCompareTimer   ( void );
	void FixTimers         ( void );
};
