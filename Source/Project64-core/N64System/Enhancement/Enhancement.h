#pragma once
#include <stdint.h>
#include <string>
#include <vector>

class CEnhancement
{
public:
    static const char * CheatIdent;
    static const char * EnhancementIdent;

    struct CodeEntry
    {
        uint32_t Command;
        std::string Value;
    };
    struct CodeOption
    {
        std::string Name;
        uint16_t Value;
    };
    typedef std::vector<CodeEntry> CodeEntries;
    typedef std::vector<CodeOption> CodeOptions;
    typedef std::vector<std::string> PluginList;

    CEnhancement(const char * Ident);
    CEnhancement(const char * Ident, const char * Entry);
    void SetName(const char * Name);
    void SetAuthor(const char * Author);
    void SetNote(const char * Note);
    void SetEntries(const CodeEntries & Entries);
    void SetOptions(const CodeOptions & Options);
    void SetPluginList(const PluginList & List);
    void SetSelectedOption(uint16_t Value);
    void SetActive(bool Active);
    void SetOnByDefault(bool OnByDefault);
    void SetOverClock(bool OverClock, uint32_t OverClockModifier);
    void SetCounterFactor(bool hasValue, uint32_t value);
    void SetViRefresh(bool hasValue, uint32_t value);
    void SetRdramSize(bool hasValue, uint32_t value);
    void SetSmmProtect(bool hasValue, bool value);
    void SetFixedAudio(bool hasValue, bool value);
    void SetSyncAudio(bool hasValue, bool value);

    inline const std::string & GetName(void) const
    {
        return m_Name;
    }
    inline const std::string & GetNameAndExtension(void) const
    {
        return m_NameAndExtension;
    }
    inline const std::string & GetAuthor(void) const
    {
        return m_Author;
    }
    inline const std::string & GetNote(void) const
    {
        return m_Note;
    }
    inline const CodeEntries & GetEntries(void) const
    {
        return m_Entries;
    }
    inline const CodeOptions & GetOptions(void) const
    {
        return m_Options;
    }
    inline const PluginList & GetPluginList(void) const
    {
        return m_PluginList;
    }
    inline uint32_t CodeOptionSize(void) const
    {
        return m_CodeOptionSize;
    }
    inline bool Valid(void) const
    {
        return m_Valid;
    }
    inline bool Active(void) const
    {
        return m_Active;
    }
    inline bool GetOnByDefault(void) const
    {
        return m_OnByDefault;
    }
    inline bool OverClock(void) const
    {
        return m_OverClock;
    }
    inline uint32_t OverClockModifier(void) const
    {
        return m_OverClockModifier;
    }
    inline bool HasCounterFactor() const
    {
        return m_HasCounterFactor;
    }
    inline uint32_t CounterFactor() const
    {
        return m_CounterFactor;
    }
    inline bool HasViRefresh() const
    {
        return m_HasViRefresh;
    }
    inline uint32_t ViRefresh() const
    {
        return m_ViRefresh;
    }
    inline bool HasRdramSize() const
    {
        return m_HasRdramSize;
    }
    inline uint32_t RdramSize() const
    {
        return m_RdramSize;
    }
    inline bool HasSmmProtect() const
    {
        return m_HasSmmProtect;
    }
    inline bool SmmProtect() const
    {
        return m_SmmProtect;
    }
    inline bool HasFixedAudio() const
    {
        return m_HasFixedAudio;
    }
    inline bool FixedAudio() const
    {
        return m_FixedAudio;
    }
    inline bool HasSyncAudio() const
    {
        return m_HasSyncAudio;
    }
    inline bool SyncAudio() const
    {
        return m_SyncAudio;
    }
    bool OptionSelected() const
    {
        return (m_SelectedOption & 0xFFFF0000) == 0;
    }
    uint16_t SelectedOption() const
    {
        return (uint16_t)(m_SelectedOption & 0xFFFF);
    }

private:
    CEnhancement();

    void CheckValid();

    std::string m_Ident;
    std::string m_Name;
    std::string m_NameAndExtension;
    std::string m_Author;
    std::string m_Note;
    PluginList m_PluginList;
    CodeEntries m_Entries;
    CodeOptions m_Options;
    std::string m_OptionValue;
    uint32_t m_CodeOptionSize;
    uint32_t m_SelectedOption;
    bool m_OnByDefault;
    bool m_OverClock;
    uint32_t m_OverClockModifier;
    bool m_Active;
    bool m_Valid;
    bool m_HasCounterFactor = false;
    uint32_t m_CounterFactor = 0;
    bool m_HasViRefresh = false;
    uint32_t m_ViRefresh = 0;
    bool m_HasRdramSize = false;
    uint32_t m_RdramSize = 0;
    bool m_HasSmmProtect = false;
    bool m_SmmProtect = false;
    bool m_HasFixedAudio = false;
    bool m_FixedAudio = false;
    bool m_HasSyncAudio = false;
    bool m_SyncAudio = false;
};
