#include "stdafx.h"
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/Arm/ArmOps.h>

CArmOps::CArmOps(CCodeBlock & CodeBlock) :
    m_CodeBlock(CodeBlock)
{
}

#endif