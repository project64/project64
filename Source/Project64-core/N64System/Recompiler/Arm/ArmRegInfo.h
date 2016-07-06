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
#pragma once

#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/RegBase.h>

class CArmRegInfo :
    public CRegBase
{
public:
    bool operator==(const CArmRegInfo& right) const;
};

#endif
