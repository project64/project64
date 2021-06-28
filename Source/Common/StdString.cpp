#include "StdString.h"
#include "Platform.h"
#include <malloc.h>
#include <algorithm>
#ifdef _WIN32
#include <Windows.h>
#endif

stdstr::stdstr()
{
}

stdstr::stdstr(const std::string & str) :
    std::string(str)
{
}

stdstr::stdstr(const stdstr & str) :
    std::string((const std::string &)str)
{
}

stdstr::stdstr(const char * str) :
    std::string(str ? str : "")
{
}

strvector stdstr::Tokenize(const char * delimiter) const
{
    strvector tokens;

    stdstr::size_type lastPos = find_first_not_of(delimiter, 0);
    stdstr::size_type pos = find_first_of(delimiter, lastPos);
    size_t DelLen = strlen(delimiter);
    while (stdstr::npos != pos)
    {
        tokens.push_back(substr(lastPos, pos - lastPos));
        lastPos = pos + DelLen;
        pos = find_first_of(delimiter, lastPos);
    }
    if (stdstr::npos != lastPos)
    {
        tokens.push_back(substr(lastPos));
    }
    return tokens;
}

strvector stdstr::Tokenize(char delimiter) const
{
    strvector tokens;

    stdstr::size_type lastPos = find_first_not_of(delimiter, 0);
    stdstr::size_type pos = find_first_of(delimiter, lastPos);
    while (stdstr::npos != pos)
    {
        tokens.push_back(substr(lastPos, pos - lastPos));
        lastPos = pos + 1;
        pos = find_first_of(delimiter, lastPos);
    }
    if (stdstr::npos != lastPos)
    {
        tokens.push_back(substr(lastPos));
    }
    return tokens;
}

void stdstr::ArgFormat(const char * strFormat, va_list & args)
{
#pragma warning(push)
#pragma warning(disable:4996)

    size_t nlen = _vscprintf(strFormat, args) + 1;
    char * buffer = (char *)alloca(nlen * sizeof(char));
    buffer[nlen - 1] = 0;
    if (buffer != nullptr)
    {
        vsprintf(buffer, strFormat, args);
        *this = buffer;
    }
#pragma warning(pop)
}

void stdstr::Format(const char * strFormat, ...)
{
    va_list args;
    va_start(args, strFormat);
    ArgFormat(strFormat, args);
    va_end(args);
}

stdstr& stdstr::ToLower(void)
{
    std::transform(begin(), end(), begin(), (int(*)(int)) tolower);
    return *this;
}

stdstr& stdstr::ToUpper(void)
{
    std::transform(begin(), end(), begin(), (int(*)(int)) toupper);
    return *this;
}

void stdstr::Replace(const char search, const char replace)
{
    std::string& str = *this;
    std::string::size_type pos = str.find(search);
    while (pos != std::string::npos)
    {
        str.replace(pos, 1, &replace);
        pos = str.find(search, pos + 1);
    }
}

void stdstr::Replace(const char * search, const char replace)
{
    std::string& str = *this;
    std::string::size_type pos = str.find(search);
    size_t SearchSize = strlen(search);
    while (pos != std::string::npos)
    {
        str.replace(pos, SearchSize, &replace);
        pos = str.find(search, pos + 1);
    }
}

void stdstr::Replace(const std::string& search, const std::string& replace)
{
    std::string& str = *this;
    std::string::size_type pos = str.find(search);
    size_t SearchSize = search.size();
    while (pos != std::string::npos)
    {
        str.replace(pos, SearchSize, replace);
        pos = str.find(search, pos + replace.length());
    }
}

stdstr & stdstr::TrimLeft(const char * chars2remove)
{
    if (!empty())
    {
        std::string::size_type pos = find_first_not_of(chars2remove);
        if (pos != std::string::npos)
        {
            erase(0, pos);
        }
        else
        {
            erase(begin(), end()); // Make empty
        }
    }
    return *this;
}

stdstr & stdstr::TrimRight(const char * chars2remove)
{
    if (!empty())
    {
        std::string::size_type pos = find_last_not_of(chars2remove);
        if (pos != std::string::npos)
        {
            erase(pos + 1);
        }
        else
        {
            erase(begin(), end()); // Make empty
        }
    }
    return *this;
}

stdstr & stdstr::Trim(const char * chars2remove)
{
    if (!empty())
    {
        std::string::size_type pos = find_first_not_of(chars2remove);
        if (pos != std::string::npos)
        {
            erase(0, pos);
        }
        else
        {
            erase(begin(), end()); // Make empty
        }

        pos = find_last_not_of(chars2remove);
        if (pos != std::string::npos)
        {
            erase(pos + 1);
        }
        else
        {
            erase(begin(), end()); // Make empty
        }
    }
    return *this;
}

#ifdef _WIN32
stdstr & stdstr::FromUTF16(const wchar_t * UTF16Source, bool * bSuccess)
{
    bool bConverted = false;

    if (UTF16Source == nullptr)
    {
        *this = "";
        bConverted = true;
    }
    else if (wcslen(UTF16Source) > 0)
    {
        uint32_t nNeeded = WideCharToMultiByte(CODEPAGE_UTF8, 0, UTF16Source, -1, nullptr, 0, nullptr, nullptr);
        if (nNeeded > 0)
        {
            char * buf = (char *)alloca(nNeeded + 1);
            if (buf != nullptr)
            {
                memset(buf, 0, nNeeded + 1);

                nNeeded = WideCharToMultiByte(CODEPAGE_UTF8, 0, UTF16Source, -1, buf, nNeeded, nullptr, nullptr);
                if (nNeeded)
                {
                    *this = buf;
                    bConverted = true;
                }
            }
        }
    }
    if (bSuccess)
    {
        *bSuccess = bConverted;
    }
    return *this;
}

std::wstring stdstr::ToUTF16(unsigned int CodePage, bool * bSuccess) const
{
    bool bConverted = false;
    std::wstring res;

    DWORD nNeeded = MultiByteToWideChar(CodePage, 0, this->c_str(), (int)this->length(), nullptr, 0);
    if (nNeeded > 0)
    {
        wchar_t * buf = (wchar_t *)alloca((nNeeded + 1) * sizeof(wchar_t));
        if (buf != nullptr)
        {
            memset(buf, 0, (nNeeded + 1) * sizeof(wchar_t));

            nNeeded = MultiByteToWideChar(CodePage, 0, this->c_str(), (int)this->length(), buf, nNeeded);
            if (nNeeded)
            {
                res = buf;
                bConverted = true;
            }
        }
    }
    if (bSuccess)
    {
        *bSuccess = bConverted;
    }
    return res;
}
#endif

stdstr_f::stdstr_f(const char * strFormat, ...)
{
    va_list args;
    va_start(args, strFormat);
    ArgFormat(strFormat, args);
    va_end(args);
}

#ifdef _WIN32
stdwstr_f::stdwstr_f(const wchar_t * strFormat, ...)
{
    va_list args;
    va_start(args, strFormat);

    wchar_t Msg[1000];
    _vsnwprintf(Msg, sizeof(Msg) - 1, strFormat, args);

    va_end(args);

    this->assign(Msg);
}
#endif
