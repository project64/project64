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
#include "stdafx.h"
#include "UISettings.h"
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>
#include <Project64-core/Settings/SettingType/SettingsType-ApplicationIndex.h>
#include <Project64-core/Settings/SettingType/SettingsType-ApplicationPath.h>
#include <Project64-core/Settings/SettingType/SettingsType-RelativePath.h>
#include <Project64-core/Settings/SettingType/SettingsType-RomDatabase.h>
#include <Project64-core/Settings/SettingType/SettingsType-SelectedDirectory.h>
#include <Project64-core/Settings/SettingType/SettingsType-TempBool.h>

void RegisterUISettings (void)
{
    //information - temp keys
    g_Settings->AddHandler((SettingID)(FirstUISettings + Info_ShortCutsChanged), new CSettingTypeTempBool(false));

    //Support Files
    g_Settings->AddHandler((SettingID)(FirstUISettings + SupportFile_ShortCuts), new CSettingTypeApplicationPath("", "ShortCuts", (SettingID)(FirstUISettings + SupportFile_ShortCutsDefault)));
    g_Settings->AddHandler((SettingID)(FirstUISettings + SupportFile_ShortCutsDefault), new CSettingTypeRelativePath("Config", "Project64.sc3"));

    //Settings location
    g_Settings->AddHandler((SettingID)(FirstUISettings + Setting_PluginPageFirst), new CSettingTypeApplication("Settings", "Plugin Page First", false));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Setting_DisableScrSaver), new CSettingTypeApplication("Settings", "Disable Screen Saver", (uint32_t)true));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Setting_AutoSleep), new CSettingTypeApplication("Settings", "Auto Sleep", (uint32_t)true));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Setting_AutoFullscreen), new CSettingTypeApplication("Settings", "Auto Full Screen", (uint32_t)false));

    //RDB Settings
    g_Settings->AddHandler((SettingID)(FirstUISettings + Rdb_Status), new CSettingTypeRomDatabase("Status", "Unknown"));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Rdb_NotesCore), new CSettingTypeRomDatabase("Core Note", ""));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Rdb_NotesPlugin), new CSettingTypeRomDatabase("Plugin Note", ""));

    //User Interface
    g_Settings->AddHandler((SettingID)(FirstUISettings + UserInterface_InFullScreen), new CSettingTypeTempBool(false));
    g_Settings->AddHandler((SettingID)(FirstUISettings + UserInterface_MainWindowTop), new CSettingTypeApplication("Main Window", "Top", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + UserInterface_MainWindowLeft), new CSettingTypeApplication("Main Window", "Left", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + UserInterface_AlwaysOnTop), new CSettingTypeApplication("", "Always on top", (uint32_t)false));

    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_Enabled), new CSettingTypeApplication("Rom Browser", "Rom Browser", true));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_ColoumnsChanged), new CSettingTypeTempBool(false));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_Top), new CSettingTypeApplication("Rom Browser", "Top", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_Left), new CSettingTypeApplication("Rom Browser", "Left", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_Width), new CSettingTypeApplication("Rom Browser", "Width", (uint32_t)(640 * DPIScale())));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_Height), new CSettingTypeApplication("Rom Browser", "Height", (uint32_t)(480 * DPIScale())));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_PosIndex), new CSettingTypeApplicationIndex("Rom Browser\\Field Pos", "Field", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_WidthIndex), new CSettingTypeApplicationIndex("Rom Browser\\Field Width", "Field", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_SortFieldIndex), new CSettingTypeApplicationIndex("Rom Browser", "Sort Field", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_SortAscendingIndex), new CSettingTypeApplicationIndex("Rom Browser", "Sort Ascending", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + RomBrowser_Maximized), new CSettingTypeApplication("Rom Browser", "Maximized", false));

    g_Settings->AddHandler((SettingID)(FirstUISettings + Directory_RecentGameDirCount), new CSettingTypeApplication("", "Remembered Rom Dirs", (uint32_t)10));
    g_Settings->AddHandler((SettingID)(FirstUISettings + Directory_RecentGameDirIndex), new CSettingTypeApplicationIndex("Recent Dir", "Recent Dir", Default_None));

    g_Settings->AddHandler((SettingID)(FirstUISettings + Directory_LastSave), new CSettingTypeApplication("Directory", "Last Save Directory", Directory_InstantSave));
    g_Settings->AddHandler((SettingID)(FirstUISettings + File_RecentGameFileCount), new CSettingTypeApplication("", "Remembered Rom Files", (uint32_t)10));
    g_Settings->AddHandler((SettingID)(FirstUISettings + File_RecentGameFileIndex), new CSettingTypeApplicationIndex("Recent File", "Recent Rom", Default_None));

    g_Settings->AddHandler((SettingID)(FirstUISettings + SupportWindows_RunCount), new CSettingTypeApplication("Support Project64", "Run Count", (uint32_t)0));

    //Debugger UI
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_CommandsPos), new CSettingTypeApplication("Debugger UI", "Commands Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_MemoryPos), new CSettingTypeApplication("Debugger UI", "Memory Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_MemoryDumpPos), new CSettingTypeApplication("Debugger UI", "Memory Dump Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_MemorySearchPos), new CSettingTypeApplication("Debugger UI", "Memory Search Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_DMALogPos), new CSettingTypeApplication("Debugger UI", "DMA Log Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_CPULogPos), new CSettingTypeApplication("Debugger UI", "CPU Log Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_ScriptsPos), new CSettingTypeApplication("Debugger UI", "Scripts Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_StackPos), new CSettingTypeApplication("Debugger UI", "Stack Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_StackTracePos), new CSettingTypeApplication("Debugger UI", "Stack Trace Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_SymbolsPos), new CSettingTypeApplication("Debugger UI", "Symbols Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_TLBPos), new CSettingTypeApplication("Debugger UI", "TLB Pos", Default_None));
    g_Settings->AddHandler((SettingID)(FirstUISettings + DebuggerUI_ExceptionBPPos), new CSettingTypeApplication("Debugger UI", "Exception BP Pos", Default_None));
}

