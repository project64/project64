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
	WriteTraceF(TraceAudio,__FUNCTION__ ": Start (m_SecondBuff = %d)",m_SecondBuff);
	DWORD TimeLeft = _SystemTimer->GetTimer(CSystemTimer::AiTimer), Res = 0;
	if (TimeLeft > 0)
	{
		Res = (TimeLeft / m_CountsPerByte) + m_SecondBuff;
	}
	WriteTraceF(TraceAudio,__FUNCTION__ ": Done (res = %d, TimeLeft = %d)",Res, TimeLeft);
	return Res;
}

DWORD CAudio::GetStatus ( void )
{
	WriteTraceF(TraceAudio,__FUNCTION__ ": m_Status = %X",m_Status);
	return m_Status;
}

void CAudio::LenChanged ( void )
{
	WriteTraceF(TraceAudio,__FUNCTION__ ": Start (_Reg->AI_LEN_REG = %d)",_Reg->AI_LEN_REG);
	if (_Reg->AI_LEN_REG != 0)
	{
		if (_Reg->AI_LEN_REG >= 0x20000)
		{
			WriteTraceF(TraceAudio,__FUNCTION__ ": *** Ignoring Write, To Large (%X)",_Reg->AI_LEN_REG);
		} else {
			m_Status |= 0x80000000;
			if (_SystemTimer->GetTimer(CSystemTimer::AiTimer) == 0)
			{
				if (m_SecondBuff)
				{
					_Notify->BreakPoint(__FILE__,__LINE__);
				}
				WriteTraceF(TraceAudio,__FUNCTION__ ": Set Timer  AI_LEN_REG: %d m_CountsPerByte: %d",_Reg->AI_LEN_REG,m_CountsPerByte);
				_SystemTimer->SetTimer(CSystemTimer::AiTimer,_Reg->AI_LEN_REG * m_CountsPerByte,false);
			} else {
				WriteTraceF(TraceAudio,__FUNCTION__ ": Increasing Second Buffer (m_SecondBuff %d Increase: %d)",m_SecondBuff,_Reg->AI_LEN_REG);
				m_SecondBuff += _Reg->AI_LEN_REG;
			}
		}
	} else {
		WriteTraceF(TraceAudio,__FUNCTION__ ": *** Reset Timer to 0");
		_SystemTimer->StopTimer(CSystemTimer::AiTimer);
		m_SecondBuff = 0;
		m_Status = 0;
	}

	if (_Plugins->Audio()->LenChanged != NULL) 
	{
		_Plugins->Audio()->LenChanged(); 
	}
	WriteTraceF(TraceAudio,__FUNCTION__ ": Done");
}

void CAudio::TimerDone ( void )
{
	WriteTraceF(TraceAudio,__FUNCTION__ ": Start (m_SecondBuff = %d)",m_SecondBuff);
	if (m_SecondBuff != 0) 
	{
		_SystemTimer->SetTimer(CSystemTimer::AiTimer,m_SecondBuff * m_CountsPerByte,false);
		m_SecondBuff = 0;
	} else {
		_Reg->MI_INTR_REG |= MI_INTR_AI;
		_Reg->CheckInterrupts();
		m_Status &= 0x7FFFFFFF;
	}
	WriteTraceF(TraceAudio,__FUNCTION__ ": Done",m_SecondBuff);
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

