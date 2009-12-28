#include "stdafx.h"

#pragma comment(lib, "version.lib")

stdstr ReadVersionInfo( BYTE * Array, LPCTSTR Info )
{
	// Read the list of languages and code pages.  We will use only the first one.
	struct SLangCP { WORD wLang; WORD wCP; } * pTrans ;
	UINT nVersionLen = 0 ;

	BOOL  bRetCode = ::VerQueryValue( Array, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&pTrans, &nVersionLen );
	if (!bRetCode) 
	{
		WriteTrace(TraceError,_T("ReadVersionInfo(): VerQueryValue failed"));

		return _T("");
	}
    
    // Get the file version
	TCHAR szLookup[MAX_PATH] ;    
	_stprintf( szLookup, TEXT("\\StringFileInfo\\%04x%04x\\%s"),pTrans[0].wLang, pTrans[0].wCP,Info ) ;
	
    LPTSTR pszVersion = NULL ;
	 bRetCode = ::VerQueryValue( Array, szLookup, (LPVOID*)&pszVersion, &nVersionLen ) ;

	if (!bRetCode) 
	{
		WriteTrace(TraceError,_T("ReadVersionInfo(): VerQueryValue failed"));

		return _T("");
	}

	if (_tcscmp(Info,VERSION_PRODUCT_VERSION) == 0 || _tcscmp(Info,VERSION_FILE_VERSION) == 0) 
	{
		for (ULONG Pos = 0; Pos < _tcslen(pszVersion); Pos ++) {
			if (pszVersion[Pos] == ',') 
			{
				pszVersion[Pos] = '.';
			}
		}
	}
		
	return pszVersion;
}

bool HasFileVersionInfo( LPCTSTR /*Info*/, LPCTSTR FileName ) 
{
	DWORD dwHandle;
	DWORD Size = GetFileVersionInfoSize((LPTSTR)FileName,&dwHandle);
	if ( Size == 0) 
	{
		return false;
	}
	return true;
}

stdstr FileVersionInfo( LPCTSTR Info, LPCTSTR FileName ) 
{
	stdstr Result;

	if(FileName)
	{
		if(_tcslen(FileName) != 0)
		{
			DWORD dwHandle = 0;
			DWORD Size = GetFileVersionInfoSize((LPSTR)FileName,&dwHandle);
			if ( Size == 0) 
			{ 
				//WriteTraceF(TraceError,_T("FileVersionInfo(%s, %s): GetFileVersionInfoSize failed, error (%s)"),Info,FileName,BaseException::GetLastErrorDescription(GetLastError()).c_str());

				return TEXT(""); 
			}

			BYTE * Array = new BYTE [Size+(1*sizeof(TCHAR))];
			if(Array)
			{
				memset(Array, 0, Size+(1*sizeof(TCHAR)));

				try
				{
					if (!GetFileVersionInfo((LPSTR)FileName,0,Size,Array)) 
					{
						//WriteTraceF(TraceError,_T("FileVersionInfo(%s, %s): GetFileVersionInfo(%d) failed, error (%s)"),Info,FileName,Size,BaseException::GetLastErrorDescription(GetLastError()).c_str());

						delete [] Array;
                        return _T("");
					}
				}
				catch (...)
				{
					WriteTrace(TraceError,_T("Unhandled exception in FileVersionInfo (1)"));

					delete [] Array;
					return _T("");
				}

				try
				{
					Result = ReadVersionInfo(Array,Info);
					if(Result.empty())
					{
						WriteTraceF(TraceError,_T("FileVersionInfo(%s), ReadVersionInfo() failed"), FileName);
					}
				}
				catch (...)
				{
					WriteTrace(TraceError,_T("Unhandled exception in FileVersionInfo (2)"));
					delete [] Array;
					return _T("");
				}

				delete [] Array;
			}
			else
			{
				WriteTraceF(TraceError,_T("Failed to allocate %d bytes"), Size+(1*sizeof(TCHAR)));
			}
		}
		else
		{
			WriteTraceF(TraceError,_T("FileVersionInfo received a pointer to an empty string"));
		}
	}
	else
	{
		WriteTraceF(TraceError,_T("FileVersionInfo received NULL pointer"));
	}
	
	return Result;
}

stdstr VersionInfo(LPCTSTR Info, HMODULE hModule) 
{
	TCHAR FileName[MAX_PATH];
	if (GetModuleFileName(hModule,FileName,sizeof(FileName)) != 0)
	{
		return FileVersionInfo(Info,FileName);
	}
	return TEXT("");
}
