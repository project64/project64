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
#include "SettingsType-RDBRamSize.h"

// == 8 ? 0x800000 : 0x400000

CSettingTypeRDBRDRamSize::CSettingTypeRDBRDRamSize(const char * Name, SettingID DefaultSetting ) :
    CSettingTypeRomDatabase(Name,DefaultSetting)
{
}

CSettingTypeRDBRDRamSize::CSettingTypeRDBRDRamSize(const char * Name, int DefaultValue ) :
    CSettingTypeRomDatabase(Name,DefaultValue)
{
}

CSettingTypeRDBRDRamSize::~CSettingTypeRDBRDRamSize()
{
}

bool CSettingTypeRDBRDRamSize::Load ( int /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeRDBRDRamSize::Load ( int Index, uint32_t & Value ) const
{
    uint32_t ulValue;
    bool bRes = m_SettingsIniFile->GetNumber(m_SectionIdent->c_str(),m_KeyName.c_str(),m_DefaultValue,ulValue);
    if (!bRes)
    {
        LoadDefault(Index,ulValue);
    }
    Value = 0x400000;
    if (ulValue == 8)
    {
        Value = 0x800000;
    }
    return bRes;
}

bool CSettingTypeRDBRDRamSize::Load ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

//return the default values
void CSettingTypeRDBRDRamSize::LoadDefault ( int /*Index*/, bool & /*Value*/   ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::LoadDefault ( int /*Index*/, uint32_t & Value  ) const
{
    Value = m_DefaultValue;
}

void CSettingTypeRDBRDRamSize::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

//Update the settings
void CSettingTypeRDBRDRamSize::Save ( int /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::Save ( int /*Index*/, uint32_t Value )
{
    m_SettingsIniFile->SaveNumber(m_SectionIdent->c_str(),m_KeyName.c_str(),Value == 0x800000 ? 8 : 4);
}

void CSettingTypeRDBRDRamSize::Save ( int /*Index*/, const stdstr & /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::Save ( int /*Index*/, const char * /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeRDBRDRamSize::Delete( int /*Index*/ )
{
    m_SettingsIniFile->SaveString(m_SectionIdent->c_str(),m_KeyName.c_str(),NULL);
}
