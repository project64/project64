#pragma once
#include <Project64-core\Settings\SettingsID.h>
#include <stdint.h>
#include <string>

enum UISettingID
{
    StartUISetting = SettingID::FirstUISettings,

    AssertsVersion,
    BuildVersion,
    ScreenOrientation,

    //Recent Game
    FileRecentGameFileCount,
    FileRecentGameFileIndex,

    //Touch Screen
    TouchScreenButtonScale,
    TouchScreenLayout,

    //Controller Config
    ControllerConfigFile,
    ControllerCurrentProfile,
    ControllerDeadzone,
    ControllerSensitivity,

    //App Info
    AppInfoRunCount,
};

void RegisterUISettings(void);

void UISettingsSaveBool(UISettingID Type, bool Value);
void UISettingsSaveDword(UISettingID Type, uint32_t Value);
void UISettingsSaveString(UISettingID Type, const std::string & Value);

bool UISettingsLoadBool(UISettingID Type);
uint32_t UISettingsLoadDword(UISettingID Type);
std::string UISettingsLoadStringVal(UISettingID Type);

/*
void UISettingsSaveBoolIndex(UISettingID Type, int32_t index, bool Value);
void UISettingsSaveDwordIndex(UISettingID Type, int32_t index, uint32_t Value);
void UISettingsSaveString(UISettingID Type, const std::string & Value);
void UISettingsSaveStringIndex(UISettingID Type, int32_t index, const std::string & Value);

void UISettingsDeleteSettingIndex(UISettingID Type, int32_t index);

bool UISettingsLoadBoolIndex(UISettingID Type, int32_t index);
bool UISettingsLoadDword(UISettingID Type, uint32_t & Value);
bool UISettingsLoadDwordIndex(UISettingID Type, int index, uint32_t & Value);
bool UISettingsLoadStringIndex(UISettingID Type, int32_t index, std::string & Value);
std::string UISettingsLoadStringIndex(UISettingID Type, int32_t index);
std::string UISettingsLoadStringVal(UISettingID Type);
bool UISettingsLoadStringVal(UISettingID Type, char * Buffer, int32_t BufferSize);
*/
