#include "stdafx.h"

CSystemTimer::CSystemTimer( int & NextTimer ) :
	m_NextTimer(NextTimer),
	m_inFixTimer(false)
{
}

void CSystemTimer::Reset ( void ) 
{
	//initialise Structure
	for (int i = 0; i < MaxTimer; i++)
	{
		m_TimerDetatils[i].Active = false;
		m_TimerDetatils[i].CyclesToTimer = 0;
	}
	m_Current    = UnknownTimer;
	m_LastUpdate = 0;
	m_NextTimer  = 0;

	SetTimer(ViTimer,50000,false);
	SetCompareTimer();
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
			m_TimerDetatils[Type].CyclesToTimer = (__int64)Cycles - (__int64)m_NextTimer;  //replace the new cycles
		}
	} else {
		m_TimerDetatils[Type].CyclesToTimer = (__int64)Cycles - (__int64)m_NextTimer;  //replace the new cycles
	}
	FixTimers();
}

DWORD CSystemTimer::GetTimer ( TimerType Type )
{
	if (Type >= MaxTimer || Type == UnknownTimer) 
	{
		_Notify->BreakPoint(__FILE__,__LINE__); 
		return 0;
	}
	if (!m_TimerDetatils[Type].Active)
	{
		return 0;
	}
	__int64 CyclesToTimer = m_TimerDetatils[Type].CyclesToTimer + m_NextTimer;
	if (CyclesToTimer < 0)
	{
		return 0;
	}
	if (CyclesToTimer > 0x7FFFFFFF)
	{
		return 0x7FFFFFFF;
	}
	return (DWORD)CyclesToTimer;
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

	if (m_inFixTimer)
	{
		return;
	}
	m_inFixTimer = true;
	
	UpdateTimers();
	if (GetTimer(CompareTimer) > 0x60000000)
	{
		SetCompareTimer();
	}

	//Update the cycles for the remaining number of cycles to timer
	int count;
	for (count = 0; count < MaxTimer; count++) 
	{
		if (!m_TimerDetatils[count].Active) 
		{
			continue; 
		}
		m_TimerDetatils[count].CyclesToTimer += m_NextTimer;
	}

	//Set Max timer 
	m_NextTimer = 0x7FFFFFFF;
		
	//Find the smallest timer left to go
	for (count = 0; count < MaxTimer; count++) 
	{
		if (!m_TimerDetatils[count].Active) 
		{
			continue; 
		}
		if (m_TimerDetatils[count].CyclesToTimer >= m_NextTimer) 
		{
			continue; 
		}
		m_NextTimer = (int)m_TimerDetatils[count].CyclesToTimer;
		m_Current = (TimerType)count;
	}

	//Move the timer back this value
	for (count = 0; count < MaxTimer; count++) 
	{
		if (!m_TimerDetatils[count].Active) 
		{
			continue; 
		}
		m_TimerDetatils[count].CyclesToTimer -= m_NextTimer;
	}
	m_LastUpdate = m_NextTimer;
	m_inFixTimer = false;
}

void CSystemTimer::UpdateTimers ( void )
{
	int TimeTaken = m_LastUpdate - m_NextTimer;
	if (TimeTaken != 0)
	{
		m_LastUpdate = m_NextTimer;
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
		_System->ExternalEvent(SysEvent_ResetCPU_SoftDone); 
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
			_System->RefreshScreen();
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
		_Audio->TimerDone();
		break;
	default:
		BreakPoint(__FILE__,__LINE__);
	}
	//CheckTimer();
	/*if (Profiling) { 
		StartTimer(LastTimer); 
	}*/
}

void CSystemTimer::SetCompareTimer ( void )
{
	DWORD NextCompare = 0x7FFFFFFF;
	if (_Reg)
	{
		NextCompare = _Reg->COMPARE_REGISTER - _Reg->COUNT_REGISTER;
		if ((NextCompare & 0x80000000) != 0) 
		{
			NextCompare = 0x7FFFFFFF; 
		}
	}
	SetTimer(CompareTimer,NextCompare,false);
}

void CSystemTimer::UpdateCompareTimer ( void )
{
	SetCompareTimer();
}

bool CSystemTimer::SaveAllowed  ( void )
{
	if (GetTimer(CompareTimer) <= 0)
	{
		return false;
	}
	for (int i = 0; i < MaxTimer; i++)
	{
		if (i == CompareTimer) { continue; }
		if (i == ViTimer) { continue; }
		if (m_TimerDetatils[i].Active)
		{
			return false;
		}
	}
	return true;
}

void CSystemTimer::SaveData ( void * file ) const
{
	DWORD TimerDetailsSize = sizeof(TIMER_DETAILS);
	DWORD Entries = sizeof(m_TimerDetatils)/sizeof(m_TimerDetatils[0]);
	zipWriteInFileInZip(file,&TimerDetailsSize,sizeof(TimerDetailsSize));
	zipWriteInFileInZip(file,&Entries,sizeof(Entries));
	zipWriteInFileInZip(file,(void *)&m_TimerDetatils,sizeof(m_TimerDetatils));
	zipWriteInFileInZip(file,(void *)&m_LastUpdate,sizeof(m_LastUpdate));
	zipWriteInFileInZip(file,&m_NextTimer,sizeof(m_NextTimer));
	zipWriteInFileInZip(file,(void *)&m_Current,sizeof(m_Current));
}

void CSystemTimer::LoadData ( void * file )
{
	DWORD TimerDetailsSize, Entries;

	unzReadCurrentFile( file,&TimerDetailsSize,sizeof(TimerDetailsSize));
	unzReadCurrentFile( file,&Entries,sizeof(Entries));

	if (TimerDetailsSize != sizeof(TIMER_DETAILS)) { _Notify->BreakPoint(__FILE__,__LINE__); return; }
	if (Entries != sizeof(m_TimerDetatils)/sizeof(m_TimerDetatils[0])) { _Notify->BreakPoint(__FILE__,__LINE__); return; }

	unzReadCurrentFile(file,(void *)&m_TimerDetatils,sizeof(m_TimerDetatils));
	unzReadCurrentFile(file,(void *)&m_LastUpdate,sizeof(m_LastUpdate));
	unzReadCurrentFile(file,&m_NextTimer,sizeof(m_NextTimer));
	unzReadCurrentFile(file,(void *)&m_Current,sizeof(m_Current));
}