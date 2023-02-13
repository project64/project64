#include "stdafx.h"

#include "SettingsType-RDB.h"
#include "SettingsType-RomDatabase.h"

CSettingTypeRDB::CSettingTypeRDB(const char * Name, SettingID DefaultSetting) :
    CSettingTypeRomDatabase(Name, DefaultSetting)
{
}

CSettingTypeRDB::CSettingTypeRDB(const char * Name, uint32_t DefaultValue) :
    CSettingTypeRomDatabase(Name, DefaultValue)
{
}

CSettingTypeRDB::~CSettingTypeRDB()
{
}

bool CSettingTypeRDB::Load(uint32_t Index, bool & Value) const
{
    uint32_t dwValue = 0;
    bool bRes = m_SettingsIniFile->GetNumber(m_SectionIdent->c_str(), m_KeyName.c_str(), Value, dwValue);
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

bool CSettingTypeRDB::Load(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDB::Load(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

// Return the default values
void CSettingTypeRDB::LoadDefault(uint32_t /*Index*/, bool & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else
        {
            g_Settings->LoadBool(m_DefaultSetting, Value);
        }
    }
}

void CSettingTypeRDB::LoadDefault(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDB::LoadDefault(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// Update the settings
void CSettingTypeRDB::Save(uint32_t /*Index*/, bool Value)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), Value ? "Yes" : "No");
}

void CSettingTypeRDB::Save(uint32_t /*Index*/, uint32_t Value)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), Value ? "Yes" : "No");
}

void CSettingTypeRDB::Save(uint32_t /*Index*/, const std::string & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDB::Save(uint32_t /*Index*/, const char * /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDB::Delete(uint32_t /*Index*/)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), nullptr);
}