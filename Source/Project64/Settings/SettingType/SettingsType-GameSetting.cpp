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
#include "SettingsType-Application.h"
#include "SettingsType-GameSetting.h"

bool     CSettingTypeGame::m_RdbEditor = false;
bool     CSettingTypeGame::m_EraseDefaults = true;
stdstr * CSettingTypeGame::m_SectionIdent = NULL;

CSettingTypeGame::CSettingTypeGame(LPCSTR Name, LPCSTR DefaultValue )	:
	CSettingTypeApplication("",Name,DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(LPCSTR Name, DWORD DefaultValue ) :
	CSettingTypeApplication("",Name,DefaultValue)
{
}

CSettingTypeGame::CSettingTypeGame(LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeApplication("",Name,DefaultSetting)
{
}

CSettingTypeGame::~CSettingTypeGame()
{
}

void CSettingTypeGame::Initialize ( void )
{
	UpdateSettings(NULL);
	g_Settings->RegisterChangeCB(Game_IniKey,NULL,UpdateSettings);
}

void CSettingTypeGame::CleanUp   ( void )
{
	g_Settings->UnregisterChangeCB(Game_IniKey,NULL,UpdateSettings);
	if (m_SectionIdent)
	{
		delete m_SectionIdent;
		m_SectionIdent = NULL;
	}
}

LPCSTR CSettingTypeGame::SectionName ( void ) const
{
	return m_SectionIdent ? m_SectionIdent->c_str() : "";
}

void CSettingTypeGame::UpdateSettings ( void * /*Data */ )
{
	m_RdbEditor     = g_Settings->LoadBool(Setting_RdbEditor);
	m_EraseDefaults = g_Settings->LoadBool(Setting_EraseGameDefaults);
	stdstr SectionIdent = g_Settings->LoadString(Game_IniKey);

	if (m_SectionIdent == NULL)
	{
		m_SectionIdent = new stdstr;
	}
	if (SectionIdent != *m_SectionIdent)
	{
		*m_SectionIdent = SectionIdent;
		g_Settings->SettingTypeChanged(SettingType_GameSetting);
		g_Settings->SettingTypeChanged(SettingType_RomDatabase);
	}

}

bool CSettingTypeGame::Load ( int Index, bool & Value ) const
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			return g_Settings->LoadBoolIndex(m_DefaultSetting,Index,Value);
		} else {
			return g_Settings->LoadBool(m_DefaultSetting,Value);
		}
	}
	return CSettingTypeApplication::Load(Index,Value);
}

bool CSettingTypeGame::Load ( int Index, ULONG & Value ) const
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			return g_Settings->LoadDwordIndex(m_DefaultSetting,Index,Value);
		} else {
			return g_Settings->LoadDword(m_DefaultSetting,Value);
		}
	}
	return CSettingTypeApplication::Load(Index,Value);
}

bool CSettingTypeGame::Load ( int Index,  stdstr & Value ) const
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			return g_Settings->LoadStringIndex(m_DefaultSetting,Index,Value);
		} else {
			return g_Settings->LoadString(m_DefaultSetting,Value);
		}
	}	
	return CSettingTypeApplication::Load(Index,Value);
}

//return the default values
void CSettingTypeGame::LoadDefault ( int Index, bool & Value   ) const
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->LoadDefaultBoolIndex(m_DefaultSetting,Index,Value);
		} else {
			g_Settings->LoadDefaultBool(m_DefaultSetting,Value);
		}
	} else {
		CSettingTypeApplication::LoadDefault(Index,Value);
	}
}

void CSettingTypeGame::LoadDefault ( int Index, ULONG & Value  ) const
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->LoadDefaultDwordIndex(m_DefaultSetting,Index,Value);
		} else {
			g_Settings->LoadDefaultDword(m_DefaultSetting,Value);
		}
	} else {
		CSettingTypeApplication::LoadDefault(Index,Value);
	}
}

void CSettingTypeGame::LoadDefault ( int Index, stdstr & Value ) const
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->LoadDefaultStringIndex(m_DefaultSetting,Index,Value);
		} else {
			g_Settings->LoadDefaultString(m_DefaultSetting,Value);
		}
	} else {
		CSettingTypeApplication::LoadDefault(Index,Value);
	}
}

//Update the settings
void CSettingTypeGame::Save ( int Index, bool Value )
{
	if (m_EraseDefaults)
	{
		bool bDefault;
		LoadDefault(Index,bDefault);
		if (bDefault == Value)
		{
			Delete(Index);
			return;
		}
	}
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->SaveBoolIndex(m_DefaultSetting,Index,Value);
		} else {
			 g_Settings->SaveBool(m_DefaultSetting,Value);
		}
	} else {
		CSettingTypeApplication::Save(Index,Value);
	}
}

void CSettingTypeGame::Save ( int Index, ULONG Value )
{
	if (m_EraseDefaults)
	{
		ULONG ulDefault;
		CSettingTypeGame::LoadDefault(Index,ulDefault);
		if (ulDefault == Value)
		{
			Delete(Index);
			return;
		}
	}
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->SaveDwordIndex(m_DefaultSetting,Index,Value);
		} else {
			 g_Settings->SaveDword(m_DefaultSetting,Value);
		}
	} else {
		CSettingTypeApplication::Save(Index,Value);
	}
}

void CSettingTypeGame::Save ( int Index, const stdstr & Value )
{
	Save(Index,Value.c_str()); 
}

void CSettingTypeGame::Save ( int Index, const char * Value )
{
	if (m_EraseDefaults && m_DefaultSetting != Rdb_GoodName)
	{
		stdstr szDefault;
		CSettingTypeGame::LoadDefault(Index,szDefault);
		if (_stricmp(szDefault.c_str(),Value) == 0)
		{
			Delete(Index);
			return;
		}
	}
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->SaveStringIndex(m_DefaultSetting,Index,Value);
		} else {
			 g_Settings->SaveString(m_DefaultSetting,Value);
		}
	} else {
		CSettingTypeApplication::Save(Index,Value);
	}
}

void CSettingTypeGame::Delete ( int Index )
{
	if (m_RdbEditor && g_Settings->GetSettingType(m_DefaultSetting) == SettingType_RomDatabase)
	{
		if (g_Settings->IndexBasedSetting(m_DefaultSetting))
		{
			g_Settings->DeleteSettingIndex(m_DefaultSetting,Index);
		} else {
			g_Settings->DeleteSetting(m_DefaultSetting);
		}
	} else {
		CSettingTypeApplication::Delete(Index);
	}
}
