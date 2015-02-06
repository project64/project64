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
#pragma once

#include "SettingsType-RomDatabase.h"

class CSettingTypeRomDatabaseSetting :
	public CSettingTypeRomDatabase
{
public:
	CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, LPCSTR DefaultValue, bool DeleteOnDefault = false );
	CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, bool DefaultValue, bool DeleteOnDefault = false );
	CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, int DefaultValue, bool DeleteOnDefault = false );
	CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, SettingID DefaultSetting, bool DeleteOnDefault = false );
	
	virtual ~CSettingTypeRomDatabaseSetting();

	virtual SettingType GetSettingType    ( void ) const { return SettingType_RdbSetting; }	

private:
	virtual LPCSTR Section ( void ) const { return m_SectionIdent.c_str(); }

	stdstr m_SectionIdent;
};

