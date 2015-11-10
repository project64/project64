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

#include "SettingsType-Base.h"

class CSettingTypeSelectedDirectory :
    public CSettingType
{
public:
    CSettingTypeSelectedDirectory(const char * Name, SettingID InitialDir, SettingID SelectedDir, SettingID UseSelected );
    ~CSettingTypeSelectedDirectory();

    virtual bool        IndexBasedSetting ( void ) const { return false; }
    virtual SettingType GetSettingType    ( void ) const { return SettingType_SelectedDirectory; }

    const char * GetName ( void ) const { return m_Name.c_str(); }

    //return the values
    virtual bool Load   ( int Index, bool & Value   ) const;
    virtual bool Load   ( int Index, uint32_t & Value  ) const;
    virtual bool Load   ( int Index, stdstr & Value ) const;

    //return the default values
    virtual void LoadDefault ( int Index, bool & Value   ) const;
    virtual void LoadDefault ( int Index, uint32_t & Value  ) const;
    virtual void LoadDefault ( int Index, stdstr & Value ) const;

    //Update the settings
    virtual void Save   ( int Index, bool Value );
    virtual void Save   ( int Index, uint32_t Value );
    virtual void Save   ( int Index, const stdstr & Value );
    virtual void Save   ( int Index, const char * Value );

    // Delete the setting
    virtual void Delete ( int Index );

private:
    CSettingTypeSelectedDirectory(void);                                              // Disable default constructor
    CSettingTypeSelectedDirectory(const CSettingTypeSelectedDirectory&);              // Disable copy constructor
    CSettingTypeSelectedDirectory& operator=(const CSettingTypeSelectedDirectory&);   // Disable assignment

    std::string m_Name;
    SettingID m_InitialDir;
    SettingID m_SelectedDir;
    SettingID m_UseSelected;
};
