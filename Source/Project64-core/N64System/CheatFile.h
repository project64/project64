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
#include <Common\FileClass.h>
#include <Common\CriticalSection.h>
#include <string>
#include <vector>
#include <memory>

class CCheatDetails
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

    CCheatDetails();
    void SetName(const char * Name);
    void SetNote(const char * Note);
    bool SetEntries(const CodeEntries & Entries);
    void SetOptions(const CodeOptions & Options);

    inline const std::string GetName(void) const { return m_Name; }
    inline const std::string GetNote(void) const { return m_Note; }
    inline const CodeEntries & GetEntries(void) const { return m_Entries; }
    inline const CodeOptions & GetOptions(void) const { return m_Options; }
    inline uint32_t CodeOptionSize(void) const { return m_CodeOptionSize; }
    inline bool Valid (void) const { return m_Valid; }

private:
    void CheckValid();

    std::string m_Name;
    std::string m_Note;
    CodeEntries m_Entries;
    CodeOptions m_Options;
    std::string m_OptionValue;
    uint32_t m_CodeOptionSize;
    bool m_Valid;
};

class CCheatFile
{
    struct insensitive_compare
    {
        bool operator() (const std::string & a, const std::string & b) const
        {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        }
    };

    typedef std::map<std::string, CCheatDetails, insensitive_compare> KeyValueList;
    typedef std::map<std::string, long> FILELOC;
    typedef FILELOC::iterator FILELOC_ITR;

public:
    CCheatFile(const char * FileName);
    ~CCheatFile();

    void SetName(const char * Section, const char * Name);
    void SetCode(const char * Section, const CCheatDetails & Details);

private:
    CCheatFile();
    CCheatFile(const CCheatFile&);
    CCheatFile& operator=(const CCheatFile&);

    void OpenCheatFile(void);
    bool MoveToSection(const char * lpSectionName, bool ChangeCurrentSection);
    void SaveCurrentSection(void);
    int GetStringFromFile(char * & String, std::unique_ptr<char> &Data, int & MaxDataSize, int & DataSize, int & ReadPos);
    const char * CleanLine(char * Line);

    CriticalSection m_CS;
    std::string m_CurrentSection;
    std::string m_SectionName;
    KeyValueList m_CurrentSectionData;
    long m_lastSectionSearch;
    FILELOC m_SectionsPos;
    int m_CurrentSectionFilePos;
    bool m_CurrentSectionDirty;
    bool m_ReadOnly;
    CFile m_File;
    std::string m_FileName;
    const char * m_LineFeed;
};