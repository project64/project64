#include "stdafx.h"
#include "SettingsType-Application.h"
#include "SettingsType-ApplicationIndex.h"


CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, DWORD DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, bool DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue ) :
	CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeApplication(Section,Name,DefaultSetting)
{
}

CSettingTypeApplicationIndex::~CSettingTypeApplicationIndex ( void )
{
}

bool CSettingTypeApplicationIndex::Load ( int Index, bool & Value ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	return CSettingTypeApplication::Load(0,Value);
}

bool CSettingTypeApplicationIndex::Load ( int Index, ULONG & Value ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	return CSettingTypeApplication::Load(0,Value);
}

bool CSettingTypeApplicationIndex::Load ( int Index, stdstr & Value ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	return CSettingTypeApplication::Load(0,Value);
}

//return the default values
void CSettingTypeApplicationIndex::LoadDefault ( int Index, bool & Value   ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::LoadDefault(0,Value);
}

void CSettingTypeApplicationIndex::LoadDefault ( int Index, ULONG & Value  ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::LoadDefault(0,Value);
}

void CSettingTypeApplicationIndex::LoadDefault ( int Index, stdstr & Value ) const
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::LoadDefault(0,Value);
}

//Update the settings
void CSettingTypeApplicationIndex::Save ( int Index, bool Value )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::Save(0,Value);
}

void CSettingTypeApplicationIndex::Save ( int Index, ULONG Value )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::Save(0,Value);
}

void CSettingTypeApplicationIndex::Save ( int Index, const stdstr & Value )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::Save(0,Value);
}

void CSettingTypeApplicationIndex::Save ( int Index, const char * Value )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::Save(0,Value);
}

void CSettingTypeApplicationIndex::Delete ( int Index )
{
	m_KeyNameIdex.Format("%s %d",m_KeyName.c_str(),Index);
	CSettingTypeApplication::Save(0,(const char *)NULL);
}
