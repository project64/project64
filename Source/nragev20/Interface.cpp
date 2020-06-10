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

#include <stdio.h>

#include <windows.h>
#include <commctrl.h>

#include "commonIncludes.h"
#include "DirectInput.h"
#include "FileAccess.h"
#include "Interface.h"
#include "International.h"
#include "NRagePluginV2.h"
#include "PakIO.h"
#include "Version.h"
#include "XInputController.h"
#include <Common\StdString.h>

// Prototypes //
BOOL CALLBACK ControllerTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK ControlsTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK XControlsTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );   // Xinput Control main proc. --tecnicors
BOOL CALLBACK DevicesTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK ModifierTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK ControllerPakTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK PakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK MemPakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK RumblePakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK TransferPakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK ShortcutsTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK FoldersDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK BlockerProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
BOOL InitOverlay();
HWND MakeOverlay();

// BOOL CALLBACK EnumGetKeyDesc( LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef ); // nope, sorry. --rabid
bool GetButtonID( LPDWORD ButtonID, const BYTE bIndex, const BYTE bButtonSet );
bool GetButtonText( const BUTTON& btnButton, LPTSTR Buffer);
DWORD ScanDevices( LPDWORD lpdwCounter, LPBUTTON pButton );

void DeleteControllerSettings( int indexController );
void GetCurrentConfiguration();
void UpdateControllerStructures();

// global Variables //
INTERFACEVALUES *g_ivConfig = NULL;                 // this structure holds all GUI-Data; it points to a valid struct only if the config window is open
LPDIRECTINPUTDEVICE8 g_pConfigDevice = NULL;        // one device handle for current FF device; between messages so it needs to be persistent
LPDIRECTINPUTEFFECT  g_pConfigEffect = NULL;        // FF-effect handle
HWND g_hMainDialog = NULL;                          // handle of base-dialog

// Main dialog control handler
BOOL CALLBACK MainDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static HWND hTabControl;
    HWND hDlgItem;
    long i,j;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            DebugWriteA("Config interface: Open\n");
            g_ivConfig = (INTERFACEVALUES*)P_malloc( sizeof(INTERFACEVALUES) ); // TODO: CHECK THIS MALLOC!
            ZeroMemory( g_ivConfig, sizeof(INTERFACEVALUES) );

            CopyMemory( &g_ivConfig->Shortcuts, &g_scShortcuts ,sizeof(SHORTCUTS) );
            for( i = 0; i < 4; ++i )
                DeleteControllerSettings( i );

            EnterCriticalSection( &g_critical );    // block because the InitiateControllers code may still be running.
            if( !g_strEmuInfo.fInitialisedPlugin )
            {
                LoadConfigFromINI();
            }

            GetCurrentConfiguration();

            LeaveCriticalSection( &g_critical );


            g_hMainDialog = hDlg;

            // Display Version
            {
                TCHAR tszBuffer[DEFAULT_BUFFER], tszMsg[DEFAULT_BUFFER / 2];

                LoadString( g_hResourceDLL, IDS_VERSTRING, tszBuffer, DEFAULT_BUFFER / 2);
                _stprintf(tszMsg, tszBuffer, stdstr(VER_FILE_VERSION_STR).ToUTF16().c_str());
                SetDlgItemText( hDlg, IDC_VERSIONSTRING, tszMsg );
            }


            // Tab - Control //
            hTabControl = NULL;
            hDlgItem = GetDlgItem( hDlg, IDC_UPPERTAB );

            TCITEM tcItem;
            tcItem.mask = TCIF_TEXT;

            TCHAR tszText[DEFAULT_BUFFER], tszBuff[DEFAULT_BUFFER];
            tcItem.pszText = tszText;

            LoadString( g_hResourceDLL, IDS_TAB_CONTROLLER, tszBuff, DEFAULT_BUFFER );

            for (int i = 0; i < 4; i++ )
            {
                wsprintf( tszText, tszBuff, i + 1 );
                TabCtrl_InsertItem( hDlgItem, i, &tcItem );
            }

            LoadString( g_hResourceDLL, IDS_TAB_SHORTCUTS, tszText, DEFAULT_BUFFER );
            TabCtrl_InsertItem( hDlgItem, TAB_SHORTCUTS, &tcItem );
            // Tab - Control  End //

            g_ivConfig->ChosenTab = TAB_CONTROLLER1;
            TabCtrl_SetCurSel( GetDlgItem( hDlg, IDC_UPPERTAB), g_ivConfig->ChosenTab );
            NMHDR nmInit;
            nmInit.hwndFrom = hDlgItem;
            nmInit.idFrom = IDC_UPPERTAB;
            nmInit.code = TCN_SELCHANGE;
            MainDlgProc( hDlg, WM_NOTIFY, IDC_UPPERTAB, (LPARAM)&nmInit );

            // we can't click "Use" without saving when not emulating, because the emu resets controls before loading a ROM
            if( !g_bRunning )
                EnableWindow( GetDlgItem( hDlg, IDUSE ), FALSE );

#ifndef _UNICODE
            EnableWindow( GetDlgItem( hDlg, IDC_LANGUAGE), FALSE );
#endif // #ifdef _UNICODE
            InitOverlay();
        }

        // call routine to show content( recursive call )
        MainDlgProc( hDlg, WM_USER_UPDATE, 0, 0 );
        return TRUE; // the system sets focus to one control element

    case WM_NOTIFY:
        hDlgItem = ((LPNMHDR)lParam)->hwndFrom;
        i = ((LPNMHDR)lParam)->code;
        j = ((LPNMHDR)lParam)->idFrom;

        switch( j )
        {
        case IDC_UPPERTAB:
            if( i == TCN_SELCHANGE )
            {
                i = g_ivConfig->ChosenTab;
                g_ivConfig->ChosenTab = TabCtrl_GetCurSel( hDlgItem );

                if( hTabControl )
                {
                    if( (( i == TAB_CONTROLLER1 ) || ( i == TAB_CONTROLLER2 ) || ( i == TAB_CONTROLLER3 ) || ( i == TAB_CONTROLLER4 )) &&
                        (( g_ivConfig->ChosenTab == TAB_CONTROLLER1 ) || ( g_ivConfig->ChosenTab == TAB_CONTROLLER2 ) || ( g_ivConfig->ChosenTab == TAB_CONTROLLER3 ) || ( g_ivConfig->ChosenTab == TAB_CONTROLLER4 )))
                    {
                        MainDlgProc( hDlg, WM_USER_UPDATE, 0, 0 );
                        return TRUE;
                    }
                    else
                        DestroyWindow( hTabControl );
                }

                switch( g_ivConfig->ChosenTab )
                {
                case TAB_CONTROLLER1:
                case TAB_CONTROLLER2:
                case TAB_CONTROLLER3:
                case TAB_CONTROLLER4:
                    hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_CONTROLLER), hDlg, (DLGPROC)ControllerTabProc);
                    break;
                case TAB_SHORTCUTS:
                    hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_SHORTCUT), hDlg, (DLGPROC)ShortcutsTabProc);
                    break;
                default:
                    hTabControl = NULL;
                    return TRUE;
                }

                RECT rectWindow,
                    rectTab, rectMain; // need to know the position of the calling tab window relative to its parent

                GetWindowRect( hDlg, &rectMain );
                GetWindowRect( hDlgItem, &rectTab );

                GetClientRect( hDlgItem, &rectWindow );
                TabCtrl_AdjustRect( hDlgItem, FALSE, &rectWindow );

                // second, third param: plus Tabwindow's position relative to main
                MoveWindow( hTabControl, (rectTab.left - rectMain.left), (rectTab.top - rectMain.top), rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, FALSE );
                ShowWindow( hTabControl, SW_SHOW );
            }
            return TRUE;
        default:
            return FALSE;
        }

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );

        switch( LOWORD(wParam) )
        {
        case IDSAVE:
            if( hTabControl )
            {
                DestroyWindow( hTabControl );
                hTabControl = NULL;
            }
            DebugWriteA("Config interface: Save\n");
            StoreConfigToINI();
            // NO BREAK-- this is intentional
        case IDUSE:
            if( hTabControl )
            {
                DestroyWindow( hTabControl );
                hTabControl = NULL;
            }
            DebugWriteA("Config interface: Use\n");
            CopyMemory( &g_scShortcuts ,&g_ivConfig->Shortcuts, sizeof(SHORTCUTS) );

            UpdateControllerStructures();

            EndDialog( hDlg, TRUE );
            return TRUE;
        case IDCANCEL:
            if( hTabControl )
            {
                DestroyWindow( hTabControl );
                hTabControl = NULL;
            }
            EndDialog( hDlg, FALSE );
            return TRUE;

        case IDC_LANGUAGE:
            if( HIWORD(wParam) == CBN_SELCHANGE )
            {
                i = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
                g_ivConfig->Language = (LANGID) SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );
            }
            return TRUE;

        case IDC_STOREINDB:
        case IDC_UPDATEDB:
        case IDC_ERASEFROMDB:
        case IDC_SAVEPROFILE:
        case IDC_LOADPROFILE:
            // we don't have handlers for these, so fall through to FALSE
        default:
            return FALSE;
        }

    case WM_CLOSE:
        if( hTabControl )
        {
            DestroyWindow( hTabControl );
            hTabControl = NULL;
        }
        EndDialog( hDlg, FALSE );
        return TRUE;

    case WM_DESTROY:
        if( hTabControl )
        {
            DestroyWindow( hTabControl );
            hTabControl = NULL;
        }
        EnterCriticalSection(&g_critical);
        ReleaseEffect( g_pConfigEffect );
        ReleaseDevice( g_pConfigDevice );

        for( i = 0; i < 4; ++i )
        {
            freePakData( &g_ivConfig->Controllers[i] );
            freeModifiers( &g_ivConfig->Controllers[i] );
        }
        P_free( g_ivConfig );
        g_ivConfig = NULL;
        DebugWriteA("Config interface: Closed\n");
        LeaveCriticalSection(&g_critical);
        return TRUE;

    case WM_USER_UPDATE:
#ifdef _UNICODE
        // filling DropDownlist with languages
        hDlgItem = GetDlgItem( hDlg, IDC_LANGUAGE );
        SendMessage (hDlgItem, CB_RESETCONTENT, 0, 0);

        {
            TCHAR szBuffer[MAX_PATH];
            TCHAR * pSlash;
            WIN32_FIND_DATA fData;
            long lLangFound = 0; // the index matching our current language

            VerLanguageName(0x0009, szBuffer, DEFAULT_BUFFER); // English (default resource)
            j = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)szBuffer );
            SendMessage( hDlgItem, CB_SETITEMDATA, j, 0 );

            GetModuleFileName( NULL, szBuffer, MAX_PATH );
            pSlash = _tcsrchr( szBuffer, '\\' );
            _tcscpy(pSlash + 1, _T("NRage-Language-*.dll"));

            // Search for any file available called NRage-Language-XXXX.dll (where X is numbers)
            HANDLE fSearch = FindFirstFile(szBuffer, &fData);

            while ( fSearch != INVALID_HANDLE_VALUE )
            {
                DWORD dwLangID = 0;
                if (_stscanf(fData.cFileName, _T("NRage-Language-%u.dll"), &dwLangID))
                {
                    VerLanguageName(dwLangID, szBuffer, DEFAULT_BUFFER);
                    j = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)szBuffer );
                    SendMessage( hDlgItem, CB_SETITEMDATA, j, dwLangID );
                    if (dwLangID == g_ivConfig->Language)
                        lLangFound = j;
                }
                if (!FindNextFile(fSearch, &fData))
                    fSearch = INVALID_HANDLE_VALUE;
            }
            FindClose(fSearch);

            SendMessage( hDlgItem, CB_SETCURSEL, lLangFound, 0 ); // set combo box selection
        }
        // DropDownlist End
#else
        hDlgItem = GetDlgItem( hDlg, IDC_LANGUAGE );
        SendMessage(hDlgItem, CB_RESETCONTENT, 0, 0);
        j = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM) _T("Language selection disabled"));
        SendMessage( hDlgItem, CB_SETITEMDATA, j, 0 );
        SendMessage(hDlgItem, CB_SETCURSEL, 0, 0);
#endif // #ifdef _UNICODE
        // call child-dialog(s) to update their content as well
        if( hTabControl )
            SendMessage( hTabControl, WM_USER_UPDATE, 0, 0 );
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
//  return TRUE; //msg got processed
}

BOOL CALLBACK ControllerTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static HWND hTabControl;

    CONTROLLER *pcController = &g_ivConfig->Controllers[g_ivConfig->ChosenTab];;
    HWND hDlgItem;
    long i,j;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        hTabControl = NULL;

        // Tab - Control //
        hDlgItem = GetDlgItem( hDlg, IDC_CONTROLLERTAB );

        TCITEM tcItem;
        tcItem.mask = TCIF_TEXT;

        TCHAR tszText[DEFAULT_BUFFER];
        tcItem.pszText = tszText;

        LoadString( g_hResourceDLL, IDS_TAB_CONTROLS, tszText, DEFAULT_BUFFER );
        TabCtrl_InsertItem( hDlgItem, TAB_CONTROLS, &tcItem );
        LoadString( g_hResourceDLL, IDS_TAB_DEVICES, tszText, DEFAULT_BUFFER );
        TabCtrl_InsertItem( hDlgItem, TAB_DEVICES, &tcItem );
        LoadString( g_hResourceDLL, IDS_TAB_MODIFIERS, tszText, DEFAULT_BUFFER );
        TabCtrl_InsertItem( hDlgItem, TAB_MODIFIERS, &tcItem );
        LoadString( g_hResourceDLL, IDS_TAB_CONTROLLERPAK, tszText, DEFAULT_BUFFER );
        TabCtrl_InsertItem( hDlgItem, TAB_PAK, &tcItem );

        NMHDR nmInit;
        nmInit.hwndFrom = hDlgItem;
        nmInit.idFrom = IDC_CONTROLLERTAB;
        nmInit.code = TCN_SELCHANGE;

        // initiate Tab-Display
        ControllerTabProc( hDlg, WM_NOTIFY, IDC_CONTROLLERTAB, (LPARAM)&nmInit );
        // Tab - Control End //

        // call routine to show content( recursive call )
        ControllerTabProc( hDlg, WM_USER_UPDATE, 0, 0 );


        return FALSE; // don't give it focus

    case WM_NOTIFY:
        hDlgItem = ((LPNMHDR)lParam)->hwndFrom;
        i = ((LPNMHDR)lParam)->code;
        j = ((LPNMHDR)lParam)->idFrom;

        switch( j )
        {
            case IDC_CONTROLLERTAB:
                if( i == TCN_SELCHANGE )
                {
                    if( hTabControl )
                        DestroyWindow( hTabControl );

                    i = TabCtrl_GetCurSel( hDlgItem );

                    switch( i )
                    {
                    case TAB_CONTROLS:
                        if( pcController->fXInput)  // added to show the xinput controller config tab --tecnicors
                            hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_XCONTROLS), hDlg, (DLGPROC)XControlsTabProc);
                        else
                            hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_CONTROLS), hDlg, (DLGPROC)ControlsTabProc);
                        break;
                    case TAB_DEVICES:
                        hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_DEVICES), hDlg, (DLGPROC)DevicesTabProc);
                        break;
                    case TAB_MODIFIERS:
                        hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_MODIFIER), hDlg, (DLGPROC)ModifierTabProc);
                        break;
                    case TAB_PAK:
                        hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_CONTROLLERPAK), hDlg, (DLGPROC)ControllerPakTabProc);
                        break;
                    default:
                        hTabControl = NULL;
                        return TRUE;
                    }
                    hDlgItem = GetDlgItem( hDlg, IDC_CONTROLLERTAB );

                    RECT rectWindow,
                        rectTab, rectMain; // need to know the position of the calling tab window relative to its parent

                    GetWindowRect( hDlg, &rectMain );
                    GetWindowRect( hDlgItem, &rectTab );

                    GetClientRect( hDlgItem, &rectWindow );
                    TabCtrl_AdjustRect( hDlgItem, FALSE, &rectWindow );

                    MoveWindow( hTabControl, rectWindow.left + (rectTab.left - rectMain.left), rectWindow.top + (rectTab.top - rectMain.top), rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, FALSE );
                    ShowWindow( hTabControl, SW_SHOW );
                    return TRUE;
                }
                else
                    return FALSE;
            default:
                return FALSE;
        }

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );

        switch( LOWORD(wParam) )
        {
            TCHAR tszTitle[DEFAULT_BUFFER], tszText[DEFAULT_BUFFER];
            case IDC_PLUGGED:
                pcController->fPlugged = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
                return TRUE;
            case IDC_CLEARCONTROLLER:
                LoadString( g_hResourceDLL, IDS_DLG_CLEAR, tszText, DEFAULT_BUFFER );
                LoadString( g_hResourceDLL, IDS_DLG_CLEAR_TITLE, tszTitle, DEFAULT_BUFFER );
                if( MessageBox( hDlg, tszText, tszTitle, MB_OKCANCEL | MB_ICONWARNING ) == IDOK )
                {
                    DeleteControllerSettings( g_ivConfig->ChosenTab );
                    ControllerTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
                }
                return TRUE;
            case IDC_SETDEFAULT:
                LoadString( g_hResourceDLL, IDS_DLG_CONTROLRESTORE, tszText, DEFAULT_BUFFER );
                LoadString( g_hResourceDLL, IDS_DLG_CONTROLRESTORE_TITLE, tszTitle, DEFAULT_BUFFER );
                if( MessageBox( hDlg, tszText, tszTitle, MB_OKCANCEL | MB_ICONWARNING ) == IDOK )
                {
                    DeleteControllerSettings( g_ivConfig->ChosenTab );
                    LoadProfileFromResource( g_ivConfig->ChosenTab, true);
                    *(g_ivConfig->FFDevices[g_ivConfig->ChosenTab].szProductName) = '\0'; // default profile has no FF device
                    g_ivConfig->FFDevices[g_ivConfig->ChosenTab].bProductCounter = 0;
                    ControllerTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
                }
                return TRUE;

            case IDC_SAVEPROFILE:
                {
                    TCHAR szFilename[MAX_PATH+1] = _T("");
                    if( BrowseFile( hDlg, szFilename, BF_PROFILE, BF_SAVE ))
                    {
                        FILE * fFile = _tfopen( szFilename, _T("wS") );
                        if (fFile)
                        {
                            fprintf(fFile, "@" STRING_PROFILEVERSION "\n\n");
                            FormatProfileBlock( fFile, g_ivConfig->ChosenTab );

                            fclose(fFile);
                        }
                        else
                            WarningMessage( IDS_ERR_PROFWRITE, MB_OK );
                    }
                }
                return TRUE;

            case IDC_LOADPROFILE:
                {
                    TCHAR szFilename[MAX_PATH+1] = TEXT( "" );
                    if( BrowseFile( hDlg, szFilename, BF_PROFILE, BF_LOAD ))
                    {
                        DebugWrite(_T("Config interface: Load shortcuts file: %s\n"), szFilename);
                        if( LoadProfileFile( szFilename, g_ivConfig->ChosenTab, g_ivConfig->FFDevices[g_ivConfig->ChosenTab].szProductName, &g_ivConfig->FFDevices[g_ivConfig->ChosenTab].bProductCounter ))
                            ControllerTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
                        else
                            WarningMessage( IDS_ERR_PROFREAD, MB_OK );

                    }
                }
                return TRUE;

            case IDC_XINPUT_ENABLER:    // change to xinput config --tecnicors
                pcController->fXInput = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
                if( hTabControl )
                        DestroyWindow( hTabControl );
                if( pcController->fXInput )
                    hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_XCONTROLS), hDlg, (DLGPROC)XControlsTabProc);
                else
                    hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_CONTROLS), hDlg, (DLGPROC)ControlsTabProc);
                {
                    hDlgItem = GetDlgItem( hDlg, IDC_CONTROLLERTAB );

                    RECT rectWindow,
                        rectTab, rectMain; // need to know the position of the calling tab window relative to its parent

                    GetWindowRect( hDlg, &rectMain );
                    GetWindowRect( hDlgItem, &rectTab );

                    GetClientRect( hDlgItem, &rectWindow );
                    TabCtrl_AdjustRect( hDlgItem, FALSE, &rectWindow );

                    MoveWindow( hTabControl, rectWindow.left + (rectTab.left - rectMain.left), rectWindow.top + (rectTab.top - rectMain.top), rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, FALSE );
                    ShowWindow( hTabControl, SW_SHOW );
                }
                return TRUE;    // END IDC_XINPUT_ENABLER

            case IDC_N64MOUSE:
                pcController->fN64Mouse = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
                return TRUE;

            case IDC_BACKGROUNDINPUT:
                pcController->bBackgroundInput = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
                return TRUE;

            default:
                return FALSE;
        }

    case WM_USER_UPDATE:
        // for this dialog
        CheckDlgButton( hDlg, IDC_PLUGGED, pcController->fPlugged ? BST_CHECKED : BST_UNCHECKED );
        CheckDlgButton( hDlg, IDC_XINPUT_ENABLER, pcController->fXInput ? BST_CHECKED : BST_UNCHECKED );
        CheckDlgButton( hDlg, IDC_N64MOUSE, pcController->fN64Mouse ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton( hDlg, IDC_BACKGROUNDINPUT, pcController->bBackgroundInput ? BST_CHECKED : BST_UNCHECKED);

        if( hTabControl )
            DestroyWindow( hTabControl );

        if( pcController->fXInput )
            hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_XCONTROLS), hDlg, (DLGPROC)XControlsTabProc);
        else
            hTabControl = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_CONTROLS), hDlg, (DLGPROC)ControlsTabProc);

        {
            hDlgItem = GetDlgItem( hDlg, IDC_CONTROLLERTAB );

            RECT rectWindow,
                rectTab, rectMain; // need to know the position of the calling tab window relative to its parent

            GetWindowRect( hDlg, &rectMain );
            GetWindowRect( hDlgItem, &rectTab );

            GetClientRect( hDlgItem, &rectWindow );
            TabCtrl_AdjustRect( hDlgItem, FALSE, &rectWindow );

            MoveWindow( hTabControl, rectWindow.left + (rectTab.left - rectMain.left), rectWindow.top + (rectTab.top - rectMain.top), rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, FALSE );
            ShowWindow( hTabControl, SW_SHOW );
        }

        // call child-dialog(s) to update their content as well
        if( hTabControl )
            SendMessage( hTabControl, WM_USER_UPDATE, 0, 0 );
        return TRUE;
    default:
        return FALSE; //false means the msg didn't got processed
    }
}

