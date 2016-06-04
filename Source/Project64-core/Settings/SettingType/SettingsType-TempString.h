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

class CSettingTypeTempString :
    public CSettingType
{
public:
    CSettingTypeTempString(const char * initialValue);
    ~CSettingTypeTempString();

    bool        IndexBasedSetting ( void ) const { return false; }
    SettingType GetSettingType    ( void ) const { return SettingType_StringVariable; }
    bool        IsSettingSet(void) const { return false; }

    //return the values
    bool Load   ( int32_t Index, bool & Value   ) const;
    bool Load   ( int32_t Index, uint32_t & Value  ) const;
    bool Load   ( int32_t Index, stdstr & Value ) const;

    //return the default values
    void LoadDefault ( int32_t Index, bool & Value   ) const;
    void LoadDefault ( int32_t Index, uint32_t & Value  ) const;
    void LoadDefault ( int32_t Index, stdstr & Value ) const;

    //Update the settings
    void Save   ( int32_t Index, bool Value );
    void Save   ( int32_t Index, uint32_t Value );
    void Save   ( int32_t Index, const stdstr & Value );
    void Save   ( int32_t Index, const char * Value );

    // Delete the setting
    void Delete ( int32_t Index );

private:
    CSettingTypeTempString(void);                                     // Disable default constructor
    CSettingTypeTempString(const CSettingTypeTempString&);            // Disable copy constructor
    CSettingTypeTempString& operator=(const CSettingTypeTempString&); // Disable assignment

    stdstr m_value;
};
