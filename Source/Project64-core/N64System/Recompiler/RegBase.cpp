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
#include <Project64-core/N64System/Recompiler/RegBase.h>

CRegBase::CRegBase() :
m_CycleCount(0)
{
    m_MIPS_RegState[0] = STATE_CONST_32_SIGN;
    for (int32_t i = 1; i < 32; i++)
    {
        m_MIPS_RegState[i] = STATE_UNKNOWN;
        m_MIPS_RegVal[i].DW = 0;
    }
}