#include "UISettings.h"
#include <Project64-core/Settings.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/Settings/SettingType/SettingsType-ApplicationIndex.h>
#include <Project64-core/Settings/SettingType/SettingsType-GameSetting.h>
#include <Project64-core/Settings/SettingType/SettingsType-RelativePath.h>
#include <Project64-core/Settings/SettingType/SettingsType-TempNumber.h>
#include <Project64-core/Version.h>

void RegisterUISettings(void)
{
    g_Settings->AddHandler((SettingID)AssertsVersion, new CSettingTypeApplication("Settings", "Asserts Version", (uint32_t)0));
    g_Settings->AddHandler((SettingID)BuildVersion, new CSettingTypeTempNumber(VERSION_BUILD));
    g_Settings->AddHandler((SettingID)ScreenOrientation, new CSettingTypeApplication("Settings", "Screen Orientation", (uint32_t)0));
    g_Settings->AddHandler((SettingID)FileRecentGameFileCount, new CSettingTypeApplication("Settings", "Remembered ROM Files", (uint32_t)10));
    g_Settings->AddHandler((SettingID)FileRecentGameFileIndex, new CSettingTypeApplicationIndex("Recent File", "Recent ROM", Default_None));
    g_Settings->AddHandler((SettingID)TouchScreenButtonScale, new CSettingTypeApplication("Touch Screen", "Button Scale", (uint32_t)100));
    g_Settings->AddHandler((SettingID)TouchScreenLayout, new CSettingTypeApplication("Touch Screen", "Layout", "Analog"));
    g_Settings->AddHandler((SettingID)ControllerConfigFile, new CSettingTypeRelativePath("Config", "Controller.cfg"));
    g_Settings->AddHandler((SettingID)ControllerCurrentProfile, new CSettingTypeApplication("Controller", "Profile", "User"));
    g_Settings->AddHandler((SettingID)ControllerDeadzone, new CSettingTypeApplication("Controller", "Deadzone", (uint32_t)0));
    g_Settings->AddHandler((SettingID)ControllerSensitivity, new CSettingTypeApplication("Controller", "Sensitivity", (uint32_t)100));
    g_Settings->AddHandler((SettingID)AppInfoRunCount, new CSettingTypeGame("Run Count", (uint32_t)0));
}

void UISettingsSaveBool(UISettingID Type, bool Value)
{
    g_Settings->SaveBool((SettingID)(FirstUISettings + Type), Value);
    CSettings::FlushSettings(g_Settings);
}

void UISettingsSaveDword(UISettingID Type, uint32_t Value)
{
    g_Settings->SaveDword((SettingID)(FirstUISettings + Type), Value);
    CSettings::FlushSettings(g_Settings);
}

void UISettingsSaveString(UISettingID Type, const std::string & Value)
{
    g_Settings->SaveString((SettingID)(FirstUISettings + Type), Value);
    CSettings::FlushSettings(g_Settings);
}

bool UISettingsLoadBool(UISettingID Type)
{
    return g_Settings->LoadBool((SettingID)(FirstUISettings + Type));
}

uint32_t UISettingsLoadDword(UISettingID Type)
{
    return g_Settings->LoadDword((SettingID)(FirstUISettings + Type));
}

std::string UISettingsLoadStringVal(UISettingID Type)
{
    return g_Settings->LoadStringVal((SettingID)(FirstUISettings + Type));
}
