#pragma once

#pragma warning(disable : 4786)
#include <list>
#include <map>
#include <stdint.h>
#include <string>

typedef std::map<int32_t, std::string, std::less<int32_t>> LANG_STRINGS;
typedef LANG_STRINGS::value_type LANG_STR;

struct LanguageFile
{
    std::string Filename;
    std::string LanguageName;
};

typedef std::list<LanguageFile> LanguageList;

class CLanguage
{
public:
    CLanguage();
    ~CLanguage();

    const std::string & GetString(LanguageStringID StringID);
    LanguageList & GetLangList(void);
    void SetLanguage(const char * LanguageName);
    bool LoadCurrentStrings(void);
    bool IsCurrentLang(LanguageFile & File);
    bool IsLanguageLoaded(void) const
    {
        return m_LanguageLoaded;
    }

private:
    CLanguage(const CLanguage &);
    CLanguage & operator=(const CLanguage &);

    static void StaticResetStrings(CLanguage * _this)
    {
        _this->ResetStrings();
    }

    void ResetStrings(void);

    std::string m_SelectedLanguage;
    const std::string m_emptyString;

    LANG_STRINGS m_CurrentStrings, m_DefaultStrings;
    LanguageList m_LanguageList;

    std::string GetLangString(const char * FileName, LanguageStringID ID);
    LANG_STR GetNextLangString(void * OpenFile);
    void LoadDefaultStrings(void);

    bool m_LanguageLoaded;
};

extern CLanguage * g_Lang;

inline const char * GS(LanguageStringID StringID)
{
    return g_Lang->GetString(StringID).c_str();
}

#ifdef _WIN32
const std::wstring wGS(LanguageStringID StringID);
#endif
