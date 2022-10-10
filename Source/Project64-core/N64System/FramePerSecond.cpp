#include "stdafx.h"

#include "FramePerSecond.h"
#include <Project64-core/N64System/N64Types.h>

CFramePerSecond::CFramePerSecond() :
    m_CurrentViFrame(0),
    m_CurrentDlistFrame(0),
    m_iFrameRateType(g_Settings->LoadDword(UserInterface_FrameDisplayType)),
    m_ScreenHertz(g_Settings->LoadDword(GameRunning_ScreenHertz)),
    m_ViFrameRateWhole(0),
    m_ViFrameRateFraction(0)
{
    g_Settings->RegisterChangeCB(UserInterface_FrameDisplayType, this, (CSettings::SettingChangedFunc)FrameRateTypeChanged);
    g_Settings->RegisterChangeCB(GameRunning_ScreenHertz, this, (CSettings::SettingChangedFunc)ScreenHertzChanged);

    if (m_ScreenHertz == 0)
    {
        m_ScreenHertz = 60;
    }
    Reset(true);
}

CFramePerSecond::~CFramePerSecond()
{
    g_Settings->UnregisterChangeCB(UserInterface_FrameDisplayType, this, (CSettings::SettingChangedFunc)FrameRateTypeChanged);
    g_Settings->UnregisterChangeCB(GameRunning_ScreenHertz, this, (CSettings::SettingChangedFunc)ScreenHertzChanged);
}

void CFramePerSecond::Reset(bool ClearDisplay)
{
    m_CurrentDlistFrame = 0;
    m_CurrentViFrame = 0;
    m_LastViFrame.SetMicroSeconds(0);

    for (int count = 0; count < NoOfFrames; count++)
    {
        m_ViFrames[count] = 0;
        m_FramesDlist[count] = 0;
    }
    if (ClearDisplay)
    {
        g_Notify->DisplayMessage2("");
        return;
    }

    if (m_iFrameRateType == FR_VIs)
    {
        DisplayViCounter(-1, 0);
    }
}

void CFramePerSecond::UpdateViCounter(void)
{
    if (!bDisplayFrameRate())
    {
        return;
    }
    if (m_iFrameRateType != FR_VIs && m_iFrameRateType != FR_VIs_DLs && m_iFrameRateType != FR_PERCENT)
    {
        return;
    }
    if ((m_CurrentViFrame & 7) == 0)
    {
        HighResTimeStamp Time;
        Time.SetToNow();

        uint64_t time_diff = Time.GetMicroSeconds() - m_LastViFrame.GetMicroSeconds();
        m_ViFrames[(m_CurrentViFrame >> 3) % NoOfFrames] = time_diff;
        m_LastViFrame = Time;
        DisplayViCounter(-1, 0);
    }
    m_CurrentViFrame += 1;
}

void CFramePerSecond::UpdateDisplay(void)
{
    std::string DisplayString;
    if (m_iFrameRateType == FR_VIs || m_iFrameRateType == FR_VIs_DLs)
    {
        DisplayString = stdstr_f(m_ViFrameRateWhole >= 0 ? "VI/s: %d.%d" : "VI/s: -.--", m_ViFrameRateWhole, m_ViFrameRateFraction);
    }
    if (m_iFrameRateType == FR_PERCENT && m_ViFrameRateWhole > 0)
    {
        float Percent = ((float)m_ViFrameRateWhole + ((float)m_ViFrameRateFraction / 100)) / m_ScreenHertz;
        DisplayString = stdstr_f("%.1f %%", Percent * 100).c_str();
    }
    if (m_iFrameRateType == FR_DLs || m_iFrameRateType == FR_VIs_DLs)
    {
        if (DisplayString.length() > 0)
        {
            DisplayString += " ";
        }
        DisplayString += stdstr_f(m_DlistFrameRate >= 0 ? "DL/s: %.1f" : "DL/s: -.--", m_DlistFrameRate);
    }
    g_Notify->DisplayMessage2(DisplayString.c_str());
}

void CFramePerSecond::DisplayViCounter(int32_t FrameRateWhole, uint32_t FrameRateFraction)
{
    if (m_iFrameRateType != FR_VIs && m_iFrameRateType != FR_VIs_DLs && m_iFrameRateType != FR_PERCENT)
    {
        return;
    }
    if (FrameRateWhole >= 0)
    {
        m_ViFrameRateWhole = FrameRateWhole;
        m_ViFrameRateFraction = FrameRateFraction;
    }
    else
    {
        if (m_CurrentViFrame > (NoOfFrames << 3))
        {
            uint64_t Total;

            Total = 0;
            for (int count = 0; count < NoOfFrames; count++)
            {
                Total += m_ViFrames[count];
            }
            int baseFPS = (uint32_t)(((uint64_t)NoOfFrames << 3) * 100000000 / Total);
            m_ViFrameRateWhole = baseFPS / 100;
            m_ViFrameRateFraction = baseFPS % 100;
        }
        else
        {
            m_ViFrameRateWhole = -1;
            m_ViFrameRateFraction = 0;
        }
    }
    UpdateDisplay();
}

void CFramePerSecond::FrameRateTypeChanged(CFramePerSecond * _this)
{
    _this->m_iFrameRateType = g_Settings->LoadDword(UserInterface_FrameDisplayType);
    _this->Reset(true);
}

void CFramePerSecond::ScreenHertzChanged(CFramePerSecond * _this)
{
    _this->m_ScreenHertz = g_Settings->LoadDword(GameRunning_ScreenHertz);
    _this->Reset(true);
}

void CFramePerSecond::UpdateDlCounter(void)
{
    if (!bDisplayFrameRate())
    {
        return;
    }
    if (m_iFrameRateType != FR_DLs && m_iFrameRateType != FR_VIs_DLs)
    {
        return;
    }
    if ((m_CurrentDlistFrame & 3) == 0)
    {
        HighResTimeStamp Now;
        Now.SetToNow();
        m_FramesDlist[(m_CurrentDlistFrame >> 2) % NoOfFrames] = Now.GetMicroSeconds() - m_LastDlistFrame.GetMicroSeconds();
        m_LastDlistFrame = Now;
        if (m_CurrentDlistFrame > (NoOfFrames << 2))
        {
            int64_t Total;

            Total = 0;
            for (int count = 0; count < NoOfFrames; count++)
            {
                Total += m_FramesDlist[count];
            }
            m_DlistFrameRate = ((NoOfFrames << 2) / ((float)Total / 1000000));
        }
        else
        {
            m_DlistFrameRate = -1.0;
        }
        UpdateDisplay();
    }
    m_CurrentDlistFrame += 1;
}