#include "..\..\N64 System.h"

extern CLog TlbLog;

CSystemTimer::CSystemTimer(CN64System * System, CNotification * Notify) :
	_System(System),
	_Notify(Notify)
{
	ResetTimer(50000);
}

void CSystemTimer::ChangeTimerFixed (TimerType Type, DWORD Cycles) {
	if (Type >= MaxTimer || Type == UnknownTimer) { return; }
	if (Cycles == 0) { 
		return; //Ignore when your setting time to go in 0 cycles
	}
	TimerDetatils[Type].CyclesToTimer = (double)Cycles - Timer;  //replace the new cycles
	TimerDetatils[Type].Active = true;
	FixTimers();
}

void CSystemTimer::ChangeTimerRelative (TimerType Type, DWORD Cycles) {
	if (Type >= MaxTimer || Type == UnknownTimer) { return; }
	if (TimerDetatils[Type].Active) {		
		TimerDetatils[Type].CyclesToTimer += Cycles; //Add to the timer
	} else {
		TimerDetatils[Type].CyclesToTimer = Cycles - Timer;  //replace the new cycles
	}
	TimerDetatils[Type].Active = true;
	FixTimers();
}

void CSystemTimer::CheckTimer (void) {
	if (Timer > 0) { return; }
//	TlbLog.Log("%s: Timer = %d, CurrentTimerType = %d",_System->GetRecompiler() ? "Recomp" : "Interp",Timer, CurrentTimerType);

	switch (CurrentTimerType) {
	case ViTimer:      _System->ExternalEvent(TimerDone_Vi); break;
	case AiTimer:      _System->ExternalEvent(TimerDone_Ai); break;
	case AiTimerDMA:   _System->ExternalEvent(TimerDone_AiDMA); break;
	case RSPTimerDlist:_System->ExternalEvent(TimerDone_RSPDlist); break;
	case CompareTimer: _System->ExternalEvent(TimerDone_Compare); break;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
}


void CSystemTimer::DeactiateTimer (TimerType Type)  {
	if (Type >= MaxTimer || Type == UnknownTimer) { return; }
	TimerDetatils[Type].Active = false;
	FixTimers();	
}

double CSystemTimer::GetTimer (TimerType Type) const {
	if (!TimerDetatils[Type].Active) { return 0; }
	return TimerDetatils[Type].CyclesToTimer + Timer;
}

void CSystemTimer::FixTimers (void) {
	int count;

	//Update the cycles for the remaining number of cycles to timer
	for (count = 0; count < MaxTimer; count++) {
		if (!TimerDetatils[count].Active) { continue; }
		TimerDetatils[count].CyclesToTimer += Timer;
	}

	//Set Max timer 
	Timer = 0x7FFFFFFF;
		
	//Find the smallest timer left to go
	for (count = 0; count < MaxTimer; count++) {
		if (!TimerDetatils[count].Active) { continue; }
		if (TimerDetatils[count].CyclesToTimer >= Timer) { continue; }
		Timer = TimerDetatils[count].CyclesToTimer;
		CurrentTimerType = (TimerType)count;
	}

	//Move the timer back this value
	for (count = 0; count < MaxTimer; count++) {
		if (!TimerDetatils[count].Active) { continue; }
		TimerDetatils[count].CyclesToTimer -= Timer;
	}
}

void CSystemTimer::ResetTimer ( int NextVITimer ) {
	//initilize Structure
	for (int count = 0; count < MaxTimer; count ++) {
		TimerDetatils[count].Active        = false;
		TimerDetatils[count].CyclesToTimer = 0;
	}
	CurrentTimerType = UnknownTimer;
	Timer            = 0;

	//set the initial timer for Video Interrupts
	ChangeTimerRelative(ViTimer,NextVITimer); 
}

void CSystemTimer::UpdateTimer (int StepIncrease) {
	Timer -= StepIncrease;
}