// "Controls" tab
BOOL CALLBACK ControlsTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static LPBUTTON aButtons = NULL;    // should point to g_ivConfig->Controllers[g_ivConfig->ChosenTab].aButton array
    static DWORD dwButtonID[3];         // 0:ID of PushButton   1:ID of TextWindow  2:offset in aButtons struct
    static DWORD dwCounter;
    static bool bScanRunning;
    static HWND hFocus = NULL;
    static HWND hBlocker = NULL;

    TCHAR szBuffer[40], szTemp[40];
    HWND hDlgItem;
    long i;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        // SetTicks on TrackBar
        hDlgItem = GetDlgItem( hDlg, IDC_CTRRANGE );
        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 100 ));
        SendMessage( hDlgItem, TBM_SETTICFREQ, (WPARAM) 10, 0 );
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );

        // SetTicks on RapidFire Bar
        hDlgItem = GetDlgItem( hDlg, IDC_RAPIDFIRERATE );
        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 32 ));
        SendMessage( hDlgItem, TBM_SETTICFREQ, (WPARAM) 1, 0 );
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );

        // call routine to show content( recursive call )
        ControlsTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
        return FALSE; // don't give it focus

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );

        switch( LOWORD(wParam) )
        {
        case IDC_N64RANGE:
            g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRealN64Range = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
            return TRUE;

        case IDC_RAPIDFIREENABLE:
            g_ivConfig->Controllers[g_ivConfig->ChosenTab].bRapidFireEnabled = (IsDlgButtonChecked( hDlg, LOWORD(wParam)) == BST_CHECKED);
            return TRUE;

        case IDC_CONFIG1:
        case IDC_CONFIG2:
        case IDC_CONFIG3:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                if( LOWORD(wParam) == IDC_CONFIG1 )
                    g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet = 0;
                else if( LOWORD(wParam) == IDC_CONFIG2 )
                    g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet = 1;
                else if( LOWORD(wParam) == IDC_CONFIG3 )
                    g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet = 2;

                ControlsTabProc( hDlg, WM_USER_UPDATE, 1, 0 );
            }
            return TRUE;

        case IDC_LEFTTRIGGER:
        case IDC_RIGHTTRIGGER:
        case IDC_ZTRIGGER:
        case IDC_DUP:
        case IDC_DLEFT:
        case IDC_DRIGHT:
        case IDC_DDOWN:
        case IDC_AUP:
        case IDC_ALEFT:
        case IDC_ARIGHT:
        case IDC_ADOWN:
        case IDC_CUP:
        case IDC_CLEFT:
        case IDC_CRIGHT:
        case IDC_CDOWN:
        case IDC_ABUTTON:
        case IDC_BBUTTON:
        case IDC_SBUTTON: // any of these cases means user wants to assign a button
            if( bScanRunning )
            {   ; // do nothing
/*              bScanRunning = false;
                KillTimer( hDlg, TIMER_BUTTON );
                GetButtonText( aButtons[dwButtonID[2]], szBuffer );
                SendMessage( GetDlgItem( hDlg, dwButtonID[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer ); */
            }
            else if( HIWORD(wParam) == BN_CLICKED )
            {
                EnterCriticalSection(&g_critical);
                dwButtonID[0] = LOWORD(wParam);
                dwCounter = 0;
                GetButtonID( dwButtonID, 0, BSET_CONTROLS );
                if( dwButtonID[2] >= PF_APADR )
                    dwButtonID[2] += g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet * 4;
                if (hFocus == NULL)
                    hFocus = SetFocus( NULL );
                hBlocker = MakeOverlay();

                SetTimer( hDlg, TIMER_BUTTON, INTERVAL_BUTTON, NULL );
                bScanRunning = true;
                LeaveCriticalSection(&g_critical);
            }
            return TRUE;

        default:
            return FALSE;
        }

    case WM_TIMER: // when assigning buttons, this gets called every 20ms (or value in INTERVAL_BUTTON)
        if( wParam == TIMER_BUTTON && bScanRunning )
        {
            BUTTON newButton;

            i = ScanDevices( &dwCounter, &newButton);
            if( i || dwCounter > 500 )
            {
                bScanRunning = false;
                KillTimer( hDlg, TIMER_BUTTON );

                EnterCriticalSection(&g_critical);
                if( i == SC_SCANESCAPE ) // Scan aborted
                    ZeroMemory(&aButtons[dwButtonID[2]], sizeof(BUTTON)); //aButtons[dwButtonID[2]].dwButton = 0;
                else if( i == SC_SCANSUCCEED  ) // Got a key, mouseclick, joybutton, or axis
                    aButtons[dwButtonID[2]] = newButton;
                DestroyWindow( hBlocker );

                LeaveCriticalSection(&g_critical);

                GetButtonText( aButtons[dwButtonID[2]], szBuffer );
                SendMessage( GetDlgItem( hDlg, dwButtonID[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
                if( hFocus != NULL )
                {
                    SetFocus( hFocus );
                    hFocus = NULL;
                }
            }
            else
            {
                if(( dwCounter % 50 ) == 0 )
                {
                    TCHAR tszText[DEFAULT_BUFFER];

                    LoadString( g_hResourceDLL, IDS_C_POLLING, tszText, DEFAULT_BUFFER );
                    wsprintf( szBuffer, tszText, 10 - dwCounter / 50 );
                    SendMessage( GetDlgItem( hDlg, dwButtonID[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
                }
                ++dwCounter;
            }
            return TRUE;
        }
        else
            return FALSE;

    case WM_HSCROLL: // TrackBars
    case WM_VSCROLL:
        switch ( GetWindowLong( (HWND)lParam, GWL_ID ) )
        {
        case IDC_CTRRANGE:
            TCHAR tszText[DEFAULT_BUFFER];

            LoadString( g_hResourceDLL, IDS_C_RANGE, tszText, DEFAULT_BUFFER );
            g_ivConfig->Controllers[g_ivConfig->ChosenTab].bStickRange = (BYTE)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            wsprintf( szBuffer, tszText, g_ivConfig->Controllers[g_ivConfig->ChosenTab].bStickRange );
            SendMessage( GetDlgItem( hDlg, IDT_RANGE ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            return TRUE;
        case IDC_RAPIDFIRERATE:
            g_ivConfig->Controllers[g_ivConfig->ChosenTab].bRapidFireRate = 33 - (BYTE)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            return TRUE;
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        bScanRunning = false;
        KillTimer( hDlg, TIMER_BUTTON );
        EnterCriticalSection(&g_critical);
        aButtons = g_ivConfig->Controllers[g_ivConfig->ChosenTab].aButton;
        LeaveCriticalSection(&g_critical);

        if( wParam == 0 )
        {
            CheckDlgButton( hDlg, IDC_N64RANGE, g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRealN64Range ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hDlg, IDC_RAPIDFIREENABLE, g_ivConfig->Controllers[g_ivConfig->ChosenTab].bRapidFireEnabled ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hDlg, IDC_CONFIG1, ( g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet == 0 ) ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hDlg, IDC_CONFIG2, ( g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet == 1 ) ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hDlg, IDC_CONFIG3, ( g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet == 2 ) ? BST_CHECKED : BST_UNCHECKED );

            SendMessage( GetDlgItem( hDlg, IDC_CTRRANGE ), TBM_SETPOS, TRUE, g_ivConfig->Controllers[g_ivConfig->ChosenTab].bStickRange );
            LoadString( g_hResourceDLL, IDS_C_RANGE, szTemp, 40 );
            wsprintf( szBuffer, szTemp, g_ivConfig->Controllers[g_ivConfig->ChosenTab].bStickRange );
            SendMessage( GetDlgItem( hDlg, IDT_RANGE ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            SendMessage( GetDlgItem( hDlg, IDC_RAPIDFIRERATE ), TBM_SETPOS, TRUE, 33 - g_ivConfig->Controllers[g_ivConfig->ChosenTab].bRapidFireRate );

            i = 0;
        }
        else
            i = PF_APADR;

        for( ;i < 18; ++i )
        {
            DWORD dwIDs[3];
            dwIDs[2] = i;
            if( !GetButtonID( dwIDs, 2, BSET_CONTROLS ))
                continue;

            if( dwIDs[2] >= PF_APADR )
                dwIDs[2] += g_ivConfig->Controllers[g_ivConfig->ChosenTab].bAxisSet * 4;

            GetButtonText( aButtons[dwIDs[2]], szBuffer );
            SendMessage( GetDlgItem( hDlg, dwIDs[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
        }
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
//  return TRUE; //msg got processed
}

// XInput controllers tab   --tecnicors
BOOL CALLBACK XControlsTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LPXCONTROLLER gController = &g_ivConfig->Controllers[g_ivConfig->ChosenTab].xiController;

    switch( uMsg )
    {
    case WM_INITDIALOG:
        for( int i = IDC_XC_A; i <= IDC_XC_START; i++ )
            FillN64ButtonComboBox( hDlg, i );
        for( int i = IDC_XC_DPAD; i <= IDC_XC_RTS; i++ )
            FillN64AnalogComboBox( hDlg, i );
        XControlsTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
        return FALSE;

    case WM_USER_UPDATE:
        {
            TCHAR buffer[MAX_PATH];
            GetDirectory( buffer, DIRECTORY_CONFIG );
            _stprintf_s( buffer, _T("%sXInput Controller %d Config.xcc"), buffer, gController->nControl + 1 );
            FILE *saveFile = _tfopen( buffer, _T("rS") );
            if( saveFile )
            {
                LoadXInputConfigFromFile( saveFile, gController );
                fclose( saveFile );
            }
        }
        if( !ReadXInputControllerKeys( hDlg, gController ))
            for( int i = IDC_XC_A; i <= IDC_XC_RTS; i++ )
                SendDlgItemMessage( hDlg, i, CB_SETCURSEL, 0, ( LPARAM )0 );
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ))
        {
        case IDC_XC_USE:
            StoreXInputControllerKeys( hDlg, gController );
            {
                TCHAR buffer[MAX_PATH];
                GetDirectory( buffer, DIRECTORY_CONFIG );
                _stprintf_s( buffer, _T("%sXInput Controller %d Config.xcc"), buffer, gController->nControl + 1 );
                FILE *saveFile = _tfopen( buffer, _T("wS") );
                SaveXInputConfigToFile( saveFile, gController );
                fclose( saveFile );
            }
            return TRUE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}// END Xinput Controller Tab --tecnicors

BOOL CALLBACK DevicesTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CONTROLLER *pcController = &g_ivConfig->Controllers[g_ivConfig->ChosenTab];
    TCHAR szBuffer[DEFAULT_BUFFER], szTemp[DEFAULT_BUFFER];
    HWND hDlgItem;
    long i;

    switch(uMsg)
    {
    case WM_INITDIALOG:

        // TrackBars
        hDlgItem = GetDlgItem( hDlg, IDC_DEADZONE );

        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 100 ));
        SendMessage( hDlgItem, TBM_SETTICFREQ, (WPARAM) 10, 0 );
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );

        hDlgItem = GetDlgItem( hDlg, IDC_MSSENSITIVITY_X );

        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 1000 ));
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );

        hDlgItem = GetDlgItem( hDlg, IDC_MSSENSITIVITY_Y );

        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 1000 ));
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );
        {
            short sTics[] = { 25, 50, 75, 100, 125, 150, 200, 250, 300, 400, 500, 600, 700, 800, 900 };
            for( i = 0; i < (sizeof(sTics) / sizeof(short)); ++i )
                SendMessage( hDlgItem, TBM_SETTIC, 0, sTics[i] );
        }
        // TrackBars End

        DevicesTabProc( hDlg, WM_USER_UPDATE, 0, 0 ); // setting values

        return FALSE; // don't give it focus

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );

        switch( LOWORD(wParam) )
        {
        case IDC_DEADPANMOUSEX:
            if( LOWORD(wParam) == IDC_DEADPANMOUSEX )
                i = MM_DEAD;
        case IDC_BUFFEREDMOUSEX:
            if( LOWORD(wParam) == IDC_BUFFEREDMOUSEX )
                i = MM_BUFF;
        case IDC_ABSOLUTEMOUSEX:
            if( LOWORD(wParam) == IDC_ABSOLUTEMOUSEX )
                i = MM_ABS;
            if(( HIWORD(wParam) == BN_CLICKED ) &&  ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED ))
                pcController->bMouseMoveX = i;
            return TRUE;

        case IDC_DEADPANMOUSEY:
            if( LOWORD(wParam) == IDC_DEADPANMOUSEY )
                i = MM_DEAD;
        case IDC_BUFFEREDMOUSEY:
            if( LOWORD(wParam) == IDC_BUFFEREDMOUSEY )
                i = MM_BUFF;
        case IDC_ABSOLUTEMOUSEY:
            if( LOWORD(wParam) == IDC_ABSOLUTEMOUSEY )
                i = MM_ABS;
            if(( HIWORD(wParam) == BN_CLICKED ) &&  ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED ))
                pcController->bMouseMoveY = i;
            return TRUE;

        case IDC_ACCELERATEX:
            pcController->fKeyAbsoluteX = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
            return TRUE;
        case IDC_ACCELERATEY:
            pcController->fKeyAbsoluteY = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
            return TRUE;

        default:
            return FALSE;
        }

    case WM_HSCROLL: // TrackBars
    case WM_VSCROLL:
        i = GetWindowLong( (HWND)lParam, GWL_ID );
        switch( i )
        {
        case IDC_MSSENSITIVITY_X:
            pcController->wMouseSensitivityX = (WORD)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            LoadString( g_hResourceDLL, IDS_D_MSX, szTemp, DEFAULT_BUFFER );
            wsprintf( szBuffer, szTemp, pcController->wMouseSensitivityX );
            SendMessage( GetDlgItem( hDlg, IDT_MSSENSITIVITY_X ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            return TRUE;

        case IDC_MSSENSITIVITY_Y:
            pcController->wMouseSensitivityY = (WORD)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            LoadString( g_hResourceDLL, IDS_D_MSY, szTemp, DEFAULT_BUFFER );
            wsprintf( szBuffer, szTemp, pcController->wMouseSensitivityY );
            SendMessage( GetDlgItem( hDlg, IDT_MSSENSITIVITY_Y ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            return TRUE;

        case IDC_DEADZONE:
            pcController->bPadDeadZone = (BYTE)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            LoadString( g_hResourceDLL, IDS_D_DEADZONE, szTemp, DEFAULT_BUFFER );
            wsprintf( szBuffer, szTemp, pcController->bPadDeadZone );
            SendMessage( GetDlgItem( hDlg, IDT_DEADZONE ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            return TRUE;
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        if( pcController->bMouseMoveX == MM_DEAD )
            CheckDlgButton( hDlg, IDC_DEADPANMOUSEX, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_DEADPANMOUSEX, BST_UNCHECKED );

        if( pcController->bMouseMoveX == MM_BUFF )
            CheckDlgButton( hDlg, IDC_BUFFEREDMOUSEX, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_BUFFEREDMOUSEX, BST_UNCHECKED );

        if( pcController->bMouseMoveX == MM_ABS )
            CheckDlgButton( hDlg, IDC_ABSOLUTEMOUSEX, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_ABSOLUTEMOUSEX, BST_UNCHECKED );

        if( pcController->bMouseMoveY == MM_DEAD )
            CheckDlgButton( hDlg, IDC_DEADPANMOUSEY, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_DEADPANMOUSEY, BST_UNCHECKED );

        if( pcController->bMouseMoveY == MM_BUFF )
            CheckDlgButton( hDlg, IDC_BUFFEREDMOUSEY, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_BUFFEREDMOUSEY, BST_UNCHECKED );

        if( pcController->bMouseMoveY == MM_ABS )
            CheckDlgButton( hDlg, IDC_ABSOLUTEMOUSEY, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_ABSOLUTEMOUSEY, BST_UNCHECKED );

        CheckDlgButton( hDlg, IDC_ACCELERATEX, pcController->fKeyAbsoluteX ? BST_CHECKED : BST_UNCHECKED );
        CheckDlgButton( hDlg, IDC_ACCELERATEY, pcController->fKeyAbsoluteY ? BST_CHECKED : BST_UNCHECKED );

        // TrackBars
        SendMessage( GetDlgItem( hDlg, IDC_DEADZONE ), TBM_SETPOS, TRUE, pcController->bPadDeadZone );
        LoadString( g_hResourceDLL, IDS_D_DEADZONE, szTemp, DEFAULT_BUFFER );
        wsprintf( szBuffer, szTemp, pcController->bPadDeadZone );
        SendMessage( GetDlgItem( hDlg, IDT_DEADZONE ), WM_SETTEXT , 0, (LPARAM)szBuffer );

        SendMessage( GetDlgItem( hDlg, IDC_MSSENSITIVITY_X ), TBM_SETPOS, TRUE, pcController->wMouseSensitivityX );
        LoadString( g_hResourceDLL, IDS_D_MSX, szTemp, DEFAULT_BUFFER );
        wsprintf( szBuffer, szTemp, pcController->wMouseSensitivityX );
        SendMessage( GetDlgItem( hDlg, IDT_MSSENSITIVITY_X ), WM_SETTEXT , 0, (LPARAM)szBuffer );

        SendMessage( GetDlgItem( hDlg, IDC_MSSENSITIVITY_Y ), TBM_SETPOS, TRUE, pcController->wMouseSensitivityY );
        LoadString( g_hResourceDLL, IDS_D_MSY, szTemp, DEFAULT_BUFFER );
        wsprintf( szBuffer, szTemp, pcController->wMouseSensitivityY );
        SendMessage( GetDlgItem( hDlg, IDT_MSSENSITIVITY_Y ), WM_SETTEXT , 0, (LPARAM)szBuffer );
        // TrackBars End
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK MoveModifierDialog( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HWND hDlgItem;
    long i,j;
    DWORD dwValue;
    TCHAR szBuffer[DEFAULT_BUFFER], szTemp[DEFAULT_BUFFER];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        // TrackBars
        hDlgItem = GetDlgItem( hDlg, IDC_XMODIFIER );

        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 500 ));
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );
        {
            short sTics[] = { 25, 50, 75, 100, 125, 150, 200, 250, 300, 400, 500 };
            for( i = 0; i < (sizeof(sTics) / sizeof(short)); ++i )
                SendMessage( hDlgItem, TBM_SETTIC, 0, sTics[i] );
        }

        hDlgItem = GetDlgItem( hDlg, IDC_YMODIFIER );

        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 500 ));
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );
        {
            short sTics[] = { 25, 50, 75, 100, 125, 150, 200, 250, 300, 400, 500 };
            for( i = 0; i < (sizeof(sTics) / sizeof(short)); ++i )
                SendMessage( hDlgItem, TBM_SETTIC, 0, sTics[i] );
        }
        return FALSE; // don't give it focus

    case WM_HSCROLL: // TrackBars
    case WM_VSCROLL:
        switch (GetWindowLong( (HWND)lParam, GWL_ID ))
        {
        case IDC_XMODIFIER:
            i = SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            LoadString( g_hResourceDLL, IDS_M_MOVEVALUE, szTemp, DEFAULT_BUFFER );
            wsprintf( szBuffer, szTemp, i );
            SendMessage( GetDlgItem( hDlg, IDT_XMODIFIER ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            return TRUE;
        case IDC_YMODIFIER:
            i = SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            LoadString( g_hResourceDLL, IDS_M_MOVEVALUE, szTemp, DEFAULT_BUFFER );
            wsprintf( szBuffer, szTemp, i );
            SendMessage( GetDlgItem( hDlg, IDT_YMODIFIER ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            return TRUE;
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        if( wParam == MDT_MOVE )
            dwValue = (DWORD)lParam;
        else
            dwValue = MAKELONG( 100, 100 );

        i = (short)(dwValue & 0x0000FFFF);

        if( i < 0 )
        {
            i = -i;
            CheckDlgButton( hDlg, IDC_XNEGATE, BST_CHECKED );
        }
        else
            CheckDlgButton( hDlg, IDC_XNEGATE, BST_UNCHECKED );
        SendMessage( GetDlgItem( hDlg, IDC_XMODIFIER ), TBM_SETPOS, TRUE, i );
        LoadString( g_hResourceDLL, IDS_M_MOVEVALUE, szTemp, DEFAULT_BUFFER );
        wsprintf( szBuffer, szTemp, i );
        SendMessage( GetDlgItem( hDlg, IDT_XMODIFIER ), WM_SETTEXT , 0, (LPARAM)szBuffer );

        i = (short)((dwValue >> 16) & 0x0000FFFF);

        if( i < 0 )
        {
            i = -i;
            CheckDlgButton( hDlg, IDC_YNEGATE, BST_CHECKED );
        }
        else
            CheckDlgButton( hDlg, IDC_YNEGATE, BST_UNCHECKED );
        SendMessage( GetDlgItem( hDlg, IDC_YMODIFIER ), TBM_SETPOS, TRUE, i );
        LoadString( g_hResourceDLL, IDS_M_MOVEVALUE, szTemp, DEFAULT_BUFFER );
        wsprintf( szBuffer, szTemp, i );
        SendMessage( GetDlgItem( hDlg, IDT_YMODIFIER ), WM_SETTEXT , 0, (LPARAM)szBuffer );
        return TRUE;

    case WM_USER_READVALUES:
        i = SendMessage( GetDlgItem( hDlg, IDC_XMODIFIER ), TBM_GETPOS, 0, 0 );
        if( IsDlgButtonChecked( hDlg, IDC_XNEGATE ) == BST_CHECKED )
            i = -i;

        j = SendMessage( GetDlgItem( hDlg, IDC_YMODIFIER ), TBM_GETPOS, 0, 0 );
        if( IsDlgButtonChecked( hDlg, IDC_YNEGATE ) == BST_CHECKED )
            j = -j;

        *(DWORD*)wParam = MAKELONG( (short)i, (short)j );
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK MacroModifierDialog( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    long i;
    DWORD dwValue;

    const DWORD aButtons[] ={   IDC_DRIGHT  , IDC_DLEFT , IDC_DDOWN     , IDC_DUP,
                        IDC_SBUTTON , IDC_ZTRIG , IDC_BBUTTON   , IDC_ABUTTON,
                        IDC_CRIGHT  , IDC_CLEFT , IDC_CDOWN     , IDC_CUP,
                        IDC_RTRIG   , IDC_LTRIG , 0             , 0,
                        IDC_ARIGHT  , IDC_ALEFT , IDC_ADOWN     , IDC_AUP,
                        IDC_RAPIDFIREMODE       , IDC_RAPIDFIREMODERATE };

    switch(uMsg)
    {
    case WM_INITDIALOG:
        return FALSE; // don't give it focus

    case WM_USER_UPDATE:
        if( wParam == MDT_MACRO )
            dwValue = (DWORD)lParam;
        else
            dwValue = 0;

        i = sizeof(aButtons) / sizeof(aButtons[0]) - 1;

        while( i >= 0 )
        {
            if( aButtons[i] )
            {
                if( dwValue & ( 1 << i ) )
                    CheckDlgButton( hDlg, aButtons[i], BST_CHECKED );
                else
                    CheckDlgButton( hDlg, aButtons[i], BST_UNCHECKED );
            }

            i--;
        }
        return TRUE;

    case WM_USER_READVALUES:
        dwValue = 0;

        i = sizeof(aButtons) / sizeof(aButtons[0]) - 1;
        while( i >= 0 )
        {
            if( aButtons[i] && ( IsDlgButtonChecked( hDlg, aButtons[i] ) == BST_CHECKED ))
                dwValue = dwValue | ( 1 << i );

            i--;
        }

        *(DWORD*)wParam = dwValue;
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK ConfigModifierDialog( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    DWORD dwValue;

    const DWORD aButtons[] ={   IDC_CONFIG1     , 0x000001,
                        IDC_CONFIG2     , 0x000003,
                        IDC_CONFIG3     , 0x000005,
                        IDC_CONFIGCYCLE , 0x0000FF,
                        IDC_MOUSEX      , 0x000100,
                        IDC_MOUSEY      , 0x000200,
                        IDC_KEYX        , 0x010000,
                        IDC_KEYY        , 0x020000 };

    int i;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        return FALSE; // don't give it focus

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDC_CONFIG1:
        case IDC_CONFIG2:
        case IDC_CONFIG3:
        case IDC_CONFIGCYCLE:
            if(( HIWORD(wParam) == BN_CLICKED ) || ( HIWORD(wParam) == BN_DBLCLK ))
            {
                EnterCriticalSection(&g_critical);  // has a possibility of affecting the buttons we're writing to
                bool bCheck = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED ) ? false : true;
                CheckDlgButton( hDlg, IDC_CONFIG1, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDC_CONFIG2, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDC_CONFIG3, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDC_CONFIGCYCLE, BST_UNCHECKED );
                if( bCheck )
                    CheckDlgButton( hDlg, LOWORD(wParam), BST_CHECKED );
                LeaveCriticalSection(&g_critical);
            }
            return TRUE;
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        if( wParam == MDT_CONFIG )
            dwValue = (DWORD)lParam;
        else
            dwValue = 0;
        if( dwValue & 0x01 )
        {
            BYTE bConfig = (BYTE)((dwValue >> 1) & 0x7F);
            if( bConfig >= PF_AXESETS )
            {
                CheckDlgButton( hDlg, IDC_CONFIG1, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDC_CONFIG2, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDC_CONFIG3, BST_UNCHECKED );
                CheckDlgButton( hDlg, IDC_CONFIGCYCLE, BST_CHECKED );
            }
            else
            {
                i = PF_AXESETS - 1;
                while( i >= 0 )
                {
                    if( bConfig == i )
                        CheckDlgButton( hDlg, aButtons[i*2], BST_CHECKED );
                    else
                        CheckDlgButton( hDlg, aButtons[i*2], BST_UNCHECKED );

                    i--;
                }
            }
        }

        i = sizeof(aButtons) / sizeof(aButtons[0]) - 2;
        while( i >= 8 )
        {
            if(( dwValue & aButtons[i+1] ) == aButtons[i+1] )
                CheckDlgButton( hDlg, aButtons[i], BST_CHECKED );
            else
                CheckDlgButton( hDlg, aButtons[i], BST_UNCHECKED );

            i -= 2;
        }
        return TRUE;

    case WM_USER_READVALUES:
        dwValue = 0;

        i = sizeof(aButtons) / sizeof(aButtons[0]) - 2;
        while( i >= 0 )
        {
            if( IsDlgButtonChecked( hDlg, aButtons[i] ) == BST_CHECKED )
                dwValue |= aButtons[i+1];
            i -= 2;
        }

        *(DWORD*)wParam = dwValue;
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

void ModDescription( HWND hListView, int iEntry, const LPMODIFIER pModifier )
{
    TCHAR szBuffer[DEFAULT_BUFFER];
    const UINT iModTypes[] = { IDS_M_TAB_NONE, IDS_M_TAB_MOVE, IDS_M_TAB_MACRO, IDS_M_TAB_CONFIG };
    TCHAR pszModTypes[4][16];

    for (int i = 0; i < ARRAYSIZE(iModTypes); i++ )
        LoadString( g_hResourceDLL, iModTypes[i], pszModTypes[i], ARRAYSIZE(pszModTypes[i]) );

    GetButtonText( pModifier->btnButton, szBuffer );

    ListView_SetItemText( hListView, iEntry, 0,szBuffer );

    switch( pModifier->bModType )
    {
    case MDT_MOVE:
        ListView_SetItemText( hListView, iEntry, 1, pszModTypes[1] );
        wsprintf( szBuffer, _T("X:%i%% / Y:%i%%"), (short)(pModifier->dwSpecific & 0x0000FFFF), (short)((pModifier->dwSpecific >> 16) & 0x0000FFFF));
        break;
    case MDT_MACRO:
        ListView_SetItemText( hListView, iEntry, 1, pszModTypes[2] );
        szBuffer[0] = '\0';
        {
            bool bGotKey = false;
            DWORD dwValue = pModifier->dwSpecific;
            const TCHAR *apszButtons[] ={   _T("Dp->"), _T("Dp<-"), _T("Dp\\/"), _T("Dp/\\"), _T("St"), _T("Z"), _T("B"), _T("A"),
                                    _T("Cb->"), _T("Cb<-"), _T("Cb\\/"), _T("Cb/\\"), _T("R"), _T("L"), NULL, NULL,
                                    _T("As->"), _T("As<-"), _T("As\\/"), _T("As/\\"), _T("(<Rf>)") };

            int i = sizeof(apszButtons) / sizeof(apszButtons[0]) - 1;

            while( i >= 0 )
            {
                if( apszButtons[i] && ( dwValue & ( 1 << i ) ))
                {
                    if( bGotKey )
                        lstrcat( szBuffer, _T(" ") );
                    else
                        bGotKey = true;

                    lstrcat( szBuffer, apszButtons[i] );
                }

                i--;
            }
        }
        break;
    case MDT_CONFIG:
        ListView_SetItemText( hListView, iEntry, 1, pszModTypes[3] );
        szBuffer[0] = '\0';
        {
            DWORD dwValue = pModifier->dwSpecific;
            bool bGotKey = false;

            if( dwValue & 0x1 )
            {
                lstrcat( szBuffer, _T("C-") );
                if((( dwValue >> 1 ) & 0x7F ) < PF_AXESETS )
                    wsprintf( &szBuffer[lstrlen(szBuffer)], _T("%i"), (( dwValue >> 1 ) & 0x7F ));
                else
                    lstrcat( szBuffer, _T("Sw") );
                bGotKey = true;
            }

            if( dwValue & 0x300 )
            {
                if( bGotKey )
                    lstrcat( szBuffer, _T(" ") );
                else
                    bGotKey = true;

                lstrcat( szBuffer, _T("Ms-") );
                if( dwValue & 0x100 )
                    lstrcat( szBuffer, _T("X") );
                if( dwValue & 0x200 )
                    lstrcat( szBuffer, _T("Y") );
            }

            if( dwValue & 0x30000 )
            {
                if( bGotKey )
                    lstrcat( szBuffer, _T(" ") );
                else
                    bGotKey = true;

                lstrcat( szBuffer, _T("Kb-") );
                if( dwValue & 0x10000 )
                    lstrcat( szBuffer, _T("X") );
                if( dwValue & 0x20000 )
                    lstrcat( szBuffer, _T("Y") );
            }
        }
        break;

    case MDT_NONE:
    default:
        ListView_SetItemText( hListView, iEntry, 1, pszModTypes[0] );
        szBuffer[0] = '\0';
    }
    ListView_SetItemText( hListView, iEntry, 2, szBuffer );
}

BOOL CALLBACK ModifierTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static BUTTON storage;
    static bool bScanRunning;
    static DWORD dwCounter;
    static HWND hFocus = NULL, hBlocker;
    static int iSelectedMod;
    static BYTE bDisplayedProbs;
    static HWND hModProperties;
    static LPCONTROLLER pcController;

    const UINT iModTypes[] = { IDS_M_NONE, IDS_M_MOVE, IDS_M_MACRO, IDS_M_CONFIG };
    TCHAR pszModTypes[4][16];

    for (int j = 0; j < ARRAYSIZE(iModTypes); j++ )
        LoadString( g_hResourceDLL, iModTypes[j], pszModTypes[j], ARRAYSIZE(pszModTypes[j]) );

    TCHAR szBuffer[40];
    HWND hDlgItem;
    long i;
    BYTE bByte;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        bScanRunning = false;
        hModProperties = NULL;
        bDisplayedProbs = MDT_NONE;
        // List View
        hDlgItem = GetDlgItem( hDlg, IDC_MODIFIERLIST );

        LVCOLUMN lvColumn;

        lvColumn.mask = LVCFMT_CENTER | LVCF_WIDTH | LVCF_TEXT;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.cx = 100;
        LoadString( g_hResourceDLL, IDS_M_ASSIGNED, szBuffer, 40 );
        lvColumn.pszText = szBuffer;
        ListView_InsertColumn( hDlgItem, 0, &lvColumn );

        lvColumn.fmt = LVCFMT_CENTER | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvColumn.cx = 43;
        lvColumn.iSubItem = 1;
        LoadString( g_hResourceDLL, IDS_M_TYPE, szBuffer, 40 );
        lvColumn.pszText = szBuffer;
        ListView_InsertColumn( hDlgItem, 1, &lvColumn );

        lvColumn.cx = 128;
        lvColumn.iSubItem = 2;
        LoadString( g_hResourceDLL, IDS_M_PARAM, szBuffer, 40 );
        lvColumn.pszText = szBuffer;
        ListView_InsertColumn( hDlgItem, 2, &lvColumn );

        ListView_SetExtendedListViewStyle( hDlgItem, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP );
        // ListView End

        // DropDown
        hDlgItem = GetDlgItem( hDlg, IDC_MODTYP );

        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)pszModTypes[0] );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, MDT_NONE );

        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)pszModTypes[1] );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, MDT_MOVE );

        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)pszModTypes[2] );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, MDT_MACRO );

        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)pszModTypes[3] );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, MDT_CONFIG );
        // DropDown End

        ZeroMemory(&storage, sizeof(BUTTON));

        ModifierTabProc( hDlg, WM_USER_UPDATE, 0, 0 ); // setting values

        return FALSE; // don't give it focus

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );
        switch( LOWORD(wParam) )
        {
        case IDC_NEWMODIFIER:
            if( pcController->nModifiers < MAX_MODIFIERS )
            {
                EnterCriticalSection(&g_critical);
                pcController->nModifiers++;
                pcController->pModifiers = (MODIFIER*)P_realloc( pcController->pModifiers, sizeof(MODIFIER) * pcController->nModifiers );

                ZeroMemory( &pcController->pModifiers[pcController->nModifiers - 1], sizeof(MODIFIER) );
                iSelectedMod = pcController->nModifiers - 1;
                LeaveCriticalSection(&g_critical);

                ModifierTabProc( hDlg, WM_USER_UPDATE, 0, 1 );
            }
            return TRUE;

        case IDC_KILLMODIFIER:
            hDlgItem = GetDlgItem( hDlg, IDC_MODIFIERLIST );

            if( ListView_GetSelectedCount( hDlgItem ) > 0 )
            {
                EnterCriticalSection(&g_critical);
                for( i = ListView_GetItemCount( hDlgItem ) - 1; i >= 0; i-- )
                {
                    if( ListView_GetItemState( hDlgItem, i, LVIS_SELECTED ))
                    {
                        MoveMemory( &pcController->pModifiers[i], &pcController->pModifiers[i+1], sizeof(MODIFIER) * ( pcController->nModifiers - 1 - i ));
                        pcController->nModifiers--;
                    }
                }
                pcController->pModifiers = (MODIFIER*)P_realloc( pcController->pModifiers, sizeof(MODIFIER) * pcController->nModifiers );
                LeaveCriticalSection(&g_critical);
                ModifierTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
            }
            return TRUE;

        case IDC_ASSIGNMOD:
            EnterCriticalSection(&g_critical);
            hDlgItem = GetDlgItem( hDlg, IDC_MODIFIERLIST );
            if( bScanRunning )
            {
/*              bScanRunning = false;
                KillTimer( hDlg, TIMER_BUTTON );
                BUTTON btnButton;
                btnButton = storage; // btnButton.dwButton = GetWindowLong( hDlgItem, GWL_USERDATA );
                GetButtonText( btnButton, szBuffer );
                SendMessage( GetDlgItem( hDlg, IDT_ASSIGNMOD ), WM_SETTEXT , 0, (LPARAM)szBuffer ); */ ;
            }
            else if( HIWORD(wParam) == BN_CLICKED && ListView_GetItemCount( hDlgItem ))
            {
                dwCounter = 0;
                if (hFocus == NULL)
                    hFocus = SetFocus( NULL );
                hBlocker = MakeOverlay();

                SetTimer( hDlg, TIMER_BUTTON, INTERVAL_BUTTON, NULL );
                bScanRunning = true;
            }
            LeaveCriticalSection(&g_critical);
            return TRUE;

        case IDC_MODTYP:
            if( HIWORD (wParam) == CBN_SELCHANGE )
            {
                ModifierTabProc( hDlg, WM_USER_UPDATE, 1, 1 );
            }
            return TRUE;

        case IDC_APPCHANGES:
            // ModType
            EnterCriticalSection(&g_critical);
            hDlgItem = GetDlgItem( hDlg, IDC_MODTYP );
            i = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
            pcController->pModifiers[iSelectedMod].bModType = (BYTE)SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );

            // Toggle
            pcController->pModifiers[iSelectedMod].fToggle = ( IsDlgButtonChecked( hDlg, IDC_TOGGLE ) == BST_CHECKED );
            // Status
            pcController->pModifiers[iSelectedMod].fStatus = ( IsDlgButtonChecked( hDlg, IDC_STATE ) == BST_CHECKED );

            // Specific Data
            if( hModProperties )
                SendMessage( hModProperties, WM_USER_READVALUES, (WPARAM)&pcController->pModifiers[iSelectedMod].dwSpecific, 0 );
            else
                pcController->pModifiers[iSelectedMod].dwSpecific = 0;

            // ModButton
            pcController->pModifiers[iSelectedMod].btnButton = storage;
            LeaveCriticalSection(&g_critical);

            ModDescription( GetDlgItem( hDlg, IDC_MODIFIERLIST ), iSelectedMod, &pcController->pModifiers[iSelectedMod] );

            return TRUE;
        case IDC_RESET:
            ModifierTabProc( hDlg, WM_USER_UPDATE, 1, 0 );
            return TRUE;

        case IDC_TOGGLE:
            EnableWindow( GetDlgItem( hDlg, IDC_STATE ), ( IsDlgButtonChecked( hDlg, IDC_TOGGLE ) == BST_CHECKED ) ? TRUE : FALSE );
            return TRUE;
        case IDC_STATE:
            return TRUE;

        default:
            return FALSE;
        }

    case WM_TIMER: // when assigning modifiers, this gets called every 20ms (or value in INTERVAL_BUTTON)
        if( wParam == TIMER_BUTTON && bScanRunning )
        {
            BUTTON newButton;
            i = ScanDevices( &dwCounter, &newButton );
            if( i || dwCounter > 500 )
            {
                bScanRunning = false;
                KillTimer( hDlg, TIMER_BUTTON );

                hDlgItem = GetDlgItem( hDlg, IDC_ASSIGNMOD );

                EnterCriticalSection(&g_critical);
                if( i == SC_SCANESCAPE ) // Got an escape char from keyboard; cancel
                    ZeroMemory(&storage, sizeof(BUTTON));
                else if( i == SC_SCANSUCCEED  ) // Got a button or axis
                    storage = newButton;
                newButton = storage;
                DestroyWindow( hBlocker );

                LeaveCriticalSection(&g_critical);

                GetButtonText( newButton, szBuffer );
                SendMessage( GetDlgItem( hDlg, IDT_ASSIGNMOD ), WM_SETTEXT , 0, (LPARAM)szBuffer );
                if( hFocus != NULL )
                {
                    SetFocus( hFocus );
                    hFocus = NULL;
                }
            }
            else
            {
                if(( dwCounter % 50 ) == 0 )
                {
                    TCHAR tszText[DEFAULT_BUFFER];

                    LoadString( g_hResourceDLL, IDS_C_POLLING, tszText, DEFAULT_BUFFER );
                    wsprintf( szBuffer, tszText, 10 - dwCounter / 50 );
                    SendMessage( GetDlgItem( hDlg, IDT_ASSIGNMOD ), WM_SETTEXT , 0, (LPARAM)szBuffer );
                }
                ++dwCounter;
            }
            return TRUE;
        }
        else
            return FALSE;

    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam)->idFrom )
        {
        case IDC_MODIFIERLIST:
            switch( ((LPNMHDR)lParam)->code )
            {
            case LVN_ITEMCHANGED:
                ModifierTabProc( hDlg, WM_USER_UPDATE, 1, 0 );
                return TRUE;
            default:
                return FALSE;
            }
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        pcController = &g_ivConfig->Controllers[g_ivConfig->ChosenTab];
        hDlgItem = GetDlgItem( hDlg, IDC_MODIFIERLIST );

        if( wParam == 0 )
        {
            // Update Modifier List
            ListView_DeleteAllItems( hDlgItem );
            if( lParam == 0 )
                iSelectedMod = -1;

            LVITEM lvItem;
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.iItem = 0;
            lvItem.iSubItem = 0;
            lvItem.pszText = _T("");

            ListView_SetItemCount( hDlgItem, pcController->nModifiers );
            for( lvItem.lParam = 0; lvItem.lParam < pcController->nModifiers; ++lvItem.lParam )
            {
                lvItem.iItem = lvItem.lParam;
                i = ListView_InsertItem( hDlgItem, &lvItem );

                ModDescription( hDlgItem, i, &pcController->pModifiers[lvItem.lParam] );
            }
            if( iSelectedMod >= 0 && iSelectedMod < ListView_GetItemCount( hDlgItem ))

                ListView_SetItemState( hDlgItem, iSelectedMod, LVIS_SELECTED, LVIS_SELECTED )
            else
                iSelectedMod = -1;

        }
        else
        {
            // Get selected Modifier
            iSelectedMod = -1;
            if( ListView_GetSelectedCount( hDlgItem ) > 0 )
            {
                for( i = ListView_GetItemCount( hDlgItem ) - 1; i >= 0; i-- )
                {
                    if( ListView_GetItemState( hDlgItem, i, LVIS_SELECTED ))
                    {
                        iSelectedMod = i;
                        i = -1;
                    }
                }
            }
        }

        hDlgItem = GetDlgItem( hDlg, IDC_MODTYP );

        if( lParam == 0 )
        {
            if( iSelectedMod >= 0 )
            {   // a mod is selected
                EnableWindow( GetDlgItem( hDlg, IDC_ASSIGNMOD ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDT_ASSIGNMOD ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_TOGGLE ), TRUE );
                EnableWindow( hDlgItem, TRUE );
                EnableWindow( GetDlgItem( hDlg, IDT_MODTYP ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_APPCHANGES ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_RESET ), TRUE );

                for( i = SendMessage( hDlgItem, CB_GETCOUNT, 0, 0 )-1; i >= 0; i-- )
                { // looking which Mod-Typ
                    bByte = (BYTE)SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );
                    if( pcController->pModifiers[iSelectedMod].bModType == bByte )
                    {
                        SendMessage( hDlgItem, CB_SETCURSEL, i, 0 );
                        i = -10;
                    }
                }
                if( i > -5 )
                    SendMessage( hDlgItem, CB_SETCURSEL, 0, 0 );

                CheckDlgButton( hDlg, IDC_TOGGLE, pcController->pModifiers[iSelectedMod].fToggle ? BST_CHECKED : BST_UNCHECKED );
                EnableWindow( GetDlgItem( hDlg, IDC_STATE ), pcController->pModifiers[iSelectedMod].fToggle ? TRUE : FALSE );
                CheckDlgButton( hDlg, IDC_STATE, pcController->pModifiers[iSelectedMod].fStatus ? BST_CHECKED : BST_UNCHECKED );


                storage = pcController->pModifiers[iSelectedMod].btnButton;
                GetButtonText( pcController->pModifiers[iSelectedMod].btnButton, szBuffer );
                SendMessage( GetDlgItem( hDlg, IDT_ASSIGNMOD ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            }
            else
            {
                EnableWindow( GetDlgItem( hDlg, IDC_ASSIGNMOD ), FALSE );
                EnterCriticalSection(&g_critical);
                ZeroMemory(&storage, sizeof(BUTTON));
                LeaveCriticalSection(&g_critical);
                SendMessage( GetDlgItem( hDlg, IDT_ASSIGNMOD ), WM_SETTEXT , 0, (LPARAM) _T("") );
                EnableWindow( GetDlgItem( hDlg, IDT_ASSIGNMOD ), FALSE );

                SendMessage( hDlgItem, CB_SETCURSEL, 0, 0 );
                EnableWindow( hDlgItem, FALSE );
                EnableWindow( GetDlgItem( hDlg, IDT_MODTYP ), FALSE );

                EnableWindow( GetDlgItem( hDlg, IDC_TOGGLE ), FALSE );
                CheckDlgButton( hDlg, IDC_TOGGLE , BST_UNCHECKED );
                EnableWindow( GetDlgItem( hDlg, IDC_STATE ), FALSE );
                CheckDlgButton( hDlg, IDC_STATE , BST_UNCHECKED );
                EnableWindow( GetDlgItem( hDlg, IDC_APPCHANGES ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDC_RESET ), FALSE );
            }
        }

        i = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
        bByte = (BYTE)SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );


        if(( bByte == bDisplayedProbs ) && hModProperties )
            SendMessage( hModProperties, WM_USER_UPDATE, pcController->pModifiers[iSelectedMod].bModType, pcController->pModifiers[iSelectedMod].dwSpecific );
        else
        {
            if( hModProperties )
                DestroyWindow( hModProperties );

            switch( bByte )
            {
            case MDT_MOVE:
                hModProperties = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_MOD_MOVE), hDlg, (DLGPROC)MoveModifierDialog);
                break;
            case MDT_MACRO:
                hModProperties = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_MOD_MACRO), hDlg, (DLGPROC)MacroModifierDialog);
                break;
            case MDT_CONFIG:
                hModProperties = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_MOD_CONFIG), hDlg, (DLGPROC)ConfigModifierDialog);
                break;
            case MDT_NONE:
            default:
                hModProperties = NULL;
            }

            hDlgItem = GetDlgItem( hDlg, IDC_PROPWINDOW );

            if( hModProperties )
            {
                RECT rectProp, rectMain; // need to know the position of the calling tab window relative to its parent

                GetWindowRect( hDlg, &rectMain );
                GetWindowRect( hDlgItem, &rectProp );

                MoveWindow( hModProperties, rectProp.left - rectMain.left, rectProp.top - rectMain.top, rectProp.right - rectProp.left, rectProp.bottom - rectProp.top, FALSE );
                SendMessage( hModProperties, WM_USER_UPDATE, pcController->pModifiers[iSelectedMod].bModType, pcController->pModifiers[iSelectedMod].dwSpecific );
                ShowWindow( hModProperties, SW_SHOW );
            }
            bDisplayedProbs = bByte;
        }
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK ControllerPakTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static HWND hPakWindow;
    static bool bAdaptoidInList;
    static BYTE bCurrentPak;
    HWND hDlgItem;
    long i,j;
    BYTE bByte;
    TCHAR tszMsg[DEFAULT_BUFFER];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        hPakWindow = NULL;
        bAdaptoidInList = false;
        bCurrentPak = (BYTE)(-1);

        // DropDown-List;
        hDlgItem = GetDlgItem( hDlg, IDC_PAKTYPE );
        LoadString( g_hResourceDLL, IDS_P_NONE, tszMsg, DEFAULT_BUFFER );
        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)tszMsg );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, PAK_NONE );

        LoadString( g_hResourceDLL, IDS_P_MEMPAK, tszMsg, DEFAULT_BUFFER );
        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)tszMsg );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, PAK_MEM );

        LoadString( g_hResourceDLL, IDS_P_RUMBLEPAK, tszMsg, DEFAULT_BUFFER );
        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)tszMsg );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, PAK_RUMBLE );

