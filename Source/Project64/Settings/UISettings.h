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
#pragma once
#include <string>
#include <Common/stdtypes.h>

enum UISettingID
{
    //information - temp keys
    Info_ShortCutsChanged,

    //Support Files
    SupportFile_ShortCuts,
    SupportFile_ShortCutsDefault,

    //Settings location
    Setting_PluginPageFirst,
    Setting_DisableScrSaver,
    Setting_EnableDiscordRPC,
    Setting_AutoSleep,
    Setting_AutoFullscreen,

    //RDB TLB Settings
    Rdb_Status,
    Rdb_NotesCore,
    Rdb_NotesPlugin,

    //User Interface
    UserInterface_InFullScreen,
    UserInterface_MainWindowTop,
    UserInterface_MainWindowLeft,
    UserInterface_AlwaysOnTop,

    RomBrowser_Enabled,
    RomBrowser_ColoumnsChanged,
    RomBrowser_Top,
    RomBrowser_Left,
    RomBrowser_Width,
    RomBrowser_Height,
    RomBrowser_PosIndex,
    RomBrowser_WidthIndex,
    RomBrowser_SortFieldIndex,
    RomBrowser_SortAscendingIndex,
    RomBrowser_Maximized,

    //Directory Info
    Directory_LastSave,
    Directory_RecentGameDirCount,
    Directory_RecentGameDirIndex,

    //Recent Game
    File_RecentGameFileCount,
    File_RecentGameFileIndex,

    //Support Window
    SupportWindows_RunCount,

    //Debugger UI window positions and sizes
    DebuggerUI_CommandsPos,
    DebuggerUI_MemoryPos,
    DebuggerUI_MemoryDumpPos,
    DebuggerUI_MemorySearchPos,
    DebuggerUI_DMALogPos,
    DebuggerUI_CPULogPos,
    DebuggerUI_ScriptsPos,
    DebuggerUI_StackPos,
    DebuggerUI_StackTracePos,
    DebuggerUI_SymbolsPos,
    DebuggerUI_TLBPos,
    DebuggerUI_ExceptionBPPos
};

void RegisterUISettings (void);
void UISettingsSaveBool(UISettingID Type, bool Value);
void UISettingsSaveBoolIndex(UISettingID Type, int32_t index, bool Value);
void UISettingsSaveDword(UISettingID Type, uint32_t Value);
void UISettingsSaveDwordIndex(UISettingID Type, int32_t index, uint32_t Value);
void UISettingsSaveString(UISettingID Type, const std::string & Value);
void UISettingsSaveStringIndex(UISettingID Type, int32_t index, const std::string & Value);

void UISettingsDeleteSettingIndex(UISettingID Type, int32_t index);

bool UISettingsLoadBool(UISettingID Type);
bool UISettingsLoadBoolIndex(UISettingID Type, int32_t index);
uint32_t UISettingsLoadDword(UISettingID Type);
bool UISettingsLoadDword(UISettingID Type, uint32_t & Value);
bool UISettingsLoadDwordIndex(UISettingID Type, int index, uint32_t & Value);
bool UISettingsLoadStringIndex(UISettingID Type, int32_t index, std::string & Value);
std::string UISettingsLoadStringIndex(UISettingID Type, int32_t index);
std::string UISettingsLoadStringVal(UISettingID Type);
bool UISettingsLoadStringVal(UISettingID Type, char * Buffer, int32_t BufferSize);
