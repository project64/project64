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

#include <string>
#include <stdio.h>

#include <windows.h>
#include <CommDlg.h>
#include <shlobj.h>
#include <tchar.h>

#include "commonIncludes.h"
#include "DirectInput.h"
#include "FileAccess.h"
#include "Interface.h"
#include "NRagePluginV2.h"
#include "PakIO.h"

using std::string;

#ifndef IDR_PROFILE_DEFAULT1
#define IDR_PROFILE_DEFAULT1 -1
#endif
#ifndef IDR_PROFILE_DEFAULT2
#define IDR_PROFILE_DEFAULT2 -1
#endif
#ifndef IDR_PROFILE_DEFAULT3
#define IDR_PROFILE_DEFAULT3 -1
#endif
#ifndef IDR_PROFILE_DEFAULT4
#define IDR_PROFILE_DEFAULT4 -1
#endif

void DumpStreams(FILE * fFile, string strMouse, string strDevs[], string strNull, bool bIsINI);
void DumpControllerSettings(FILE * fFile, int i, bool bIsINI);
void FormatControlsBlock(string * strMouse, string strDevs[], string * strNull, int i);
void FormatModifiersBlock(string * strMouse, string strDevs[], string * strNull, int i);

// return true if the file exists... let's just use CreateFile with OPEN_EXISTING
bool CheckFileExists( LPCTSTR FileName )
{
	HANDLE hFile = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	else
	{
		CloseHandle(hFile);
		return true;
	}
}

// A rather ugly function, but does its job.  Called by LoadProfile and LoadProfileFromResource.
// Parses the config data that gets written to profile files.
// returns:
//		PL_CATEGORY and removes the brackets if the line looks like a keymapping
//		PL_VERSIONSTRING and returns the version number if it's a version line
//		for other cases, another PL return value and truncates before the equal sign; so strings like "Button=blah" -> "blah"
// TODO: Perhaps buffer overflow and crash potential here... needs auditing
DWORD ParseLine( LPSTR pszLine )
{
	DWORD dwReturn = PL_NOHIT;
	char *pChar = pszLine;

	switch (pszLine[0])
	{
	case '\0':	// shortcut out on null string
	case '#':	// # indicates comment line
		return PL_NOHIT;
	case '[':
		while( *pChar != ']' && *pChar != '\0' )
		{
			*pChar = toupper(*pChar);
			++pChar;
		}
		if( *pChar == ']' )
		{
			MoveMemory( pszLine, pszLine+1, (pChar-pszLine) - 1 * sizeof(pszLine[0]) ); // TODO: please double check this --rabid
			*(pChar - 1) = '\0';	// since we moved everything back one character, we need to change this ref as well
			return PL_CATEGORY;
		}
		else
			return PL_NOHIT; // an open bracket with no closing returns nohit
	case '@':
		switch( djbHash(pszLine))	// the hash check is case sensitive, and includes the @ symbol
		{
		case CHK_PROFILEVERSION20:
			lstrcpyA( pszLine, "2.0" );
			return PL_VERSIONSTRING;
		case CHK_PROFILEVERSION21:
			lstrcpyA( pszLine, "2.1" );
			return PL_VERSIONSTRING;
		case CHK_PROFILEVERSION22:
			lstrcpyA( pszLine, "2.2" );
			return PL_VERSIONSTRING;
		default:
			DebugWriteA("Unknown version string found with hash %u: %s\n", djbHash(pszLine), pszLine);
			return PL_NOHIT;
		} // end switch (dbjHash(pszLine))
	// default: keep running
	}

	pChar = strchr(pszLine, '=');
 
	if( !pChar ) // no = sign
	{
		return PL_NOHIT;
	}
	else // there is an '=' sign
	{
		// We hash the string.  If the hash matches the hash of one of our targets, we compare strings to verify.
		// If we don't use hashes, we have to compare vs a LOT of strings.
		*pChar = '\0'; // truncate at the '=' for now
		for (char *pIter = pszLine; *pIter; pIter++)
			*pIter = toupper(*pIter);
		dwReturn = djbHash(pszLine);

		pChar++;

		MoveMemory( pszLine, pChar, (lstrlenA(pChar) + 1) * sizeof(pszLine[0]) ); // change string to match what's to the right of '='
	}

	return dwReturn;
}

