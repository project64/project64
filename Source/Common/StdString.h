#pragma once

class stdstr;

#include <stdarg.h>
#include <vector>
#include <string>
#include <list>

typedef std::vector<stdstr> strvector;

class stdstr :
    public std::string
{
public:
    enum
    {
        CODEPAGE_UTF8 = 65001,
        CODEPAGE_932 = 932,
    };

    stdstr();
    stdstr(const std::string & str);
    stdstr(const stdstr & str);
    stdstr(const char * str);

    strvector  Tokenize(char delimiter) const;
    strvector  Tokenize(const char * delimiter) const;
    void       Format(const char * strFormat, ...);
    stdstr&    ToLower(void);
    stdstr&    ToUpper(void);

    void       Replace(const char search, const char replace);
    void       Replace(const char * search, const char replace);
    void       Replace(const std::string & search, const std::string & replace);

    stdstr   & Trim(const char * chars2remove = "\t ");
    stdstr   & TrimLeft(const char * chars2remove = "\t ");
    stdstr   & TrimRight(const char * chars2remove = "\t ");

#ifdef _WIN32
	stdstr   & FromUTF16(const wchar_t * UTF16Source, bool * bSuccess = nullptr);
    std::wstring ToUTF16(unsigned int CodePage = CODEPAGE_UTF8, bool * bSuccess = nullptr) const;
#endif

    void ArgFormat(const char * strFormat, va_list & args);
};

class stdstr_f : public stdstr
{
public:
	stdstr_f(const char * strFormat, ...);
};

#ifdef _WIN32
class stdwstr_f : public std::wstring
{
public:
	stdwstr_f(const wchar_t * strFormat, ... );
};
#endif

typedef std::list<stdstr> strlist;
typedef strlist::iterator strlist_iter;
