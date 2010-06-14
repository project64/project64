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
	m_CountsPerByte = 50; // should be calculated ... see below
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
}