// Called immediately after ParseLine to assign values based on whatever the keyname was
// notes: pszFFDevice may be overwritten with whatever is in pszLine; please make sure pszLine is not too big!
bool ProcessKey( DWORD dwKey, DWORD dwSection, LPCSTR pszLine, LPTSTR pszFFDevice, LPBYTE bFFDeviceNr, bool bIsInterface )
{
	static TCHAR pszDeviceName[MAX_PATH];
	static BYTE bDeviceNr = 0;
	static GUID gGUID;

	bool bReturn = true;
	LPCONTROLLER pController = NULL;	// used when we're assigning things in the [Controller X] category
	LPSHORTCUTS pShortcuts = NULL;
	unsigned int iLength = lstrlenA( pszLine ) / 2;	// 2 HEX characters correspond to one BYTE; thus iLength represents the length of pszLine after conversion to BYTEs

	switch (dwSection)
	{
	case CHK_CONTROLLER_1:
		if (bIsInterface)
			pController = &(g_ivConfig->Controllers[0]);
		else
			pController = &(g_pcControllers[0]);
		break;
	case CHK_CONTROLLER_2:
		if (bIsInterface)
			pController = &(g_ivConfig->Controllers[1]);
		else
			pController = &(g_pcControllers[1]);
		break;
	case CHK_CONTROLLER_3:
		if (bIsInterface)
			pController = &(g_ivConfig->Controllers[2]);
		else
			pController = &(g_pcControllers[2]);
		break;
	case CHK_CONTROLLER_4:
		if (bIsInterface)
			pController = &(g_ivConfig->Controllers[3]);
		else
			pController = &(g_pcControllers[3]);
		break;
	case CHK_SHORTCUTS:
		if (bIsInterface)
			pShortcuts = &(g_ivConfig->Shortcuts);
		else
			pShortcuts = &g_scShortcuts;
		break;
	}

	switch( dwKey )
	{
	case PL_RESET:
		ZeroMemory( pszDeviceName, sizeof(pszDeviceName) );
		gGUID = GUID_NULL;
		bDeviceNr = 0;
		break;

	case CHK_LANGUAGE:
		if (dwSection == CHK_GENERAL)
			if (bIsInterface)
				g_ivConfig->Language = atoi(pszLine);
			else
				g_strEmuInfo.Language = atoi(pszLine);
		break;
	case CHK_SHOWMESSAGES:
		if (dwSection == CHK_GENERAL)
			if (bIsInterface)
				g_ivConfig->fDisplayShortPop = (atoi(pszLine) != 0);
			else
				g_strEmuInfo.fDisplayShortPop = (atoi(pszLine) != 0);
		break;

	case CHK_MEMPAK:
		if (dwSection == CHK_LASTBROWSERDIR)
			CHAR_TO_TCHAR(g_aszLastBrowse[BF_MEMPAK], pszLine, MAX_PATH);
		else if (dwSection == CHK_FOLDERS)
			CHAR_TO_TCHAR(g_aszDefFolders[BF_MEMPAK], pszLine, MAX_PATH);
		break;
	case CHK_GBXROM:
		if (dwSection == CHK_LASTBROWSERDIR)
			CHAR_TO_TCHAR(g_aszLastBrowse[BF_GBROM], pszLine, MAX_PATH);
		else if (dwSection == CHK_FOLDERS)
			CHAR_TO_TCHAR(g_aszDefFolders[BF_GBROM], pszLine, MAX_PATH);
		break;
	case CHK_GBXSAVE:
		if (dwSection == CHK_LASTBROWSERDIR)
			CHAR_TO_TCHAR(g_aszLastBrowse[BF_GBSAVE], pszLine, MAX_PATH);
		else if (dwSection == CHK_FOLDERS)
			CHAR_TO_TCHAR(g_aszDefFolders[BF_GBSAVE], pszLine, MAX_PATH);
		break;
	case CHK_PROFILE:
		if (dwSection == CHK_LASTBROWSERDIR)
			CHAR_TO_TCHAR(g_aszLastBrowse[BF_PROFILE], pszLine, MAX_PATH);
		break;
	case CHK_NOTE:
		if (dwSection == CHK_LASTBROWSERDIR)
			CHAR_TO_TCHAR(g_aszLastBrowse[BF_NOTE], pszLine, MAX_PATH);
		break;
	case CHK_SHORTCUTS:
		if (dwSection == CHK_LASTBROWSERDIR)
			CHAR_TO_TCHAR(g_aszLastBrowse[BF_SHORTCUTS], pszLine, MAX_PATH);
		break;
	case CHK_PLUGGED:
		if (pController)
			pController->fPlugged = atoi(pszLine);
		break;
	case CHK_RAWDATA:
		if (pController)
			pController->fRawData = atoi(pszLine);
		break;
	case CHK_XINPUT:
		if (pController)
			pController->fXInput = atoi(pszLine);
		break;
	case CHK_N64MOUSE:
		if (pController)
			pController->fN64Mouse = atoi(pszLine);
		break;
	case CHK_BACKGROUNDINPUT:
		if (pController)
			pController->bBackgroundInput = atoi(pszLine);
		break;
	case CHK_PAKTYPE:
		if (pController)
			pController->PakType = atoi(pszLine);
		break;
	case CHK_REALN64RANGE:
		if (pController)
			pController->fRealN64Range = atoi(pszLine);
		break;
	case CHK_RAPIDFIREENABLED:
		if (pController)
			pController->bRapidFireEnabled = atoi(pszLine) != 0;
		break;
	case CHK_RAPIDFIRERATE:
		if (pController)
			pController->bRapidFireRate = atoi(pszLine);
		break;
	case CHK_STICKRANGE:
		if (pController)
			pController->bStickRange = atoi(pszLine);
		break;
	case CHK_MOUSEMOVEX:
		if (pController)
			pController->bMouseMoveX = atoi(pszLine);
		break;
	case CHK_MOUSEMOVEY:
		if (pController)
			pController->bMouseMoveY = atoi(pszLine);
		break;
	case CHK_AXISSET:
		if (pController)
			pController->bAxisSet = atoi(pszLine);
		break;
	case CHK_KEYABSOLUTEX:
		if (pController)
			pController->fKeyAbsoluteX = atoi(pszLine);
		break;
	case CHK_KEYABSOLUTEY:
		if (pController)
			pController->fKeyAbsoluteY = atoi(pszLine);
		break;
	case CHK_PADDEADZONE:
		if (pController)
			pController->bPadDeadZone = atoi(pszLine);
		break;
	case CHK_MOUSESENSITIVITYX:
		if (pController)
			pController->wMouseSensitivityX = atoi(pszLine);
		break;
	case CHK_MOUSESENSITIVITYY:
		if (pController)
			pController->wMouseSensitivityY = atoi(pszLine);
		break;
	case CHK_RUMBLETYPE:
		if (pController)
			pController->bRumbleTyp = atoi(pszLine);
		break;
	case CHK_RUMBLESTRENGTH:
		if (pController)
			pController->bRumbleStrength = atoi(pszLine);
		break;
	case CHK_VISUALRUMBLE:
		if (pController)
			pController->fVisualRumble = atoi(pszLine);
		break;
	case CHK_FFDEVICEGUID:
		if (pController)
		{
			bReturn = StringtoGUIDA(&pController->guidFFDevice, pszLine);
			if (bIsInterface && bReturn)
			{
				// For some reason, we use ONLY device names and numbers inside the interface for FF device selection.  So if we don't set those,
				// FFDevice won't load properly.
				int nDevice = FindDeviceinList(pController->guidFFDevice);
				if (nDevice != -1 && pszFFDevice && bFFDeviceNr)
				{
					_tcsncpy(pszFFDevice, g_devList[nDevice].szProductName, DEFAULT_BUFFER);
					*bFFDeviceNr = g_devList[nDevice].bProductCounter;
				}
				else
				{
					pController->guidFFDevice = GUID_NULL;
					return false;
				}
			}
			else
				return bReturn;
		}
		break;
	case CHK_FFDEVICENAME:
		if( pController && pszFFDevice )
		{
			CHAR_TO_TCHAR( pszFFDevice, pszLine, MAX_PATH ); // HACK: pszLine is read from a file; could overflow easily. guessed size of pszFFDevice buffer.
			return true;
		}
		break;

	case CHK_FFDEVICENR:
		if( pController && bFFDeviceNr && ( iLength >= sizeof(BYTE) ))
		{
			*bFFDeviceNr = atoi( pszLine );
			return true;
		}
		break;
	case CHK_MEMPAKFILE:
		if( pController )
		{
			CHAR_TO_TCHAR( pController->szMempakFile, pszLine, MAX_PATH );
		}
		break;
	case CHK_GBROMFILE:
		if( pController )
		{
			CHAR_TO_TCHAR( pController->szTransferRom, pszLine, MAX_PATH );
		}
		break;
	case CHK_GBROMSAVE:
		if( pController )
		{
			CHAR_TO_TCHAR( pController->szTransferSave, pszLine, MAX_PATH );
		}
		break;

	case CHK_DINPUTNAME:
		gGUID = GUID_NULL;	// invalidate current GUID
		CHAR_TO_TCHAR( pszDeviceName, pszLine, MAX_PATH );
		break;

	case CHK_DINPUTNR:
		gGUID = GUID_NULL;	// invalidate current GUID
		if( iLength >= sizeof(BYTE) )
		{
			TexttoHexA( pszLine, &bDeviceNr, sizeof(BYTE) );
		}
		break;
	case CHK_DINPUTGUID:
		if (StringtoGUIDA(&gGUID, pszLine))
			return true;
		else
		{
			gGUID = GUID_NULL;	// invalidate current GUID
			return false;
		}
		break;

	case CHK_BUTTON:
		if ( dwSection == CHK_CONTROLS || pShortcuts || pController )
		{
			int controlnum = 0, buttonID = 0;
			BUTTON btnWorking;

			ZeroMemory(&btnWorking, sizeof(btnWorking));

			unsigned int tOffset, tAxisID, tBtnType;

			if (sscanf(pszLine, "%d %d %x %u %u", &controlnum, &buttonID, &tOffset, &tAxisID, &tBtnType) != 5)
				return false;

			// done to overcome issues with sscanf and "small" data blocks
			btnWorking.bOffset = tOffset;
			btnWorking.bAxisID = tAxisID;
			btnWorking.bBtnType = tBtnType;

			if (pController)
			{
				// special case: if we're in one of the categories CHK_CONTROLLER_n, assume we're processing a Profile file.
				// Ignore the read controlnum and use our input controller number.
				controlnum = (int)(dwSection - CHK_CONTROLLER_1);	// HACK: assume our hash reproduces these linearly
			}
			
			// Now we need to assign parentdevice. If we have a valid gGUID, we'll use that...
			int found = FindDeviceinList(gGUID);
			if (found != -1)
				btnWorking.parentDevice = &g_devList[found];
			else
			{
				// ... otherwise, we do the following in order:
				//   1. If bBtnType is of type DT_MOUSEBUTTON or DT_MOUSEAXE, set gGUID to that of g_sysMouse (ignoring the given name and number)
				if ( btnWorking.bBtnType == DT_MOUSEBUTTON || btnWorking.bBtnType == DT_MOUSEAXE )
				{
					btnWorking.parentDevice = &g_sysMouse;
				}
				//   2. If bBtnType is of type DT_KEYBUTTON, set gGUID to that of SysKeyboard
				else if ( btnWorking.bBtnType == DT_KEYBUTTON )
				{
					gGUID = GUID_SysKeyboard;
					found = FindDeviceinList(gGUID);
					if (found != -1)
						btnWorking.parentDevice = &g_devList[found];
					else
						btnWorking.parentDevice = NULL;
				}
				//   3. otherwise, look up the name and number using FindDeviceinList, and set gGUID to that
				else
				{
					found = FindDeviceinList(pszDeviceName, bDeviceNr, true);
					if (found != -1)
					{
						gGUID = g_devList[found].guidInstance;
						btnWorking.parentDevice = &g_devList[found];
					}
					else
					{
						DebugWrite(_T("ProcessKey: couldn't find a device in g_devList for %s %d\n"), pszDeviceName, bDeviceNr);
						gGUID = GUID_NULL;
						btnWorking.parentDevice = NULL;
				return false;
					}
				}
			}

			if (pShortcuts)
			{
				// bounds check on controlnum and buttonID
				if ( (controlnum == -1 && buttonID != 0) && ((controlnum < 0) || (controlnum > 3) || (buttonID < 0) || (buttonID >= SC_TOTAL)) )
				{
					gGUID = GUID_NULL;	// since we may have cached an invalid GUID, invalidate it
					return false;
				}

				// Copy the completed button to the correct shortcut
				if (bIsInterface)
					if (controlnum == -1)
						g_ivConfig->Shortcuts.bMouseLock = btnWorking;
					else
						g_ivConfig->Shortcuts.Player[controlnum].aButtons[buttonID] = btnWorking;
				else // if (!bIsInterface)
					if (controlnum == -1)
						g_scShortcuts.bMouseLock = btnWorking;
					else
						g_scShortcuts.Player[controlnum].aButtons[buttonID] = btnWorking;
			}
			else // it's a controller button
			{
				// bounds check on controlnum and buttonID
				if ( (controlnum < 0) || (controlnum > 3) || (buttonID < 0) || (buttonID >= ARRAYSIZE(g_pcControllers[0].aButton)) )
				{
					gGUID = GUID_NULL;	// since we may have cached an invalid GUID, invalidate it
					return false;
				}

				// Copy the completed button to the correct controller and buttonID
				if (bIsInterface)
					g_ivConfig->Controllers[controlnum].aButton[buttonID] = btnWorking;
				else
					g_pcControllers[controlnum].aButton[buttonID] = btnWorking;
			}
		}
		break;

	case CHK_MODIFIER:
		// Modifiers format: controlnum bOffset bAxisID bBtnType bModType fToggle fStatus dwSpecific
		if ( dwSection == CHK_MODIFIERS || pController )
		{
			int controlnum = 0;
			MODIFIER modWorking;

			ZeroMemory(&modWorking, sizeof(modWorking));

			unsigned int tOffset, tAxisID, tBtnType, tModType, tToggle, tStatus, tSpecific;

			if (sscanf(pszLine, "%u %x %u %u %u %u %u %x", &controlnum, &tOffset, &tAxisID,
					&tBtnType, &tModType, &tToggle, &tStatus, &tSpecific) != 8)
				return false;

			// done to overcome issues with sscanf and "small" data blocks
			modWorking.btnButton.bOffset = tOffset;
			modWorking.btnButton.bAxisID = tAxisID;
			modWorking.btnButton.bBtnType = tBtnType;
			modWorking.bModType = tModType;
			modWorking.fToggle = tToggle;
			modWorking.fStatus = tStatus;
			modWorking.dwSpecific = tSpecific; // looks stupid, but unsigned int might not always be DWORD32
			
			// Now we need to assign parentdevice. If we have a valid gGUID, we'll use that...
			int found = FindDeviceinList(gGUID);
			if (found != -1)
				modWorking.btnButton.parentDevice = &g_devList[found];
			else
			{
				// ... otherwise, we do the following in order:
				//   1. If bBtnType is of type DT_MOUSEBUTTON or DT_MOUSEAXE, set gGUID to that of g_sysMouse (ignoring the given name and number)
				if ( modWorking.btnButton.bBtnType == DT_MOUSEBUTTON || modWorking.btnButton.bBtnType == DT_MOUSEAXE )
				{
					modWorking.btnButton.parentDevice = &g_sysMouse;
				}
				//   2. If bBtnType is of type DT_KEYBUTTON, set gGUID to that of SysKeyboard
				else if ( modWorking.btnButton.bBtnType == DT_KEYBUTTON )
				{
					gGUID = GUID_SysKeyboard;
					int found = FindDeviceinList(gGUID);
					if (found != -1)
						modWorking.btnButton.parentDevice = &g_devList[found];
					else
						modWorking.btnButton.parentDevice = NULL;
				}
				//   3. otherwise, look up the name and number using FindDeviceinList, and set gGUID to that
				else
				{
					found = FindDeviceinList(pszDeviceName, bDeviceNr, true);
					if (found != -1)
					{
						gGUID = g_devList[found].guidInstance;
						modWorking.btnButton.parentDevice = &g_devList[found];
					}
					else
					{
						DebugWrite(_T("ProcessKey: couldn't find a device in g_devList for %s %d\n"), pszDeviceName, bDeviceNr);
						gGUID = GUID_NULL;
						modWorking.btnButton.parentDevice = NULL;
					return false;
					}
				}
			}

			// bounds check on controlnum and buttonID
			if ( (controlnum < 0) || (controlnum > 3) )
			{
				gGUID = GUID_NULL;	// since we may have cached an invalid GUID, invalidate it
				return false;
			}

			// Allocate and add the completed modifier
			if (bIsInterface)
			{
				if (g_ivConfig->Controllers[controlnum].nModifiers > 0)
				{
					g_ivConfig->Controllers[controlnum].pModifiers = (LPMODIFIER)P_realloc(g_ivConfig->Controllers[controlnum].pModifiers, (g_ivConfig->Controllers[controlnum].nModifiers + 1) * sizeof(MODIFIER));
				}
				else
				{
					g_ivConfig->Controllers[controlnum].pModifiers = (LPMODIFIER)P_malloc( sizeof(MODIFIER));
				}
				g_ivConfig->Controllers[controlnum].pModifiers[g_ivConfig->Controllers[controlnum].nModifiers] = modWorking;
				(g_ivConfig->Controllers[controlnum].nModifiers)++;
			}
			else
			{
				if (g_pcControllers[controlnum].nModifiers > 0)
				{
					g_pcControllers[controlnum].pModifiers = (LPMODIFIER)P_realloc(g_pcControllers[controlnum].pModifiers, (g_pcControllers[controlnum].nModifiers + 1) * sizeof(MODIFIER));
				}
				else
				{
					g_pcControllers[controlnum].pModifiers = (LPMODIFIER)P_malloc( sizeof(MODIFIER));
				}
				g_pcControllers[controlnum].pModifiers[g_pcControllers[controlnum].nModifiers] = modWorking;
				(g_pcControllers[controlnum].nModifiers)++;
			}

		}
		break;
	}

	return bReturn;
}

