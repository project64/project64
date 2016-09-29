/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>

CArmRegInfo::CArmRegInfo() :
m_InCallDirect(false)
{
}

CArmRegInfo::CArmRegInfo(const CArmRegInfo& rhs)
{
    *this = rhs;
}

CArmRegInfo::~CArmRegInfo()
{
}

CArmRegInfo& CArmRegInfo::operator=(const CArmRegInfo& right)
{
    CRegBase::operator=(right);

    m_InCallDirect = right.m_InCallDirect;
#ifdef _DEBUG
    if (*this != right)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
#endif
    return *this;
}

void CArmRegInfo::BeforeCallDirect(void)
{
    if (m_InCallDirect)
    {
        CPU_Message("%s: in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_InCallDirect = true;
}

void CArmRegInfo::AfterCallDirect(void)
{
    if (!m_InCallDirect)
    {
        CPU_Message("%s: Not in CallDirect",__FUNCTION__);
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_InCallDirect = false;
}

void CArmRegInfo::WriteBackRegisters()
{
}
#endif