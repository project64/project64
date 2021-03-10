#pragma once
#include <Common/stdtypes.h>
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
    void SetNote(const char * Note);
    void SetEntries(const CodeEntries & Entries);
    void SetOptions(const CodeOptions & Options);
    void SetPluginList(const PluginList & List);
    void SetSelectedOption(uint16_t Value);
    void SetActive(bool Active);
    void SetOnByDefault(bool OnByDefault);
    void SetOverClock(bool OverClock, uint32_t OverClockModifier);

    inline const std::string & GetName(void) const { return m_Name; }
    inline const std::string & GetNameAndExtension(void) const { return m_NameAndExtension; }
    inline const std::string & GetNote(void) const { return m_Note; }
    inline const CodeEntries & GetEntries(void) const { return m_Entries; }
    inline const CodeOptions & GetOptions(void) const { return m_Options; }
    inline const PluginList & GetPluginList(void) const { return m_PluginList; }
    inline uint32_t CodeOptionSize(void) const { return m_CodeOptionSize; }
    inline bool Valid(void) const { return m_Valid; }
    inline bool Active(void) const { return m_Active; }
    inline bool GetOnByDefault(void) const { return m_OnByDefault; }
    inline bool OverClock(void) const { return m_OverClock; }
    inline uint32_t OverClockModifier(void) const { return m_OverClockModifier; }
    bool OptionSelected() const { return (m_SelectedOption & 0xFFFF0000) == 0; }
    uint16_t SelectedOption() const { return (uint16_t)(m_SelectedOption & 0xFFFF); }

private:
    CEnhancement();

    void CheckValid();

    std::string m_Ident;
    std::string m_Name;
    std::string m_NameAndExtension;
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
};
