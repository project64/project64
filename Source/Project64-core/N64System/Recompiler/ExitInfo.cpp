#include "stdafx.h"

#include <Project64-core\N64System\Recompiler\CodeBlock.h>
#include <Project64-core\N64System\Recompiler\ExitInfo.h>

CExitInfo::CExitInfo(CCodeBlock & CodeBlock) :
    ID(0),
    TargetPC(0),
    ExitRegSet(CodeBlock, CodeBlock.RecompilerOps()->Assembler())
{
#if defined(__i386__) || defined(_M_IX86)
    JumpLabel = CodeBlock.RecompilerOps()->Assembler().newLabel();
#else
    g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
}
