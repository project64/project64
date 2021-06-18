#include "stdafx.h"
#include "UISettings.h"
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/Settings/SettingType/SettingsType-ApplicationIndex.h>
#include <Project64-core/Settings/SettingType/SettingsType-ApplicationPath.h>
#include <Project64-core/Settings/SettingType/SettingsType-RelativePath.h>
#include <Project64-core/Settings/SettingType/SettingsType-RomDatabase.h>
#include <Project64-core/Settings/SettingType/SettingsType-SelectedDirectory.h>
#include <Project64-core/Settings/SettingType/SettingsType-TempBool.h>

void AddUISetting(UISettingID TypeID, CSettingType * Handler)
{
    g_Settings->AddHandler((SettingID)TypeID, Handler);
}

void RegisterUISettings (void)
{
    // Information - temporary keys
    AddUISetting(Info_ShortCutsChanged, new CSettingTypeTempBool(false));

    // Support files
    AddUISetting(SupportFile_ShortCuts, new CSettingTypeApplicationPath("", "ShortCuts", (SettingID)SupportFile_ShortCutsDefault));
    AddUISetting(SupportFile_ShortCutsDefault, new CSettingTypeRelativePath("Config", "Project64.sc3"));

    // Settings location
    AddUISetting(Setting_PluginPageFirst, new CSettingTypeApplication("Settings", "Plugin Page First", false));
    AddUISetting(Setting_DisableScrSaver, new CSettingTypeApplication("Settings", "Disable Screen Saver", (uint32_t)true));
    AddUISetting(Setting_EnableDiscordRPC, new CSettingTypeApplication("Settings", "Enable Discord RPC", true));
    AddUISetting(Setting_AutoSleep, new CSettingTypeApplication("Settings", "Auto Sleep", (uint32_t)true));
    AddUISetting(Setting_AutoFullscreen, new CSettingTypeApplication("Settings", "Auto Full Screen", (uint32_t)false));

    // RDB settings
    AddUISetting(Rdb_Status, new CSettingTypeRomDatabase("Status", "Unknown"));
    AddUISetting(Rdb_NotesCore, new CSettingTypeRomDatabase("Core Note", ""));
    AddUISetting(Rdb_NotesPlugin, new CSettingTypeRomDatabase("Plugin Note", ""));

    // User interface
    AddUISetting(UserInterface_InFullScreen, new CSettingTypeTempBool(false));
    AddUISetting(UserInterface_MainWindowTop, new CSettingTypeApplication("Main Window", "Top", Default_None));
    AddUISetting(UserInterface_MainWindowLeft, new CSettingTypeApplication("Main Window", "Left", Default_None));
    AddUISetting(UserInterface_AlwaysOnTop, new CSettingTypeApplication("Settings", "Always on top", (uint32_t)false));
    AddUISetting(UserInterface_ShowStatusBar, new CSettingTypeApplication("Settings", "Show Status Bar", true));
    AddUISetting(UserInterface_ExitFullscreenOnLoseFocus, new CSettingTypeApplication("Settings", "Exit Full Screen On Lose Focus", false));
    AddUISetting(UserInterface_ShowingNagWindow, new CSettingTypeTempBool(false));

    AddUISetting(RomBrowser_Enabled, new CSettingTypeApplication("Rom Browser", "Rom Browser", true));
    AddUISetting(RomBrowser_ColoumnsChanged, new CSettingTypeTempBool(false));
    AddUISetting(RomBrowser_Top, new CSettingTypeApplication("Rom Browser", "Top", Default_None));
    AddUISetting(RomBrowser_Left, new CSettingTypeApplication("Rom Browser", "Left", Default_None));
    AddUISetting(RomBrowser_Width, new CSettingTypeApplication("Rom Browser", "Width", (uint32_t)(640 * DPIScale())));
    AddUISetting(RomBrowser_Height, new CSettingTypeApplication("Rom Browser", "Height", (uint32_t)(480 * DPIScale())));
    AddUISetting(RomBrowser_PosIndex, new CSettingTypeApplicationIndex("Rom Browser\\Field Pos", "Field", Default_None));
    AddUISetting(RomBrowser_WidthIndex, new CSettingTypeApplicationIndex("Rom Browser\\Field Width", "Field", Default_None));
    AddUISetting(RomBrowser_SortFieldIndex, new CSettingTypeApplicationIndex("Rom Browser", "Sort Field", Default_None));
    AddUISetting(RomBrowser_SortAscendingIndex, new CSettingTypeApplicationIndex("Rom Browser", "Sort Ascending", Default_None));
    AddUISetting(RomBrowser_Maximized, new CSettingTypeApplication("Rom Browser", "Maximized", false));

    AddUISetting(Directory_RecentGameDirCount, new CSettingTypeApplication("Settings", "Remembered Rom Dirs", (uint32_t)10));
    AddUISetting(Directory_RecentGameDirIndex, new CSettingTypeApplicationIndex("Recent Dir", "Recent Dir", Default_None));

    AddUISetting(Directory_LastSave, new CSettingTypeApplication("Directory", "Last Save Directory", Directory_InstantSave));
    AddUISetting(File_RecentGameFileCount, new CSettingTypeApplication("Settings", "Remembered Rom Files", (uint32_t)10));
    AddUISetting(File_RecentGameFileIndex, new CSettingTypeApplicationIndex("Recent File", "Recent Rom", Default_None));

    // Debugger UI
    AddUISetting(DebuggerUI_CommandsPos, new CSettingTypeApplication("Debugger UI", "Commands Pos", Default_None));
    AddUISetting(DebuggerUI_MemoryPos, new CSettingTypeApplication("Debugger UI", "Memory Pos", Default_None));
    AddUISetting(DebuggerUI_MemoryDumpPos, new CSettingTypeApplication("Debugger UI", "Memory Dump Pos", Default_None));
    AddUISetting(DebuggerUI_MemorySearchPos, new CSettingTypeApplication("Debugger UI", "Memory Search Pos", Default_None));
    AddUISetting(DebuggerUI_DMALogPos, new CSettingTypeApplication("Debugger UI", "DMA Log Pos", Default_None));
    AddUISetting(DebuggerUI_CPULogPos, new CSettingTypeApplication("Debugger UI", "CPU Log Pos", Default_None));
    AddUISetting(DebuggerUI_ScriptsPos, new CSettingTypeApplication("Debugger UI", "Scripts Pos", Default_None));
    AddUISetting(DebuggerUI_StackPos, new CSettingTypeApplication("Debugger UI", "Stack Pos", Default_None));
    AddUISetting(DebuggerUI_StackTracePos, new CSettingTypeApplication("Debugger UI", "Stack Trace Pos", Default_None));
    AddUISetting(DebuggerUI_SymbolsPos, new CSettingTypeApplication("Debugger UI", "Symbols Pos", Default_None));
    AddUISetting(DebuggerUI_TLBPos, new CSettingTypeApplication("Debugger UI", "TLB Pos", Default_None));
    AddUISetting(DebuggerUI_ExceptionBPPos, new CSettingTypeApplication("Debugger UI", "Exception BP Pos", Default_None));
}