/******************
Load the default profile from the raw "resource" data (i.e. the builtin defaults contained in the dll)
******************/
bool LoadProfileFromResource( LPCTSTR pszResource, int iController, bool bIsInterface )
{
	const DWORD dwControllerSect[] = { CHK_CONTROLLER_1 , CHK_CONTROLLER_2, CHK_CONTROLLER_3, CHK_CONTROLLER_4 };
	if( iController > 3 || iController < 0 )
		return false;
	HRSRC res = FindResource( g_strEmuInfo.hinst, pszResource, _T("PROFILE") );
	if( res == NULL )
		return false;
	char *profile = (char*)LockResource( LoadResource( g_strEmuInfo.hinst, res ));
	char *profileend = profile + SizeofResource( g_strEmuInfo.hinst, res );
	
	ProcessKey( PL_RESET, 0, 0, 0, 0, bIsInterface );
	DWORD dwCommand = PL_NOHIT;
	char szLine[4096];
	while( profile < profileend )
	{
		while( profile < profileend && (CHECK_WHITESPACES( *profile ) || *profile == ' ' ))
			++profile;
		int i = 0;
		while( profile < profileend && i < sizeof(szLine)-1 && !(CHECK_WHITESPACES( *profile )) )
			szLine[i++] = *profile++;

		szLine[i] = '\0';
		dwCommand = ParseLine( szLine );
		ProcessKey( dwCommand, dwControllerSect[iController], szLine, 0, 0, bIsInterface ); // resource will not contain a FF device
	}
	return true;
}

/******************
See overloaded function above
******************/
bool LoadProfileFromResource( int indexController, bool bIsInterface )
{
	const int resIds[] = { IDR_PROFILE_DEFAULT1, IDR_PROFILE_DEFAULT2, IDR_PROFILE_DEFAULT3, IDR_PROFILE_DEFAULT4 };

	TCHAR szId[20];
	wsprintf( szId, _T("#%i"), resIds[indexController] );
	return LoadProfileFromResource( szId, indexController, bIsInterface );
}

// Load a controller profile from a saved configuration file
// need to incorporate type (keyb/mouse/joy), GUID for joy, and bOffset
bool LoadProfileFile( const TCHAR *pszFileName, int iController, TCHAR *pszFFDevice, BYTE *bFFDeviceNr )
{
	const DWORD dwControllerSect[] = { CHK_CONTROLLER_1 , CHK_CONTROLLER_2, CHK_CONTROLLER_3, CHK_CONTROLLER_4 };
	FILE *proFile = NULL;
	char szLine[4096];
	int iVersion = 0;

	if ( (proFile = _tfopen(pszFileName, _T("rS")) ) == NULL)
		return false;
	
	// Test if right Version
	while( !iVersion && ( fgets(szLine, sizeof(szLine) - 1, proFile) ) )
	{
		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		if( ParseLine( szLine ) == PL_VERSIONSTRING )
			iVersion = (int)(atof( szLine ) * 100);
	}
	if( iVersion != 220 ) // HACK: this should probably not be a hardcoded value
	{
		fclose(proFile);
		return false;
	}

	SetControllerDefaults( &(g_ivConfig->Controllers[iController]) );
	pszFFDevice[0] = pszFFDevice[1] = '\0';
	*bFFDeviceNr = 0;

	ProcessKey( PL_RESET, 0, 0, 0, 0, true );
	DWORD dwCommand = PL_NOHIT;
	while( fgets(szLine, sizeof(szLine) - 1, proFile) )
	{
		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		dwCommand = ParseLine( szLine );
		ProcessKey( dwCommand, dwControllerSect[iController], szLine, pszFFDevice, bFFDeviceNr, true );
	}

	fclose(proFile);

	return true;
}

