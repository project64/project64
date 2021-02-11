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
#include <Project64-core\N64System\Enhancement\Enhancement.h>
#include <Project64-core\Settings\SettingType\SettingsType-GameSetting.h>
#include <Project64-core\N64System\SystemGlobals.h>
#include <Project64-core\N64System\N64Class.h>

#pragma warning(disable:4996)

static std::string GenerateKeyName(const char * Name, const char * Ident, const char * PostIdent)
{
    stdstr KeyName(Name);
    KeyName.Replace('\\', '_');
    KeyName.Replace(' ', '_');
    if (KeyName.empty())
    {
        return "";
    }
    return stdstr_f("%s_%s_%s", Ident, KeyName.c_str(), PostIdent);
}

class CSettingEnhancementActive :
    public CSettingTypeGame
{
public:
    CSettingEnhancementActive(const char * Name, const char * Ident, bool Default) :
        CSettingTypeGame("",false),
        m_Default(Default)
    {
        m_KeyNameIdex = GenerateKeyName(Name, Ident, "Active");
    }

    bool Active()
    {
        if (m_KeyNameIdex.empty())
        {
            return false;
        }
        bool Active = false;
        if (Load(0, Active))
        {
            return Active;
        }
        return m_Default;
    }

    void SetActive(bool Active)
    {
        if (m_KeyNameIdex.empty())
        {
            return;
        }
        m_SettingsIniFile->SaveNumber(SectionName(), m_KeyNameIdex.c_str(), Active ? 1 : 0);
        Flush();
    }

    void Delete()
    {
        if (m_KeyNameIdex.empty())
        {
            return;
        }
        CSettingTypeApplication::Delete(0);
        Flush();
    }

private:
    bool m_Default;
};

class CSettingEnhancementSelectedOption :
    public CSettingTypeGame
{
public:
    CSettingEnhancementSelectedOption(const char * Name, const char * Ident) :
        CSettingTypeGame("", false)
    {
        m_KeyNameIdex = GenerateKeyName(Name, Ident, "Option");
    }

    void SetOption(uint16_t Value)
    {
        if (m_KeyNameIdex.empty())
        {
            return;
        }
        Save(0, (uint32_t)Value);
        Flush();
    }

    bool SelectedValue(uint16_t &Value)
    {
        uint32_t StoredValue = 0;
        if (!Load(0, StoredValue))
        {
            return false;
        }
        Value = (uint16_t)(StoredValue & 0xFFFF);
        return true;
    }

    void Delete()
    {
        if (m_KeyNameIdex.empty())
        {
            return;
        }
        CSettingTypeGame::Delete(0);
        Flush();
    }
};

CEnhancement::CEnhancement(const char * Ident) :
    m_Ident(Ident),
    m_CodeOptionSize(0),
    m_SelectedOption(0xFFFF0000),
    m_OnByDefault(false),
    m_Active(false),
    m_Valid(false)
{
}

