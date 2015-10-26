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

class CSettingTypeApplicationIndex :
    public CSettingTypeApplication
{
public:
    CSettingTypeApplicationIndex(const char * Section, const char * Name, const char * DefaultValue );
    CSettingTypeApplicationIndex(const char * Section, const char * Name, bool DefaultValue );
    CSettingTypeApplicationIndex(const char * Section, const char * Name, uint32_t DefaultValue );
    CSettingTypeApplicationIndex(const char * Section, const char * Name, SettingID DefaultSetting );
    ~CSettingTypeApplicationIndex();

    virtual bool IndexBasedSetting ( void ) const { return true; }

    //return the values
    virtual bool Load ( int Index, bool & Value   ) const;
    virtual bool Load ( int Index, uint32_t & Value  ) const;
    virtual bool Load ( int Index, stdstr & Value ) const;

    //return the default values
    virtual void LoadDefault ( int Index, bool & Value   ) const;
    virtual void LoadDefault ( int Index, uint32_t & Value  ) const;
    virtual void LoadDefault ( int Index, stdstr & Value ) const;

    //Update the settings
    virtual void Save ( int Index, bool Value );
    virtual void Save ( int Index, uint32_t Value );
    virtual void Save ( int Index, const stdstr & Value );
    virtual void Save ( int Index, const char * Value );

    // Delete the setting
    virtual void Delete ( int Index );

private:
    CSettingTypeApplicationIndex(void);                                             // Disable default constructor
    CSettingTypeApplicationIndex(const CSettingTypeApplicationIndex&);              // Disable copy constructor
    CSettingTypeApplicationIndex& operator=(const CSettingTypeApplicationIndex&);   // Disable assignment
};
