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
#include "N64RomClass.h"
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/Plugins/PluginClass.h>

class CCheats
{
public:
    CCheats(CMipsMemoryVM & MMU);
    ~CCheats(void);

    enum
    {
        MaxCheats = 50000,
        MaxGSEntries = 100,
    };

    void ApplyCheats();
    void ApplyGSButton();
    void LoadCheats(bool DisableSelected, CPlugins * Plugins);

    static bool IsValid16BitCode(const char * CheatString);

private:
    struct GAMESHARK_CODE
    {
        uint32_t Command;
        uint16_t Value;
    };

    struct MEM_VALUE16
    {
        uint16_t Original;
        uint16_t Changed;
    };

    struct MEM_VALUE8
    {
        uint8_t Original;
        uint8_t Changed;
    };

    typedef std::vector<GAMESHARK_CODE> CODES;
    typedef std::vector<CODES> CODES_ARRAY;
    typedef std::map<uint32_t, MEM_VALUE16> ORIGINAL_VALUES16;
    typedef std::map<uint32_t, MEM_VALUE8> ORIGINAL_VALUES8;

    void LoadPermCheats(CPlugins * Plugins);
    int32_t EntrySize(const CODES & CodeEntry, int32_t CurrentEntry);

    CMipsMemoryVM & m_MMU;
    CODES_ARRAY m_Codes;
    ORIGINAL_VALUES16 m_OriginalValues16;
    ORIGINAL_VALUES8 m_OriginalValues8;

    bool LoadCode(const stdstr & CheatEntry, SettingID ExtensionSetting, int ExtensionIndex);
    void ApplyCheatEntry(CODES & CodeEntry, int32_t CurrentEntry);
    void ModifyMemory8(uint32_t Address, uint8_t Value);
    void ModifyMemory16(uint32_t Address, uint16_t Value);
    void ResetCodes(void);
};
