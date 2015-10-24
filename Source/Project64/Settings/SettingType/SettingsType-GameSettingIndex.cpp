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
#include "SettingsType-GameSettingIndex.h"


CSettingTypeGameIndex::CSettingTypeGameIndex(LPCSTR PreIndex, LPCSTR PostIndex, SettingID DefaultSetting ) :
	CSettingTypeGame("", DefaultSetting),
	m_PreIndex(PreIndex),
	m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::CSettingTypeGameIndex(LPCSTR PreIndex, LPCSTR PostIndex, DWORD DefaultValue ) :
	CSettingTypeGame("", DefaultValue),
	m_PreIndex(PreIndex),
	m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::CSettingTypeGameIndex(LPCSTR PreIndex, LPCSTR PostIndex, LPCSTR DefaultValue ) :
	CSettingTypeGame("", DefaultValue),
	m_PreIndex(PreIndex),
	m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::~CSettingTypeGameIndex()
{
}

bool CSettingTypeGameIndex::Load ( int Index, bool & Value ) const
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	return CSettingTypeGame::Load(Index,Value);
}

bool CSettingTypeGameIndex::Load ( int /*Index*/, ULONG & /*Value*/ ) const
{
	Notify().BreakPoint(__FILEW__,__LINE__);
	return false;
}

bool CSettingTypeGameIndex::Load ( int Index,  stdstr & Value ) const
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	return CSettingTypeGame::Load(0,Value);
}

//return the default values
void CSettingTypeGameIndex::LoadDefault ( int Index, bool & Value   ) const
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	CSettingTypeGame::LoadDefault(0,Value);
}

void CSettingTypeGameIndex::LoadDefault ( int /*Index*/, ULONG & /*Value*/  ) const
{
	Notify().BreakPoint(__FILEW__,__LINE__);
}

void CSettingTypeGameIndex::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
	Notify().BreakPoint(__FILEW__,__LINE__);
}

//Update the settings
void CSettingTypeGameIndex::Save ( int Index, bool Value )
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	CSettingTypeGame::Save(Index,Value);
}

void CSettingTypeGameIndex::Save ( int Index, ULONG Value )
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	CSettingTypeGame::Save(0,Value);
}

void CSettingTypeGameIndex::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
	Notify().BreakPoint(__FILEW__,__LINE__);
}

void CSettingTypeGameIndex::Save ( int Index, const char * Value )
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	CSettingTypeGame::Save(0,Value);
}

void CSettingTypeGameIndex::Delete ( int Index )
{
	m_KeyNameIdex.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
	CSettingTypeGame::Delete(0);
}
