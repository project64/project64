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
#include <Project64-core/Settings/SettingsClass.h>
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/Settings/SettingType/SettingsType-ApplicationIndex.h>
#include "UISettings.h"

void RegisterUISettings(void)
{
    g_Settings->AddHandler((SettingID)(FirstUISettings + Asserts_Version), new CSettingTypeApplication("", "Asserts Version", (uint32_t)0));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Screen_Orientation), new CSettingTypeApplication("", "Screen Orientation", (uint32_t)0));
    g_Settings->AddHandler((SettingID)(FirstUISettings + File_RecentGameFileCount), new CSettingTypeApplication("", "Remembered Rom Files", (uint32_t)10));
    g_Settings->AddHandler((SettingID)(FirstUISettings + File_RecentGameFileIndex), new CSettingTypeApplicationIndex("Recent File", "Recent Rom", Default_None));
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

bool UISettingsLoadBool(UISettingID Type)
{
    return g_Settings->LoadBool((SettingID)(FirstUISettings + Type));
}

uint32_t UISettingsLoadDword(UISettingID Type)
{
    return g_Settings->LoadDword((SettingID)(FirstUISettings + Type));
}
