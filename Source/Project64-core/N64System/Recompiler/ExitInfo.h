/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <Project64-core/N64System/Recompiler/RegInfo.h>

struct CExitInfo
{
	enum EXIT_REASON
	{
		Normal = 0,
		Normal_NoSysCheck = 1,
		DoCPU_Action = 2,
		COP1_Unuseable = 3,
		DoSysCall = 4,
		TLBReadMiss = 5,
		TLBWriteMiss = 6,
		DivByZero = 7,
		ExitResetRecompCode = 8,
	};

	uint32_t    ID;
	uint32_t    TargetPC;
	CRegInfo    ExitRegSet;
	EXIT_REASON reason;
	STEP_TYPE   NextInstruction;
	uint32_t *  JumpLoc; //32bit jump
};

typedef std::list<CExitInfo> EXIT_LIST;
