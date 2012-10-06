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
	m_SecondBuff = 0;
	m_Status = 0;
	m_BytesPerSecond = 0;
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
	WriteTraceF(TraceAudio,__FUNCTION__ ": _Reg->AI_LEN_REG = %d",_Reg->AI_LEN_REG);
	if (_Reg->AI_LEN_REG != 0)
	{
		DWORD NewLength = GetLength() + _Reg->AI_LEN_REG;
		_SystemTimer->SetTimer(CSystemTimer::AiTimer,NewLength * m_CountsPerByte,false);
		WriteTraceF(TraceAudio,__FUNCTION__ ": NewLength = %d",NewLength);
		/*if (m_CurrentLength == 0) {
			m_CurrentLength = _Reg->AI_LEN_REG;
		} else {
			m_SecondBuff = _Reg->AI_LEN_REG;
			//m_Status |= 0x80000000;
		}*/
	} else {
		_SystemTimer->StopTimer(CSystemTimer::AiTimer);
		m_SecondBuff = 0;
		m_Status = 0;
	}

	if (_Plugins->Audio()->LenChanged != NULL) 
	{
		_Plugins->Audio()->LenChanged(); 
	}
}

void CAudio::TimerDone ( void )
{
	/*_Reg->MI_INTR_REG |= MI_INTR_AI;
	_Reg->CheckInterrupts();

	if (m_SecondBuff != 0) {
		_SystemTimer->SetTimer(CSystemTimer::AiTimer,m_SecondBuff * m_CountsPerByte,false);
	}
	m_CurrentLength = m_SecondBuff;
	m_SecondBuff = 0;
	m_Status &= 0x7FFFFFFF;
*/
	m_SecondBuff = 0;
}

void CAudio::SetViIntr ( DWORD /*VI_INTR_TIME*/ )
{
	/*
	double CountsPerSecond = (DWORD)((double)VI_INTR_TIME * m_FramesPerSecond);
	if (m_BytesPerSecond != 0)
	{
		//m_CountsPerByte = (double)CountsPerSecond / (double)m_BytesPerSecond;
	}
	*/
}


void CAudio::SetFrequency (DWORD Dacrate, DWORD System) 
{
	WriteTraceF(TraceAudio,__FUNCTION__ "(Dacrate: %X System: %d): AI_BITRATE_REG = %X",Dacrate,System,_Reg->AI_BITRATE_REG);
	DWORD Frequency;

	switch (System) {
	case SYSTEM_PAL:  Frequency = 49656530 / (Dacrate + 1); break;
	case SYSTEM_MPAL: Frequency = 48628316 / (Dacrate + 1); break;
	default:          Frequency = 48681812 / (Dacrate + 1); break;
	}

	//nBlockAlign = 16 / 8 * 2;
	m_BytesPerSecond = Frequency * 4;
	m_BytesPerSecond = 194532;
	m_BytesPerSecond = 128024;

	if (System == SYSTEM_PAL) {
		m_FramesPerSecond = 50;
	} else {
		m_FramesPerSecond = 60;
	}
}

