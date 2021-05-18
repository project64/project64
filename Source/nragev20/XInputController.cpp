/*
XInput Controller support for N-Rage`s Dinput8 Plugin by tecnicors
(C) 2009  Daniel Rehren - XInput Controller support

Author's Email: rehren_007@hotmail.com

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

// Code from http://msdn.microsoft.com/en-us/library/ee417014(VS.85).aspx
#include <stdio.h>
#include <wchar.h>

#include <windows.h>
#include <wbemidl.h>
#include <tchar.h>

#include "XInputController.h"
#include "FileAccess.h"
#include "resource.h"

// We need to keep track of XInput control ID's
int iXinputControlId = 0;

BOOL IsXInputDevice( const GUID* pGuidProductFromDirectInput )
{
    IWbemLocator*           pIWbemLocator  = NULL;
    IEnumWbemClassObject*   pEnumDevices   = NULL;
    IWbemClassObject*       pDevices[20]   = {0};
    IWbemServices*          pIWbemServices = NULL;
    BSTR                    bstrNamespace  = NULL;
    BSTR                    bstrDeviceID   = NULL;
    BSTR                    bstrClassName  = NULL;
    DWORD                   uReturned      = 0;
    bool                    bIsXinputDevice= false;
    UINT                    iDevice        = 0;
    VARIANT                 var;
    HRESULT                 hr;

    // CoInit if needed
    hr = CoInitialize(NULL);
    bool bCleanupCOM = SUCCEEDED(hr);

    // Create WMI
    hr = CoCreateInstance( __uuidof(WbemLocator),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(IWbemLocator),
                           (LPVOID*) &pIWbemLocator);
    if( FAILED(hr) || pIWbemLocator == NULL )
        goto LCleanup;

    bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );if( bstrNamespace == NULL ) goto LCleanup;
    bstrClassName = SysAllocString( L"Win32_PNPEntity" );   if( bstrClassName == NULL ) goto LCleanup;
    bstrDeviceID  = SysAllocString( L"DeviceID" );          if( bstrDeviceID == NULL )  goto LCleanup;

    // Connect to WMI
    hr = pIWbemLocator->ConnectServer( bstrNamespace, NULL, NULL, 0L,
                                       0L, NULL, NULL, &pIWbemServices );
    if( FAILED(hr) || pIWbemServices == NULL )
        goto LCleanup;

    // Switch security level to IMPERSONATE
    CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

    hr = pIWbemServices->CreateInstanceEnum( bstrClassName, 0, NULL, &pEnumDevices );
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;

    // Loop over all devices
    for( ;; )
    {
        // Get 20 at a time
        hr = pEnumDevices->Next( 10000, 20, pDevices, &uReturned );
        if( FAILED(hr) )
            goto LCleanup;
        if( uReturned == 0 )
            break;

        for( iDevice=0; iDevice<uReturned; iDevice++ )
        {
            // For each device, get its device ID
            hr = pDevices[iDevice]->Get( bstrDeviceID, 0L, &var, NULL, NULL );
            if( SUCCEEDED( hr ) )
            {
                if(var.vt == VT_BSTR && var.bstrVal != NULL)
                {
                    // Check if the device ID contains "IG_".  If it does, then it's an XInput device
                        // This information can not be found from DirectInput
                    if( wcsstr( var.bstrVal, L"IG_" ) )
                    {
                        // If it does, then get the VID/PID from var.bstrVal
                        DWORD dwPid = 0, dwVid = 0;
                        WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
                        if (strVid && wscanf(strVid, L"VID_%4X", &dwVid) != 1)
                            dwVid = 0;
                        WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
                        if (strPid && wscanf(strPid, L"PID_%4X", &dwPid) != 1)
                            dwPid = 0;

                        // Compare the VID/PID to the DInput device
                        DWORD dwVidPid = MAKELONG( dwVid, dwPid );
                        if( dwVidPid == pGuidProductFromDirectInput->Data1 )
                        {
                            bIsXinputDevice = true;
                            goto LCleanup;
                        }
                    }
                }
                VariantClear(&var);
            }
            SAFE_RELEASE( pDevices[iDevice] );
        }
    }

LCleanup:
    if(bstrNamespace)
        SysFreeString(bstrNamespace);
    if(bstrDeviceID)
        SysFreeString(bstrDeviceID);
    if(bstrClassName)
        SysFreeString(bstrClassName);
    for( iDevice=0; iDevice<20; iDevice++ )
        SAFE_RELEASE( pDevices[iDevice] );
    SAFE_RELEASE( pEnumDevices );
    SAFE_RELEASE( pIWbemLocator );
    SAFE_RELEASE( pIWbemServices );

    if( bCleanupCOM )
        CoUninitialize();

    return bIsXinputDevice;
}

void AxisDeadzone( SHORT &AxisValue, long  lDeadZoneValue, float fDeadZoneRelation )
{
    short sign = AxisValue < 0 ? -1 : 1;
    float value = (float)(AxisValue < 0 ? -AxisValue : AxisValue);

    if(value < lDeadZoneValue)
        value = 0;
    else
    {
        value = (value - lDeadZoneValue) * fDeadZoneRelation;
        value = value > 32767.0f ? 32767.0f : value;
    }

    AxisValue = (SHORT)(value * sign);
}

void GetXInputControllerKeys( const int indexController, LPDWORD Keys )
{
    if (fnXInputGetState == NULL)
    {
        return;
    }

    using namespace N64_BUTTONS;

    LPCONTROLLER pcController = &g_pcControllers[indexController];
    LPXCONTROLLER gController = &g_pcControllers[indexController].xiController;

    *Keys = 0;

    if ( !gController->bConfigured )
        return;

	ULONGLONG time = GetTickCount() / 1000;
	if (g_pcControllers[indexController].XcheckTime != NULL && (time - g_pcControllers[indexController].XcheckTime) < 3)
		return;

    DWORD result;
    XINPUT_STATE state;

    result = fnXInputGetState(gController->nControl, &state);

	if (result == ERROR_DEVICE_NOT_CONNECTED) {
		g_pcControllers[indexController].XcheckTime = time;
	}
	else {
		g_pcControllers[indexController].XcheckTime = NULL;
	}

    DWORD wButtons = state.Gamepad.wButtons;

    if( pcController->bPadDeadZone > 0 )
    {
        const int RANGERELATIVE = 32767;
        long lDeadZoneValue = pcController->bPadDeadZone * RANGERELATIVE / 100;
        float fDeadZoneRelation = (float)RANGERELATIVE  / (float)( RANGERELATIVE - lDeadZoneValue );

        AxisDeadzone(state.Gamepad.sThumbLX, lDeadZoneValue, fDeadZoneRelation);
        AxisDeadzone(state.Gamepad.sThumbLY, lDeadZoneValue, fDeadZoneRelation);
        AxisDeadzone(state.Gamepad.sThumbRX, lDeadZoneValue, fDeadZoneRelation);
        AxisDeadzone(state.Gamepad.sThumbRY, lDeadZoneValue, fDeadZoneRelation);
    }

    short LY = state.Gamepad.sThumbLY * N64_ANALOG_MAX / XC_ANALOG_MAX;
    short LX = state.Gamepad.sThumbLX * N64_ANALOG_MAX / XC_ANALOG_MAX;

    short RY = state.Gamepad.sThumbRY * N64_ANALOG_MAX / XC_ANALOG_MAX;
    short RX = state.Gamepad.sThumbRX * N64_ANALOG_MAX / XC_ANALOG_MAX;

    short XAx = 0, XAxc = 0;
    short YAx = 0, YAxc = 0;

    WORD valButtons = 0;
    valButtons |= ( wButtons & gController->stButtons.iDRight ) ? DRight : 0;
    valButtons |= ( wButtons & gController->stButtons.iDLeft ) ? DLeft : 0;
    valButtons |= ( wButtons & gController->stButtons.iDDown ) ? DDown : 0;
    valButtons |= ( wButtons & gController->stButtons.iDUp ) ? DUp : 0;
    valButtons |= ( wButtons & gController->stButtons.iStart ) ? Start : 0;
    valButtons |= ( wButtons & gController->stButtons.iZ ) ? Z : 0;
    valButtons |= ( wButtons & gController->stButtons.iB ) ? B : 0;
    valButtons |= ( wButtons & gController->stButtons.iA ) ? A : 0;
    valButtons |= ( wButtons & gController->stButtons.iCRight ) ? CRight : 0;
    valButtons |= ( wButtons & gController->stButtons.iCLeft ) ? CLeft : 0;
    valButtons |= ( wButtons & gController->stButtons.iCDown ) ? CDown : 0;
    valButtons |= ( wButtons & gController->stButtons.iCUp ) ? CUp : 0;
    valButtons |= ( wButtons & gController->stButtons.iR ) ? R : 0;
    valButtons |= ( wButtons & gController->stButtons.iL ) ? L : 0;

    valButtons |= state.Gamepad.bLeftTrigger > 30 ? gController->stAnalogs.iLeftTrigger : 0;
    valButtons |= state.Gamepad.bRightTrigger > 30 ? gController->stAnalogs.iRightTrigger : 0;

    if (LX >= BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iLXAxis & ((CRight | DRight) << 16) ? gController->stAnalogs.iLXAxis >> 16 : 0;
    if (LX <= -BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iLXAxis & (CLeft | DLeft) ? gController->stAnalogs.iLXAxis : 0;
    if (LY >= BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iLYAxis & ((CUp | DUp) << 16) ? gController->stAnalogs.iLYAxis >> 16 : 0;
    if (LY <= -BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iLYAxis & (CDown | DDown) ? gController->stAnalogs.iLYAxis : 0;

    if (RX >= BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iRXAxis & ((CRight | DRight) << 16) ? gController->stAnalogs.iRXAxis >> 16 : 0;
    if (RX <= -BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iRXAxis & (CLeft | DLeft) ? gController->stAnalogs.iRXAxis : 0;
    if (RY >= BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iRYAxis & ((CUp | DUp) << 16) ? gController->stAnalogs.iRYAxis >> 16 : 0;
    if (RY <= -BUTTON_ANALOG_VALUE)
        valButtons |= gController->stAnalogs.iRYAxis & (CDown | DDown) ? gController->stAnalogs.iRYAxis : 0;

    if (gController->stAnalogs.iLXAxis == XAxis)
    {
        XAx += LX;
        XAxc += LX > 0 ? 1 : 0;
    }
    if (gController->stAnalogs.iRXAxis == XAxis)
    {
        XAx += RX;
        XAxc += RX > 0 ? 1 : 0;
    }
    if( XAxc )
        XAx /= XAxc;

    if (gController->stAnalogs.iLYAxis == YAxis)
    {
        YAx += LY;
        YAxc += LY > 0 ? 1 : 0;
    }
    if (gController->stAnalogs.iRYAxis == YAxis)
    {
        YAx += RY;
        YAxc += RY > 0 ? 1 : 0;
    }
    if( YAxc )
        YAx /= YAxc;

    *Keys = MAKELONG(valButtons, MAKEWORD(XAx, YAx));
}

void DefaultXInputControllerKeys( LPXCONTROLLER gController)
{
    using namespace N64_BUTTONS;

    gController->stButtons.iA = XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_Y;
    gController->stButtons.iB = XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X;
    gController->stButtons.iStart = XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK;
    gController->stButtons.iDDown = XINPUT_GAMEPAD_DPAD_DOWN;
    gController->stButtons.iDLeft = XINPUT_GAMEPAD_DPAD_LEFT;
    gController->stButtons.iDRight = XINPUT_GAMEPAD_DPAD_RIGHT;
    gController->stButtons.iDUp = XINPUT_GAMEPAD_DPAD_UP;
    gController->stButtons.iL = XINPUT_GAMEPAD_LEFT_SHOULDER;
    gController->stButtons.iR = XINPUT_GAMEPAD_RIGHT_SHOULDER;
    gController->stAnalogs.iLeftTrigger = Z;
    gController->stAnalogs.iRightTrigger = R;
    gController->stAnalogs.iRXAxis = (CRight << 16) | CLeft;
    gController->stAnalogs.iRYAxis = (CUp << 16) | CDown;
    gController->stAnalogs.iLXAxis = XAxis;
    gController->stAnalogs.iLYAxis = YAxis;
    gController->bConfigured = true;
}

void VibrateXInputController( DWORD nController, int LeftMotorVal, int RightMotorVal )
{
    if (fnXInputSetState == NULL)
    {
        return;
    }

    XINPUT_VIBRATION vibration;

    ZeroMemory( &vibration, sizeof( XINPUT_VIBRATION ) );

    vibration.wLeftMotorSpeed = LeftMotorVal;
    vibration.wRightMotorSpeed = RightMotorVal;

    fnXInputSetState(nController, &vibration);
}

bool InitXinput()
{
    // Lets dynamically load in the XInput library
    if (g_hXInputDLL == NULL)
        g_hXInputDLL = LoadLibrary(L"Xinput1_4.dll");

    if (g_hXInputDLL == NULL)
    {
        // OK, since 1.4 is present, try 9.1.0 as its present on Vista and newer
        g_hXInputDLL = LoadLibrary(L"Xinput9_1_0.dll");
    }
    if (g_hXInputDLL == NULL)
    {
        return false;
    }

    // Prepare the functions we're going to use, nice and simple for XInput
    fnXInputSetState = (DWORD(WINAPI *) (DWORD, XINPUT_VIBRATION*))GetProcAddress(g_hXInputDLL, "XInputSetState");
    fnXInputGetState = (DWORD(WINAPI *) (DWORD, XINPUT_STATE*))GetProcAddress(g_hXInputDLL, "XInputGetState");
    return true;
}

void FreeXinput()
{
    // Unload the Library
    if (g_hXInputDLL != NULL)
    {
        FreeLibrary(g_hXInputDLL);
        g_hXInputDLL = NULL;
    }
}

bool InitiateXInputController( LPXCONTROLLER gController, int nControl )
{
    if (fnXInputGetState == NULL || fnXInputSetState == NULL)
    {
        return false;
    }

    gController->nControl = iXinputControlId;
    iXinputControlId++;

    TCHAR buffer[MAX_PATH];
    GetDirectory( buffer, DIRECTORY_CONFIG );
    _stprintf_s( buffer, _T("%sXInput Controller %d Config.xcc"), buffer, nControl + 1 );
    FILE *file = _tfopen( buffer, _T("rS") );
    if( file )
    {
        LoadXInputConfigFromFile( file, gController );
        fclose( file );
    }

    if( !gController->bConfigured )
        DefaultXInputControllerKeys( gController );

    return true;
}

TCHAR * GetN64ButtonNameFromButtonCode( int Button )
{
    using namespace N64_BUTTONS;

    TCHAR *btnName;
    btnName = new TCHAR[10];

    switch( Button )
    {
    case A:         _tcscpy_s( btnName, 10, _T( "A" ));         break;
    case B:         _tcscpy_s( btnName, 10, _T( "B" ));         break;
    case Z:         _tcscpy_s( btnName, 10, _T( "Z" ));         break;
    case L:         _tcscpy_s( btnName, 10, _T( "L" ));         break;
    case R:         _tcscpy_s( btnName, 10, _T( "R" ));         break;
    case Start:     _tcscpy_s( btnName, 10, _T( "Start" ));     break;
    case CUp:       _tcscpy_s( btnName, 10, _T( "C-Up" ));      break;
    case CDown:     _tcscpy_s( btnName, 10, _T( "C-Down" ));        break;
    case CRight:    _tcscpy_s( btnName, 10, _T( "C-Right" ));   break;
    case CLeft:     _tcscpy_s( btnName, 10, _T( "C-Left" ));        break;
    case DUp:       _tcscpy_s( btnName, 10, _T( "D-Up" ));      break;
    case DDown:     _tcscpy_s( btnName, 10, _T( "D-Down" ));        break;
    case DRight:    _tcscpy_s( btnName, 10, _T( "D-Right" ));   break;
    case DLeft:     _tcscpy_s( btnName, 10, _T( "D-Left" ));        break;
    default:        _tcscpy_s( btnName, 10, _T( "None" ));
    }
    return btnName;
}

TCHAR * GetN64ButtonFromXInputControl( LPXCONTROLLER gController, int XInputButton )
{
    using namespace N64_BUTTONS;

    if( !gController || !gController->bConfigured )
        return GetN64ButtonNameFromButtonCode( 0 );

    int N64ButtonCode = 0;

    N64ButtonCode |= gController->stButtons.iA & XInputButton ? A : 0;
    N64ButtonCode |= gController->stButtons.iB & XInputButton ? B : 0;
    N64ButtonCode |= gController->stButtons.iCDown & XInputButton ? CDown : 0;
    N64ButtonCode |= gController->stButtons.iCLeft & XInputButton ? CLeft : 0;
    N64ButtonCode |= gController->stButtons.iCRight & XInputButton ? CRight : 0;
    N64ButtonCode |= gController->stButtons.iCUp & XInputButton ? CUp : 0;
    N64ButtonCode |= gController->stButtons.iDDown & XInputButton ? DDown : 0;
    N64ButtonCode |= gController->stButtons.iDLeft & XInputButton ? DLeft : 0;
    N64ButtonCode |= gController->stButtons.iDRight & XInputButton ? DRight : 0;
    N64ButtonCode |= gController->stButtons.iDUp & XInputButton ? DUp : 0;
    N64ButtonCode |= gController->stButtons.iL & XInputButton ? L : 0;
    N64ButtonCode |= gController->stButtons.iR & XInputButton ? R : 0;
    N64ButtonCode |= gController->stButtons.iStart & XInputButton ? Start : 0;
    N64ButtonCode |= gController->stButtons.iZ & XInputButton ? Z : 0;

    return GetN64ButtonNameFromButtonCode( N64ButtonCode );
}

TCHAR * GetN64ButtonArrayFromXAnalog( LPXCONTROLLER gController, int XThStickOrXDpad )
{
    using namespace N64_BUTTONS;

    if( !gController || !gController->bConfigured )
        return NULL;

    TCHAR *name;
    name = new TCHAR[15];

    switch( XThStickOrXDpad )
    {
    case XC_LTBS:
        if( gController->stAnalogs.iLXAxis == XAxis )
            _tcscpy_s( name, 15, _T( "Analog Stick" ));
        else if( gController->stAnalogs.iLXAxis & CLeft )
            _tcscpy_s( name, 15, _T( "C Buttons" ));
        else if( gController->stAnalogs.iLXAxis & DLeft )
            _tcscpy_s( name, 15, _T( "DPad" ));
        else
            _tcscpy_s( name, 15, _T( "None" ));
        break;

    case XC_RTBS:
        if( gController->stAnalogs.iRXAxis == XAxis )
            _tcscpy_s( name, 15, _T( "Analog Stick" ));
        else if( gController->stAnalogs.iRXAxis & CLeft )
            _tcscpy_s( name, 15, _T( "C Buttons" ));
        else if( gController->stAnalogs.iRXAxis & DLeft )
            _tcscpy_s( name, 15, _T( "DPad" ));
        else
            _tcscpy_s( name, 15, _T( "None" ));
        break;

    case XC_DPAD:
        if( gController->stButtons.iCDown == XINPUT_GAMEPAD_DPAD_DOWN )
            _tcscpy_s( name, 15, _T( "C Button" ));
        else if( gController->stButtons.iDDown == XINPUT_GAMEPAD_DPAD_DOWN )
            _tcscpy_s( name, 15, _T( "DPad" ));
        else
            _tcscpy_s( name, 15, _T( "None" ));
        break;

    default:
        _tcscpy_s( name, 15, _T( "None" ));
    }
    return name;
}

bool ReadXInputControllerKeys( HWND hDlg, LPXCONTROLLER gController )
{
    if( hDlg == NULL || gController == NULL || !gController->bConfigured)
        return false;

    SendDlgItemMessage( hDlg, IDC_XC_A, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_A ));
    SendDlgItemMessage( hDlg, IDC_XC_B, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_B ));
    SendDlgItemMessage( hDlg, IDC_XC_X, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_X ));
    SendDlgItemMessage( hDlg, IDC_XC_Y, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_Y ));
    SendDlgItemMessage( hDlg, IDC_XC_BACK, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_BACK ));
    SendDlgItemMessage( hDlg, IDC_XC_START, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_START ));
    SendDlgItemMessage( hDlg, IDC_XC_LB, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_LEFT_SHOULDER ));
    SendDlgItemMessage( hDlg, IDC_XC_RB, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_RIGHT_SHOULDER ));
    SendDlgItemMessage( hDlg, IDC_XC_LTSB, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_LEFT_THUMB ));
    SendDlgItemMessage( hDlg, IDC_XC_RTSB, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonFromXInputControl( gController, XINPUT_GAMEPAD_RIGHT_THUMB ));

    SendDlgItemMessage( hDlg, IDC_XC_LT, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonNameFromButtonCode( gController->stAnalogs.iLeftTrigger ));
    SendDlgItemMessage( hDlg, IDC_XC_RT, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonNameFromButtonCode( gController->stAnalogs.iRightTrigger ));

    SendDlgItemMessage( hDlg, IDC_XC_DPAD, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonArrayFromXAnalog( gController, XC_DPAD ));
    SendDlgItemMessage( hDlg, IDC_XC_LTS, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonArrayFromXAnalog( gController, XC_LTBS ));
    SendDlgItemMessage( hDlg, IDC_XC_RTS, CB_SELECTSTRING, -1, (LPARAM)GetN64ButtonArrayFromXAnalog( gController, XC_RTBS ));

    return true;
}

int GetComboBoxXInputKey( int ComboBox )
{
    switch( ComboBox )
    {
    case IDC_XC_A:      return XINPUT_GAMEPAD_A;
    case IDC_XC_B:      return XINPUT_GAMEPAD_B;
    case IDC_XC_X:      return XINPUT_GAMEPAD_X;
    case IDC_XC_Y:      return XINPUT_GAMEPAD_Y;
    case IDC_XC_BACK:   return XINPUT_GAMEPAD_BACK;
    case IDC_XC_START:  return XINPUT_GAMEPAD_START;
    case IDC_XC_LB:     return XINPUT_GAMEPAD_LEFT_SHOULDER;
    case IDC_XC_RB:     return XINPUT_GAMEPAD_RIGHT_SHOULDER;
    case IDC_XC_LT:     return -1;  // Triggers don't use these macros
    case IDC_XC_RT:     return -1;
    case IDC_XC_LTSB:   return XINPUT_GAMEPAD_LEFT_THUMB;
    case IDC_XC_RTSB:   return XINPUT_GAMEPAD_RIGHT_THUMB;
    case IDC_XC_DPAD:   return -2;  // To handle analogs and D-pad
    case IDC_XC_LTS:    return -2;
    case IDC_XC_RTS:    return -2;
    default: return 0;
    }
}

int GetN64ButtonCode( TCHAR *btnName )  //esta wea esta muy fea, hay que buscar una mejor manera definitivamente..
{										// ^ This translated means "This wea is very ugly, we must definitely find a better way"
										// I'm assuming there must be some perceived "better way to handle this, so maybe TODO: ?"
    using namespace N64_BUTTONS;

    int value = 0;

    if( !_tcscmp( btnName, _T( "A" )))
        value = A;
    else if( !_tcscmp( btnName, _T( "B" )))
        value = B;
    else if( !_tcscmp( btnName, _T( "R" )))
        value = R;
    else if( !_tcscmp( btnName, _T( "L" )))
        value = L;
    else if( !_tcscmp( btnName, _T( "Z" )))
        value = Z;
    else if( !_tcscmp( btnName, _T( "Start" )))
        value = Start;
    else if( !_tcscmp( btnName, _T( "C-Up" )))
        value = CUp;
    else if( !_tcscmp( btnName, _T( "C-Down" )))
        value = CDown;
    else if( !_tcscmp( btnName, _T( "C-Left" )))
        value = CLeft;
    else if( !_tcscmp( btnName, _T( "C-Right" )))
        value = CRight;
    else if( !_tcscmp( btnName, _T( "D-Up" )))
        value = DUp;
    else if( !_tcscmp( btnName, _T( "D-Down" )))
        value = DDown;
    else if( !_tcscmp( btnName, _T( "D-Left" )))
        value = DLeft;
    else if( !_tcscmp( btnName, _T( "D-Right" )))
        value = DRight;

    return value;
}

void ResetXInputControllerKeys( LPXCONTROLLER gController )
{
    gController->stButtons.iA = 0;
    gController->stButtons.iB = 0;
    gController->stButtons.iZ = 0;
    gController->stButtons.iL = 0;
    gController->stButtons.iR = 0;
    gController->stButtons.iStart = 0;
    gController->stButtons.iCUp = 0;
    gController->stButtons.iCLeft = 0;
    gController->stButtons.iCDown = 0;
    gController->stButtons.iCRight = 0;
    gController->stButtons.iDUp = 0;
    gController->stButtons.iDLeft = 0;
    gController->stButtons.iDDown = 0;
    gController->stButtons.iDRight = 0;

    gController->stAnalogs.iLeftTrigger = 0;
    gController->stAnalogs.iLXAxis = 0;
    gController->stAnalogs.iLYAxis = 0;
    gController->stAnalogs.iRightTrigger = 0;
    gController->stAnalogs.iRXAxis = 0;
    gController->stAnalogs.iRYAxis = 0;
}

void StoreAnalogConfig( LPXCONTROLLER gController, int ComboBox, int index )
{
    using namespace N64_BUTTONS;

    switch( index )
    {
    case 1: // D-pad
        switch( ComboBox )
        {
        case IDC_XC_DPAD:
            gController->stButtons.iDDown   |= XINPUT_GAMEPAD_DPAD_DOWN;
            gController->stButtons.iDUp     |= XINPUT_GAMEPAD_DPAD_UP;
            gController->stButtons.iDLeft   |= XINPUT_GAMEPAD_DPAD_LEFT;
            gController->stButtons.iDRight  |= XINPUT_GAMEPAD_DPAD_RIGHT;
            break;
        case IDC_XC_LTS:
            gController->stAnalogs.iLXAxis = ( DRight << 16 ) | DLeft;
            gController->stAnalogs.iLYAxis = ( DUp << 16 ) | DDown;
            break;
        case IDC_XC_RTS:
            gController->stAnalogs.iRXAxis = ( DRight << 16 ) | DLeft;
            gController->stAnalogs.iRYAxis = ( DUp << 16 ) | DDown;
            break;
        }
        break;
    case 2: // C buttons
        switch( ComboBox )
        {
        case IDC_XC_DPAD:
            gController->stButtons.iCDown   |= XINPUT_GAMEPAD_DPAD_DOWN;
            gController->stButtons.iCUp     |= XINPUT_GAMEPAD_DPAD_UP;
            gController->stButtons.iCLeft   |= XINPUT_GAMEPAD_DPAD_LEFT;
            gController->stButtons.iCRight  |= XINPUT_GAMEPAD_DPAD_RIGHT;
            break;
        case IDC_XC_LTS:
            gController->stAnalogs.iLXAxis = ( CRight << 16 ) | CLeft;
            gController->stAnalogs.iLYAxis = ( CUp << 16 ) | CDown;
            break;
        case IDC_XC_RTS:
            gController->stAnalogs.iRXAxis = ( CRight << 16 ) | CLeft;
            gController->stAnalogs.iRYAxis = ( CUp << 16 ) | CDown;
            break;
        }
        break;
    case 3: // Analog
        switch( ComboBox )
        {
        case IDC_XC_LTS:
            gController->stAnalogs.iLXAxis = XAxis;
            gController->stAnalogs.iLYAxis = YAxis;
            break;
        case IDC_XC_RTS:
            gController->stAnalogs.iRXAxis = XAxis;
            gController->stAnalogs.iRYAxis = YAxis;
            break;
        }
        break;
    }
}

void StoreXInputControllerKeys( HWND hDlg, LPXCONTROLLER gController )
{
    LRESULT index = -1;
    DWORD value = 0;

    ResetXInputControllerKeys( gController );

    for( int i = IDC_XC_A; i <= IDC_XC_RTS; i++ )
    {
        index = SendDlgItemMessage( hDlg, i, CB_GETCURSEL, 0, 0 );
        value = GetComboBoxXInputKey( i );
        if( value == 0 )
            continue;
        else if ( value == -1 )
        {
            TCHAR btnName[10] = _T( "\0" );
            SendDlgItemMessage( hDlg, i, CB_GETLBTEXT, index, (LPARAM)(LPTSTR)btnName );
            switch( i )
            {
            case IDC_XC_LT:
                gController->stAnalogs.iLeftTrigger = GetN64ButtonCode( btnName );
                break;
            case IDC_XC_RT:
                gController->stAnalogs.iRightTrigger = GetN64ButtonCode( btnName );
                break;
            }
            continue;
        }
        else if ( value == -2 )
        {
            StoreAnalogConfig( gController, i, index);
            continue;
        }

        switch( index )
        {
        case 1:     gController->stButtons.iA       |= value;   break;
        case 2:     gController->stButtons.iB       |= value;   break;
        case 3:     gController->stButtons.iZ       |= value;   break;
        case 4:     gController->stButtons.iL       |= value;   break;
        case 5:     gController->stButtons.iR       |= value;   break;
        case 6:     gController->stButtons.iStart   |= value;   break;
        case 7:     gController->stButtons.iCUp     |= value;   break;
        case 8:     gController->stButtons.iCLeft   |= value;   break;
        case 9:     gController->stButtons.iCDown   |= value;   break;
        case 10:    gController->stButtons.iCRight  |= value;   break;
        case 11:    gController->stButtons.iDUp     |= value;   break;
        case 12:    gController->stButtons.iDLeft   |= value;   break;
        case 13:    gController->stButtons.iDDown   |= value;   break;
        case 14:    gController->stButtons.iDRight  |= value;   break;
        }
    }
    gController->bConfigured = true;
}

void SaveXInputConfigToFile( FILE *file, LPXCONTROLLER gController )
{
//  fprintf( file, "[XInput Controller %d]\n", gController->nControl );
    fprintf( file, "A=%lu\n", gController->stButtons.iA );
    fprintf( file, "B=%lu\n", gController->stButtons.iB );
    fprintf( file, "CDown=%lu\n", gController->stButtons.iCDown );
    fprintf( file, "CLeft=%lu\n", gController->stButtons.iCLeft );
    fprintf( file, "CRight=%lu\n", gController->stButtons.iCRight );
    fprintf( file, "CUp=%lu\n", gController->stButtons.iCUp );
    fprintf( file, "DDown=%lu\n", gController->stButtons.iDDown );
    fprintf( file, "DLeft=%lu\n", gController->stButtons.iDLeft );
    fprintf( file, "DRight=%lu\n", gController->stButtons.iDRight );
    fprintf( file, "DUp=%lu\n", gController->stButtons.iDUp );
    fprintf( file, "L=%lu\n", gController->stButtons.iL );
    fprintf( file, "R=%lu\n", gController->stButtons.iR );
    fprintf( file, "Start=%lu\n", gController->stButtons.iStart );
    fprintf( file, "Z=%lu\n", gController->stButtons.iZ );
    fprintf( file, "XAxis=%lu\n", gController->stButtons.iXAxis );
    fprintf( file, "YAxis=%lu\n", gController->stButtons.iYAxis );
    fprintf( file, "LeftTrigger=%lu\n", gController->stAnalogs.iLeftTrigger );
    fprintf( file, "RightTrigger=%lu\n", gController->stAnalogs.iRightTrigger );
    fprintf( file, "LeftXAxis=%lu\n", gController->stAnalogs.iLXAxis );
    fprintf( file, "LeftYAxis=%lu\n", gController->stAnalogs.iLYAxis );
    fprintf( file, "RightXAxis=%lu\n", gController->stAnalogs.iRXAxis );
    fprintf( file, "RightYAxis=%lu\n\n", gController->stAnalogs.iRYAxis );
}

void LoadXInputConfigFromFile( FILE *file, LPXCONTROLLER gController )
{
    char buffer[4096];
    int c = 0;

    while( fgets( buffer, 4096, file ))
    {
        if( strlen( buffer ) == 1 ) // Means end of controller config
            break;
        c++;
        switch( buffer[0] )
        {
        case 'A':
            sscanf(buffer, "A=%lu", &gController->stButtons.iA); break;
        case 'B':
            sscanf(buffer, "B=%lu", &gController->stButtons.iB); break;
        case 'C':
            switch( buffer[1] )
            {
            case 'U':
                sscanf(buffer, "CUp=%lu", &gController->stButtons.iCUp); break;
            case 'D':
                sscanf(buffer, "CDown=%lu", &gController->stButtons.iCDown); break;
            case 'L':
                sscanf(buffer, "CLeft=%lu", &gController->stButtons.iCLeft); break;
            case 'R':
                sscanf(buffer, "CRight=%lu", &gController->stButtons.iCRight); break;
            }
            break;
        case 'D':
            switch( buffer[1] )
            {
            case 'U':
                sscanf(buffer, "DUp=%lu", &gController->stButtons.iDUp); break;
            case 'D':
                sscanf(buffer, "DDown=%lu", &gController->stButtons.iDDown); break;
            case 'L':
                sscanf(buffer, "DLeft=%lu", &gController->stButtons.iDLeft); break;
            case 'R':
                sscanf(buffer, "DRight=%lu", &gController->stButtons.iDRight); break;
            }
            break;
        case 'L':
            switch( buffer[1] )
            {
            case '=':
                sscanf(buffer, "L=%lu", &gController->stButtons.iL); break;
            case 'e':
                switch( buffer[4] )
                {
                case 'T':
                    sscanf(buffer, "LeftTrigger=%lu", &gController->stAnalogs.iLeftTrigger); break;
                case 'X':
                    sscanf(buffer, "LeftXAxis=%lu", &gController->stAnalogs.iLXAxis); break;
                case 'Y':
                    sscanf(buffer, "LeftYAxis=%lu", &gController->stAnalogs.iLYAxis); break;
                }
                break;
            }
            break;
        case 'R':
            switch( buffer[1] )
            {
            case '=':
                sscanf(buffer, "R=%lu", &gController->stButtons.iR); break;
            case 'i':
                switch( buffer[5] )
                {
                case 'T':
                    sscanf(buffer, "RightTrigger=%lu", &gController->stAnalogs.iRightTrigger); break;
                case 'X':
                    sscanf(buffer, "RightXAxis=%lu", &gController->stAnalogs.iRXAxis); break;
                case 'Y':
                    sscanf(buffer, "RightYAxis=%lu", &gController->stAnalogs.iRYAxis); break;
                }
                break;
            }
            break;
        case 'Z':
            sscanf(buffer, "Z=%lu", &gController->stButtons.iZ); break;
        case 'S':
            sscanf(buffer, "Start=%lu", &gController->stButtons.iStart); break;
        case 'X':
            sscanf(buffer, "XAxis=%lu", &gController->stButtons.iXAxis); break;
        case 'Y':
            sscanf(buffer, "YAxis=%lu", &gController->stButtons.iYAxis); break;
        }
    }

    gController->bConfigured = c > 20 ;
}
