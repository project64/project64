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
#include <Project64-core/N64System/Recompiler/ExitInfo.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>

struct CJumpInfo
{
    typedef CExitInfo::EXIT_REASON EXIT_REASON;
    CJumpInfo();

    uint32_t	TargetPC;
    uint32_t	JumpPC;
    stdstr		BranchLabel;
    uint32_t *	LinkLocation;
    uint32_t *	LinkLocation2;
    bool		FallThrough;
    bool		PermLoop;
    bool		DoneDelaySlot;  //maybe deletable
    CRegInfo	RegSet;
    EXIT_REASON ExitReason;
};
