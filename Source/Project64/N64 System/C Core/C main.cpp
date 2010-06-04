#include "stdafx.h"

int GetStoredWinPos( char * WinName, DWORD * X, DWORD * Y ) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",GetAppName());
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0, KEY_ALL_ACCESS,&hKeyResults);
	
	if (lResult == ERROR_SUCCESS) {
		DWORD Type, Value, Bytes = 4;

		sprintf(String,"%s Top",WinName);
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
			*Y = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}
	
		sprintf(String,"%s Left",WinName);
		lResult = RegQueryValueEx(hKeyResults,String,0,&Type,(LPBYTE)(&Value),&Bytes);
		if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
			*X = Value;
		} else {
			RegCloseKey(hKeyResults);
			return FALSE;
		}
		RegCloseKey(hKeyResults);
		return TRUE;
	}
	return FALSE;
}

void StoreCurrentWinPos (  char * WinName, HWND hWnd ) {
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	RECT WinRect;
	char String[200];

	GetWindowRect(hWnd, &WinRect );
	sprintf(String,"Software\\N64 Emulation\\%s\\Page Setup",GetAppName());
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER, String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL, &hKeyResults,&Disposition);
	if (lResult == ERROR_SUCCESS) {
		sprintf(String,"%s Top",WinName);
		RegSetValueEx(hKeyResults,String,0, REG_DWORD,(CONST BYTE *)(&WinRect.top),
			sizeof(DWORD));
		sprintf(String,"%s Left",WinName);
		RegSetValueEx(hKeyResults,String,0, REG_DWORD,(CONST BYTE *)(&WinRect.left),
			sizeof(DWORD));
	}
	RegCloseKey(hKeyResults);
}
