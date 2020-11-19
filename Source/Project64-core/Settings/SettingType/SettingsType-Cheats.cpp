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
#include "SettingsType-Cheats.h"
#include <Project64-core\N64System\CheatClass.h>
#include <Common\path.h>
#include <Common\Thread.h>
#include <algorithm>

class CCheatInfo
{
    typedef std::map<std::string, std::string> SectionFiles;

public:
    CCheatInfo();
    ~CCheatInfo();

    void FlushChanges (void);
    bool Load(const char * PostFix, uint32_t Index, std::string & Value) const;
    void Save(const char * PostFix, uint32_t Index, const char * Value);
    void Delete(const char * PostFix, uint32_t Index);

private:
    void ScanFileThread(void);
    void GameChanged(void);
    void UseUserFile();

    static void CustomSortData(CIniFileBase::KeyValueVector & data);
    static uint32_t stScanFileThread(void * lpThreadParameter) { ((CCheatInfo *)lpThreadParameter)->ScanFileThread(); return 0; }
    static void stGameChanged(void * lpData) { ((CCheatInfo *)lpData)->GameChanged(); }

    std::string m_SectionIdent;
    CThread m_ScanFileThread;
    SectionFiles m_SectionFiles;
    CriticalSection m_CS;
    std::unique_ptr<CIniFile> m_CheatIniFile;
    bool m_Scanned;
    bool m_UsingUserFile;
};

CCheatInfo * CheatInfo = nullptr;

CSettingTypeCheats::CSettingTypeCheats(const char * PostFix) :
    m_PostFix(PostFix)
{
}

CSettingTypeCheats::~CSettingTypeCheats ( void )
{
}

void CSettingTypeCheats::Initialize ( void )
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");
    CheatInfo = new CCheatInfo();
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeCheats::CleanUp   ( void )
{
    if (CheatInfo != nullptr)
    {
        delete CheatInfo;
        CheatInfo = nullptr;
    }
}

void CSettingTypeCheats::FlushChanges( void )
{
    if (CheatInfo != nullptr)
    {
        CheatInfo->FlushChanges();
    }
}