#ifdef V_TRANSFERPAK
        LoadString( g_hResourceDLL, IDS_P_TRANSFERPAK, tszMsg, DEFAULT_BUFFER );
        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)tszMsg );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, PAK_TRANSFER );
#pragma message( "Transferpak enabled in Interface" )
#endif
#ifdef V_VOICEPAK
        LoadString( g_hResourceDLL, IDS_P_VOICEPAK, tszMsg, DEFAULT_BUFFER );
        i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)tszMsg );
        SendMessage( hDlgItem, CB_SETITEMDATA, i, PAK_VOICE );
#pragma message( "Voicepak enabled in Interface" )
#endif

        ControllerPakTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
        return FALSE; // don't give it focus

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );

        switch( LOWORD(wParam) )
        {
        case IDC_PAKTYPE:
            if( HIWORD (wParam) == CBN_SELCHANGE )
            {
                i = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
                g_ivConfig->Controllers[g_ivConfig->ChosenTab].PakType = SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );
                ControllerPakTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
            }
            return TRUE;
        case IDC_RAWMODE:
            g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRawData = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
            ControllerPakTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
            return TRUE;

        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        CheckDlgButton( hDlg, IDC_RAWMODE, g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRawData ? BST_CHECKED : BST_UNCHECKED );
        //Set Dropdownlist
        hDlgItem = GetDlgItem( hDlg, IDC_PAKTYPE );

        if( g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRawData &&
            !lstrcmp( g_ivConfig->FFDevices[g_ivConfig->ChosenTab].szProductName, _T(STRING_ADAPTOID) ) &&
            !bAdaptoidInList )
        {
            // add Adaptoid Pak to list
            LoadString( g_hResourceDLL, IDS_P_ADAPTOIDPAK, tszMsg, DEFAULT_BUFFER );
            i = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)tszMsg );
            SendMessage( hDlgItem, CB_SETITEMDATA, i, PAK_ADAPTOID );
            bAdaptoidInList = true;
        }

        if( bAdaptoidInList &&
            ( lstrcmp( g_ivConfig->FFDevices[g_ivConfig->ChosenTab].szProductName, _T(STRING_ADAPTOID) ) ||
              !g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRawData ))
        {
            // remove Adaptoid Pak from list
            i = SendMessage( hDlgItem, CB_GETCOUNT, 0, 0 ) - 1;
            while(( i >= 0 ))
            {
                j = SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );
                if( j == PAK_ADAPTOID )
                {
                    SendMessage( hDlgItem, CB_DELETESTRING, i, 0 );
                    i = -1;
                }
                i--;
            }
            bAdaptoidInList = false;
        }


        i = SendMessage( hDlgItem, CB_GETCOUNT, 0, 0 ) - 1;

        while(( i >= 0 ))
        {
            bByte = (BYTE)SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );
            if( g_ivConfig->Controllers[g_ivConfig->ChosenTab].PakType == bByte )
            {
                SendMessage( hDlgItem, CB_SETCURSEL, i, 0 );
                i = -10;
            }
            else
                i--;
        }
        if( i != -10 )
            SendMessage( hDlgItem, CB_SETCURSEL, 0, 0 );

        // Update Pak-Display
        if( g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRawData )
        {
            i = SendMessage( GetDlgItem( hDlg, IDC_PAKTYPE ), CB_GETCURSEL, 0, 0 );
            bByte = (BYTE)SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );
        }
        else
            bByte = PAK_NONRAW;


        if( bByte == bCurrentPak && hPakWindow )
            SendMessage( hPakWindow, WM_USER_UPDATE, 0, 0 );
        else
        {
            if( hPakWindow )
                DestroyWindow( hPakWindow );

            hDlgItem = GetDlgItem( hDlg, IDC_PAKAREA );

            switch( bByte )
            {
            case PAK_MEM:
                hPakWindow = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_PAK_MEMPAK), hDlg, (DLGPROC)MemPakProc);
                break;
            case PAK_RUMBLE:
                hPakWindow = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_PAK_RUMBLE), hDlg, (DLGPROC)RumblePakProc);
                break;
            case PAK_TRANSFER:
                hPakWindow = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_PAK_TRANSFER), hDlg, (DLGPROC)TransferPakProc);
                break;
            case PAK_ADAPTOID:
                hPakWindow = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_PAK_ADAPTOID), hDlg, (DLGPROC)PakProc);
                break;

            case PAK_VOICE:
            case PAK_NONRAW:
            case PAK_NONE:
            default:
                hPakWindow = CreateDialog(g_hResourceDLL, MAKEINTRESOURCE(IDD_PAK_TEXT), hDlg, (DLGPROC)PakProc);

            }
            if( hPakWindow )
            {
                RECT rectProp, rectMain; // need to know the position of the calling tab window relative to its parent

                GetWindowRect( hDlg, &rectMain );
                GetWindowRect( hDlgItem, &rectProp );

                MoveWindow( hPakWindow, rectProp.left - rectMain.left, rectProp.top - rectMain.top, rectProp.right - rectProp.left, rectProp.bottom - rectProp.top, FALSE );
                ShowWindow( hPakWindow, SW_SHOW );
            }
            bCurrentPak = bByte;
        }
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK PakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TCHAR tszLineOne[DEFAULT_BUFFER] = _T(""), tszLineTwo[DEFAULT_BUFFER] = _T("");
    TCHAR *pszDescription[2] = { tszLineOne, tszLineTwo };
    bool bRAW;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        PakProc( hDlg, WM_USER_UPDATE, 0, 0 );
        return FALSE; // don't give it focus

    case WM_USER_UPDATE:
        bRAW = g_ivConfig->Controllers[g_ivConfig->ChosenTab].fRawData ? 1 : 0;

        switch( g_ivConfig->Controllers[g_ivConfig->ChosenTab].PakType )
        {
        case PAK_MEM:
            LoadString( g_hResourceDLL, IDS_P_MEM_NORAW, pszDescription[0], DEFAULT_BUFFER );
            break;
        case PAK_RUMBLE:
            LoadString( g_hResourceDLL, IDS_P_RUMBLE_NORAW, pszDescription[0], DEFAULT_BUFFER );
            break;
        case PAK_TRANSFER:
            LoadString( g_hResourceDLL, IDS_P_TRANSFER_NORAW, pszDescription[0], DEFAULT_BUFFER );
            break;
        case PAK_VOICE:
            LoadString( g_hResourceDLL, IDS_P_VOICE_RAW, pszDescription[1], DEFAULT_BUFFER );
            pszDescription[0] = pszDescription[1];
            break;
        case PAK_ADAPTOID:
            LoadString( g_hResourceDLL, IDS_P_ADAPTOID_NORAW, pszDescription[0], DEFAULT_BUFFER );
            break;
        case PAK_NONE:
        default:
            LoadString( g_hResourceDLL, IDS_P_NONE_RAWNORAW, pszDescription[1], DEFAULT_BUFFER );
            pszDescription[0] = pszDescription[1];
        }
        SendMessage( GetDlgItem( hDlg, IDT_PAKDESC ), WM_SETTEXT, 0, (LPARAM)pszDescription[bRAW] );

        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK MemPakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static TCHAR *pszMemPakFile;
    static WORD wMemPakState;
    static int iSelectedNote;
    TCHAR szBuffer[MAX_PATH+1], szTemp[MAX_PATH+1];

    HWND hDlgItem;
    long i,j;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        if( g_bRunning )
        {
            EnableWindow( GetDlgItem( hDlg, IDC_CHGDIR ), FALSE );
            LoadString( g_hResourceDLL, IDS_P_MEM_NOCHANGE, szBuffer, MAX_PATH + 1 );
            SendMessage( GetDlgItem( hDlg, IDC_CHGDIR ), WM_SETTEXT, 0, (LPARAM)szBuffer );
        }
        iSelectedNote = -1;
        // Browser
        hDlgItem = GetDlgItem( hDlg, IDC_MEMPAKBROWSER );

        LVCOLUMN lvColumn;

        lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.cx = 147;
        LoadString( g_hResourceDLL, IDS_P_MEM_NAME, szBuffer, MAX_PATH + 1 );
        lvColumn.pszText = szBuffer;
        ListView_InsertColumn( hDlgItem, 0, &lvColumn );

        lvColumn.fmt = LVCFMT_CENTER | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

        lvColumn.cx = 70;
        lvColumn.iSubItem = 1;
        LoadString( g_hResourceDLL, IDS_P_MEM_REGION, szBuffer, MAX_PATH + 1 );
        lvColumn.pszText = szBuffer;
        ListView_InsertColumn( hDlgItem, 1, &lvColumn );

        lvColumn.cx = 50;
        lvColumn.iSubItem = 2;
        LoadString( g_hResourceDLL, IDS_P_MEM_BLOCKS, szBuffer, MAX_PATH + 1 );
        lvColumn.pszText = szBuffer;
        ListView_InsertColumn( hDlgItem, 2, &lvColumn );

        ListView_SetExtendedListViewStyle( hDlgItem, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP );


        MemPakProc( hDlg, WM_USER_UPDATE, 0, 0 ); // setting values
        return FALSE; // don't give it focus

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );
        switch( LOWORD(wParam) )
        {
            TCHAR tszTitle[DEFAULT_BUFFER], tszText[DEFAULT_BUFFER];

        case IDC_MEMPAKLIST:
            if( HIWORD(wParam) == LBN_SELCHANGE )
            {
                i = SendMessage( hDlgItem, LB_GETCURSEL, 0, 0 );
                SendMessage( hDlgItem, LB_GETTEXT, i, (LPARAM)pszMemPakFile );
                MemPakProc( hDlg, WM_USER_UPDATE, 1, 0 );
            }
            return TRUE;
        case IDC_BROWSE:
            lstrcpyn( szBuffer, pszMemPakFile, ARRAYSIZE(szBuffer)  );
            if( BrowseFile( hDlg, szBuffer, BF_MEMPAK, BF_LOAD ))
            {
                if( !CheckFileExists( szBuffer ) )
                {
                    BYTE aMemPak[PAK_MEM_SIZE];

                    FormatMemPak( aMemPak );
                    WriteMemPakFile( szBuffer, aMemPak, true );
                }
                if( CheckFileExists( szBuffer ) )
                {
                    lstrcpyn( pszMemPakFile, szBuffer, MAX_PATH );
                    MemPakProc( hDlg, WM_USER_UPDATE, 0, 0 );
                }
            }
            return TRUE;
        case IDC_CHGDIR:
            if (DialogBox(g_hResourceDLL, MAKEINTRESOURCE(IDD_FOLDERS), hDlg, (DLGPROC)FoldersDialogProc) == TRUE)
                MemPakProc( hDlg, WM_USER_UPDATE, 0, 0 );
            return TRUE;

        case IDC_DELMEMPAK:
            LoadString( g_hResourceDLL, IDS_DLG_DELETEPAK, tszText, DEFAULT_BUFFER );
            LoadString( g_hResourceDLL, IDS_DLG_MSG_TITLE, tszTitle, DEFAULT_BUFFER );
            if( MessageBox( hDlg, tszText, tszTitle, MB_OKCANCEL | MB_ICONQUESTION ) == IDOK )
            {
                GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );
                DeleteFile( szBuffer );
                pszMemPakFile[0] = '\0';
                MemPakProc( hDlg, WM_USER_UPDATE, 0, 0 );
            }
            return TRUE;

        case IDC_FORMATMEMPAK:
            LoadString( g_hResourceDLL, IDS_DLG_FORMATPAK, tszText, DEFAULT_BUFFER );
            LoadString( g_hResourceDLL, IDS_DLG_MSG_TITLE, tszTitle, DEFAULT_BUFFER );
            if( MessageBox( hDlg, tszText, tszTitle, MB_OKCANCEL | MB_ICONQUESTION ) == IDOK )
            {
                GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );
                BYTE aMemPak[PAK_MEM_SIZE];

                FormatMemPak( aMemPak );
                WriteMemPakFile( szBuffer, aMemPak, false );
                MemPakProc( hDlg, WM_USER_UPDATE, 1, 0 );
            }
            return TRUE;
        case IDC_SAVENOTE:
            {
                BYTE aMemPak[PAK_MEM_SIZE];
                if( g_bRunning && ( HIWORD(wMemPakState) == MPAK_INUSE )
                    && g_pcControllers[g_ivConfig->ChosenTab].pPakData
                    && ( *(BYTE*)g_pcControllers[g_ivConfig->ChosenTab].pPakData == PAK_MEM ))
                {
                    EnterCriticalSection( &g_critical );
                    CopyMemory( aMemPak, ((MEMPAK*)g_pcControllers[g_ivConfig->ChosenTab].pPakData)->aMemPakData, PAK_MEM_SIZE );
                    LeaveCriticalSection( &g_critical );
                }
                else
                {
                    GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );
                    ReadMemPakFile( szBuffer, aMemPak, false );
                }

                int iChars = TranslateNotes( &aMemPak[0x300 + iSelectedNote * 32 + 0x10], szBuffer, 16 );

                if( TranslateNotes( &aMemPak[0x300 + iSelectedNote * 32 + 0x0C], &szBuffer[iChars + 1], 1 ) )
                    szBuffer[iChars] = _T('_');

                if( BrowseFile( hDlg, szBuffer, BF_NOTE, BF_SAVE ))
                {
                    TCHAR szAbsoluteMemPak[MAX_PATH+1];
                    GetAbsoluteFileName( szAbsoluteMemPak, pszMemPakFile, DIRECTORY_MEMPAK );

                    SaveNoteFileA( aMemPak, iSelectedNote, szBuffer );
                }
            }
            return TRUE;
        case IDC_INSERTNOTE:
            {
                GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );
                BYTE aMemPak[PAK_MEM_SIZE];
                ReadMemPakFile( szBuffer, aMemPak, false );
                szBuffer[0] = '\0';

                if( BrowseFile( hDlg, szBuffer, BF_NOTE, BF_LOAD ))
                {
                    if( InsertNoteFile( aMemPak, szBuffer ) )
                    {
                        GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );
                        WriteMemPakFile( szBuffer, aMemPak, false );
                        MemPakProc( hDlg, WM_USER_UPDATE, 1, 0 );
                    }
                }
            }
            return TRUE;
        case IDC_DELETENOTE:
            LoadString( g_hResourceDLL, IDS_DLG_DELETENOTE, tszText, DEFAULT_BUFFER );
            LoadString( g_hResourceDLL, IDS_DLG_MSG_TITLE, tszTitle, DEFAULT_BUFFER );
            if( MessageBox( hDlg, tszText, tszTitle, MB_OKCANCEL | MB_ICONQUESTION ) == IDOK )
            {
                GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );
                BYTE aMemPak[PAK_MEM_SIZE];
                ReadMemPakFile( szBuffer, aMemPak, false );
                if( RemoveNote( aMemPak, iSelectedNote ) )
                {
                    WriteMemPakFile( szBuffer, aMemPak, false );
                    MemPakProc( hDlg, WM_USER_UPDATE, 1, 0 );
                }
            }
            return TRUE;
        default:
            return FALSE;
        }

    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam)->idFrom )
        {
        case IDC_MEMPAKBROWSER:
            switch( ((LPNMHDR)lParam)->code )
            {
            case LVN_ITEMCHANGED:
                if( ((LPNMLISTVIEW)lParam)->uNewState & LVIS_SELECTED )
                {
                    iSelectedNote = ((LPNMLISTVIEW)lParam)->lParam;
                    if( HIBYTE(wMemPakState) & MPAK_READABLE )
                        EnableWindow( GetDlgItem( hDlg, IDC_SAVENOTE ), TRUE );
                    if( HIBYTE(wMemPakState) & MPAK_WRITEABLE )
                        EnableWindow( GetDlgItem( hDlg, IDC_DELETENOTE ), TRUE );
                }
                else
                {
                    iSelectedNote = -1;
                    EnableWindow( GetDlgItem( hDlg, IDC_SAVENOTE ), FALSE );
                    EnableWindow( GetDlgItem( hDlg, IDC_DELETENOTE ), FALSE );
                }
                return TRUE;
            default:
                return FALSE;
            }
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        pszMemPakFile = g_ivConfig->Controllers[g_ivConfig->ChosenTab].szMempakFile;
        iSelectedNote = -1;

        // Mempak List Window
        if( wParam == 0 && lParam == 0 )
        {
            hDlgItem = GetDlgItem( hDlg, IDC_MEMPAKLIST );
            SendMessage( hDlgItem, LB_RESETCONTENT, 0, 0 );
            SendFilestoList( hDlgItem, FILIST_MEM );

            if( pszMemPakFile[1] == ':' || ( pszMemPakFile[1] == '\\' && pszMemPakFile[0] == '\\' ))
            {
                GetDirectory( szBuffer, DIRECTORY_MEMPAK );
                i = lstrlen( szBuffer );
                if( !_tcsncmp( szBuffer, pszMemPakFile, i ))
                {
                    lstrcpyn( szBuffer, &pszMemPakFile[i], MAX_PATH );
                    lstrcpyn( pszMemPakFile, szBuffer, MAX_PATH );
                }

            }

            j = -1;
            if( !( pszMemPakFile[1] == ':' || ( pszMemPakFile[1] == '\\' && pszMemPakFile[0] == '\\' )) )
            {
                i = SendMessage( hDlgItem, LB_FINDSTRINGEXACT, (WPARAM)(-1), (LPARAM)pszMemPakFile );
                if( i != LB_ERR )
                    j = i;
            }
            SendMessage( hDlgItem, LB_SETCURSEL, j, 0 );
        }
        // Mempak List Window End

        // MamPak Full Path+Name
        GetAbsoluteFileName( szBuffer, pszMemPakFile, DIRECTORY_MEMPAK );

        // MemPak Browser
        ListView_DeleteAllItems( GetDlgItem( hDlg, IDC_MEMPAKBROWSER ));
        if( (!lstrcmpi( &szBuffer[lstrlen(szBuffer)-4], _T(".mpk") ))
            || (!lstrcmpi( &szBuffer[lstrlen(szBuffer)-4], _T(".n64") )))
        {
            BYTE aMemPakHeader[0x500];

            bool bMemPakUsed = false;
            // first, if we're running emulation we need to make sure we haven't selected a file that's currently in use
            if (g_bRunning)
            {
                TCHAR szMemPakFile[MAX_PATH+1];
                for( i = 0; i < 4; ++i )
                {
                    if( g_pcControllers[i].pPakData && ( *(BYTE*)g_pcControllers[i].pPakData == PAK_MEM ) && !((LPMEMPAK)g_pcControllers[i].pPakData)->fReadonly )
                    {
                        GetAbsoluteFileName( szMemPakFile, g_pcControllers[i].szMempakFile, DIRECTORY_MEMPAK );
                        if( !lstrcmp( szMemPakFile, szBuffer ))
                        {
                            // grab the file info from memory instead of the file... but keep in mind we can't do anything dangerous with it
                            EnterCriticalSection( &g_critical );
                            wMemPakState = ShowMemPakContent( ((MEMPAK*)g_pcControllers[i].pPakData)->aMemPakData, GetDlgItem( hDlg, IDC_MEMPAKBROWSER ));
                            LeaveCriticalSection( &g_critical );
                            if (HIBYTE(wMemPakState) == MPAK_OK)
                            {
                                LoadString( g_hResourceDLL, IDS_P_MEM_INUSE, szTemp, MAX_PATH + 1 );
                                wsprintf( szBuffer, szTemp, LOBYTE(wMemPakState) );
                                wMemPakState = MAKEWORD( MPAK_READABLE, MPAK_INUSE );
                            }
                            bMemPakUsed = true;
                        }
                    }
                }
            }

            if( !bMemPakUsed )
            {
                HANDLE hFile;
                hFile = CreateFile( szBuffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
                if ( hFile != INVALID_HANDLE_VALUE )
                {
                    DWORD dwFileSize = GetFileSize( hFile, NULL );
                    DWORD dwCorrectFileSize = PAK_MEM_SIZE;

                    TCHAR *pcPoint = _tcsrchr( szBuffer, _T('.') );
                    if( !lstrcmpi( pcPoint, _T(".n64") ) )
                    {
                        SetFilePointer( hFile, PAK_MEM_DEXOFFSET, NULL, FILE_BEGIN );
                        dwCorrectFileSize += PAK_MEM_DEXOFFSET;
                    }
                    else
                        SetFilePointer( hFile, 0L, NULL, FILE_BEGIN );

                    if( dwFileSize > ( dwCorrectFileSize - 0x7500 ))
                    {
                        DWORD dwBytesRead;

                        ReadFile( hFile, aMemPakHeader, 0x500, &dwBytesRead, NULL);
                        ListView_DeleteAllItems( GetDlgItem( hDlg, IDC_MEMPAKBROWSER ));
                        wMemPakState = ShowMemPakContent( aMemPakHeader, GetDlgItem( hDlg, IDC_MEMPAKBROWSER ));
                    }
                    else
                        wMemPakState = MAKEWORD( 0, MPAK_DAMAGED );

                    if( HIBYTE( wMemPakState ) == MPAK_OK &&
                        dwFileSize != dwCorrectFileSize )
                        wMemPakState = MAKEWORD( LOBYTE( wMemPakState ), MPAK_WRONGSIZE );

                    CloseHandle( hFile );
                }
                else
                    wMemPakState = MAKEWORD( 0, MPAK_ERROR );
            }
        }
        else
            wMemPakState = MAKEWORD( 0, MPAK_NOSELECTION );

        switch( HIBYTE(wMemPakState) )
        {
        case MPAK_OK:
            LoadString( g_hResourceDLL, IDS_P_MEM_BLOCKSFREE, szTemp, MAX_PATH + 1 );
            wsprintf( szBuffer, szTemp, LOBYTE(wMemPakState) );
            break;
        case MPAK_INUSE:
            // text field already set
            break;
        case MPAK_WRONGSIZE:
            LoadString( g_hResourceDLL, IDS_P_MEM_WRONGSIZE, szTemp, MAX_PATH + 1 );
            wsprintf( szBuffer, szTemp, LOBYTE(wMemPakState) );
            break;
        case MPAK_NOSELECTION:
            LoadString( g_hResourceDLL, IDS_P_MEM_NONESELECTED, szBuffer, MAX_PATH + 1 );
            break;
        case MPAK_DAMAGED:
            LoadString( g_hResourceDLL, IDS_P_MEM_DAMAGED, szBuffer, MAX_PATH + 1 );
            break;

        case MPAK_ERROR:
        default:
            {
                LoadString( g_hResourceDLL, IDS_P_MEM_ERROR, szTemp, MAX_PATH + 1 );
                DWORD dwError = GetLastError();
                DWORD dwLength = wsprintf( szBuffer, szTemp, dwError );
                FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError , 0, &szBuffer[dwLength], ARRAYSIZE(szBuffer) - dwLength - 1, NULL );
            }
            break;
        }
        SendMessage( GetDlgItem( hDlg, IDT_MEMPAKSTATE ), WM_SETTEXT, 0, (LPARAM)szBuffer );

        if( HIBYTE(wMemPakState) & MPAK_FORMATABLE )
        {
            EnableWindow( GetDlgItem( hDlg, IDC_FORMATMEMPAK ), TRUE );
            EnableWindow( GetDlgItem( hDlg, IDC_DELMEMPAK ), TRUE );
        }
        else
        {
            EnableWindow( GetDlgItem( hDlg, IDC_FORMATMEMPAK ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_DELMEMPAK ), FALSE );
        }

        if( HIBYTE(wMemPakState) & MPAK_WRITEABLE )
            EnableWindow( GetDlgItem( hDlg, IDC_INSERTNOTE ), TRUE );
        else
            EnableWindow( GetDlgItem( hDlg, IDC_INSERTNOTE ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_SAVENOTE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_DELETENOTE ), FALSE );

        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK RumblePakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static BOOL bNewRumbleTyp = true;
    IFDEVICE *pifDevice = &g_ivConfig->FFDevices[g_ivConfig->ChosenTab];
    CONTROLLER *pcController = &g_ivConfig->Controllers[g_ivConfig->ChosenTab];
    TCHAR szBuffer[DEFAULT_BUFFER], szTemp[DEFAULT_BUFFER];
    HWND hDlgItem;
    long i = 0,j;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        hDlgItem = GetDlgItem( hDlg, IDC_RUMBLESTRENGTH );

        SendMessage( hDlgItem, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 100 ));
        SendMessage( hDlgItem, TBM_SETTICFREQ, (WPARAM) 10, 0 );
        SendMessage( hDlgItem, TBM_SETPAGESIZE, (WPARAM) 0, 1 );

        RumblePakProc( hDlg, WM_USER_UPDATE, 0, 0 );
        return FALSE; // don't give it focus

    case WM_COMMAND:
        hDlgItem = GetDlgItem( hDlg, LOWORD(wParam) );

        switch( LOWORD(wParam) )
        {
        case IDC_CTRDEVICE:
            if( HIWORD(wParam) == CBN_SELCHANGE )
            {
                i = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
                j = SendMessage( hDlgItem, CB_GETITEMDATA, i, 0 );

                EnterCriticalSection(&g_critical);
                if( j == -1 )
                {
                    pifDevice->bProductCounter = 0;
                    pifDevice->szProductName[0] = '\0';
                }
                else
                {
                    pifDevice->bProductCounter = g_devList[j].bProductCounter;
                    lstrcpyn( pifDevice->szProductName, g_devList[j].szProductName, sizeof(pifDevice->szProductName) / sizeof(TCHAR) );
                }
                LeaveCriticalSection(&g_critical);

                RumblePakProc( hDlg, WM_USER_UPDATE, 0, 0 ); // en/disabling RumbleOptions
            }
            return TRUE;

        // the following three cases use fallthrough assignment
        case IDC_RUMBLE1:
            if( LOWORD(wParam) == IDC_RUMBLE1 )
                i = RUMBLE_EFF1;
        case IDC_RUMBLE2:
            if( LOWORD(wParam) == IDC_RUMBLE2 )
                i = RUMBLE_EFF2;
        case IDC_RUMBLE3:
            if( LOWORD(wParam) == IDC_RUMBLE3 )
                i = RUMBLE_EFF3;
            if(( HIWORD(wParam) == BN_CLICKED ) &&
                ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED ))
            {
                pcController->bRumbleTyp = (BYTE)i;
                bNewRumbleTyp = true;
            }
            return TRUE;

        case IDC_VISUALRUMBLE:
            pcController->fVisualRumble = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
            return TRUE;

        case IDC_RUMBLETEST:
            if ( !g_pConfigEffect || bNewRumbleTyp )
            {
                EnterCriticalSection(&g_critical);
                ReleaseEffect( g_pConfigEffect );
                if ( !CreateEffectHandle(g_hMainDialog, g_pConfigDevice, g_pConfigEffect, pcController->bRumbleTyp, pcController->bRumbleStrength) )
                {
                    DebugWriteA("Interface: CreateEffectHandle failed\n");
                }
                bNewRumbleTyp = false;
                LeaveCriticalSection(&g_critical);
            }
            g_pConfigDevice->Acquire();
            {
                HRESULT hRslt = g_pConfigEffect->Start(1, 0);
                if (hRslt != DI_OK )
                {
                    switch (hRslt)
                    {
                    case DIERR_INCOMPLETEEFFECT:
                        DebugWriteA("Test Rumble: DIError: incomplete effect.\n");
                        break;
                    case DIERR_INVALIDPARAM:
                        DebugWriteA("Test Rumble: DIError: invalid param.\n");
                        break;
                    case DIERR_NOTEXCLUSIVEACQUIRED:
                        DebugWriteA("Test Rumble: DIError: not exclusive acquired.\n");
                        break;
                    case DIERR_NOTINITIALIZED:
                        DebugWriteA("Test Rumble: DIError: not initialized.\n");
                        break;
                    case DIERR_UNSUPPORTED:
                        DebugWriteA("Test Rumble: DIError: unsupported.\n");
                        break;
                    default:
                        DebugWriteA("Test Rumble: DIError: undocumented: %lX\n", hRslt);
                    }
                }
                else
                    DebugWriteA("Test Rumble: OK\n");
            }
            return TRUE;

        default:
            return FALSE;
        }

    case WM_HSCROLL: // TrackBars
    case WM_VSCROLL:
        i = GetWindowLong( (HWND)lParam, GWL_ID );
        switch( i )
        {
        case IDC_RUMBLESTRENGTH:
            pcController->bRumbleStrength = (BYTE)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0 );
            LoadString( g_hResourceDLL, IDS_D_RUMBLESTR, szTemp, DEFAULT_BUFFER );
            wsprintf( szBuffer, szTemp, pcController->bRumbleStrength );
            SendMessage( GetDlgItem( hDlg, IDT_RUMBLESTRENGTH ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            bNewRumbleTyp = true;
            return TRUE;
        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        // filling DropDownlist with devices
        hDlgItem = GetDlgItem( hDlg, IDC_CTRDEVICE );
        SendMessage (hDlgItem, CB_RESETCONTENT, 0, 0); // HACK: yeah this isn't the best way to do this, but it works.
        LoadString( g_hResourceDLL, IDS_P_R_NODEVICE, szBuffer, DEFAULT_BUFFER );
        j = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)szBuffer );
        SendMessage( hDlgItem, CB_SETITEMDATA, j, -1 );
        for( i = 0;  i < g_nDevices; i++ )
        {
            bool bMatch = false;
            // if device is not already set as a FF device in some other tab
            for( int m = 0; m < 4; m++)
            {
                if( g_ivConfig->ChosenTab != m)
                    if (g_devList[i].bProductCounter == g_ivConfig->FFDevices[m].bProductCounter && !lstrcmp( g_devList[i].szProductName, g_ivConfig->FFDevices[m].szProductName ) )
                        bMatch = true;
            }

            if (!bMatch && g_devList[i].bEffType != 0 )
            {
                if( g_devList[i].bProductCounter == 0 )
                    lstrcpyn( szBuffer, g_devList[i].szProductName, ARRAYSIZE(szBuffer) );
                else
                    wsprintf( szBuffer, _T("%s %i"), g_devList[i].szProductName, g_devList[i].bProductCounter );

                j = SendMessage( hDlgItem, CB_ADDSTRING, 0, (LPARAM)szBuffer );
                SendMessage( hDlgItem, CB_SETITEMDATA, j, i );
            }
        }
        // DropDownlist End

        EnterCriticalSection(&g_critical);
        j = FindDeviceinList( pifDevice->szProductName, pifDevice->bProductCounter, true );
        hDlgItem = GetDlgItem( hDlg, IDC_CTRDEVICE );
        ReleaseEffect( g_pConfigEffect );
        ReleaseDevice( g_pConfigDevice );
        if( j == -1 || !g_devList[j].bEffType)
        {
            SendMessage( hDlgItem, CB_SETCURSEL, 0, 0 ); // set "None"
            EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE1 ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE2 ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE3 ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_RUMBLESTRENGTH ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDT_RUMBLESTRENGTH ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_RUMBLETEST ), FALSE );
        }
        else
        {
            if (! GetInputDevice( g_hMainDialog, g_pConfigDevice, g_devList[j].guidInstance, g_devList[j].dwDevType, DIB_FF ) )
            {
                g_pConfigDevice = NULL;
                DebugWriteA("Could not GetInputDevice in user update, RumblePakProc.\n");
            }
            else
            {
                DebugWriteA("GetInputDevice in RumblePakProc: OK\n");
            }

            // DropDownList
            if( g_devList[j].bProductCounter == 0 )
                i = SendMessage( hDlgItem, CB_FINDSTRINGEXACT, (WPARAM)(-1), (LPARAM)g_devList[j].szProductName );
            else
            {
                wsprintf( szBuffer, _T("%s %i"), g_devList[j].szProductName, g_devList[j].bProductCounter );
                i = SendMessage( hDlgItem, CB_FINDSTRINGEXACT, (WPARAM)(-1), (LPARAM)szBuffer ); // search index of Device-String
            }

            SendMessage( hDlgItem, CB_SETCURSEL, i, 0 ); // select the right string

            if( g_devList[j].bEffType & RUMBLE_EFF1 )
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE1 ), TRUE );
            else
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE1 ), FALSE );
            if( g_devList[j].bEffType & RUMBLE_EFF2 )
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE2 ), TRUE );
            else
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE2 ), FALSE );
            if( g_devList[j].bEffType & RUMBLE_EFF3 )
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE3 ), TRUE );
            else
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLE3 ), FALSE );

            if( ( g_devList[j].bEffType & RUMBLE_EFF1 ) ||
                ( g_devList[j].bEffType & RUMBLE_EFF2 ))
            {
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLESTRENGTH ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDT_RUMBLESTRENGTH ), TRUE );
            }
            else
            {
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLESTRENGTH ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDT_RUMBLESTRENGTH ), FALSE );
            }

            if( g_devList[j].bEffType != RUMBLE_NONE )
            {
                // if (!CreateEffectHandle(g_hMainDialog, g_pConfigDevice, g_pConfigEffect, pcController->bRumbleTyp, pcController->bRumbleStrength) )
                // {
                //  DebugWriteA("Interface: CreateEffectHandle failed\n");
                // }
                bNewRumbleTyp = true;
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLETEST ), TRUE );
            }
            else
                EnableWindow( GetDlgItem( hDlg, IDC_RUMBLETEST ), FALSE );

        }
        LeaveCriticalSection(&g_critical);

        if( pcController->bRumbleTyp == RUMBLE_EFF1 )
            CheckDlgButton( hDlg, IDC_RUMBLE1, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_RUMBLE1, BST_UNCHECKED );

        if( pcController->bRumbleTyp == RUMBLE_EFF2 )
            CheckDlgButton( hDlg, IDC_RUMBLE2, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_RUMBLE2, BST_UNCHECKED );

        if( pcController->bRumbleTyp == RUMBLE_EFF3 )
            CheckDlgButton( hDlg, IDC_RUMBLE3, BST_CHECKED );
        else
            CheckDlgButton( hDlg, IDC_RUMBLE3, BST_UNCHECKED );

        CheckDlgButton( hDlg, IDC_VISUALRUMBLE, pcController->fVisualRumble ? BST_CHECKED : BST_UNCHECKED );

        // TrackBars
        SendMessage( GetDlgItem( hDlg, IDC_RUMBLESTRENGTH ), TBM_SETPOS, TRUE, pcController->bRumbleStrength );
        LoadString( g_hResourceDLL, IDS_D_RUMBLESTR, szTemp, DEFAULT_BUFFER );
        wsprintf( szBuffer, szTemp, pcController->bRumbleStrength );
        SendMessage( GetDlgItem( hDlg, IDT_RUMBLESTRENGTH ), WM_SETTEXT , 0, (LPARAM)szBuffer );

        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK TransferPakProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static LPTSTR pszGBRomFile = NULL;
    static LPTSTR pszGBSaveFile = NULL;
    TCHAR tszMsg[DEFAULT_BUFFER];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        if( g_bRunning )
        {
            EnableWindow( GetDlgItem( hDlg, IDC_CHGDIR ), FALSE );
            LoadString( g_hResourceDLL, IDS_P_TRANS_NOCHANGE, tszMsg, DEFAULT_BUFFER );
            SendMessage( GetDlgItem( hDlg, IDC_CHGDIR ), WM_SETTEXT, 0, (LPARAM)tszMsg );
        }

        TransferPakProc( hDlg, WM_USER_UPDATE, 0, 0 ); // setting values
        return FALSE; // don't give it focus

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDC_GBROM_EDIT:
            if( HIWORD(wParam) == EN_CHANGE )
                GetDlgItemText( hDlg, IDC_GBROM_EDIT, pszGBRomFile, ARRAYSIZE(g_ivConfig->Controllers->szTransferRom) );
            return TRUE;

        case IDC_GBSAVE_EDIT:
            if( HIWORD(wParam) == EN_CHANGE )
                GetDlgItemText( hDlg, IDC_GBSAVE_EDIT, pszGBSaveFile, ARRAYSIZE(g_ivConfig->Controllers->szTransferSave) );
            return TRUE;

        case IDC_GBROM_BROWSE:
            {
                TCHAR szBuffer[MAX_PATH+1];
                GetAbsoluteFileName( szBuffer, pszGBRomFile, DIRECTORY_GBROMS );
                if( BrowseFile( hDlg, szBuffer, BF_GBROM, BF_LOAD ))
                {
                    lstrcpyn( pszGBRomFile, szBuffer, ARRAYSIZE(g_ivConfig->Controllers->szTransferRom) );
                    TransferPakProc( hDlg, WM_USER_UPDATE, 0, 0 );
                }
            }
            return TRUE;

        case IDC_GBSAVE_BROWSE:
            {
                TCHAR szBuffer[MAX_PATH+1];
                GetAbsoluteFileName( szBuffer, pszGBSaveFile, DIRECTORY_GBSAVES );
                if( BrowseFile( hDlg, szBuffer, BF_GBSAVE, BF_LOAD ))
                {
                    lstrcpyn( pszGBSaveFile, szBuffer, ARRAYSIZE(g_ivConfig->Controllers->szTransferSave) );
                    TransferPakProc( hDlg, WM_USER_UPDATE, 0, 0 );
                }
            }
            return TRUE;

        case IDC_CHGDIR:
            if (DialogBox(g_hResourceDLL, MAKEINTRESOURCE(IDD_FOLDERS), hDlg, (DLGPROC)FoldersDialogProc) == TRUE)
                TransferPakProc( hDlg, WM_USER_UPDATE, 0, 0 );
            return TRUE;

        default:
            return FALSE;
        }

    case WM_USER_UPDATE:
        pszGBRomFile = g_ivConfig->Controllers[g_ivConfig->ChosenTab].szTransferRom;
        pszGBSaveFile = g_ivConfig->Controllers[g_ivConfig->ChosenTab].szTransferSave;

        SetDlgItemText( hDlg, IDC_GBROM_EDIT, pszGBRomFile );
        SetDlgItemText( hDlg, IDC_GBSAVE_EDIT, pszGBSaveFile );
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK ShortcutsTabProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static bool bScanRunning;
    static DWORD dwButtonID[3];
    static DWORD dwCounter;
    static HWND hFocus = NULL;
    static int iPlayer = 5;     // HACK: player "5" is obviously out of bounds, so indicates no control has been set

    long i;
    static HWND hBlocker = NULL;

    TCHAR szBuffer[40];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        bScanRunning = false;
        iPlayer = 5;

        ShortcutsTabProc( hDlg, WM_USER_UPDATE, 0, 0 ); // setting values
        return FALSE; // don't give it focus

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDC_SHOWMESSAGES:
            g_ivConfig->fDisplayShortPop = ( IsDlgButtonChecked( hDlg, LOWORD(wParam) ) == BST_CHECKED );
            return TRUE;
        case IDC_LOCKMOUSE:
            iPlayer = -1;

        case IDC_SETNOPAK_P1:
        case IDC_SETMEMPAK_P1:
        case IDC_SETRUMBLEPAK_P1:
        case IDC_SETTRANSFERPAK_P1:
        case IDC_SETADAPTOIDPAK_P1:
        case IDC_SWMEMRUMBLE_P1:
        case IDC_SWMEMADAPTOID_P1:
            if (iPlayer > -1)
                iPlayer = 0;

        case IDC_SETNOPAK_P2:
        case IDC_SETMEMPAK_P2:
        case IDC_SETRUMBLEPAK_P2:
        case IDC_SETTRANSFERPAK_P2:
        case IDC_SETADAPTOIDPAK_P2:
        case IDC_SWMEMRUMBLE_P2:
        case IDC_SWMEMADAPTOID_P2:
            if (iPlayer > 0)
                iPlayer = 1;

        case IDC_SETNOPAK_P3:
        case IDC_SETMEMPAK_P3:
        case IDC_SETRUMBLEPAK_P3:
        case IDC_SETTRANSFERPAK_P3:
        case IDC_SETADAPTOIDPAK_P3:
        case IDC_SWMEMRUMBLE_P3:
        case IDC_SWMEMADAPTOID_P3:
            if (iPlayer > 1)
                iPlayer = 2;

        case IDC_SETNOPAK_P4:
        case IDC_SETMEMPAK_P4:
        case IDC_SETRUMBLEPAK_P4:
        case IDC_SETTRANSFERPAK_P4:
        case IDC_SETADAPTOIDPAK_P4:
        case IDC_SWMEMRUMBLE_P4:
        case IDC_SWMEMADAPTOID_P4:
            if (iPlayer > 2)
                iPlayer = 3;

            if( bScanRunning )
            {
/*              bScanRunning = false;
                KillTimer( hDlg, TIMER_BUTTON );
                if( ((LPBYTE)&g_ivConfig->Shortcuts)[dwButtonID[2]] == VK_ESCAPE )
                    lstrcpyn( szKeyname, "Unassigned", sizeof(szKeyname) );
                else
                    GetKeyNameText(( MapVirtualKey( ((LPBYTE)&g_ivConfig->Shortcuts)[dwButtonID[2]], 0 ) << 16 ), szKeyname, sizeof(szKeyname) );

                SendMessage( GetDlgItem( hDlg, dwButtonID[1] ), WM_SETTEXT, 0, (LPARAM)szKeyname ); */; // do nothing!
            }
            if( HIWORD(wParam) == BN_CLICKED )
            {
                EnterCriticalSection(&g_critical);
                dwButtonID[0] = LOWORD(wParam);
                dwCounter = 0;
                GetButtonID( dwButtonID, 0, BSET_SHORTCUTS );
                if (hFocus == NULL)
                    hFocus = SetFocus( NULL );
                hBlocker = MakeOverlay();

                SetTimer( hDlg, TIMER_BUTTON, INTERVAL_BUTTON, NULL );
                bScanRunning = true;
                LeaveCriticalSection(&g_critical);
            }
            return TRUE;

        case IDC_SETDEFAULTSC:
            {
                TCHAR tszTitle[DEFAULT_BUFFER], tszMsg[DEFAULT_BUFFER];

                LoadString( g_hResourceDLL, IDS_DLG_SHORTCUTRESTORE, tszMsg, DEFAULT_BUFFER );
                LoadString( g_hResourceDLL, IDS_DLG_CONTROLRESTORE_TITLE, tszTitle, DEFAULT_BUFFER );

                if( MessageBox( hDlg, tszMsg, tszTitle, MB_OKCANCEL | MB_ICONWARNING ) == IDOK )
                {
                    LoadShortcutsFromResource(true);
                    ShortcutsTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
                }
            }
            return TRUE;

        case IDC_SAVESHORTCUTS:
            {
                TCHAR szFilename[MAX_PATH+1] = _T( "" );
                if( BrowseFile( hDlg, szFilename, BF_SHORTCUTS, BF_SAVE ))
                {
                    FILE * fFile = _tfopen( szFilename, _T("wS") );
                    if (fFile)
                    {
                        fprintf(fFile, "@" STRING_PROFILEVERSION "\n\n");
                        FormatShortcutsBlock( fFile, false );

                        fclose(fFile);
                    }
                    else
                        WarningMessage( IDS_ERR_PROFWRITE, MB_OK );
                }
            }
            return TRUE;

        case IDC_LOADSHORTCUTS:
            {
                TCHAR szFilename[MAX_PATH+1] = TEXT( "" );
                if( BrowseFile( hDlg, szFilename, BF_SHORTCUTS, BF_LOAD ))
                {
                    DebugWrite(_T("Config interface: Load shortcuts file: %s\n"), szFilename);
                    if( LoadShortcutsFile( szFilename ))
                        ShortcutsTabProc( hDlg, WM_USER_UPDATE, 0, 0 );
                    else
                        WarningMessage( IDS_ERR_SHORTREAD, MB_OK );

                }
            }
            return TRUE;

        default:
            return FALSE;
        }

    case WM_TIMER: // when assigning shortcuts, this gets called every 20ms (or value in INTERVAL_BUTTON)
        if( wParam == TIMER_BUTTON && bScanRunning )
        {
            BUTTON newButton;

            i = ScanDevices( &dwCounter, &newButton);
            if( i || dwCounter > 500 )
            {
                bScanRunning = false;
                KillTimer( hDlg, TIMER_BUTTON );

                EnterCriticalSection(&g_critical);
                if( i == SC_SCANESCAPE ) // Scan aborted
                {
                    if (iPlayer == -1)
                        ZeroMemory(&g_ivConfig->Shortcuts.bMouseLock, sizeof(BUTTON) );
                    else
                        ZeroMemory(&g_ivConfig->Shortcuts.Player[iPlayer].aButtons[dwButtonID[2] % SC_TOTAL], sizeof(BUTTON));
                }
                else if( i == SC_SCANSUCCEED  ) // Got a key, mouseclick, joybutton, or axis
                    if (iPlayer == -1)
                        g_ivConfig->Shortcuts.bMouseLock = newButton;
                    else
                        g_ivConfig->Shortcuts.Player[iPlayer].aButtons[dwButtonID[2] % SC_TOTAL] = newButton;
                DestroyWindow( hBlocker );


                if (iPlayer == -1)
                    GetButtonText( g_ivConfig->Shortcuts.bMouseLock, szBuffer );
                else
                    GetButtonText( g_ivConfig->Shortcuts.Player[iPlayer].aButtons[dwButtonID[2] % SC_TOTAL], szBuffer );
                iPlayer = 5;        // reset invalid player value
                LeaveCriticalSection(&g_critical);

                SendMessage( GetDlgItem( hDlg, dwButtonID[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
                if( hFocus != NULL )
                {
                    SetFocus( hFocus );
                    hFocus = NULL;
                }
            }
            else
            {
                if(( dwCounter % 50 ) == 0 )
                {
                    TCHAR tszText[DEFAULT_BUFFER];

                    LoadString( g_hResourceDLL, IDS_C_POLLING, tszText, DEFAULT_BUFFER );
                    wsprintf( szBuffer, tszText, 10 - dwCounter / 50 );
                    SendMessage( GetDlgItem( hDlg, dwButtonID[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
                }
                ++dwCounter;
            }
            return TRUE;
        }
        else
            return FALSE;

    case WM_USER_UPDATE:
        bScanRunning = false;
        KillTimer( hDlg, TIMER_BUTTON );

        for ( i = 0; i < 4; i++ ) // controllers
            for( int j = 0; j < SC_TOTAL * 4; j++ ) // shortcuts
            {
                DWORD aIDs[3];
                aIDs[2] = i * SC_TOTAL + j;
                if( !GetButtonID( aIDs, 2, BSET_SHORTCUTS ))
                    continue; // for not implemented Buttons, otherwise the behaviour is unsafe

                GetButtonText( g_ivConfig->Shortcuts.Player[i].aButtons[j], szBuffer );
                SendMessage( GetDlgItem( hDlg, aIDs[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
            }

        DWORD aIDs[3];
        aIDs[2] = (DWORD)(-1);
        if( GetButtonID( aIDs, 2, BSET_SHORTCUTS ) )
        {
            GetButtonText( g_ivConfig->Shortcuts.bMouseLock, szBuffer );
            SendMessage( GetDlgItem( hDlg, aIDs[1] ), WM_SETTEXT , 0, (LPARAM)szBuffer );
        }

        CheckDlgButton( hDlg, IDC_SHOWMESSAGES, g_ivConfig->fDisplayShortPop ? BST_CHECKED : BST_UNCHECKED );
        return TRUE;

    default:
        return FALSE; //false means the msg didn't got processed
    }
}

BOOL CALLBACK FoldersDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    long i,j;
    TCHAR szBuffer[MAX_PATH+1];
    TCHAR szApplication[MAX_PATH+1],
          *pcSlash;

    switch(uMsg)
    {
    case WM_INITDIALOG:

#if defined(HIDEUNIMPLEMENTED) && !defined(V_TRANSFERPAK)
        {
            RECT rRect, rGBSave;
            GetWindowRect( GetDlgItem( hDlg, IDC_GBSAVEPANEL ), &rGBSave );
            GetWindowRect( GetDlgItem( hDlg, IDC_MEMPAKPANEL ), &rRect );
            int iShift = rRect.bottom - rGBSave.bottom;

            WINDOWPLACEMENT pos;
            pos.length = sizeof(WINDOWPLACEMENT);

            GetWindowPlacement( GetDlgItem( hDlg, IDOK ), &pos );
            pos.rcNormalPosition.top += iShift;
            pos.rcNormalPosition.bottom += iShift;
            SetWindowPlacement( GetDlgItem( hDlg, IDOK ), &pos );

            GetWindowPlacement( GetDlgItem( hDlg, IDCANCEL ), &pos );
            pos.rcNormalPosition.top += iShift;
            pos.rcNormalPosition.bottom += iShift;
            SetWindowPlacement( GetDlgItem( hDlg, IDCANCEL ), &pos );

            GetWindowRect( hDlg, &rRect );
            rRect.bottom += iShift;

            SetWindowPos( hDlg, NULL, rRect.left, rRect.top, rRect.right-rRect.left, rRect.bottom-rRect.top, SWP_NOMOVE | SWP_NOZORDER );

            int aChilds[] = {   IDC_GBROMPANEL, IDC_GBROM_REL, IDC_GBROM_REL_EDIT, IDC_GBROM_BROWSE_REL, IDC_GBROM_ABS,
                                IDC_GBROM_ABS_EDIT, IDC_GBROM_BROWSE_ABS, IDC_GBSAVEPANEL, IDC_GBSAVE_SAME, IDC_GBSAVE_REL,
                                IDC_GBSAVE_REL_EDIT, IDC_GBSAVE_BROWSE_REL, IDC_GBSAVE_ABS, IDC_GBSAVE_ABS_EDIT, IDC_GBSAVE_BROWSE_ABS };

            for( int i = 0; i < sizeof(aChilds)/sizeof(int); ++i )
                DestroyWindow( GetDlgItem( hDlg, aChilds[i] ));
        }
#pragma message( "unimplemented Features from the Folderdialog will be removed" )
#endif // #ifdef HIDEUNIMPLEMENTED

        GetDirectory( szApplication, DIRECTORY_GBROMS );
        GetDirectory( szBuffer, DIRECTORY_GBSAVES );
        j = lstrcmp( szApplication, szBuffer );

        GetDirectory( szApplication, DIRECTORY_APPLICATION );
        i = lstrlen( szApplication );

        // MemPak Directory
        lstrcpyn(szBuffer, g_aszDefFolders[DIRECTORY_MEMPAK], MAX_PATH);

        if( szBuffer[0] != 0 && ( szBuffer[1] == ':' || ( szBuffer[1] == '\\' &&  szBuffer[0] == '\\' )))
        {
            CheckDlgButton( hDlg, IDC_MEMPAK_ABS, BST_CHECKED );

            GetDirectory( szBuffer, DIRECTORY_MEMPAK );
            if( !_tcsncmp( szBuffer, szApplication, i ))
            {
                TCHAR *pcSub = &szBuffer[i];
                pcSlash = _tcsrchr( pcSub, _T('\\') );
                if( pcSlash && ( pcSlash[1] == '\0' )) *pcSlash = '\0';
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)pcSub );
            }
            else
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)STRING_DEF_MEMPAKFILE );
        }
        else
        {
            CheckDlgButton( hDlg, IDC_MEMPAK_REL, BST_CHECKED );

            if( szBuffer[0] != 0 )
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            else
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)STRING_DEF_MEMPAKFILE );
        }

        GetDirectory( szBuffer, DIRECTORY_MEMPAK );
        pcSlash = _tcsrchr( szBuffer, '\\' );
        if( pcSlash && ( pcSlash[1] == '\0' )) *pcSlash = '\0';
        SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_ABS_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );

        // GBRom Directory
        lstrcpyn(szBuffer, g_aszDefFolders[DIRECTORY_GBROMS], MAX_PATH);

        if( szBuffer[0] != 0 && ( szBuffer[1] == _T(':') || ( szBuffer[1] == _T('\\') &&  szBuffer[0] == _T('\\') )))
        {
            CheckDlgButton( hDlg, IDC_GBROM_ABS, BST_CHECKED );

            GetDirectory( szBuffer, DIRECTORY_GBROMS );
            if( !_tcsncmp( szBuffer, szApplication, i ))
            {
                TCHAR *pcSub = &szBuffer[i];
                pcSlash = _tcsrchr( pcSub, _T('\\') );
                if( pcSlash && ( pcSlash[1] == '\0' )) *pcSlash = '\0';
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            }
            else
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)STRING_DEF_GBROMFILE );
        }
        else
        {
            CheckDlgButton( hDlg, IDC_GBROM_REL, BST_CHECKED );

            if( szBuffer[0] != 0 )
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            else
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)STRING_DEF_GBROMFILE );
        }

        GetDirectory( szBuffer, DIRECTORY_GBROMS );
        pcSlash = _tcsrchr( szBuffer, _T('\\') );
        if( pcSlash && ( pcSlash[1] == _T('\0') )) *pcSlash = '\0';
        SendMessage( GetDlgItem( hDlg, IDC_GBROM_ABS_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );

        // GBSave Directory
        lstrcpyn(szBuffer, g_aszDefFolders[DIRECTORY_GBSAVES], MAX_PATH);

        if( !j )
        {
            CheckDlgButton( hDlg, IDC_GBSAVE_SAME, BST_CHECKED );
            FoldersDialogProc( hDlg, WM_COMMAND, (WPARAM)MAKELONG( IDC_GBSAVE_SAME, BN_CLICKED ), (LPARAM)GetDlgItem( hDlg, IDC_GBSAVE_SAME ));
            SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_GETTEXT, sizeof(szBuffer), (LPARAM)szBuffer );
            SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
        }
        else
        {
            if( szBuffer[0] != 0 && ( szBuffer[1] == _T(':') || ( szBuffer[1] == _T('\\') &&  szBuffer[0] == _T('\\') )))
            {
                CheckDlgButton( hDlg, IDC_GBSAVE_ABS, BST_CHECKED );

                GetDirectory( szBuffer, DIRECTORY_GBSAVES );
                if( !_tcsncmp( szBuffer, szApplication, i ))
                {
                    TCHAR *pcSub = &szBuffer[i];
                    pcSlash = _tcsrchr( pcSub, '\\' );
                    if( pcSlash && ( pcSlash[1] == '\0' )) *pcSlash = '\0';
                    SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
                }
                else
                    SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)STRING_DEF_GBROMSAVE );
            }
            else
            {
                CheckDlgButton( hDlg, IDC_GBSAVE_REL, BST_CHECKED );

                if( szBuffer[0] != 0 )
                    SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
                else
                    SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)STRING_DEF_GBROMSAVE );
            }
        }

        GetDirectory( szBuffer, DIRECTORY_GBSAVES );
        pcSlash = _tcsrchr( szBuffer, _T('\\') );
        if( pcSlash && ( pcSlash[1] == _T('\0') )) *pcSlash = '\0';
        SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_ABS_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );

        return FALSE; // don't give it focus

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDC_GBSAVE_ABS:
        case IDC_GBSAVE_REL:
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), TRUE );
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_ABS_EDIT ), TRUE );
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_BROWSE_REL ), TRUE );
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_BROWSE_ABS ), TRUE );
            return TRUE;

        case IDC_GBSAVE_SAME:
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_ABS_EDIT ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_BROWSE_REL ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_GBSAVE_BROWSE_ABS ), FALSE );
            return TRUE;

        case IDC_MEMPAK_BROWSE_ABS:
            szBuffer[0] = '\0';
            if( BrowseFolders( hDlg, NULL, szBuffer ))
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_ABS_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            return TRUE;
        case IDC_GBROM_BROWSE_ABS:
            szBuffer[0] = '\0';
            if( BrowseFolders( hDlg, NULL, szBuffer ))
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_ABS_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            return TRUE;
        case IDC_GBSAVE_BROWSE_ABS:
            szBuffer[0] = '\0';
            if( BrowseFolders( hDlg, NULL, szBuffer ))
                SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_ABS_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            return TRUE;
        case IDC_MEMPAK_BROWSE_REL:
            GetDirectory( szApplication, DIRECTORY_APPLICATION );
            lstrcpyn( szBuffer, szApplication, ARRAYSIZE(szBuffer) );
            if( BrowseFolders( hDlg, NULL, szBuffer ))
            {
                i = lstrlen( szApplication );
                lstrcpyn( szBuffer, &szBuffer[i], ARRAYSIZE(szBuffer) ); // HACK: The lstrcpyn function has an undefined behavior if source and destination buffers overlap.
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            }
            return TRUE;

        case IDC_GBROM_BROWSE_REL:
            GetDirectory( szApplication, DIRECTORY_APPLICATION );
            lstrcpyn( szBuffer, szApplication, ARRAYSIZE(szBuffer) );
            if( BrowseFolders( hDlg, NULL, szBuffer ))
            {
                i = lstrlen( szApplication );
                lstrcpyn( szBuffer, &szBuffer[i], ARRAYSIZE(szBuffer) ); // HACK: The lstrcpyn function has an undefined behavior if source and destination buffers overlap.
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            }
            return TRUE;

        case IDC_GBSAVE_BROWSE_REL:
            GetDirectory( szApplication, DIRECTORY_APPLICATION );
            lstrcpyn( szBuffer, szApplication, ARRAYSIZE(szBuffer) );
            if( BrowseFolders( hDlg, NULL, szBuffer ))
            {
                i = lstrlen( szApplication );
                lstrcpyn( szBuffer, &szBuffer[i], ARRAYSIZE(szBuffer) ); // HACK: The lstrcpyn function has an undefined behavior if source and destination buffers overlap.
                SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_SETTEXT, 0, (LPARAM)szBuffer );
            }
            return TRUE;

        case IDOK:
            if( IsDlgButtonChecked( hDlg, IDC_MEMPAK_ABS ) == BST_CHECKED )
            {
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_ABS_EDIT ), WM_GETTEXT, ARRAYSIZE(szApplication), (LPARAM)szApplication );
                GetFullPathName( szApplication, ARRAYSIZE(szBuffer), szBuffer, &pcSlash );
            }
            else
                SendMessage( GetDlgItem( hDlg, IDC_MEMPAK_REL_EDIT ), WM_GETTEXT, ARRAYSIZE(szBuffer), (LPARAM)szBuffer );
            pcSlash = _tcsrchr( szBuffer, '\\' );
            if( pcSlash && ( pcSlash[1] == '\0' ))
                *pcSlash = '\0';

            if( !lstrcmp( szBuffer, STRING_DEF_MEMPAKFILE ))
                g_aszDefFolders[DIRECTORY_MEMPAK][0] = 0;
            else
                lstrcpyn(g_aszDefFolders[DIRECTORY_MEMPAK], szBuffer, MAX_PATH);

            if( IsDlgButtonChecked( hDlg, IDC_GBROM_ABS ) == BST_CHECKED )
            {
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_ABS_EDIT ), WM_GETTEXT, ARRAYSIZE(szApplication), (LPARAM)szApplication );
                GetFullPathName( szApplication, ARRAYSIZE(szBuffer), szBuffer, &pcSlash );
            }
            else
                SendMessage( GetDlgItem( hDlg, IDC_GBROM_REL_EDIT ), WM_GETTEXT, ARRAYSIZE(szBuffer), (LPARAM)szBuffer );
            pcSlash = _tcsrchr( szBuffer, '\\' );
            if( pcSlash && ( pcSlash[1] == '\0' )) *pcSlash = '\0';

            if( !lstrcmp( szBuffer, STRING_DEF_GBROMFILE ))
                g_aszDefFolders[DIRECTORY_GBROMS][0] = 0;
            else
                lstrcpyn(g_aszDefFolders[DIRECTORY_GBROMS], szBuffer, MAX_PATH);

            if( !IsDlgButtonChecked( hDlg, IDC_GBSAVE_SAME ) == BST_CHECKED )
            {
                if( IsDlgButtonChecked( hDlg, IDC_GBSAVE_ABS ) == BST_CHECKED )
                {
                    SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_ABS_EDIT ), WM_GETTEXT, ARRAYSIZE(szApplication), (LPARAM)szApplication );
                    GetFullPathName( szApplication, ARRAYSIZE(szBuffer), szBuffer, &pcSlash );
                }
                else
                    SendMessage( GetDlgItem( hDlg, IDC_GBSAVE_REL_EDIT ), WM_GETTEXT, ARRAYSIZE(szBuffer), (LPARAM)szBuffer );
            }

            pcSlash = _tcsrchr( szBuffer, _T('\\') );
            if( pcSlash && ( pcSlash[1] == '\0' )) *pcSlash = '\0';

            if( !lstrcmp( szBuffer, STRING_DEF_GBROMSAVE ))
                g_aszDefFolders[DIRECTORY_GBSAVES][0] = 0;
            else
                lstrcpyn(g_aszDefFolders[DIRECTORY_GBSAVES], szBuffer, MAX_PATH);

            EndDialog( hDlg, TRUE );
            return TRUE;

        case IDCANCEL:
            EndDialog( hDlg, FALSE );
            return TRUE;

        default:
            return FALSE;   // unhandled WM_COMMAND
        }
    default:
        return FALSE; //false means the msg didn't got processed
    }
}

