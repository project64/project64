#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-RomDatabase.h"

CIniFile * CSettingTypeRomDatabase::m_SettingsIniFile = NULL;
stdstr CSettingTypeRomDatabase::m_SectionIdent;

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, int DefaultValue ) :
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(DefaultValue),
	m_DefaultSetting(Default_Constant)
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, bool DefaultValue ) :
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(DefaultValue),
	m_DefaultSetting(Default_Constant)
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, LPCSTR DefaultValue ) :
	m_KeyName(Name),
	m_DefaultStr(DefaultValue),
	m_DefaultValue(0),
	m_DefaultSetting(Default_Constant)
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, SettingID DefaultSetting ) :
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(0),
	m_DefaultSetting(DefaultSetting)
{
}

CSettingTypeRomDatabase::~CSettingTypeRomDatabase()
{
}

void CSettingTypeRomDatabase::Initilize( void )
{
	m_SettingsIniFile = new CIniFile(_Settings->LoadString(SupportFile_RomDatabase).c_str());

	_Settings->RegisterChangeCB(Game_IniKey,NULL,GameChanged);
	
	m_SectionIdent = _Settings->LoadString(Game_IniKey);
}

void CSettingTypeRomDatabase::CleanUp( void )
{
	_Settings->UnregisterChangeCB(Game_IniKey,NULL,GameChanged);
	if (m_SettingsIniFile)
	{
		delete m_SettingsIniFile;
		m_SettingsIniFile = NULL;
	}
}

void CSettingTypeRomDatabase::GameChanged ( void * /*Data */ )
{
	m_SectionIdent = _Settings->LoadString(Game_IniKey);
}

bool CSettingTypeRomDatabase::Load ( int Index, bool & Value ) const
{
	DWORD temp_value = Value;
	bool bRes = Load(Index,temp_value);
	Value = temp_value != 0;
	return bRes;
}

bool CSettingTypeRomDatabase::Load ( int Index, ULONG & Value ) const
{
	bool bRes = m_SettingsIniFile->GetNumber(m_SectionIdent.c_str(),m_KeyName.c_str(),Value,Value);
	if (!bRes)
	{
		LoadDefault(Index,Value);
	}
	return bRes;
}

bool CSettingTypeRomDatabase::Load ( int Index, stdstr & Value ) const
{
	stdstr temp_value;
	bool bRes = m_SettingsIniFile->GetString(m_SectionIdent.c_str(),m_KeyName.c_str(),m_DefaultStr,temp_value);
	if (bRes)
	{
		Value = temp_value;
	}
	else 
	{
		LoadDefault(Index,Value);
	}
	return bRes;
}


//return the default values
void CSettingTypeRomDatabase::LoadDefault ( int Index, bool & Value   ) const
{
	if (m_DefaultSetting != Default_None)
	{
		if (m_DefaultSetting == Default_Constant)
		{
			Value = m_DefaultValue != 0;
		} else {
			_Settings->LoadBool(m_DefaultSetting,Value);
		}
	}
}

void CSettingTypeRomDatabase::LoadDefault ( int Index, ULONG & Value  ) const
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

void CSettingTypeRomDatabase::LoadDefault ( int Index, stdstr & Value ) const
{
	if (m_DefaultSetting != Default_None)
	{
		if (m_DefaultSetting == Default_Constant)
		{
			Value = m_DefaultStr;
		} else {
			_Settings->LoadString(m_DefaultSetting,Value);
		}
	}
}


//Update the settings
void CSettingTypeRomDatabase::Save ( int Index, bool Value )
{
	m_SettingsIniFile->SaveNumber(m_SectionIdent.c_str(),m_KeyName.c_str(),Value);
}

void CSettingTypeRomDatabase::Save ( int Index, ULONG Value )
{
	m_SettingsIniFile->SaveNumber(m_SectionIdent.c_str(),m_KeyName.c_str(),Value);
}

void CSettingTypeRomDatabase::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRomDatabase::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRomDatabase::Delete ( int Index )
{
	m_SettingsIniFile->SaveString(m_SectionIdent.c_str(),m_KeyName.c_str(),NULL);
}
