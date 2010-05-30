#include "main.h"
#include "CPU.h"
#include "debugger.h"

//int NextInstruction, ManualPaused;
//int DlistCount, AlistCount;
//SYSTEM_TIMERS Timers;
//DWORD MemoryStack;
DWORD JumpToLocation;

//R4300iOp_FUNC * R4300i_Opcode;

void InitializeCPUCore ( void ) 
{
	LARGE_INTEGER PerformanceFrequency; 
	
	//R4300i_Opcode = R4300iOp::BuildInterpreter();
	//R4300i_Opcode = R4300iOp32::BuildInterpreter();
	CurrentFrame = 0;

	QueryPerformanceFrequency(&PerformanceFrequency);
	Frequency = PerformanceFrequency.QuadPart;

	{
		switch (_Rom->GetCountry())
		{
			case Germany: case french:  case Italian:
			case Europe:  case Spanish: case Australia:
			case X_PAL:   case Y_PAL:
				Timer_Initialize(50);
				g_SystemType = SYSTEM_PAL;
				break;
			default:
				Timer_Initialize(60);
				g_SystemType = SYSTEM_NTSC;
				break;
		}
	}
#ifndef EXTERNAL_RELEASE
	LogOptions.GenerateLog = g_GenerateLog;
	LoadLogOptions(&LogOptions, FALSE);
	StartLog();
#endif

//	switch (CPU_Type) {
//	case CPU_Interpreter: StartInterpreterCPU(); break;
//	case CPU_Recompiler: StartRecompilerCPU();	break;
//	case CPU_SyncCores: StartRecompilerCPU();	 break;
//	default:
//		DisplayError("Unhandled CPU %d",CPU_Type);
//	}
//	if (TargetInfo)
//	{
//		VirtualFree(TargetInfo,0,MEM_RELEASE);
//		TargetInfo = NULL;
//	}

}

#ifdef toremove
void ChangeCompareTimer(void) {
	DWORD NextCompare = _Reg->COMPARE_REGISTER - _Reg->COUNT_REGISTER;
	if ((NextCompare & 0x80000000) != 0) {  NextCompare = 0x7FFFFFFF; }
	if (NextCompare == 0) { NextCompare = 0x1; }	
	_SystemTimer->SetTimer(CSystemTimer::CompareTimer,NextCompare,false);
}
#endif

void CheckTimer (void) {
	BreakPoint(__FILE__,__LINE__);
/*	int count;

	for (count = 0; count < MaxTimers; count++) {
		if (!Timers.Active[count]) { continue; }
		if (!(count == CompareTimer && Timers.NextTimer[count] == 0x7FFFFFFF)) {
			Timers.NextTimer[count] += Timers.Timer;
		}
	}
	Timers.CurrentTimerType = -1;
	Timers.Timer = 0x7FFFFFFF;
	for (count = 0; count < MaxTimers; count++) {
		if (!Timers.Active[count]) { continue; }
		if (Timers.NextTimer[count] >= Timers.Timer) { continue; }
		Timers.Timer = Timers.NextTimer[count];
		Timers.CurrentTimerType = count;
	}
	if (Timers.CurrentTimerType == -1) {
		DisplayError("No active timers ???\nEmulation Stoped");
		ExitThread(0);
	}
	for (count = 0; count < MaxTimers; count++) {
		if (!Timers.Active[count]) { continue; }
		if (!(count == CompareTimer && Timers.NextTimer[count] == 0x7FFFFFFF)) {
			Timers.NextTimer[count] -= Timers.Timer;
		}
	}
	
	if (Timers.NextTimer[CompareTimer] == 0x7FFFFFFF) {
		DWORD NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
		if ((NextCompare & 0x80000000) == 0 && NextCompare != 0x7FFFFFFF) {
			ChangeCompareTimer();
		}
	}*/
}

