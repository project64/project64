#include "stdafx.h"

#if defined(__aarch64__)
#include <Project64-core/N64System/Recompiler/Aarch64/Aarch64RegInfo.h>

CAarch64RegInfo::CAarch64RegInfo(CCodeBlock & /*CodeBlock*/, CAarch64Ops & /*Assembler*/)
{
}

CAarch64RegInfo::CAarch64RegInfo(const CAarch64RegInfo &)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CAarch64RegInfo::~CAarch64RegInfo()
{
}

CAarch64RegInfo & CAarch64RegInfo::operator=(const CAarch64RegInfo & right)
{
    CRegBase::operator=(right);
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return *this;
}

bool CAarch64RegInfo::operator==(const CAarch64RegInfo & /*right*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CAarch64RegInfo::operator!=(const CAarch64RegInfo & /*right*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

#endif
