#pragma once
#include <Project64-core/Settings/SettingType/SettingsType-Application.h>

class CSettingTypeGame :
    public CSettingTypeApplication
{
public:
    CSettingTypeGame(const char * Name, bool DefaultValue);
    CSettingTypeGame(const char * Name, const char * DefaultValue);
    CSettingTypeGame(const char * Name, uint32_t DefaultValue);
    CSettingTypeGame(const char * Name, SettingID DefaultSetting);
    virtual ~CSettingTypeGame();

    virtual bool IndexBasedSetting ( void ) const { return false; }
    virtual SettingType GetSettingType ( void ) const { return SettingType_GameSetting; }

    static void Initialize( void );
    static void CleanUp ( void );

    // Return the values
    virtual bool Load (uint32_t Index, bool & Value) const;
    virtual bool Load (uint32_t Index, uint32_t & Value) const;
    virtual bool Load (uint32_t Index, std::string & Value) const;

    // Return the default values
    virtual void LoadDefault (uint32_t Index, bool & Value) const;
    virtual void LoadDefault (uint32_t Index, uint32_t & Value) const;
    virtual void LoadDefault (uint32_t Index, std::string & Value) const;

    // Update the settings
    virtual void Save (uint32_t Index, bool Value);
    virtual void Save (uint32_t Index, uint32_t Value);
    virtual void Save (uint32_t Index, const std::string & Value);
    virtual void Save (uint32_t Index, const char * Value);

    // Delete the setting
    virtual void Delete (uint32_t Index);

protected:
    static bool m_RdbEditor;
    static bool m_EraseDefaults;
    static std::string * m_SectionIdent;

    static void UpdateSettings(void * /*Data */);

    virtual const char * SectionName(void) const;

private:
    CSettingTypeGame(void);
    CSettingTypeGame(const CSettingTypeGame&);
    CSettingTypeGame& operator=(const CSettingTypeGame&);
};
