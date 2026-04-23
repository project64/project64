#include "stdafx.h"

#include "SettingsType-RDBLinking.h"
#include "SettingsType-RomDatabase.h"
#include <Common/Platform.h>
#include <Project64-core/N64System/N64Types.h>

CSettingTypeRDBLinking::CSettingTypeRDBLinking(const char * Name, uint32_t DefaultValue) :
    CSettingTypeRomDatabase(Name, DefaultValue)
{
}

CSettingTypeRDBLinking::~CSettingTypeRDBLinking()
{
}

bool CSettingTypeRDBLinking::Load(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDBLinking::Load(uint32_t Index, uint32_t & Value) const
{
    stdstr strValue;
    bool bRes = m_SettingsIniFile->GetString(m_SectionIdent->c_str(), m_KeyName.c_str(), m_DefaultStr, strValue);
    if (!bRes)
    {
        LoadDefault(Index, Value);
        return false;
    }
    const char * String = strValue.c_str();
    if (_stricmp(String, "on") == 0)
    {
        Value = BlockLinking_Eager;
    }
    else if (_stricmp(String, "off") == 0)
    {
        Value = BlockLinking_None;
    }
    else if (_stricmp(String, "none") == 0)
    {
        Value = BlockLinking_None;
    }
    else if (_stricmp(String, "eager") == 0)
    {
        Value = BlockLinking_Eager;
    }
    else if (_stricmp(String, "adaptive") == 0)
    {
        Value = BlockLinking_Adaptive;
    }
    else if (_stricmp(String, "Global") == 0 || _stricmp(String, "default"))
    {
        LoadDefault(Index, Value);
        return false;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    return true;
}

bool CSettingTypeRDBLinking::Load(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CSettingTypeRDBLinking::LoadDefault(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBLinking::LoadDefault(uint32_t /*Index*/, uint32_t & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        Value = m_DefaultSetting == Default_Constant ? m_DefaultValue : g_Settings->LoadDword(m_DefaultSetting);
    }
}

void CSettingTypeRDBLinking::LoadDefault(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBLinking::Save(uint32_t /*Index*/, bool /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBLinking::Save(uint32_t /*Index*/, uint32_t Value)
{
    switch (Value)
    {
    case BlockLinking_None: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), "off"); break;
    case BlockLinking_Eager: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), "eager"); break;
    case BlockLinking_Adaptive: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), "Adaptive"); break;
    default: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), "off"); break;
    }
}

void CSettingTypeRDBLinking::Save(uint32_t /*Index*/, const std::string & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBLinking::Save(uint32_t /*Index*/, const char * /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBLinking::Delete(uint32_t /*Index*/)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), nullptr);
}
