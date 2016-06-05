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

class CSettingTypeApplicationPath :
    public CSettingTypeApplication
{
public:
    virtual ~CSettingTypeApplicationPath();

    CSettingTypeApplicationPath(const char * Section, const char * Name, SettingID DefaultSetting );
    bool IsSettingSet(void) const;

    //return the values
    virtual bool Load   ( int32_t Index, stdstr & Value ) const;

private:
    CSettingTypeApplicationPath(void);                                            // Disable default constructor
    CSettingTypeApplicationPath(const CSettingTypeApplicationPath&);              // Disable copy constructor
    CSettingTypeApplicationPath& operator=(const CSettingTypeApplicationPath&);   // Disable assignment

    CSettingTypeApplicationPath(const char * Section, const char * Name, const char * DefaultValue );
    CSettingTypeApplicationPath(const char * Section, const char * Name, bool DefaultValue );
    CSettingTypeApplicationPath(const char * Section, const char * Name, uint32_t DefaultValue );
};
