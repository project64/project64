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
#pragma once

__interface CDebugger
{
	virtual void TLBChanged(void) = 0;
	virtual bool CPUStepStarted(void) = 0;
	virtual void CPUStep(void) = 0;
	virtual void FrameDrawn(void) = 0;
};
