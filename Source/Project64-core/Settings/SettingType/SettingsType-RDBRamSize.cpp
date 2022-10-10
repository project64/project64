#include "stdafx.h"

#include "SettingsType-RDBRamSize.h"
#include "SettingsType-RomDatabase.h"

// == 8 ? 0x800000 : 0x400000

CSettingTypeRDBRDRamSize::CSettingTypeRDBRDRamSize(const char * Name, SettingID DefaultSetting) :
    CSettingTypeRomDatabase(Name, DefaultSetting)
{
}

CSettingTypeRDBRDRamSize::CSettingTypeRDBRDRamSize(const char * Name, uint32_t DefaultValue) :
    CSettingTypeRomDatabase(Name, DefaultValue)
{
}

CSettingTypeRDBRDRamSize::~CSettingTypeRDBRDRamSize()
{
}

bool CSettingTypeRDBRDRamSize::Load(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDBRDRamSize::Load(uint32_t Index, uint32_t & Value) const
{
    uint32_t ulValue;
    bool bRes = m_SettingsIniFile->GetNumber(m_SectionIdent->c_str(), m_KeyName.c_str(), m_DefaultValue, ulValue);
    if (!bRes)
    {
        LoadDefault(Index, ulValue);
    }
    Value = 0x400000;
    if (ulValue == 8 || ulValue == 0x800000)
    {
        Value = 0x800000;
    }
    return bRes;
}

bool CSettingTypeRDBRDRamSize::Load(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

// Return the default values
void CSettingTypeRDBRDRamSize::LoadDefault(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::LoadDefault(uint32_t /*Index*/, uint32_t & Value) const
{
    Value = m_DefaultSetting == Default_Constant ? m_DefaultValue : g_Settings->LoadDword(m_DefaultSetting);
}

void CSettingTypeRDBRDRamSize::LoadDefault(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// Update the settings
void CSettingTypeRDBRDRamSize::Save(uint32_t /*Index*/, bool /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::Save(uint32_t /*Index*/, uint32_t Value)
{
    m_SettingsIniFile->SaveNumber(m_SectionIdent->c_str(), m_KeyName.c_str(), Value == 0x800000 ? 8 : 4);
}

void CSettingTypeRDBRDRamSize::Save(uint32_t /*Index*/, const std::string & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::Save(uint32_t /*Index*/, const char * /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::Delete(uint32_t /*Index*/)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), nullptr);
}
