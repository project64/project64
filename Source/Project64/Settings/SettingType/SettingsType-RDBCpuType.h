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

class CSettingTypeRDBCpuType :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRDBCpuType(const char * Name, SettingID DefaultSetting );
    CSettingTypeRDBCpuType(const char * Name, int DefaultValue );
    ~CSettingTypeRDBCpuType();

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
    CSettingTypeRDBCpuType(void);                                       // Disable default constructor
    CSettingTypeRDBCpuType(const CSettingTypeRDBCpuType&);              // Disable copy constructor
    CSettingTypeRDBCpuType& operator=(const CSettingTypeRDBCpuType&);   // Disable assignment
};