/*void RefreshScreen (void ){ 
	static DWORD OLD_VI_V_SYNC_REG = 0, VI_INTR_TIME = 500000;
	LARGE_INTEGER Time;
	char Label[100];

	if (Profiling || ShowCPUPer) { memcpy(Label,ProfilingLabel,sizeof(Label)); }
	if (Profiling) { StartTimer("RefreshScreen"); }

	if (OLD_VI_V_SYNC_REG != VI_V_SYNC_REG) {
		if (VI_V_SYNC_REG == 0) {
			VI_INTR_TIME = 500000;
		} else {
			VI_INTR_TIME = (VI_V_SYNC_REG + 1) * 1500;
			if ((VI_V_SYNC_REG % 1) != 0) {
				VI_INTR_TIME -= 38;
			}
		}
	}
	ChangeTimerRelative(ViTimer,VI_INTR_TIME);
	if (g_FixedAudio)
	{
		UpdateAudioTimer (VI_INTR_TIME);	
	}

	if ((VI_STATUS_REG & 0x10) != 0) {
		if (ViFieldNumber == 0) {
			ViFieldNumber = 1;
		} else {
			ViFieldNumber = 0;
		}
	} else {
		ViFieldNumber = 0;
	}
	
	if (ShowCPUPer || Profiling) { StartTimer("CPU Idel"); }
	if (Limit_FPS()) {	Timer_Process(NULL); }
	if (ShowCPUPer || Profiling) { StopTimer(); }
	if (Profiling) { StartTimer("RefreshScreen: Update FPS"); }
	if ((CurrentFrame & 7) == 0) {
		//Disables Screen saver
		//mouse_event(MOUSEEVENTF_MOVE,1,1,0,GetMessageExtraInfo());
		//mouse_event(MOUSEEVENTF_MOVE,-1,-1,0,GetMessageExtraInfo());

		QueryPerformanceCounter(&Time);
		Frames[(CurrentFrame >> 3) % NoOfFrames] = Time.QuadPart - LastFrame;
		LastFrame = Time.QuadPart;	
		DisplayFPS();
	}
	if (Profiling) { StopTimer(); }
	if (ShowCPUPer) { DisplayCPUPer(); }
	CurrentFrame += 1;

	if (Profiling) { StartTimer("RefreshScreen: Update Screen"); }
	__try {
		if (UpdateScreen != NULL) 
		{
			UpdateScreen(); 
		}
	} __except( r4300i_CPU_MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		DisplayError("Unknown memory action in trying to update the screen\n\nEmulation stop");
		ExitThread(0);
	}
	if (Profiling) { StartTimer("RefreshScreen: Cheats"); }
	if ((STATUS_REGISTER & STATUS_IE) != 0 ) { ApplyCheats(); }
	if (Profiling || ShowCPUPer) { StartTimer(Label); }
}

void RunRsp (void) {
	if ( ( SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
		if ( ( SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {
			DWORD Task = *( DWORD *)(DMEM + 0xFC0);

			if (Task == 1 && (DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0) 
			{
				return;
			}
			
			switch (Task) {
			case 1:  
				DlistCount += 1; 
				/*if ((DlistCount % 2) == 0) { 
					SP_STATUS_REG |= (0x0203 );
					MI_INTR_REG |= MI_INTR_SP | MI_INTR_DP;
					CheckInterrupts();
					return; 
				}*/
/*				break;
			case 2:  
				AlistCount += 1; 
				break;
			}

			if (ShowDListAListCount) {
				DisplayMessage("Dlist: %d   Alist: %d",DlistCount,AlistCount);
			}
			if (Profiling || DisplayCPUPer) {
				char Label[100];

				strncpy(Label,ProfilingLabel,sizeof(Label));

				if (IndvidualBlock && !DisplayCPUPer) {
					StartTimer("RSP");
				} else {
					switch (*( DWORD *)(DMEM + 0xFC0)) {
					case 1:  StartTimer("RSP: Dlist"); break;
					case 2:  StartTimer("RSP: Alist"); break;
					default: StartTimer("RSP: Unknown"); break;
					}
				}
				DoRspCycles(100);
				StartTimer(Label); 
			} else {
				DoRspCycles(100);
			}
#ifdef CFB_READ
			if (VI_ORIGIN_REG > 0x280) {
				SetFrameBuffer(VI_ORIGIN_REG, (DWORD)(VI_WIDTH_REG * (VI_WIDTH_REG *.75)));
			}
#endif
		} 
	}
}
*/
void DisplayFPS (void) {
	if (CurrentFrame > (NoOfFrames << 3)) {
		__int64 Total = 0;
		int count;
		
		for (count = 0; count < NoOfFrames; count ++) {
			Total += Frames[count];
		}
		DisplayMessage2("FPS: %.2f", (__int64)Frequency/ ((double)Total / (NoOfFrames << 3)));
	} else {
		DisplayMessage2("FPS: -.--" );
	}
}

void CloseCpu (void) {
//	DWORD ExitCode, count, OldProtect;
//	
//	if (!CPURunning) { return; }
//	ManualPaused = FALSE;
//	if (CPU_Paused) { PauseCpu (); Sleep(1000); }
//	
//	{
//		BOOL Temp = AlwaysOnTop;
//		AlwaysOnTop = FALSE;
//		AlwaysOnTopWindow(hMainWindow);
//		AlwaysOnTop = Temp;
//	}
//
//	for (count = 0; count < 20; count ++ ) {
//		CPU_Action.CloseCPU = TRUE;
//		CPU_Action.Stepping = FALSE;
//		CPU_Action.DoSomething = TRUE;
//		PulseEvent( CPU_Action.hStepping );
//		Sleep(100);
//		GetExitCodeThread(hCPU,&ExitCode);
//		if (ExitCode != STILL_ACTIVE) {
//			hCPU = NULL;
//			count = 100;
//		}
//	}
//	if (hCPU != NULL) {  TerminateThread(hCPU,0); hCPU = NULL; }
//	CPURunning = FALSE;
//	VirtualProtect(N64MEM,RdramSize,PAGE_READWRITE,&OldProtect);
//	VirtualProtect(N64MEM + 0x04000000,0x2000,PAGE_READWRITE,&OldProtect);
//	Timer_Stop();
//	SetCurrentSaveState(hMainWindow,ID_CURRENTSAVE_DEFAULT);
//	CloseEeprom();
//	CloseMempak();
//	CloseSram();
//	FreeSyncMemory();
//	if (GfxRomClosed != NULL)  { GfxRomClosed(); }
//	if (AiRomClosed != NULL)   { AiRomClosed(); }
//	if (ContRomClosed != NULL) { ContRomClosed(); }
//	if (RSPRomClosed) { RSPRomClosed(); }
//	if (Profiling) { GenerateTimerResults(); }
//	CloseHandle(CPU_Action.hStepping);
//	SendMessage( hStatusWnd, SB_SETTEXT, 0, (LPARAM)GS(MSG_EMULATION_ENDED) );
}

