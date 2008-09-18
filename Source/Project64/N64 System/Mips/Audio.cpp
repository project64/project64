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
CAudio::CAudio (CRegisters * Reg) :
	_Reg(Reg),
	VSyncTiming(789000.0f)
{
	//float CAudio::VSyncTiming = 789000.0f; // 500000
	////const float VSyncTiming = 760000.0f;
	ResetAudioSettings();
}

// I seem to be getting clicking when I set CF to 1 and VSyncTiming to 789000
void CAudio::ResetAudioSettings (void)
{
	FramesPerSecond = 60.0f;
	BytesPerSecond = 0;
	Length = 0;
	Status = 0;
	CountsPerByte = 0;
	SecondBuff = 0;
	CurrentCount = 0;
	CurrentLength = 0;
	IntScheduled = 0;
}

void CAudio::AiCallBack () {
	if (SecondBuff != 0) {
		IntScheduled = (DWORD)((double)SecondBuff * CountsPerByte);
		_Reg->ChangeTimerFixed(AiTimer, IntScheduled);
	}
	CurrentCount = _Reg->COUNT_REGISTER;
	CurrentLength = SecondBuff;
	SecondBuff = 0;
	Status &= 0x7FFFFFFF;
}

DWORD CAudio::AiGetLength (CAudio * _this) {
	double AiCounts;
//	static DWORD LengthReadHack = 0;
//	if ((COUNT_REGISTER - LengthReadHack) < 0x20) {
//		// This is a Spin Lock... ;-/
//		//COUNT_REGISTER += (DWORD)(CountsPerByte*0.5); // Lets speed up the CPU to the next Event
//		//CurrentLength = 0;
//		COUNT_REGISTER+=0xA; // This hack is necessary... but what is a good value??
//	}
//	LengthReadHack = COUNT_REGISTER;
	AiCounts = _this->CountsPerByte * _this->CurrentLength;
	AiCounts = AiCounts - (double)(_this->_Reg->COUNT_REGISTER - _this->CurrentCount);
	if (AiCounts < 0)
		return 0;
//	return 0;
	return (DWORD)(AiCounts/_this->CountsPerByte);
}

DWORD CAudio::AiGetStatus (CAudio * _this) {
	return _this->Status;
}

void CAudio::AiSetLength (CAudio * _this, DWORD data) {
	// Set Status to FULL for a few COUNT cycles
	if (_this->CurrentLength == 0) {
		_this->CurrentLength = _this->_Reg->AI_LEN_REG;
		_this->CurrentCount = _this->_Reg->COUNT_REGISTER;
		_this->IntScheduled = (DWORD)((double)_this->_Reg->AI_LEN_REG * _this->CountsPerByte);
		_this->_Reg->ChangeTimerFixed(AiTimer, _this->IntScheduled);
	} else {
		_this->SecondBuff = _this->_Reg->AI_LEN_REG;
		_this->Status |= 0x80000000;
	}
}

void CAudio::UpdateAudioTimer (DWORD CountsPerFrame) {
	double CountsPerSecond;
	CountsPerSecond = (DWORD)((double)CountsPerFrame * FramesPerSecond); // This will only work with NTSC...	VSyncTiming...
	CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
}

void CAudio::AiSetFrequency (DWORD Dacrate, DWORD System) {
	double CountsPerSecond;
	switch (System) {
		case SYSTEM_NTSC: BytesPerSecond = 48681812 / (Dacrate + 1); break;
		case SYSTEM_PAL:  BytesPerSecond = 49656530 / (Dacrate + 1); break;
		case SYSTEM_MPAL: BytesPerSecond = 48628316 / (Dacrate + 1); break;
	}
	if (System == SYSTEM_PAL) {
		FramesPerSecond = 50.0;
	} else {
		FramesPerSecond = 60.0;
	}
	BytesPerSecond = (BytesPerSecond * 4); // This makes it Bytes Per Second...
	CountsPerSecond = (double)(((double)VSyncTiming) * (double)60.0); // This will only work with NTSC...	VSyncTiming...
	CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
	SecondBuff = Status = CurrentLength = 0;
	//CountsPerByte /= CountPerOp;
}
