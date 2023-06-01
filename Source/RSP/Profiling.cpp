#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include "profiling.h"
#pragma warning(disable:4786)
#include <Common/StdString.h>
#include <Common/File.h>
#include <Common/Log.h>
#include <map>
#include <vector>

class CProfiling
{
typedef std::map<DWORD, __int64 >     PROFILE_ENRTIES;
typedef PROFILE_ENRTIES::iterator     PROFILE_ENRTY;
typedef PROFILE_ENRTIES::value_type   PROFILE_VALUE;
typedef struct { SPECIAL_TIMERS Timer; char * Name; } TIMER_NAME;

	DWORD m_CurrentTimerAddr, CurrentDisplayCount;
	DWORD m_StartTimeHi, m_StartTimeLo; // The current timer start time
	PROFILE_ENRTIES m_Entries;

public:	
	CProfiling ()
	{
		m_CurrentTimerAddr = Timer_None;
	}
	
	// Recording timing against current timer, returns the address of the timer stopped
	DWORD StartTimer ( DWORD Address )
	{
		DWORD OldTimerAddr = StopTimer();
		m_CurrentTimerAddr = Address;

#if defined(_M_IX86) && defined(_MSC_VER)
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
#else
		DebugBreak();
#endif
		return OldTimerAddr;
	}
	DWORD StopTimer  ( void )
	{		
		if (m_CurrentTimerAddr == Timer_None) { return m_CurrentTimerAddr; }

#if defined(_M_IX86) && defined(_MSC_VER)
		DWORD HiValue, LoValue;
		_asm {
			pushad
			rdtsc
			mov HiValue, edx
			mov LoValue, eax
			popad
		}

		__int64 StopTime = ((unsigned __int64)HiValue << 32) + (unsigned __int64)LoValue;
		__int64 StartTime = ((unsigned __int64)m_StartTimeHi << 32) + (unsigned __int64)m_StartTimeLo;
		__int64 TimeTaken = StopTime - StartTime;

		PROFILE_ENRTY Entry = m_Entries.find(m_CurrentTimerAddr);
		if (Entry != m_Entries.end())
		{
			Entry->second += TimeTaken;
		}
		else
		{
			m_Entries.insert(PROFILE_ENRTIES::value_type(m_CurrentTimerAddr, TimeTaken));
		}
#else
		DebugBreak();
#endif

		DWORD OldTimerAddr = m_CurrentTimerAddr;
		m_CurrentTimerAddr = Timer_None;
		return OldTimerAddr;
	}

	// Reset all the counters back to 0
	void ResetCounters ( void )
	{
		m_Entries.clear();
	}

	// Generate a log file with the current results, this will also reset the counters
	void GenerateLog   ( void )
	{
		stdstr LogFileName;
		{
			CLog Log;
			Log.Open("RSP Profiling.txt");
			LogFileName = Log.FileName();

			// Get the total time
			__int64 TotalTime = 0;
			for (PROFILE_ENRTY itemTime = m_Entries.begin(); itemTime != m_Entries.end(); itemTime++ )
			{
				TotalTime += itemTime->second;
			}

			// Create a sortable list of items
			std::vector<PROFILE_VALUE *> ItemList;
			for (PROFILE_ENRTY Entry = m_Entries.begin(); Entry != m_Entries.end(); Entry++ )
			{
				ItemList.push_back(&(*Entry));
			}

			// Sort the list with a basic bubble sort
			if (ItemList.size() > 0)
			{
				for (size_t OuterPass = 0; OuterPass < (ItemList.size() - 1); OuterPass++ )
				{
					for (size_t InnerPass = 0; InnerPass < (ItemList.size() - 1); InnerPass++ )
					{
						if (ItemList[InnerPass]->second < ItemList[InnerPass + 1]->second)
						{
							PROFILE_VALUE * TempPtr = ItemList[InnerPass];
							ItemList[InnerPass] = ItemList[InnerPass + 1];
							ItemList[InnerPass + 1] = TempPtr;
						}
					}
				}
			}

			TIMER_NAME TimerNames[] = {
				{Timer_Compiling,     "RSP: Compiling"},
				{Timer_RSP_Running,   "RSP: Running"},
				{Timer_R4300_Running, "R4300i: Running"},
				{Timer_RDP_Running,   "RDP: Running"},
			};

			for (size_t count = 0; count < ItemList.size(); count++ )
			{
				char Buffer[255];
				float CpuUsage = (float)(((double)ItemList[count]->second / (double)TotalTime) * 100);

				if (CpuUsage <= 0.2) { continue; }
				sprintf(Buffer, "Function 0x%08X", ItemList[count]->first);
				for (int NameID = 0; NameID < (sizeof(TimerNames) / sizeof(TIMER_NAME)); NameID++)
				{
					if (ItemList[count]->first == (DWORD)TimerNames[NameID].Timer)
					{
						strcpy(Buffer,TimerNames[NameID].Name);
						break;
					}
				}
				Log.LogF("%s\t%2.2f",Buffer,  CpuUsage);
			}
		}

		ShellExecuteA(NULL,"open",LogFileName.c_str(),NULL,NULL,SW_SHOW);
		ResetCounters();
	}
};