// Load a controller profile from a saved configuration file
// need to incorporate type (keyb/mouse/joy), GUID for joy, and bOffset
bool LoadShortcutsFile( const TCHAR *pszFileName )
{
	FILE *fShortsFile = NULL;
	char szLine[4096];
	int iVersion = 0;

	if ( (fShortsFile = _tfopen(pszFileName, _T("rS")) ) == NULL)
		return false;
	
	// Test if right Version
	while( !iVersion && ( fgets(szLine, sizeof(szLine) - 1, fShortsFile) ) )
	{
		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		if( ParseLine( szLine ) == PL_VERSIONSTRING )
			iVersion = (int)(atof( szLine ) * 100);
	}
	if( iVersion != 220 ) // HACK: this should probably not be a hardcoded value
	{
		fclose(fShortsFile);
		return false;
	}

	ZeroMemory( &(g_ivConfig->Shortcuts), sizeof(SHORTCUTS) );

	ProcessKey( PL_RESET, 0, 0, 0, 0, true );
	DWORD dwCommand = PL_NOHIT;
	while( fgets(szLine, sizeof(szLine) - 1, fShortsFile) )
	{
		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		dwCommand = ParseLine( szLine );
		ProcessKey( dwCommand, CHK_SHORTCUTS, szLine, 0, 0, true );
	}

	fclose(fShortsFile);

	return true;
}

// Serializes the profile for the CURRENT controller for saving to a file
// called in one place, from within Interface.cpp, ControllerTabProc (when you click "Save Profile")
void FormatProfileBlock( FILE * fFile, const int i )
{
	DumpControllerSettings(fFile, i, false);

	string strMouse;
	string strDevs[MAX_DEVICES];
	string strNull;

	FormatControlsBlock(&strMouse, strDevs, &strNull, i);

	DumpStreams(fFile, strMouse, strDevs, strNull, false);

	strMouse.clear();
	for (int j = 0; j < g_nDevices; j++)
		strDevs[j].clear();
	strNull.clear();

	FormatModifiersBlock(&strMouse, strDevs, &strNull, i);

	DumpStreams(fFile, strMouse, strDevs, strNull, false);
}

// same as FormatProfileBlock, but saves shortcuts instead
void FormatShortcutsBlock(FILE * fFile, bool bIsINI)
{
	// I'm going to use STL strings here because I don't want to screw with buffer management
	string strMouse;
	string strDevs[MAX_DEVICES];
	string strNull;

	for ( int i = 0; i < 4; i++ ) // Player for
	{
		for ( int j = 0; j < SC_TOTAL; j++ ) // aButtons for
		{
			if (g_ivConfig->Shortcuts.Player[i].aButtons[j].parentDevice) // possibly unbound
			{
				if ( IsEqualGUID(g_sysMouse.guidInstance, g_ivConfig->Shortcuts.Player[i].aButtons[j].parentDevice->guidInstance) )
				{
					char szBuf[DEFAULT_BUFFER];
					// add to the mouse stream
					sprintf(szBuf, STRING_INI_BUTTON "=%d %d %02X %d %d\n", i, j, g_ivConfig->Shortcuts.Player[i].aButtons[j].bOffset, g_ivConfig->Shortcuts.Player[i].aButtons[j].bAxisID, g_ivConfig->Shortcuts.Player[i].aButtons[j].bBtnType);
					strMouse.append(szBuf);
				}
				else
					for (int match = 0; match < g_nDevices; match++)
						if ( IsEqualGUID(g_devList[match].guidInstance, g_ivConfig->Shortcuts.Player[i].aButtons[j].parentDevice->guidInstance) )
						{
							char szBuf[DEFAULT_BUFFER];
							// add to the appropriate device stream
							sprintf(szBuf, STRING_INI_BUTTON "=%d %d %02X %d %d\n", i, j, g_ivConfig->Shortcuts.Player[i].aButtons[j].bOffset, g_ivConfig->Shortcuts.Player[i].aButtons[j].bAxisID, g_ivConfig->Shortcuts.Player[i].aButtons[j].bBtnType);
							strDevs[match].append(szBuf);
							break;
						}
			}
		} // end buttons for
	} // end Player for

	// gotta do it again for that one pesky mouselock button
	if (g_ivConfig->Shortcuts.bMouseLock.parentDevice) // possibly unbound
	{
		if ( IsEqualGUID(g_sysMouse.guidInstance, g_ivConfig->Shortcuts.bMouseLock.parentDevice->guidInstance) )
		{
			char szBuf[DEFAULT_BUFFER];
			// add to the mouse stream
			sprintf(szBuf, STRING_INI_BUTTON "=%d %d %02X %d %d\n", -1, 0, g_ivConfig->Shortcuts.bMouseLock.bOffset, g_ivConfig->Shortcuts.bMouseLock.bAxisID, g_ivConfig->Shortcuts.bMouseLock.bBtnType);
			strMouse.append(szBuf);
		}
		else
			for (int match = 0; match < g_nDevices; match++)
				if ( IsEqualGUID(g_devList[match].guidInstance, g_ivConfig->Shortcuts.bMouseLock.parentDevice->guidInstance) )
				{
					char szBuf[DEFAULT_BUFFER];
					// add to the appropriate device stream
					sprintf(szBuf, STRING_INI_BUTTON "=%d %d %02X %d %d\n", -1, 0, g_ivConfig->Shortcuts.bMouseLock.bOffset, g_ivConfig->Shortcuts.bMouseLock.bAxisID, g_ivConfig->Shortcuts.bMouseLock.bBtnType);
					strDevs[match].append(szBuf);
					break;
				}
	} // end shortcuts edge case

	DumpStreams(fFile, strMouse, strDevs, strNull, bIsINI);
}

// load shortcuts from "resources", i.e. builtin defaults
bool LoadShortcutsFromResource(bool bIsInterface)
{
	if (bIsInterface)
		ZeroMemory( &(g_ivConfig->Shortcuts), sizeof(SHORTCUTS) );
	TCHAR szId[20];
	wsprintf( szId, _T("#%i"), IDR_SHORTCUTS_DEFAULT );
	HRSRC res = FindResource( g_strEmuInfo.hinst, szId, _T("SHORTCUT") );
	if( res == NULL )
		return false;
	char *profile = (char*)LockResource( LoadResource( g_strEmuInfo.hinst, res ));
	char *profileend = profile + SizeofResource( g_strEmuInfo.hinst, res );
	
	ProcessKey( PL_RESET, 0, 0, 0, 0, bIsInterface );
	DWORD dwCommand = PL_NOHIT;
	char szLine[4096];
	while( profile < profileend )
	{
		while( profile < profileend && (CHECK_WHITESPACES( *profile ) || *profile == ' ' ))
			++profile;
		int i = 0;
		while( profile < profileend && i < sizeof(szLine)-1 && !(CHECK_WHITESPACES( *profile )) )
			szLine[i++] = *profile++;

		szLine[i] = '\0';
		dwCommand = ParseLine( szLine );
		ProcessKey( dwCommand, CHK_SHORTCUTS, szLine, 0, 0, bIsInterface );
	}
	return true;

}

// returns the user-chosen default directory (path) for each of the following:
//		application dir, mempak dir, gameboy rom dir, gameboyrom save dir
//		Tries to query user settings; if blank or invalid, returns their defaults
// Massages the output directory a bit
bool GetDirectory( LPTSTR pszDirectory, WORD wDirID )
{
	bool bReturn = true;
	TCHAR szBuffer[MAX_PATH + 1];
	const TCHAR szDefaultStrings[3][DEFAULT_BUFFER] = { STRING_DEF_MEMPAKFILE, STRING_DEF_GBROMFILE, STRING_DEF_GBROMSAVE };
	TCHAR *pSlash;

	pszDirectory[0] = pszDirectory[1] = '\0';

	switch( wDirID )
	{
	case DIRECTORY_MEMPAK:
	case DIRECTORY_GBROMS:
	case DIRECTORY_GBSAVES:
		if (g_aszDefFolders[wDirID][0] == 0)
			lstrcpyn( pszDirectory, szDefaultStrings[wDirID], MAX_PATH);
		else
			lstrcpyn( pszDirectory, g_aszDefFolders[wDirID], MAX_PATH);
		break;

	case DIRECTORY_DLL:
		if (GetModuleFileName(g_strEmuInfo.hinst, szBuffer, MAX_PATH))
		{
			GetFullPathName( szBuffer, MAX_PATH, pszDirectory, &pSlash );
			*pSlash = 0;
		}
		break;
	case DIRECTORY_LOG:
	case DIRECTORY_CONFIG:
	case DIRECTORY_APPLICATION:
		break;

	default:
		// we don't know what the hell you're talking about, set pszFileName to current .exe directory
		// and return false
		bReturn = false;
	}

	if( pszDirectory[1] == ':' || ( pszDirectory[1] == '\\' && pszDirectory[0] == '\\' )) // Absolute Path( x: or \\ )
		lstrcpyn( szBuffer, pszDirectory, MAX_PATH );
	else
	{
		GetModuleFileName( NULL, szBuffer, MAX_PATH );
		pSlash = _tcsrchr( szBuffer, '\\' );
		++pSlash;
		lstrcpyn( pSlash, pszDirectory, MAX_PATH );
	}
	GetFullPathName( szBuffer, MAX_PATH, pszDirectory, &pSlash );

	pSlash = &pszDirectory[lstrlen( pszDirectory ) - 1];
	if( *pSlash != '\\' )
	{
		pSlash[1] = '\\';
		pSlash[2] = '\0';
	}

	if (bReturn && wDirID == DIRECTORY_CONFIG)
	{
		strcat(pszDirectory,"Config\\");
	}
	if (bReturn && wDirID == DIRECTORY_LOG)
	{
		strcat(pszDirectory,"Logs\\");
	}
	return bReturn;
}

