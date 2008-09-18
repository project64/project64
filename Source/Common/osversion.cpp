//---------------------------------------------------------------------------
// Copyright (C) 1998, Interscope Ltd. All rights reserved.
// Reproduction or distribution of this program, or any portion of it, 
// is permitted only if this header is kept as it is.
// For more information, contact:
//
// Interscope Ltd., 5 Culturii St., 5th Floor, 4800 Baia Mare, RO
//    Phone/Fax: +40-62-215023
//    E-mail: office@interscope.ro
//
//   $Author: Levente Farkas $
//     $Date: 5/12/98 11:50p $
//  $Modtime: 4/30/98 8:22a $
// $Revision: 23 $
//  $Archive: /Interscope/Thebe/InstallMaster/OSVersion.cpp $
// $Workfile: OSVersion.cpp $
//---------------------------------------------------------------

// 4 more details about API calls in this module, see
// Knowledge Base Atricle Q113998 and
// Knowledge Base Atricle Q114470 on
// Microsoft Development Library (October 1995)

//Ads - This is very out of date!
//		Rewrote the O/S detection in the constructor 15/3/2002

#include "stdafx.h"

#pragma comment(lib, "version.lib")


//--- Some types and consts -----------------------------------

#define WNNC_NET_MULTINET        0x8000
#define WNNC_SUBNET_WINWORKGROUP 0x0004
#define WNNC_NET_TYPE            0x0002

typedef WORD (WINAPI *NETCAPFUNC)(int);

//-------------------------------------------------------------
// Task    : Constructs and OS version class and extracts all the
//           version information necessary
//-------------------------------------------------------------
COSVersion::COSVersion():
            m_nOSType(OS_UNKNOWN),
            m_nWinType(WIN_UNKNOWN),
            m_dwVersion(GetVersion())
{
#if defined(__WIN32__) || defined(_WIN32) // 32-bit platform

	osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

	//Determine which versions of the O/S we are running
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32s)  //Win32s on windows 3.1
	{
        m_nWinType = WIN_32S;

        // Determine if Win 3.X or WFW
        if(IsWindows4Workgroups(METHOD_FILEVERSION)) 
            m_nOSType = OS_WFW;
        else if(IsWindows4Workgroups(METHOD_MULTINET)) 
            m_nOSType = OS_WFW;
        else 
            m_nOSType = OS_WIN3X;
	}
	else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) //Win 95, 98 or ME
	{
        m_nWinType = WIN_32C;

        // Determine if Win95 or Win98 or WinMe
        if(osvi.dwMinorVersion == 0)
            m_nOSType = OS_WIN95;
        else if (osvi.dwMinorVersion == 10)
            m_nOSType = OS_WIN98;
        else if (osvi.dwMinorVersion == 90)
            m_nOSType = OS_WINME;
		else
			m_nOSType = OS_UNKNOWN;
	}
	else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) //Win NT 3.51, 4.0, 2000, XP or .Net Server
	{
        m_nWinType = WIN_32;

		//Determine which version
		if (osvi.dwMajorVersion == 3)
		{
			m_nOSType = OS_WINNT351;
		}
		else if (osvi.dwMajorVersion == 4)
		{
			m_nOSType = OS_WINNT40;
		} 
		if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 0)
			{
				m_nOSType = OS_WIN2000;
			}
			else
			{
				m_nOSType = OS_WINXP | OS_WINNET;
			}
		}
		else
		{
			m_nOSType = OS_WINNT; //Future proof
		}
	}
	/*
	else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_CE) //Windows CE
	{
		m_nOSType = OS_WINCE;
	}
	*/

	/* Original code
	// Test 4 NT/NTS/NTAS by the highest bit of the version
    if(m_dwVersion & 0x80000000)
    {
        // This is not NT
        // Check major version number 2 distinguish between
        // WIN32s (Win 3.X) and WIN32c (Win 95)
        if(LOBYTE(LOWORD(m_dwVersion)) <= 3)
        {
            m_nWinType =WIN_32S;

            // Determine if Win 3.X or WFW
            if(IsWindows4Workgroups(METHOD_FILEVERSION)) 
                m_nOSType =OS_WFW;
            else if(IsWindows4Workgroups(METHOD_MULTINET)) 
                m_nOSType =OS_WFW;
            else 
                m_nOSType =OS_WIN3X;
        }
        else
        {
            m_nWinType =WIN_32C;

            // Determine if Win95 or Win98 or WinMe
            // Note: We consider Win98 from beta 2 up (4.10.1650)
            ASSERT(osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
            if(osvi.dwMinorVersion == 0)
                m_nOSType =OS_WIN95;
            else if (osvi.dwMinorVersion == 10)
                m_nOSType =OS_WIN98;
            else if (osvi.dwMinorVersion == 90)
                m_nOSType =OS_WINME;

        }
    }
    else
    {
        // Must be NTWS, NTS or NTAS
        // Check the registry 2 distinguish between them
        HKEY  hKey;
        BYTE  szValue[128];
        DWORD dwSize =sizeof(szValue);
        RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                     0,KEY_READ,&hKey);
        RegQueryValueEx(hKey,_T("ProductType"),0,NULL,szValue,&dwSize);
        RegCloseKey(hKey);
        if(!STRICMP((TCHAR *)&szValue[0],_T("WinNT"))) 
            // WinNT (Windows NT Workstation)
            m_nOSType =OS_WINNTWS;
        else if(!STRICMP((TCHAR *)&szValue[0],_T("ServerNT"))) 
            // ServerNT (Windows NT Server)
            m_nOSType =OS_WINNTS;
        else 
            // LanmanNT (Windows NT Advanced Server)
            m_nOSType =OS_WINNTAS;

        m_nWinType =WIN_32;
    }
	*/