float DPIScale(void) 
{
    return CClientDC(0).GetDeviceCaps(LOGPIXELSX) / 96.0f;
}

void UISettingsSaveBool(UISettingID Type, bool Value)
{
    g_Settings->SaveBool((SettingID)Type, Value);
}

void UISettingsSaveBoolIndex(UISettingID Type, int32_t index, bool Value)
{
    g_Settings->SaveBoolIndex((SettingID)Type, index, Value);
}

void UISettingsSaveDword(UISettingID Type, uint32_t Value)
{
    g_Settings->SaveDword((SettingID)Type, Value);
}

void UISettingsSaveDwordIndex(UISettingID Type, int32_t index, uint32_t Value)
{
    g_Settings->SaveDwordIndex((SettingID)Type, index, Value);
}

void UISettingsSaveString(UISettingID Type, const std::string & Value)
{
    g_Settings->SaveString((SettingID)Type, Value);
}

void UISettingsSaveStringIndex(UISettingID Type, int32_t index, const std::string & Value)
{
    g_Settings->SaveStringIndex((SettingID)Type, index, Value);
}

void UISettingsDeleteSettingIndex(UISettingID Type, int32_t index)
{
    g_Settings->DeleteSettingIndex((SettingID)Type, index);
}

bool UISettingsLoadBool(UISettingID Type)
{
    return g_Settings->LoadBool((SettingID)Type);
}

bool UISettingsLoadBoolIndex(UISettingID Type, int32_t index)
{
    return g_Settings->LoadBoolIndex((SettingID)Type,index);
}

uint32_t UISettingsLoadDword(UISettingID Type)
{
    return g_Settings->LoadDword((SettingID)Type);
}

bool UISettingsLoadDword(UISettingID Type, uint32_t & Value)
{
    return g_Settings->LoadDword((SettingID)Type, Value);
}

bool UISettingsLoadDwordIndex(UISettingID Type, int index, uint32_t & Value)
{
    return g_Settings->LoadDwordIndex((SettingID)Type, index, Value);
}

bool UISettingsLoadStringIndex(UISettingID Type, int32_t index, std::string & Value)
{
    stdstr ValueRes;
    bool res = g_Settings->LoadStringIndex((SettingID)Type, index, ValueRes);
    Value = ValueRes;
    return res;
}

std::string UISettingsLoadStringIndex(UISettingID Type, int32_t index)
{
    return g_Settings->LoadStringIndex((SettingID)Type, index);
}

std::string UISettingsLoadStringVal(UISettingID Type)
{
    return g_Settings->LoadStringVal((SettingID)Type);
}

bool UISettingsLoadStringVal(UISettingID Type, char * Buffer, int32_t BufferSize)
{
    return g_Settings->LoadStringVal((SettingID)Type,Buffer,BufferSize);
}
