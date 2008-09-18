#include "main.h"
#include "CPU.h"
#include "debugger.h"

//int NextInstruction, ManualPaused;
//int DlistCount, AlistCount;
//SYSTEM_TIMERS Timers;
//DWORD MemoryStack;
DWORD JumpToLocation;

void InitializeCPUCore ( void ) 
{
	LARGE_INTEGER PerformanceFrequency; 
	
	BuildInterpreter();
	CurrentFrame = 0;

	QueryPerformanceFrequency(&PerformanceFrequency);
	Frequency = PerformanceFrequency.QuadPart;

	{
		BYTE Country = *(ROM + 0x3D);
		switch (Country)
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

int DelaySlotEffectsJump (DWORD JumpPC) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(JumpPC, &Command.Hex)) { return TRUE; }

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_JR:	return DelaySlotEffectsCompare(JumpPC,Command.rs,0);
		case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC,Command.rs,31);
		}
		break;
	case R4300i_REGIMM:
		switch (Command.rt) {
		case R4300i_REGIMM_BLTZ:
		case R4300i_REGIMM_BGEZ:
		case R4300i_REGIMM_BLTZL:
		case R4300i_REGIMM_BGEZL:
		case R4300i_REGIMM_BLTZAL:
		case R4300i_REGIMM_BGEZAL:
			return DelaySlotEffectsCompare(JumpPC,Command.rs,0);
		}
		break;
	case R4300i_JAL: 
	case R4300i_SPECIAL_JALR: return DelaySlotEffectsCompare(JumpPC,31,0); break;
	case R4300i_J: return FALSE;
	case R4300i_BEQ: 
	case R4300i_BNE: 
	case R4300i_BLEZ: 
	case R4300i_BGTZ: 
		return DelaySlotEffectsCompare(JumpPC,Command.rs,Command.rt);
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_BC:
			switch (Command.ft) {
			case R4300i_COP1_BC_BCF:
			case R4300i_COP1_BC_BCT:
			case R4300i_COP1_BC_BCFL:
			case R4300i_COP1_BC_BCTL:
				{
					int EffectDelaySlot;
					OPCODE NewCommand;

					if (!r4300i_LW_VAddr(JumpPC + 4, &NewCommand.Hex)) { return TRUE; }
					
					EffectDelaySlot = FALSE;
					if (NewCommand.op == R4300i_CP1) {
						if (NewCommand.fmt == R4300i_COP1_S && (NewCommand.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
						if (NewCommand.fmt == R4300i_COP1_D && (NewCommand.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
					}
					return EffectDelaySlot;
				} 
				break;
			}
			break;
		}
		break;
	case R4300i_BEQL: 
	case R4300i_BNEL: 
	case R4300i_BLEZL: 
	case R4300i_BGTZL: 
		return DelaySlotEffectsCompare(JumpPC,Command.rs,Command.rt);
	}
	return TRUE;
}

void DoSomething ( void ) {
	if (CPU_Action.CloseCPU) { 
		return;
	}
	
	if (CPU_Action.SoftReset)
	{
		CPU_Action.SoftReset = false;

		ChangeTimer(SoftResetTimer,0x3000000);
		ShowCFB();
		FAKE_CAUSE_REGISTER |= CAUSE_IP4;
		CheckInterrupts();
		g_Plugins->Gfx()->SoftReset();
	}

	if (CPU_Action.GenerateInterrupt)
	{
		CPU_Action.GenerateInterrupt = FALSE;
		MI_INTR_REG |= CPU_Action.InterruptFlag;
		CPU_Action.InterruptFlag = 0;
		CheckInterrupts();
	}
	if (CPU_Action.CheckInterrupts) {
		CPU_Action.CheckInterrupts = FALSE;
		CheckInterrupts();
	}
	if (CPU_Action.ProfileStartStop) {
		CPU_Action.ProfileStartStop = FALSE;
		ResetTimer();
	}
	if (CPU_Action.ProfileResetStats) {
		CPU_Action.ProfileResetStats = FALSE;
		ResetTimer();
	}
	if (CPU_Action.ProfileGenerateLogs) {
		CPU_Action.ProfileGenerateLogs = FALSE;
		GenerateProfileLog();
	}

	if (CPU_Action.DoInterrupt) {
		CPU_Action.DoInterrupt = FALSE;
		if (DoIntrException(FALSE) && !CPU_Action.InterruptExecuted)
		{
			CPU_Action.InterruptExecuted = TRUE;
			ClearRecompCodeInitialCode();
		}
	}

	if (CPU_Action.ChangeWindow) {
		CPU_Action.ChangeWindow = FALSE;
		ChangeFullScreenFunc();
	}

	if (CPU_Action.Pause) {
		PauseExecution();
		CPU_Action.Pause = FALSE;
	}
	if (CPU_Action.ChangePlugin) {
		ChangePluginFunc();
		CPU_Action.ChangePlugin = FALSE;
	}
	if (CPU_Action.GSButton) {
		ApplyGSButtonCheats();
		CPU_Action.GSButton = FALSE;
	}

	CPU_Action.DoSomething = FALSE;
	
	if (CPU_Action.SaveState) {
		//test if allowed
		CPU_Action.SaveState = FALSE;
		if (!Machine_SaveState()) {
			CPU_Action.SaveState = TRUE;
			CPU_Action.DoSomething = TRUE;
		}
	}
	if (CPU_Action.RestoreState) {
		CPU_Action.RestoreState = FALSE;
		Machine_LoadState();
	}
	if (CPU_Action.DoInterrupt == TRUE) { CPU_Action.DoSomething = TRUE; }
}

void TimerDone (void) {
	DWORD LastTimer;
	if (Profiling) { 
		LastTimer = StartTimer(Timer_Done); 
	}
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.GenerateLog && LogOptions.LogExceptions && !LogOptions.NoInterrupts) {
		LogMessage("%08X: Timer Done (Type: %d CurrentTimer: %d)", PROGRAM_COUNTER, *g_CurrentTimerType, *g_Timer );
	}
#endif

	switch (*g_CurrentTimerType) {
	case CompareTimer:
		FAKE_CAUSE_REGISTER |= CAUSE_IP7;
		CheckInterrupts();
		ChangeCompareTimer();
		break;
	case SoftResetTimer:
		ChangeTimer(SoftResetTimer,0);
		g_N64System->SoftReset();
		break;
	case SiTimer:
		ChangeTimer(SiTimer,0);
		MI_INTR_REG |= MI_INTR_SI;
		SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		CheckInterrupts();
		break;
	case PiTimer:
		ChangeTimer(PiTimer,0);
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts();
		break;
	case ViTimer:
		RefreshScreen();
		MI_INTR_REG |= MI_INTR_VI;
		CheckInterrupts();
		break;
	case RspTimer:
		ChangeTimer(RspTimer,0);
		RunRsp();
		break;
	case AiTimer:
		ChangeTimer(AiTimer,0);
		MI_INTR_REG |= MI_INTR_AI;
		CheckInterrupts();
		g_Audio->AiCallBack();
		break;
	default:
		BreakPoint(__FILE__,__LINE__);
	}
	//CheckTimer();
	if (Profiling) { 
		StartTimer(LastTimer); 
	}
}

