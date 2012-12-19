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

class CJumpInfo
{
public:
	CJumpInfo();

	DWORD		TargetPC;
	DWORD		JumpPC;
	stdstr		BranchLabel;
	DWORD *		LinkLocation;
	DWORD *		LinkLocation2;	
	bool		FallThrough;	
	bool		PermLoop;
	bool		DoneDelaySlot;  //maybe deletable
	CRegInfo	RegSet;
};
