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

typedef std::map<SPECIAL_TIMERS, __int64 >     PROFILE_ENRTIES;
typedef PROFILE_ENRTIES::iterator     PROFILE_ENRTY;
typedef PROFILE_ENRTIES::value_type   PROFILE_VALUE;

class CProfiling 
{
public:	
	CProfiling (void);
	
	//recording timing against current timer, returns the address of the timer stoped
	SPECIAL_TIMERS StartTimer ( SPECIAL_TIMERS Address );
	SPECIAL_TIMERS StopTimer  ( void );

	//Display the CPU Usage
	void ShowCPU_Usage ( void ); 

	//Reset all the counters back to 0
	void ResetCounters ( void );

	//Generate a log file with the current results, this will also reset the counters
	void GenerateLog   ( void );

private:
	CProfiling(const CProfiling&);				// Disable copy constructor
	CProfiling& operator=(const CProfiling&);		// Disable assignment

	SPECIAL_TIMERS m_CurrentTimerAddr;
	DWORD m_CurrentDisplayCount;
	DWORD m_StartTimeHi, m_StartTimeLo; //The Current Timer start time
	PROFILE_ENRTIES m_Entries;

};
