/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include "SettingsType-RomDatabase.h"
#include "SettingsType-RomDatabaseIndex.h"

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, const char * DefaultValue ) :
    CSettingTypeRomDatabase("",DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, bool DefaultValue ) :
    CSettingTypeRomDatabase("",DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, int DefaultValue ) :
    CSettingTypeRomDatabase("",DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::CSettingTypeRomDatabaseIndex(const char * PreIndex, const char * PostIndex, SettingID DefaultSetting ) :
    CSettingTypeRomDatabase("",DefaultSetting),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeRomDatabaseIndex::~CSettingTypeRomDatabaseIndex()
{
}

bool CSettingTypeRomDatabaseIndex::Load ( int /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRomDatabaseIndex::Load ( int /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRomDatabaseIndex::Load ( int Index, stdstr & Value ) const
{
    m_KeyName.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    return CSettingTypeRomDatabase::Load(0,Value);
}

void CSettingTypeRomDatabaseIndex::LoadDefault ( int Index, bool & Value   ) const
{
    m_KeyName.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeRomDatabase::LoadDefault(0,Value);
}

void CSettingTypeRomDatabaseIndex::LoadDefault ( int Index, uint32_t & Value  ) const
{
    m_KeyName.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeRomDatabase::LoadDefault(0,Value);
}

void CSettingTypeRomDatabaseIndex::LoadDefault ( int Index, stdstr & Value ) const
{
    m_KeyName.Format("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeRomDatabase::LoadDefault(0,Value);
}

void CSettingTypeRomDatabaseIndex::Save ( int /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Save ( int /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Save ( int /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRomDatabaseIndex::Delete ( int /*Index*/ )
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),NULL);
}
