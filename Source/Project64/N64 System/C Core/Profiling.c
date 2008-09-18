/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "main.h"

typedef struct {
	char Label[100];
	_int64 TimeTotal;
} TIME_STAMP_ENTRY;

DWORD StartTimeHi, StartTimeLo, StopTimeHi, StopTimeLo, TSE_Count, TSE_Max;
TIME_STAMP_ENTRY * TS_Entries = NULL;
char ProfilingLabel[100];

float CPUPercent[3][15];
int CurrentPercent = 0;

void ResetTimerList (void) {
	if (TS_Entries) { free(TS_Entries); }
	TS_Entries = NULL;
	TSE_Count = 0;
	TSE_Max = 0;
}

void SortTimerList(void) {
	BOOL Changed;
	DWORD i, n;

	for (i = 1; i < TSE_Count; i++) {
		Changed = FALSE;
		for (n = 0; n < TSE_Count - i; n++) {
			if (strcmp(TS_Entries[n].Label, TS_Entries[n+1].Label) > 0) {
				TIME_STAMP_ENTRY TempEntry;
				
				memcpy(&TempEntry,&TS_Entries[n],sizeof(TIME_STAMP_ENTRY));
				memcpy(&TS_Entries[n],&TS_Entries[n+1],sizeof(TIME_STAMP_ENTRY));
				memcpy(&TS_Entries[n + 1],&TempEntry,sizeof(TIME_STAMP_ENTRY));
				Changed = TRUE;
			}
		}
		if (!Changed) { return; }
	}  
}

int SearchTimerList(char * Key) {
	int low = 0;
	int high = TSE_Count -1;
	int middle;
	int cmp;

	while (low <= high) {
		middle = (low + high)/2;
		cmp = strcmp(Key,TS_Entries[middle].Label);
		if (cmp == 0) { return middle; }
		if (cmp > 0) {
			low = middle + 1;
		} else {
			high = middle - 1;
		}      
	}    
	return TSE_Count;
}

void StartTimer (char * Label) {
	if (Label == NULL) { BreakPoint(__FILE__,__LINE__);  }
	StopTimer();
	strcpy(ProfilingLabel,Label);
	_asm {
		pushad
		rdtsc
		mov StartTimeHi, edx
		mov StartTimeLo, eax
		popad
	}
}

