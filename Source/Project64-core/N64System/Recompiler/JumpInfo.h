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
