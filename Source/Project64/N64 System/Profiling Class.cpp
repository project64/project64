#include "..\N64 System.h"

#include <Windows.h>

enum { MAX_FRAMES = 13 };

CProfiling::CProfiling (CNotification * Notify) {
	_Notify = Notify;
	CurrentTimerAddr = Timer_None;
	CurrentDisplayCount = MAX_FRAMES;
}

DWORD CProfiling::StartTimer(DWORD Address) {
	DWORD OldTimerAddr = StopTimer();
	CurrentTimerAddr = Address;

	DWORD HiValue, LoValue;
	_asm {
		pushad
		rdtsc
		mov HiValue, edx
		mov LoValue, eax
		popad
	}
	StartTimeHi = HiValue;
	StartTimeLo = LoValue;
	return OldTimerAddr;
}

DWORD CProfiling::StopTimer(void) {
	DWORD HiValue, LoValue;
	
	if (CurrentTimerAddr == Timer_None) { return CurrentTimerAddr; }

	_asm {
		pushad
		rdtsc
		mov HiValue, edx
		mov LoValue, eax
		popad
	}

	__int64 StopTime  = ((unsigned __int64)HiValue << 32) + (unsigned __int64)LoValue;
	__int64 StartTime = ((unsigned __int64)StartTimeHi << 32) + (unsigned __int64)StartTimeLo;
	__int64 TimeTaken = StopTime - StartTime;
	
	PROFILE_ENRTY Entry = Entries.find(CurrentTimerAddr);
	if (Entry != Entries.end()) {
		Entry->second += TimeTaken;
	} else {
		Entries.insert(PROFILE_ENRTIES::value_type(CurrentTimerAddr,TimeTaken));
	}

	DWORD OldTimerAddr = CurrentTimerAddr;
	CurrentTimerAddr = Timer_None;
	return OldTimerAddr;
}

void CProfiling::ShowCPU_Usage (void) {
	__int64 TotalTime, CPU = 0, Alist = 0, Dlist = 0, Idle = 0;
	PROFILE_ENRTY Entry;
	
	if (CurrentDisplayCount > 0) { CurrentDisplayCount -= 1; return;  }
	CurrentDisplayCount = MAX_FRAMES;

	Entry = Entries.find(Timer_R4300);
	if (Entry != Entries.end()) { CPU = Entry->second; }

	Entry = Entries.find(Timer_RefreshScreen);
	if (Entry != Entries.end()) { CPU += Entry->second; }

	Entry = Entries.find(Timer_RSP_Dlist);
	if (Entry != Entries.end()) { Dlist = Entry->second; }
	
	Entry = Entries.find(Timer_UpdateScreen);
	if (Entry != Entries.end()) { Dlist += Entry->second; }

	Entry = Entries.find(Timer_RSP_Alist);
	if (Entry != Entries.end()) { Alist = Entry->second; }

	Entry = Entries.find(Timer_Idel);
	if (Entry != Entries.end()) { Idle = Entry->second; }


	TotalTime = CPU + Alist + Dlist + Idle;

	_Notify->DisplayMessage(0,"r4300i: %0.1f%c    GFX: %0.1f%c    Alist: %0.1f%c    Idle: %0.1f%c",
		(float)(((double)CPU / (double)TotalTime) * 100),'%',
		(float)(((double)Dlist / (double)TotalTime) * 100),'%',
		(float)(((double)Alist / (double)TotalTime) * 100),'%',
		(float)(((double)Idle / (double)TotalTime) * 100),'%');

	ResetCounters();
}

void CProfiling::ResetCounters (void) {
	Entries.clear();
}

typedef struct { SPECIAL_TIMERS Timer; char * Name; } TIMER_NAME;

void CProfiling::GenerateLog(void) {
	stdstr LogFileName;
	{
		CLog Log;
		Log.Open("Profiling.txt");
		LogFileName = Log.FileName();

		//Get the total time
		__int64 TotalTime = 0;
		for (PROFILE_ENRTY itemTime = Entries.begin(); itemTime != Entries.end(); itemTime++ ) {
			TotalTime += itemTime->second;
		}

		//Create a sortable list of items
		std::vector<PROFILE_VALUE *> ItemList;
		for (PROFILE_ENRTY Entry = Entries.begin(); Entry != Entries.end(); Entry++ ) {
			ItemList.push_back(&(*Entry));
		}
		
		//sort the list with a basic bubble sort
		for (int OuterPass = 0; OuterPass < (ItemList.size() - 1); OuterPass++ ) {
			for (int InnerPass = 0; InnerPass < (ItemList.size() - 1); InnerPass++ ) {
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
		
		for (int count =0; count < ItemList.size(); count++ ) {
			char Buffer[255];
			float CpuUsage = ((double)ItemList[count]->second / (double)TotalTime) * 100;
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
