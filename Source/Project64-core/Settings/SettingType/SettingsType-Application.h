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

#include <Common/IniFileClass.h>
#include "SettingsType-Base.h"

class CSettingTypeApplication :
    public CSettingType
{
public:
    CSettingTypeApplication(const char * Section, const char * Name, const char * DefaultValue);
    CSettingTypeApplication(const char * Section, const char * Name, bool DefaultValue);
    CSettingTypeApplication(const char * Section, const char * Name, uint32_t DefaultValue);
    CSettingTypeApplication(const char * Section, const char * Name, SettingID DefaultSetting);
    virtual ~CSettingTypeApplication();

    virtual bool        IndexBasedSetting(void) const { return false; }
    virtual SettingType GetSettingType(void) const { return SettingType_CfgFile; }

    //return the values
    virtual bool Load(int32_t Index, bool & Value) const;
    virtual bool Load(int32_t Index, uint32_t & Value) const;
    virtual bool Load(int32_t Index, stdstr & Value) const;

    //return the default values
    virtual void LoadDefault(int32_t Index, bool & Value) const;
    virtual void LoadDefault(int32_t Index, uint32_t & Value) const;
    virtual void LoadDefault(int32_t Index, stdstr & Value) const;

    //Update the settings
    virtual void Save(int32_t Index, bool Value);
    virtual void Save(int32_t Index, uint32_t Value);
    virtual void Save(int32_t Index, const stdstr & Value);
    virtual void Save(int32_t Index, const char * Value);

    // Delete the setting
    virtual void Delete(int32_t Index);

    // Initialize this class to use ini or registry
    static void Initialize(const char * AppName);
    static void CleanUp(void);
    static void Flush(void);

    const char * GetKeyName(void) const { return m_KeyName.c_str(); }

protected:
    const char * m_DefaultStr;
    const uint32_t m_DefaultValue;
    const SettingID m_DefaultSetting;

    stdstr FixSectionName(const char * Section);

    static CIniFile * m_SettingsIniFile;
    const stdstr      m_Section;
    const stdstr      m_KeyName;
    mutable stdstr    m_KeyNameIdex;

    virtual const char * SectionName(void) const;

private:
    CSettingTypeApplication(const CSettingTypeApplication&);				// Disable copy constructor
    CSettingTypeApplication& operator=(const CSettingTypeApplication&);		// Disable assignment
};
