/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include <windows.h>
#include <stdio.h>
#include "..\..\N64 System.h"


// ****************** Testing Audio Stuff *****************
CAudio::CAudio (void)
{
	ResetAudioSettings();
}

// I seem to be getting clicking when I set CF to 1 and VSyncTiming to 789000
void CAudio::ResetAudioSettings (void)
{
	//float CAudio::VSyncTiming = 789000.0f; // 500000
	////const float VSyncTiming = 760000.0f;
	m_FramesPerSecond = 60.0f;
	m_BytesPerSecond = 0;
	m_Length = 0;
	m_Status = 0;
	m_CountsPerByte = 0;
	m_SecondBuff = 0;
	m_CurrentCount = 0;
	m_CurrentLength = 0;
	m_IntScheduled = 0;
	m_VSyncTiming = 789000.0f;
}

void CAudio::AiCallBack () 
{
	if (m_SecondBuff != 0) {
		m_IntScheduled = (DWORD)((double)m_SecondBuff * m_CountsPerByte);
		_Reg->ChangeTimerFixed(AiTimer, m_IntScheduled);
	}
	m_CurrentCount = _Reg->COUNT_REGISTER;
	m_CurrentLength = m_SecondBuff;
	m_SecondBuff = 0;
	m_Status &= 0x7FFFFFFF;
}

DWORD CAudio::AiGetLength (void)
{
	double AiCounts;
//	static DWORD LengthReadHack = 0;
//	if ((COUNT_REGISTER - LengthReadHack) < 0x20) {
//		// This is a Spin Lock... ;-/
//		//COUNT_REGISTER += (DWORD)(CountsPerByte*0.5); // Lets speed up the CPU to the next Event
//		//CurrentLength = 0;
//		COUNT_REGISTER+=0xA; // This hack is necessary... but what is a good value??
//	}
//	LengthReadHack = COUNT_REGISTER;
	AiCounts = m_CountsPerByte * m_CurrentLength;
	AiCounts = AiCounts - (double)(_Reg->COUNT_REGISTER - m_CurrentCount);
	if (AiCounts < 0)
	{
		return 0;
	}
//	return 0;
	return (DWORD)(AiCounts / m_CountsPerByte);
}

DWORD CAudio::AiGetStatus (void)
{
	return m_Status;
}

void CAudio::AiSetLength (void) 
{
	// Set Status to FULL for a few COUNT cycles
	if (m_CurrentLength == 0) {
		m_CurrentLength = _Reg->AI_LEN_REG;
		m_CurrentCount = _Reg->COUNT_REGISTER;
		m_IntScheduled = (DWORD)((double)_Reg->AI_LEN_REG * m_CountsPerByte);
		_Reg->ChangeTimerFixed(AiTimer, m_IntScheduled);
	} else {
		m_SecondBuff = _Reg->AI_LEN_REG;
		m_Status |= 0x80000000;
	}
}

void CAudio::UpdateAudioTimer (DWORD CountsPerFrame) 
{
	double CountsPerSecond = (DWORD)((double)CountsPerFrame * m_FramesPerSecond); // This will only work with NTSC...	VSyncTiming...
	m_CountsPerByte = CountsPerSecond / (double)m_BytesPerSecond;
}

void CAudio::AiSetFrequency (DWORD Dacrate, DWORD System) {
	double CountsPerSecond;
	switch (System) {
		case SYSTEM_NTSC: m_BytesPerSecond = 48681812 / (Dacrate + 1); break;
		case SYSTEM_PAL:  m_BytesPerSecond = 49656530 / (Dacrate + 1); break;
		case SYSTEM_MPAL: m_BytesPerSecond = 48628316 / (Dacrate + 1); break;
	}
	if (System == SYSTEM_PAL) {
		m_FramesPerSecond = 50.0;
	} else {
		m_FramesPerSecond = 60.0;
	}
	m_BytesPerSecond = (m_BytesPerSecond * 4); // This makes it Bytes Per Second...
	CountsPerSecond = (double)(((double)m_VSyncTiming) * (double)60.0); // This will only work with NTSC...	VSyncTiming...
	m_CountsPerByte = (double)CountsPerSecond / (double)m_BytesPerSecond;
	m_SecondBuff = m_Status = m_CurrentLength = 0;
	//CountsPerByte /= CountPerOp;
}
