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
#include <string.h>
#include "CheatClass.h"

#include <Project64-core/Settings/SettingType/SettingsType-Cheats.h>
#include <Project64-core/Plugins/GFXPlugin.h>
#include <Project64-core/Plugins/AudioPlugin.h>
#include <Project64-core/Plugins/RSPPlugin.h>
#include <Project64-core/Plugins/ControllerPlugin.h>

CCheats::CCheats()
{
}

CCheats::~CCheats()
{
}

bool CCheats::LoadCode(int CheatNo, const char * CheatString)
{
    if (!IsValid16BitCode(CheatString))
    {
        return false;
    }

    const char * ReadPos = CheatString;

    CODES Code;
    while (ReadPos)
    {
        GAMESHARK_CODE CodeEntry;

        CodeEntry.Command = std::strtoul(ReadPos, 0, 16);
        ReadPos = strchr(ReadPos, ' ');
        if (ReadPos == NULL) { break; }
        ReadPos += 1;

        if (strncmp(ReadPos, "????", 4) == 0)
        {
            if (CheatNo < 0 || CheatNo > MaxCheats) { return false; }
            stdstr CheatExt = g_Settings->LoadStringIndex(Cheat_Extension, CheatNo);
            if (CheatExt.empty()) { return false; }
            CodeEntry.Value = CheatExt[0] == '$' ? (uint16_t)std::strtoul(&CheatExt.c_str()[1], 0, 16) : (uint16_t)atol(CheatExt.c_str());
        }
        else if (strncmp(ReadPos, "??", 2) == 0)
        {
            if (CheatNo < 0 || CheatNo > MaxCheats) { return false; }
            stdstr CheatExt = g_Settings->LoadStringIndex(Cheat_Extension, CheatNo);
            if (CheatExt.empty()) { return false; }
            CodeEntry.Value = (uint8_t)(std::strtoul(ReadPos, 0, 16));
            CodeEntry.Value |= (CheatExt[0] == '$' ? (uint8_t)std::strtoul(&CheatExt.c_str()[1], 0, 16) : (uint8_t)atol(CheatExt.c_str())) << 16;
        }
        else if (strncmp(&ReadPos[2], "??", 2) == 0)
        {
            if (CheatNo < 0 || CheatNo > MaxCheats) { return false; }
            stdstr CheatExt = g_Settings->LoadStringIndex(Cheat_Extension, CheatNo);
            if (CheatExt.empty()) { return false; }
            CodeEntry.Value = (uint16_t)(std::strtoul(ReadPos, 0, 16) << 16);
            CodeEntry.Value |= CheatExt[0] == '$' ? (uint8_t)std::strtoul(&CheatExt.c_str()[1], 0, 16) : (uint8_t)atol(CheatExt.c_str());
        }
        else
        {
            CodeEntry.Value = (uint16_t)std::strtoul(ReadPos, 0, 16);
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
                if (strstr(Plugins->Gfx()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
                if (strstr(Plugins->Audio()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
                if (strstr(Plugins->RSP()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
                if (strstr(Plugins->Control()->PluginName(), PluginName.c_str()) != NULL)
                {
                    LoadEntry = true;
                    break;
                }
            }
        }

        if (LoadEntry)
        {
            LoadCode(-1, LineEntry.c_str());
        }
    }
}

void CCheats::LoadCheats(bool DisableSelected, CPlugins * Plugins)
{
    m_Codes.clear();
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

        //Find the start and end of the name which is surrounded in ""
        int StartOfName = LineEntry.find("\"");
        if (StartOfName == -1) { continue; }
        int EndOfName = LineEntry.find("\"", StartOfName + 1);
        if (EndOfName == -1) { continue; }

        LoadCode(CheatNo, &LineEntry.c_str()[EndOfName + 2]);
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

void CCheats::ApplyCheats(CMipsMemoryVM * MMU)
{
    for (size_t CurrentCheat = 0; CurrentCheat < m_Codes.size(); CurrentCheat++)
    {
        const CODES & CodeEntry = m_Codes[CurrentCheat];
        for (size_t CurrentEntry = 0; CurrentEntry < CodeEntry.size();)
        {
            CurrentEntry += ApplyCheatEntry(MMU, CodeEntry, CurrentEntry, true);
        }
    }
}

void CCheats::ApplyGSButton(CMipsMemoryVM * MMU)
{
    uint32_t Address;
    for (size_t CurrentCheat = 0; CurrentCheat < m_Codes.size(); CurrentCheat++)
    {
        const CODES & CodeEntry = m_Codes[CurrentCheat];
        for (size_t CurrentEntry = 0; CurrentEntry < CodeEntry.size(); CurrentEntry++)
        {
            const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
            switch (Code.Command & 0xFF000000) {
            case 0x88000000:
                Address = 0x80000000 | (Code.Command & 0xFFFFFF);
                MMU->SB_VAddr(Address, (uint8_t)Code.Value);
                break;
            case 0x89000000:
                Address = 0x80000000 | (Code.Command & 0xFFFFFF);
                MMU->SH_VAddr(Address, Code.Value);
                break;
                // Xplorer64
            case 0xA8000000:
                Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
                MMU->SB_VAddr(Address, (uint8_t)ConvertXP64Value(Code.Value));
                break;
            case 0xA9000000:
                Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
                MMU->SH_VAddr(Address, ConvertXP64Value(Code.Value));
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

        CodeEntry.Command = std::strtoul(ReadPos, 0, 16);
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
        case 0xD1000000:													// Added by Witten (witten@pj64cheats.net)
        case 0xD3000000:
            if (((CodeEntry.Command & 0xFFFFFF) & 1) == 1)
            {
                return false;
            }
            break;
        case 0x88000000:
        case 0xA8000000:
            if (FirstEntry)     { GSButtonCheat = true; }
            if (!GSButtonCheat) { return false; }
            break;
        case 0x89000000:
            if (FirstEntry)     { GSButtonCheat = true; }
            if (!GSButtonCheat) { return false; }
            if (((CodeEntry.Command & 0xFFFFFF) & 1) == 1)
            {
                return false;
            }
            break;
        case 0xA9000000:
            if (FirstEntry)     { GSButtonCheat = true; }
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

int CCheats::ApplyCheatEntry(CMipsMemoryVM * MMU, const CODES & CodeEntry, int CurrentEntry, bool Execute)
{
    if (CurrentEntry < 0 || CurrentEntry >= (int)CodeEntry.size())
    {
        return 0;
    }
    const GAMESHARK_CODE & Code = CodeEntry[CurrentEntry];
    uint32_t Address;
    uint16_t  wMemory;
    uint8_t  bMemory;

    switch (Code.Command & 0xFF000000)
    {
        // Gameshark / AR
    case 0x50000000:													// Added by Witten (witten@pj64cheats.net)
    {
        if ((CurrentEntry + 1) >= (int)CodeEntry.size())
        {
            return 1;
        }

        const GAMESHARK_CODE & NextCodeEntry = CodeEntry[CurrentEntry + 1];
        int numrepeats = (Code.Command & 0x0000FF00) >> 8;
        int offset = Code.Command & 0x000000FF;
        int incr = Code.Value;
        int i;

        switch (NextCodeEntry.Command & 0xFF000000) {
        case 0x10000000: // Xplorer64
        case 0x80000000:
            Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
            wMemory = NextCodeEntry.Value;
            for (i = 0; i < numrepeats; i++)
            {
                MMU->SB_VAddr(Address, (uint8_t)wMemory);
                Address += offset;
                wMemory += (uint16_t)incr;
            }
            return 2;
        case 0x11000000: // Xplorer64
        case 0x81000000:
            Address = 0x80000000 | (NextCodeEntry.Command & 0xFFFFFF);
            wMemory = NextCodeEntry.Value;
            for (i = 0; i < numrepeats; i++)
            {
                MMU->SH_VAddr(Address, wMemory);
                Address += offset;
                wMemory += (uint16_t)incr;
            }
            return 2;
        default: return 1;
        }
    }
    break;
    case 0x80000000:
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        if (Execute) { MMU->SB_VAddr(Address, (uint8_t)Code.Value); }
        break;
    case 0x81000000:
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        if (Execute) { MMU->SH_VAddr(Address, Code.Value); }
        break;
    case 0xA0000000:
        Address = 0xA0000000 | (Code.Command & 0xFFFFFF);
        if (Execute) { MMU->SB_VAddr(Address, (uint8_t)Code.Value); }
        break;
    case 0xA1000000:
        Address = 0xA0000000 | (Code.Command & 0xFFFFFF);
        if (Execute) { MMU->SH_VAddr(Address, Code.Value); }
        break;
    case 0xD0000000:													// Added by Witten (witten@pj64cheats.net)
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        MMU->LB_VAddr(Address, bMemory);
        if (bMemory != Code.Value) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0xD1000000:													// Added by Witten (witten@pj64cheats.net)
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        MMU->LH_VAddr(Address, wMemory);
        if (wMemory != Code.Value) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0xD2000000:													// Added by Witten (witten@pj64cheats.net)
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        MMU->LB_VAddr(Address, bMemory);
        if (bMemory == Code.Value) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0xD3000000:													// Added by Witten (witten@pj64cheats.net)
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        MMU->LH_VAddr(Address, wMemory);
        if (wMemory == Code.Value) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;

        // Xplorer64 (Author: Witten)
    case 0x30000000:
    case 0x82000000:
    case 0x84000000:
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        if (Execute) { MMU->SB_VAddr(Address, (uint8_t)Code.Value); }
        break;
    case 0x31000000:
    case 0x83000000:
    case 0x85000000:
        Address = 0x80000000 | (Code.Command & 0xFFFFFF);
        if (Execute) { MMU->SH_VAddr(Address, Code.Value); }
        break;
    case 0xE8000000:
        Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        if (Execute) { MMU->SB_VAddr(Address, (uint8_t)ConvertXP64Value(Code.Value)); }
        break;
    case 0xE9000000:
        Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        if (Execute) { MMU->SH_VAddr(Address, ConvertXP64Value(Code.Value)); }
        break;
    case 0xC8000000:
        Address = 0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        if (Execute) { MMU->SB_VAddr(Address, (uint8_t)Code.Value); }
        break;
    case 0xC9000000:
        Address = 0xA0000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        if (Execute) { MMU->SH_VAddr(Address, ConvertXP64Value(Code.Value)); }
        break;
    case 0xB8000000:
        Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        MMU->LB_VAddr(Address, bMemory);
        if (bMemory != ConvertXP64Value(Code.Value)) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0xB9000000:
        Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        MMU->LH_VAddr(Address, wMemory);
        if (wMemory != ConvertXP64Value(Code.Value)) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0xBA000000:
        Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        MMU->LB_VAddr(Address, bMemory);
        if (bMemory == ConvertXP64Value(Code.Value)) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0xBB000000:
        Address = 0x80000000 | (ConvertXP64Address(Code.Command) & 0xFFFFFF);
        MMU->LH_VAddr(Address, wMemory);
        if (wMemory == ConvertXP64Value(Code.Value)) { Execute = false; }
        return ApplyCheatEntry(MMU, CodeEntry, CurrentEntry + 1, Execute) + 1;
    case 0: return MaxGSEntries; break;
    }
    return 1;
}