///////////////////////////////////////////////////////////////////////////////
// A wonderful n squared algorithm to store the key names in a string... what for???
// called by EnumObjects in MainDlgProcess to enumerate the keys on the keyboard...?
/* BOOL CALLBACK EnumGetKeyDesc( LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef )
{
    // REMOVED //
} // I am putting this function out of its misery before it causes more grief. --rabid */


// ButtonID is where we store an array of 3 DWORDs: ID of pushbutton, ID of text window, and an offset in the button-structure controls
// bIndex tells us what information to copy, but in a confusing way
// bButtonSet tells us whether we want the data from the Controls array or the Shortcuts array below
// This function is confusing, but not much I can do to fix it now. I'm sorry. --rabid
bool GetButtonID( LPDWORD ButtonID, const BYTE bIndex, const BYTE bButtonSet )
{
    // TODO: make these const, read from a resource or a define, or something... I don't know
    LPDWORD ButtonTable = NULL;
    int nEntries = 0;

                            //  ID of PushButton  ID of TextWindow    place in Button-structure
    const DWORD Controls[][3] = {   { IDC_DRIGHT        , IDT_DRIGHT        , PF_DPADR },       // Controls
                            { IDC_DLEFT         , IDT_DLEFT         , PF_DPADL },
                            { IDC_DDOWN         , IDT_DDOWN         , PF_DPADD },
                            { IDC_DUP           , IDT_DUP           , PF_DPADU },
                            { IDC_SBUTTON       , IDT_SBUTTON       , PF_START },
                            { IDC_ZTRIGGER      , IDT_ZTRIGGER      , PF_TRIGGERZ },
                            { IDC_BBUTTON       , IDT_BBUTTON       , PF_BBUTTON },
                            { IDC_ABUTTON       , IDT_ABUTTON       , PF_ABUTTON },
                            { IDC_CRIGHT        , IDT_CRIGHT        , PF_CBUTTONR },
                            { IDC_CLEFT         , IDT_CLEFT         , PF_CBUTTONL },
                            { IDC_CDOWN         , IDT_CDOWN         , PF_CBUTTOND },
                            { IDC_CUP           , IDT_CUP           , PF_CBUTTONU },
                            { IDC_RIGHTTRIGGER  , IDT_RIGHTTRIGGER  , PF_TRIGGERR },
                            { IDC_LEFTTRIGGER   , IDT_LEFTTRIGGER   , PF_TRIGGERL },
                            { IDC_ARIGHT        , IDT_ARIGHT        , PF_APADR },
                            { IDC_ALEFT         , IDT_ALEFT         , PF_APADL },
                            { IDC_ADOWN         , IDT_ADOWN         , PF_APADD },
                            { IDC_AUP           , IDT_AUP           , PF_APADU }    };

    const DWORD Shortcuts[][3]= {
                            { IDC_SETNOPAK_P1       , IDC_SETNOPAK_P1       , SC_NOPAK },
                            { IDC_SETMEMPAK_P1      , IDC_SETMEMPAK_P1      , SC_MEMPAK },
                            { IDC_SETRUMBLEPAK_P1   , IDC_SETRUMBLEPAK_P1   , SC_RUMBPAK },
                            { IDC_SETTRANSFERPAK_P1 , IDC_SETTRANSFERPAK_P1 , SC_TRANSPAK },
                            { IDC_SETADAPTOIDPAK_P1 , IDC_SETADAPTOIDPAK_P1 , SC_ADAPTPAK },
                            { IDC_SWMEMRUMBLE_P1    , IDC_SWMEMRUMBLE_P1    , SC_SWMEMRUMB },
                            { IDC_SWMEMADAPTOID_P1  , IDC_SWMEMADAPTOID_P1  , SC_SWMEMADAPT },

                            { IDC_SETNOPAK_P2       , IDC_SETNOPAK_P2       , SC_NOPAK + SC_TOTAL },
                            { IDC_SETMEMPAK_P2      , IDC_SETMEMPAK_P2      , SC_MEMPAK + SC_TOTAL },
                            { IDC_SETRUMBLEPAK_P2   , IDC_SETRUMBLEPAK_P2   , SC_RUMBPAK + SC_TOTAL },
                            { IDC_SETTRANSFERPAK_P2 , IDC_SETTRANSFERPAK_P2 , SC_TRANSPAK + SC_TOTAL },
                            { IDC_SETADAPTOIDPAK_P2 , IDC_SETADAPTOIDPAK_P2 , SC_ADAPTPAK + SC_TOTAL },
                            { IDC_SWMEMRUMBLE_P2    , IDC_SWMEMRUMBLE_P2    , SC_SWMEMRUMB + SC_TOTAL },
                            { IDC_SWMEMADAPTOID_P2  , IDC_SWMEMADAPTOID_P2  , SC_SWMEMADAPT + SC_TOTAL },

                            { IDC_SETNOPAK_P3       , IDC_SETNOPAK_P3       , SC_NOPAK + SC_TOTAL * 2 },
                            { IDC_SETMEMPAK_P3      , IDC_SETMEMPAK_P3      , SC_MEMPAK + SC_TOTAL * 2 },
                            { IDC_SETRUMBLEPAK_P3   , IDC_SETRUMBLEPAK_P3   , SC_RUMBPAK + SC_TOTAL * 2 },
                            { IDC_SETTRANSFERPAK_P3 , IDC_SETTRANSFERPAK_P3 , SC_TRANSPAK + SC_TOTAL * 2 },
                            { IDC_SETADAPTOIDPAK_P3 , IDC_SETADAPTOIDPAK_P3 , SC_ADAPTPAK + SC_TOTAL * 2 },
                            { IDC_SWMEMRUMBLE_P3    , IDC_SWMEMRUMBLE_P3    , SC_SWMEMRUMB + SC_TOTAL * 2 },
                            { IDC_SWMEMADAPTOID_P3  , IDC_SWMEMADAPTOID_P3  , SC_SWMEMADAPT + SC_TOTAL * 2 },

                            { IDC_SETNOPAK_P4       , IDC_SETNOPAK_P4       , SC_NOPAK + SC_TOTAL * 3 },
                            { IDC_SETMEMPAK_P4      , IDC_SETMEMPAK_P4      , SC_MEMPAK + SC_TOTAL * 3 },
                            { IDC_SETRUMBLEPAK_P4   , IDC_SETRUMBLEPAK_P4   , SC_RUMBPAK + SC_TOTAL * 3 },
                            { IDC_SETTRANSFERPAK_P4 , IDC_SETTRANSFERPAK_P4 , SC_TRANSPAK + SC_TOTAL * 3 },
                            { IDC_SETADAPTOIDPAK_P4 , IDC_SETADAPTOIDPAK_P4 , SC_ADAPTPAK + SC_TOTAL * 3 },
                            { IDC_SWMEMRUMBLE_P4    , IDC_SWMEMRUMBLE_P4    , SC_SWMEMRUMB + SC_TOTAL * 3 },
                            { IDC_SWMEMADAPTOID_P4  , IDC_SWMEMADAPTOID_P4  , SC_SWMEMADAPT + SC_TOTAL * 3 },

                            { IDC_LOCKMOUSE         , IDC_LOCKMOUSE         , (DWORD)(-1) }
                                };


    if( bButtonSet == BSET_CONTROLS )
    {
        ButtonTable = (LPDWORD)Controls; // HACK: what an ugly solution, using a cast like that
        nEntries = ARRAYSIZE( Controls );
    }

    if( bButtonSet == BSET_SHORTCUTS )
    {
        ButtonTable = (LPDWORD)Shortcuts;
        nEntries = ARRAYSIZE( Shortcuts );
    }

    bool bReturn = false;

    // TODO: fix this! just use 2 dimensional arrays for crying out loud!
    if( ButtonTable != NULL )
    {
        nEntries *= 3;
        for( int i = 0; i < nEntries; i+=3 )
        {
            if( ButtonTable[i+bIndex] == ButtonID[bIndex] )
            {
                ButtonID[0] = ButtonTable[i+0];
                ButtonID[1] = ButtonTable[i+1];
                ButtonID[2] = ButtonTable[i+2];

                bReturn = true;
                break;
            }
        }
    }

    return bReturn;
}

