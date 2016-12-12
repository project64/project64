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

#include <Project64-core/N64System/N64Types.h>

class CDebugSettings
{
public:
    CDebugSettings();
    virtual ~CDebugSettings();

    static inline bool bHaveDebugger(void) { return m_bHaveDebugger; }
    static inline bool bRecordRecompilerAsm(void) { return m_bRecordRecompilerAsm; }
    static inline bool bShowTLBMisses(void) { return m_bShowTLBMisses; }
    static inline bool bShowDivByZero(void) { return m_bShowDivByZero; }
    static inline bool bRecordExecutionTimes(void) { return m_RecordExecutionTimes; }

private:
    static void StaticRefreshSettings(CDebugSettings * _this)
    {
        _this->RefreshSettings();
    }

    void RefreshSettings(void);

    //Settings that can be changed on the fly
    static bool m_bHaveDebugger;
    static bool m_bRecordRecompilerAsm;
    static bool m_bShowTLBMisses;
    static bool m_bShowDivByZero;
    static bool m_RecordExecutionTimes;

    static int32_t m_RefCount;
    static bool m_Registered;
};
