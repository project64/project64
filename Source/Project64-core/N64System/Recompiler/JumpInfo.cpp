#include "stdafx.h"
#include <Project64-core\N64System\Recompiler\CodeBlock.h>
#include "SectionInfo.h"
#include "JumpInfo.h"

CJumpInfo::CJumpInfo(CCodeBlock & CodeBlock) :
    RegSet(CodeBlock, CodeBlock.RecompilerOps()->Assembler())
{
	TargetPC = (uint32_t)-1;
	JumpPC = (uint32_t)-1;
	BranchLabel = "";
	LinkLocation = nullptr;
	LinkLocation2 = nullptr;
	FallThrough = false;
	PermLoop = false;
	DoneDelaySlot = false;
	Reason = ExitReason_Normal;
}
