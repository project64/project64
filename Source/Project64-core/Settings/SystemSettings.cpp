#include "stdafx.h"

#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/Settings/SystemSettings.h>

SystemSettings g_SystemSettings = {};

void RefreshSystemSettings(void)
{
    if (g_Settings == nullptr)
    {
        return;
    }
    g_SystemSettings.basicMode = g_Settings->LoadBool(UserInterface_BasicMode);
    g_SystemSettings.displayFrameRate = g_Settings->LoadBool(UserInterface_DisplayFrameRate);
    g_SystemSettings.showCpuPer = g_Settings->LoadBool(UserInterface_ShowCPUPer);
    g_SystemSettings.showDlistAListCount = g_Settings->LoadBool(Debugger_ShowDListAListCount);
    g_SystemSettings.limitFps = g_Settings->LoadBool(GameRunning_LimitFPS);
    g_SystemSettings.updateControllerOnRefresh = g_Settings->LoadBool(Setting_UpdateControllerOnRefresh);
    if (g_Settings->LoadDword(Game_CpuType) == CPU_SyncCores)
    {
        g_SystemSettings.updateControllerOnRefresh = true;
    }
    g_SystemSettings.showRecompMemSize = g_Settings->LoadBool(Debugger_ShowRecompMemSize);
}

static bool s_SystemSettingsRegistered = false;

static void SystemSettingsChanged(void * /*Data*/)
{
    RefreshSystemSettings();
}

void SetupSystemSettings(void)
{
    if (g_Settings == nullptr || s_SystemSettingsRegistered)
    {
        return;
    }

    static const SettingID kWatch[] = {
        UserInterface_BasicMode,
        UserInterface_ShowCPUPer,
        UserInterface_DisplayFrameRate,
        Debugger_ShowDListAListCount,
        Debugger_ShowRecompMemSize,
        GameRunning_LimitFPS,
        Setting_UpdateControllerOnRefresh,
        Game_CpuType,
    };

    for (SettingID id : kWatch)
    {
        g_Settings->RegisterChangeCB(id, nullptr, SystemSettingsChanged);
    }

    RefreshSystemSettings();
    s_SystemSettingsRegistered = true;
}

void ShutdownSystemSettings(void)
{
    if (g_Settings == nullptr || !s_SystemSettingsRegistered)
    {
        return;
    }

    static const SettingID kWatch[] = {
        UserInterface_BasicMode,
        UserInterface_ShowCPUPer,
        UserInterface_DisplayFrameRate,
        Debugger_ShowDListAListCount,
        Debugger_ShowRecompMemSize,
        GameRunning_LimitFPS,
        Setting_UpdateControllerOnRefresh,
        Game_CpuType,
    };

    for (SettingID id : kWatch)
    {
        g_Settings->UnregisterChangeCB(id, nullptr, SystemSettingsChanged);
    }

    s_SystemSettingsRegistered = false;
}
