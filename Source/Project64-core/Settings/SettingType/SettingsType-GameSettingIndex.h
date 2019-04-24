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

class CSettingTypeGameIndex :
    public CSettingTypeGame
{
public:
    CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, const char * DefaultValue );
    CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, uint32_t DefaultValue );
    CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, bool DefaultSetting);
    CSettingTypeGameIndex(const char * PreIndex, const char * PostIndex, SettingID DefaultSetting);
    ~CSettingTypeGameIndex();

    virtual bool IndexBasedSetting ( void ) const { return true; }
    virtual SettingType GetSettingType ( void ) const { return SettingType_GameSetting; }

    //return the values
    virtual bool Load ( uint32_t Index, bool & Value   ) const;
    virtual bool Load ( uint32_t Index, uint32_t & Value  ) const;
    virtual bool Load ( uint32_t Index, std::string & Value ) const;

    //return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value   ) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value  ) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value ) const;

    //Update the settings
    virtual void Save (uint32_t Index, bool Value );
    virtual void Save (uint32_t Index, uint32_t Value );
    virtual void Save (uint32_t Index, const std::string & Value );
    virtual void Save (uint32_t Index, const char * Value );

    // Delete the setting
    virtual void Delete (uint32_t Index );

private:
    CSettingTypeGameIndex(void);                                      // Disable default constructor
    CSettingTypeGameIndex(const CSettingTypeGameIndex&);              // Disable copy constructor
    CSettingTypeGameIndex& operator=(const CSettingTypeGameIndex&);   // Disable assignment

    std::string m_PreIndex, m_PostIndex;
};
