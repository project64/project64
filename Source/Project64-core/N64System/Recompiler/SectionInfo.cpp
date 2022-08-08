#include "stdafx.h"
#include "SectionInfo.h"
#include "JumpInfo.h"

CJumpInfo::CJumpInfo(CCodeBlock & CodeBlock) :
    RegSet(CodeBlock)
{
	TargetPC = (uint32_t)-1;
	JumpPC = (uint32_t)-1;
	BranchLabel = "";
	LinkLocation = nullptr;
	LinkLocation2 = nullptr;
	FallThrough = false;
	PermLoop = false;
	DoneDelaySlot = false;
	ExitReason = CExitInfo::Normal;
}
