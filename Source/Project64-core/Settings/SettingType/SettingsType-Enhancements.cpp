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
#include "SettingsType-Enhancements.h"

CIniFile * CSettingTypeEnhancements::m_EnhancementIniFile = NULL;
std::string * CSettingTypeEnhancements::m_SectionIdent = NULL;
std::string * CSettingTypeEnhancements::m_GameName = NULL;

CSettingTypeEnhancements::CSettingTypeEnhancements(const char * PostFix ) :
    m_PostFix(PostFix)
{
}

CSettingTypeEnhancements::~CSettingTypeEnhancements( void )
{
}

void CSettingTypeEnhancements::Initialize ( void )
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");
	m_EnhancementIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Enhancements).c_str());
	m_EnhancementIniFile->SetAutoFlush(false);
    g_Settings->RegisterChangeCB(Game_IniKey,NULL,GameChanged);
	m_SectionIdent = new std::string(g_Settings->LoadStringVal(Game_IniKey));
	m_GameName = new std::string(g_Settings->LoadStringVal(Game_IniKey));
	GameChanged(NULL);
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeEnhancements::CleanUp   ( void )
{
    if (m_EnhancementIniFile)
    {
		m_EnhancementIniFile->SetAutoFlush(true);
        delete m_EnhancementIniFile;
		m_EnhancementIniFile = NULL;
    }
	if (m_SectionIdent)
	{
		delete m_SectionIdent;
		m_SectionIdent = NULL;
	}
	if (m_GameName)
	{
		delete m_GameName;
		m_GameName = NULL;
	}
}

void CSettingTypeEnhancements::FlushChanges( void )
{
    if (m_EnhancementIniFile)
    {
		m_EnhancementIniFile->FlushChanges();
    }
}

void CSettingTypeEnhancements::GameChanged ( void * /*Data */ )
{
    *m_SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
	*m_GameName = g_Settings->LoadStringVal(Rdb_GoodName);
}

bool CSettingTypeEnhancements::IsSettingSet(void) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeEnhancements::Load (uint32_t Index, bool & Value ) const
{
	if (m_EnhancementIniFile == NULL)
	{
		return false;
	}
	stdstr_f Key("Enhancement%d%s", Index, m_PostFix);
	uint32_t dwValue = 0;
	bool bRes = m_EnhancementIniFile->GetNumber(m_SectionIdent->c_str(), Key.c_str(), 0, dwValue);
	if (bRes)
	{
		Value = dwValue != 0;
	}
	return bRes;
}

bool CSettingTypeEnhancements::Load (uint32_t Index, uint32_t & Value ) const
{
	if (m_EnhancementIniFile == NULL)
	{
		return false;
	}
	stdstr_f Key("Enhancement%d%s", Index, m_PostFix);
	return m_EnhancementIniFile->GetNumber(m_SectionIdent->c_str(), Key.c_str(), 0, Value);
}

bool CSettingTypeEnhancements::Load (uint32_t Index, std::string & Value ) const
{
    if (m_EnhancementIniFile == NULL)
    {
        return false;
    }
    stdstr_f Key("Enhancement%d%s",Index,m_PostFix);
    return m_EnhancementIniFile->GetString(m_SectionIdent->c_str(),Key.c_str(),"",Value);
}

//return the default values
void CSettingTypeEnhancements::LoadDefault (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeEnhancements::LoadDefault (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeEnhancements::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

//Update the settings
void CSettingTypeEnhancements::Save (uint32_t Index, bool Value )
{
	if (m_EnhancementIniFile == NULL) { return; }

	stdstr_f Key("Enhancement%d%s", Index, m_PostFix);
	m_EnhancementIniFile->SaveNumber(m_SectionIdent->c_str(), Key.c_str(), Value);
}

void CSettingTypeEnhancements::Save (uint32_t Index, uint32_t Value )
{
	if (m_EnhancementIniFile == NULL) { return; }

	stdstr_f Key("Enhancement%d%s", Index, m_PostFix);
	m_EnhancementIniFile->SaveNumber(m_SectionIdent->c_str(), Key.c_str(), Value);
}

void CSettingTypeEnhancements::Save (uint32_t Index, const std::string & Value )
{
    if (m_EnhancementIniFile == NULL) {  return;  }

    stdstr_f Key("Enhancement%d%s",Index,m_PostFix);
	m_EnhancementIniFile->SaveString(m_SectionIdent->c_str(),Key.c_str(),Value.c_str());
}

void CSettingTypeEnhancements::Save (uint32_t Index, const char * Value )
{
    if (m_EnhancementIniFile == NULL) {  return;  }

    stdstr_f Key("Enhancement%d%s",Index,m_PostFix);
	m_EnhancementIniFile->SaveString(m_SectionIdent->c_str(),Key.c_str(),Value);
}

void CSettingTypeEnhancements::Delete (uint32_t Index )
{
    stdstr_f Key("Enhancement%d%s",Index,m_PostFix);
	m_EnhancementIniFile->SaveString(m_SectionIdent->c_str(),Key.c_str(),NULL);
}
