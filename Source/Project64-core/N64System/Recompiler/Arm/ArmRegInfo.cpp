#include "stdafx.h"

#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>

CArmRegInfo::CArmRegInfo(CCodeBlock & /*CodeBlock*/, CArmOps & /*Assembler*/)
{
}

CArmRegInfo::CArmRegInfo(const CArmRegInfo &)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CArmRegInfo::~CArmRegInfo()
{
}

CArmRegInfo & CArmRegInfo::operator=(const CArmRegInfo & right)
{
    CRegBase::operator=(right);
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return *this;
}

bool CArmRegInfo::operator==(const CArmRegInfo & /*right*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CArmRegInfo::operator!=(const CArmRegInfo & /*right*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CArmRegInfo::UnMap_GPR(uint32_t Reg, bool WriteBackValue)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

#endif
