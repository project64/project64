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
#include <Common/HighResTimeStamp.h>
#include "../Settings/N64SystemSettings.h"

class CFramePerSecond : public CN64SystemSettings
{
public:
    CFramePerSecond(void);
    ~CFramePerSecond(void);

    void Reset(bool ClearDisplay);

    void UpdateDlCounter(void);
    void UpdateViCounter(void);
    void DisplayViCounter(int32_t FrameRateWhole, uint32_t FrameRateFraction);

private:
    CFramePerSecond(const CFramePerSecond&);            // Disable copy constructor
    CFramePerSecond& operator=(const CFramePerSecond&); // Disable assignment

    static void FrameRateTypeChanged(CFramePerSecond * _this);
    static void ScreenHertzChanged(CFramePerSecond * _this);
    void UpdateDisplay(void);

    int32_t  m_iFrameRateType, m_ScreenHertz;

    enum { NoOfFrames = 7 };

    HighResTimeStamp m_LastViFrame;
    uint64_t m_ViFrames[NoOfFrames];
    uint32_t m_CurrentViFrame;
    int32_t m_ViFrameRateWhole;
    uint32_t m_ViFrameRateFraction;

    //Dlist
    HighResTimeStamp m_LastDlistFrame;
    uint64_t m_FramesDlist[NoOfFrames];
    uint32_t m_CurrentDlistFrame;
    float m_DlistFrameRate;
};
