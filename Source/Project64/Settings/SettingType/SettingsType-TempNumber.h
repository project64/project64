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

class CSettingTypeTempNumber :
    public CSettingType
{
public:
    CSettingTypeTempNumber(uint32_t initialValue);
    ~CSettingTypeTempNumber();

    bool        IndexBasedSetting ( void ) const { return false; }
    SettingType GetSettingType    ( void ) const { return SettingType_NumberVariable; }

    //return the values
    bool Load   ( int Index, bool & Value   ) const;
    bool Load   ( int Index, uint32_t & Value  ) const;
    bool Load   ( int Index, stdstr & Value ) const;

    //return the default values
    void LoadDefault ( int Index, bool & Value   ) const;
    void LoadDefault ( int Index, uint32_t & Value  ) const;
    void LoadDefault ( int Index, stdstr & Value ) const;

    //Update the settings
    void Save   ( int Index, bool Value );
    void Save   ( int Index, uint32_t Value );
    void Save   ( int Index, const stdstr & Value );
    void Save   ( int Index, const char * Value );

    // Delete the setting
    void Delete ( int Index );

private:
    CSettingTypeTempNumber(void);                                     // Disable default constructor
    CSettingTypeTempNumber(const CSettingTypeTempNumber&);            // Disable copy constructor
    CSettingTypeTempNumber& operator=(const CSettingTypeTempNumber&); // Disable assignment

    uint32_t m_value;
};
