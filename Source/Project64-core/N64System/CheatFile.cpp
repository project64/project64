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
#include <Project64-core\N64System\CheatFile.h>

#include <Windows.h> // for DebugBreak should be removed

CCheatDetails::CCheatDetails() :
    m_CodeOptionSize(0),
    m_Valid(false)
{
}

void CCheatDetails::SetName(const char * Name)
{
    m_Name = Name != NULL ? Name : "";
    m_Name = stdstr(m_Name).Trim("\t ,");
    CheckValid();
}

void CCheatDetails::SetNote(const char * Note)
{
    m_Note = Note != NULL ? Note : "";
}

bool CCheatDetails::SetEntries(const CodeEntries & Entries)
{
    std::string OptionValue;
    uint32_t CodeOptionSize = 0;
    for (CodeEntries::const_iterator itr = Entries.begin(); itr != Entries.end(); itr++)
    {
        switch (itr->Command & 0xFF000000)
        {
        case 0x80000000:
        case 0x81000000:
        case 0x88000000:
        case 0x89000000:
        case 0xA0000000:
        case 0xA1000000:
        case 0xD0000000:
        case 0xD1000000:
        case 0xD2000000:
        case 0xD3000000:
            if (strchr(itr->Value.c_str(), '?') != NULL)
            {
                if (strncmp(itr->Value.c_str(), "????", 4) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "????")
                    {
                        return false;
                    }
                    OptionValue = "????";
                    CodeOptionSize = 4;
                }
                else if (itr->Value.length() == 4 && strncmp(&itr->Value.c_str()[2], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "XX??")
                    {
                        return false;
                    }
                    OptionValue = "XX??";
                    CodeOptionSize = 2;
                }
                else if (itr->Value.length() == 4 && strncmp(&itr->Value.c_str()[0], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "??XX")
                    {
                        return false;
                    }
                    OptionValue = "??XX";
                    CodeOptionSize = 2;
                }
                else
                {
                    return false;
                }
            }
            break;
        case 0x50000000:
            if (strchr(itr->Value.c_str(), '?') != NULL)
            {
                return false;
            }
            break;
        default:
            return false;
        }
    }
    m_Entries = Entries;
    m_OptionValue = OptionValue;
    m_CodeOptionSize = CodeOptionSize;
    CheckValid();
    return true;
}

void CCheatDetails::SetOptions(const CodeOptions & Options)
{
    m_Options = Options;
    CheckValid();
}

void CCheatDetails::CheckValid(void)
{
    m_Valid = false;
    if (m_Name.empty() || m_Entries.size() == 0)
    {
        return;
    }

    if (m_Options.size() > 0 && m_OptionValue.empty())
    {
        return;
    }
    m_Valid = true;
}

CCheatFile::CCheatFile(const char * FileName) :
    m_lastSectionSearch(0),
    m_CurrentSectionFilePos(0),
    m_CurrentSectionDirty(false),
    m_ReadOnly(false),
    m_FileName(FileName),
    m_LineFeed("\r\n")
{
    OpenCheatFile();
}

CCheatFile::~CCheatFile()
{
    SaveCurrentSection();
}

void CCheatFile::SetName(const char * Section, const char * Name)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen())
    {
        return;
    }

    if (!MoveToSection(Section, true))
    {
        m_CurrentSection = Section;
        m_CurrentSectionData.clear();
        m_CurrentSectionFilePos = -1;
    }
    if (strcmp(m_SectionName.c_str(), Name) != 0)
    {
        m_SectionName = Name;
        m_CurrentSectionDirty = false;
    }
}

void CCheatFile::SetCode(const char * Section, const CCheatDetails & Details)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen() || m_ReadOnly)
    {
        return;
    }

    m_CurrentSectionDirty = true;
    if (!MoveToSection(Section, true))
    {
        m_CurrentSection = Section;
        m_CurrentSectionData.clear();
        m_CurrentSectionFilePos = -1;
    }
    KeyValueList::iterator itr = m_CurrentSectionData.find(Details.GetName());
    if (itr != m_CurrentSectionData.end())
    {
        itr->second = Details;

    }
    else
    {
        m_CurrentSectionData.insert(KeyValueList::value_type(Details.GetName(), Details));
    }
}

void CCheatFile::OpenCheatFile(void)
{
    m_ReadOnly = false;
    if (!m_File.Open(m_FileName.c_str(), CFileBase::modeReadWrite | CFileBase::shareDenyWrite))
    {
        if (!m_File.Open(m_FileName.c_str(), CFileBase::modeRead))
        {
            if (!m_File.Open(m_FileName.c_str(), CFileBase::modeReadWrite | CFileBase::modeCreate | CFileBase::shareDenyWrite))
            {
                return;
            }
        }
        else
        {
            m_ReadOnly = true;
        }
    }
    m_File.Seek(0, CFileBase::begin);
}