#else // 16-bit platform

    // For 16-bit code, use GetWinFlags 2 find out if running on the
    // Windows on Windows layer of NT/NTAS
    if(GetWinFlags() & 0x4000) 
        m_nOSType =OS_WINNTWS;
    else
    {
        // Check version number 2 distinguish between Win 3.X and Win 95
        if((LOBYTE(LOWORD(dwVersion)) > 3) ||
           (HIBYTE(LOWORD(dwVersion)) > 50)) 
            // Windows 95
            m_nOSType =OS_WIN95;
        else
        {
           // Determine if Win 3.X or WFW
           if(IsWindows4Workgroups(MEHOD_FILEVERSION)) 
               m_nOSType =OS_WFW;
           else if(IsWindows4Workgroups(METHOD_MULTINET)) 
               m_nOSType =OS_WFW;
           else 
               m_nOSType =OS_WIN3X;
        }
    }
    m_nWinType =WIN_16;

#endif
}

//-------------------------------------------------------------
// Pre     : Check method
// Task    : Checks if running on Windows 4 Workgroups
//-------------------------------------------------------------
BOOL COSVersion::IsWindows4Workgroups(unsigned short usMethod) {
    WORD       wNetType;
    DWORD      dwVerSize, dwVerHandle;
    HANDLE     hMem;
    VS_VERSION FAR *lpVerInfo;
    BOOL       bWfW =FALSE;

    if(usMethod == METHOD_MULTINET)
    {
        // Use the method that checks 4 multinet driver
        HINSTANCE hLib =LoadLibrary((LPTSTR)"USER.EXE");
        if((WORD)hLib >= (WORD)HINSTANCE_ERROR)
        {
            // Check if library loaded OK
			//NETCAPFUNC lpWNetGetCaps =(NETCAPFUNC)GetProcAddress(hLib,_T("WNetGetCaps"));
			NETCAPFUNC lpWNetGetCaps =(NETCAPFUNC)GetProcAddress(hLib,"WNetGetCaps");
            if(lpWNetGetCaps != NULL)
            {
                wNetType =(*lpWNetGetCaps)(WNNC_NET_TYPE);
                if((wNetType & WNNC_NET_MULTINET) && 
                   (LOBYTE(wNetType) & WNNC_SUBNET_WINWORKGROUP)) 
                    // Yes, it is Windows 4 Workgroups
                    bWfW =TRUE;
            }

            if(hLib) 
                FreeLibrary(hLib);
        }
    }
    else
    {
        // Use the method that checks the fileversion of USER.EXE
        // Allocate memory 4 the file info struct
        dwVerSize =GetFileVersionInfoSize(_T("USER.EXE"),&dwVerHandle);
        hMem =GlobalAlloc(GMEM_MOVEABLE,dwVerSize);
        if(hMem != NULL)
        {
            lpVerInfo =(VS_VERSION FAR *)GlobalLock(hMem);
            // Get the file version
            // in Win32, the dwVerHandle is zero, ignored
            if(GetFileVersionInfo(_T("USER.EXE"),dwVerHandle,dwVerSize,lpVerInfo))
            if((HIWORD(lpVerInfo->vffInfo.dwProductVersionMS) == 3) &&
               (LOWORD(lpVerInfo->vffInfo.dwProductVersionMS) == 11)) 
                // Yes, it is Windows 4 Workgroups
                bWfW =TRUE;
            GlobalUnlock(hMem);
            GlobalFree(hMem);
        }
    }

    return bWfW;
}

//-------------------------------------------------------------
// Task    : Get OS major version number
//-------------------------------------------------------------
DWORD COSVersion::GetMajorVersion() const
{
#if defined(__WIN32__) || defined(_WIN32)
    return osvi.dwMajorVersion;
#else
    return (DWORD)(LOBYTE(LOWORD(dwVersion)));
#endif
}

//-------------------------------------------------------------
// Task    : Get OS minor version number
//-------------------------------------------------------------
DWORD COSVersion::GetMinorVersion() const
{
#if defined(__WIN32__) || defined(_WIN32)
    return osvi.dwMinorVersion;
#else
    return (DWORD)(HIBYTE(LOWORD(dwVersion)));        
#endif
}

#if defined(__WIN32__) || defined(_WIN32)
//-------------------------------------------------------------
// Task    : Get the build number of the OS
//-------------------------------------------------------------
DWORD COSVersion::GetBuildNumber() const
{
    return (m_nOSType & OS_WINNT) ? osvi.dwBuildNumber : LOWORD(osvi.dwBuildNumber);
}
#endif

#if defined(__WIN32__) || defined(_WIN32)
//-------------------------------------------------------------
// Task    : Get the special version data (string got from GetVersionEX)
//-------------------------------------------------------------
LPCTSTR COSVersion::GetSpecialVersion() const
{
	return osvi.szCSDVersion;
}
#endif

