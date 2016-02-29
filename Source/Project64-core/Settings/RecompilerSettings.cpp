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
#include "RecompilerSettings.h"

int   CRecompilerSettings::m_RefCount = 0;

bool  CRecompilerSettings::m_bShowRecompMemSize;
bool  CRecompilerSettings::m_bProfiling;

CRecompilerSettings::CRecompilerSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(Debugger_ShowRecompMemSize, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_ProfileCode, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);

        RefreshSettings();
    }
}

CRecompilerSettings::~CRecompilerSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(Debugger_ShowRecompMemSize, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_ProfileCode, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
    }
}

void CRecompilerSettings::RefreshSettings()
{
    m_bShowRecompMemSize = g_Settings->LoadBool(Debugger_ShowRecompMemSize);
    m_bProfiling = g_Settings->LoadBool(Debugger_ProfileCode);
}