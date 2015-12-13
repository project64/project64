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

#include <Common/LogClass.h>
#include <Project64-core/N64System/N64Types.h>

class CSystemTimer
{
public:
    enum TimerType
    {
        UnknownTimer,
        CompareTimer,
        SoftResetTimer,
        ViTimer,
        AiTimerInterrupt,
        AiTimerBusy,
        AiTimerDMA,
        SiTimer,
        PiTimer,
        RspTimer,
        RSPTimerDlist,
        MaxTimer
    };

    struct TIMER_DETAILS
    {
        bool    Active;
        __int64 CyclesToTimer;
    };

public:
    CSystemTimer(int & NextTimer);
    void      SetTimer(TimerType Type, uint32_t Cycles, bool bRelative);
    uint32_t  GetTimer(TimerType Type);
    void      StopTimer(TimerType Type);
    void      UpdateTimers();
    void      TimerDone();
    void      Reset();
    void      UpdateCompareTimer();
    bool      SaveAllowed();

    void      SaveData(void * file) const;
    void      LoadData(void * file);

    void RecordDifference(CLog &LogFile, const CSystemTimer& rSystemTimer);

    TimerType CurrentType() const { return m_Current; }

    bool operator == (const CSystemTimer& rSystemTimer) const;
    bool operator != (const CSystemTimer& rSystemTimer) const;

private:
    TIMER_DETAILS m_TimerDetatils[MaxTimer];
    int           m_LastUpdate; //Timer at last update
    int         & m_NextTimer;
    TimerType     m_Current;
    bool          m_inFixTimer;

    void SetCompareTimer();
    void FixTimers();
};
