#include "stdafx.h"
#include <Project64-core\N64System\Recompiler\ExitInfo.h>

CExitInfo::CExitInfo(CCodeBlock & CodeBlock) :
    ID(0),
    TargetPC(0),
    JumpLoc(nullptr),
    ExitRegSet(CodeBlock)
{
}
