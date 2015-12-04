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

#include <Project64/N64System/N64Types.h>

class CRecompilerSettings
{
public:
    CRecompilerSettings();
    virtual ~CRecompilerSettings();

    static bool  bShowRecompMemSize(void) { return m_bShowRecompMemSize; }

    static bool  bProfiling(void) { return m_bProfiling; }

private:
    static void StaticRefreshSettings(CRecompilerSettings * _this)
    {
        _this->RefreshSettings();
    }

    void RefreshSettings(void);

    //Settings that can be changed on the fly
    static bool m_bShowRecompMemSize;
    static bool m_bProfiling;

    static int32_t m_RefCount;
};
