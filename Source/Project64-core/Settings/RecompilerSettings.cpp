#include "stdafx.h"
#include "RecompilerSettings.h"

int CRecompilerSettings::m_RefCount = 0;

bool CRecompilerSettings::m_bShowRecompMemSize;

CRecompilerSettings::CRecompilerSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(Debugger_ShowRecompMemSize, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);

        RefreshSettings();
    }
}

CRecompilerSettings::~CRecompilerSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(Debugger_ShowRecompMemSize, this, (CSettings::SettingChangedFunc)StaticRefreshSettings);
    }
}

void CRecompilerSettings::RefreshSettings()
{
    m_bShowRecompMemSize = g_Settings->LoadBool(Debugger_ShowRecompMemSize);
}