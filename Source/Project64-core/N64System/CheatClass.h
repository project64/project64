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
#include <Project64-core/N64System/Mips/MemoryClass.h>
#include <Project64-core/Plugins/PluginClass.h>

class CCheats
{
public:
    CCheats();
    ~CCheats(void);

    enum
    {
        MaxCheats = 50000,
        MaxGSEntries = 100,
    };

    void ApplyCheats(CMipsMemory * MMU);
    void ApplyGSButton(CMipsMemory * MMU);
    void LoadCheats(bool DisableSelected, CPlugins * Plugins);

    static bool IsValid16BitCode(const char * CheatString);

private:
    struct GAMESHARK_CODE
    {
        uint32_t Command;
        uint16_t Value;
    };

    typedef std::vector<GAMESHARK_CODE> CODES;
    typedef std::vector<CODES>          CODES_ARRAY;

    void LoadPermCheats(CPlugins * Plugins);

    const CN64Rom * m_Rom;
    CODES_ARRAY   m_Codes;

    bool LoadCode(int32_t CheatNo, const char * CheatString);
    int32_t ApplyCheatEntry(CMipsMemory * MMU, const CODES & CodeEntry, int32_t CurrentEntry, bool Execute);
};
