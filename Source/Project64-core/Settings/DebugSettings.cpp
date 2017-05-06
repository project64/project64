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
#include "DebugSettings.h"

int CDebugSettings::m_RefCount = 0;

bool CDebugSettings::m_bHaveDebugger = false;
bool CDebugSettings::m_bRecordRecompilerAsm = false;
bool CDebugSettings::m_bShowTLBMisses = false;
bool CDebugSettings::m_bShowDivByZero = false;
bool CDebugSettings::m_Registered = false;
bool CDebugSettings::m_RecordExecutionTimes = false;

CDebugSettings::CDebugSettings()
{
    m_RefCount += 1;
    if (!m_Registered && g_Settings)
    {
        m_Registered = true;
        g_Settings->RegisterChangeCB(Debugger_Enabled, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_RecordRecompilerAsm, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_ShowTLBMisses, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_ShowDivByZero, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_RecordExecutionTimes, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);

        RefreshSettings();
    }
}

CDebugSettings::~CDebugSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0 && g_Settings)
    {
        g_Settings->UnregisterChangeCB(Debugger_Enabled, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_RecordRecompilerAsm, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_ShowTLBMisses, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_ShowDivByZero, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_RecordExecutionTimes, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
    }
}

void CDebugSettings::RefreshSettings()
{
    m_bHaveDebugger = g_Settings->LoadBool(Debugger_Enabled);
    m_bRecordRecompilerAsm = m_bHaveDebugger && g_Settings->LoadBool(Debugger_RecordRecompilerAsm);
    m_bShowTLBMisses = m_bHaveDebugger && g_Settings->LoadBool(Debugger_ShowTLBMisses);
    m_bShowDivByZero = m_bHaveDebugger && g_Settings->LoadBool(Debugger_ShowDivByZero);
    m_RecordExecutionTimes = g_Settings->LoadBool(Debugger_RecordExecutionTimes);
}