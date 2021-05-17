#include "stdafx.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBYesNo.h"

CSettingTypeRDBYesNo::CSettingTypeRDBYesNo(const char * Name, SettingID DefaultSetting) :
CSettingTypeRomDatabase(Name, DefaultSetting)
{
}

CSettingTypeRDBYesNo::CSettingTypeRDBYesNo(const char * Name, uint32_t DefaultValue) :
CSettingTypeRomDatabase(Name, DefaultValue)
{
}

CSettingTypeRDBYesNo::~CSettingTypeRDBYesNo()
{
}

bool CSettingTypeRDBYesNo::Load(uint32_t Index, bool & Value) const
{
    stdstr strValue;
    bool bRes = m_SettingsIniFile->GetString(m_SectionIdent->c_str(), m_KeyName.c_str(), m_DefaultStr, strValue);
    if (!bRes)
    {
        LoadDefault(Index, Value);
        return false;
    }
    const char * String = strValue.c_str();

    if (_stricmp(String, "Yes") == 0)
    {
        Value = true;
    }
    else if (_stricmp(String, "No") == 0)
    {
        Value = false;
    }
    else if (_stricmp(String, "Default") == 0)
    {
        LoadDefault(Index, Value);
        return false;
    }
    else
    {
        WriteTrace(TraceSettings, TraceError, "Invalid yes/no setting value (Section: %s Key: %s Value: %s)", m_SectionIdent->c_str(), String, m_KeyName.c_str(), strValue.c_str());
        LoadDefault(Index, Value);
        return false;
    }

    return true;
}

bool CSettingTypeRDBYesNo::Load(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDBYesNo::Load(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

// Return the default values
void CSettingTypeRDBYesNo::LoadDefault(uint32_t /*Index*/, bool & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else {
            g_Settings->LoadBool(m_DefaultSetting, Value);
        }
    }
}

void CSettingTypeRDBYesNo::LoadDefault(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBYesNo::LoadDefault(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

// Update the settings
void CSettingTypeRDBYesNo::Save(uint32_t /*Index*/, bool Value)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), Value ? "Yes" : "No");
}

void CSettingTypeRDBYesNo::Save(uint32_t /*Index*/, uint32_t Value)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), Value ? "Yes" : "No");
}

void CSettingTypeRDBYesNo::Save(uint32_t /*Index*/, const std::string & /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBYesNo::Save(uint32_t /*Index*/, const char * /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBYesNo::Delete(uint32_t /*Index*/)
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(), m_KeyName.c_str(), nullptr);
}
