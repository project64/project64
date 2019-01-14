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

bool CDebugSettings::m_Registered = false;

bool CDebugSettings::m_HaveDebugger = false;
bool CDebugSettings::m_Debugging = false;
bool CDebugSettings::m_Stepping = false;
bool CDebugSettings::m_SkipOp = false;
bool CDebugSettings::m_WaitingForStep = false;
bool CDebugSettings::m_bRecordRecompilerAsm = false;
bool CDebugSettings::m_bShowTLBMisses = false;
bool CDebugSettings::m_bShowDivByZero = false;
bool CDebugSettings::m_RecordExecutionTimes = false;
bool CDebugSettings::m_HaveExecutionBP = false;
bool CDebugSettings::m_HaveWriteBP = false;
bool CDebugSettings::m_HaveReadBP = false;
bool CDebugSettings::m_bShowPifRamErrors = false;
bool CDebugSettings::m_bCPULoggingEnabled = false;

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
        g_Settings->RegisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_SkipOp, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_HaveExecutionBP, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_WriteBPExists, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_ReadBPExists, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_WaitingForStep, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_ShowPifErrors, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_CPULoggingEnabled, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);

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
        g_Settings->UnregisterChangeCB(Debugger_SteppingOps, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_SkipOp, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_HaveExecutionBP, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_WriteBPExists, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_WaitingForStep, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_ShowPifErrors, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_CPULoggingEnabled, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
    }
}

void CDebugSettings::RefreshSettings()
{
    m_HaveDebugger = g_Settings->LoadBool(Debugger_Enabled);
    m_bRecordRecompilerAsm = m_HaveDebugger && g_Settings->LoadBool(Debugger_RecordRecompilerAsm);
    m_bShowTLBMisses = m_HaveDebugger && g_Settings->LoadBool(Debugger_ShowTLBMisses);
    m_bShowDivByZero = m_HaveDebugger && g_Settings->LoadBool(Debugger_ShowDivByZero);
    m_RecordExecutionTimes = m_HaveDebugger && g_Settings->LoadBool(Debugger_RecordExecutionTimes);
    m_Stepping = m_HaveDebugger && g_Settings->LoadBool(Debugger_SteppingOps);
    m_SkipOp = m_HaveDebugger && g_Settings->LoadBool(Debugger_SkipOp);
    m_WaitingForStep = g_Settings->LoadBool(Debugger_WaitingForStep);
    m_HaveExecutionBP = m_HaveDebugger && g_Settings->LoadBool(Debugger_HaveExecutionBP);
    m_HaveWriteBP = m_HaveDebugger && g_Settings->LoadBool(Debugger_WriteBPExists);
    m_HaveReadBP = m_HaveDebugger && g_Settings->LoadBool(Debugger_ReadBPExists);
    m_bShowPifRamErrors = m_HaveDebugger && g_Settings->LoadBool(Debugger_ShowPifErrors);
    m_bCPULoggingEnabled = m_HaveDebugger && g_Settings->LoadBool(Debugger_CPULoggingEnabled);

    m_Debugging = m_HaveDebugger && (m_HaveExecutionBP || m_WaitingForStep || m_HaveWriteBP || m_HaveReadBP);
}