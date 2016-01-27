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
#include "SettingsType-ApplicationIndex.h"

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(const char * Section, const char * Name, uint32_t DefaultValue ) :
    CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(const char * Section, const char * Name, bool DefaultValue ) :
    CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(const char * Section, const char * Name, const char * DefaultValue ) :
    CSettingTypeApplication(Section,Name,DefaultValue)
{
}

CSettingTypeApplicationIndex::CSettingTypeApplicationIndex(const char * Section, const char * Name, SettingID DefaultSetting ) :
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

bool CSettingTypeApplicationIndex::Load ( int Index, uint32_t & Value ) const
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

void CSettingTypeApplicationIndex::LoadDefault ( int Index, uint32_t & Value  ) const
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

void CSettingTypeApplicationIndex::Save ( int Index, uint32_t Value )
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