// Attempts to store the "absolute" filename for a file;
//	if szFileName is an absolute filename (starting with a letter and colon or two backslashes) it is simply copied
//	otherwise, it is concatenated with the known directory, such as mempak directory (type given by wDirID)
void GetAbsoluteFileName( TCHAR *szAbsolute, const TCHAR *szFileName, const WORD wDirID )
{
	if( szFileName[1] == ':' || (szFileName[1] == '\\' && szFileName[0] == '\\'))
		lstrcpyn( szAbsolute, szFileName, MAX_PATH );
	else
	{
		GetDirectory( szAbsolute, wDirID );
		lstrcat( szAbsolute, szFileName); // HACK: possible buffer overflow
	}
}

// Populates the list of mempak/transfer pak files from the config'd directory
BOOL SendFilestoList( HWND hDlgItem, WORD wType )
{
	HANDLE hFindFile;

	WIN32_FIND_DATA FindFile;
	TCHAR szPattern[MAX_PATH + 10];
	TCHAR *pszExtensions;
	BOOL Success;

	switch( wType )
	{
	case FILIST_MEM:
		GetDirectory( szPattern, DIRECTORY_MEMPAK );
		lstrcat( szPattern, _T("*.*") );
		pszExtensions = _T(".mpk\0.n64\0");
		break;
	case FILIST_TRANSFER:
		GetDirectory( szPattern, DIRECTORY_GBROMS );
		lstrcat( szPattern, _T("*.gb?") );
		pszExtensions = _T(".gb\0.gbc\0");
		break;
	default:
		return FALSE;
	}

	TCHAR *pcPoint;
	TCHAR *pszExt;
	bool bValidFile;

	hFindFile = FindFirstFile( szPattern, &FindFile );
	if( hFindFile != INVALID_HANDLE_VALUE )
	{
		do
		{
			pszExt = pszExtensions;
			pcPoint = _tcsrchr( FindFile.cFileName, _T('.') );
			bValidFile = false;
			do
			{
				if( !lstrcmpi( pcPoint, pszExt ))
					bValidFile = true;
				pszExt += lstrlen( pszExt ) + 1;	
			}
			while( *pszExt && !bValidFile );

			if( bValidFile )
				SendMessage( hDlgItem, LB_ADDSTRING, 0, (LPARAM)FindFile.cFileName );
		}
		while( FindNextFile( hFindFile, &FindFile ));
		FindClose( hFindFile );
		Success = TRUE;
	}
	else
		Success = FALSE;

	return Success;
}

bool BrowseFolders( HWND hwndParent, TCHAR *pszHeader, TCHAR *pszDirectory )
{
	ITEMIDLIST *piStart = NULL;

	if( pszDirectory[0] != '\0')
	{
		IShellFolder* pDesktopFolder;
		if( SUCCEEDED( SHGetDesktopFolder( &pDesktopFolder )))
		{
			OLECHAR olePath[MAX_PATH];
			ULONG chEaten;

			pDesktopFolder->ParseDisplayName( NULL, NULL, olePath, &chEaten, &piStart, NULL );

			pDesktopFolder->Release();
		}
	}

	BROWSEINFO brInfo;
	brInfo.hwndOwner = hwndParent;
	brInfo.pidlRoot = piStart;
	brInfo.pszDisplayName = pszDirectory;
	brInfo.lpszTitle  = pszHeader;
	brInfo.ulFlags = BIF_RETURNONLYFSDIRS;
	brInfo.lpfn = NULL;

	ITEMIDLIST *piList;

	piList = SHBrowseForFolder( &brInfo );
	if( piList )
	{
		SHGetPathFromIDList( (const LPITEMIDLIST)piList, pszDirectory );
		LPMALLOC pMal;
		if( SUCCEEDED( SHGetMalloc( &pMal )))
		{
			pMal->Free( piList );
			pMal->Release();
		}
		return true;
	}

	return false;
}

bool GetInitialBrowseDir( TCHAR *pszFileName, DWORD dwType )
{
	// DIRECTORY_INVALID means there's no corresponding entry in g_aszDefFolders
	const WORD wDirectory[] = { DIRECTORY_MEMPAK, DIRECTORY_GBROMS, DIRECTORY_GBSAVES,
								DIRECTORY_INVALID, DIRECTORY_INVALID, DIRECTORY_INVALID };
	switch( dwType )
	{
	case BF_PROFILE:
	case BF_MEMPAK:
	case BF_NOTE:
	case BF_GBROM:
	case BF_GBSAVE:
	case BF_SHORTCUTS:
		if (g_aszLastBrowse[dwType][0] == 0)
			return GetDirectory( pszFileName, wDirectory[dwType]);
		else
			lstrcpyn(pszFileName, g_aszLastBrowse[dwType], MAX_PATH);
		return true;

	default:	// we don't know what the hell you're talking about
		return GetDirectory( pszFileName, DIRECTORY_INVALID );
	}
}

bool SaveLastBrowseDir( TCHAR *pszFileName, DWORD dwType )
{
	TCHAR *cSlash = _tcsrchr( pszFileName, _T('\\') );
	if( cSlash )
	{
		switch( dwType )
		{
		case BF_PROFILE:
		case BF_MEMPAK:
		case BF_NOTE:
		case BF_GBROM:
		case BF_GBSAVE:
		case BF_SHORTCUTS:
			*cSlash = '\0';
			lstrcpyn(g_aszLastBrowse[dwType], pszFileName, MAX_PATH);
			*cSlash = '\\';
			return true;
		default:
			return false;
		}
	}
	else
		return true;
}

