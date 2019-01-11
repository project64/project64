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

#include <Project64-core/Settings/SettingType/SettingsType-Base.h>
#include <Common/IniFileClass.h>

class CSettingTypeEnhancements :
    public CSettingType
{
public:
	CSettingTypeEnhancements(const char * PostFix );
    ~CSettingTypeEnhancements();

    virtual bool IndexBasedSetting ( void ) const { return true; }
    virtual SettingType GetSettingType ( void ) const { return SettingType_Enhancement; }
    virtual bool IsSettingSet ( void ) const;

    //return the values
    virtual bool Load (uint32_t Index, bool & Value ) const;
    virtual bool Load (uint32_t Index, uint32_t & Value ) const;
    virtual bool Load (uint32_t Index, std::string & Value ) const;

    //return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value ) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value ) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value ) const;

    //Update the settings
    virtual void Save (uint32_t Index, bool Value );
    virtual void Save (uint32_t Index, uint32_t Value );
    virtual void Save (uint32_t Index, const std::string & Value );
    virtual void Save (uint32_t Index, const char * Value );

    // Delete the setting
    virtual void Delete (uint32_t Index );

    // Initialize this class to use ini or registry
    static void Initialize ( void );
    static void CleanUp ( void );
    static void FlushChanges ( void );

protected:
    static CIniFile * m_EnhancementIniFile;
    static std::string * m_SectionIdent;
	static std::string * m_GameName;
    const char * const m_PostFix;
    static void GameChanged ( void * /*Data */ );

private:
	CSettingTypeEnhancements(void);                                       // Disable default constructor
	CSettingTypeEnhancements(const CSettingTypeEnhancements&);            // Disable copy constructor
	CSettingTypeEnhancements& operator=(const CSettingTypeEnhancements&); // Disable assignment
};
