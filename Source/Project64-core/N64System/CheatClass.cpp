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
#include "CheatClass.h"

#include <Project64-core/Settings/SettingType/SettingsType-Cheats.h>
#include <Project64-core/Plugins/GFXPlugin.h>
#include <Project64-core/Plugins/AudioPlugin.h>
#include <Project64-core/Plugins/RSPPlugin.h>
#include <Project64-core/Plugins/ControllerPlugin.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <stdlib.h>

CCheats::CCheats(CMipsMemoryVM & MMU) :
    m_MMU(MMU)
{
}

CCheats::~CCheats()
{
}

bool CCheats::LoadCode(const stdstr & CheatEntry, SettingID ExtensionSetting, int ExtensionIndex)
{
    //Find the start and end of the name which is surrounded in ""
    int StartOfName = CheatEntry.find("\"");
    if (StartOfName == -1)
    {
        return false;
    }
    int EndOfName = CheatEntry.find("\"", StartOfName + 1);
    if (EndOfName == -1)
    {
        return false;
    }
    const char * CheatString = &CheatEntry.c_str()[EndOfName + 2];
    if (!IsValid16BitCode(CheatString))
    {
        return false;
    }

    stdstr Extension;
    if (!g_Settings->LoadStringIndex(ExtensionSetting, ExtensionIndex, Extension))
    {
        Extension.clear();
    }

    const char * ReadPos = CheatString;
    CODES Code;
    while (ReadPos)
    {
        GAMESHARK_CODE CodeEntry;

        CodeEntry.Command = strtoul(ReadPos, 0, 16);
        ReadPos = strchr(ReadPos, ' ');
        if (ReadPos == NULL) { break; }
        ReadPos += 1;

        if (strncmp(ReadPos, "????", 4) == 0)
        {
            if (Extension.length() == 0) { return false; }
            CodeEntry.Value = Extension[0] == '$' ? (uint16_t)strtoul(&Extension[1], 0, 16) : (uint16_t)atol(Extension.c_str());
        }
        else if (strncmp(ReadPos, "??", 2) == 0)
        {
            if (Extension.length() == 0) { return false; }
            CodeEntry.Value = (uint8_t)(strtoul(ReadPos, 0, 16));
            CodeEntry.Value |= (Extension[0] == '$' ? (uint8_t)strtoul(&Extension[1], 0, 16) : (uint8_t)atol(Extension.c_str())) << 16;
        }
        else if (strncmp(&ReadPos[2], "??", 2) == 0)
        {
            if (Extension.length() == 0) { return false; }
            CodeEntry.Value = (uint16_t)(strtoul(ReadPos, 0, 16) << 16);
            CodeEntry.Value |= Extension[0] == '$' ? (uint8_t)strtoul(&Extension[1], 0, 16) : (uint8_t)atol(Extension.c_str());
        }
        else
        {
            CodeEntry.Value = (uint16_t)strtoul(ReadPos, 0, 16);
        }
        Code.push_back(CodeEntry);

        ReadPos = strchr(ReadPos, ',');
        if (ReadPos == NULL)
        {
            continue;
        }
        ReadPos++;
    }
    if (Code.size() == 0)
    {
        return false;
    }

    m_Codes.push_back(Code);
    return true;
}

