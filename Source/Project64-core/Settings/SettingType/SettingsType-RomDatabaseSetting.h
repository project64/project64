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
#pragma once

#include "SettingsType-RomDatabase.h"

class CSettingTypeRomDatabaseSetting :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, const char * DefaultValue, bool DeleteOnDefault = false );
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, bool DefaultValue, bool DeleteOnDefault = false );
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, int DefaultValue, bool DeleteOnDefault = false );
    CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, SettingID DefaultSetting, bool DeleteOnDefault = false );

    virtual ~CSettingTypeRomDatabaseSetting();

    virtual SettingType GetSettingType    ( void ) const { return SettingType_RdbSetting; }

private:
    virtual const char * Section ( void ) const { return m_SectionIdent.c_str(); }

    stdstr m_SectionIdent;

private:
    CSettingTypeRomDatabaseSetting(void);                                               // Disable default constructor
    CSettingTypeRomDatabaseSetting(const CSettingTypeRomDatabaseSetting&);              // Disable copy constructor
    CSettingTypeRomDatabaseSetting& operator=(const CSettingTypeRomDatabaseSetting&);   // Disable assignment
};
