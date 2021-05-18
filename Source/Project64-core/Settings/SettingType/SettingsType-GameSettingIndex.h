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

    // Return the values
    virtual bool Load ( uint32_t Index, bool & Value   ) const;
    virtual bool Load ( uint32_t Index, uint32_t & Value  ) const;
    virtual bool Load ( uint32_t Index, std::string & Value ) const;

    // Return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value   ) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value  ) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value ) const;

    // Update the settings
    virtual void Save (uint32_t Index, bool Value );
    virtual void Save (uint32_t Index, uint32_t Value );
    virtual void Save (uint32_t Index, const std::string & Value );
    virtual void Save (uint32_t Index, const char * Value );

    // Delete the setting
    virtual void Delete (uint32_t Index );

private:
    CSettingTypeGameIndex(void);
    CSettingTypeGameIndex(const CSettingTypeGameIndex&);
    CSettingTypeGameIndex& operator=(const CSettingTypeGameIndex&);

    std::string m_PreIndex, m_PostIndex;
};
