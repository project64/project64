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
#include <Project64-core/N64System/Enhancement/EnhancementFile.h>

#pragma warning(disable:4996)

CEnhancmentFile::CEnhancmentFile(const char * FileName, const char * Ident) :
    m_Ident(Ident),
    m_lastSectionSearch(0),
    m_CurrentSectionFilePos(0),
    m_CurrentSectionDirty(false),
    m_ReadOnly(false),
    m_FileName(FileName),
    m_LineFeed("\r\n")
{
    OpenFile();
}

CEnhancmentFile::~CEnhancmentFile()
{
    SaveCurrentSection();
}

void CEnhancmentFile::GetSections(SectionList & sections)
{
    sections.clear();

    CGuard Guard(m_CS);
    if (!m_File.IsOpen())
    {
        return;
    }

    MoveToSection(stdstr_f("DoesNotExist%d%d%d", rand(), rand(), rand()).c_str(), false);
    for (FILELOC::const_iterator iter = m_SectionsPos.begin(); iter != m_SectionsPos.end(); iter++)
    {
        sections.insert(iter->first);
    }
}

void CEnhancmentFile::SetName(const char * Section, const char * Name)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen())
    {
        return;
    }

    MoveToSection(Section, true);
    if (strcmp(m_SectionName.c_str(), Name) != 0)
    {
        m_SectionName = Name;
        m_CurrentSectionDirty = true;
    }
}

bool CEnhancmentFile::AddEnhancement(const char * Section, const CEnhancement & Details)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen() || m_ReadOnly)
    {
        return false;
    }

    MoveToSection(Section, true);
    if (AddEnhancement(Details))
    {
        m_CurrentSectionDirty = true;
        return true;
    }
    return false;
}

bool CEnhancmentFile::GetEnhancementList(const char * Section, CEnhancementList & List)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen())
    {
        return false;
    }

    if (!MoveToSection(Section, true))
    {
        return false;
    }
    List = m_EnhancementList;
    return true;
}

bool CEnhancmentFile::RemoveEnhancements(const char * Section)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen())
    {
        return false;
    }

    if (!MoveToSection(Section, true))
    {
        return false;
    }
    if (m_EnhancementList.size() > 0)
    {
        m_EnhancementList.clear();
        m_CurrentSectionDirty = true;
    }
    return true;
}

bool CEnhancmentFile::GetName(const char * Section, std::string &Name)
{
    CGuard Guard(m_CS);
    if (!m_File.IsOpen())
    {
        return false;
    }

    if (!MoveToSection(Section, true))
    {
        return false;
    }
    Name = m_SectionName;
    return true;
}

bool CEnhancmentFile::AddEnhancement(const CEnhancement & Details)
{
    CEnhancementList::iterator itr = m_EnhancementList.FindItem(Details.GetName());
    if (itr != m_EnhancementList.end())
    {
        return false;
    }
    m_EnhancementList.AddItem(Details);
    return true;
}

void CEnhancmentFile::OpenFile(void)
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

