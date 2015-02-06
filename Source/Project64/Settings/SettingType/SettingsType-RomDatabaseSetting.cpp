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
#include "SettingsType-RomDatabaseSetting.h"

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, int DefaultValue, bool DeleteOnDefault ) :
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

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, bool DefaultValue, bool DeleteOnDefault ) :
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

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, LPCSTR DefaultValue, bool DeleteOnDefault ) :
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

CSettingTypeRomDatabaseSetting::CSettingTypeRomDatabaseSetting(LPCSTR SectionIdent, LPCSTR Name, SettingID DefaultSetting, bool DeleteOnDefault ) :
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