bool CSettingTypeCheats::IsSettingSet(void) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeCheats::Load (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeCheats::Load (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeCheats::Load (uint32_t Index, std::string & Value ) const
{
    if (CheatInfo != nullptr)
    {
        return CheatInfo->Load(m_PostFix.c_str(), Index, Value);
    }
    return false;
}

void CSettingTypeCheats::LoadDefault (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::LoadDefault (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::Save (uint32_t /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::Save (uint32_t /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::Save (uint32_t Index, const std::string & Value )
{
    if (CheatInfo != nullptr)
    {
        CheatInfo->Save(m_PostFix.c_str(), Index, Value.c_str());
    }
}

void CSettingTypeCheats::Save (uint32_t Index, const char * Value )
{
    if (CheatInfo != nullptr)
    {
        CheatInfo->Save(m_PostFix.c_str(), Index, Value);
    }
}

void CSettingTypeCheats::Delete (uint32_t Index )
{
    if (CheatInfo != nullptr)
    {
        CheatInfo->Delete(m_PostFix.c_str(), Index);
    }
}

CCheatInfo::CCheatInfo() :
    m_ScanFileThread(stScanFileThread),
    m_Scanned(false),
    m_UsingUserFile(false)
{
    m_ScanFileThread.Start(this);
    g_Settings->RegisterChangeCB(Game_IniKey, this, stGameChanged);
}

CCheatInfo::~CCheatInfo()
{
    g_Settings->UnregisterChangeCB(Game_IniKey, this, stGameChanged);
}

void CCheatInfo::GameChanged(void)
{
    m_SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    
    for (uint32_t i = 0; i < 500; i++)
    {
        bool Scanned = false;
        {
            CGuard Guard(m_CS);
            Scanned = m_Scanned;
        }
        if (Scanned)
        {
            break;
        }
        Sleep(100);
    }

    CGuard Guard(m_CS);
    SectionFiles::const_iterator itr = m_SectionFiles.find(m_SectionIdent);
    if (itr != m_SectionFiles.end())
    {
        CPath CheatFile(itr->second);
        if (CheatFile.Exists())
        {
            m_CheatIniFile = std::make_unique<CIniFile>(CheatFile);
            m_CheatIniFile->SetCustomSort(CustomSortData);
        }
    }
}

void CCheatInfo::FlushChanges(void)
{
    if (m_CheatIniFile.get() != nullptr)
    {
        m_CheatIniFile->FlushChanges();
    }
}

bool CCheatInfo::Load(const char * PostFix, uint32_t Index, std::string & Value) const
{
    if (m_CheatIniFile.get() == nullptr)
    {
        return false;
    }
    stdstr_f Key("Cheat%d%s", Index, PostFix);
    return m_CheatIniFile->GetString(m_SectionIdent.c_str(), Key.c_str(), "", Value);
}

void CCheatInfo::Save(const char * PostFix, uint32_t Index, const char * Value)
{
    UseUserFile();
    if (m_CheatIniFile.get() == nullptr)
    {
        return;
    }
    if (strlen(Value) == 0)
    {
        if (strcmp(PostFix, "_O") == 0 || strcmp(PostFix, "_N") == 0)
        {
            Value = nullptr;
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    stdstr_f Key("Cheat%d%s", Index, PostFix);
    m_CheatIniFile->SaveString(m_SectionIdent.c_str(), Key.c_str(), Value);
}

void CCheatInfo::Delete(const char * PostFix, uint32_t Index)
{
    UseUserFile();
    stdstr_f Key("Cheat%d%s", Index, PostFix);
    m_CheatIniFile->SaveString(m_SectionIdent.c_str(), Key.c_str(), nullptr);
}

void CCheatInfo::ScanFileThread(void)
{
    SectionFiles Files;
    CPath CheatFile(g_Settings->LoadStringVal(SupportFile_CheatDir), "*.cht");
#ifdef _WIN32
    CheatFile.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (CheatFile.FindFirst())
    {
        do
        {
            CIniFile CheatIniFile(CheatFile);
            CIniFile::SectionList Sections;
            CheatIniFile.GetVectorOfSections(Sections);
            for (CIniFile::SectionList::const_iterator itr = Sections.begin(); itr != Sections.end(); itr++)
            {
                Files.insert(SectionFiles::value_type(itr->c_str(), CheatFile));
            }
        } while (CheatFile.FindNext());
    }

    CheatFile = CPath(g_Settings->LoadStringVal(SupportFile_UserCheatDir), "*.cht");
#ifdef _WIN32
    CheatFile.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (CheatFile.FindFirst())
    {
        do
        {
            CIniFile CheatIniFile(CheatFile);
            CIniFile::SectionList Sections;
            CheatIniFile.GetVectorOfSections(Sections);
            for (CIniFile::SectionList::const_iterator itr = Sections.begin(); itr != Sections.end(); itr++)
            {
                Files[itr->c_str()] = (const char *)CheatFile;
            }
        } while (CheatFile.FindNext());
    }

    {
        CGuard Guard(m_CS);
        m_SectionFiles = Files;
        m_Scanned = true;
    }
}

void CCheatInfo::CustomSortData(CIniFileBase::KeyValueVector & data)
{
    struct compareKeyValueItem
    {
        inline bool operator() (CIniFileBase::KeyValueItem & struct1, const CIniFileBase::KeyValueItem & struct2)
        {
            std::string a = *struct1.first;
            std::string b = *struct2.first;
            if (_stricmp(a.c_str(), "Name") == 0)
            {
                return true;
            }
            if (_stricmp(b.c_str(), "Name") == 0)
            {
                return false;
            }
            if (a.length() > 5 && _strnicmp(a.c_str(), "cheat", 5) == 0 &&
                b.length() > 5 && _strnicmp(b.c_str(), "cheat", 5) == 0)
            {
                int i1 = atoi(&(*struct1.first)[5]);
                int i2 = atoi(&(*struct2.first)[5]);
                if (i1 != i2)
                {
                    return i1 < i2;
                }
                char Buffer[40];
                int number_len = strlen(_itoa(i1, Buffer, 10));
                if (strlen(&a[5 + number_len]) == 0)
                {
                    return true;
                }
                if (strlen(&b[5 + number_len]) == 0)
                {
                    return false;
                }
                return _stricmp(&a[5 + number_len], &b[5 + number_len]) <= 0;
            }
            return _stricmp(a.c_str(), b.c_str()) <= 0;
        }
    };
    std::sort(data.begin(), data.end(), compareKeyValueItem());
}

void CCheatInfo::UseUserFile()
{
    if (m_UsingUserFile)
    {
        return;
    }

    std::string UserCheatDir = g_Settings->LoadStringVal(SupportFile_UserCheatDir);
    CPath FileName(UserCheatDir.c_str(), ".cht");

    CIniFile::strlist KeyList;
    if (m_CheatIniFile.get() != nullptr)
    {
        CPath CurrentIniFolder(m_CheatIniFile->GetFileName());
        CurrentIniFolder.SetNameExtension("");
        CPath UserCheatDirPath(UserCheatDir, "");

#ifdef _WIN32
        CurrentIniFolder.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
        UserCheatDirPath.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
        if (CurrentIniFolder == UserCheatDirPath)
        {
            m_UsingUserFile = true;
            return;
        }
        m_CheatIniFile->GetKeyList(m_SectionIdent.c_str(), KeyList);
        FileName.SetName(CPath(m_CheatIniFile->GetFileName()).GetName().c_str());
    }

    if (FileName.GetName().empty())
    {
        FileName.SetName(g_Settings->LoadStringVal(Rdb_GoodName).c_str());
    }
#ifdef _WIN32
    FileName.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
#endif
    if (!FileName.DirectoryExists())
    {
        FileName.DirectoryCreate();
    }

    std::unique_ptr<CIniFile> UserIniFile = std::make_unique<CIniFile>(FileName);
    UserIniFile->SetCustomSort(CustomSortData);
    for (CIniFile::strlist::const_iterator itr = KeyList.begin(); itr != KeyList.end(); itr++)
    {
        std::string Value = m_CheatIniFile->GetString(m_SectionIdent.c_str(), itr->c_str(), "");
        UserIniFile->SaveString(m_SectionIdent.c_str(), itr->c_str(), Value.c_str());
    }
    UserIniFile->FlushChanges();
    m_CheatIniFile.reset(UserIniFile.release());
    m_UsingUserFile = true;
}
