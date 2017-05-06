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
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/3rdParty/zip.h>

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
        DDPiTimer,
        MaxTimer
    };

    struct TIMER_DETAILS
    {
        union 
        {
            int64_t reserved;
            bool Active;
        };
        int64_t CyclesToTimer;
    };

public:
    CSystemTimer(CRegisters &Reg, int32_t & NextTimer);
    void      SetTimer(TimerType Type, uint32_t Cycles, bool bRelative);
    uint32_t  GetTimer(TimerType Type);
    void      StopTimer(TimerType Type);
    void      UpdateTimers();
    void      TimerDone();
    void      Reset();
    void      UpdateCompareTimer();
    bool      SaveAllowed();

    void      SaveData(zipFile & file) const;
    void      SaveData(CFile & file) const;
    void      LoadData(zipFile & file);
    void      LoadData(CFile & file);

    void RecordDifference(CLog &LogFile, const CSystemTimer& rSystemTimer);

    TimerType CurrentType() const { return m_Current; }

    bool operator == (const CSystemTimer& rSystemTimer) const;
    bool operator != (const CSystemTimer& rSystemTimer) const;

private:
    CSystemTimer(void);                           // Disable default constructor
    CSystemTimer(const CSystemTimer&);            // Disable copy constructor
    CSystemTimer& operator=(const CSystemTimer&); // Disable assignment

    TIMER_DETAILS m_TimerDetatils[MaxTimer];
    int32_t       m_LastUpdate; //Timer at last update
    int32_t     & m_NextTimer;
    TimerType     m_Current;
    bool          m_inFixTimer;
    CRegisters  & m_Reg;

    void SetCompareTimer();
    void FixTimers();
};