// a text-munging routine to spit out what PC control was assigned to a button
bool GetButtonText( const BUTTON& btnButton, LPTSTR Buffer )
{
    const UINT iDevice[] ={ IDS_BUTTON_UNASSIGNED,
                        IDS_C_GAMEPAD,
                        IDS_C_KEYBOARD,
                        IDS_C_MOUSE };
    const UINT iGamepad[] ={ IDS_C_XAXIS,
                        IDS_C_YAXIS,
                        IDS_C_ZAXIS,
                        IDS_C_XROT,
                        IDS_C_YROT,
                        IDS_C_ZROT,
                        IDS_C_SLIDER,
                        IDS_C_SLIDER,
                        IDS_C_POV,
                        IDS_C_POV,
                        IDS_C_POV,
                        IDS_C_POV,
                        IDS_C_BUTTON };
    const UINT iMouse[] = { IDS_C_XAXIS,
                        IDS_C_YAXIS,
                        IDS_C_WHEEL,
                        IDS_C_BUTTON };
    const LPTSTR AxeID[] = {    TEXT( " +" ),
                        TEXT( " -" ),
                        TEXT( " /\\" ),
                        TEXT( " >" ),
                        TEXT( " \\/" ),
                        TEXT( " <" ) };
    TCHAR a[16], b[16], buff[16]; // gotta allocate the space, stack is easier than P_malloc
    TCHAR Btn[6];
    LPTSTR Text[] = {a, b, NULL };
    bool bReturn = true;
    DIDEVICEOBJECTINSTANCE didoi;
    didoi.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

    switch ( btnButton.bBtnType )
    {
    case DT_JOYBUTTON:
        LoadString( g_hResourceDLL, iDevice[1], Text[0], 16 );
        LoadString( g_hResourceDLL, iGamepad[12], Text[1], 16 );
        Text[2] = Btn;
        wsprintf( Btn, TEXT( "%02u" ), btnButton.bOffset );
        break;
    case DT_JOYAXE:
        LoadString( g_hResourceDLL, iDevice[1], Text[0], 16 );
        LoadString( g_hResourceDLL, iGamepad[btnButton.bOffset], Text[1], 16 );
        Text[2] = AxeID[btnButton.bAxisID];
        break;
    case DT_JOYSLIDER:
        LoadString( g_hResourceDLL, iDevice[1], Text[0], 16 );
        LoadString( g_hResourceDLL, iGamepad[btnButton.bOffset], buff, 16 );
        wsprintf(Text[1], buff, btnButton.bOffset - 5 );
        Text[2] = AxeID[btnButton.bAxisID];
    case DT_JOYPOV:
        LoadString( g_hResourceDLL, iDevice[1], Text[0], 16 );
        LoadString( g_hResourceDLL, iGamepad[btnButton.bOffset], buff, 16 );
        wsprintf(Text[1], buff, btnButton.bOffset - 7 );
        Text[2] = AxeID[2 + btnButton.bAxisID];
        break;
    case DT_KEYBUTTON:
        LoadString( g_hResourceDLL, iDevice[2], Text[0], 16 );
        //TODO: this is great! can we do this for all of them?
        if (btnButton.parentDevice->didHandle->GetObjectInfo(&didoi, btnButton.bOffset, DIPH_BYOFFSET) == DI_OK)
            Text[1] = didoi.tszName;
        else
        LoadString( g_hResourceDLL, IDS_C_UNKNOWN, Text[1], 16 );
        Text[2] = TEXT( "" );
        break;
    case DT_MOUSEBUTTON:
        LoadString( g_hResourceDLL, iDevice[3], Text[0], 16 );
        LoadString( g_hResourceDLL, iMouse[3], Text[1], 16 );
        Text[2] = Btn;
        wsprintf( Btn, TEXT( "%02u" ), btnButton.bOffset );
        break;
    case DT_MOUSEAXE:
        LoadString( g_hResourceDLL, iDevice[3], Text[0], 16 );
        LoadString( g_hResourceDLL, iMouse[btnButton.bOffset], Text[1], 16 );
        Text[2] = AxeID[btnButton.bAxisID];
        break;
    case DT_UNASSIGNED:
    default:
        LoadString( g_hResourceDLL, iDevice[0], Text[0], 16 );
        Text[1] = Text[2] = TEXT( "" );
        bReturn = false;
    }

    wsprintf( Buffer, TEXT( "%s%s%s" ), Text[0], Text[1], Text[2] );
    return ( bReturn );
}

