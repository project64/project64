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

class CSettingTypeRDBOnOff :
    public CSettingTypeRomDatabase
{
public:
    CSettingTypeRDBOnOff(const char * Name, SettingID DefaultSetting );
    CSettingTypeRDBOnOff(const char * Name, int32_t DefaultValue );
    ~CSettingTypeRDBOnOff();

    //return the values
    virtual bool Load ( int32_t Index, bool & Value   ) const;
    virtual bool Load ( int32_t Index, uint32_t & Value  ) const;
    virtual bool Load ( int32_t Index, stdstr & Value ) const;

    //return the default values
    virtual void LoadDefault ( int32_t Index, bool & Value   ) const;
    virtual void LoadDefault ( int32_t Index, uint32_t & Value  ) const;
    virtual void LoadDefault ( int32_t Index, stdstr & Value ) const;

    //Update the settings
    virtual void Save ( int32_t Index, bool Value );
    virtual void Save ( int32_t Index, uint32_t Value );
    virtual void Save ( int32_t Index, const stdstr & Value );
    virtual void Save ( int32_t Index, const char * Value );

    // Delete the setting
    virtual void Delete ( int32_t Index );

private:
    CSettingTypeRDBOnOff(void);                                     // Disable default constructor
    CSettingTypeRDBOnOff(const CSettingTypeRDBOnOff&);              // Disable copy constructor
    CSettingTypeRDBOnOff& operator=(const CSettingTypeRDBOnOff&);   // Disable assignment
};
