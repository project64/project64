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
#include "stdafx.h"
#include "FramePerSecondClass.h"
#include <Project64-core/N64System/N64Types.h>
#ifdef _WIN32
#include <Windows.h>
#endif

CFramePerSecond::CFramePerSecond() :
m_CurrentViFrame(0),
m_CurrentDlistFrame(0),
m_iFrameRateType(g_Settings->LoadDword(UserInterface_FrameDisplayType)),
m_ScreenHertz(g_Settings->LoadDword(GameRunning_ScreenHertz)),
m_ViFrameRate(0)
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
    m_LastViFrame.SetValue(0);

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
        DisplayViCounter(0);
    }
}

void CFramePerSecond::UpdateViCounter(void)
{
    if (m_iFrameRateType != FR_VIs &&
        m_iFrameRateType != FR_VIs_DLs &&
        m_iFrameRateType != FR_PERCENT)
    {
        return;
    }
    if ((m_CurrentViFrame & 7) == 0)
    {
        CDateTime Time;
        Time.SetToNow();
        m_ViFrames[(m_CurrentViFrame >> 3) % NoOfFrames] = Time.Value() - m_LastViFrame.Value();
        m_LastViFrame = Time;
        DisplayViCounter(0);
    }
    m_CurrentViFrame += 1;
}

void CFramePerSecond::UpdateDisplay(void)
{
    std::string DisplayString;
    if (m_iFrameRateType == FR_VIs || m_iFrameRateType == FR_VIs_DLs)
    {
        DisplayString = stdstr_f(m_ViFrameRate >= 0 ? "VI/s: %.2f" : "VI/s: -.--", m_ViFrameRate);
    }
    if (m_iFrameRateType == FR_PERCENT && m_ViFrameRate > 0)
    {
        float Percent = ((float)m_ViFrameRate) / m_ScreenHertz;
        DisplayString = stdstr_f("%.1f %%", Percent * 100).c_str();
    }
    if (m_iFrameRateType == FR_DLs || m_iFrameRateType == FR_VIs_DLs)
    {
        if (DisplayString.length() > 0) { DisplayString += " "; }
        DisplayString += stdstr_f(m_DlistFrameRate >= 0 ? "DL/s: %.1f" : "DL/s: -.--", m_DlistFrameRate);
    }
    g_Notify->DisplayMessage2(DisplayString.c_str());
}

void CFramePerSecond::DisplayViCounter(uint32_t FrameRate)
{
    if (m_iFrameRateType != FR_VIs && m_iFrameRateType != FR_VIs_DLs && m_iFrameRateType != FR_PERCENT)
    {
        return;
    }
    if (FrameRate != 0)
    {
        m_ViFrameRate = (float)FrameRate;
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
            m_ViFrameRate = ((NoOfFrames << 3) / ((float)Total / 1000));
        }
        else
        {
            m_ViFrameRate = -1.0;
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
    if (m_iFrameRateType != FR_DLs && m_iFrameRateType != FR_VIs_DLs)
    {
        return;
    }
    if ((m_CurrentDlistFrame & 3) == 0)
    {
        CDateTime Time;
        Time.SetToNow();
        m_FramesDlist[(m_CurrentDlistFrame >> 2) % NoOfFrames] = Time.Value() - m_LastDlistFrame.Value();
        m_LastDlistFrame = Time;
        if (m_CurrentDlistFrame > (NoOfFrames << 2))
        {
            int64_t Total;

            Total = 0;
            for (int count = 0; count < NoOfFrames; count++)
            {
                Total += m_FramesDlist[count];
            }
            m_DlistFrameRate = ((NoOfFrames << 2) / ((float)Total / 1000));
        }
        else
        {
            m_DlistFrameRate = -1.0;
        }
        UpdateDisplay();
    }
    m_CurrentDlistFrame += 1;
}