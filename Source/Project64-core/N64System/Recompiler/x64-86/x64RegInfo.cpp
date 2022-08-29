#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Recompiler/x64-86/x64RegInfo.h>

CX64RegInfo::CX64RegInfo(CCodeBlock & /*CodeBlock*/, CX64Ops & /*Assembler*/)
{
}

CX64RegInfo::CX64RegInfo(const CX64RegInfo&)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CX64RegInfo::~CX64RegInfo()
{
}

CX64RegInfo& CX64RegInfo::operator=(const CX64RegInfo & right)
{
    CRegBase::operator=(right);
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return *this;
}

bool CX64RegInfo::operator==(const CX64RegInfo & /*right*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CX64RegInfo::operator!=(const CX64RegInfo & /*right*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

#endif