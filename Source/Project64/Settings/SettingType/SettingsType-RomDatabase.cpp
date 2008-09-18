#include "..\..\Settings.h"
#include "..\..\User Interface.h"
#include "SettingsType-RomDatabase.h"

CIniFile * CSettingTypeRomDatabase::m_SettingsIniFile = NULL;
stdstr CSettingTypeRomDatabase::m_SectionIdent;

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, int DefaultValue ) :
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(DefaultValue),
	m_DefaultSetting(No_Default)
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, bool DefaultValue ) :
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(DefaultValue),
	m_DefaultSetting(No_Default)
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, LPCSTR DefaultValue ) :
	m_KeyName(Name),
	m_DefaultStr(DefaultValue),
	m_DefaultValue(0),
	m_DefaultSetting(No_Default)
{
}

CSettingTypeRomDatabase::CSettingTypeRomDatabase(LPCSTR Name, SettingID DefaultSetting ) :
	m_KeyName(Name),
	m_DefaultStr(""),
	m_DefaultValue(0),
	m_DefaultSetting(DefaultSetting)
{
}


void CSettingTypeRomDatabase::Initilize( void )
{
	m_SettingsIniFile = new CIniFile(_Settings->LoadString(RomDatabaseFile).c_str());

	_Settings->RegisterChangeCB(ROM_IniKey,NULL,GameChanged);
	
	m_SectionIdent = _Settings->LoadString(ROM_IniKey);
}

void CSettingTypeRomDatabase::CleanUp( void )
{
	if (m_SettingsIniFile)
	{
		delete m_SettingsIniFile;
		m_SettingsIniFile = NULL;
	}
}

void CSettingTypeRomDatabase::GameChanged ( void * /*Data */ )
{
	m_SectionIdent = _Settings->LoadString(ROM_IniKey);
}

bool CSettingTypeRomDatabase::Load ( int Index, bool & Value ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeRomDatabase::Load ( int Index, ULONG & Value ) const
{
	if (m_SectionIdent.empty())
	{
		return false;
	}
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeRomDatabase::Load ( int Index, stdstr & Value ) const
{
	bool bRes = m_SettingsIniFile->GetString(m_SectionIdent.c_str(),m_KeyName,m_DefaultStr,Value);
	if (!bRes && m_DefaultSetting != No_Default)
	{
		_Settings->LoadString(m_DefaultSetting,Value);
	}
	return bRes;
}


//Update the settings
void CSettingTypeRomDatabase::Save ( int Index, bool Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRomDatabase::Save ( int Index, ULONG Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRomDatabase::Save ( int Index, const stdstr & Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeRomDatabase::Save ( int Index, const char * Value )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
