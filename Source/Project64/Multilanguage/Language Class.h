#ifdef __cplusplus

#include "..\support.h"

#pragma warning(disable:4786)
#include <string>   //stl string
#include <map>      //stl map
#include <list>     //stl list

typedef std::map<int, stdstr, std::less<int> > LANG_STRINGS;
typedef LANG_STRINGS::value_type               LANG_STR;

typedef struct {
	stdstr Filename;
	stdstr LanguageName;
} LanguageFile;

typedef std::list<LanguageFile>   LanguageList;

class CLanguage  {
	stdstr       m_SelectedLanguage;
	const stdstr m_emptyString;

	LANG_STRINGS CurrentStrings, DefaultStrings;
	LanguageList m_LanguageList;

	stdstr       GetLangString      ( const char * FileName, LanguageStringID ID );
	LANG_STR     GetNextLangString  ( void * OpenFile );
	void         LoadDefaultStrings ( void );

public:
	               CLanguage       ( );
	const stdstr & GetString       ( LanguageStringID StringID );
	LanguageList & GetLangList     ( void );
	void           SetLanguage     ( char * LanguageName );
	void           LoadCurrentStrings ( bool ShowSelectDialog );
	bool           IsCurrentLang   ( LanguageFile & File );
};

extern CLanguage * _Lang;

inline LPCSTR GS (LanguageStringID StringID)
{
	return _Lang->GetString(StringID).c_str();
}

#endif