void InPermLoop (void) {
	// *** Changed ***/
	if (CPU_Action.DoInterrupt) 
	{
		CPU_Action.DoSomething = TRUE;
		return; 
	}
	
	//if (CPU_Type == CPU_SyncCores) { SyncRegisters.CP0[9] +=5; }

	/* Interrupts enabled */
	if (( STATUS_REGISTER & STATUS_IE  ) == 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_EXL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & STATUS_ERL ) != 0 ) { goto InterruptsDisabled; }
	if (( STATUS_REGISTER & 0xFF00) == 0) { goto InterruptsDisabled; }
	
	/* check sound playing */
	g_N64System->SyncToAudio();
	
	/* check RSP running */
	/* check RDP running */

	if (*g_Timer > 0) {
		COUNT_REGISTER += *g_Timer + 1;
		//if (CPU_Type == CPU_SyncCores) { SyncRegisters.CP0[9] += Timers.Timer + 1; }
		*g_Timer = -1;
	}
	return;

InterruptsDisabled:
	if (UpdateScreen != NULL) { UpdateScreen(); }
	//CurrentFrame = 0;
	//CurrentPercent = 0;
	//DisplayFPS();
	DisplayError(GS(MSG_PERM_LOOP));
	StopEmulation();

}

int DelaySlotEffectsCompare (DWORD PC, DWORD Reg1, DWORD Reg2) {
	OPCODE Command;

	if (!r4300i_LW_VAddr(PC + 4, &Command.Hex)) {
		//DisplayError("Failed to load word 2");
		//ExitThread(0);
		return TRUE;
	}

	switch (Command.op) {
	case R4300i_SPECIAL:
		switch (Command.funct) {
		case R4300i_SPECIAL_SLL:
		case R4300i_SPECIAL_SRL:
		case R4300i_SPECIAL_SRA:
		case R4300i_SPECIAL_SLLV:
		case R4300i_SPECIAL_SRLV:
		case R4300i_SPECIAL_SRAV:
		case R4300i_SPECIAL_MFHI:
		case R4300i_SPECIAL_MTHI:
		case R4300i_SPECIAL_MFLO:
		case R4300i_SPECIAL_MTLO:
		case R4300i_SPECIAL_DSLLV:
		case R4300i_SPECIAL_DSRLV:
		case R4300i_SPECIAL_DSRAV:
		case R4300i_SPECIAL_ADD:
		case R4300i_SPECIAL_ADDU:
		case R4300i_SPECIAL_SUB:
		case R4300i_SPECIAL_SUBU:
		case R4300i_SPECIAL_AND:
		case R4300i_SPECIAL_OR:
		case R4300i_SPECIAL_XOR:
		case R4300i_SPECIAL_NOR:
		case R4300i_SPECIAL_SLT:
		case R4300i_SPECIAL_SLTU:
		case R4300i_SPECIAL_DADD:
		case R4300i_SPECIAL_DADDU:
		case R4300i_SPECIAL_DSUB:
		case R4300i_SPECIAL_DSUBU:
		case R4300i_SPECIAL_DSLL:
		case R4300i_SPECIAL_DSRL:
		case R4300i_SPECIAL_DSRA:
		case R4300i_SPECIAL_DSLL32:
		case R4300i_SPECIAL_DSRL32:
		case R4300i_SPECIAL_DSRA32:
			if (Command.rd == 0) { return FALSE; }
			if (Command.rd == Reg1) { return TRUE; }
			if (Command.rd == Reg2) { return TRUE; }
			break;
		case R4300i_SPECIAL_MULT:
		case R4300i_SPECIAL_MULTU:
		case R4300i_SPECIAL_DIV:
		case R4300i_SPECIAL_DIVU:
		case R4300i_SPECIAL_DMULT:
		case R4300i_SPECIAL_DMULTU:
		case R4300i_SPECIAL_DDIV:
		case R4300i_SPECIAL_DDIVU:
			break;
		default:
#ifndef EXTERNAL_RELEASE
			DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
			return TRUE;
		}
		break;
	case R4300i_CP0:
		switch (Command.rs) {
		case R4300i_COP0_MT: break;
		case R4300i_COP0_MF:
			if (Command.rt == 0) { return FALSE; }
			if (Command.rt == Reg1) { return TRUE; }
			if (Command.rt == Reg2) { return TRUE; }
			break;
		default:
			if ( (Command.rs & 0x10 ) != 0 ) {
				switch( Opcode.funct ) {
				case R4300i_COP0_CO_TLBR: break;
				case R4300i_COP0_CO_TLBWI: break;
				case R4300i_COP0_CO_TLBWR: break;
				case R4300i_COP0_CO_TLBP: break;
				default: 
#ifndef EXTERNAL_RELEASE
					DisplayError("Does %s effect Delay slot at %X?\n6",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
					return TRUE;
				}
			} else {
#ifndef EXTERNAL_RELEASE
				DisplayError("Does %s effect Delay slot at %X?\n7",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
				return TRUE;
			}
		}
		break;
	case R4300i_CP1:
		switch (Command.fmt) {
		case R4300i_COP1_MF:
			if (Command.rt == 0) { return FALSE; }
			if (Command.rt == Reg1) { return TRUE; }
			if (Command.rt == Reg2) { return TRUE; }
			break;
		case R4300i_COP1_CF: break;
		case R4300i_COP1_MT: break;
		case R4300i_COP1_CT: break;
		case R4300i_COP1_S: break;
		case R4300i_COP1_D: break;
		case R4300i_COP1_W: break;
		case R4300i_COP1_L: break;
#ifndef EXTERNAL_RELEASE
		default:
			DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
			return TRUE;
		}
		break;
	case R4300i_ANDI:
	case R4300i_ORI:
	case R4300i_XORI:
	case R4300i_LUI:
	case R4300i_ADDI:
	case R4300i_ADDIU:
	case R4300i_SLTI:
	case R4300i_SLTIU:
	case R4300i_DADDI:
	case R4300i_DADDIU:
	case R4300i_LB:
	case R4300i_LH:
	case R4300i_LW:
	case R4300i_LWL:
	case R4300i_LWR:
	case R4300i_LDL:
	case R4300i_LDR:
	case R4300i_LBU:
	case R4300i_LHU:
	case R4300i_LD:
	case R4300i_LWC1:
	case R4300i_LDC1:
		if (Command.rt == 0) { return FALSE; }
		if (Command.rt == Reg1) { return TRUE; }
		if (Command.rt == Reg2) { return TRUE; }
		break;
	case R4300i_CACHE: break;
	case R4300i_SB: break;
	case R4300i_SH: break;
	case R4300i_SW: break;
	case R4300i_SWR: break;
	case R4300i_SWL: break;
	case R4300i_SWC1: break;
	case R4300i_SDC1: break;
	case R4300i_SD: break;
	default:
#ifndef EXTERNAL_RELEASE
		DisplayError("Does %s effect Delay slot at %X?",R4300iOpcodeName(Command.Hex,PC+4), PC);
#endif
		return TRUE;
	}
	return FALSE;
}

void ChangeCompareTimer(void) {
	DWORD NextCompare = COMPARE_REGISTER - COUNT_REGISTER;
	if ((NextCompare & 0x80000000) != 0) {  NextCompare = 0x7FFFFFFF; }
	if (NextCompare == 0) { NextCompare = 0x1; }	
	ChangeTimer(CompareTimer,NextCompare);
}

//void ChangeTimer(enum TimerType Type, int Value) {
//		_Reg->ChangeTimerFixed(CompareTimer,COMPARE_REGISTER - COUNT_REGISTER); 

	/*if (Value == 0) { 
		Timers.NextTimer[Type] = 0;
		Timers.Active[Type] = FALSE; 
		return;
	}
	Timers.NextTimer[Type] = Value - Timers.Timer;
	Timers.Active[Type] = TRUE;
	CheckTimer();*/
//	_asm int 3
//}

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

