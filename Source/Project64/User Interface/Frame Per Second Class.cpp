/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

CFramePerSecond::CFramePerSecond ()
{
	m_iFrameRateType = g_Settings->LoadDword(UserInterface_FrameDisplayType);
	m_ScreenHertz = g_Settings->LoadDword(GameRunning_ScreenHertz);
	g_Settings->RegisterChangeCB(UserInterface_FrameDisplayType,this,(CSettings::SettingChangedFunc)FrameRateTypeChanged);
	g_Settings->RegisterChangeCB(GameRunning_ScreenHertz,this,(CSettings::SettingChangedFunc)ScreenHertzChanged);
	
	if (m_ScreenHertz == 0)
	{
		m_ScreenHertz = 60;
	}
	
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	m_Frequency = Freq.QuadPart;
	Reset(true);
}

CFramePerSecond::~CFramePerSecond()
{
	g_Settings->UnregisterChangeCB(UserInterface_FrameDisplayType,this,(CSettings::SettingChangedFunc)FrameRateTypeChanged);
	g_Settings->UnregisterChangeCB(GameRunning_ScreenHertz,this,(CSettings::SettingChangedFunc)ScreenHertzChanged);
}

void CFramePerSecond::Reset (bool ClearDisplay) 
{
	m_CurrentFrame = 0;
	m_LastFrame = 0;

	for (int count = 0; count < NoOfFrames; count ++) 
    {
		m_Frames[count] = 0;
	}
	if (ClearDisplay)
	{
		g_Notify->DisplayMessage2(L"");
		return;
	}
	
	if (m_iFrameRateType == FR_VIs) 
	{
		DisplayViCounter(0);
	}
}

void CFramePerSecond::UpdateViCounter ( void )
{
	if (m_iFrameRateType != FR_VIs && m_iFrameRateType != FR_PERCENT)
	{
		return;
	}
	if ((m_CurrentFrame & 7) == 0)
	{
		LARGE_INTEGER Time;
		QueryPerformanceCounter(&Time);
		m_Frames[(m_CurrentFrame >> 3) % NoOfFrames] = Time.QuadPart - m_LastFrame;
		m_LastFrame = Time.QuadPart;
		DisplayViCounter(0);
	}
	m_CurrentFrame += 1;
}

void CFramePerSecond::DisplayViCounter(DWORD FrameRate) 
{
	if (m_iFrameRateType == FR_VIs)
	{
		if (FrameRate != 0) 
        {
			g_Notify->DisplayMessage2(stdstr_f("VI/s: %d.00", FrameRate).ToUTF16().c_str());
		} 
        else 
        {
			if (m_CurrentFrame > (NoOfFrames << 3))
            {
				__int64 Total;
				
				Total = 0;
				for (int count = 0; count < NoOfFrames; count ++) 
                {
					Total += m_Frames[count];
				}
				g_Notify->DisplayMessage2(stdstr_f("VI/s: %.2f", m_Frequency / ((double)Total / (NoOfFrames << 3))).ToUTF16().c_str());
			}
            else 
            {
				g_Notify->DisplayMessage2(L"VI/s: -.--");
			}
		}
	}
	if (m_iFrameRateType == FR_PERCENT)
	{
		float Percent;
		if (FrameRate != 0) 
        {
			Percent = ((float)FrameRate) / m_ScreenHertz;
		}
        else 
        {
			if (m_CurrentFrame > (NoOfFrames << 3))
            {
				__int64 Total;
				
				Total = 0;
				for (int count = 0; count < NoOfFrames; count ++) 
                {
					Total += m_Frames[count];
				}
				Percent = ((float)(m_Frequency / ((double)Total / (NoOfFrames << 3)))) / m_ScreenHertz;
			}
            else 
            {
				g_Notify->DisplayMessage2(L"");
				return;
			}
		}
		g_Notify->DisplayMessage2(stdstr_f("%.1f %%",Percent * 100).ToUTF16().c_str());
	}
}

void CFramePerSecond::FrameRateTypeChanged (CFramePerSecond * _this)
{
	_this->m_iFrameRateType = g_Settings->LoadDword(UserInterface_FrameDisplayType);
	_this->Reset(true);
}

void CFramePerSecond::ScreenHertzChanged (CFramePerSecond * _this)
{
	_this->m_ScreenHertz = g_Settings->LoadDword(GameRunning_ScreenHertz);
	_this->Reset(true);
}

void CFramePerSecond::UpdateDlCounter  ( void )
{
	if (m_iFrameRateType != FR_DLs)
	{
		return;
	}
	if ((m_CurrentFrame & 3) == 0) {
		LARGE_INTEGER Time;
		QueryPerformanceCounter(&Time);
		m_Frames[(m_CurrentFrame >> 2) % NoOfFrames] = Time.QuadPart - m_LastFrame;
		m_LastFrame = Time.QuadPart;
		DisplayDlCounter(0);
	}
	m_CurrentFrame += 1;
}

void CFramePerSecond::DisplayDlCounter(DWORD FrameRate) 
{
	if (m_iFrameRateType != FR_DLs)
	{
		return;
	}
	if (FrameRate != 0)
	{
		g_Notify->DisplayMessage2(stdstr_f("DL/s: %d.00", FrameRate).ToUTF16().c_str());
	}
	else
	{
		if (m_CurrentFrame > (NoOfFrames << 2)) 
		{
			__int64 Total;
			
			Total = 0;
			for (int count = 0; count < NoOfFrames; count ++) 
			{
				Total += m_Frames[count];
			}
			g_Notify->DisplayMessage2(stdstr_f("DL/s: %.1f", m_Frequency / ((double)Total / (NoOfFrames << 2))).ToUTF16().c_str());
		} 
		else 
		{
			g_Notify->DisplayMessage2(L"DL/s: -.--");
		}
	}
}
