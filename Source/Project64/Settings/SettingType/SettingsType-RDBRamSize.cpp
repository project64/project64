#include "stdafx.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RDBRamSize.h"

// == 8 ? 0x800000 : 0x400000

CSettingTypeRDBRDRamSize::CSettingTypeRDBRDRamSize(LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBRDRamSize::CSettingTypeRDBRDRamSize(LPCSTR Name, int DefaultValue ) :
	CSettingTypeRomDatabase(Name,DefaultValue)
{
}

CSettingTypeRDBRDRamSize::~CSettingTypeRDBRDRamSize()
{
}

bool CSettingTypeRDBRDRamSize::Load ( int Index, bool & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeRDBRDRamSize::Load ( int Index, ULONG & Value ) const
{
	ULONG ulValue;
	bool bRes = m_SettingsIniFile->GetNumber(m_SectionIdent->c_str(),m_KeyName.c_str(),m_DefaultValue,ulValue);
	if (!bRes)
	{
		LoadDefault(Index,ulValue);
	}
	Value = 0x400000;
	if (ulValue == 8)
	{
		Value = 0x800000;
	}
	return bRes;
}

bool CSettingTypeRDBRDRamSize::Load ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

//return the default values
void CSettingTypeRDBRDRamSize::LoadDefault ( int Index, bool & Value   ) const
{
	Notify().BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeRDBRDRamSize::LoadDefault ( int Index, ULONG & Value  ) const
{
	Value = m_DefaultValue; 
}

void CSettingTypeRDBRDRamSize::LoadDefault ( int Index, stdstr & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

//Update the settings
void CSettingTypeRDBRDRamSize::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBRDRamSize::Save ( int Index, ULONG Value )
{
	m_SettingsIniFile->SaveNumber(m_SectionIdent->c_str(),m_KeyName.c_str(),Value == 0x800000 ? 8 : 4);
}

void CSettingTypeRDBRDRamSize::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBRDRamSize::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRDBRDRamSize::Delete( int Index )
{
	m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),NULL);
}
