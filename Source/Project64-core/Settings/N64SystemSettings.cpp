#include "stdafx.h"
#include "N64SystemSettings.h"
#include <Project64-core/N64System/N64Types.h>

int32_t CN64SystemSettings::m_RefCount = 0;

bool CN64SystemSettings::m_bShowCPUPer;
bool CN64SystemSettings::m_bBasicMode;
bool CN64SystemSettings::m_bLimitFPS;
bool CN64SystemSettings::m_bShowDListAListCount;
bool CN64SystemSettings::m_bDisplayFrameRate;
bool CN64SystemSettings::m_UpdateControllerOnRefresh;

CN64SystemSettings::CN64SystemSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(UserInterface_BasicMode, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(UserInterface_ShowCPUPer, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(UserInterface_DisplayFrameRate, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(Debugger_ShowDListAListCount, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB(GameRunning_LimitFPS, nullptr, RefreshSettings);

        RefreshSettings(nullptr);
    }
}

CN64SystemSettings::~CN64SystemSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(UserInterface_BasicMode, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(UserInterface_DisplayFrameRate, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(UserInterface_ShowCPUPer, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(Debugger_ShowDListAListCount, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB(GameRunning_LimitFPS, nullptr, RefreshSettings);
    }
}

void CN64SystemSettings::RefreshSettings(void *)
{
    m_bBasicMode = g_Settings->LoadBool(UserInterface_BasicMode);
    m_bDisplayFrameRate = g_Settings->LoadBool(UserInterface_DisplayFrameRate);
    m_bShowCPUPer = g_Settings->LoadBool(UserInterface_ShowCPUPer);
    m_bShowDListAListCount = g_Settings->LoadBool(Debugger_ShowDListAListCount);
    m_bLimitFPS = g_Settings->LoadBool(GameRunning_LimitFPS);
    m_UpdateControllerOnRefresh = g_Settings->LoadBool(Setting_UpdateControllerOnRefresh);
    if (g_Settings->LoadDword(Game_CpuType) == CPU_SyncCores)
    {
        m_UpdateControllerOnRefresh = true;
    }
}