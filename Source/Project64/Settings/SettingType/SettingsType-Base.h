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

#include "../Settings.h"

enum SettingType {
    SettingType_Unknown           = -1,
    SettingType_ConstString       = 0,
    SettingType_ConstValue        = 1,
    SettingType_CfgFile           = 2,
    SettingType_Registry          = 3,
    SettingType_RelativePath      = 4,
    TemporarySetting              = 5,
    SettingType_RomDatabase       = 6,
    SettingType_CheatSetting      = 7,
    SettingType_GameSetting       = 8,
    SettingType_BoolVariable      = 9,
    SettingType_NumberVariable    = 10,
    SettingType_StringVariable    = 11,
    SettingType_SelectedDirectory = 12,
    SettingType_RdbSetting        = 13,
};

class CSettingType
{
public:
    virtual ~CSettingType() {};

    virtual SettingType GetSettingType    ( void ) const = 0;
    virtual bool        IndexBasedSetting ( void ) const = 0;

    //return the values
    virtual bool Load ( int Index, bool & Value ) const = 0;
    virtual bool Load ( int Index, uint32_t & Value ) const = 0;
    virtual bool Load ( int Index, stdstr & Value ) const = 0;

    //return the default values
    virtual void LoadDefault ( int Index, bool & Value ) const = 0;
    virtual void LoadDefault ( int Index, uint32_t & Value ) const = 0;
    virtual void LoadDefault ( int Index, stdstr & Value ) const = 0;

    //Update the settings
    virtual void Save ( int Index, bool Value ) = 0;
    virtual void Save ( int Index, uint32_t Value ) = 0;
    virtual void Save ( int Index, const stdstr & Value ) = 0;
    virtual void Save ( int Index, const char * Value ) = 0;

    // Delete the setting
    virtual void Delete ( int Index ) = 0;
};
