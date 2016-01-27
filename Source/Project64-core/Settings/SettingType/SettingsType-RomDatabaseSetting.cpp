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
#include "SettingsType-RomDatabaseSetting.h"

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, int DefaultValue, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
	if (!m_GlideSetting)
	{
		m_GlideSetting = IsGlideSetting(m_SectionIdent.c_str());
		m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
	}
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, bool DefaultValue, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
	if (!m_GlideSetting)
	{
		m_GlideSetting = IsGlideSetting(m_SectionIdent.c_str());
		m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
	}
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, const char * DefaultValue, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultValue, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
	if (!m_GlideSetting)
	{
		m_GlideSetting = IsGlideSetting(m_SectionIdent.c_str());
		m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
	}
}

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(const char * SectionIdent, const char * Name, SettingID DefaultSetting, bool DeleteOnDefault ) :
	CSettingTypeRomDatabase(Name, DefaultSetting, DeleteOnDefault),
	m_SectionIdent(SectionIdent)
{
	m_SectionIdent.Replace("\\","-");
	if (!m_GlideSetting)
	{
		m_GlideSetting = IsGlideSetting(m_SectionIdent.c_str());
		m_SectionIdent = StripNameSection(m_SectionIdent.c_str());
	}
}

CSettingTypeRomDatabaseSetting::~CSettingTypeRomDatabaseSetting()
{
}
