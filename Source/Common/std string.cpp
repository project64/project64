#include "stdafx.h"

void stdstr::ArgFormat(const TCHAR * strFormat, va_list & args)
{
	if (strFormat != NULL) {
		// declare buffer (default max buffer size = 32k)
		const int nMaxSize = 32*1024;
		TCHAR pBuffer[nMaxSize];
		
		_vsntprintf(pBuffer,nMaxSize - 1,strFormat,args);
		pBuffer[nMaxSize - 1] = 0;

		tstring * _this = this;
		*_this = pBuffer;
	} else {
		*((tstring *)this) = _T("");
	}
}

stdstr::stdstr()
{

}

stdstr::stdstr( const tstring & str ) 
{ 
	*((tstring *)this) = str; 
}

stdstr::stdstr( const stdstr & str ) 
{
	*((tstring *)this) = (tstring)str; 
}

stdstr::stdstr( const TCHAR * str ) 
{
	*((tstring *)this) = str ? str : ""; 
}

void stdstr::Format(const TCHAR * strFormat, ...)
{
	va_list args;
	va_start(args, strFormat);
	ArgFormat(strFormat,args);
	va_end(args);
}

void stdstr::replace(const TCHAR search, const TCHAR replace )
{
	tstring& str = *this;
	tstring::size_type pos = str.find( search );
	while ( pos != tstring::npos )
	{
		str.replace( pos, 1, &replace );
		pos = str.find( search, pos + 1 );
	}
}

void stdstr::replace(const TCHAR * search, const TCHAR replace )
{
	tstring& str = *this;
	tstring::size_type pos = str.find( search );
	size_t SearchSize = _tcslen(search);
	while ( pos != tstring::npos )
	{
		str.replace( pos, SearchSize, &replace );
		pos = str.find( search, pos + 1 );
	}
}

void stdstr::replace(const tstring& search, const tstring& replace )
{
	tstring& str = *this;
	tstring::size_type pos = str.find( search );
	size_t SearchSize = search.size();
	while ( pos != tstring::npos )
	{
		str.replace( pos, SearchSize, replace );
		pos = str.find( search, pos + replace.length() );
	}
}

stdstr& stdstr::Trim(bool StripEnter)
{
	tstring& str = *this;
	while ((str.size() && str[0] == ' ') || 
		(StripEnter && (str[0] == '\r' || str[0] == '\n')))
	{
		str = str.substr(1);
	}
		
	while (str.size() && (str[str.size()-1] == ' ' || 
		(StripEnter && (str[str.size()-1] == '\r' || str[str.size()-1] == '\n'))))
	{
		str = str.substr(0, str.size() - 1);
	}
	return *this;
}

stdstr& stdstr::ToLower(void)
{
	tstring& str = *this;
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) tolower);
	return *this;
}

stdstr& stdstr::ToUpper(void)
{
	tstring& str = *this;
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) toupper);
	return *this;
}

strvector stdstr::Tokenize(const stdstr& delimiters) const
{
	const stdstr& str = *this;

	strvector tokens;
    		
	// skip delimiters at beginning.
   		stdstr::size_type lastPos = str.find_first_not_of(delimiters, 0);
    		
	// find first "non-delimiter".
   		stdstr::size_type pos = str.find_first_of(delimiters, lastPos);

   		while (stdstr::npos != pos || stdstr::npos != lastPos)
   		{
       		// found a token, add it to the vector.
       		tokens.push_back(str.substr(lastPos, pos - lastPos));
			
       		// skip delimiters.  Note the "not_of"
       		lastPos = str.find_first_not_of(delimiters, pos);
			
      		// find next "non-delimiter"
       		pos = str.find_first_of(delimiters, lastPos);
   		}

	return tokens;
}

void stdstr::TrimLeft (const TCHAR * chars2remove)
{
	if (!empty())
	{
		tstring::size_type pos = find_first_not_of(chars2remove);
		if (pos != tstring::npos)
		{
			erase(0,pos);
		} else {
			erase(begin(), end()); // make empty
		}
	}
}

void stdstr::TrimRight (const TCHAR * chars2remove)
{
	if (!empty())
	{
		tstring::size_type pos = find_last_not_of(chars2remove);
		if (pos != tstring::npos)
		{
			erase(pos+1);
		} else {
			erase(begin(), end()); // make empty
		}
	}
}

stdstr stdstr::toTString(const char *pstrSource)
{
#ifndef _UNICODE
	return pstrSource;
#else
	stdstr strRet;
	// Allocate enough space for the UNICODE string
	int nNeeded = MultiByteToWideChar(CP_UTF8, 
		0, 
		pstrSource, 
		-1, //str is null terminated
		NULL, 
		0);
	if (nNeeded == 0)
	{
		return strRet;
	}

	wchar_t* lpStrW = new wchar_t[nNeeded + 1];
	ZeroMemory(lpStrW, (nNeeded + 1)*sizeof(wchar_t));

	// Convert the string from ansi to Unicode
	nNeeded= MultiByteToWideChar(CP_UTF8, 
		0, 
		pstrSource, 
		-1, //str is null terminated
		lpStrW, 
		nNeeded+1);
	if (!nNeeded)
	{
		delete [] lpStrW;
		return strRet;
	}
	strRet = lpStrW;
	delete [] lpStrW;

	return strRet;
#endif
}

std::string stdstr::fromTString(const stdstr &strSource)
{
#ifndef _UNICODE
	return strSource;
#else
	std::string strRet;
	// Get the size of the Unicode string 

	// Allocate enough space for the Ansi string
	int nNeeded = WideCharToMultiByte(CP_UTF8, 
		0, 
		strSource.c_str(), 
		-1, //str is null terminated
		NULL, 
		0, 
		NULL, 
		NULL);
	if (nNeeded == 0)
	{
		return strRet;
	}

	char* lpStrA = new char[nNeeded + 1];
	ZeroMemory(lpStrA, nNeeded + 1);

	// Convert the string from Unicode to ansi
	nNeeded= WideCharToMultiByte(CP_UTF8, 
		0, 
		strSource.c_str(), 
		-1, //str is null terminated
		lpStrA, 
		nNeeded+1, 
		NULL, 
		NULL);
	if (!nNeeded)
	{
		delete [] lpStrA;
		return strRet;
	}

	strRet = lpStrA;
	delete [] lpStrA;

	return strRet;
#endif
}
