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

#pragma warning(disable:4786)
#include <string>   //stl string
#include <map>      //stl map
#include <list>     //stl list
#include <Common/stdtypes.h>

typedef std::map<int32_t, std::string, std::less<int32_t> > LANG_STRINGS;
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

    const std::string & GetString(LanguageStringID StringID);
    LanguageList & GetLangList(void);
    void SetLanguage(const char * LanguageName);
    bool LoadCurrentStrings(void);
    bool IsCurrentLang(LanguageFile & File);
    bool IsLanguageLoaded(void) const { return m_LanguageLoaded; }

private:
    CLanguage(const CLanguage&);				// Disable copy constructor
    CLanguage& operator=(const CLanguage&);		// Disable assignment

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

const std::wstring wGS(LanguageStringID StringID);

inline const char * GS(LanguageStringID StringID)
{
    return g_Lang->GetString(StringID).c_str();
}
