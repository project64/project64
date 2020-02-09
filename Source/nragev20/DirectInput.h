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

#ifndef _DIRECTINPUT_H_
#define _DIRECTINPUT_H_

#include <dinput.h>

#include "NRagePluginV2.h"

extern LPDIRECTINPUT8 g_pDIHandle;


bool InitDirectInput( HWND hWnd );
void FreeDirectInput ();
bool PrepareInputDevices();
void InitMouse();
void GetDeviceDatas();
bool GetNControllerInput ( const int indexController, LPDWORD pdwData );

BOOL CALLBACK EnumMakeDeviceList( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef );

bool IsBtnPressed(BUTTON btnButton);
bool GetInputDevice( HWND hWnd, LPDIRECTINPUTDEVICE8 &lpDirectInputDevice, GUID gGuid, DWORD dwDevType, DWORD dwCooperativeLevel );
void ReleaseDevice( LPDIRECTINPUTDEVICE8 &lpDirectInputDevice );
bool CreateEffectHandle( HWND hWnd, LPDIRECTINPUTDEVICE8 lpDirectInputDevice, LPDIRECTINPUTEFFECT &pDIEffect, BYTE bRumbleTyp, long lStrength );
void ReleaseEffect( LPDIRECTINPUTEFFECT &lpDirectEffect );


BYTE GetAdaptoidStatus( LPDIRECTINPUTDEVICE8 lpDirectInputDevice );
bool IsAdaptoidCommandSupported( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD cmd );
HRESULT DirectRumbleCommand( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD cmd );
HRESULT InitializeAdaptoid( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, LPBYTE status );
HRESULT ReadAdaptoidPak( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD addr, LPBYTE data );
HRESULT WriteAdaptoidPak( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD addr, LPBYTE data );


#define MINAXISVALUE	-32767
#define MAXAXISVALUE	32767
#define ZEROVALUE		0
#define THRESHOLD		50
#define RANGERELATIVE	(MAXAXISVALUE - ZEROVALUE + 1)
#define ABSTHRESHOLD	(RANGERELATIVE * THRESHOLD / 100)
		// plus or minus this many 1/100 degrees counts as GetJoyPadPOV being pressed
#define POVANGLETHRESH	5675

#define MOUSEMOVE			5
		// by default, scale the mouse input by this much
#define MOUSESCALEVALUE		10
		// percent to decay mouse buffer every frame.
		// Tweak this from 0-100 to control how much the mouse drifts; values closer to 100 drift more,
		// while values closer to 0 are very stiff (deadpan) and don't turn well
#define MOUSEBUFFERDECAY	80

#define N64DIVIDER		258


#define DID_KEYBOARD	0
#define DID_MOUSE		1
#define DID_GAMEPAD		2

#define DIB_KEYBOARD	DISCL_NONEXCLUSIVE | DISCL_FOREGROUND
#define DIB_MOUSE		DISCL_EXCLUSIVE | DISCL_FOREGROUND
#define DIB_FF			DISCL_EXCLUSIVE | DISCL_BACKGROUND
#define DIB_DEVICE		DISCL_NONEXCLUSIVE | DISCL_BACKGROUND

#define DIB_CONFIG	DISCL_NONEXCLUSIVE | DISCL_BACKGROUND

#define RUMBLE_NONE			0x00
#define RUMBLE_CONSTANT		0x01
#define RUMBLE_RAMP			0x02
#define RUMBLE_CONDITION	0x04
#define RUMBLE_PERIODIC		0x08
#define RUMBLE_CUSTOM		0x10

#define RUMBLE_DIRECT		0x80

#define RUMBLE_EFF1			RUMBLE_CONSTANT
#define RUMBLE_EFF2			RUMBLE_RAMP
#define RUMBLE_EFF3			RUMBLE_DIRECT

		// Reported Name of the Adaptoid
#define STRING_ADAPTOID			"Adaptoid"
#define STRING_GUID_SYSKEYBOARD	_T("Keyboard")
#define STRING_GUID_SYSMOUSE	_T("SysMouse")

	// Query API - pass in command #, returns 0xB0CAB0CA if supported
#define ADAPT_TEST			0x7834BB00
	// Send command to rumble pack (DWORD 0=stop, 1=go)
#define ADAPT_RUMBLE		0x7834BB08
	// Initialize pak (returns pak status bit flags)
#define ADAPT_INIT			0x7834BB0C
	// Read from pak  (reads 32 bytes of data)
#define ADAPT_READPAK		0x7834BB0D
	// Write to pak   (writes 32 bytes of data)
#define ADAPT_WRITEPAK		0x7834BB0E
	// Send command directly to controller - synchronous
#define ADAPT_DIRECTCOMMAND	0x7834BB28

// The following inline functions are all overloads for existing functions
inline bool CreateEffectHandle( int iDevice, BYTE bRumbleTyp, long lStrength )
{
	return CreateEffectHandle( g_strEmuInfo.hMainWindow, g_apFFDevice[iDevice], g_apdiEffect[iDevice], bRumbleTyp, lStrength );
}

// this used to exist, but it was only used once and makes things more confusing. Removed. --rabid
//inline void ReleaseEffect( int iEffect )
//{
//	ReleaseEffect( g_apdiEffect[iEffect] );
//}

inline BYTE GetAdaptoidStatus( int iDevice )
{
	return GetAdaptoidStatus( g_apFFDevice[iDevice] );
}
inline HRESULT WriteAdaptoidPak( int iDevice, DWORD addr, LPBYTE data )
{
	return WriteAdaptoidPak( g_apFFDevice[iDevice], addr, data );
}
inline HRESULT ReadAdaptoidPak( int iDevice, DWORD addr, LPBYTE data )
{
	return ReadAdaptoidPak( g_apFFDevice[iDevice], addr, data );
}
inline HRESULT InitializeAdaptoid( int iDevice, LPBYTE status )
{
	return InitializeAdaptoid( g_apFFDevice[iDevice], status );
}
inline HRESULT DirectRumbleCommand( int iDevice, DWORD cmd )
{
	return DirectRumbleCommand( g_apFFDevice[iDevice], cmd );
}
inline bool IsAdaptoidCommandSupported( int iDevice, DWORD cmd )
{
	return IsAdaptoidCommandSupported( g_apFFDevice[iDevice], cmd );
}


#endif // #ifndef _DIRECTINPUT_H_