// Pop up a dialog asking for a filename from the user.  Returns true if returning a valid filename, false if user cancelled.
// Used when either loading (fSave == false) or saving (fSave == true) some type of file.
// Handy, because it handles all our file type extensions for us.
bool BrowseFile( HWND hDlg, TCHAR *pszFileName, DWORD dwType, bool fSave )
{
	TCHAR pszFilter[DEFAULT_BUFFER];
	TCHAR pszTitle[DEFAULT_BUFFER];
	DWORD dwFlags = /*OFN_DONTADDTORECENT |*/ OFN_NOCHANGEDIR;
	dwFlags |= (fSave)	? OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT
						: OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	TCHAR *pszExt = NULL;

	TCHAR *pszTemp = pszFilter;
	int nFilters = 0;

	switch( dwType )
	{
	case BF_PROFILE:
		LoadString( g_hResourceDLL, IDS_DLG_CPF, pszFilter, DEFAULT_BUFFER );
		pszExt = _T("cpf");
		nFilters = 1;
		break;
	case BF_MEMPAK:
		LoadString( g_hResourceDLL, IDS_DLG_MPKN64, pszFilter, DEFAULT_BUFFER );
		if( !fSave )
		{
			LoadString( g_hResourceDLL, IDS_DLG_MPCHOOSE, pszTitle, DEFAULT_BUFFER );
			dwFlags = OFN_HIDEREADONLY;
		}
		pszExt = _T("mpk");
		nFilters = 2;
		break;
	case BF_NOTE:
		LoadString( g_hResourceDLL, IDS_DLG_A64, pszFilter, DEFAULT_BUFFER );
		pszExt = _T("a64");
		nFilters = 1;
		break;
	case BF_GBROM:
		LoadString( g_hResourceDLL, IDS_DLG_GBGBC, pszFilter, DEFAULT_BUFFER );
		pszExt = _T("gb");
		nFilters = 1;
		break;
	case BF_GBSAVE:
		LoadString( g_hResourceDLL, IDS_DLG_SVSAV, pszFilter, DEFAULT_BUFFER );
		pszExt = _T("sv");
		nFilters = 1;
		break;
	case BF_SHORTCUTS:
		LoadString( g_hResourceDLL, IDS_DLG_SC, pszFilter, DEFAULT_BUFFER );
		pszExt = _T("sc");
		nFilters = 1;
		break;
	default:
		return false;
	}

	for ( ; nFilters > 0; nFilters--)
	{
		pszTemp += _tcslen(pszTemp);
		pszTemp += 1;
		pszTemp += _tcslen(pszTemp);
		pszTemp += 1;
	}
	*pszTemp = _T('\0');

	dwFlags |= OFN_NOCHANGEDIR;

	TCHAR szFileName[MAX_PATH+1] = _T(""),
		  szInitialDir[MAX_PATH+1] = _T(""),
		  *pcSlash;

	if( pszFileName[1] == _T(':') || ( pszFileName[1] == _T('\\') && pszFileName[0] == _T('\\') ))
	{
		lstrcpyn( szInitialDir, pszFileName, ARRAYSIZE(szInitialDir) );
		pcSlash = _tcsrchr( szInitialDir, _T('\\') );
		if( pcSlash )
		{
			*pcSlash = _T('\0');
			lstrcpyn( szFileName, &pcSlash[1], ARRAYSIZE(szFileName) );
		}
	}
	else
	{
		if( !GetInitialBrowseDir( szInitialDir, dwType ))
			GetDirectory( szInitialDir, DIRECTORY_APPLICATION );
		lstrcpyn( szFileName, pszFileName, ARRAYSIZE(szFileName) );
	}

	OPENFILENAME oFile;

	oFile.lStructSize		= sizeof (OPENFILENAME);
	oFile.hwndOwner			= hDlg;
	oFile.hInstance			= NULL;
	oFile.lpstrFilter		= pszFilter;
	oFile.lpstrCustomFilter	= NULL;
	oFile.nMaxCustFilter	= 0;
	oFile.nFilterIndex		= 0;
	oFile.lpstrFile			= szFileName;
	oFile.nMaxFile			= MAX_PATH;
	oFile.lpstrFileTitle	= NULL;
	oFile.nMaxFileTitle		= MAX_PATH; // ignored
	oFile.lpstrInitialDir	= szInitialDir;
	oFile.lpstrTitle		= pszTitle;
	oFile.Flags				= dwFlags;
	oFile.nFileOffset		= 0;
	oFile.nFileExtension	= 0;
	oFile.lpstrDefExt		= pszExt;
	oFile.lCustData			= 0L;
	oFile.lpfnHook			= NULL;
	oFile.lpTemplateName	= NULL;

	if( fSave )
	{
		if( !GetSaveFileName( &oFile ))
			return false;
	}
	else
	{
		if( !GetOpenFileName( &oFile ))
			return false;
	}

	lstrcpy( pszFileName, szFileName );
	SaveLastBrowseDir( szFileName, dwType );
	return true;
}

bool ReadMemPakFile( TCHAR *pszMemPakFile, BYTE *aMemPak, bool fCreate )
{
	DWORD dwCreationDisposition = fCreate ? OPEN_ALWAYS : OPEN_EXISTING;

	HANDLE hFile = CreateFile( pszMemPakFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, dwCreationDisposition, 0, NULL);
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		ZeroMemory( aMemPak, PAK_MEM_SIZE );
		TCHAR *pcPoint = _tcsrchr( pszMemPakFile, '.' );
		if( !lstrcmpi( pcPoint, _T(".n64") ) )
			SetFilePointer( hFile, 0x1040, NULL, FILE_BEGIN );
		else
			SetFilePointer( hFile, 0L, NULL, FILE_BEGIN );
		
		DWORD dwBytesRead;
		bool Success = ( ReadFile( hFile, aMemPak, PAK_MEM_SIZE, &dwBytesRead, NULL) != 0 );

		CloseHandle( hFile );
		return Success;
	}
	else
		ErrorMessage( IDS_ERR_MPREAD, GetLastError(), false );
	return false;
}

// Used by Interface to create or modify mempak files (not mapped).
// pszMemPakFile is a filename, aMemPak is the data, fCreate tells whether to create a new file
bool WriteMemPakFile( TCHAR *pszMemPakFile, BYTE *aMemPak, bool fCreate )
{
	DWORD dwCreationDisposition = fCreate ? OPEN_ALWAYS : OPEN_EXISTING;

	HANDLE hFile = CreateFile( pszMemPakFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, 0, NULL);
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		DWORD dwBytesWritten = 0;
		TCHAR *pcPoint = _tcsrchr( pszMemPakFile, _T('.') );
		if( !lstrcmpi( pcPoint, _T(".n64") ) )
		{
			if( fCreate && !GetFileSize( hFile, NULL ))
			{
				char szHeader[] = "123-456-STD";
				SetFilePointer( hFile, 0L, NULL, FILE_BEGIN );
				WriteFile( hFile, szHeader, sizeof(szHeader), &dwBytesWritten, NULL );
			}
			SetFilePointer( hFile, 0x1040, NULL, FILE_BEGIN );
		}
		else
			SetFilePointer( hFile, 0L, NULL, FILE_BEGIN );
		
		bool Success = ( WriteFile( hFile, aMemPak, PAK_MEM_SIZE, &dwBytesWritten, NULL ) != 0 );
		if( Success )
			SetEndOfFile( hFile );
		
		CloseHandle( hFile );
		return Success;
	}
	else
		ErrorMessage( IDS_ERR_MPCREATE, GetLastError(), false );

	return false;
}

// This func stores the current config data to INI.  It stores the Interface's idea of configuration
// As such, it should only be called from the config window (Interface). Otherwise, it will fail.
// Returns true if saved OK, false if there was a problem.
bool StoreConfigToINI()
{
	char szANSIBuf[DEFAULT_BUFFER];
	if (!g_ivConfig)
		return false;

	TCHAR szFilename[MAX_PATH];
	GetDirectory(szFilename, DIRECTORY_CONFIG);
	_tcscat(szFilename, _T("NRage.ini"));
	FILE *fFile = _tfopen(szFilename, _T("wS"));	// write, optimize for sequential

	if (!fFile)
	{
		DebugWriteA("Couldn't open INI file for output!\n");
		return false;
	}

	// first write out any standard header stuff here
	fputs(STRING_INI_HEADER, fFile);

	// General
	fputs("\n[" STRING_INI_GENERAL "]\n", fFile);
	fprintf(fFile, STRING_INI_LANGUAGE "=%d\n", g_ivConfig->Language);
	fprintf(fFile, STRING_INI_SHOWMESSAGES "=%d\n", (int)(g_ivConfig->fDisplayShortPop));

	// Folders
	fputs("\n[" STRING_INI_FOLDERS "]\n", fFile);
	const char szFolders[ARRAYSIZE(g_aszDefFolders)][DEFAULT_BUFFER] = {STRING_INI_BRMEMPAK "=%s\n", STRING_INI_BRGBROM "=%s\n", STRING_INI_BRGBSAVE "=%s\n"};
	for (int i = 0; i < ARRAYSIZE(szFolders); i++)
	{
		TCHAR_TO_CHAR( szANSIBuf, g_aszDefFolders[i], DEFAULT_BUFFER );
		fprintf(fFile, szFolders[i], szANSIBuf);
	}

	// lastBrowserDir
	fputs("\n[" STRING_INI_BROWSER "]\n", fFile);
	const char szBrowser[ARRAYSIZE(g_aszLastBrowse)][DEFAULT_BUFFER] = {STRING_INI_BRMEMPAK "=%s\n", STRING_INI_BRGBROM "=%s\n", STRING_INI_BRGBSAVE "=%s\n",
		STRING_INI_BRPROFILE "=%s\n", STRING_INI_BRNOTE "=%s\n", STRING_INI_SHORTCUTS "=%s\n" };
	for (int i = 0; i < ARRAYSIZE(szBrowser); i++)
	{
		TCHAR_TO_CHAR( szANSIBuf, g_aszLastBrowse[i], DEFAULT_BUFFER );
		fprintf(fFile, szBrowser[i], szANSIBuf);
	}

	// Controller 1 through 4
	for (int i = 0; i < 4; i++)
	{
		fprintf(fFile, "\n[" STRING_INI_CONTROLLER " %d]\n", i + 1);
		DumpControllerSettings(fFile, i, true);
	}

	// Controls

	// I'm going to use STL strings here because I don't want to screw with buffer management
	string strMouse;
	string strDevs[MAX_DEVICES];
	string strNull;

	fputs("\n[" STRING_INI_CONTROLS "]\n", fFile);
	fputs("# Button format: controlnum buttonID bOffset bAxisID bBtnType\n", fFile);

	for ( int i = 0; i < 4; i++ ) // controllers for
	{
		FormatControlsBlock(&strMouse, strDevs, &strNull, i);
	} // end controllers for

	DumpStreams(fFile, strMouse, strDevs, strNull, true);

	strMouse.clear();
	for (int i = 0; i < g_nDevices; i++)
		strDevs[i].clear();
	strNull.clear();

	// Shortcuts

	fputs("\n[" STRING_INI_SHORTCUTS "]\n", fFile);
	fputs("# Shortcuts format: controlnum buttonID bOffset bAxisID bBtnType\n", fFile);

	FormatShortcutsBlock(fFile, true);

	// Modifiers
	fputs("\n[" STRING_INI_MODIFIERS "]\n", fFile);
	fputs("# Modifiers format: controlnum bOffset bAxisID bBtnType bModType fToggle fStatus dwSpecific\n", fFile);

	for ( int i = 0; i < 4; i++ ) // controllers for
	{
		FormatModifiersBlock(&strMouse, strDevs, &strNull, i);
	} // end controllers for

	DumpStreams(fFile, strMouse, strDevs, strNull, true);

	fclose(fFile);
	DebugWriteA("Config stored to INI\n");
	return true;
}