void StopTimer (void) {
	DWORD count;

	_asm {
		pushad
		rdtsc
		mov StopTimeHi, edx
		mov StopTimeLo, eax
		popad
	}
	
	if (strlen(ProfilingLabel) == 0) { return; }	
	count = SearchTimerList(ProfilingLabel);
	if (count < TSE_Count) {
		_int64 Time = ((unsigned _int64)StopTimeHi << 32) + (unsigned _int64)StopTimeLo;
		Time -= ((unsigned _int64)StartTimeHi << 32) + (unsigned _int64)StartTimeLo;
		TS_Entries[count].TimeTotal += Time;
		//LogMessage("%-30s: time = %X",ProfilingLabel,Time);
		memset(ProfilingLabel,0,sizeof(ProfilingLabel));
		return;
	}
	if (TSE_Count == 0) {
		TS_Entries = (TIME_STAMP_ENTRY *)malloc(sizeof(TIME_STAMP_ENTRY) * 100);
		if (TS_Entries == NULL) {
			MessageBox(NULL,"TIME_STAMP_ENTRY == NULL ??",GS(MSG_MSGBOX_TITLE),MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
		}
		TSE_Max = 100;
	} else if (TSE_Count == TSE_Max) {
		TSE_Max += 100;
		TS_Entries = (TIME_STAMP_ENTRY *)realloc(TS_Entries,sizeof(TIME_STAMP_ENTRY) * TSE_Max);
		if (TS_Entries == NULL) {
			MessageBox(NULL,"TIME_STAMP_ENTRY == NULL ??",GS(MSG_MSGBOX_TITLE),MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
		}
	}
	strcpy(TS_Entries[TSE_Count].Label,ProfilingLabel);
	TS_Entries[TSE_Count].TimeTotal  = ((unsigned _int64)StopTimeHi << 32) + (unsigned _int64)StopTimeLo;
	TS_Entries[TSE_Count].TimeTotal -= ((unsigned _int64)StartTimeHi << 32) + (unsigned _int64)StartTimeLo;
	TSE_Count +=1;
	memset(ProfilingLabel,0,sizeof(ProfilingLabel));
	SortTimerList();
}

void GenerateTimerResults (void) {
	char buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT], LogFileName[_MAX_PATH];
	DWORD dwWritten, count, count2;
	HANDLE hLogFile = NULL;
	_int64 TotalTime;

	StopTimer();

	GetModuleFileName(NULL,buffer,sizeof(buffer));
	_splitpath( buffer, drive, dir, fname, ext );
   	_makepath( LogFileName, drive, dir, "Profiling", "log" );

	hLogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);
	
	for (count = 0; count < TSE_Count; count ++) {
		for (count2 = 0; count2 < (TSE_Count - 1); count2 ++) {
			if (TS_Entries[count2].TimeTotal < TS_Entries[count2 + 1].TimeTotal) {
				TIME_STAMP_ENTRY Temp;
				memcpy(&Temp,&TS_Entries[count2],sizeof(TIME_STAMP_ENTRY));
				memcpy(&TS_Entries[count2],&TS_Entries[count2 + 1],sizeof(TIME_STAMP_ENTRY));
				memcpy(&TS_Entries[count2 + 1],&Temp,sizeof(TIME_STAMP_ENTRY));
			}
		}
	}
	TotalTime = 0;
	for (count = 0; count < TSE_Count; count ++) {
		TotalTime += TS_Entries[count].TimeTotal;
	}
	for (count = 0; count < (TSE_Count < 200?TSE_Count:200); count ++) {
		sprintf(buffer,"%-30s %2.2f%c\r\n",
			TS_Entries[count].Label,
			(((double)TS_Entries[count].TimeTotal / (double)TotalTime) * 100),'%'
		);
		WriteFile( hLogFile,buffer,strlen(buffer),&dwWritten,NULL );
	}
	CloseHandle(hLogFile);
	ResetTimerList();
}

void DisplayCPUPer (void) {
	__int64 TotalTime, CPU = 0, Alist = 0, Dlist = 0, Idle = 0;
	DWORD count;

	//CurrentPercent = (CurrentPercent & 0xF) + 1;
	CurrentPercent += 1;
	if (CurrentPercent < 30) { return; }
	CurrentPercent = 0;

	for (count = 0; count < TSE_Count; count ++) {
		if (strcmp(TS_Entries[count].Label,"r4300i Running") == 0) { CPU   = TS_Entries[count].TimeTotal; }
		if (strcmp(TS_Entries[count].Label,"RSP: Dlist"    ) == 0) { Dlist = TS_Entries[count].TimeTotal; }
		if (strcmp(TS_Entries[count].Label,"RSP: Alist"    ) == 0) { Alist = TS_Entries[count].TimeTotal; }
		if (strcmp(TS_Entries[count].Label,"CPU Idel"      ) == 0) { Idle  = TS_Entries[count].TimeTotal; }
	}
	
	TotalTime = 0;
	for (count = 0; count < TSE_Count; count ++) {
		TotalTime += TS_Entries[count].TimeTotal;
	}
	DisplayMessage("r4300i: %0.1f%c    Dlist: %0.1f%c    Alist: %0.1f%c    Idle: %0.1f%c",
		(float)(((double)CPU / (double)TotalTime) * 100),'%',
		(float)(((double)Dlist / (double)TotalTime) * 100),'%',
		(float)(((double)Alist / (double)TotalTime) * 100),'%',
		(float)(((double)Idle / (double)TotalTime) * 100),'%');
	ResetTimerList();
}
