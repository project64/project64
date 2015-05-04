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

enum { MAX_FRAMES = 13 };

CProfiling::CProfiling () :
	m_CurrentTimerAddr(Timer_None),
	m_CurrentDisplayCount(MAX_FRAMES),
	m_StartTimeHi(0),
	m_StartTimeLo(0)
{
}

SPECIAL_TIMERS CProfiling::StartTimer(SPECIAL_TIMERS Address) 
{
	SPECIAL_TIMERS OldTimerAddr = StopTimer();
	m_CurrentTimerAddr = Address;

	DWORD HiValue, LoValue;
	_asm {
		pushad
		rdtsc
		mov HiValue, edx
		mov LoValue, eax
		popad
	}
	m_StartTimeHi = HiValue;
	m_StartTimeLo = LoValue;
	return OldTimerAddr;
}

SPECIAL_TIMERS CProfiling::StopTimer() {
	DWORD HiValue, LoValue;
	
	if (m_CurrentTimerAddr == Timer_None) { return m_CurrentTimerAddr; }

	_asm {
		pushad
		rdtsc
		mov HiValue, edx
		mov LoValue, eax
		popad
	}

	__int64 StopTime  = ((unsigned __int64)HiValue << 32) + (unsigned __int64)LoValue;
	__int64 StartTime = ((unsigned __int64)m_StartTimeHi << 32) + (unsigned __int64)m_StartTimeLo;
	__int64 TimeTaken = StopTime - StartTime;
	
	PROFILE_ENRTY Entry = m_Entries.find(m_CurrentTimerAddr);
	if (Entry != m_Entries.end()) {
		Entry->second += TimeTaken;
	} else {
		m_Entries.insert(PROFILE_ENRTIES::value_type(m_CurrentTimerAddr,TimeTaken));
	}

	SPECIAL_TIMERS OldTimerAddr = m_CurrentTimerAddr;
	m_CurrentTimerAddr = Timer_None;
	return OldTimerAddr;
}

void CProfiling::ShowCPU_Usage() {
	__int64 TotalTime, CPU = 0, Alist = 0, Dlist = 0, Idle = 0;
	PROFILE_ENRTY Entry;
	
	if (m_CurrentDisplayCount > 0) { m_CurrentDisplayCount -= 1; return;  }
	m_CurrentDisplayCount = MAX_FRAMES;

	Entry = m_Entries.find(Timer_R4300);
	if (Entry != m_Entries.end()) { CPU = Entry->second; }

	Entry = m_Entries.find(Timer_RefreshScreen);
	if (Entry != m_Entries.end()) { CPU += Entry->second; }

	Entry = m_Entries.find(Timer_RSP_Dlist);
	if (Entry != m_Entries.end()) { Dlist = Entry->second; }
	
	Entry = m_Entries.find(Timer_UpdateScreen);
	if (Entry != m_Entries.end()) { Dlist += Entry->second; }

	Entry = m_Entries.find(Timer_RSP_Alist);
	if (Entry != m_Entries.end()) { Alist = Entry->second; }

	Entry = m_Entries.find(Timer_Idel);
	if (Entry != m_Entries.end()) { Idle = Entry->second; }


	TotalTime = CPU + Alist + Dlist + Idle;

	g_Notify->DisplayMessage(0,L"r4300i: %0.1f%c    GFX: %0.1f%c    Alist: %0.1f%c    Idle: %0.1f%c",
		(float)(((double)CPU / (double)TotalTime) * 100),'%',
		(float)(((double)Dlist / (double)TotalTime) * 100),'%',
		(float)(((double)Alist / (double)TotalTime) * 100),'%',
		(float)(((double)Idle / (double)TotalTime) * 100),'%');

	ResetCounters();
}

void CProfiling::ResetCounters() {
	m_Entries.clear();
}

struct TIMER_NAME {
	SPECIAL_TIMERS Timer;
	char * Name;
};

void CProfiling::GenerateLog() {
	stdstr LogFileName;
	{
		CLog Log;
		Log.Open("Profiling.txt");
		LogFileName = Log.FileName();

		//Get the total time
		__int64 TotalTime = 0;
		for (PROFILE_ENRTY itemTime = m_Entries.begin(); itemTime != m_Entries.end(); itemTime++ ) {
			TotalTime += itemTime->second;
		}

		//Create a sortable list of items
		std::vector<PROFILE_VALUE *> ItemList;
		for (PROFILE_ENRTY Entry = m_Entries.begin(); Entry != m_Entries.end(); Entry++ ) {
			ItemList.push_back(&(*Entry));
		}
		
		//sort the list with a basic bubble sort
		for (size_t OuterPass = 0; OuterPass < (ItemList.size() - 1); OuterPass++ ) {
			for (size_t InnerPass = 0; InnerPass < (ItemList.size() - 1); InnerPass++ ) {
				if (ItemList[InnerPass]->second < ItemList[InnerPass + 1]->second) {
					PROFILE_VALUE * TempPtr = ItemList[InnerPass];
					ItemList[InnerPass] = ItemList[InnerPass + 1];
					ItemList[InnerPass + 1] = TempPtr;
				}
			}
		}

		TIMER_NAME TimerNames[] = {
			{Timer_R4300,         "R4300"},
			{Timer_RSP_Dlist,     "RSP: Dlist"},
			{Timer_RSP_Alist,     "RSP: Alist"},
			{Timer_RSP_Unknown,   "RSP: Unknown"},
			{Timer_RefreshScreen, "Refresh Screen"},
			{Timer_UpdateScreen,  "Update Screen"},
			{Timer_UpdateFPS,     "Update FPS"},
			{Timer_FuncLookup,    "Function Lookup"},
			{Timer_Done,    "Timer_Done"},
			{Timer_GetBlockInfo,    "Timer_GetBlockInfo"},
			{Timer_AnalyseBlock,    "Timer_AnalyseBlock"},
			{Timer_CompileBlock,    "Timer_CompileBlock"},
			{Timer_CompileDone,    "Timer_CompileDone"},
		};
		
		for (size_t count =0; count < ItemList.size(); count++ ) {
			char Buffer[255];
			double CpuUsage = ((double)ItemList[count]->second / (double)TotalTime) * 100;
			if (CpuUsage <= 0.2) { continue; }
			sprintf(Buffer,"Func 0x%08X",ItemList[count]->first);
			for (int NameID = 0; NameID < (sizeof(TimerNames) / sizeof(TIMER_NAME)); NameID++) {
				if (ItemList[count]->first == TimerNames[NameID].Timer) {
					strcpy(Buffer,TimerNames[NameID].Name);
					break;
				}
			}
			Log.LogF("%s\t%2.2f",Buffer,  CpuUsage);
		}
	}

	ShellExecute(NULL,"open",LogFileName.c_str(),NULL,NULL,SW_SHOW);
	ResetCounters();
}
