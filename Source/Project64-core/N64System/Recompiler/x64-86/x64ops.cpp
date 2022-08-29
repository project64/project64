#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Recompiler/x64-86/x64ops.h>

CX64Ops::CX64Ops(CCodeBlock & CodeBlock) :
    m_CodeBlock(CodeBlock)
{
}

#endif