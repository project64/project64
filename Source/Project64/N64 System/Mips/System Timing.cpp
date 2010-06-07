#include "stdafx.h"

CSystemTimer::CSystemTimer( int & NextTimer ) :
	m_NextTimer(NextTimer)
{
	Reset();
	SetTimer(ViTimer,50000,false);
}

void CSystemTimer::Reset ( void ) 
{
	//initialise Structure
	for (int i = 0; i < MaxTimer; i++)
	{
		m_TimerDetatils[i].Active = false;
		m_TimerDetatils[i].CyclesToTimer = 0;
	}
	m_Current   = UnknownTimer;
	m_Timer     = 0;
	m_NextTimer = 0;
}

void CSystemTimer::SetTimer ( TimerType Type, DWORD Cycles, bool bRelative )
{
	if (Type >= MaxTimer || Type == UnknownTimer) 
	{
		_Notify->BreakPoint(__FILE__,__LINE__); 
		return;
	}
	UpdateTimers();
	
	m_TimerDetatils[Type].Active = true;
	if (bRelative)
	{
		if (m_TimerDetatils[Type].Active) 
		{
			m_TimerDetatils[Type].CyclesToTimer += Cycles; //Add to the timer
		} else {
			m_TimerDetatils[Type].CyclesToTimer = (__int64)Cycles - (__int64)m_Timer;  //replace the new cycles
		}
	} else {
		m_TimerDetatils[Type].CyclesToTimer = (__int64)Cycles - (__int64)m_Timer;  //replace the new cycles
	}
	FixTimers();
}

void CSystemTimer::StopTimer ( TimerType Type )
{
	if (Type >= MaxTimer || Type == UnknownTimer) 
	{
		_Notify->BreakPoint(__FILE__,__LINE__); 
		return;
	}
	m_TimerDetatils[Type].Active = false;
	FixTimers();
}


void CSystemTimer::FixTimers (void)
{
	int count;
	
	//Update the cycles for the remaining number of cycles to timer
	for (count = 0; count < MaxTimer; count++) 
	{
		if (!m_TimerDetatils[count].Active) 
		{
			continue; 
		}
		m_TimerDetatils[count].CyclesToTimer += m_Timer;
	}

	//Set Max timer 
	m_Timer = 0x7FFFFFFF;
		
	//Find the smallest timer left to go
	for (count = 0; count < MaxTimer; count++) 
	{
		if (!m_TimerDetatils[count].Active) 
		{
			continue; 
		}
		if (m_TimerDetatils[count].CyclesToTimer >= m_Timer) 
		{
			continue; 
		}
		m_Timer = m_TimerDetatils[count].CyclesToTimer;
		m_Current = (TimerType)count;
	}

	//Move the timer back this value
	for (count = 0; count < MaxTimer; count++) 
	{
		if (!m_TimerDetatils[count].Active) 
		{
			continue; 
		}
		m_TimerDetatils[count].CyclesToTimer -= m_Timer;
	}
	m_NextTimer = m_Timer;
}

void CSystemTimer::UpdateTimers ( void )
{
	int TimeTaken = m_Timer - m_NextTimer;
	if (TimeTaken != 0)
	{
		m_Timer = m_NextTimer;
		_Reg->COUNT_REGISTER += TimeTaken;
		_Reg->RANDOM_REGISTER -= TimeTaken / g_CountPerOp;
		while ((int)_Reg->RANDOM_REGISTER < (int)_Reg->WIRED_REGISTER) 
		{
			_Reg->RANDOM_REGISTER += 32 - _Reg->WIRED_REGISTER;
		}
	}
}

void CSystemTimer::TimerDone (void) 
{
	UpdateTimers();

/*	DWORD LastTimer;
	if (Profiling) { 
		LastTimer = StartTimer(Timer_Done); 
	}
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.GenerateLog && LogOptions.LogExceptions && !LogOptions.NoInterrupts) {
		LogMessage("%08X: Timer Done (Type: %d CurrentTimer: %d)", *_PROGRAM_COUNTER, m_Current, *_Timer );
	}
#endif
*/
	switch (m_Current) {
	case CSystemTimer::CompareTimer:
		_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP7;
		_Reg->CheckInterrupts();
		UpdateCompareTimer();
		break;
	case CSystemTimer::SoftResetTimer:
		_SystemTimer->StopTimer(CSystemTimer::SoftResetTimer);
		_N64System->SoftReset();
		break;
	case CSystemTimer::SiTimer:
		_SystemTimer->StopTimer(CSystemTimer::SiTimer);
		_Reg->MI_INTR_REG |= MI_INTR_SI;
		_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		_Reg->CheckInterrupts();
		break;
	case CSystemTimer::PiTimer:
		_SystemTimer->StopTimer(CSystemTimer::PiTimer);
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		break;
	case CSystemTimer::ViTimer:
		try
		{
			_N64System->RefreshScreen();
		} 
		catch (...)
		{
			WriteTraceF(TraceError,"Exception caught in Refresh Screen\nFile: %s\nLine: %d",__FILE__,__LINE__);
		}
		_Reg->MI_INTR_REG |= MI_INTR_VI;
		_Reg->CheckInterrupts();
		break;
	case CSystemTimer::RspTimer:
		_SystemTimer->StopTimer(CSystemTimer::RspTimer);
		RunRsp();
		break;
	case CSystemTimer::AiTimer:
		_SystemTimer->StopTimer(CSystemTimer::AiTimer);
		_Reg->MI_INTR_REG |= MI_INTR_AI;
		_Reg->CheckInterrupts();
		_Audio->AiCallBack();
		break;
	default:
		BreakPoint(__FILE__,__LINE__);
	}
	//CheckTimer();
	/*if (Profiling) { 
		StartTimer(LastTimer); 
	}*/
}

void CSystemTimer::UpdateCompareTimer ( void )
{
	DWORD NextCompare = _Reg->COMPARE_REGISTER - _Reg->COUNT_REGISTER;
	if ((NextCompare & 0x80000000) != 0) 
	{
		NextCompare = 0x7FFFFFFF; 
	}
	_SystemTimer->SetTimer(CSystemTimer::CompareTimer,NextCompare,false);
}

#ifdef toremove
extern CLog TlbLog;

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
	case ViTimer:      _N64System->ExternalEvent(TimerDone_Vi); break;
	case AiTimer:      _N64System->ExternalEvent(TimerDone_Ai); break;
	case AiTimerDMA:   _N64System->ExternalEvent(TimerDone_AiDMA); break;
	case RSPTimerDlist:_N64System->ExternalEvent(TimerDone_RSPDlist); break;
	case CompareTimer: _N64System->ExternalEvent(TimerDone_Compare); break;
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

#endif