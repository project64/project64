#pragma once

#include <Project64-core/Settings/GameSettings.h>

#include <Common/HighResTimeStamp.h>

class CSpeedLimiter :
    private CGameSettings
{
public:
    enum ESpeedChange
    {
        INCREASE_SPEED,
        DECREASE_SPEED
    };

    CSpeedLimiter();
    ~CSpeedLimiter();

    void SetHertz(const uint32_t Hertz);
    bool Timer_Process(uint32_t * const FrameRate);

    void AlterSpeed(const ESpeedChange SpeedChange);

    void SetSpeed(int Speed);
    int GetSpeed(void) const;
    int GetBaseSpeed(void) const;

private:
    CSpeedLimiter(const CSpeedLimiter &);
    CSpeedLimiter & operator=(const CSpeedLimiter &);

    void FixSpeedRatio();

    HighResTimeStamp m_LastTime;

    uint32_t m_Speed, m_BaseSpeed, m_Frames, m_MicroSecondsPerFrame;

    static const uint32_t m_DefaultSpeed;
};