CProfiling& GetProfiler ( void )
{
	static CProfiling Profile;
	return Profile;
}

void ResetTimerList (void)
{
	GetProfiler().ResetCounters();
}

DWORD StartTimer (DWORD Address)
{
	return GetProfiler().StartTimer(Address);
}

void StopTimer (void)
{
	GetProfiler().StopTimer();
}

void GenerateTimerResults (void)
{
	GetProfiler().GenerateLog();
}

#ifdef todelete
#include <windows.h>
#include <stdio.h>

typedef struct {
	char Label[100];
	__int64 TimeTotal;
} TIME_STAMP_ENTRY;

DWORD StartTimeHi, StartTimeLo, StopTimeHi, StopTimeLo, TSE_Count, TSE_Max;
TIME_STAMP_ENTRY * TS_Entries = NULL;
char LastLabel[100];

void ResetTimerList (void)
{
	if (TS_Entries) { free(TS_Entries); }
	TS_Entries = NULL;
	TSE_Count = 0;
	TSE_Max = 0;
}

void StartTimer (char * Label)
{
	strcpy(LastLabel,Label);
	_asm {
		pushad
		rdtsc
		mov StartTimeHi, edx
		mov StartTimeLo, eax
		popad
	}
}

void StopTimer (void)
{
	_asm {
		pushad
		rdtsc
		mov StopTimeHi, edx
		mov StopTimeLo, eax
		popad
	}
	if (strlen(LastLabel) == 0) { return; }
	{
		DWORD count;

		for (count = 0; count < TSE_Count; count ++)
		{
			if (strcmp(LastLabel,TS_Entries[count].Label) == 0)
			{
				__int64 Time = ((unsigned __int64)StopTimeHi << 32) + (unsigned __int64)StopTimeLo;
				Time -= ((unsigned __int64)StartTimeHi << 32) + (unsigned __int64)StartTimeLo;
				TS_Entries[count].TimeTotal += Time;
				return;
			}
		}
	}
	if (TSE_Count == 0)
	{
		TS_Entries = (TIME_STAMP_ENTRY *)malloc(sizeof(TIME_STAMP_ENTRY) * 100);
		if (TS_Entries == NULL)
		{
			MessageBox(NULL,"TIME_STAMP_ENTRY == NULL ??","ERROR",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
		}
		TSE_Max = 100;
	}
	else if (TSE_Count == TSE_Max)
	{
		TSE_Max += 100;
		TS_Entries = (TIME_STAMP_ENTRY *)realloc(TS_Entries,sizeof(TIME_STAMP_ENTRY) * TSE_Max);
		if (TS_Entries == NULL)
		{
			MessageBox(NULL,"TIME_STAMP_ENTRY == NULL ??","ERROR",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
		}
	}
	strcpy(TS_Entries[TSE_Count].Label,LastLabel);
	TS_Entries[TSE_Count].TimeTotal  = ((unsigned __int64)StopTimeHi << 32) + (unsigned __int64)StopTimeLo;
	TS_Entries[TSE_Count].TimeTotal -= ((unsigned __int64)StartTimeHi << 32) + (unsigned __int64)StartTimeLo;
	TSE_Count +=1;
}

void GenerateTimerResults (void)
{
	char buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT], LogFileName[_MAX_PATH];
	DWORD dwWritten, count, count2;
	HANDLE hLogFile = NULL;
	__int64 TotalTime;

	StopTimer();

	GetModuleFileName(NULL,buffer,sizeof(buffer));
	_splitpath( buffer, drive, dir, fname, ext );
   	_makepath( LogFileName, drive, dir, "RSP Profiling", "log" );

	hLogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);

	for (count = 0; count < TSE_Count; count ++)
	{
		for (count2 = 0; count2 < (TSE_Count - 1); count2 ++)
		{
			if (TS_Entries[count2].TimeTotal < TS_Entries[count2 + 1].TimeTotal)
			{
				TIME_STAMP_ENTRY Temp;
				memcpy(&Temp,&TS_Entries[count2],sizeof(TIME_STAMP_ENTRY));
				memcpy(&TS_Entries[count2],&TS_Entries[count2 + 1],sizeof(TIME_STAMP_ENTRY));
				memcpy(&TS_Entries[count2 + 1],&Temp,sizeof(TIME_STAMP_ENTRY));
			}
		}
	}
	TotalTime = 0;
	for (count = 0; count < TSE_Count; count ++)
	{
		TotalTime += TS_Entries[count].TimeTotal;
	}
	for (count = 0; count < (TSE_Count < 50?TSE_Count:50); count ++)
	{
		sprintf(buffer,"%s - %0.2f%c\r\n",
			TS_Entries[count].Label,
			(((double)TS_Entries[count].TimeTotal / (double)TotalTime) * 100),'%'
		);
		WriteFile( hLogFile,buffer,strlen(buffer),&dwWritten,NULL );
	}
	CloseHandle(hLogFile);
	ResetTimerList();
}

#endif
