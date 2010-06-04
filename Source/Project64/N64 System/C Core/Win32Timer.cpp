#include "stdafx.h"

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
		_Notify->DisplayError("Error during timer begin");
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