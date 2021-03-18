#include "stdafx.h"

int  CGuiSettings::m_RefCount = 0;
bool CGuiSettings::m_bCPURunning;
bool CGuiSettings::m_bAutoSleep;

CGuiSettings::CGuiSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
        g_Settings->RegisterChangeCB((SettingID)Setting_AutoSleep,NULL,RefreshSettings);
        RefreshSettings(NULL);
    }
}

CGuiSettings::~CGuiSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Running,NULL,RefreshSettings);
        g_Settings->UnregisterChangeCB((SettingID)Setting_AutoSleep,NULL,RefreshSettings);
    }
}

void CGuiSettings::RefreshSettings(void *)
{
    m_bCPURunning = g_Settings->LoadBool(GameRunning_CPU_Running);
    m_bAutoSleep = g_Settings->LoadBool((SettingID)Setting_AutoSleep);
}
