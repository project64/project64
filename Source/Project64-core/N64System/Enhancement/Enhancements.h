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
#include <Project64-core\N64System\Enhancement\EnhancementFile.h>
#include <Project64-core\N64System\Enhancement\EnhancementList.h>
#include <Common\Thread.h>
#include <Common\CriticalSection.h>
#include <map>
#include <string>

class CMipsMemoryVM;

class CEnhancements
{
    enum
    {
        MaxGSEntries = 100,
    };

public:
    CEnhancements();
    ~CEnhancements();
    
    void ApplyActive(CMipsMemoryVM & MMU, bool UpdateChanges);
    void ApplyGSButton(CMipsMemoryVM & MMU, bool UpdateChanges);
    void UpdateCheats(const CEnhancementList & Cheats );
    void UpdateCheats(void);

    inline const CEnhancementList & Cheats(void) const { return m_Cheats; }

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

    typedef std::map<std::string, std::string> SectionFiles;
    typedef std::vector<GAMESHARK_CODE> CODES;
    typedef std::vector<CODES> CODES_ARRAY;
    typedef std::map<uint32_t, MEM_VALUE16> ORIGINAL_VALUES16;
    typedef std::map<uint32_t, MEM_VALUE8> ORIGINAL_VALUES8;

    void ResetCodes(CMipsMemoryVM * MMU);
    void LoadCheats(CMipsMemoryVM * MMU);
    void ApplyGameSharkCodes(CMipsMemoryVM & MMU, CODES & CodeEntry, uint32_t CurrentEntry);
    uint32_t EntrySize(const CODES & CodeEntry, uint32_t CurrentEntry);
    void ModifyMemory8(CMipsMemoryVM & MMU, uint32_t Address, uint8_t Value);
    void ModifyMemory16(CMipsMemoryVM & MMU, uint32_t Address, uint16_t Value);
    void ScanFileThread(void);
    void WaitScanDone(void);
    void GameChanged(void);

    static uint32_t ConvertXP64Address(uint32_t Address);
    static uint16_t ConvertXP64Value(uint16_t Value);

    static uint32_t stScanFileThread(void * lpThreadParameter) { ((CEnhancements *)lpThreadParameter)->ScanFileThread(); return 0; }
    static void stGameChanged(void * lpData) { ((CEnhancements *)lpData)->GameChanged(); }

    CriticalSection m_CS;
    std::string m_SectionIdent;
    SectionFiles m_CheatFiles;
    std::unique_ptr<CEnhancmentFile> m_CheatFile;
    CEnhancementList m_Cheats;
    CODES_ARRAY m_ActiveCodes;
    CODES_ARRAY m_GSButtonCodes;
    ORIGINAL_VALUES16 m_OriginalValues16;
    ORIGINAL_VALUES8 m_OriginalValues8;
    CThread m_ScanFileThread;
    bool m_Scan;
    bool m_Scanned;
    bool m_UpdateCheats;
};