float DPIScale(void) {
    return CClientDC(0).GetDeviceCaps(LOGPIXELSX) / 96.0f;
}

void UISettingsSaveBool(UISettingID Type, bool Value)
{
    g_Settings->SaveBool((SettingID)(FirstUISettings + Type), Value);
}

void UISettingsSaveBoolIndex(UISettingID Type, int32_t index, bool Value)
{
    g_Settings->SaveBoolIndex((SettingID)(FirstUISettings + Type), index, Value);
}

void UISettingsSaveDword(UISettingID Type, uint32_t Value)
{
    g_Settings->SaveDword((SettingID)(FirstUISettings + Type), Value);
}

void UISettingsSaveDwordIndex(UISettingID Type, int32_t index, uint32_t Value)
{
    g_Settings->SaveDwordIndex((SettingID)(FirstUISettings + Type), index, Value);
}

void UISettingsSaveString(UISettingID Type, const std::string & Value)
{
    g_Settings->SaveString((SettingID)(FirstUISettings + Type), Value);
}

void UISettingsSaveStringIndex(UISettingID Type, int32_t index, const std::string & Value)
{
    g_Settings->SaveStringIndex((SettingID)(FirstUISettings + Type), index, Value);
}

void UISettingsDeleteSettingIndex(UISettingID Type, int32_t index)
{
    g_Settings->DeleteSettingIndex((SettingID)(FirstUISettings + Type), index);
}

bool UISettingsLoadBool(UISettingID Type)
{
    return g_Settings->LoadBool((SettingID)(FirstUISettings + Type));
}

bool UISettingsLoadBoolIndex(UISettingID Type, int32_t index)
{
    return g_Settings->LoadBoolIndex((SettingID)(FirstUISettings + Type),index);
}

uint32_t UISettingsLoadDword(UISettingID Type)
{
    return g_Settings->LoadDword((SettingID)(FirstUISettings + Type));
}

bool UISettingsLoadDword(UISettingID Type, uint32_t & Value)
{
    return g_Settings->LoadDword((SettingID)(FirstUISettings + Type), Value);
}

bool UISettingsLoadDwordIndex(UISettingID Type, int index, uint32_t & Value)
{
    return g_Settings->LoadDwordIndex((SettingID)(FirstUISettings + Type), index, Value);
}

bool UISettingsLoadStringIndex(UISettingID Type, int32_t index, std::string & Value)
{
    stdstr ValueRes;
    bool res = g_Settings->LoadStringIndex((SettingID)(FirstUISettings + Type), index, ValueRes);
    Value = ValueRes;
    return res;
}

std::string UISettingsLoadStringIndex(UISettingID Type, int32_t index)
{
    return g_Settings->LoadStringIndex((SettingID)(FirstUISettings + Type), index);
}

std::string UISettingsLoadStringVal(UISettingID Type)
{
    return g_Settings->LoadStringVal((SettingID)(FirstUISettings + Type));
}

bool UISettingsLoadStringVal(UISettingID Type, char * Buffer, int32_t BufferSize)
{
    return g_Settings->LoadStringVal((SettingID)(FirstUISettings + Type),Buffer,BufferSize);
}
