#pragma once
#include <Common/CriticalSection.h>
#include <Common/Thread.h>
#include <Project64-core/N64System/Enhancement/EnhancementFile.h>
#include <Project64-core/N64System/Enhancement/EnhancementList.h>
#include <map>
#include <string>

class CMipsMemoryVM;
class CPlugins;

class CEnhancements
{
    enum
    {
        MaxGSEntries = 100,
    };

public:
    CEnhancements();
    ~CEnhancements();

    void ApplyActive(CMipsMemoryVM & MMU, CPlugins * Plugins, bool UpdateChanges);
    void ApplyGSButton(CMipsMemoryVM & MMU, bool UpdateChanges);
    void UpdateCheats(const CEnhancementList & Cheats);
    void UpdateCheats(void);
    void UpdateEnhancements(const CEnhancementList & Enhancements);
    void ResetActive(CPlugins * Plugins);
    void Load(void);
    void LoadActive(CMipsMemoryVM * MMU, CPlugins * Plugins);

    CEnhancementList Cheats(void);
    CEnhancementList Enhancements(void);

private:
    class GAMESHARK_CODE
    {
    public:
        GAMESHARK_CODE(const GAMESHARK_CODE &);
        GAMESHARK_CODE(uint32_t Command, uint16_t Value, bool HasDisableValue, uint16_t DisableValue);

        uint32_t Command(void) const
        {
            return m_Command;
        }
        uint16_t Value(void) const
        {
            return m_Value;
        }
        bool HasDisableValue(void) const
        {
            return m_HasDisableValue;
        }
        uint16_t DisableValue(void) const
        {
            return m_DisableValue;
        }

    private:
        GAMESHARK_CODE();
        GAMESHARK_CODE & operator=(const GAMESHARK_CODE &);

        uint32_t m_Command;
        uint16_t m_Value;
        bool m_HasDisableValue;
        uint16_t m_DisableValue;
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
    void LoadActive(CEnhancementList & List, CPlugins * Plugins);
    void LoadEnhancements(const char * Ident, SectionFiles & Files, std::unique_ptr<CEnhancmentFile> & File, CEnhancementList & EnhancementList);
    void ApplyGameSharkCodes(CMipsMemoryVM & MMU, CODES & CodeEntry, uint32_t CurrentEntry);
    uint32_t EntrySize(const CODES & CodeEntry, uint32_t CurrentEntry);
    void ModifyMemory8(CMipsMemoryVM & MMU, uint32_t Address, uint8_t Value, bool HasDisableValue, uint8_t DisableValue);
    void ModifyMemory16(CMipsMemoryVM & MMU, uint32_t Address, uint16_t Value, bool HasDisableValue, uint16_t DisableValue);
    void ScanFileThread(void);
    void WaitScanDone(void);
    void GameChanged(void);

    static uint32_t ConvertXP64Address(uint32_t Address);
    static uint16_t ConvertXP64Value(uint16_t Value);

    static uint32_t stScanFileThread(void * lpThreadParameter)
    {
        ((CEnhancements *)lpThreadParameter)->ScanFileThread();
        return 0;
    }

    CriticalSection m_CS;
    SectionFiles m_CheatFiles, m_EnhancementFiles;
    std::unique_ptr<CEnhancmentFile> m_CheatFile, m_EnhancementFile;
    CEnhancementList m_Cheats;
    CEnhancementList m_Enhancements;
    CODES_ARRAY m_ActiveCodes;
    CODES_ARRAY m_GSButtonCodes;
    ORIGINAL_VALUES16 m_OriginalValues16;
    ORIGINAL_VALUES8 m_OriginalValues8;
    CThread m_ScanFileThread;
    bool m_Scan;
    bool m_Scanned;
    bool m_UpdateCheats;
    bool m_OverClock;
    uint32_t m_OverClockModifier;
};