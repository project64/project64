#include "stdafx.h"

#include "SettingsType-RDBUser.h"
#include <Common/path.h>
#include <algorithm>

CPath CSettingTypeRDBUser::m_IniFilePath;
CIniFile * CSettingTypeRDBUser::m_IniFile = nullptr;

CSettingTypeRDBUser::CSettingTypeRDBUser(const char * Section, const char * Name, uint32_t DefaultValue) :
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeRDBUser::CSettingTypeRDBUser(const char * Section, const char * Name, bool DefaultValue) :
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeRDBUser::CSettingTypeRDBUser(const char * Section, const char * Name, const char * DefaultValue) :
    m_DefaultStr(DefaultValue),
    m_DefaultValue(0),
    m_DefaultSetting(Default_Constant),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeRDBUser::CSettingTypeRDBUser(const char * Section, const char * Name, SettingID DefaultSetting) :
    m_DefaultStr(""),
    m_DefaultValue(0),
    m_DefaultSetting(DefaultSetting),
    m_Section(FixSectionName(Section)),
    m_KeyName(Name),
    m_KeyNameIdex(m_KeyName)
{
}

CSettingTypeRDBUser::~CSettingTypeRDBUser()
{
}

bool CSettingTypeRDBUser::IsSettingSet(void) const
{
    return m_IniFile != nullptr ? m_IniFile->EntryExists(SectionName(), m_KeyNameIdex.c_str()) : false;
}

void CSettingTypeRDBUser::Initialize(void)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");
    m_IniFilePath = CPath(g_Settings->LoadStringVal(SupportFile_SettingsDirectory).c_str(), "Project64.rdb.user");
    if (m_IniFilePath.Exists())
    {
        CreateIniFile();
    }
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeRDBUser::Flush()
{
    if (m_IniFile != nullptr)
    {
        m_IniFile->FlushChanges();
    }
}

void CSettingTypeRDBUser::ResetAll()
{
    if (m_IniFile == nullptr)
    {
        return;
    }
    CIniFile::SectionList sections;
    m_IniFile->GetVectorOfSections(sections);
    for (CIniFile::SectionList::const_iterator itr = sections.begin(); itr != sections.end(); itr++)
    {
        m_IniFile->DeleteSection(itr->c_str());
    }
}

void CSettingTypeRDBUser::CleanUp()
{
    if (m_IniFile)
    {
        m_IniFile->SetAutoFlush(true);
        delete m_IniFile;
        m_IniFile = nullptr;
    }
}

bool CSettingTypeRDBUser::Load(uint32_t Index, bool & Value) const
{
    bool bRes = false;

    uint32_t dwValue = 0;
    bRes = m_IniFile ? m_IniFile->GetNumber(SectionName(), m_KeyNameIdex.c_str(), Value, dwValue) : false;
    if (bRes)
    {
        Value = dwValue != 0;
    }

    if (!bRes && m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else
        {
            if (g_Settings->IndexBasedSetting(m_DefaultSetting))
            {
                g_Settings->LoadBoolIndex(m_DefaultSetting, Index, Value);
            }
            else
            {
                g_Settings->LoadBool(m_DefaultSetting, Value);
            }
        }
    }
    return bRes;
}

bool CSettingTypeRDBUser::Load(uint32_t /*Index*/, uint32_t & Value) const
{
    bool bRes = m_IniFile != nullptr ? m_IniFile->GetNumber(SectionName(), m_KeyNameIdex.c_str(), Value, Value) : false;
    if (!bRes && m_DefaultSetting != Default_None)
    {
        Value = m_DefaultSetting == Default_Constant ? m_DefaultValue : g_Settings->LoadDword(m_DefaultSetting);
    }
    return bRes;
}

const char * CSettingTypeRDBUser::SectionName(void) const
{
    return m_Section.c_str();
}

bool CSettingTypeRDBUser::Load(uint32_t Index, std::string & Value) const
{
    bool bRes = m_IniFile != nullptr ? m_IniFile->GetString(SectionName(), m_KeyNameIdex.c_str(), m_DefaultStr, Value) : false;
    if (!bRes)
    {
        CSettingTypeRDBUser::LoadDefault(Index, Value);
    }
    return bRes;
}

// Return the default values
void CSettingTypeRDBUser::LoadDefault(uint32_t Index, bool & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else if (g_Settings->IndexBasedSetting(m_DefaultSetting))
        {
            g_Settings->LoadBoolIndex(m_DefaultSetting, Index, Value);
        }
        else
        {
            g_Settings->LoadBool(m_DefaultSetting, Value);
        }
    }
}

