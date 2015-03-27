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
#include "SettingsType-Cheats.h"

CIniFile * CSettingTypeCheats::m_CheatIniFile = NULL;
stdstr   * CSettingTypeCheats::m_SectionIdent = NULL;

CSettingTypeCheats::CSettingTypeCheats(LPCSTR PostFix ) :
	m_PostFix(PostFix)
{
}

CSettingTypeCheats::~CSettingTypeCheats ( void )
{
}

void CSettingTypeCheats::Initilize ( void )
{
	m_CheatIniFile = new CIniFile(g_Settings->LoadString(SupportFile_Cheats).c_str());
	m_CheatIniFile->SetAutoFlush(false);
	g_Settings->RegisterChangeCB(Game_IniKey,NULL,GameChanged);
	m_SectionIdent = new stdstr(g_Settings->LoadString(Game_IniKey));
	GameChanged(NULL);
}

void CSettingTypeCheats::CleanUp   ( void )
{
	if (m_CheatIniFile)
	{
		m_CheatIniFile->SetAutoFlush(true);
		delete m_CheatIniFile;
		m_CheatIniFile = NULL;
	}
	if (m_SectionIdent)
	{
		delete m_SectionIdent;
		m_SectionIdent = NULL;
	}
}

void CSettingTypeCheats::FlushChanges( void )
{
	if (m_CheatIniFile)
	{
		m_CheatIniFile->FlushChanges();
	}
}

void CSettingTypeCheats::GameChanged ( void * /*Data */ )
{
	*m_SectionIdent = g_Settings->LoadString(Game_IniKey);
}


/*stdstr CSettingTypeCheats::FixName ( LPCSTR Section, LPCSTR Name )
{
}

LPCSTR CSettingTypeCheats::SectionName ( void ) const
{
	return "";
}

void CSettingTypeCheats::UpdateSettings ( void *  )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
}*/

bool CSettingTypeCheats::Load ( int /*Index*/, bool & /*Value*/ ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	return false;
}

bool CSettingTypeCheats::Load ( int /*Index*/, ULONG & /*Value*/ ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
	return false;
}

bool CSettingTypeCheats::Load ( int Index,  stdstr & Value ) const
{
	if (m_CheatIniFile == NULL) 
	{ 
		return false; 
	}
	stdstr_f Key("Cheat%d%s",Index,m_PostFix);
	return m_CheatIniFile->GetString(m_SectionIdent->c_str(),Key.c_str(),"",Value);
}

//return the default values
void CSettingTypeCheats::LoadDefault ( int /*Index*/, bool & /*Value*/ ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeCheats::LoadDefault ( int /*Index*/, ULONG & /*Value*/ ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeCheats::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
}

//Update the settings
void CSettingTypeCheats::Save ( int /*Index*/, bool /*Value*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeCheats::Save ( int /*Index*/, ULONG /*Value*/ )
{
	g_Notify->BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeCheats::Save ( int Index, const stdstr & Value )
{
	if (m_CheatIniFile == NULL) {  return;  }
	
	stdstr_f Key("Cheat%d%s",Index,m_PostFix);
	m_CheatIniFile->SaveString(m_SectionIdent->c_str(),Key.c_str(),Value.c_str());
}

void CSettingTypeCheats::Save ( int Index, const char * Value )
{
	if (m_CheatIniFile == NULL) {  return;  }
	
	stdstr_f Key("Cheat%d%s",Index,m_PostFix);
	m_CheatIniFile->SaveString(m_SectionIdent->c_str(),Key.c_str(),Value);
}

void CSettingTypeCheats::Delete ( int Index )
{
	stdstr_f Key("Cheat%d%s",Index,m_PostFix);
	m_CheatIniFile->SaveString(m_SectionIdent->c_str(),Key.c_str(),NULL);
}
