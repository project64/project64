/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
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
	m_CountsPerByte = g_System->AiCountPerBytes(); // should be calculated ... see below, instead allow from user settings
	m_FramesPerSecond = 60;
}

DWORD CAudio::GetLength ( void )
{
	WriteTraceF(TraceAudio,__FUNCTION__ ": Start (m_SecondBuff = %d)",m_SecondBuff);
	DWORD TimeLeft = g_SystemTimer->GetTimer(CSystemTimer::AiTimer), Res = 0;
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
	WriteTraceF(TraceAudio,__FUNCTION__ ": Start (g_Reg->AI_LEN_REG = %d)",g_Reg->AI_LEN_REG);
	if (g_Reg->AI_LEN_REG != 0)
	{
		if (g_Reg->AI_LEN_REG >= 0x20000)
		{
			WriteTraceF(TraceAudio,__FUNCTION__ ": *** Ignoring Write, To Large (%X)",g_Reg->AI_LEN_REG);
		} else {
			m_Status |= 0x80000000;
			if (g_SystemTimer->GetTimer(CSystemTimer::AiTimer) == 0)
			{
				if (m_SecondBuff)
				{
					g_Notify->BreakPoint(__FILE__,__LINE__);
				}
				WriteTraceF(TraceAudio,__FUNCTION__ ": Set Timer  AI_LEN_REG: %d m_CountsPerByte: %d",g_Reg->AI_LEN_REG,m_CountsPerByte);
				g_SystemTimer->SetTimer(CSystemTimer::AiTimer,g_Reg->AI_LEN_REG * m_CountsPerByte,false);
			} else {
				WriteTraceF(TraceAudio,__FUNCTION__ ": Increasing Second Buffer (m_SecondBuff %d Increase: %d)",m_SecondBuff,g_Reg->AI_LEN_REG);
				m_SecondBuff += g_Reg->AI_LEN_REG;
			}
		}
	} else {
		WriteTraceF(TraceAudio,__FUNCTION__ ": *** Reset Timer to 0");
		g_SystemTimer->StopTimer(CSystemTimer::AiTimer);
		m_SecondBuff = 0;
		m_Status = 0;
	}

	if (g_Plugins->Audio()->LenChanged != NULL) 
	{
		g_Plugins->Audio()->LenChanged(); 
	}
	WriteTraceF(TraceAudio,__FUNCTION__ ": Done");
}

void CAudio::TimerDone ( void )
{
	WriteTraceF(TraceAudio,__FUNCTION__ ": Start (m_SecondBuff = %d)",m_SecondBuff);
	if (m_SecondBuff != 0) 
	{
		g_SystemTimer->SetTimer(CSystemTimer::AiTimer,m_SecondBuff * m_CountsPerByte,false);
		m_SecondBuff = 0;
	} else {
		g_Reg->MI_INTR_REG |= MI_INTR_AI;
		g_Reg->CheckInterrupts();
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
	WriteTraceF(TraceAudio,__FUNCTION__ "(Dacrate: %X System: %d): AI_BITRATE_REG = %X",Dacrate,System,g_Reg->AI_BITRATE_REG);
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