// This func loads the config data from INI into working emulator space.  Does not copy into Interface;
// you need to call GetCurrentConfiguration() if you want to do that.
// Returns true if loaded OK, false if there was a problem.
bool LoadConfigFromINI()
{
	FILE *fFile = NULL;
	DWORD dwSection = 0;	// this will eval to the bracketed "[Section]" we are currently in.
	char szLine[4096];

	TCHAR szFilename[MAX_PATH];
	GetDirectory(szFilename, DIRECTORY_CONFIG);
	_tcscat(szFilename, _T("NRage.ini"));
	fFile = _tfopen(szFilename, _T("rS"));	// read, optimize for sequential

	if (!fFile)
	{
		DebugWriteA("Couldn't open INI file for input!\n");
		return false;
	}
	
	for (int i = 0; i < 4; i++)
		SetControllerDefaults( &(g_pcControllers[i]) );

	ProcessKey( PL_RESET, 0, 0, 0, 0, false );
	DWORD dwCommand = PL_NOHIT;
	while(( fgets(szLine, sizeof(szLine) - 1, fFile) ) )
	{
		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		dwCommand = ParseLine( szLine );
		if (dwCommand == PL_NOHIT)
			continue;
		else if (dwCommand == PL_CATEGORY)
			// section changed to szLine
			dwSection = djbHash(szLine);
		else
			ProcessKey( dwCommand, dwSection, szLine, 0, 0, false );
	}

	fclose(fFile);

	return true;
}

// basically a stripped down version of GetConfigFromINI, called at the very beginning to get our language
LANGID GetLanguageFromINI()
{
	FILE *fFile = NULL;
	char szLine[4096];

	TCHAR szFilename[MAX_PATH];
	GetDirectory(szFilename, DIRECTORY_CONFIG);
	_tcscat(szFilename, _T("NRage.ini"));
	fFile = _tfopen(szFilename, _T("rS"));	// read, optimize for sequential

	if (!fFile)
	{
		DebugWriteA("Couldn't open INI file for input!\n");
		return 0;
	}
	
	ProcessKey( PL_RESET, 0, 0, 0, 0, false );
	DWORD dwCommand = PL_NOHIT;
	while(( fgets(szLine, sizeof(szLine) - 1, fFile) ) )
	{
		szLine[strlen(szLine) - 1] = '\0'; // remove newline
		dwCommand = ParseLine( szLine );
		if (dwCommand == CHK_LANGUAGE)
		{
			LANGID lTemp = 0;
            if (sscanf(szLine, "%hu", &lTemp))
			{
				fclose(fFile);
				return lTemp;
			}
		}
	}

	fclose(fFile);
	DebugWriteA("Couldn't find a Language= line in INI file...\n");
	return 0;
}

