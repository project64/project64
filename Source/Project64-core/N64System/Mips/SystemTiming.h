#pragma once

#include <Common/Log.h>
#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/N64System/Mips/Register.h>
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
        DDSeekTimer,
        DDMotorTimer,
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

    CSystemTimer(CRegisters &Reg, int32_t & NextTimer);
    void SetTimer(TimerType Type, uint32_t Cycles, bool bRelative);
    uint32_t  GetTimer(TimerType Type);
    void StopTimer(TimerType Type);
    void UpdateTimers();
    void TimerDone();
    void Reset();
    void UpdateCompareTimer();
    bool SaveAllowed();

    void SaveData(zipFile & file) const;
    void SaveData(CFile & file) const;
    void LoadData(zipFile & file);
    void LoadData(CFile & file);

    void RecordDifference(CLog &LogFile, const CSystemTimer& rSystemTimer);

    TimerType CurrentType() const { return m_Current; }

    bool operator == (const CSystemTimer& rSystemTimer) const;
    bool operator != (const CSystemTimer& rSystemTimer) const;

private:
    CSystemTimer(void);
    CSystemTimer(const CSystemTimer&);
    CSystemTimer& operator=(const CSystemTimer&);

    void SetCompareTimer();
    void FixTimers();

    TIMER_DETAILS m_TimerDetatils[MaxTimer];
    int32_t m_LastUpdate;
    int32_t & m_NextTimer;
    TimerType m_Current;
    bool m_inFixTimer;
    CRegisters & m_Reg;
};
