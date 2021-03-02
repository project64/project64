#pragma once
#include <Project64-core/N64System/N64Types.h>
#include <Common/HighResTimeStamp.h>

class CProfiling
{
public:
    CProfiling();

    void RecordTime(PROFILE_TIMERS timer, uint32_t time);
    uint64_t NonCPUTime(void);

    //recording timing against current timer, returns the address of the timer stopped
    PROFILE_TIMERS StartTimer(PROFILE_TIMERS TimerType);
    PROFILE_TIMERS StopTimer();

    //Display the CPU Usage
    void ShowCPU_Usage();

    void ResetTimers(void);

private:
    CProfiling(const CProfiling&);            // Disable copy constructor
    CProfiling& operator=(const CProfiling&); // Disable assignment

    uint32_t m_CurrentDisplayCount;
    PROFILE_TIMERS m_CurrentTimerType;
    HighResTimeStamp m_StartTime;
    uint64_t m_Timers[Timer_Max];
};