void CCheats::LoadPermCheats(CPlugins * Plugins)
{
    if (g_Settings->LoadBool(Debugger_DisableGameFixes))
    {
        return;
    }
    for (int CheatNo = 0; CheatNo < MaxCheats; CheatNo++)
    {
        stdstr LineEntry;
        if (!g_Settings->LoadStringIndex(Rdb_GameCheatFix, CheatNo, LineEntry) || LineEntry.empty())
        {
            break;
        }

        stdstr CheatPlugins;
        bool LoadEntry = true;
        if (g_Settings->LoadStringIndex(Rdb_GameCheatFixPlugin, CheatNo, CheatPlugins) && !CheatPlugins.empty())
        {
            LoadEntry = false;

            strvector PluginList = CheatPlugins.Tokenize(',');
            for (size_t i = 0, n = PluginList.size(); i < n; i++)
            {
                stdstr PluginName = PluginList[i].Trim();
                if (Plugins->Gfx() != NULL && strstr(Plugins->Gfx()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
                if (Plugins->Audio() != NULL && strstr(Plugins->Audio()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
                if (Plugins->RSP() != NULL && strstr(Plugins->RSP()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
                if (Plugins->Control() != NULL && strstr(Plugins->Control()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
            }
        }

        if (LoadEntry)
        {
            LoadCode(LineEntry.c_str(), Default_None, CheatNo);
        }
    }
}

void CCheats::LoadCheats(bool DisableSelected, CPlugins * Plugins)
{
    ResetCodes();
    LoadPermCheats(Plugins);

    for (int CheatNo = 0; CheatNo < MaxCheats; CheatNo++)
    {
        stdstr LineEntry = g_Settings->LoadStringIndex(Cheat_Entry, CheatNo);
        if (LineEntry.empty()) { break; }
        if (!g_Settings->LoadBoolIndex(Cheat_Active, CheatNo))
        {
            continue;
        }
        if (DisableSelected)
        {
            g_Settings->SaveBoolIndex(Cheat_Active, CheatNo, false);
            continue;
        }

        LoadCode(LineEntry, Cheat_Extension, CheatNo);
    }
}

/********************************************************************************************
ConvertXP64Address

Purpose: Decode encoded XP64 address to physical address
Parameters:
Returns:
Author: Witten

********************************************************************************************/
uint32_t ConvertXP64Address(uint32_t Address)
{
    uint32_t tmpAddress;

    tmpAddress = (Address ^ 0x68000000) & 0xFF000000;
    tmpAddress += ((Address + 0x002B0000) ^ 0x00810000) & 0x00FF0000;
    tmpAddress += ((Address + 0x00002B00) ^ 0x00008200) & 0x0000FF00;
    tmpAddress += ((Address + 0x0000002B) ^ 0x00000083) & 0x000000FF;
    return tmpAddress;
}

/********************************************************************************************
ConvertXP64Value

Purpose: Decode encoded XP64 value
Parameters:
Returns:
Author: Witten

********************************************************************************************/
uint16_t ConvertXP64Value(uint16_t Value)
{
    uint16_t  tmpValue;

    tmpValue = ((Value + 0x2B00) ^ 0x8400) & 0xFF00;
    tmpValue += ((Value + 0x002B) ^ 0x0085) & 0x00FF;
    return tmpValue;
}

void CCheats::ApplyCheats()
{
    for (size_t CurrentCheat = 0; CurrentCheat < m_Codes.size(); CurrentCheat++)
    {
        CODES & CodeEntry = m_Codes[CurrentCheat];
        for (size_t CurrentEntry = 0; CurrentEntry < CodeEntry.size();)
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry);
            CurrentEntry += EntrySize(CodeEntry, CurrentEntry);
        }
    }
}

void CCheats::ApplyGSButton()
{
    for (size_t CurrentCheat = 0; CurrentCheat < m_Codes.size(); CurrentCheat++)
    {
        const CODES & CodeEntry = m_Codes[CurrentCheat];
        for (size_t CurrentEntry = 0; CurrentEntry < CodeEntry.size(); CurrentEntry++)
        {
            const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
            switch (Code.Command & 0xFF000000) {
            case 0x88000000:
                ModifyMemory8(0x80000000 | (Code.Command & 0xFFFFFF), (uint8_t)Code.Value);
                break;
            case 0x89000000:
                ModifyMemory16(0x80000000 | (Code.Command & 0xFFFFFF), Code.Value);
                break;
                // Xplorer64
            case 0xA8000000:
                ModifyMemory8(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), (uint8_t)ConvertXP64Value(Code.Value));
                break;
            case 0xA9000000:
                ModifyMemory16(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), ConvertXP64Value(Code.Value));
                break;
            }
        }
    }
}

bool CCheats::IsValid16BitCode(const char * CheatString)
{
    const char * ReadPos = CheatString;
    bool GSButtonCheat = false, FirstEntry = true;

    while (ReadPos)
    {
        GAMESHARK_CODE CodeEntry;

        CodeEntry.Command = strtoul(ReadPos, 0, 16);
        ReadPos = strchr(ReadPos, ' ');
        if (ReadPos == NULL) { break; }
        ReadPos += 1;

        //validate Code Entry
        switch (CodeEntry.Command & 0xFF000000) {
        case 0x50000000:
        case 0x80000000:
        case 0xA0000000:
        case 0xD0000000:
        case 0xD2000000:
        case 0xC8000000:
        case 0xE8000000:
        case 0x10000000: // Xplorer64
            break;
        case 0x81000000:
        case 0xA1000000:
        case 0xD1000000:
        case 0xD3000000:
            if (((CodeEntry.Command & 0xFFFFFF) & 1) == 1)
            {
                return false;
            }
            break;
        case 0x88000000:
        case 0xA8000000:
            if (FirstEntry) { GSButtonCheat = true; }
            if (!GSButtonCheat) { return false; }
            break;
        case 0x89000000:
            if (FirstEntry) { GSButtonCheat = true; }
            if (!GSButtonCheat) { return false; }
            if (((CodeEntry.Command & 0xFFFFFF) & 1) == 1)
            {
                return false;
            }
            break;
        case 0xA9000000:
            if (FirstEntry) { GSButtonCheat = true; }
            if (!GSButtonCheat) { return false; }
            if (((ConvertXP64Address(CodeEntry.Command) & 0xFFFFFF) & 1) == 1)
            {
                return false;
            }
            break;
        case 0x11000000: // Xplorer64
        case 0xE9000000:
        case 0xC9000000:
            if (((ConvertXP64Address(CodeEntry.Command) & 0xFFFFFF) & 1) == 1)
            {
                return false;
            }
            break;
        default:
            return false;
        }

        FirstEntry = false;

        ReadPos = strchr(ReadPos, ',');
        if (ReadPos == NULL)
        {
            continue;
        }
        ReadPos++;
    }
    return true;
}

void CCheats::ModifyMemory8(uint32_t Address, uint8_t Value)
{
    MEM_VALUE8 OriginalValue;
    if (!m_MMU.LB_VAddr(Address, OriginalValue.Original))
    {
        return;
    }
    if (OriginalValue.Original == Value)
    {
        return;
    }
    OriginalValue.Changed = Value;
    std::pair<ORIGINAL_VALUES8::iterator, bool> itr = m_OriginalValues8.insert(ORIGINAL_VALUES8::value_type(Address, OriginalValue));
    m_MMU.SB_VAddr(Address, OriginalValue.Changed);
    if (g_Recompiler)
    {
        g_Recompiler->ClearRecompCode_Virt(Address, 1, CRecompiler::Remove_Cheats);
    }
}

void CCheats::ModifyMemory16(uint32_t Address, uint16_t Value)
{
    MEM_VALUE16 OriginalValue;
    if (!m_MMU.LH_VAddr(Address, OriginalValue.Original))
    {
        return;
    }
    if (OriginalValue.Original == Value)
    {
        return;
    }
    OriginalValue.Changed = Value;
    std::pair<ORIGINAL_VALUES16::iterator, bool> itr = m_OriginalValues16.insert(ORIGINAL_VALUES16::value_type(Address, OriginalValue));
    m_MMU.SH_VAddr(Address, OriginalValue.Changed);
    if (g_Recompiler)
    {
        g_Recompiler->ClearRecompCode_Virt(Address, 2, CRecompiler::Remove_Cheats);
    }
}

void CCheats::ApplyCheatEntry(CODES & CodeEntry, int32_t CurrentEntry)
{
    if (CurrentEntry < 0 || CurrentEntry >= (int)CodeEntry.size())
    {
        return;
    }
    GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
    uint16_t wMemory;
    uint8_t bMemory;

    switch (Code.Command & 0xFF000000)
    {
    case 0x50000000: // Gameshark / AR
        if ((CurrentEntry + 1) >= (int)CodeEntry.size())
        {
            return;
        }

        {
            const GAMESHARK_CODE & NextCodeEntry = CodeEntry[CurrentEntry + 1];
            int numrepeats = (Code.Command & 0x0000FF00) >> 8;
            int offset = Code.Command & 0x000000FF;
            uint32_t Address;
            int incr = Code.Value;
            int i;

            switch (NextCodeEntry.Command & 0xFF000000) {
            case 0x10000000: // Xplorer64
            case 0x80000000:
                Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
                wMemory = NextCodeEntry.Value;
                for (i = 0; i < numrepeats; i++)
                {
                    ModifyMemory8(Address, (uint8_t)wMemory);
                    Address += offset;
                    wMemory += (uint16_t)incr;
                }
                break;
            case 0x11000000: // Xplorer64
            case 0x81000000:
                Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
                wMemory = NextCodeEntry.Value;
                for (i = 0; i < numrepeats; i++)
                {
                    ModifyMemory16(Address, wMemory);
                    Address += offset;
                    wMemory += (uint16_t)incr;
                }
                break;
            }
        }
        break;
    case 0x80000000:
    case 0x30000000:
    case 0x82000000:
    case 0x84000000:
        ModifyMemory8(0x80000000 | (Code.Command & 0xFFFFFF), (uint8_t)Code.Value);
        break;
    case 0x81000000:
        ModifyMemory16(0x80000000 | (Code.Command & 0xFFFFFF), Code.Value);
        break;
    case 0xA0000000:
        ModifyMemory8(0xA0000000 | (Code.Command & 0xFFFFFF), (uint8_t)Code.Value);
        break;
    case 0xA1000000:
        ModifyMemory16(0xA0000000 | (Code.Command & 0xFFFFFF), Code.Value);
        break;
    case 0xD0000000:
        m_MMU.LB_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), bMemory);
        if (bMemory == Code.Value)
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xD1000000:
        m_MMU.LH_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), wMemory);
        if (wMemory == Code.Value)
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xD2000000:
        m_MMU.LB_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), bMemory);
        if (bMemory != Code.Value)
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xD3000000:
        m_MMU.LH_VAddr(0x80000000 | (Code.Command & 0xFFFFFF), wMemory);
        if (wMemory != Code.Value)
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0x31000000:
    case 0x83000000:
    case 0x85000000:
        ModifyMemory16(0x80000000 | (Code.Command & 0xFFFFFF), Code.Value);
        break;
    case 0xE8000000:
        ModifyMemory8(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), (uint8_t)ConvertXP64Value(Code.Value));
        break;
    case 0xE9000000:
        ModifyMemory16(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), ConvertXP64Value(Code.Value));
        break;
    case 0xC8000000:
        ModifyMemory8(0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), (uint8_t)Code.Value);
        break;
    case 0xC9000000:
        ModifyMemory16(0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), ConvertXP64Value(Code.Value));
        break;
    case 0xB8000000:
    case 0xBA000000:
        m_MMU.LB_VAddr(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), bMemory);
        if (bMemory == ConvertXP64Value(Code.Value))
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry + 1);
        }
        break;
    case 0xB9000000:
    case 0xBB000000:
        m_MMU.LH_VAddr(0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF), wMemory);
        if (wMemory == ConvertXP64Value(Code.Value))
        {
            ApplyCheatEntry(CodeEntry, CurrentEntry + 1);
        }
        break;
    }
}

