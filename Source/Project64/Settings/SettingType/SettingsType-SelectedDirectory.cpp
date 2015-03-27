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
#include "SettingsType-SelectedDirectory.h"


CSettingTypeSelectedDirectory::CSettingTypeSelectedDirectory(LPCSTR Name, SettingID InitialDir, SettingID SelectedDir, SettingID UseSelected ) :
	m_Name(Name),
	m_InitialDir(InitialDir),
	m_SelectedDir(SelectedDir),
	m_UseSelected(UseSelected)
{
}


CSettingTypeSelectedDirectory::~CSettingTypeSelectedDirectory()
{
}

bool CSettingTypeSelectedDirectory::Load ( int /*Index*/, bool & /*Value*/ ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeSelectedDirectory::Load ( int /*Index*/, ULONG & /*Value*/ ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
	return false;
}

bool CSettingTypeSelectedDirectory::Load ( int /*Index*/, stdstr & Value ) const
{
	SettingID DirSettingId = g_Settings->LoadBool(m_UseSelected) ? m_SelectedDir : m_InitialDir; 
	return g_Settings->LoadString(DirSettingId, Value);
}

//return the default values
void CSettingTypeSelectedDirectory::LoadDefault ( int /*Index*/, bool & /*Value*/   ) const
{
	Notify().BreakPoint(__FILE__,__LINE__);
}

void CSettingTypeSelectedDirectory::LoadDefault ( int /*Index*/, ULONG & /*Value*/  ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeSelectedDirectory::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

//Update the settings
void CSettingTypeSelectedDirectory::Save ( int /*Index*/, bool /*Value*/ )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeSelectedDirectory::Save ( int /*Index*/, ULONG /*Value*/ )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeSelectedDirectory::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}

void CSettingTypeSelectedDirectory::Save ( int /*Index*/, const char * Value )
{
	g_Settings->SaveBool(m_UseSelected,true);
	g_Settings->SaveString(m_SelectedDir,Value);
}

void CSettingTypeSelectedDirectory::Delete( int /*Index*/ )
{
	Notify().BreakPoint(__FILE__,__LINE__); 
}
