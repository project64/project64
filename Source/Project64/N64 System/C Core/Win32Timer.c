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
#include "main.h"
#pragma comment(lib, "Winmm.lib")

/********************* Win32 Thread Timer ********************/

struct {
	DWORD Frames, LastTime;
	DOUBLE Ratio;
} FPSTimer = { 0,0, 1000.0F / 60.0F };

void Timer_Initialize(double Hertz) {
	FPSTimer.Ratio = 1000.0f / Hertz;
}

void Timer_Start(void) {
	TIMECAPS Caps;
	timeGetDevCaps(&Caps, sizeof(Caps));
	if (timeBeginPeriod(Caps.wPeriodMin) == TIMERR_NOCANDO)
	{
		DisplayError("Error during timer begin");
	}

	FPSTimer.Frames = 0;
	FPSTimer.LastTime = timeGetTime();
}

void Timer_Stop(void) {
	TIMECAPS Caps;
	timeGetDevCaps(&Caps, sizeof(Caps));
	timeEndPeriod(Caps.wPeriodMin);
}

BOOL Timer_Process(DWORD * FrameRate) {
	double CalculatedTime;
	DWORD CurrentTime;

	FPSTimer.Frames++;
	CurrentTime = timeGetTime();

	/* Calculate time that should of elapsed for this frame */
	CalculatedTime = (double)FPSTimer.LastTime + (FPSTimer.Ratio * (double)FPSTimer.Frames);
	if ((double)CurrentTime < CalculatedTime) {
		long time = (int)(CalculatedTime - (double)CurrentTime);
		if (time > 0) {
			Sleep(time);
		}

		/* Refresh current time */
		CurrentTime = timeGetTime();
	}

	if (CurrentTime - FPSTimer.LastTime >= 1000) {
		/* Output FPS */
		if (FrameRate != NULL) { *FrameRate = FPSTimer.Frames; }
		FPSTimer.Frames = 0;
		FPSTimer.LastTime = CurrentTime;

		return TRUE;
	} else 
		return FALSE;
}