// called when scanning devices to assign N64 buttons
DWORD ScanKeyboard( LPDEVICE lpDevice, LPDWORD lpdwCounter, LPBUTTON pButton )
{
    HRESULT hr;
    BYTE cKeys[256];

    hr = lpDevice->didHandle->GetDeviceState( sizeof( cKeys ), (LPVOID)&cKeys );
    if ( FAILED(hr) )
    {
            lpDevice->didHandle->Acquire();
            return FALSE;
    }

    int iGotKey = FALSE;
    int i = 0;

    for( i = 0; i < ARRAYSIZE( cKeys ); ++i )
    {
        if (( cKeys[i] & 0x80 ) )
        {
            if( i == DIK_ESCAPE )
                iGotKey = SC_SCANESCAPE;
            else
            {
                iGotKey = SC_SCANSUCCEED;
                pButton->bBtnType = (BYTE)DT_KEYBUTTON;
                pButton->bAxisID = (BYTE)0;
                pButton->bOffset = (BYTE)i;
                pButton->parentDevice = lpDevice;
            }
        }
    }
    return iGotKey;
}

// called when scanning devices to assign N64 buttons
DWORD ScanMouse( LPDEVICE lpDevice, LPDWORD lpdwCounter, LPBUTTON pButton )
{
    static BYTE rgbInitButtons[8];
    static bool bFirstScan = true;
    DIMOUSESTATE2 dm_Current;
    HRESULT hr;

    if ( *lpdwCounter == 0 )
        bFirstScan = true;

    hr = lpDevice->didHandle->GetDeviceState( sizeof(DIMOUSESTATE2), &dm_Current );
    if ( FAILED(hr) )
    {
        lpDevice->didHandle->Acquire();
        return FALSE;
    }

    if ( bFirstScan )
    {
        CopyMemory( rgbInitButtons, dm_Current.rgbButtons, sizeof(rgbInitButtons));
        bFirstScan = false;
        lpDevice->didHandle->GetDeviceState( sizeof(DIMOUSESTATE2), &dm_Current );
    }

    int iGotKey = FALSE;
    int i = 0;
    BYTE bAxeDirection = 0;
    long Mouse[] ={     FIELD_OFFSET( DIMOUSESTATE2, lX ) / sizeof(long),
                        FIELD_OFFSET( DIMOUSESTATE2, lY ) / sizeof(long),
                        FIELD_OFFSET( DIMOUSESTATE2, lZ ) / sizeof(long) };

    long lValue;

    for( i = 0; i < ARRAYSIZE(Mouse); ++i )
    {
        lValue =  ((long*)&dm_Current)[Mouse[i]];

        if( lValue > MOUSE_THRESHOLD )
        {
            iGotKey = SC_SCANSUCCEED;
            bAxeDirection = AI_AXE_P;
        }
        if( lValue < -MOUSE_THRESHOLD )
        {
            iGotKey = SC_SCANSUCCEED;
            bAxeDirection = AI_AXE_N;
        }
        if( iGotKey )
            break;
    }

    if( iGotKey == SC_SCANSUCCEED )
    {
        pButton->bBtnType = (BYTE)DT_MOUSEAXE;
        pButton->bAxisID = (BYTE)bAxeDirection;
        pButton->bOffset = (BYTE)Mouse[i];
        pButton->parentDevice = lpDevice;
    }
    else
    {
        for( i = 0; i < ARRAYSIZE( dm_Current.rgbButtons ); ++i )
        {
            if(( dm_Current.rgbButtons[i] != rgbInitButtons[i] ) && ( dm_Current.rgbButtons[i] & 0x80 ))
            {
                iGotKey = SC_SCANSUCCEED;
                pButton->bBtnType = (BYTE)DT_MOUSEBUTTON;
                pButton->bAxisID = (BYTE)0;
                pButton->bOffset = (BYTE)i;
                pButton->parentDevice = lpDevice;
                break;
            }
        }
    }
    CopyMemory( rgbInitButtons, dm_Current.rgbButtons, sizeof(rgbInitButtons));

    return iGotKey;
}