// both the following functions assume the buffer is big enough
inline void GUIDtoStringA( char * szGUIDbuf, const GUID guid )
{
	_snprintf( szGUIDbuf, GUID_STRINGLENGTH + 1, "{%08.8lX-%04.4hX-%04.4hX-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

inline bool StringtoGUIDA( LPGUID guid, const char * szGUIDbuf )
{
	short unsigned int lastbyte;
	int blah = sscanf(szGUIDbuf, "{%08lX-%04hX-%04hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX}", &guid->Data1, &guid->Data2, &guid->Data3, &guid->Data4[0], &guid->Data4[1], &guid->Data4[2], &guid->Data4[3], &guid->Data4[4], &guid->Data4[5], &guid->Data4[6], &lastbyte);
	if (blah == 11)
	{
        guid->Data4[7] = (BYTE)lastbyte;
		return true;
	}
	else
		return false;
}

// Takes in a file to dump to, and an int i telling which controller's settings to dump. Does not dump buttons or modifiers.
void DumpControllerSettings(FILE * fFile, int i, bool bIsINI)
{
	char szANSIBuf[DEFAULT_BUFFER];

	fprintf(fFile, STRING_INI_PLUGGED "=%u\n", g_ivConfig->Controllers[i].fPlugged);
	fprintf(fFile, STRING_INI_XINPUT "=%u\n", g_ivConfig->Controllers[i].fXInput);
	fprintf(fFile, STRING_INI_N64MOUSE "=%u\n", g_ivConfig->Controllers[i].fN64Mouse);
	fprintf(fFile, STRING_INI_BACKGROUNDINPUT "=%u\n", g_ivConfig->Controllers[i].bBackgroundInput);
	fprintf(fFile, STRING_INI_RAWDATA "=%u\n", g_ivConfig->Controllers[i].fRawData);
	fprintf(fFile, STRING_INI_PAKTYPE "=%u\n", g_ivConfig->Controllers[i].PakType);
	fprintf(fFile, STRING_INI_REALN64RANGE "=%u\n", g_ivConfig->Controllers[i].fRealN64Range);
	fprintf(fFile, STRING_INI_RAPIDFIREENABLED "=%u\n", g_ivConfig->Controllers[i].bRapidFireEnabled);
	fprintf(fFile, STRING_INI_RAPIDFIRERATE "=%u\n", g_ivConfig->Controllers[i].bRapidFireRate);
	fprintf(fFile, STRING_INI_STICKRANGE "=%u\n", g_ivConfig->Controllers[i].bStickRange);
	fprintf(fFile, STRING_INI_MOUSEMOVEX "=%u\n", g_ivConfig->Controllers[i].bMouseMoveX);
	fprintf(fFile, STRING_INI_MOUSEMOVEY "=%u\n", g_ivConfig->Controllers[i].bMouseMoveY);
	fprintf(fFile, STRING_INI_AXISSET "=%u\n", g_ivConfig->Controllers[i].bAxisSet);
	fprintf(fFile, STRING_INI_KEYABSOLUTEX "=%u\n", g_ivConfig->Controllers[i].fKeyAbsoluteX);
	fprintf(fFile, STRING_INI_KEYABSOLUTEY "=%u\n", g_ivConfig->Controllers[i].fKeyAbsoluteY);
	fprintf(fFile, STRING_INI_PADDEADZONE "=%u\n", g_ivConfig->Controllers[i].bPadDeadZone);
	fprintf(fFile, STRING_INI_MOUSESENSX "=%u\n", g_ivConfig->Controllers[i].wMouseSensitivityX);
	fprintf(fFile, STRING_INI_MOUSESENSY "=%u\n", g_ivConfig->Controllers[i].wMouseSensitivityY);
	fprintf(fFile, STRING_INI_RUMBLETYPE "=%u\n", g_ivConfig->Controllers[i].bRumbleTyp);
	fprintf(fFile, STRING_INI_RUMBLESTRENGTH "=%u\n", g_ivConfig->Controllers[i].bRumbleStrength);
	fprintf(fFile, STRING_INI_VISUALRUMBLE "=%u\n", g_ivConfig->Controllers[i].fVisualRumble);

	if (bIsINI)
	{
		char szGUID[DEFAULT_BUFFER];
		int iDevice = FindDeviceinList( g_ivConfig->FFDevices[i].szProductName, g_ivConfig->FFDevices[i].bProductCounter, true );
		if (iDevice == -1)
		{
			fprintf(fFile, STRING_INI_FFDEVICEGUID "=\n");
		}
		else
		{
			g_ivConfig->Controllers[i].guidFFDevice = g_devList[iDevice].guidInstance;
			GUIDtoStringA(szGUID, g_ivConfig->Controllers[i].guidFFDevice);
			fprintf(fFile, STRING_INI_FFDEVICEGUID "=%s\n", szGUID);
		}
	}
	else
	{
		TCHAR_TO_CHAR(szANSIBuf, g_ivConfig->FFDevices[i].szProductName, DEFAULT_BUFFER);
		fprintf(fFile, STRING_INI_FFDEVICENAME "=%s\n", szANSIBuf);
		fprintf(fFile, STRING_INI_FFDEVICENR "=%u\n", g_ivConfig->FFDevices[i].bProductCounter);
	}

	TCHAR_TO_CHAR( szANSIBuf, g_ivConfig->Controllers[i].szMempakFile, DEFAULT_BUFFER );
	fprintf(fFile, STRING_INI_MEMPAKFILE "=%s\n", szANSIBuf);

	TCHAR_TO_CHAR( szANSIBuf, g_ivConfig->Controllers[i].szTransferRom, DEFAULT_BUFFER );
	fprintf(fFile, STRING_INI_GBROMFILE "=%s\n", szANSIBuf);

	TCHAR_TO_CHAR( szANSIBuf, g_ivConfig->Controllers[i].szTransferSave, DEFAULT_BUFFER );
	fprintf(fFile, STRING_INI_GBROMSAVE "=%s\n", szANSIBuf);
}

// private func, called by StoreConfigToINI to dump cached Button strings to file
void DumpStreams(FILE * fFile, string strMouse, string strDevs[], string strNull, bool bIsINI)
{
	// dump all streams to file, with appropriate DInput lines and name comment
	char szANSIBuf[DEFAULT_BUFFER];
	if (!(strMouse.empty()))
	{
		TCHAR_TO_CHAR( szANSIBuf, g_sysMouse.szProductName, DEFAULT_BUFFER );
		if (bIsINI)
		{
			fprintf(fFile, "# %s\n", szANSIBuf);
			char szGUID[DEFAULT_BUFFER];
			GUIDtoStringA(szGUID, g_sysMouse.guidInstance);
			fprintf(fFile, STRING_INI_DINPUTGUID "=%s\n", szGUID);
		}
		else
		{
			fprintf(fFile, STRING_INI_DINPUTNAME "=%s\n", szANSIBuf);
			fprintf(fFile, STRING_INI_DINPUTNR "=%d\n", 0);
		}
		fputs(strMouse.c_str(), fFile);
	}
	if (!(strNull.empty()))
	{
		fputs("# NOT ASSIGNED\n", fFile);
		if (bIsINI)
		{
			char szGUID[DEFAULT_BUFFER];
			GUIDtoStringA(szGUID, GUID_NULL);
			fprintf(fFile, STRING_INI_DINPUTGUID "=%s\n", szGUID);
		}
		else
		{
			fprintf(fFile, STRING_INI_DINPUTNAME "=\n");	// leave blank
			fprintf(fFile, STRING_INI_DINPUTNR "=\n");
		}
		fputs(strNull.c_str(), fFile);
	}
	for (int i = 0; i < g_nDevices; i++)
	{
		if (!(strDevs[i].empty()))
			{
				TCHAR_TO_CHAR( szANSIBuf, g_devList[i].szProductName, DEFAULT_BUFFER );
				if (bIsINI)
				{
					if (g_devList[i].bProductCounter > 0)
						fprintf(fFile, "# %s %d\n", szANSIBuf, g_devList[i].bProductCounter);
					else
						fprintf(fFile, "# %s\n", szANSIBuf);
					char szGUID[DEFAULT_BUFFER];
					GUIDtoStringA(szGUID, g_devList[i].guidInstance);
					fprintf(fFile, STRING_INI_DINPUTGUID "=%s\n", szGUID);
				}
				else
				{
					fprintf(fFile, STRING_INI_DINPUTNAME "=%s\n", szANSIBuf);
					fprintf(fFile, STRING_INI_DINPUTNR "=%d\n", g_devList[i].bProductCounter);
				}
				fputs(strDevs[i].c_str(), fFile);
			}
	}
}

void FormatControlsBlock(string * strMouse, string strDevs[], string * strNull, int i)
{
	for ( int j = 0; j < ARRAYSIZE(g_ivConfig->Controllers[i].aButton); j++ ) // buttons for
	{
		if (g_ivConfig->Controllers[i].aButton[j].parentDevice) // possibly unbound
		{
			if ( IsEqualGUID(g_sysMouse.guidInstance, g_ivConfig->Controllers[i].aButton[j].parentDevice->guidInstance) )
			{
				char szBuf[DEFAULT_BUFFER];
				// add to the mouse stream
				sprintf(szBuf, STRING_INI_BUTTON "=%d %d %02X %d %d\n", i, j, g_ivConfig->Controllers[i].aButton[j].bOffset, g_ivConfig->Controllers[i].aButton[j].bAxisID, g_ivConfig->Controllers[i].aButton[j].bBtnType);
				strMouse->append(szBuf);
			}
			else
			{
				for (int match = 0; match < g_nDevices; match++)
				{
					if ( IsEqualGUID(g_devList[match].guidInstance, g_ivConfig->Controllers[i].aButton[j].parentDevice->guidInstance) )
					{
						char szBuf[DEFAULT_BUFFER];
						// add to the appropriate device stream
						sprintf(szBuf, STRING_INI_BUTTON "=%d %d %02X %d %d\n", i, j, g_ivConfig->Controllers[i].aButton[j].bOffset, g_ivConfig->Controllers[i].aButton[j].bAxisID, g_ivConfig->Controllers[i].aButton[j].bBtnType);
						strDevs[match].append(szBuf);
						break;
					}
				}
			}
		}
		else if (g_ivConfig->Controllers[i].aButton[j].bBtnType != DT_UNASSIGNED)
		{
			int k = g_ivConfig->Controllers[i].aButton[j].bBtnType;
			DebugWriteA("Controller %d button %d is of bBtnType %d but has no parentDevice!\n", i, j, k);
		}
	} // end buttons for
}

void FormatModifiersBlock(string * strMouse, string strDevs[], string * strNull, int i)
{
	for ( int j = 0; j < g_ivConfig->Controllers[i].nModifiers; j++ )
	{
		if (g_ivConfig->Controllers[i].pModifiers[j].btnButton.parentDevice) // is it assigned to a key?
		{
			if ( IsEqualGUID(g_sysMouse.guidInstance, g_ivConfig->Controllers[i].pModifiers[j].btnButton.parentDevice->guidInstance) )
			{
				char szBuf[DEFAULT_BUFFER];
				// add to the mouse stream
				sprintf(szBuf, STRING_INI_MODIFIER "=%d %02X %d %d %d %d %d %08X\n", i, g_ivConfig->Controllers[i].pModifiers[j].btnButton.bOffset, 
					g_ivConfig->Controllers[i].pModifiers[j].btnButton.bAxisID, g_ivConfig->Controllers[i].pModifiers[j].btnButton.bBtnType,
					g_ivConfig->Controllers[i].pModifiers[j].bModType, g_ivConfig->Controllers[i].pModifiers[j].fToggle,
					g_ivConfig->Controllers[i].pModifiers[j].fStatus, g_ivConfig->Controllers[i].pModifiers[j].dwSpecific);
				strMouse->append(szBuf);
			}
			else
				for (int match = 0; match < g_nDevices; match++)
					if ( IsEqualGUID(g_devList[match].guidInstance, g_ivConfig->Controllers[i].pModifiers[j].btnButton.parentDevice->guidInstance) )
					{
						char szBuf[DEFAULT_BUFFER];
						// add to the mouse stream
						sprintf(szBuf, STRING_INI_MODIFIER "=%d %02X %d %d %d %d %d %08X\n", i, g_ivConfig->Controllers[i].pModifiers[j].btnButton.bOffset, 
							g_ivConfig->Controllers[i].pModifiers[j].btnButton.bAxisID, g_ivConfig->Controllers[i].pModifiers[j].btnButton.bBtnType,
							g_ivConfig->Controllers[i].pModifiers[j].bModType, g_ivConfig->Controllers[i].pModifiers[j].fToggle,
							g_ivConfig->Controllers[i].pModifiers[j].fStatus, g_ivConfig->Controllers[i].pModifiers[j].dwSpecific);
						strDevs[match].append(szBuf);
						break;
					}
		}
		else // save modifiers without a keybind
		{
			char szBuf[DEFAULT_BUFFER];
			// add to the mouse stream
			sprintf(szBuf, STRING_INI_MODIFIER "=%d %02X %d %d %d %d %d %08X\n", i, g_ivConfig->Controllers[i].pModifiers[j].btnButton.bOffset, 
				g_ivConfig->Controllers[i].pModifiers[j].btnButton.bAxisID, g_ivConfig->Controllers[i].pModifiers[j].btnButton.bBtnType,
				g_ivConfig->Controllers[i].pModifiers[j].bModType, g_ivConfig->Controllers[i].pModifiers[j].fToggle,
				g_ivConfig->Controllers[i].pModifiers[j].fStatus, g_ivConfig->Controllers[i].pModifiers[j].dwSpecific);
			strNull->append(szBuf);
		}
	}
}

unsigned long djbHash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
