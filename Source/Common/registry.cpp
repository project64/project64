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
//  $Modtime: 4/27/98 6:50a $
// $Revision: 41 $
//  $Archive: /Interscope/Thebe/InstallMaster/Registry.cpp $
// $Workfile: Registry.cpp $
//-----------------------------------------------------------------------
#include "stdafx.h"

//--- Miscellaneous -----------------------------------------------------

const int MAX_REG_MACHINE_NAME_LEN  = 50;

//-----------------------------------------------------------------------
// Pre     :
// Post    : 
// Globals :
// I/O     :
// Task    : Create a new registry key object, butdo not attach it 2 any key yet
//-----------------------------------------------------------------------
CRegistry::CRegistry():
           m_hKey(NULL),
           m_bStatus(FALSE)
{
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Set the status 2 TRUE if opened OK
// Globals :
// I/O     :
// Task    : Open a registry key
//-----------------------------------------------------------------------
CRegistry::CRegistry(HKEY    hKey,         // A previously open key or a section
                     LPCTSTR lpcszSubKey,  // Path relative 2 hKey
                     REGSAM  dwDesiredSecurityAccessMask,        // Desired open/create access
                     BOOL    bAllowCreate,                       // Create new, if does not exist
                     DWORD   dwOptions,                          // New key options
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes, // New key security
                     LPDWORD lpdwDisposition):                   // Cretion disposition (created or opened)
           m_hKey(NULL),
           m_bStatus(FALSE)
{
    Open(hKey,lpcszSubKey,dwDesiredSecurityAccessMask,bAllowCreate,dwOptions,lpSecurityAttributes,lpdwDisposition);
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Set the status 2 TRUE if opened OK
// Globals :
// I/O     :
// Task    : Open a registry key
//-----------------------------------------------------------------------
BOOL CRegistry::Open(HKEY    hKey,         // A previously open key or a section
                     LPCTSTR lpcszSubKey,  // Path relative 2 hKey
                     REGSAM  dwDesiredSecurityAccessMask,        // Desired open/create access
                     BOOL    bAllowCreate,                       // Create new, if does not exist
                     DWORD   dwOptions,                          // New key options
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes, // New key security
                     LPDWORD lpdwDisposition)                    // Creation disposition (created or opened)
{
    // If already attached 2 a key, close it now
    Close();

#ifdef _DEBUG
    // Store the key's parent and the key's path
    m_hKeyParent =hKey;
    _tcsncpy(m_hKeyPath,lpcszSubKey,sizeof(m_hKeyPath)/sizeof(TCHAR));
#endif

    // Attempt 2 open specified key
    m_nErrorCode =RegOpenKeyEx(hKey,lpcszSubKey,0,dwDesiredSecurityAccessMask,&m_hKey);
    if(m_nErrorCode == ERROR_SUCCESS)
    {
        m_bStatus=TRUE;
        if(lpdwDisposition)
            *lpdwDisposition =REG_OPENED_EXISTING_KEY;
    }

    if(!m_bStatus && bAllowCreate)
    {
        // Could not open key, probably inexistent
        // Attempt 2 create it
        DWORD operation;
        m_nErrorCode =RegCreateKeyEx(hKey,lpcszSubKey,0,NULL,dwOptions,dwDesiredSecurityAccessMask,lpSecurityAttributes,&m_hKey,&operation);
        if(m_nErrorCode == ERROR_SUCCESS)
        {
            m_bStatus=TRUE;
            if(lpdwDisposition)
                *lpdwDisposition =operation;
        }
    }

    return m_bStatus;
}

//---------------------------------------------------------------------------
// Pre     : 
// Post    : Returns size in bytes, -1 on error
// Globals : 
// I/O     : 
// Task    : Get the size of the specified value
//---------------------------------------------------------------------------
DWORD CRegistry::GetValueSize(LPCTSTR name)
{
    if(!m_bStatus)
        return (DWORD)-1;

    DWORD value_size;
    if(!GetValue(name,NULL,value_size))
        return (DWORD)-1;

    return value_size;
}

//-----------------------------------------------------------------------
// Pre     : data_size must be initialized before this call 2 the size of 
//           the buffer where you expect data 2 be returned
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Extract a value from current key
//-----------------------------------------------------------------------
BOOL CRegistry::GetValue(LPCTSTR name, BYTE *data, DWORD &data_size, DWORD *type)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegQueryValueEx(m_hKey,name,NULL,type,data,&data_size);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     : buffsize must be initialized before this call 2 the size of 
//           the buffer where you expect data 2 be returned
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Extract default value from current key
//-----------------------------------------------------------------------
BOOL CRegistry::GetDefaultValue(LPTSTR def_value_buff, DWORD &buffsize)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegQueryValueEx(m_hKey,NULL,0,NULL,(BYTE *)def_value_buff,&buffsize);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Set a value of the current key
//-----------------------------------------------------------------------
BOOL CRegistry::SetValue(LPCTSTR name, const BYTE *data, DWORD data_size, DWORD type)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegSetValueEx(m_hKey,name,0,type,data,data_size);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Set default value of current key
//-----------------------------------------------------------------------
BOOL CRegistry::SetDefaultValue(LPCTSTR def_value)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegSetValueEx(m_hKey,NULL,0,REG_SZ,(BYTE *)def_value,lstrlen(def_value)+1);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Deletete specified value of current key
//-----------------------------------------------------------------------
BOOL CRegistry::DeleteValue(LPCTSTR name)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegDeleteValue(m_hKey,name);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     : data_size must be initialized before this call 2 the size of 
//           the buffer where you expect data 2 be returned
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Extract a value from current key
//-----------------------------------------------------------------------
/*BOOL CRegistry::GetValue(LPCTSTR name, BYTE *data, DWORD &data_size, DWORD *type)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegQueryValueEx(m_hKey,name,NULL,type,data,&data_size);
    return m_nErrorCode==ERROR_SUCCESS;
}*/
stdstr CRegistry::GetString(LPCTSTR name) {
	DWORD ValSize = GetValueSize(name);
	if (static_cast<int>(ValSize) <= 0) { return _T(""); }

	DWORD ValType;
	LPTSTR String = new TCHAR[ValSize];
	if (!GetValue(name,reinterpret_cast<BYTE *>(String),ValSize,&ValType)) {
		return stdstr(_T(""));
	}
	stdstr Result(String);
	delete [] String;

	return Result;
}
//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Enumerate subkeys of the current key
//           You can use this by calling CountSubKeys 2 get the number of 
//           subkeys, then calling this member with 0-based indexes of subkey
//           2 get info about
//-----------------------------------------------------------------------
BOOL CRegistry::EnumerateSubKeys(DWORD index, LPTSTR name, DWORD &name_size)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegEnumKeyEx(m_hKey,index,name,&name_size,NULL,NULL,0,NULL);
    return (m_nErrorCode==ERROR_SUCCESS);
}

//-----------------------------------------------------------------------
// Pre     : If a pointer 2 value data is specified (retrieving value data too)
//           then data_size must not be NULL and the pointed DWORD must hold
//           the size of the data buffer
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Enumerate values of current key
//           You can use this by calling CountValues 2 get the number of 
//           values, then calling this member with 0-based indexes of value
//           2 get info about
//-----------------------------------------------------------------------
BOOL CRegistry::EnumerateValues(DWORD index, LPTSTR name, DWORD &name_size, DWORD *type, BYTE *data, DWORD *data_size)
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegEnumValue(m_hKey,index,name,&name_size,NULL,type,data,data_size);
   return (m_nErrorCode==ERROR_SUCCESS);
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE if info was retrieved OK
// Globals :
// I/O     :
// Task    : Query info about registy key
//-----------------------------------------------------------------------
BOOL CRegistry::GetInfo(LPTSTR  lpszKeyClass, LPDWORD lpcKeyClassSize, // Non-NULL values should be supplied 4 at least
                        LPDWORD lpcSubKeyCount,                        // parameters from one line
                        LPDWORD lpcbLongestSubKeyName,
                        LPDWORD lpcbLargestSubKeyClass,
                        LPDWORD lpcValueCount,
                        LPDWORD lpcbLongestValueName,
                        LPDWORD lpcbLargestValueData,
                        LPDWORD lpcbSecurityDescriptorSize,
                        PFILETIME lpLastWriteTime) // NT only
{
#ifdef _WIN95
    if(lpLastWriteTime)
        ASSERT(FALSE); // This should be used only on NT, because on Windows 95 it gets filled with 0s
#endif

    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegQueryInfoKey(m_hKey,lpszKeyClass,lpcKeyClassSize,NULL,
                                 lpcSubKeyCount,lpcbLongestSubKeyName,lpcbLargestSubKeyClass,
                                 lpcValueCount,lpcbLongestValueName,lpcbLargestValueData,
                                 lpcbSecurityDescriptorSize,
                                 lpLastWriteTime);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : On error returns -1
// Globals :
// I/O     :
// Task    : Return the number of subkeys of this key
//-----------------------------------------------------------------------
DWORD CRegistry::CountSubKeys()
{
    DWORD subkeys;
    if(!GetInfo(NULL,NULL,&subkeys))
        return (DWORD)-1;
    return subkeys;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : On error returns FALSE
// Globals :
// I/O     :
// Task    : Remove this key and its descendants
//           Because on NT the key 2 be removed must not have subkeys, remove 
//           first all the subkeys
//-----------------------------------------------------------------------
BOOL CRegistry::Delete(BOOL bDeleteSubkeys)
{
    if(!m_bStatus)
        return FALSE;

    BOOL success =TRUE;
    if(bDeleteSubkeys)
    {
        // Attempt 2 delete all subkeys
        DWORD keys =CountSubKeys();
        while(keys>0)
        {
            TCHAR key_name[MAX_PATH];
            DWORD key_name_size =sizeof(key_name);

            // Get a subkey...
            if(EnumerateSubKeys(0,key_name,key_name_size))
            {
                // Got a subkey, now delete it
                CRegistry sub_key(*this,key_name);
                success =sub_key.Delete();
            }

            if(success)
                keys =CountSubKeys();
            else
                break;
        }
    }

    if(success)
    {
        m_nErrorCode=RegDeleteKey(m_hKey,_T(""));
        if(m_nErrorCode==ERROR_SUCCESS)
            Close();
    }

    return success;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : On error returns -1
// Globals :
// I/O     :
// Task    : Return the number of values in this key
//           This count does not include the default value
//-----------------------------------------------------------------------
DWORD CRegistry::CountValues()
{
    DWORD values;
    if(!GetInfo(NULL,NULL,NULL,NULL,NULL,&values))
        return (DWORD)-1;
    return values;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Force saving of registry key data back in the registry
//-----------------------------------------------------------------------
BOOL CRegistry::Flush()
{
    if(!m_bStatus)
        return FALSE;
    m_nErrorCode=RegFlushKey(m_hKey);
    return m_nErrorCode==ERROR_SUCCESS;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Conect to a remote computer's HKEY_LOCAL_MACHINE key
//-----------------------------------------------------------------------
BOOL CRegistry::Connect_HKEY_LOCAL_MACHINE(LPCTSTR machine)
{
    Close();

    TCHAR machine_name[MAX_REG_MACHINE_NAME_LEN];
    lstrcpy(machine_name,machine);
    m_nErrorCode=RegConnectRegistry(machine_name,HKEY_LOCAL_MACHINE,&m_hKey);
    if(m_nErrorCode==ERROR_SUCCESS)
        m_bStatus=TRUE;
    else
        m_bStatus=FALSE;

    return m_bStatus;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    : Return TRUE on success
// Globals :
// I/O     :
// Task    : Conect to a remote computer's HKLM key
//-----------------------------------------------------------------------
BOOL CRegistry::Connect_HKEY_USERS(LPCTSTR machine)
{
    Close();

    TCHAR machine_name[MAX_REG_MACHINE_NAME_LEN];
    lstrcpy(machine_name,machine);
    m_nErrorCode=RegConnectRegistry(machine_name,HKEY_USERS,&m_hKey);
    if(m_nErrorCode==ERROR_SUCCESS)
        m_bStatus=TRUE;
    else	  
        m_bStatus=FALSE;

    return m_bStatus;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    :
// Globals :
// I/O     :
// Task    : Close key
//-----------------------------------------------------------------------
void CRegistry::Close()
{
    if(m_bStatus)
        RegCloseKey(m_hKey);

    m_hKey    =NULL;
    m_bStatus =FALSE;
}

//-----------------------------------------------------------------------
// Pre     :
// Post    :
// Globals :
// I/O     :
// Task    : Close registry key before destruction
//-----------------------------------------------------------------------
CRegistry::~CRegistry()
{
    Close();
}