bool CCheatFile::MoveToSection(const char * lpSectionName, bool ChangeCurrentSection)
{
    if (strcmp(lpSectionName, m_CurrentSection.c_str()) == 0)
    {
        return true;
    }
    if (ChangeCurrentSection)
    {
        SaveCurrentSection();
        m_CurrentSection = "";
    }

    std::unique_ptr<char> Data;
    char *Input = NULL;
    int MaxDataSize = 0, DataSize = 0, ReadPos = 0, result;

    FILELOC_ITR iter = m_SectionsPos.find(std::string(lpSectionName));
    bool bFoundSection = false;
    if (iter != m_SectionsPos.end())
    {
        if (ChangeCurrentSection)
        {
            m_CurrentSection = iter->first;
            m_CurrentSectionFilePos = iter->second;
        }
        m_File.Seek(iter->second, CFileBase::begin);
        bFoundSection = true;
    }
    else
    {
        m_File.Seek(m_lastSectionSearch, CFileBase::begin);

        //long Fpos;
        uint8_t pUTF8[3];
        pUTF8[0] = 0xef;
        pUTF8[1] = 0xbb;
        pUTF8[2] = 0xbf;

        do
        {
            result = GetStringFromFile(Input, Data, MaxDataSize, DataSize, ReadPos);
            if (result <= 1) { continue; }
            if (strlen(CleanLine(Input)) <= 1) { continue; }

            //We Only care about sections
            char * CurrentSection = Input;

            if (m_lastSectionSearch == 0 && !memcmp(CurrentSection, pUTF8, 3))
            {
                CurrentSection += 3;
            }

            if (CurrentSection[0] != '[') { continue; }
            int lineEndPos = (int)strlen(CurrentSection) - 1;
            if (CurrentSection[lineEndPos] != ']') { continue; }
            //take off the ']' from the end of the string
            CurrentSection[lineEndPos] = 0;
            CurrentSection += 1;
            m_lastSectionSearch = (m_File.GetPosition() - DataSize) + ReadPos;
            m_SectionsPos.insert(FILELOC::value_type(CurrentSection, m_lastSectionSearch));

            if (_stricmp(lpSectionName, CurrentSection) != 0)
            {
                continue;
            }

            if (ChangeCurrentSection)
            {
                m_CurrentSection = lpSectionName;
                m_CurrentSectionFilePos = m_lastSectionSearch;
            }
            else
            {
                m_File.Seek(m_lastSectionSearch, CFileBase::begin);
            }
            bFoundSection = true;
            break;
        } while (result >= 0);
    }

    if (!bFoundSection && strcmp(lpSectionName, "default") == 0)
    {
        m_SectionsPos.insert(FILELOC::value_type(lpSectionName, 0));
        if (ChangeCurrentSection)
        {
            m_CurrentSection = lpSectionName;
            m_CurrentSectionFilePos = 0;
        }
        m_File.Seek(m_lastSectionSearch, CFileBase::begin);
        bFoundSection = true;
    }

    if (bFoundSection && ChangeCurrentSection)
    {
        m_CurrentSectionData.clear();
        /*do
        {
            result = GetStringFromFile(Input, Data, MaxDataSize, DataSize, ReadPos);
            if (result <= 1) { continue; }
            if (strlen(CleanLine(Input)) <= 1) { continue; }
            if (Input[0] == '[') { break; }
            char * Pos = strchr(Input, '=');
            if (Pos == NULL) { continue; }
            char * Value = &Pos[1];

            char * Pos1 = Pos - 1;
            while (((*Pos1 == ' ') || (*Pos1 == '\t')) && (Pos1 > Input))
            {
                Pos1--;
            }
            Pos1[1] = 0;

            m_CurrentSectionData.insert(KeyValueList::value_type(Input, Value));
        } while (result >= 0);*/
    }
    return bFoundSection;
}

