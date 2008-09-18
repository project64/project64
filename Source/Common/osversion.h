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
//     $Date: 5/13/98 12:03a $
//  $Modtime: 4/27/98 6:50a $
// $Revision: 17 $
//  $Archive: /Interscope/Thebe/InstallMaster/OSVersion.Hpp $
// $Workfile: OSVersion.Hpp $
//-----------------------------------------------------------------------------

#ifndef __OperatingSystemVersion_Hpp__
#define __OperatingSystemVersion_Hpp__

// Define the following symbol if compiling using precompiled headers through 
// header file StdAfx.H
// #define __STDAFX__
//
// Define the following symbol if used in a MFC project

#if !defined(__WIN32__) && !defined(_WIN32)
#ifndef STRICT
#define STRICT
#endif
#endif // __WIN32__

#if defined(__WIN32__) || defined(_WIN32)
#include <WinVer.H>
#else
#include <Ver.H>
#endif


//--- Extended OS and Win32 (tm) version types --------------------------------

#define OS_UNKNOWN      0x0000
#define OS_WIN3X        0x0001
#define OS_WFW          0x0002
#define OS_WIN95        0x0004
#define OS_WIN98        0x0008
#define OS_WINME        0x0010
#define OS_WINNT351		0x0020
#define OS_WINNT40		0x0040
#define OS_WINNTWS      0x0080  // Workstation
#define OS_WINNTS       0x0100  // Server
#define OS_WINNTAS      0x0200  // Advanced server (only if older than 4.0)
#define OS_WIN2000		0x0400
#define OS_WINXP		0x0800
#define OS_WINNET		0x1000
#define OS_WINNT        (OS_WINNT351 | OS_WINNTWS | OS_WINNTS | OS_WINNTAS | OS_WIN2000 | OS_WINXP | OS_WINNET)
#define OS_WINCE		0x2000

#define WIN_UNKNOWN     0x0000
#define WIN_16          0x0100
#define WIN_32          0x0200
#define WIN_32S         0x0400
#define WIN_32C         0x0800


//--- Windows 4 Workgroups detection methods --------------------------------

#define METHOD_MULTINET   1
#define METHOD_FILEVERSION 2


//--- OS version checker class ----------------------------------------------

class COSVersion
{
// Type(s)
protected:
    struct VS_VERSION 
    {
        WORD wTotLen;
        WORD wValLen;
        char szSig[16];
        VS_FIXEDFILEINFO vffInfo;
    };

// Data members
private:
    WORD  m_nOSType;
    WORD  m_nWinType;
    DWORD m_dwVersion;

    #if defined(__WIN32__) || defined(_WIN32)
    OSVERSIONINFO osvi;
    #endif

// Construction
public:
    COSVersion();

// Implementation
public:
    WORD  GetOSType()       const { return m_nOSType;  }  // Returns one of the OS_ constants
    WORD  GetWindowsType()  const { return m_nWinType; }  // Returns one of the WIN_ constants
    DWORD GetMajorVersion() const;
    DWORD GetMinorVersion() const;

    #if defined(__WIN32__) || defined(_WIN32)
    DWORD   GetBuildNumber()	const;
	LPCTSTR GetSpecialVersion() const;
    #endif

// Helper(s)
private:
    BOOL IsWindows4Workgroups(unsigned short usMethod);
};


#endif

