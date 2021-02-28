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
#include <Common/stdtypes.h>
#include <string>
#include <vector>

class CEnhancement
{
public:
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
    bool OptionSelected() const { return (m_SelectedOption & 0xFFFF0000) == 0; }
    uint16_t SelectedOption() const { return (uint16_t)(m_SelectedOption & 0xFFFF); }

private:
    CEnhancement();
    CEnhancement& operator=(const CEnhancement&);

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
    bool m_Active;
    bool m_Valid;
};
