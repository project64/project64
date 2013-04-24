/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "SettingsType-RomDatabaseSetting.h"

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, int DefaultValue, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, bool DefaultValue, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, LPCSTR DefaultValue, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, SettingID DefaultSetting, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultSetting, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
}

CSettingTypeRomDatabaseSetting::~CSettingTypeRomDatabaseSetting()
{
}

bool CSettingTypeRomDatabaseSetting::Load ( int Index, bool & Value ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
/*	DWORD temp_value = Value;
	bool bRes = Load(Index,temp_value);
	Value = temp_value != 0;
	return bRes;*/
	return false;
}

bool CSettingTypeRomDatabaseSetting::Load ( int Index, ULONG & Value ) const
{
	bool bRes = m_SettingsIniFile->GetNumber(m_SectionIdent.c_str(),m_KeyName.c_str(),Value,Value);
	if (!bRes)
	{
		LoadDefault(Index,Value);
	}
	return bRes;
}

bool CSettingTypeRomDatabaseSetting::Load ( int Index, stdstr & Value ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*stdstr temp_value;
	bool bRes = m_SettingsIniFile->GetString(m_SectionIdent->c_str(),m_KeyName.c_str(),m_DefaultStr,temp_value);
	if (bRes)
	{
		Value = temp_value;
	}
	else 
	{
		LoadDefault(Index,Value);
	}
	return bRes;*/
	return false;
}

//return the default values
void CSettingTypeRomDatabaseSetting::LoadDefault ( int /*Index*/, bool & Value ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (m_DefaultSetting != Default_None)
	{
		if (m_DefaultSetting == Default_Constant)
		{
			Value = m_DefaultValue != 0;
		} else {
			g_Settings->LoadBool(m_DefaultSetting,Value);
		}
	}*/
}

void CSettingTypeRomDatabaseSetting::LoadDefault ( int /*Index*/, ULONG & Value  ) const
{
	if (m_DefaultSetting == Default_None) { return; }

	if (m_DefaultSetting == Default_Constant)
	{
		Value = m_DefaultValue;
	} else {
		g_Settings->LoadDword(m_DefaultSetting,Value);
	}
}

void CSettingTypeRomDatabaseSetting::LoadDefault ( int /*Index*/, stdstr & Value ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (m_DefaultSetting != Default_None)
	{
		if (m_DefaultSetting == Default_Constant)
		{
			Value = m_DefaultStr;
		} else {
			g_Settings->LoadString(m_DefaultSetting,Value);
		}
	}*/
}

//Update the settings
void CSettingTypeRomDatabaseSetting::Save ( int /*Index*/, bool Value )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (!g_Settings->LoadBool(Setting_RdbEditor))
	{
		return;
	}
	if (m_DeleteOnDefault)
	{	
		Notify().BreakPoint(__FILE__,__LINE__); 
	}
	m_SettingsIniFile->SaveNumber(m_SectionIdent->c_str(),m_KeyName.c_str(),Value);
	*/
}

void CSettingTypeRomDatabaseSetting::Save ( int Index, ULONG Value )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (!g_Settings->LoadBool(Setting_RdbEditor))
	{
		return;
	}
	if (m_DeleteOnDefault)
	{	
		ULONG defaultValue = 0;
		LoadDefault(Index,defaultValue);
		if (defaultValue == Value)
		{
			Delete(Index);
			return;
		}
	}
	m_SettingsIniFile->SaveNumber(m_SectionIdent->c_str(),m_KeyName.c_str(),Value);
	*/
}

void CSettingTypeRomDatabaseSetting::Save ( int /*Index*/, const stdstr & Value )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (!g_Settings->LoadBool(Setting_RdbEditor))
	{
		return;
	}
	m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),Value.c_str());
	*/
}

void CSettingTypeRomDatabaseSetting::Save ( int /*Index*/, const char * Value )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (!g_Settings->LoadBool(Setting_RdbEditor))
	{
		return; 
	}
	m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),Value);*/
}

void CSettingTypeRomDatabaseSetting::Delete ( int /*Index*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	/*if (!g_Settings->LoadBool(Setting_RdbEditor))
	{
		return; 
	}
	m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),NULL);*/
}
