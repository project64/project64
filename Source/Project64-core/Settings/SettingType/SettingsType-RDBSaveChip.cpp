#include "stdafx.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBSaveChip.h"
#include <Project64-core/N64System/N64Types.h>

CSettingTypeRDBSaveChip::CSettingTypeRDBSaveChip(const char * Name, SettingID DefaultSetting ) :
    CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBSaveChip::CSettingTypeRDBSaveChip(const char * Name, uint32_t DefaultValue ) :
    CSettingTypeRomDatabase(Name,DefaultValue)
{
}

CSettingTypeRDBSaveChip::~CSettingTypeRDBSaveChip()
{
}

bool CSettingTypeRDBSaveChip::Load (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDBSaveChip::Load (uint32_t Index, uint32_t & Value ) const
{
    stdstr strValue;
    bool bRes = m_SettingsIniFile->GetString(m_SectionIdent->c_str(),m_KeyName.c_str(),m_DefaultStr,strValue);
    if (!bRes)
    {
        LoadDefault(Index,Value);
        return false;
    }
    const char * String = strValue.c_str();

    if (_stricmp(String,"First Save Type") == 0)    { Value = (uint32_t)SaveChip_Auto; }
    else if (_stricmp(String,"4kbit Eeprom") == 0)  { Value = SaveChip_Eeprom_4K; }
    else if (_stricmp(String,"16kbit Eeprom") == 0) { Value = SaveChip_Eeprom_16K; }
    else if (_stricmp(String,"Sram") == 0)          { Value = SaveChip_Sram; }
    else if (_stricmp(String,"FlashRam") == 0)      { Value = SaveChip_FlashRam; }
    else if (_stricmp(String,"default") == 0)
    {
        LoadDefault(Index,Value);
        return false;
    } else 	{
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    return true;
}

bool CSettingTypeRDBSaveChip::Load (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//return the default values
void CSettingTypeRDBSaveChip::LoadDefault (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBSaveChip::LoadDefault (uint32_t /*Index*/, uint32_t & Value  ) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue;
        } else {
            g_Settings->LoadDword(m_DefaultSetting,Value);
        }
    }
}

void CSettingTypeRDBSaveChip::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

//Update the settings
void CSettingTypeRDBSaveChip::Save (uint32_t /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBSaveChip::Save (uint32_t /*Index*/, uint32_t Value )
{
    switch ((SAVE_CHIP_TYPE)Value)
    {
    case SaveChip_Auto: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),"First Save Type"); break;
    case SaveChip_Eeprom_4K: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),"4kbit Eeprom"); break;
    case SaveChip_Eeprom_16K: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),"16kbit Eeprom"); break;
    case SaveChip_Sram: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),"Sram"); break;
    case SaveChip_FlashRam: m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),"FlashRam"); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CSettingTypeRDBSaveChip::Save (uint32_t /*Index*/, const std::string & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBSaveChip::Save (uint32_t /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBSaveChip::Delete(uint32_t /*Index*/ )
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),nullptr);
}
