#pragma once

#pragma warning(disable:4786)

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <tchar.h>
#include <stdarg.h>
#include <windef.h>

#ifdef _UNICODE	
typedef std::wstring tstring;
#else	
typedef std::string tstring;
#endif

class stdstr;

typedef std::vector<stdstr> strvector;

class stdstr: public tstring 
{
public:
	stdstr();
	stdstr( const tstring & str );
	stdstr( const stdstr & str );
	stdstr( const TCHAR * str );
//	stdstr(	const TCHAR * strBuff, size_t buffSize);

	void Format(const TCHAR * strFormat, ...);
	void ArgFormat(const TCHAR * strFormat, va_list & args);

	//stdstr& operator=(const TCHAR * rhs);
	void replace(const TCHAR search, const TCHAR replace );
	void replace(const TCHAR * search, const TCHAR replace );
	void replace(const tstring& search, const tstring& replace );
	stdstr& Trim(bool StripEnter = false);
	stdstr& ToLower(void);
	stdstr& ToUpper(void);
	strvector Tokenize(const stdstr& delimiters) const;
	void TrimLeft (const TCHAR * chars2remove = _T(" "));
	void TrimRight (const TCHAR * chars2remove = _T(" "));
	static stdstr toTString(const char *pstrSource);
	static std::string fromTString(const stdstr &strSource);
}; 

class stdstr_f: public stdstr 
{
public:
	stdstr_f(const TCHAR * strFormat, ...) 
	{ 
		va_list args;
		va_start(args, strFormat);
		ArgFormat(strFormat,args);
		va_end(args);
	}
};

typedef std::list<stdstr>   strlist;
typedef strlist::iterator   strlist_iter;
