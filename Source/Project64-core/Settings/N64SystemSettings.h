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

class CN64SystemSettings
{
protected:
    CN64SystemSettings();
    virtual ~CN64SystemSettings();

    inline static bool bBasicMode(void) { return m_bBasicMode; }
    inline static bool bDisplayFrameRate(void) { return m_bDisplayFrameRate; }
    inline static bool bShowCPUPer(void) { return m_bShowCPUPer; }
    inline static bool bShowDListAListCount(void) { return m_bShowDListAListCount; }
    inline static bool bLimitFPS(void) { return m_bLimitFPS; }

private:
    static void RefreshSettings(void *);

    static bool m_bShowCPUPer;
    static bool m_bBasicMode;
    static bool m_bLimitFPS;
    static bool m_bShowDListAListCount;
    static bool m_bDisplayFrameRate;

    static int32_t     m_RefCount;
};
