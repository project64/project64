#include "stdafx.h"

#include "JumpInfo.h"
#include "SectionInfo.h"
#include <Project64-core\N64System\Recompiler\CodeBlock.h>

CJumpInfo::CJumpInfo(CCodeBlock & CodeBlock) :
    RegSet(CodeBlock, CodeBlock.RecompilerOps()->Assembler())
{
    TargetPC = (uint32_t)-1;
    JumpPC = (uint32_t)-1;
    LinkAddress = (uint32_t)-1;
    BranchLabel = "";
    FallThrough = false;
    PermLoop = false;
    DoneDelaySlot = false;
    Reason = ExitReason_Normal;
}
