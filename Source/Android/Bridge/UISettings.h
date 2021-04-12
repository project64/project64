#pragma once
#include <string>
#include <stdint.h>

enum UISettingID
{
    Asserts_Version,
    Screen_Orientation,

    // Recent game
    File_RecentGameFileCount,
    File_RecentGameFileIndex,

    // Touch screen
    TouchScreen_ButtonScale,
    TouchScreen_Layout,

    // Controller config
    Controller_ConfigFile,
    Controller_CurrentProfile,
    Controller_Deadzone,
    Controller_Sensitivity,

    // Support window
    SupportWindow_FirstRun,
    SupportWindow_AlwaysShow,
    SupportWindow_ShowingSupportWindow,
    SupportWindow_RunCount,

    // Game settings
    Game_RunCount,
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
