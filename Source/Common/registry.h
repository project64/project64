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
// $Revision: 30 $
//  $Archive: /Interscope/Thebe/InstallMaster/Registry.Hpp $
// $Workfile: Registry.Hpp $
//-----------------------------------------------------------------------

#ifndef __Registry_Hpp__
#define __Registry_Hpp__

#if !defined(__WIN32__) && !defined(_WIN32)
#ifndef STRICT
#define STRICT
#endif
#endif // __WIN32__

#include <Windows.H>
#include <WinReg.H>
#include <RegStr.H>
#include <WinError.H>
#include <TChar.H>
#include "std string.h"

//--- Registry key handler ------------------------------------------

class CRegistry
{
private:
    HKEY  m_hKey;
    BOOL  m_bStatus;
    LONG  m_nErrorCode;

#ifdef _DEBUG 
    HKEY  m_hKeyParent;
    TCHAR m_hKeyPath[MAX_PATH];
#endif

public:
    // Construction
    CRegistry();
    CRegistry(HKEY    hKey,         // A previously open key or a section
              LPCTSTR lpcszSubKey,  // Path relative 2 hKey
              REGSAM  dwDesiredSecurityAccessMask        =KEY_ALL_ACCESS,          // Desired open/create access
              BOOL    bAllowCreate                       =FALSE,                   // Create new, if does not exist
              DWORD   dwOptions                          =REG_OPTION_NON_VOLATILE, // New key options
              LPSECURITY_ATTRIBUTES lpSecurityAttributes =NULL,                    // New key security
              LPDWORD lpdwDisposition                    =NULL);                   // Cretion disposition (created or opened)
    
    BOOL Open(HKEY    hKey,         // A previously open key or a section
              LPCTSTR lpcszSubKey,  // Path relative 2 hKey
              REGSAM  dwDesiredSecurityAccessMask        =KEY_ALL_ACCESS,          // Desired open/create access
              BOOL    bAllowCreate                       =FALSE,                   // Create new, if does not exist
              DWORD   dwOptions                          =REG_OPTION_NON_VOLATILE, // New key options
              LPSECURITY_ATTRIBUTES lpSecurityAttributes =NULL,                    // New key security
              LPDWORD lpdwDisposition                    =NULL);                   // Cretion disposition (created or opened)

    // Value functions
    DWORD  GetValueSize(LPCTSTR name);
    BOOL   GetValue(LPCTSTR name,       BYTE *data, DWORD &data_size, DWORD *type=NULL);
    BOOL   SetValue(LPCTSTR name, const BYTE *data, DWORD  data_size, DWORD  type);
    BOOL   GetDefaultValue(LPTSTR  def_value_buff, DWORD &buffsize);
    BOOL   SetDefaultValue(LPCTSTR def_value);
    BOOL   DeleteValue(LPCTSTR name);
	stdstr GetString(LPCTSTR name);

    // Enumerators
    BOOL  EnumerateSubKeys(DWORD index, LPTSTR name, DWORD &name_size);
    BOOL  EnumerateValues (DWORD index, LPTSTR name, DWORD &name_size, DWORD *type=NULL, BYTE *data=NULL, DWORD *data_size=NULL);

    // Conversions
    operator HKEY() const { return m_bStatus ? m_hKey : NULL; }

    // Remote registry functions
    BOOL  Connect_HKEY_LOCAL_MACHINE(LPCTSTR machine);
    BOOL  Connect_HKEY_USERS(LPCTSTR machine);

    // Miscellaneous
    LONG  GetLastError() const { return m_nErrorCode; } // Retruns ERROR_SUCCESS when inited OK and everything is fine
    DWORD CountSubKeys();
    DWORD CountValues();
    BOOL  GetInfo(LPTSTR  lpszKeyClass =NULL, LPDWORD lpcKeyClassSize =NULL, // Must supply non-NULL values 4 at least
                  LPDWORD lpcSubKeyCount         =NULL,                      // parameters from one line
                  LPDWORD lpcbLongestSubKeyName  =NULL,
                  LPDWORD lpcbLargestSubKeyClass =NULL,
                  LPDWORD lpcValueCount          =NULL,
                  LPDWORD lpcbLongestValueName   =NULL,
                  LPDWORD lpcbLargestValueData   =NULL,
                  LPDWORD lpcbSecurityDescriptorSize =NULL,
                  PFILETIME lpLastWriteTime =NULL); // NT only

    // Cleanup
    BOOL Delete(BOOL bDeleteSubkeys =TRUE);
    BOOL Flush();
    void Close();
    virtual ~CRegistry();
};


#endif

