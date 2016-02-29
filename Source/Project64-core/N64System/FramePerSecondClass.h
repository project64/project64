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

class CFramePerSecond
{
public:
    CFramePerSecond(void);
    ~CFramePerSecond(void);

    void Reset(bool ClearDisplay);

    void UpdateDlCounter(void);
    void UpdateViCounter(void);
    void DisplayDlCounter(uint32_t FrameRate);
    void DisplayViCounter(uint32_t FrameRate);

private:
    CFramePerSecond(const CFramePerSecond&);            // Disable copy constructor
    CFramePerSecond& operator=(const CFramePerSecond&); // Disable assignment

    static void FrameRateTypeChanged(CFramePerSecond * _this);
    static void ScreenHertzChanged(CFramePerSecond * _this);

    int32_t  m_iFrameRateType, m_ScreenHertz;

    enum { NoOfFrames = 7 };

    int64_t m_Frequency, m_Frames[NoOfFrames], m_LastFrame;
    int32_t m_CurrentFrame;
};