void CCheatFile::SaveCurrentSection(void)
{
    if (!m_CurrentSectionDirty)
    {
        return;
    }
    m_CurrentSectionDirty = false;

    std::string Section;
    if (!m_SectionName.empty())
    {
        Section = stdstr_f("Name=%s%s%s", m_SectionName.c_str(), m_LineFeed, m_LineFeed);
    }
    for (KeyValueList::iterator iter = m_CurrentSectionData.begin(); iter != m_CurrentSectionData.end(); iter++)
    {
        CCheatDetails & details = iter->second;
        if (details.GetName().empty())
        {
            continue;
        }
        Section += stdstr_f("$%s%s", details.GetName().c_str(), m_LineFeed);
        if (!details.GetNote().empty())
        {
            Section += stdstr_f("Note=%s%s", details.GetNote().c_str(), m_LineFeed);
        }
        const CCheatDetails::CodeEntries & Entries = details.GetEntries();
        for (size_t i = 0, n = Entries.size(); i < n; i++)
        {
            Section += stdstr_f("%08X %s%s", Entries[i].Command, Entries[i].Value.c_str(), m_LineFeed);
        }
        const CCheatDetails::CodeOptions & Options = details.GetOptions();
        for (size_t i = 0, n = Options.size(); i < n; i++)
        {
            if (details.CodeOptionSize() == 4)
            {
                Section += stdstr_f("%04X %s%s", Options[i].Value, Options[i].Name.c_str(), m_LineFeed);
            } 
            else if (details.CodeOptionSize() == 2)
            {
                Section += stdstr_f("%02X %s%s", Options[i].Value, Options[i].Name.c_str(), m_LineFeed);
            }
        }
        Section += m_LineFeed;
    }

    int lineFeedLen = (int)strlen(m_LineFeed);
    if (m_CurrentSectionFilePos == -1)
    {
        m_File.Seek(0, CFileBase::end);

        int len = (int)m_CurrentSection.length() + (lineFeedLen * 2) + 5;
        std::unique_ptr<char> SectionName(new char[len]);
        if (m_File.GetLength() < (int)strlen(m_LineFeed))
        {
            sprintf(SectionName.get(), "[%s]%s", m_CurrentSection.c_str(), m_LineFeed);
        }
        else
        {
            sprintf(SectionName.get(), "%s[%s]%s", m_LineFeed, m_CurrentSection.c_str(), m_LineFeed);
        }
        m_File.Write(SectionName.get(), (int)strlen(SectionName.get()));
        m_CurrentSectionFilePos = m_File.GetPosition();
        m_SectionsPos.insert(FILELOC::value_type(m_CurrentSection, m_CurrentSectionFilePos));
    } 
    else
    {
        DebugBreak();
    }
    m_File.Write(Section.data(), Section.length());
}

int CCheatFile::GetStringFromFile(char * & String, std::unique_ptr<char> &Data, int & MaxDataSize, int & DataSize, int & ReadPos)
{
    enum { BufferIncrease = 0x2000 };
    if (MaxDataSize == 0)
    {
        ReadPos = 0;
        MaxDataSize = BufferIncrease;
        Data.reset(new char[MaxDataSize]);
        DataSize = m_File.Read(&Data.get()[DataSize], MaxDataSize);
    }

    for (;;)
    {
        int count;

        for (count = ReadPos; count < DataSize; count++)
        {
            if (Data.get()[count] == '\n')
            {
                int len = (count - ReadPos) + 1;
                String = &Data.get()[ReadPos];
                String[len - 1] = 0;
                ReadPos = count + 1;
                return len;
            }
        }

        if (ReadPos != 0)
        {
            if ((DataSize - ReadPos) > 0)
            {
                memmove(Data.get(), &Data.get()[ReadPos], DataSize - ReadPos);
            }
            DataSize -= ReadPos;
            ReadPos = 0;
        }
        else
        {
            //Increase buffer size
            int NewMaxDataSize = MaxDataSize + BufferIncrease;
            char * NewBuffer = new char[NewMaxDataSize];
            if (NewBuffer == NULL)
            {
                return -1;
            }
            memcpy(NewBuffer, Data.get(), DataSize);
            MaxDataSize = NewMaxDataSize;
            Data.reset(NewBuffer);
        }

        int dwRead = m_File.Read(&Data.get()[DataSize], MaxDataSize - DataSize);
        if (dwRead == 0)
        {
            if (DataSize > 0)
            {
                int len = DataSize + 1;
                String = &Data.get()[ReadPos];
                String[len - 1] = 0;
                DataSize = 0;
                ReadPos = 0;
                return len;
            }
            return -1;
        }
        DataSize += dwRead;
    }
}

const char * CCheatFile::CleanLine(char * Line)
{
    char * Pos = Line;

    //Remove any comment from the line
    while (Pos != NULL)
    {
        Pos = strchr(Pos, '/');
        if (Pos != NULL)
        {
            if (Pos[1] == '/')
            {
                if (Pos > Line)
                {
                    char * Pos_1 = Pos - 1;

                    if (Pos_1[0] != ':')
                    {
                        Pos[0] = 0;
                    }
                    else
                        Pos += 1;
                }
                else
                {
                    Pos[0] = 0;
                }
            }
            else
            {
                Pos += 1;
            }
        }
    }

    //strip any spaces or line feeds from the end of the line
    for (int count = (int)strlen(&Line[0]) - 1; count >= 0; count--)
    {
        if (Line[count] != ' ' && Line[count] != '\r') { break; }
        Line[count] = 0;
    }
    return Line;
}