bool CEnhancmentFile::MoveToSection(const char * Section, bool ChangeCurrentSection)
{
    if (ChangeCurrentSection)
    {
        SaveCurrentSection();
        m_CurrentSection = "";
        m_SectionName.clear();
        m_EnhancementList.clear();
    }

    std::unique_ptr<char> Data;
    char *Input = NULL;
    int MaxDataSize = 0, DataSize = 0, ReadPos = 0, result;

    FILELOC_ITR iter = m_SectionsPos.find(std::string(Section));
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

            if (_stricmp(Section, CurrentSection) != 0)
            {
                continue;
            }

            if (ChangeCurrentSection)
            {
                m_CurrentSection = Section;
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

    if (bFoundSection && ChangeCurrentSection)
    {
        do
        {
            result = GetStringFromFile(Input, Data, MaxDataSize, DataSize, ReadPos);
            if (result <= 1) { continue; }
            if (strlen(CleanLine(Input)) <= 1) { continue; }
            if (Input[0] == '[') { break; }
            char * Pos = strchr(Input, '=');
            if (Input[0] == '$')
            {
                std::string Entry = Input;
                do
                {
                    result = GetStringFromFile(Input, Data, MaxDataSize, DataSize, ReadPos);
                    if (result <= 1 || strlen(CleanLine(Input)) <= 1) 
                    {
                        break;
                    }
                    else
                    {
                        Entry += m_LineFeed;
                        Entry += Input;
                    }
                } while (result >= 0);

                CEnhancement Enhancement(m_Ident.c_str(), Entry.c_str());
                if (Enhancement.Valid())
                {
                    AddEnhancement(Enhancement);
                }
            }
            else if (Pos != nullptr)
            { 
                char * Value = &Pos[1];

                char * Pos1 = Pos - 1;
                while (((*Pos1 == ' ') || (*Pos1 == '\t')) && (Pos1 > Input))
                {
                    Pos1--;
                }
                Pos1[1] = 0;

                if (stricmp(Input, "Name") == 0)
                {
                    m_SectionName = Value;
                }
            }
        } while (result >= 0);
    }

    if (!bFoundSection && ChangeCurrentSection)
    {
        m_CurrentSection = Section;
        m_EnhancementList.clear();
        m_CurrentSectionFilePos = -1;
    }
    return bFoundSection;
}

void CEnhancmentFile::SaveCurrentSection(void)
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
    for (CEnhancementList::iterator iter = m_EnhancementList.begin(); iter != m_EnhancementList.end(); iter++)
    {
        CEnhancement & Enhancement = iter->second;
        if (Enhancement.GetName().empty())
        {
            continue;
        }
        Section += stdstr_f("$%s%s", Enhancement.GetName().c_str(), m_LineFeed);
        if (!Enhancement.GetPluginList().empty())
        {
            const CEnhancement::PluginList & List = Enhancement.GetPluginList();
            std::string PluginList;
            for (size_t i = 0, n = List.size(); i < n; i++)
            {
                if (i > 0)
                {
                    PluginList += ",";
                }
                PluginList += List[i].c_str();
            }
            Section += stdstr_f("PluginList=%s%s", PluginList.c_str() , m_LineFeed);
        }
        if (Enhancement.GetOnByDefault())
        {
            Section += stdstr_f("OnByDefault=1%s", m_LineFeed);
        }
        if (!Enhancement.GetNote().empty())
        {
            Section += stdstr_f("Note=%s%s", Enhancement.GetNote().c_str(), m_LineFeed);
        }
        const CEnhancement::CodeEntries & Entries = Enhancement.GetEntries();
        for (size_t i = 0, n = Entries.size(); i < n; i++)
        {
            Section += stdstr_f("%08X %s%s", Entries[i].Command, Entries[i].Value.c_str(), m_LineFeed);
        }
        const CEnhancement::CodeOptions & Options = Enhancement.GetOptions();
        for (size_t i = 0, n = Options.size(); i < n; i++)
        {
            if (Enhancement.CodeOptionSize() == 4)
            {
                Section += stdstr_f("%04X %s%s", Options[i].Value, Options[i].Name.c_str(), m_LineFeed);
            } 
            else if (Enhancement.CodeOptionSize() == 2)
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
        int NeededBufferLen = Section.length();
        int currentLen = 0;

        m_File.Seek(m_CurrentSectionFilePos, CFileBase::begin);

        int MaxDataSize = 0, DataSize = 0, ReadPos = 0, result;
        std::unique_ptr<char> Data;
        char *Input = NULL;

        //Skip first line as it is the section name
        int StartPos = m_CurrentSectionFilePos;
        int EndPos = StartPos;
        do
        {
            result = GetStringFromFile(Input, Data, MaxDataSize, DataSize, ReadPos);
            if (result <= 1) { continue; }
            if (strlen(CleanLine(Input)) <= 1 || Input[0] != '[')
            {
                EndPos = ((m_File.GetPosition() - DataSize) + ReadPos);

                continue;
            }
            if (Input[0] == '[')
            {
                NeededBufferLen += lineFeedLen;
            }
            break;
        } while (result >= 0);
        currentLen = EndPos - StartPos;

        if (NeededBufferLen != currentLen)
        {
            fInsertSpaces(StartPos, NeededBufferLen - currentLen);
            m_File.Flush();
            ClearSectionPosList(StartPos);
        }
        m_File.Seek(StartPos, CFileBase::begin);
    }
    m_File.Write(Section.data(), Section.length());
}

int CEnhancmentFile::GetStringFromFile(char * & String, std::unique_ptr<char> & Data, int & MaxDataSize, int & DataSize, int & ReadPos)
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

const char * CEnhancmentFile::CleanLine(char * Line)
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
                    {
                        Pos += 1;
                    }
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
    for (int32_t i = (int32_t)strlen(&Line[0]) - 1; i >= 0; i--)
    {
        if (Line[i] != ' ' && Line[i] != '\r') 
        { 
            break; 
        }
        Line[i] = 0;
    }
    return Line;
}

