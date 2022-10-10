#pragma once
#include "../Settings/N64SystemSettings.h"
#include <Common/HighResTimeStamp.h>

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
    CFramePerSecond(const CFramePerSecond &);
    CFramePerSecond & operator=(const CFramePerSecond &);

    static void FrameRateTypeChanged(CFramePerSecond * _this);
    static void ScreenHertzChanged(CFramePerSecond * _this);
    void UpdateDisplay(void);

    int32_t m_iFrameRateType, m_ScreenHertz;

    enum
    {
        NoOfFrames = 7
    };

    HighResTimeStamp m_LastViFrame;
    uint64_t m_ViFrames[NoOfFrames];
    uint32_t m_CurrentViFrame;
    int32_t m_ViFrameRateWhole;
    uint32_t m_ViFrameRateFraction;

    // Dlist
    HighResTimeStamp m_LastDlistFrame;
    uint64_t m_FramesDlist[NoOfFrames];
    uint32_t m_CurrentDlistFrame;
    float m_DlistFrameRate;
};
