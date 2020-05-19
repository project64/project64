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
#include "SettingsType-Cheats.h"
#include <Project64-core\N64System\CheatClass.h>

CIniFile * CSettingTypeCheats::m_CheatIniFile = NULL;
std::string * CSettingTypeCheats::m_SectionIdent = NULL;
bool CSettingTypeCheats::m_CheatsModified = false;

CSettingTypeCheats::CSettingTypeCheats(const char * PostFix, SettingID UserSetting) :
    m_PostFix(PostFix),
    m_UserSetting(UserSetting)
{
}

CSettingTypeCheats::~CSettingTypeCheats ( void )
{
}

void CSettingTypeCheats::Initialize ( void )
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");
    m_CheatIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Cheats).c_str());
    m_CheatIniFile->SetAutoFlush(false);
    g_Settings->RegisterChangeCB(Game_IniKey,NULL,GameChanged);
    m_SectionIdent = new stdstr(g_Settings->LoadStringVal(Game_IniKey));
    GameChanged(NULL);
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeCheats::CleanUp   ( void )
{
    if (m_CheatIniFile)
    {
        m_CheatIniFile->SetAutoFlush(true);
        delete m_CheatIniFile;
        m_CheatIniFile = NULL;
    }
    if (m_SectionIdent)
    {
        delete m_SectionIdent;
        m_SectionIdent = NULL;
    }
}

void CSettingTypeCheats::FlushChanges( void )
{
    if (m_CheatIniFile)
    {
        m_CheatIniFile->FlushChanges();
    }
}

void CSettingTypeCheats::GameChanged ( void * /*Data */ )
{
    *m_SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    m_CheatsModified = g_Settings->LoadBool(Cheat_Modified);
}

bool CSettingTypeCheats::IsSettingSet(void) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeCheats::Load (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeCheats::Load (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

bool CSettingTypeCheats::Load (uint32_t Index, std::string & Value ) const
{
    if (m_CheatsModified)
    {
        return g_Settings->LoadStringIndex(m_UserSetting, Index, Value);
    }
    if (m_CheatIniFile == NULL)
    {
        return false;
    }
    stdstr_f Key("Cheat%d%s",Index,m_PostFix);
    return m_CheatIniFile->GetString(m_SectionIdent->c_str(),Key.c_str(),"",Value);
}

//return the default values
void CSettingTypeCheats::LoadDefault (uint32_t /*Index*/, bool & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::LoadDefault (uint32_t /*Index*/, uint32_t & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::LoadDefault (uint32_t /*Index*/, std::string & /*Value*/ ) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

//Update the settings
void CSettingTypeCheats::Save (uint32_t /*Index*/, bool /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::Save (uint32_t /*Index*/, uint32_t /*Value*/ )
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeCheats::Save (uint32_t Index, const std::string & Value )
{
    if (!m_CheatsModified)
    {
        CopyCheats();
        m_CheatsModified = true;
        g_Settings->SaveBool(Cheat_Modified, true);
    }
    g_Settings->SaveStringIndex(m_UserSetting, Index, Value.c_str());
}

void CSettingTypeCheats::Save (uint32_t Index, const char * Value )
{
    if (!m_CheatsModified)
    {
        CopyCheats();
        m_CheatsModified = true;
        g_Settings->SaveBool(Cheat_Modified, true);
    }
    g_Settings->SaveStringIndex(m_UserSetting, Index, Value);
}

void CSettingTypeCheats::Delete (uint32_t Index )
{
    if (!m_CheatsModified)
    {
        CopyCheats();
        m_CheatsModified = true;
        g_Settings->SaveBool(Cheat_Modified, true);
    }
    g_Settings->DeleteSettingIndex(m_UserSetting, Index);
}

void CSettingTypeCheats::CopyCheats(void)
{
    for (int i = 0; i < CCheats::MaxCheats; i++)
    {
        std::string Value;
        if (!g_Settings->LoadStringIndex(Cheat_Entry, i, Value))
        {
            break;
        }
        g_Settings->SaveStringIndex(Cheat_UserEntry, i, Value);
        if (g_Settings->LoadStringIndex(Cheat_Notes, i, Value))
        {
            g_Settings->SaveStringIndex(Cheat_UserNotes, i, Value);
        }
        if (g_Settings->LoadStringIndex(Cheat_Options, i, Value))
        {
            g_Settings->SaveStringIndex(Cheat_UserOptions, i, Value);
        }
    }
}