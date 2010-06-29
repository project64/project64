#include "stdafx.h"

CAudio::CAudio (void)
{
	Reset();
}

CAudio::~CAudio (void)
{
}

void CAudio::Reset ( void )
{
	m_CurrentLength = 0;
	m_SecondBuff = 0;
	m_Status = 0;
	m_CountsPerByte = 500; // should be calculated ... see below
	m_FramesPerSecond = 60;
}

DWORD CAudio::GetLength ( void )
{
	DWORD TimeLeft = _SystemTimer->GetTimer(CSystemTimer::AiTimer);
	if (TimeLeft > 0)
	{
		return TimeLeft / m_CountsPerByte;
	}
	return 0;
}

DWORD CAudio::GetStatus ( void )
{
	return m_Status;
}

void CAudio::LenChanged ( void )
{
	if (_Reg->AI_LEN_REG == 0)
	{
		return;
	}

	if (m_CurrentLength == 0) {
		m_CurrentLength = _Reg->AI_LEN_REG;
		_SystemTimer->SetTimer(CSystemTimer::AiTimer,m_CurrentLength * m_CountsPerByte,false);
	} else {
		m_SecondBuff = _Reg->AI_LEN_REG;
		m_Status |= 0x80000000;
	}
	if (_Plugins->Audio()->LenChanged != NULL) 
	{
		_Plugins->Audio()->LenChanged(); 
	}
}

void CAudio::TimerDone ( void )
{
	if (m_SecondBuff != 0) {
		_SystemTimer->SetTimer(CSystemTimer::AiTimer,m_SecondBuff * m_CountsPerByte,true);
	}
	m_CurrentLength = m_SecondBuff;
	m_SecondBuff = 0;
	m_Status &= 0x7FFFFFFF;
}

void CAudio::SetViIntr ( DWORD VI_INTR_TIME )
{
	double CountsPerSecond = (DWORD)((double)VI_INTR_TIME * m_FramesPerSecond);
	m_CountsPerByte = (double)CountsPerSecond / (double)m_BytesPerSecond;
	//m_CountsPerByte = 490; // donkey kong
	//m_CountsPerByte = 100; // Paper mario
	//m_CountsPerByte = 490; // Paper mario
	
}


void CAudio::SetFrequency (DWORD Dacrate, DWORD System) 
{
	
	DWORD Frequency;

	switch (System) {
	case SYSTEM_PAL:  Frequency = 49656530 / (Dacrate + 1); break;
	case SYSTEM_MPAL: Frequency = 48628316 / (Dacrate + 1); break;
	default:          Frequency = 48681812 / (Dacrate + 1); break;
	}

	//nBlockAlign = 16 / 8 * 2;
	m_BytesPerSecond = Frequency * 4;

	if (System == SYSTEM_PAL) {
		m_FramesPerSecond = 50.0;
	} else {
		m_FramesPerSecond = 60.0;
	}
}

