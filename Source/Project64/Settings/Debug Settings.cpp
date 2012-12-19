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

int CDebugSettings::m_RefCount = 0; 

bool CDebugSettings::m_bHaveDebugger = false;
bool CDebugSettings::m_bLogX86Code = false;
bool CDebugSettings::m_bShowTLBMisses = false;
bool CDebugSettings::m_bShowDivByZero = false;

CDebugSettings::CDebugSettings()
{
	m_RefCount += 1;
	if (m_RefCount == 1)
	{
		g_Settings->RegisterChangeCB(Debugger_Enabled,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Debugger_GenerateLogFiles,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Debugger_ShowTLBMisses,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->RegisterChangeCB(Debugger_ShowDivByZero,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		
		RefreshSettings();
	}
}

CDebugSettings::~CDebugSettings()
{
	m_RefCount -= 1;
	if (m_RefCount == 0)
	{
		g_Settings->UnregisterChangeCB(Debugger_Enabled,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Debugger_GenerateLogFiles,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Debugger_ShowTLBMisses,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
		g_Settings->UnregisterChangeCB(Debugger_ShowDivByZero,this,(CSettings::SettingChangedFunc)StaticRefreshSettings);
	}
}

void CDebugSettings::RefreshSettings()
{
	m_bHaveDebugger = g_Settings->LoadBool(Debugger_Enabled);
	m_bLogX86Code = m_bHaveDebugger && g_Settings->LoadBool(Debugger_GenerateLogFiles);
	m_bShowTLBMisses = m_bHaveDebugger && g_Settings->LoadBool(Debugger_ShowTLBMisses);
	m_bShowDivByZero = m_bHaveDebugger && g_Settings->LoadBool(Debugger_ShowDivByZero);
}
