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
#pragma once
#include <Common/path.h>
#include "SettingsType-Base.h"

class CSettingTypeRelativePath :
    public CSettingType
{
    CPath m_FileName;

public:
    CSettingTypeRelativePath(const char * Path, const char * FileName);
    ~CSettingTypeRelativePath();

    bool        IndexBasedSetting ( void ) const { return false; }
    SettingType GetSettingType    ( void ) const { return SettingType_RelativePath; }

    //return the values
    bool Load   ( int /*Index*/, bool & /*Value*/ ) const { return false; };
    bool Load   ( int /*Index*/, uint32_t & /*Value*/  ) const { return false; };
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
    CSettingTypeRelativePath(void);                                         // Disable default constructor
    CSettingTypeRelativePath(const CSettingTypeRelativePath&);              // Disable copy constructor
    CSettingTypeRelativePath& operator=(const CSettingTypeRelativePath&);   // Disable assignment
};