void CSettingTypeRDBUser::LoadDefault(uint32_t /*Index*/, uint32_t & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        Value = m_DefaultSetting == Default_Constant ? m_DefaultValue : g_Settings->LoadDword(m_DefaultSetting);
    }
}

void CSettingTypeRDBUser::LoadDefault(uint32_t /*Index*/, std::string & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        Value = m_DefaultSetting == Default_Constant ? m_DefaultStr : g_Settings->LoadStringVal(m_DefaultSetting);
    }
}

// Update the settings
void CSettingTypeRDBUser::Save(uint32_t Index, bool Value)
{
    bool indexed = g_Settings->IndexBasedSetting(m_DefaultSetting);

    if (m_DefaultSetting != Default_None &&
        ((m_DefaultSetting == Default_Constant && m_DefaultValue == (uint32_t)Value) ||
         (m_DefaultSetting != Default_Constant && (indexed ? g_Settings->LoadBoolIndex(m_DefaultSetting, Index) : g_Settings->LoadBool(m_DefaultSetting)) == Value)))
    {
        if (m_IniFile != nullptr)
        {
            m_IniFile->SaveString(SectionName(), m_KeyNameIdex.c_str(), nullptr);
        }
    }
    else
    {
        CreateIniFile();
        m_IniFile->SaveNumber(SectionName(), m_KeyNameIdex.c_str(), Value);
    }
}

void CSettingTypeRDBUser::Save(uint32_t /*Index*/, uint32_t Value)
{
    if (m_DefaultSetting != Default_None &&
        ((m_DefaultSetting == Default_Constant && m_DefaultValue == Value) ||
         (m_DefaultSetting != Default_Constant && g_Settings->LoadDword(m_DefaultSetting) == Value)))
    {
        if (m_IniFile != nullptr)
        {
            m_IniFile->SaveString(SectionName(), m_KeyNameIdex.c_str(), nullptr);
        }
    }
    else
    {
        CreateIniFile();
        m_IniFile->SaveNumber(SectionName(), m_KeyNameIdex.c_str(), Value);
    }
}

void CSettingTypeRDBUser::Save(uint32_t Index, const std::string & Value)
{
    Save(Index, Value.c_str());
}

void CSettingTypeRDBUser::Save(uint32_t /*Index*/, const char * Value)
{
    if (m_DefaultSetting != Default_None && Value != nullptr &&
        ((m_DefaultSetting == Default_Constant && strcmp(m_DefaultStr, Value) == 0) ||
         (m_DefaultSetting != Default_Constant && strcmp(g_Settings->LoadStringVal(m_DefaultSetting).c_str(), Value) == 0)))
    {
        if (m_IniFile != nullptr)
        {
            m_IniFile->SaveString(SectionName(), m_KeyNameIdex.c_str(), nullptr);
        }
    }
    else
    {
        CreateIniFile();
        m_IniFile->SaveString(SectionName(), m_KeyNameIdex.c_str(), Value);
    }
}

std::string CSettingTypeRDBUser::FixSectionName(const char * Section)
{
    stdstr SectionName(Section);

    if (SectionName.empty())
    {
        SectionName = "default";
    }
    SectionName.Replace("\\", "-");
    return SectionName;
}

void CSettingTypeRDBUser::CreateIniFile(void)
{
    if (m_IniFile != nullptr)
    {
        return;
    }
    m_IniFile = new CIniFile(m_IniFilePath);
    m_IniFile->SetCustomSort(CustomSortData);
}

struct compareKeyValueItem
{
    inline bool operator()(CIniFileBase::KeyValueItem & struct1, const CIniFileBase::KeyValueItem & struct2)
    {
        std::string a = *struct1.first;
        std::string b = *struct2.first;
        if (_stricmp(a.c_str(), "Good Name") == 0)
        {
            return true;
        }
        if (_stricmp(b.c_str(), "Good Name") == 0)
        {
            return false;
        }
        return _stricmp(a.c_str(), b.c_str()) <= 0;
    }
};

void CSettingTypeRDBUser::CustomSortData(CIniFileBase::KeyValueVector & data)
{
    std::sort(data.begin(), data.end(), compareKeyValueItem());
}

void CSettingTypeRDBUser::Delete(uint32_t /*Index*/)
{
    if (m_IniFile != nullptr)
    {
        m_IniFile->SaveString(SectionName(), m_KeyNameIdex.c_str(), nullptr);
    }
}