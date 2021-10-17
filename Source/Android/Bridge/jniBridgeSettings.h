#pragma once
#include <Project64-core\Settings\SettingsID.h>
#include <string>
#include <map>

class CJniBridegSettings
{
    typedef std::map<std::string, SettingID> SettingNameList;

public:
    CJniBridegSettings();
    ~CJniBridegSettings();

    SettingID TranslateSettingID(const char * SettingName);
    static inline bool bCPURunning ( void) { return m_bCPURunning; }

private:
    SettingNameList m_SettingNameList;

    static void RefreshSettings (void *);

    static bool m_bCPURunning;

    static int m_RefCount;
};
