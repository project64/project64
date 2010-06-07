#include "stdafx.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBSaveChip.h"

CSettingTypeRDBSaveChip::CSettingTypeRDBSaveChip(LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBSaveChip::CSettingTypeRDBSaveChip(LPCSTR Name, int DefaultValue ) :
	CSettingTypeRomDatabase(Name,DefaultValue)
{
}

	CSettingTypeRDBSaveChip::~CSettingTypeRDBSaveChip()
{
}

bool CSettingTypeRDBSaveChip::Load ( int Index, bool & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeRDBSaveChip::Load ( int Index, ULONG & Value ) const
{
	stdstr strValue;
	bool bRes = m_SettingsIniFile->GetString(m_SectionIdent.c_str(),m_KeyName.c_str(),m_DefaultStr,strValue);
	if (!bRes)
	{
		LoadDefault(Index,Value);
		return false;
	}
	LPCSTR String = strValue.c_str();

	if (_stricmp(String,"First Save Type") == 0)    { Value = SaveChip_Auto; } 
	else if (_stricmp(String,"4kbit Eeprom") == 0)  { Value = SaveChip_Eeprom_4K; } 
	else if (_stricmp(String,"16kbit Eeprom") == 0) { Value = SaveChip_Eeprom_16K; } 
	else if (_stricmp(String,"Sram") == 0)          { Value = SaveChip_Sram; } 
	else if (_stricmp(String,"FlashRam") == 0)      { Value = SaveChip_FlashRam; } 
	else if (_stricmp(String,"default") == 0)       
	{ 
		LoadDefault(Index,Value);
		return false;
	} else 	{ 
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
	
	return true;
}

bool CSettingTypeRDBSaveChip::Load ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

//return the default values
void CSettingTypeRDBSaveChip::LoadDefault ( int Index, bool & Value   ) const
{
	Notify().BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeRDBSaveChip::LoadDefault ( int Index, ULONG & Value  ) const
{
	if (m_DefaultSetting != Default_None)
	{
		if (m_DefaultSetting == Default_Constant)
		{
			Value = m_DefaultValue;
		} else {
			_Settings->LoadDword(m_DefaultSetting,Value);
		}
	}
}

void CSettingTypeRDBSaveChip::LoadDefault ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

//Update the settings
void CSettingTypeRDBSaveChip::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBSaveChip::Save ( int Index, ULONG Value )
{
	switch (Value)
	{
	case SaveChip_Auto:       m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),"First Save Type"); break;
	case SaveChip_Eeprom_4K:  m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),"4kbit Eeprom"); break;
	case SaveChip_Eeprom_16K: m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),"16kbit Eeprom"); break;
	case SaveChip_Sram:       m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),"Sram"); break;
	case SaveChip_FlashRam:   m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),"FlashRam"); break;
	default:
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
}

void CSettingTypeRDBSaveChip::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBSaveChip::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBSaveChip::Delete( int Index )
{
	m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),NULL);
}