void CEnhancmentFile::fInsertSpaces(int Pos, int NoOfSpaces)
{
    enum { fIS_MvSize = 0x2000 };

    unsigned char Data[fIS_MvSize + 1];
    int SizeToRead, result;
    long end, WritePos;

    m_File.Seek(0, CFileBase::end);
    end = m_File.GetPosition();

    if (NoOfSpaces > 0)
    {
        std::string SpaceBuffer = stdstr_f("%*c", NoOfSpaces, ' ');

        do
        {
            SizeToRead = end - Pos;
            if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
            if (SizeToRead > 0)
            {
                m_File.Seek(SizeToRead * -1, CFileBase::current);
                WritePos = m_File.GetPosition();
                memset(Data, 0, sizeof(Data));
                result = m_File.Read(Data, SizeToRead);
                m_File.Seek(WritePos, CFileBase::begin);
                end = WritePos;

                m_File.Write(SpaceBuffer.c_str(), (uint32_t)SpaceBuffer.length());
                m_File.Write(Data, result);
                m_File.Seek(WritePos, CFileBase::begin);
            }
        } while (SizeToRead > 0);
    }
    if (NoOfSpaces < 0)
    {
        int ReadPos = Pos + (NoOfSpaces * -1);
        WritePos = Pos;

        do
        {
            SizeToRead = end - ReadPos;
            if (SizeToRead > fIS_MvSize) { SizeToRead = fIS_MvSize; }
            m_File.Seek(ReadPos, CFileBase::begin);
            m_File.Read(Data, SizeToRead);
            m_File.Seek(WritePos, CFileBase::begin);
            m_File.Write(Data, SizeToRead);
            ReadPos += SizeToRead;
            WritePos += SizeToRead;
        } while (SizeToRead > 0);

        m_File.Seek(WritePos, CFileBase::begin);
        std::string SpaceBuffer = stdstr_f("%*c", (NoOfSpaces * -1), ' ');
        m_File.Write(SpaceBuffer.c_str(), (uint32_t)SpaceBuffer.length());

        m_File.Seek(WritePos, CFileBase::begin);
        m_File.SetEndOfFile();
        m_File.Seek(0, CFileBase::begin);
    }
}

void CEnhancmentFile::ClearSectionPosList(long FilePos)
{
    if (FilePos <= 0)
    {
        m_SectionsPos.clear();
        m_lastSectionSearch = 0;
    }
    else
    {
        FILELOC::iterator iter = m_SectionsPos.begin();
        while (iter != m_SectionsPos.end())
        {
            FILELOC::iterator CurrentIter = iter;
            iter++;
            long TestFilePos = CurrentIter->second;
            if (TestFilePos > FilePos)
            {
                m_SectionsPos.erase(CurrentIter);
            }
        }
        m_lastSectionSearch = FilePos;
    }
}