CEnhancement::CEnhancement(const char * Ident, const char * Entry) :
    m_Ident(Ident),
    m_CodeOptionSize(0),
    m_SelectedOption(0xFFFF0000),
    m_OnByDefault(false),
    m_Active(false),
    m_Valid(false)
{
    stdstr EntryLine(Entry);
    EntryLine.Replace("\r", "");
    strvector Lines = EntryLine.Tokenize('\n');
    if (Lines.size() == 0 || Lines[0].length() < 2 || Lines[0][0] != '$')
    {
        return;
    }
    uint32_t CurrentLine = 1;
    m_Name = &(Lines[0][CurrentLine]);
    m_NameAndExtension = m_Name;

    //key=value
    while (CurrentLine < Lines.size())
    {
        const char * Pos = strchr(Lines[CurrentLine].c_str(), '=');
        if (Pos == nullptr)
        {
            break;
        }
        uint32_t Line = CurrentLine;
        CurrentLine += 1;
        std::string Key = Lines[Line].c_str();
        size_t Seperator = Pos - Lines[Line].c_str();
        if (Seperator >= Key.size())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        Key.resize(Seperator);

        if (stricmp(Key.c_str(), "PluginList") == 0)
        {
            strvector Plugins = stdstr(&Pos[1]).Tokenize(",");
            for (size_t i = 0, n = Plugins.size(); i < n; i++)
            {
                m_PluginList.push_back(Plugins[i]);
            }
        }
        else if (stricmp(Key.c_str(), "Note") == 0)
        {
            m_Note = &Pos[1];
        }
        else if (stricmp(Key.c_str(), "OnByDefault") == 0)
        {
            m_OnByDefault = Pos[1] == '1';
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    //Gameshark Code
    while (CurrentLine < Lines.size())
    {
        const char * Line = Lines[CurrentLine].c_str();
        const char * Pos = strchr(Line, ' ');
        if (strlen(Line) != 13 || Pos == nullptr || (Pos - Line) != 8)
        {
            break;
        }
        CurrentLine += 1;

        CodeEntry GSEntry;
        GSEntry.Command = strtoul(Line, 0, 16);
        GSEntry.Value = &Line[9];
        m_Entries.push_back(GSEntry);
    }

    //Options
    uint32_t OptionSize = 0;
    while (CurrentLine < Lines.size())
    {
        const char * Line = Lines[CurrentLine].c_str();
        size_t LineLen = strlen(Line);
        if (((OptionSize != 0 && OptionSize != 2) || LineLen < 4 || Line[2] != ' ') &&
            ((OptionSize != 0 && OptionSize != 4) || LineLen < 6 || Line[4] != ' '))
        {
            break;
        }
        CurrentLine += 1;
        if (OptionSize == 0)
        {
            OptionSize = Line[2] == ' ' ? 2 : 4;
        }
        CodeOption Option;
        Option.Value = (uint16_t)strtoul(Line, 0, 16);
        Option.Name = &Line[OptionSize + 1];
        m_Options.push_back(Option);
    }

    if (CurrentLine < Lines.size())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_Active = CSettingEnhancementActive(m_Name.c_str(), m_Ident.c_str(), m_OnByDefault).Active();
    uint16_t SelectedValue = 0;
    if (CSettingEnhancementSelectedOption(m_Name.c_str(), m_Ident.c_str()).SelectedValue(SelectedValue))
    {
        m_SelectedOption = SelectedValue;
    }
    CheckValid();
}

void CEnhancement::SetName(const char * Name)
{
    std::string NewName = stdstr(Name != NULL ? Name : "").Trim("\t ,");
    if (NewName == m_Name)
    {
        return;
    }
    CSettingEnhancementActive(m_Name.c_str(), m_Ident.c_str(), m_OnByDefault).Delete();
    CSettingEnhancementSelectedOption(m_Name.c_str(), m_Ident.c_str()).Delete();
    m_Name = stdstr(Name != NULL ? Name : "").Trim("\t ,");
    m_NameAndExtension = m_Name;
    if (m_Active != m_OnByDefault) { CSettingEnhancementActive(m_Name.c_str(), m_Ident.c_str(), m_OnByDefault).SetActive(m_OnByDefault); }
    if (OptionSelected()) { CSettingEnhancementSelectedOption(m_Name.c_str(), m_Ident.c_str()).SetOption(SelectedOption()); }
    CheckValid();
}

void CEnhancement::SetNote(const char * Note)
{
    m_Note = Note != NULL ? Note : "";
}

void CEnhancement::SetEntries(const CodeEntries & Entries)
{
    m_Entries = Entries;
    CheckValid();
}

void CEnhancement::SetPluginList(const PluginList & List)
{
    m_PluginList = List;
}

void CEnhancement::SetOptions(const CodeOptions & Options)
{
    m_Options = Options;
    CheckValid();
}

void CEnhancement::SetSelectedOption(uint16_t Value)
{
    if (m_Name.empty())
    {
        return;
    }
    m_SelectedOption = Value;
    CheckValid();
    CSettingEnhancementSelectedOption(m_Name.c_str(), m_Ident.c_str()).SetOption(Value);
}

void CEnhancement::SetActive(bool Active)
{
    if (m_Name.empty())
    {
        return;
    }
    m_Active = Active;
    CSettingEnhancementActive(m_Name.c_str(), m_Ident.c_str(), m_OnByDefault).SetActive(m_Active);
}

void CEnhancement::SetOnByDefault(bool OnByDefault)
{
    m_OnByDefault = OnByDefault;
    m_Active = CSettingEnhancementActive(m_Name.c_str(), m_Ident.c_str(), m_OnByDefault).Active();
}

void CEnhancement::CheckValid(void)
{
    m_Valid = false;
    m_OptionValue = "";
    m_CodeOptionSize = 0;

    if (m_Name.empty() || m_Entries.size() == 0)
    {
        return;
    }

    std::string OptionValue;
    uint32_t CodeOptionSize = 0;
    for (CodeEntries::const_iterator itr = m_Entries.begin(); itr != m_Entries.end(); itr++)
    {
        switch (itr->Command & 0xFF000000)
        {
        case 0x80000000:
        case 0x81000000:
        case 0x88000000:
        case 0x89000000:
        case 0xA0000000:
        case 0xA1000000:
        case 0xD0000000:
        case 0xD1000000:
        case 0xD2000000:
        case 0xD3000000:
            if (strchr(itr->Value.c_str(), '?') != NULL)
            {
                if (strncmp(itr->Value.c_str(), "????", 4) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "????")
                    {
                        return;
                    }
                    OptionValue = "????";
                    CodeOptionSize = 4;
                }
                else if (itr->Value.length() == 4 && strncmp(&itr->Value.c_str()[2], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "XX??")
                    {
                        return;
                    }
                    OptionValue = "XX??";
                    CodeOptionSize = 2;
                }
                else if (itr->Value.length() == 4 && strncmp(&itr->Value.c_str()[0], "??", 2) == 0)
                {
                    if (!OptionValue.empty() && OptionValue != "??XX")
                    {
                        return;
                    }
                    OptionValue = "??XX";
                    CodeOptionSize = 2;
                }
                else
                {
                    return;
                }
            }
            break;
        case 0x50000000:
            if (strchr(itr->Value.c_str(), '?') != NULL)
            {
                return;
            }
            break;
        default:
            return;
        }
    }
    if (m_Options.size() > 0 && OptionValue.empty())
    {
        return;
    }
    m_OptionValue = OptionValue;
    m_CodeOptionSize = CodeOptionSize;
    m_Valid = true;
    if (m_CodeOptionSize != 0)
    {
        std::string CheatValue = "??? - Not Set";
        if (OptionSelected())
        {
            bool found = false;
            for (size_t i = 0, n = m_Options.size(); i < n; i++)
            {
                if (m_Options[i].Value != SelectedOption())
                {
                    continue;
                }
                CheatValue = stdstr_f("$%02X %s", m_Options[i].Value, m_Options[i].Name.c_str());
                found = true;
                break;
            }
            if (!found)
            {
                m_SelectedOption = 0xFFFF0000;
            }
        }
        m_NameAndExtension = stdstr_f("%s (=>%s)", m_Name.c_str(), CheatValue.c_str());
    }
}