int32_t CCheats::EntrySize(const CODES & CodeEntry, int32_t CurrentEntry)
{
    if (CurrentEntry < 0 || CurrentEntry >= (int)CodeEntry.size())
    {
        return 0;
    }
    const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
    switch (Code.Command & 0xFF000000)
    {
    case 0x50000000: // Gameshark / AR
        if ((CurrentEntry + 1) >= (int)CodeEntry.size())
        {
            return 1;
        }

        switch (CodeEntry[CurrentEntry + 1].Command & 0xFF000000)
        {
        case 0x10000000: // Xplorer64
        case 0x80000000:
        case 0x11000000: // Xplorer64
        case 0x81000000:
            return 2;
        }
        break;
    case 0xD0000000:
    case 0xD1000000:
    case 0xD2000000:
    case 0xD3000000:
    case 0xB8000000:
    case 0xB9000000:
    case 0xBA000000:
    case 0xBB000000:
        return EntrySize(CodeEntry, CurrentEntry + 1) + 1;
    case 0:
        return MaxGSEntries;
    }
    return 1;
}

void CCheats::ResetCodes(void)
{
    m_Codes.clear();
    for (ORIGINAL_VALUES8::iterator itr = m_OriginalValues8.begin(); itr != m_OriginalValues8.end(); itr++)
    {
        uint8_t CurrentValue;
        if (m_MMU.LB_VAddr(itr->first, CurrentValue) &&
            itr->second.Changed == CurrentValue)
        {
            m_MMU.SB_VAddr(itr->first, itr->second.Original);
        }
    }
    m_OriginalValues8.clear();

    for (ORIGINAL_VALUES16::iterator itr = m_OriginalValues16.begin(); itr != m_OriginalValues16.end(); itr++)
    {
        uint16_t CurrentValue;
        if (m_MMU.LH_VAddr(itr->first, CurrentValue) &&
            itr->second.Changed == CurrentValue)
        {
            m_MMU.SH_VAddr(itr->first, itr->second.Original);
        }
    }
    m_OriginalValues16.clear();
}