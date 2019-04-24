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
#include "SettingsType-Application.h"
#include "SettingsType-GameSetting.h"
#include "SettingsType-GameSettingIndex.h"

CSettingTypeGameIndex::CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, SettingID DefaultSetting ) :
    CSettingTypeGame("", DefaultSetting),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, uint32_t DefaultValue ) :
    CSettingTypeGame("", DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, bool DefaultValue) :
    CSettingTypeGame("", DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, const char * DefaultValue ) :
    CSettingTypeGame("", DefaultValue),
    m_PreIndex(PreIndex),
    m_PostIndex(PostIndex)
{
}

CSettingTypeGameIndex::~CSettingTypeGameIndex()
{
}

bool CSettingTypeGameIndex::Load (uint32_t Index, bool & Value ) const
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    return CSettingTypeGame::Load(Index,Value);
}

bool CSettingTypeGameIndex::Load (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeGameIndex::Load (uint32_t Index, std::string & Value ) const
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    return CSettingTypeGame::Load(0,Value);
}

//return the default values
void CSettingTypeGameIndex::LoadDefault (uint32_t Index, bool & Value ) const
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeGame::LoadDefault(0,Value);
}

void CSettingTypeGameIndex::LoadDefault (uint32_t /*Index*/, uint32_t & /*Value*/  ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeGameIndex::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

//Update the settings
void CSettingTypeGameIndex::Save (uint32_t Index, bool Value )
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeGame::Save(Index,Value);
}

void CSettingTypeGameIndex::Save(uint32_t Index, uint32_t Value)
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeGame::Save(0,Value);
}

void CSettingTypeGameIndex::Save (uint32_t /*Index*/, const std::string & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeGameIndex::Save (uint32_t Index, const char * Value )
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeGame::Save(0,Value);
}

void CSettingTypeGameIndex::Delete (uint32_t Index )
{
    m_KeyNameIdex = stdstr_f("%s%d%s",m_PreIndex.c_str(),Index,m_PostIndex.c_str());
    CSettingTypeGame::Delete(0);
}