// called when scanning devices to assign N64 buttons
// tries to read any possible button presses or axes from lpDirectInputDevice; called by ScanDevices
DWORD ScanGamePad ( LPDEVICE lpDevice, LPDWORD lpdwCounter, LPBUTTON pButton, int iDeviceNumber )
{
    static DIJOYSTATE dj_Initial[MAX_DEVICES];
    static bool bFirstScan = true;
    HRESULT hr;

    if ( *lpdwCounter == 0 )
        bFirstScan = true;

    hr = lpDevice->didHandle->GetDeviceState( sizeof(DIJOYSTATE), &lpDevice->stateAs.joyState );
    if ( FAILED(hr) )
    {
        hr = lpDevice->didHandle->Acquire();
        return FALSE;
    }

    if ( bFirstScan )
    {
        dj_Initial[iDeviceNumber] = lpDevice->stateAs.joyState;
        bFirstScan = false;
        return FALSE; // initial scan done; gotta wait until next time
    }

    long lAxePos = ZEROVALUE + ( RANGERELATIVE * CONFIGTHRESHOLD / 100 );
    long lAxeNeg = ZEROVALUE - ( RANGERELATIVE * CONFIGTHRESHOLD / 100 );
    long lValue;

    int iGotKey = FALSE;
    BYTE bAxeDirection = 0;

    int JoyPad[][2] ={  { FIELD_OFFSET( DIJOYSTATE, lX ) / sizeof(long) , DT_JOYAXE },
                        { FIELD_OFFSET( DIJOYSTATE, lY ) / sizeof(long) , DT_JOYAXE },
                        { FIELD_OFFSET( DIJOYSTATE, lZ ) / sizeof(long) , DT_JOYAXE },
                        { FIELD_OFFSET( DIJOYSTATE, lRx ) / sizeof(long)    , DT_JOYAXE },
                        { FIELD_OFFSET( DIJOYSTATE, lRy ) / sizeof(long)    , DT_JOYAXE },
                        { FIELD_OFFSET( DIJOYSTATE, lRz ) / sizeof(long)    , DT_JOYAXE },
                        { FIELD_OFFSET( DIJOYSTATE, rglSlider[0] ) / sizeof(long)   , DT_JOYSLIDER },
                        { FIELD_OFFSET( DIJOYSTATE, rglSlider[1] ) / sizeof(long)   , DT_JOYSLIDER },
                        { FIELD_OFFSET( DIJOYSTATE, rgdwPOV[0] ) / sizeof(long) , DT_JOYPOV },
                        { FIELD_OFFSET( DIJOYSTATE, rgdwPOV[1] ) / sizeof(long) , DT_JOYPOV },
                        { FIELD_OFFSET( DIJOYSTATE, rgdwPOV[2] ) / sizeof(long) , DT_JOYPOV },
                        { FIELD_OFFSET( DIJOYSTATE, rgdwPOV[3] ) / sizeof(long) , DT_JOYPOV }   };

    int i;

    for( i = 0; i < ARRAYSIZE( JoyPad ); ++i )
    {
        lValue =  ((long*)&lpDevice->stateAs.joyState)[JoyPad[i][0]];
        if( lValue != ((long*)&(dj_Initial[iDeviceNumber]))[JoyPad[i][0]] )
        {
            if(( JoyPad[i][1] == DT_JOYAXE ) || ( JoyPad[i][1] == DT_JOYSLIDER ))
            {
                if( lValue - ((long*)&(dj_Initial[iDeviceNumber]))[JoyPad[i][0]] > lAxePos )
                {
                    iGotKey = SC_SCANSUCCEED;
                    bAxeDirection = AI_AXE_P;
                    break;
                }
                else if( lValue - ((long*)&(dj_Initial[iDeviceNumber]))[JoyPad[i][0]] < lAxeNeg )
                {
                    iGotKey = SC_SCANSUCCEED;
                    bAxeDirection = AI_AXE_N;
                    break;
                }
            }
            if( JoyPad[i][1] == DT_JOYPOV && LOWORD( lValue ) != 0xFFFF )
            {
                iGotKey = SC_SCANSUCCEED;
                if (( lValue > 31500 ) || ( lValue < 4500 ))
                    bAxeDirection = AI_POV_UP;
                if (( lValue > 4500 ) && ( lValue < 13500 ))
                    bAxeDirection = AI_POV_RIGHT;
                if (( lValue > 13500 ) && ( lValue < 22500 ))
                    bAxeDirection = AI_POV_DOWN;
                if (( lValue > 22500 ) && ( lValue < 31500 ))
                    bAxeDirection = AI_POV_LEFT;
                break;
            }
        }
    }

    if( iGotKey == SC_SCANSUCCEED )
    {
        pButton->bBtnType = (BYTE)JoyPad[i][1];
        pButton->bAxisID = (BYTE)bAxeDirection;
        pButton->bOffset = (BYTE)JoyPad[i][0];
        pButton->parentDevice = lpDevice;
    }
    else
    {
        for( i = 0; i < ARRAYSIZE( lpDevice->stateAs.joyState.rgbButtons ); ++i )
        {
            if (( lpDevice->stateAs.joyState.rgbButtons[i] & 0x80 ))
            {
                iGotKey = SC_SCANSUCCEED;
                pButton->bBtnType = (BYTE)DT_JOYBUTTON;
                pButton->bAxisID = (BYTE)0;
                pButton->bOffset = (BYTE)i;
                pButton->parentDevice = lpDevice;
                break;
            }
        }
    }

    return iGotKey;
}

