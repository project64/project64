/*
XInput Controller support for N-Rage`s Dinput8 Plugin
(C) 2002, 2006  Norbert Wladyka - N-Rage`s Dinput8 Plugin
(C) 2009  Daniel Rehren - XInput Controller support

N-Rage`s Dinput8 Plugin:
Author`s Email: norbert.wladyka@chello.at
Website: http://go.to/nrage

XInput Controller support:
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

#ifndef _XINPUTCONTROLLER_H
#define _XINPUTCONTROLLER_H

//#include <wmsstd.h> <-- only needed for SAFE_RELEASE(x)

// Fixes undefined FILE, etc. type errors in MSVC 2010 build -- cxd4
#include <stdio.h>

#ifndef SAFE_RELEASE        // When Windows Media Device M? is not present
#define SAFE_RELEASE(x) \
   if(x != NULL)        \
   {                    \
      x->Release();     \
      x = NULL;         \
   }
#endif


// Enum each PNP device using WMI and check each device ID to see if it contains
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput

BOOL IsXInputDevice( const GUID* pGuidProductFromDirectInput );
//END conde from ...

/*
XInput header for N-Rage Plugin, by tecnicors.

XInput Controller is going to take over the N64 control that matches its
number, ie. first XInput controller is first N64 player, etc.
*/

#include "commonIncludes.h"
#include <xinput.h>

// Defines
#define N64_ANALOG_MAX 88
#define XC_ANALOG_MAX 32767
#define BUTTON_ANALOG_VALUE 60

// Enums
namespace N64_BUTTONS
{
    // With this we can assign buttons to the XInput struct
    enum _N64_BUTTONS { A = 0x0080, B = 0x0040, Z = 0x0020, R = 0x1000, L = 0x2000, XAxis = 0x4000,
                        Start = 0x0010, DUp = 0x0008, DDown = 0x0004, DLeft = 0x0002, YAxis = 0x8000,
                        DRight = 0x0001, CUp = 0x0800, CDown = 0x0400, CLeft = 0x0200, CRight = 0x0100,
                        None = 0x0 };
}

//structures
typedef struct _XCONTROLLER     // XInput controller struct
{
    int nControl;
    bool bConfigured;

    struct _N64_BUTTONS         // For button configurations
    {
        int iA, iB;
        int iStart;
        int iL, iR, iZ;
        int iXAxis, iYAxis;
        int iDUp, iDDown, iDLeft, iDRight;
        int iCUp, iCDown, iCLeft, iCRight;
    }stButtons;

    struct _XINPUT_ANALOGS      // For analog configurations
    {
        int iLeftTrigger, iRightTrigger;
        unsigned int iRXAxis, iRYAxis, iLXAxis, iLYAxis;
    }stAnalogs;

}XCONTROLLER;

typedef XCONTROLLER *LPXCONTROLLER;

extern int iXinputControlId;

// Initiates XInput library
bool InitXinput();
// Free the XInput library
void FreeXinput();
// Sets the keys pressed for XInput controller gController, into keys
void GetXInputControllerKeys( const int indexController, LPDWORD Keys );
// Sets the default keys for XInput controller gController
void DefaultXInputControllerKeys( LPXCONTROLLER gController);
// Vibrates the XInput Control
void VibrateXInputController( DWORD nController, int LeftMotorVal = 65535, int RightMotorVal = 65535 );
// Initialize nControl XInput enabled controller
bool InitiateXInputController( LPXCONTROLLER gController, int nControl );

static DWORD(WINAPI * fnXInputGetState) (DWORD dwUserIndex, XINPUT_STATE* pState) = NULL;
static DWORD(WINAPI * fnXInputSetState) (DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) = NULL;

// XController dialog

#define XC_DPAD 1
#define XC_LTBS 2
#define XC_RTBS 3

// Reads current XInput controller key config, and shows it in the dialog
bool ReadXInputControllerKeys( HWND hDlg, LPXCONTROLLER gController );
// Stores dialog's button configuration into the XCONTROLLER struct
void StoreXInputControllerKeys( HWND hDlg, LPXCONTROLLER gController );
// Fills N64 button comobox with its buttons
inline void FillN64ButtonComboBox( HWND hDlg, int ComboBox )
{
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "None" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "A" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "B" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "Z" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "L" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "R" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "Start" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "C-Up" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "C-Left" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "C-Down" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "C-Right" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "D-Up" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "D-Left" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "D-Down" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "D-Right" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_SETCURSEL, 0, ( LPARAM )0 );
}

inline void FillN64AnalogComboBox( HWND hDlg, int ComboBox )
{
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "None" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "DPad" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "C Buttons" ));
    if( ComboBox != IDC_XC_DPAD )
        SendDlgItemMessage( hDlg, ComboBox, CB_ADDSTRING, 0, ( LPARAM )_T( "Analog Stick" ));
    SendDlgItemMessage( hDlg, ComboBox, CB_SETCURSEL, 0, ( LPARAM )0);
}

// Save/load keys from own config file

void SaveXInputConfigToFile( FILE *file, LPXCONTROLLER gController );
void LoadXInputConfigFromFile( FILE *file, LPXCONTROLLER gController );

#endif //_XINPUTCONTROLLER_H
