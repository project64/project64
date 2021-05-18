/*
N-Rage`s Dinput8 Plugin
(C) 2002, 2006  Norbert Wladyka

Author`s Email: norbert.wladyka@chello.at
Website: http://go.to/nrage

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Internationalization routines go in this file

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <tchar.h>

#include "International.h"
#include "Debug.h"

LANGID GetNTDLLNativeLangID();
BOOL IsHongKongVersion();
BOOL CALLBACK EnumLangProc(HANDLE hModule, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIDLanguage, LONG_PTR lParam);

// The following routines are ripped straight from the SatDLL sample project on the Visual Studio .NET CDs.
// Props to the MS coders for making this solid piece of work (comment by rabid)
// "If it ain't broke, don't fix it."

// Loads the satellite DLL specified for the language DesiredLanguage
HMODULE     LoadLanguageDLL(LANGID DesiredLanguage)
{
    TCHAR       SatellitePath[MAX_PATH];
    HMODULE     hDLL;

    // First try to load the library with the fully specified language
    _stprintf(SatellitePath, _T("NRage-Language-%u.dll"), DesiredLanguage);
    hDLL = LoadLibraryEx(SatellitePath, 0, 0);
    if( hDLL )
        return hDLL;
    else
    {   // Try the primary language ID
        DesiredLanguage = PRIMARYLANGID(DesiredLanguage);
        _stprintf(SatellitePath, _T("NRage-Language-%u.dll"), DesiredLanguage);
        hDLL = LoadLibraryEx(SatellitePath, 0, 0);
        if( hDLL )
            return hDLL;
        else
        {
            DebugWrite(_T("Couldn't load library: %s\n"), SatellitePath);
            return NULL;
        }
    }
}

// The following functions contain code to
// detect the language in which the initial
// user interface should be displayed

BOOL CALLBACK EnumLangProc(HANDLE hModule, LPCTSTR lpszType, LPCTSTR lpszName,
                           WORD wIDLanguage, LONG_PTR lParam)
{
    PLANGINFO LangInfo;

    LangInfo = (PLANGINFO) lParam;
    LangInfo->Count++;
    LangInfo->LangID  = wIDLanguage;

    return (TRUE);        // Continue enumeration
}

// Detects the language of ntdll.dll with some specific processing for
// the Hong Kong SAR version
LANGID GetNTDLLNativeLangID()
{
    LANGINFO LangInfo;
    LPCTSTR Type = (LPCTSTR) ((LPVOID)((WORD)16));
    LPCTSTR Name = (LPCTSTR) 1;

    ZeroMemory(&LangInfo,sizeof(LangInfo));

    // Get the HModule for ntdll
    HMODULE hMod = GetModuleHandle(_T("ntdll.dll"));
    if (hMod==NULL)
    {
        return(0);
    }

    BOOL result = EnumResourceLanguages(hMod, Type, Name, (ENUMRESLANGPROC)EnumLangProc, (LONG_PTR) &LangInfo);

    if (!result || (LangInfo.Count > 2) || (LangInfo.Count < 1) )
    {
        return (0);
    }
    return (LangInfo.LangID);
}

// Checks if NT4 system is Hong Kong SAR version
BOOL IsHongKongVersion()
{
    HMODULE hMod;
    BOOL bRet=FALSE;
    typedef BOOL (WINAPI *IMMRELEASECONTEXT)(HWND,HIMC);
    IMMRELEASECONTEXT pImmReleaseContext;

    hMod = LoadLibrary(_T("imm32.dll"));
    if (hMod)
    {
        pImmReleaseContext = (IMMRELEASECONTEXT)GetProcAddress(hMod,"ImmReleaseContext");
        if (pImmReleaseContext) {
            bRet = pImmReleaseContext(NULL,0);
        }
        FreeLibrary(hMod);
    }
    return (bRet);
}

// This function detects a correct initial UI language for all
// platforms (Win9x, ME, NT4, Windows 2000, Windows XP)
LANGID DetectLanguage()
{

#define MAX_KEY_BUFFER  80

    OSVERSIONINFO       VersionInfo;
    LANGID              uiLangID = 0;
    HKEY                hKey;
    DWORD               Type, BuffLen = MAX_KEY_BUFFER;
    TCHAR               LangKeyValue[MAX_KEY_BUFFER];

    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( !GetVersionEx(&VersionInfo) )
        return(0);

    switch( VersionInfo.dwPlatformId )
    {
        // On Windows NT, Windows 2000, or higher
        case VER_PLATFORM_WIN32_NT:
            if( VersionInfo.dwMajorVersion >= 5)   // Windows 2000 or higher
            {
                // We need to dynamically link the GetUserDefaultUILanguage function
                HMODULE hmKernDLL = LoadLibrary(_T("kernel32.dll"));
                if (hmKernDLL)
                {
                    LANGID (*fpGetLang)() = NULL;
                    fpGetLang = (LANGID(*)(void))GetProcAddress(hmKernDLL, "GetUserDefaultUILanguage");
                    uiLangID = fpGetLang();
                } // And if we couldn't load kernel32.dll, just fall back to default language
            }
            else
            {   // For NT4 check the language of ntdll.dll
                uiLangID = GetNTDLLNativeLangID();
                if (uiLangID == 1033)
                {       // Special processing for Hong Kong SAR version of NT4
                    if (IsHongKongVersion())
                    {
                        uiLangID = 3076;
                    }
                }
            }
            break;
        // On Windows 95, Windows 98, or Windows ME
        case VER_PLATFORM_WIN32_WINDOWS:
            // Open the registry key for the UI language
            if( RegOpenKeyEx(HKEY_CURRENT_USER,_T("Default\\Control Panel\\Desktop\\ResourceLocale"), 0,
                KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS )
            {
                // Get the type of the default key
                if( RegQueryValueEx(hKey, NULL, NULL, &Type, NULL, NULL) == ERROR_SUCCESS
                    && Type == REG_SZ )
                { // Read the key value
                    if( RegQueryValueEx(hKey, NULL, NULL, &Type, (LPBYTE)LangKeyValue, &BuffLen)
                        == ERROR_SUCCESS )
                    {
                        uiLangID = _ttoi(LangKeyValue);
                    }
                }
                RegCloseKey(hKey);
            }
            break;
    }

    if (uiLangID == 0)
    {
        uiLangID = GetUserDefaultLangID();
    }
    // Return the found language ID
    return (uiLangID);
}