// called when we assign a control to an N64 button
// returns FALSE if no keyscans recorded, SC_SCANESCAPE if escape pressed on keyboard (to abort), or SC_SUCCEED for a good read
// *apDirectInputDevices is an array of only 2 devices; sys keyboard, sys mouse
DWORD ScanDevices( LPDWORD lpdwCounter, LPBUTTON pButton )
{
    // Do ALL our polling first-- this ensures consistency (i.e. every device always gets polled at same interval)
/*  if( g_sysKeyboard.didHandle )
    {
        if( FAILED(g_sysKeyboard.didHandle->Poll()))
            g_sysKeyboard.didHandle->Acquire();
    } */
    if( g_sysMouse.didHandle )
    {
        if( FAILED(g_sysMouse.didHandle->Poll()))
            g_sysMouse.didHandle->Acquire();
    }
    for (int i = 0; i < g_nDevices; i++)
        if (g_devList[i].didHandle)
            if (FAILED(g_devList[i].didHandle->Poll()))
                g_devList[i].didHandle->Acquire();

    // While we have devices to scan,
    // if it's a keyboard, ScanKeyboard; if it's a mouse, ScanMouse;
    // otherwise ScanGamePad.

    DWORD dwReturn = FALSE;

//  if( !dwReturn && g_sysKeyboard.didHandle )
//      dwReturn = ScanKeyboard( &g_sysKeyboard, lpdwCounter, pButton );

    if( !dwReturn && g_sysMouse.didHandle )
        dwReturn = ScanMouse( &g_sysMouse, lpdwCounter, pButton );

    // ScanGamePad on ALL remaining devices
    for (int i = 0; !dwReturn && i < g_nDevices; i++)
        if (g_devList[i].didHandle)
        {
            switch (LOBYTE(g_devList[i].dwDevType))
            {
            case DI8DEVTYPE_KEYBOARD:
                dwReturn = ScanKeyboard( &g_devList[i], lpdwCounter, pButton );
                break;
            case DI8DEVTYPE_MOUSE:
                dwReturn = ScanMouse( &g_devList[i], lpdwCounter, pButton );
                break;
            default:
                dwReturn = ScanGamePad( &g_devList[i], lpdwCounter, pButton, i );
            }
        }

    return dwReturn;
}

// sets controller defaults; called when user clicks on "Clear Controller" or "Default Config"
// also called before loading stuff from the INI or from profile files, in case something isn't defined
// DEFAULTS TO NOT PLUGGED.  Make sure you "plug" the first controller manually.
void SetControllerDefaults( LPCONTROLLER pcController )
{
    freePakData( pcController );
    freeModifiers( pcController );
    ZeroMemory( pcController, sizeof(CONTROLLER) );

    pcController->fRawData =            true;
    pcController->fRealN64Range =       true;
    pcController->bRapidFireEnabled =   false;
    pcController->bRapidFireRate =      3; // Set default rapid fire rate here
    pcController->bStickRange =         DEFAULT_STICKRANGE;
    pcController->bPadDeadZone =        DEFAULT_DEADZONE;
    pcController->bRumbleTyp =          DEFAULT_RUMBLETYP;
    pcController->bRumbleStrength =     DEFAULT_RUMBLESTRENGTH;
    pcController->wMouseSensitivityX =  DEFAULT_MOUSESENSIVITY;
    pcController->wMouseSensitivityY =  DEFAULT_MOUSESENSIVITY;
    pcController->PakType =             DEFAULT_PAKTYPE;
    pcController->bMouseMoveX =         DEFAULT_MOUSEMOVE;
    pcController->bMouseMoveY =         DEFAULT_MOUSEMOVE;
}

void DeleteControllerSettings( int indexController )
{
    if( !g_ivConfig )
        return;

    SetControllerDefaults( &g_ivConfig->Controllers[indexController] );
    g_ivConfig->FFDevices[indexController].bProductCounter =        0;
    g_ivConfig->FFDevices[indexController].szProductName[0] =       _T('\0');

    return;
}

// SetModifier activates certain "always on" Config modifiers after they've been inserted or loaded
void SetModifier( LPCONTROLLER pcController )
{
    for( int i = 0; i < pcController->nModifiers; i++ )
    {
        if( pcController->pModifiers[i].bModType == MDT_CONFIG && pcController->pModifiers[i].fStatus )
        {
            MODSPEC_CONFIG args;
            args.dwValue = pcController->pModifiers[i].dwSpecific;
            if( args.fChangeAnalogConfig )
            {
                BYTE bConfig = (BYTE)args.fAnalogStickMode;
                if( bConfig < PF_AXESETS )
                    pcController->bAxisSet = bConfig;
                else
                {
                    if( pcController->bAxisSet == PF_AXESETS-1 )
                        pcController->bAxisSet = 0;
                    else
                        ++pcController->bAxisSet;
                }
            }
            if( args.fChangeMouseXAxis )
                if (pcController->bMouseMoveX == MM_BUFF)
                    pcController->bMouseMoveX = MM_ABS;
                else if (pcController->bMouseMoveX == MM_ABS)
                    pcController->bMouseMoveX = MM_BUFF;
            if( args.fChangeMouseYAxis )
                if (pcController->bMouseMoveY == MM_BUFF)
                    pcController->bMouseMoveY = MM_ABS;
                else if (pcController->bMouseMoveY == MM_ABS)
                    pcController->bMouseMoveY = MM_BUFF;

            if( args.fChangeKeyboardXAxis )
                pcController->fKeyAbsoluteX = !pcController->fKeyAbsoluteX;
            if( args.fChangeKeyboardYAxis )
                pcController->fKeyAbsoluteY = !pcController->fKeyAbsoluteY;
        }
    }
}

// Copies over the existing g_pcControllers' idea of configuration over to the interface.
void GetCurrentConfiguration()
{
    EnterCriticalSection( &g_critical );

    g_ivConfig->Language = g_strEmuInfo.Language;
    g_ivConfig->fDisplayShortPop = g_strEmuInfo.fDisplayShortPop;

    LPCONTROLLER pcController;
    for( int i = 0; i < 4; i++ )
    {
        pcController = &g_ivConfig->Controllers[i];
        CopyMemory( pcController, &g_pcControllers[i], sizeof(CONTROLLER));

        if( pcController->nModifiers > 0 )
        {
            pcController->pModifiers = (LPMODIFIER)P_malloc( sizeof(MODIFIER) * pcController->nModifiers );
            CopyMemory( pcController->pModifiers, g_pcControllers[i].pModifiers, sizeof(MODIFIER) * pcController->nModifiers );
            SetModifier( pcController );
        }
        else
            pcController->pModifiers = NULL;

        pcController->pPakData = NULL;

        for( int iDevice = 0; iDevice < ARRAYSIZE(g_devList) && g_devList[iDevice].dwDevType; ++iDevice )
        {
            if( g_devList[iDevice].guidInstance == pcController->guidFFDevice )
            {
                g_ivConfig->FFDevices[i].bProductCounter = g_devList[iDevice].bProductCounter;
                lstrcpyn( g_ivConfig->FFDevices[i].szProductName, g_devList[iDevice].szProductName, MAX_PATH );
                break;
            }
        }
    }

    LeaveCriticalSection( &g_critical );
    return;
}

// Copies over the interface's idea of configuration over to g_pcControllers.
void UpdateControllerStructures()
{
    // bDebug = g_ivConfig->bAutoConfig;
    EnterCriticalSection( &g_critical );

    g_iFirstController = -1;

#ifdef _UNICODE
    // might as well use this time to copy over language info and open the new DLL as well... --rabid
    g_strEmuInfo.Language = g_ivConfig->Language;
    if (g_hResourceDLL != g_strEmuInfo.hinst)
        FreeLibrary(g_hResourceDLL);
    g_hResourceDLL = LoadLanguageDLL(g_strEmuInfo.Language);
    if( g_hResourceDLL == NULL )
    {
        g_strEmuInfo.Language = 0;
        g_hResourceDLL = g_strEmuInfo.hinst;
        DebugWriteA("couldn't load language DLL, falling back to defaults\n");
    }
#endif // #ifdef _UNICODE

    g_strEmuInfo.fDisplayShortPop = g_ivConfig->fDisplayShortPop;

    LPCONTROLLER pcController;
    for( int i = 3; i >= 0; i-- )
    {
        pcController = &g_ivConfig->Controllers[i];

        SaveControllerPak( i );
        CloseControllerPak( i );
//      freePakData( &g_pcControllers[i] ); // called already by CloseControllerPak
        freeModifiers( &g_pcControllers[i] );

        CopyMemory( &g_pcControllers[i], pcController, sizeof(CONTROLLER));
        g_pcControllers[i].pPakData = NULL;
        g_pcControllers[i].pModifiers = NULL;

        if( g_pcControllers[i].nModifiers > 0 )
        {
            g_pcControllers[i].pModifiers = (LPMODIFIER)P_malloc( sizeof(MODIFIER) * g_pcControllers[i].nModifiers );
            CopyMemory( g_pcControllers[i].pModifiers, pcController->pModifiers, sizeof(MODIFIER) * g_pcControllers[i].nModifiers );
            SetModifier( &g_pcControllers[i] );
        }


        int iDevice = FindDeviceinList( g_ivConfig->FFDevices[i].szProductName, g_ivConfig->FFDevices[i].bProductCounter, true );
        if( iDevice != -1 && g_devList[iDevice].bEffType )
        {
            g_pcControllers[i].guidFFDevice = g_devList[iDevice].guidInstance;
        }
        else
        {
            g_pcControllers[i].guidFFDevice = GUID_NULL;
        }

        g_pcControllers[i].fPakCRCError = false;
        g_pcControllers[i].fPakInitialized = false;

        if (g_pcControllers[i].fPlugged)
            g_iFirstController = i;
    }
    LeaveCriticalSection( &g_critical );
    return;
}

LRESULT CALLBACK BlockerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;

    switch (msg)
    {
        case WM_CREATE:
            return 0;
        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
//  return 0;
}

BOOL InitOverlay(void)
{
    // registers our overlay class

    WNDCLASS wc;

    // Fill in the window class structure with parameters
    // that describe the main window.

    wc.style = 0;
    wc.lpfnWndProc = BlockerProc;     // window procedure
    wc.cbClsExtra = 0;                // no extra class memory
    wc.cbWndExtra = 0;                // no extra window memory
    wc.hInstance = g_strEmuInfo.hinst;
    wc.hIcon = 0;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = _T("BlockerClass");  // name of window class

    // Register the window class.

    return RegisterClass(&wc);
}

HWND MakeOverlay()
{
    HWND hwnd;
    RECT size;

    GetWindowRect(g_hMainDialog, &size);

    // Create the main window.

    hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        _T("BlockerClass"),
        _T("Blocker"),
        WS_POPUP,
        size.left,              // horizontal position
        size.top,               // vertical position
        size.right - size.left, // width
        size.bottom - size.top, // height
        g_hMainDialog,          // owner window
        (HMENU) NULL,           // menu
        g_strEmuInfo.hinst,     // handle to application instance
        (LPVOID) NULL           // window-creation data
    );
    if (!hwnd)
        return NULL;

    // Show the window (necessary to block input)
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    return hwnd;
}
