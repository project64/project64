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


#include <InitGuid.h>
#include "commonIncludes.h"
#include <dinput.h>
#include "NRagePluginV2.h"
#include "PakIO.h"
#include "DirectInput.h"
#include "XInputController.h"
#include <math.h>
#include <CGuid.h>

// ProtoTypes //
HRESULT AcquireDevice( LPDIRECTINPUTDEVICE8 lpDirectInputDevice );

// global Variables //
LPDIRECTINPUT8  g_pDIHandle = NULL;													// Base DirectInput8-Handle

//LPDIRECTINPUTDEVICE8 g_apInputDevice[6] = { NULL, NULL, NULL, NULL, NULL, NULL };	// array of Handles for devices
																					// 0:Keyboard, 1:Mouse, the rest are FF gamepads

// BYTE g_acKeystate[256];		// use g_sysKeyboard.stateAs.rgbButtons instead
// DIMOUSESTATE2 g_msMouseState = { 0, 0, 0 };											// Store our mouse state data between reads (because every time we read data, it resets the device data)
		// moved to g_sysMouse.stateAs...

// Update device data tables (so we only have to poll and read the devices once).  This is called by GetKeys and ReadController.
void GetDeviceDatas()
{
	HRESULT hr;

/*	if( g_sysKeyboard.didHandle )
	{
		hr = g_sysKeyboard.didHandle->Poll();
		if( FAILED( hr ))
			AcquireDevice( g_sysKeyboard.didHandle ); // we'll try again next time

		hr = g_sysKeyboard.didHandle->GetDeviceState( sizeof(g_sysKeyboard.stateAs.rgbButtons), &g_sysKeyboard.stateAs.rgbButtons );

		if( FAILED( hr ))
			ZeroMemory( g_sysKeyboard.stateAs.rgbButtons, sizeof(g_sysKeyboard.stateAs.rgbButtons) );
	} */

	if( g_sysMouse.didHandle )
	{
		hr = g_sysMouse.didHandle->Poll();

		if( FAILED( hr ))
			AcquireDevice( g_sysMouse.didHandle ); // we'll try again next time

		hr = g_sysMouse.didHandle->GetDeviceState( sizeof(DIMOUSESTATE2), &g_sysMouse.stateAs.mouseState );

		if( FAILED( hr ))
			ZeroMemory( &g_sysMouse.stateAs.mouseState, sizeof(DIMOUSESTATE2) );
	}

	// need to just poll every damn device we're using
	for( int i = 0; i < g_nDevices; i++ )
	{
		if( g_devList[i].didHandle )
		{
			if( FAILED( g_devList[i].didHandle->Poll() ))
				AcquireDevice( g_devList[i].didHandle ); // we'll try again next time
		
			switch (LOBYTE(g_devList[i].dwDevType))
			{
			case DI8DEVTYPE_KEYBOARD:
				hr = g_devList[i].didHandle->GetDeviceState( sizeof(g_devList[i].stateAs.rgbButtons), g_devList[i].stateAs.rgbButtons );
				break;
			case DI8DEVTYPE_MOUSE:
				hr = g_devList[i].didHandle->GetDeviceState( sizeof(g_devList[i].stateAs.mouseState), &g_devList[i].stateAs.mouseState );
				break;
			default:
				hr = g_devList[i].didHandle->GetDeviceState( sizeof(g_devList[i].stateAs.joyState), &g_devList[i].stateAs.joyState );
			}
		}
		else
			hr = DIERR_NOTACQUIRED;

		if( hr == DIERR_NOTACQUIRED ) // changed this because in the rare condition that we lose input between polling and GetDeviceState we don't want to reset our current controls --rabid
		{
			ZeroMemory( &g_devList[i].stateAs.joyState, sizeof(DEVICE::INPUTSTATE));
			if (g_devList[i].dwDevType != DI8DEVTYPE_KEYBOARD && g_devList[i].dwDevType != DI8DEVTYPE_MOUSE)
				FillMemory( g_devList[i].stateAs.joyState.rgdwPOV, sizeof(g_devList[i].stateAs.joyState.rgdwPOV), 0xFF ); // g_devList[i].stateAs.joyState.rgdwPOV = -1; // -1 is neutral
		}	
	}
}

// hacked up piece of shit, but it works
inline bool GetJoyPadPOV( PDWORD dwDegree, BYTE AxeId )
// TRUE if specified Direction is Pressed
{
	if( LOWORD( *dwDegree ) == 0xFFFF )
		return false;

	bool bPressed;

	switch( AxeId )
	{
	case AI_POV_DOWN:
		bPressed = (( *dwDegree >= 18000 - POVANGLETHRESH ) && (*dwDegree <= 18000 + POVANGLETHRESH ));
		break;
	case AI_POV_LEFT:
		bPressed = (( *dwDegree >= 27000 - POVANGLETHRESH ) && (*dwDegree <= 27000 + POVANGLETHRESH ));
		break;
	case AI_POV_RIGHT:
		bPressed = (( *dwDegree >= 9000 - POVANGLETHRESH ) && (*dwDegree <= 9000 + POVANGLETHRESH ));
		break;
	case AI_POV_UP:
		bPressed = (( *dwDegree >= 36000 - POVANGLETHRESH ) || ( *dwDegree <= 0 + POVANGLETHRESH ));
		break;
	default:
		bPressed = false;
	}
	
	return bPressed;
}

// Fill in button states and axis states for controller indexController, into the struct pdwData.
// pdwData is a pointer to a 4 byte BUTTONS union, if anyone cares
bool GetNControllerInput ( const int indexController, LPDWORD pdwData )
{
	*pdwData = 0;
	WORD w_Buttons = 0;
	// WORD w_Axes = 0;

	LPCONTROLLER pcController = &g_pcControllers[indexController]; // still needs to be here, but not as important --rabid

	bool b_Value;
	long l_Value = 0;

	long lAxisValueX = ZEROVALUE;
	long lAxisValueY = ZEROVALUE;

	// take this info from the N64 controller struct, regardless of input devices
	float d_ModifierX = (float)pcController->bStickRange / 100.0f;
	float d_ModifierY = (float)pcController->bStickRange / 100.0f;

	int i;

	// do N64-Buttons / modifiers
	for (i = 0; i < pcController->nModifiers; i++ )
	{
		BUTTON btnButton = pcController->pModifiers[i].btnButton;

		b_Value = IsBtnPressed( btnButton );

		bool fChangeMod = false;

		if( pcController->pModifiers[i].bModType == MDT_CONFIG )
		{ // Config-Type
			if( pcController->pModifiers[i].fToggle )
			{
				if( b_Value && !btnButton.fPrevPressed)
				{
					pcController->pModifiers[i].fStatus = !pcController->pModifiers[i].fStatus;
					fChangeMod = true;
				}
			}
			else
			{
				if(	b_Value != (bool)(btnButton.fPrevPressed))
					fChangeMod = true;
			}
		}
		else
		{ // Move / Macro Type
			if( pcController->pModifiers[i].fToggle )
			{
				if( b_Value && !btnButton.fPrevPressed )
					pcController->pModifiers[i].fStatus = !pcController->pModifiers[i].fStatus;
				fChangeMod = ( pcController->pModifiers[i].fStatus != 0 );
			}
			else
			{
				fChangeMod = b_Value;
			}
		}

		if( fChangeMod )
		{
			switch( pcController->pModifiers[i].bModType )
			{
			case MDT_MOVE:
			{
				LPMODSPEC_MOVE args = (LPMODSPEC_MOVE)&pcController->pModifiers[i].dwSpecific;
				d_ModifierX *= args->XModification / 100.0f;
				d_ModifierY *= args->YModification / 100.0f;	
			}
				break;
			case MDT_MACRO:
			{
				LPMODSPEC_MACRO args = (LPMODSPEC_MACRO)&pcController->pModifiers[i].dwSpecific;

				if (args->fRapidFire) // w00t! Rapid Fire here
				{
					if ((unsigned) b_Value != btnButton.fPrevPressed) // New macro pressed
					{
						args->fPrevFireState = 0;
						args->fPrevFireState2 = 0;
					}
					if(!args->fPrevFireState) // This round, a firing is needed
					{
						w_Buttons |= args->aButtons;
						if( args->fAnalogRight )
							lAxisValueX += MAXAXISVALUE;
						else if( args->fAnalogLeft )
							lAxisValueX -= MAXAXISVALUE;

						if( args->fAnalogDown )
							lAxisValueY -= MAXAXISVALUE;
						else if( args->fAnalogUp ) // up
								lAxisValueY += MAXAXISVALUE;
					}

					// Ok, update the firing counters here
					if (args->fRapidFireRate) // Do the rapid fire slowly
					{ // Note that this updates State2 before State... Makes a nice slower square-wave type pulse for the update
						args->fPrevFireState2 = (args->fPrevFireState2 + 1) & 1;
						if (!args->fPrevFireState2)
						{
							args->fPrevFireState = (args->fPrevFireState + 1) & 1;
							DebugWriteA("Slow Rapid Fire - Mark 2\n");
						}
					}
					else // Do a fast rapid fire
					{
						args->fPrevFireState = (args->fPrevFireState + 1) & 1;
						DebugWriteA("Fast Rapid Fire\n");
					}
				}
				else
				{
					w_Buttons |= args->aButtons; // Note this: It lets you push buttons as well as the macro buttons
					if( args->fAnalogRight )
						lAxisValueX += MAXAXISVALUE;
					else if( args->fAnalogLeft )
						lAxisValueX -= MAXAXISVALUE;

					if( args->fAnalogDown )
						lAxisValueY -= MAXAXISVALUE;
					else if( args->fAnalogUp ) // up
						lAxisValueY += MAXAXISVALUE;

					args->fPrevFireState = 0;
				}
			}
				break;
			case MDT_CONFIG:
			{
				LPMODSPEC_CONFIG args = (LPMODSPEC_CONFIG)&pcController->pModifiers[i].dwSpecific;

				if( args->fChangeAnalogConfig )
				{
					BYTE bConfig = (BYTE)args->fAnalogStickMode;
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
				if( args->fChangeMouseXAxis )
					if (pcController->bMouseMoveX == MM_BUFF)
						pcController->bMouseMoveX = MM_ABS;
					else if (pcController->bMouseMoveX == MM_ABS)
						pcController->bMouseMoveX = MM_BUFF;
				if( args->fChangeMouseYAxis )
					if (pcController->bMouseMoveY == MM_BUFF)
						pcController->bMouseMoveY = MM_ABS;
					else if (pcController->bMouseMoveY == MM_ABS)
						pcController->bMouseMoveY = MM_BUFF;

				if( args->fChangeKeyboardXAxis )
					pcController->fKeyAbsoluteX = !pcController->fKeyAbsoluteX;
				if( args->fChangeKeyboardYAxis )
					pcController->fKeyAbsoluteY = !pcController->fKeyAbsoluteY;
			}
				break;
			}
		}

		btnButton.fPrevPressed = b_Value;
		pcController->pModifiers[i].btnButton = btnButton;
	} // END N64 MODIFIERS for

	// do N64-Buttons / modifiers
	for( i = 0; i < PF_APADR; i++ )
	{
		BUTTON btnButton = pcController->aButton[i];

		b_Value = IsBtnPressed( btnButton );

		w_Buttons |= (((WORD)b_Value) << i);
	} // END N64 BUTTONS for

	long lDeadZoneValue = pcController->bPadDeadZone * RANGERELATIVE / 100;
	float fDeadZoneRelation	= (float)RANGERELATIVE  / (float)( RANGERELATIVE - lDeadZoneValue );

	// do N64 joystick axes
	for ( i = 0; i < 4; i++ )
	{
		//	0 : right
		//	1 : left
		//	2 : down
		//	3 : up

		bool fNegInput = (( i == 1 ) || ( i == 2 )); // Input has to be negated

		BUTTON btnButton = pcController->aButton[PF_APADR + pcController->bAxisSet * 4 + i];
		LPLONG plRawState = (LPLONG)&btnButton.parentDevice->stateAs.joyState;
		
		switch( btnButton.bBtnType )
		{
		case DT_JOYBUTTON:
			l_Value = MAXAXISVALUE;
			b_Value = ( btnButton.parentDevice->stateAs.joyState.rgbButtons[btnButton.bOffset] & 0x80 ) != 0;
			break;

		case DT_JOYSLIDER:
		case DT_JOYAXE:
			l_Value = plRawState[btnButton.bOffset] - ZEROVALUE;

			if( btnButton.bAxisID ) // negative Range
			{
				fNegInput = !fNegInput;

				b_Value = ( l_Value <= -lDeadZoneValue );
				if( b_Value )
					l_Value = (long) ((float)(l_Value + lDeadZoneValue ) * fDeadZoneRelation );
			}
			else
			{
				b_Value = ( l_Value >= lDeadZoneValue );
				if( b_Value )
					l_Value = (long) ((float)(l_Value - lDeadZoneValue ) * fDeadZoneRelation );
			}	
			break;

		case DT_JOYPOV:
			l_Value = MAXAXISVALUE;
			b_Value = GetJoyPadPOV( (PDWORD)&plRawState[btnButton.bOffset] , btnButton.bAxisID );
			break;

		case DT_KEYBUTTON:
			if( btnButton.parentDevice->stateAs.rgbButtons[btnButton.bOffset] & 0x80 )
			{
				b_Value = true;

				if(( pcController->fKeyAbsoluteX && i < 2 )
					|| ( pcController->fKeyAbsoluteY &&  i > 1 ))
				{
					if( pcController->wAxeBuffer[i] < MAXAXISVALUE )
					{
						l_Value = pcController->wAxeBuffer[i] = min(( pcController->wAxeBuffer[i] + N64DIVIDER*3), MAXAXISVALUE );
					}
					else
						l_Value = MAXAXISVALUE;
				}
				else
				{
					if( pcController->wAxeBuffer[i] < MAXAXISVALUE )
					{
						l_Value = pcController->wAxeBuffer[i] = min(( pcController->wAxeBuffer[i] * 2 + N64DIVIDER*5 ), MAXAXISVALUE );
					}
					else
						l_Value = MAXAXISVALUE;
				}
			}
			else
			{
				if(( pcController->fKeyAbsoluteX && i < 2 )
					|| ( pcController->fKeyAbsoluteY && i > 1 ))
				{
					l_Value = pcController->wAxeBuffer[i];
					b_Value = true;
				}
				else
				{
					if( pcController->wAxeBuffer[i] > N64DIVIDER )
					{
						b_Value = true;
						l_Value = pcController->wAxeBuffer[i] = pcController->wAxeBuffer[i] / 2 ;
					}
					else
						b_Value = false;
				}
			}
			break;

		case DT_MOUSEBUTTON:
			l_Value = MAXAXISVALUE;
			b_Value = ( btnButton.parentDevice->stateAs.mouseState.rgbButtons[btnButton.bOffset] & 0x80 ) != 0;
			break;

		case DT_MOUSEAXE:
			if( i < 2 )
				pcController->wAxeBuffer[i] += plRawState[btnButton.bOffset] * pcController->wMouseSensitivityX * MOUSESCALEVALUE;	// l_Value = btnButton.parentDevice->stateAs.mouseState[btnButton.bOffset];
			else
				pcController->wAxeBuffer[i] += plRawState[btnButton.bOffset] * pcController->wMouseSensitivityY * MOUSESCALEVALUE;	// l_Value = btnButton.parentDevice->stateAs.mouseState[btnButton.bOffset];

			l_Value = pcController->wAxeBuffer[i];

			// wAxeBuffer is positive for axes 0 and 3 if buffer remains, else zero
			// wAxeBuffer is negative for axes 1 and 2 if buffer remains, else zero

			if(( pcController->bMouseMoveX == MM_ABS && i < 2 ) || ( pcController->bMouseMoveY == MM_ABS &&  i > 1 ))
				pcController->wAxeBuffer[i] = min( max( MINAXISVALUE, pcController->wAxeBuffer[i]) , MAXAXISVALUE);
			else if (( pcController->bMouseMoveX == MM_BUFF && i < 2 ) || ( pcController->bMouseMoveY == MM_BUFF &&  i > 1 ))
				pcController->wAxeBuffer[i] = pcController->wAxeBuffer[i] * MOUSEBUFFERDECAY / 100;
			else // "deadpan" mouse
			{
				pcController->wAxeBuffer[i] = 0;
			}

			if( btnButton.bAxisID == AI_AXE_N) // the mouse axis has the '-' flag set
			{
				fNegInput = !fNegInput;

				b_Value = ( l_Value < ZEROVALUE );
			}
			else
			{
				b_Value = ( l_Value > ZEROVALUE );
			}

			break;
		
		case DT_UNASSIGNED:
		default:
			b_Value = false;
		}

		if ( b_Value )
		{
			if ( fNegInput )
				l_Value = -l_Value;
			
			if( i < 2 )
				lAxisValueX += l_Value;
			else
				lAxisValueY += l_Value;
		}
	}

	if( pcController->fKeyboard )
	{
		if( pcController->fKeyAbsoluteX )
		{
			if( pcController->wAxeBuffer[0] > pcController->wAxeBuffer[1] )
			{
				pcController->wAxeBuffer[0] -= pcController->wAxeBuffer[1];
				pcController->wAxeBuffer[1] = 0;
			}
			else
			{
				pcController->wAxeBuffer[1] -= pcController->wAxeBuffer[0];
				pcController->wAxeBuffer[0] = 0;
			}
		}
		if( pcController->fKeyAbsoluteY )
		{
			if( pcController->wAxeBuffer[2] > pcController->wAxeBuffer[3] )
			{
				pcController->wAxeBuffer[2] -= pcController->wAxeBuffer[3];
				pcController->wAxeBuffer[3] = 0;
			}
			else
			{
				pcController->wAxeBuffer[3] -= pcController->wAxeBuffer[2];
				pcController->wAxeBuffer[2] = 0;
			}
		}
	}


	if (pcController->bRapidFireEnabled)
	{
		if (pcController->bRapidFireCounter >= pcController->bRapidFireRate)
		{
			w_Buttons = (w_Buttons & 0xFF1F);
			pcController->bRapidFireCounter = 0;
		}
		else
		{
			pcController->bRapidFireCounter = pcController->bRapidFireCounter + 1;
		}
	}

	if( pcController->fRealN64Range && ( lAxisValueX || lAxisValueY ))
	{
		long lAbsoluteX = ( lAxisValueX > 0 ) ? lAxisValueX : -lAxisValueX;
		long lAbsoluteY = ( lAxisValueY > 0 ) ? lAxisValueY : -lAxisValueY;

		long lRangeX;
		long lRangeY;

		if(	lAbsoluteX > lAbsoluteY )
		{
			lRangeX = MAXAXISVALUE;
			lRangeY = lRangeX * lAbsoluteY / lAbsoluteX;
		}
		else
		{
			lRangeY = MAXAXISVALUE;
			lRangeX = lRangeY * lAbsoluteX / lAbsoluteY;
		}

		// TODO: optimize this --rabid
		double dRangeDiagonal = sqrt((double)(lRangeX * lRangeX + lRangeY * lRangeY));
//		__asm{
//			fld fRangeDiagonal
//			fsqrt
//			fstp fRangeDiagonal
//			fwait
//		}
		double dRel = MAXAXISVALUE / dRangeDiagonal;

		*pdwData = MAKELONG(w_Buttons,
							MAKEWORD(	(BYTE)(min( max( MINAXISVALUE, (long)(lAxisValueX * d_ModifierX * dRel )), MAXAXISVALUE) / N64DIVIDER ),
										(BYTE)(min( max( MINAXISVALUE, (long)(lAxisValueY * d_ModifierY * dRel )), MAXAXISVALUE) / N64DIVIDER )));
	}
	else
	{
		*pdwData = MAKELONG(w_Buttons,
							MAKEWORD(	(BYTE)(min( max( MINAXISVALUE, (long)(lAxisValueX * d_ModifierX )), MAXAXISVALUE) / N64DIVIDER ),
										(BYTE)(min( max( MINAXISVALUE, (long)(lAxisValueY * d_ModifierY )), MAXAXISVALUE) / N64DIVIDER )));
	}

	return true;
}

bool InitDirectInput( HWND hWnd )
{
	if( g_hDirectInputDLL == NULL )
		g_hDirectInputDLL = LoadLibrary( _T( "dinput8.dll" ));
	if( g_hDirectInputDLL == NULL )
	{
		ErrorMessage(IDS_ERR_DINOTFOUND, 0, false);
	}
	else if( !g_pDIHandle ) // is NULL if not yet initialized
	{
		HRESULT (WINAPI *lpGetDIHandle)( HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN ) = NULL;
		lpGetDIHandle = (HRESULT (WINAPI *)( HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN ))GetProcAddress( g_hDirectInputDLL, "DirectInput8Create" );

		if( lpGetDIHandle != NULL )
		{
			HRESULT hr;
			hr = lpGetDIHandle( g_strEmuInfo.hinst, DIRECTINPUT_VERSION, 
								IID_IDirectInput8, (LPVOID*)&g_pDIHandle, NULL );
			if( FAILED( hr ))
			{
				ErrorMessage(IDS_ERR_DICREATE, 0, false);
				g_pDIHandle = NULL;
				FreeLibrary( g_hDirectInputDLL );
				g_hDirectInputDLL = NULL;
			}
		}
	}

	return (g_pDIHandle != NULL);
}

// release a DirectInput device.  We don't need it anymore.
void ReleaseDevice( LPDIRECTINPUTDEVICE8 &lpDirectInputDevice )
{
	if( lpDirectInputDevice != NULL)
	{
		lpDirectInputDevice->Unacquire();
		lpDirectInputDevice->Release();
		lpDirectInputDevice = NULL;
	}
	return;
}

// release a DirectInput effect.  We don't need it anymore.
void ReleaseEffect( LPDIRECTINPUTEFFECT &lpDirectEffect )
{
	if( lpDirectEffect != NULL)
	{
		// should unload the effect on release, I hope
		lpDirectEffect->Release();
		lpDirectEffect = NULL;
	}
	return;
}

// release our DirectInput effects and devices, our DirectInput handle, and then unload dinput library
void FreeDirectInput ()
{
	int i;
	// release effects
	for( i = 0; i < ARRAYSIZE( g_apdiEffect ); ++i )
		ReleaseEffect( g_apdiEffect[i] );
	ZeroMemory( g_apdiEffect, sizeof(g_apdiEffect) );

	// release FF devices
	for( i = 0; i << ARRAYSIZE(g_apFFDevice); ++i )
		ReleaseDevice( g_apFFDevice[i] );
	ZeroMemory( g_apFFDevice, sizeof(g_apFFDevice) );

	// release normal devices
	for( i = 0; i < g_nDevices; i++ )
		ReleaseDevice( g_devList[i].didHandle );
	ZeroMemory( g_devList, sizeof(g_devList) );
	g_nDevices = 0;

	// release mouse device
	ReleaseDevice( g_sysMouse.didHandle );
	ZeroMemory( &g_sysMouse, sizeof(g_sysMouse) );

    // Release any DirectInput handles.
	if( g_pDIHandle != NULL )
	{
		g_pDIHandle->Release();
		g_pDIHandle = NULL;
	}
	// Unload the library.
	if( g_hDirectInputDLL != NULL )
	{
		FreeLibrary( g_hDirectInputDLL );
		g_hDirectInputDLL = NULL;
	}
	return;
}

// Acquire our device.  Our device might get unacquired for many many reasons, and we need to be able to reacquire it to get input again.
// We use this a LOT.
inline HRESULT AcquireDevice( LPDIRECTINPUTDEVICE8 lpDirectInputDevice )
{
	HRESULT hResult = lpDirectInputDevice->Acquire();
	while( hResult == DIERR_INPUTLOST )
		hResult = lpDirectInputDevice->Acquire();
	if( SUCCEEDED( hResult ))
		lpDirectInputDevice->Poll();
	return hResult;
}

// Called by the DirectInput enumerator for each FF device. What kind of force feedback effects does this device support?  We'll store it in pvRef.
BOOL CALLBACK EnumGetEffectTypes( LPCDIEFFECTINFO pdei, LPVOID pvRef )
{
	BYTE bFFType = *(LPBYTE)pvRef;
	bFFType |= ( pdei->dwEffType & DIEFT_CONSTANTFORCE )	? RUMBLE_CONSTANT : 0;
	bFFType |= ( pdei->dwEffType & DIEFT_RAMPFORCE )		? RUMBLE_RAMP : 0;
	bFFType |= ( pdei->dwEffType & DIEFT_CONDITION )		? RUMBLE_CONDITION : 0;
	bFFType |= ( pdei->dwEffType & DIEFT_PERIODIC )			? RUMBLE_PERIODIC : 0;
	bFFType |= ( pdei->dwEffType & DIEFT_CUSTOMFORCE )		? RUMBLE_CUSTOM : 0;
	*(WORD*)pvRef = bFFType;
	return DIENUM_CONTINUE;
}

// Called by the DirectInput enumerator for each attached DI device.  We use it to make a list of devices.
// EnumMakeDeviceList has been rewritten. --rabid
BOOL CALLBACK EnumMakeDeviceList( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef )
{
	switch (GET_DIDEVICE_TYPE(lpddi->dwDevType)) {
	// we don't need to do anything with these generic devices
	case DI8DEVTYPE_DEVICE:
		return DIENUM_CONTINUE;
		break;
	// these are potential xinput controllers, check them
	case DI8DEVTYPE_GAMEPAD:
	case DI8DEVTYPE_DRIVING:
	case DI8DEVTYPE_JOYSTICK:
	case DI8DEVTYPE_FLIGHT:
		if (IsXInputDevice(&lpddi->guidProduct))		// Check if is XInput device --tecnicors
			return DIENUM_CONTINUE;
		break;
	// for all other devices, continue on
	default:
		break;
	}

	if (IsEqualGUID(g_sysMouse.guidInstance, lpddi->guidInstance))
		return DIENUM_CONTINUE;

	for (int i = 0; i < g_nDevices; i++)
		if (IsEqualGUID(g_devList[i].guidInstance, lpddi->guidInstance))
			return ( g_nDevices < ARRAYSIZE(g_devList) ) ? DIENUM_CONTINUE : DIENUM_STOP;

	if (g_nDevices < ARRAYSIZE(g_devList)) // our buffer isn't full yet and the device doesn't already exist in our table
	{
		lstrcpyn( g_devList[g_nDevices].szProductName, lpddi->tszProductName, MAX_PATH );
		g_devList[g_nDevices].dwDevType = lpddi->dwDevType;
		g_devList[g_nDevices].guidInstance = lpddi->guidInstance;

		g_devList[g_nDevices].bProductCounter = 0; // counting similar devices
		for( int i = 0; i < g_nDevices; ++i )
		{
			if( !lstrcmp( lpddi->tszProductName, g_devList[i].szProductName ))
			{
				if( g_devList[g_nDevices].bProductCounter == 0 )
				{
					g_devList[g_nDevices].bProductCounter = 2;
					if( g_devList[i].bProductCounter == 0 )
						g_devList[i].bProductCounter = 1;
				}
				else
					g_devList[g_nDevices].bProductCounter++; // give em instance numbers
			}
		}
		if( !lstrcmp( lpddi->tszProductName, TEXT( STRING_ADAPTOID )))
			g_devList[g_nDevices].bEffType = RUMBLE_DIRECT;
		else
			g_devList[g_nDevices].bEffType = RUMBLE_NONE;

		if ( GetInputDevice(g_strEmuInfo.hMainWindow, g_devList[g_nDevices].didHandle, lpddi->guidInstance, lpddi->dwDevType, DIB_DEVICE) )
		{
			g_devList[g_nDevices].didHandle->EnumEffects( EnumGetEffectTypes, &g_devList[g_nDevices].bEffType, DIEFT_ALL );		
			g_nDevices++;
		}
		else
			ZeroMemory(&g_devList[g_nDevices], sizeof(DEVICE));
	}

	return ( g_nDevices < ARRAYSIZE(g_devList) ) ? DIENUM_CONTINUE : DIENUM_STOP;
}

// Called by an enumerator in GetInputDevice to determine if a device is attached at all.
BOOL CALLBACK EnumIsDeviceAvailable( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef )
{
	if( lpddi->guidInstance == *(GUID*)((LPVOID*)pvRef)[0] )
	{
		*(bool*)((LPVOID*)pvRef)[1] = true;
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

// Called by an axis enumerator in GetInputDevice.  Set the min and max range so we can pass the rec'd value right to the emulator.
BOOL CALLBACK EnumSetObjectsAxis( LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	LPDIRECTINPUTDEVICE8 lpDirectInputDevice = (LPDIRECTINPUTDEVICE8)pvRef;
	DIPROPRANGE diprg; 

	diprg.diph.dwSize       = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYID;
	diprg.diph.dwObj        = lpddoi->dwType;
	diprg.lMin              = MINAXISVALUE;
	diprg.lMax              = MAXAXISVALUE;

	lpDirectInputDevice->SetProperty(DIPROP_RANGE, &diprg.diph); // HACK: Usually works, but not all devices support setting range.

	return DIENUM_CONTINUE;
}

// **if passed an existing DirectInputDevice which matches gGuid
//		unacquires it, and sets its cooperative level (reinitialize)
// **if the existing device does not match the passed gGuid, the existing device is released
// **if no device was passed or gGuid did not match
//		searches for the controller matching gGuid in connected and available devices
//		creates a DirectInputDevice
//		sets its data format
//		sets its cooperative level
//		for joysticks, calls EnumSetObjectsAxis for each axis
// GetInputDevice always leaves the returned device in an UNACQUIRED state.
bool GetInputDevice( HWND hWnd, LPDIRECTINPUTDEVICE8 &lpDirectInputDevice, GUID gGuid, DWORD dwDevType, DWORD dwCooperativeLevel )
{
	DebugWriteA("GetInputDevice: gGuid is {%08.8lX-%04.4hX-%04.4hX-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}\n", gGuid.Data1, gGuid.Data2, gGuid.Data3, gGuid.Data4[0], gGuid.Data4[1], gGuid.Data4[2], gGuid.Data4[3], gGuid.Data4[4], gGuid.Data4[5], gGuid.Data4[6], gGuid.Data4[7]);
	if( lpDirectInputDevice != NULL)
	{
		DIDEVICEINSTANCE didDev;
		didDev.dwSize = sizeof(DIDEVICEINSTANCE);
		lpDirectInputDevice->GetDeviceInfo( &didDev );

		if( didDev.guidInstance == gGuid )
		{	// we've already gotten this device; unacquire it and initialize
			DebugWriteA("GetInputDevice: already created, attempting to reinit\n");
			lpDirectInputDevice->Unacquire();
			lpDirectInputDevice->SetCooperativeLevel( hWnd, dwCooperativeLevel );
			return true;
		}
		else
			ReleaseDevice( lpDirectInputDevice );
	}

	HRESULT hResult;
	
	LPCDIDATAFORMAT ppDiDataFormat = NULL;
	bool Success = false;

	switch( LOBYTE(dwDevType) )
	{
	case DI8DEVTYPE_KEYBOARD:
		ppDiDataFormat = &c_dfDIKeyboard;
		break;

	case DI8DEVTYPE_MOUSE:
		ppDiDataFormat = &c_dfDIMouse2;
		break;

	//case DI8DEVTYPE_GAMEPAD:
	//case DI8DEVTYPE_JOYSTICK:
	//case DI8DEVTYPE_DRIVING:
	//case DI8DEVTYPE_1STPERSON:
	//case DI8DEVTYPE_FLIGHT:
	default: // assume everything else is a gamepad; probably not the best idea but it works
		ppDiDataFormat = &c_dfDIJoystick;
		break;
	}

	bool bDeviceAvailable = false;
		
	VOID* aRef[2] = { &gGuid, &bDeviceAvailable };
		
	// for each available device in our dwDevType category, run EnumIsDeviceAvailable with params "aRef"
	g_pDIHandle->EnumDevices( DI8DEVCLASS_ALL, EnumIsDeviceAvailable, (LPVOID)aRef, DIEDFL_ATTACHEDONLY );
		
	if( !bDeviceAvailable )
	{
		DebugWriteA("GetInputDevice: Device does not appear available\n");
		return false;
	}
		
	hResult = g_pDIHandle->CreateDevice( gGuid, &lpDirectInputDevice, NULL );
	
	if( SUCCEEDED( hResult ))
	{
		hResult = lpDirectInputDevice->SetDataFormat( ppDiDataFormat );
		hResult = lpDirectInputDevice->SetCooperativeLevel( hWnd, dwCooperativeLevel );
		
		Success = SUCCEEDED( hResult );
		if (!Success)
		{
			DebugWriteA("GetInputDevice: SetCooperativeLevel failed\n");
		}
	}
	else
		DebugWriteA("GetInputDevice: CreateDevice failed\n");

	if( Success && ( ppDiDataFormat == &c_dfDIJoystick ))
		lpDirectInputDevice->EnumObjects( EnumSetObjectsAxis, lpDirectInputDevice, DIDFT_AXIS );

	return Success;
}

// How many force feedback axes (motors) does our device have?  We want to rumble them all.
BOOL CALLBACK EnumCountFFAxes( LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pnAxes )
{
	*(DWORD*)pnAxes += 1;
	return DIENUM_CONTINUE;
}

// Create a force feedback effect handle and downloads the effect.  Parameters are self-explanatory.
bool CreateEffectHandle( HWND hWnd, LPDIRECTINPUTDEVICE8 lpDirectInputDevice, LPDIRECTINPUTEFFECT &pDIEffect, BYTE bRumbleTyp, long lStrength )
{
	if( pDIEffect )
		ReleaseEffect( pDIEffect );

	if( !lpDirectInputDevice || bRumbleTyp == RUMBLE_DIRECT )
		return false;	

	DWORD nAxes = 0;
	DWORD rgdwAxes[] = { DIJOFS_X, DIJOFS_Y };

	HRESULT hResult;

	// count the FF - axes of the joystick
	lpDirectInputDevice->EnumObjects( EnumCountFFAxes, &nAxes, DIDFT_FFACTUATOR | DIDFT_AXIS );

	if( nAxes == 0 )
		return false;
	nAxes = min( nAxes, 2 );


	// Must be unaquired for setting stuff like Co-op Level
	hResult = lpDirectInputDevice->Unacquire();
	//FF Requires EXCLUSIVE LEVEL, took me hours to find the reason why it wasnt working
	hResult = lpDirectInputDevice->SetCooperativeLevel( hWnd, DIB_FF );

	// fail if we can't set coop level --rabid
	if (hResult != DI_OK)
	{
		DebugWriteA("CreateEffectHandle: couldn't set coop level: %08X\n", hResult);
		return false;
	}

    // Since we will be playing force feedback effects, we should disable the
    // auto-centering spring.
	DIPROPDWORD dipdw;
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = FALSE;
	
	hResult = lpDirectInputDevice->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph );

	long rglDirection[] = { 1, 1 };
	LPGUID EffectGuid;
    DIEFFECT eff;
    ZeroMemory( &eff, sizeof(eff) );

	eff.dwSize                  = sizeof(DIEFFECT);
	eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.dwGain                  = lStrength * 100;
    eff.dwTriggerButton         = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes                   = nAxes; //Number of Axes
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope              = NULL;
	eff.dwStartDelay            = 0;

	DICONSTANTFORCE cf;
	DIRAMPFORCE rf;
	DIPERIODIC pf;

	switch( bRumbleTyp )
	{
	case RUMBLE_CONSTANT:
		EffectGuid = (GUID*)&GUID_ConstantForce;

		eff.dwDuration              = 150000; // microseconds
		eff.dwSamplePeriod          = 0;
		eff.cbTypeSpecificParams    = sizeof(DICONSTANTFORCE);
		eff.lpvTypeSpecificParams   = &cf;

		cf.lMagnitude = 10000;
		break;
	case RUMBLE_RAMP:
		EffectGuid = (GUID*)&GUID_RampForce;
		
		eff.dwDuration              = 300000; // microseconds
		eff.dwSamplePeriod          = 0;
		eff.cbTypeSpecificParams    = sizeof(DIRAMPFORCE);
		eff.lpvTypeSpecificParams   = &rf;

		rf.lStart = 10000;
		rf.lEnd = 2000;
		break;
	
	case RUMBLE_CONDITION:
	case RUMBLE_PERIODIC:
		EffectGuid = (GUID*)&GUID_Sine;

		eff.dwDuration              = 150000; // microseconds
		eff.dwSamplePeriod          = 0;
		eff.cbTypeSpecificParams    = sizeof(DIPERIODIC);
		eff.lpvTypeSpecificParams   = &pf;

		pf.dwMagnitude = 10000;
		pf.lOffset = 0;
		pf.dwPhase = 0;
		pf.dwPeriod = 2000;
		break;

	case RUMBLE_NONE:
	case RUMBLE_CUSTOM:
	default:
		return false;
	}

	hResult = lpDirectInputDevice->CreateEffect( *EffectGuid, &eff, &pDIEffect, NULL );

	if (hResult == DI_OK)
	{
		hResult = lpDirectInputDevice->Acquire();
		hResult = pDIEffect->Download();
	}
	else
	{
		DebugWriteA("CreateEffectHandle: didn't CreateEffect: %08X\n", hResult);
	}
	return SUCCEEDED( hResult );
}

// Counts how many of each type of button assignment (keyboard, mouse, gamepad)
DWORD CountControllerStructDevs( CONTROLLER *pController )
{
	BYTE bMouse = 0, bKeyboard = 0, bGamePad = 0;
	BYTE bDType;

	bool fModifier = ( pController->pModifiers != NULL );
	int i = ( fModifier ) ? pController->nModifiers : ARRAYSIZE( pController->aButton );
	i--;

	for( ; i >= 0 || fModifier; --i )
	{
		if( i < 0 )
		{
			i = ARRAYSIZE( pController->aButton ) - 1;
			fModifier = false;
		}

		bDType = ( fModifier )	? pController->pModifiers[i].btnButton.bBtnType
								: pController->aButton[i].bBtnType;

		switch( bDType )
		{
		case DT_JOYBUTTON:
		case DT_JOYAXE:
		case DT_JOYSLIDER:
		case DT_JOYPOV:
			++bGamePad;
			break;

		case DT_KEYBUTTON:
			++bKeyboard;
			break;

		case DT_MOUSEBUTTON:
		case DT_MOUSEAXE:
			++bMouse;
			break;
		}
	}

	pController->fGamePad	= bGamePad != 0;
	pController->fKeyboard	= bKeyboard != 0;
	pController->fMouse		= bMouse != 0;

	return MAKELONG( MAKEWORD( bGamePad, bMouse ), MAKEWORD( bKeyboard, ( bMouse + bKeyboard + bGamePad )));
}


// PrepareInputDevices rewritten --rabid
bool PrepareInputDevices()
{
	bool fKeyboard = false;
	bool fMouse = false;
	bool fGamePad = false;

	for( int i = 0; i < ARRAYSIZE( g_pcControllers ); ++i )
	{
		fGamePad = false;
		if( g_pcControllers[i].fPlugged )
		{
			CountControllerStructDevs( &g_pcControllers[i] );

			fKeyboard = g_pcControllers[i].fKeyboard != 0;
			fMouse = g_pcControllers[i].fMouse != 0;
			fGamePad = ( g_pcControllers[i].fGamePad != 0); // we'll assume for now that there's a gamepad to go with those buttons
		}

		ReleaseEffect( g_apdiEffect[i] );
		if( g_pcControllers[i].guidFFDevice != GUID_NULL && GetInputDevice( g_strEmuInfo.hMainWindow, g_apFFDevice[i], g_pcControllers[i].guidFFDevice, DI8DEVTYPE_JOYSTICK, DIB_FF )) // not necessarily a joystick type device, but we don't use the data anyway
		{
			DIDEVICEINSTANCE diDev;
			diDev.dwSize = sizeof( DIDEVICEINSTANCE );

			g_apFFDevice[i]->GetDeviceInfo( &diDev );

			if( !lstrcmp( diDev.tszProductName, _T( STRING_ADAPTOID )))
			{
				g_pcControllers[i].fIsAdaptoid = true;
				DebugWriteA("FF device on controller %d is of type Adaptoid\n", i+1);
			}
			else
			{
				g_pcControllers[i].fIsAdaptoid = false;
			}

			if ( CreateEffectHandle( i, g_pcControllers[i].bRumbleTyp, g_pcControllers[i].bRumbleStrength ) )
			{
				AcquireDevice( g_apFFDevice[i] );
				DebugWriteA("Got FF device %d\n", i);
			}
			else
				DebugWriteA("Couldn't get FF device: CreateEffectHandle failed!\n");
		}
		else
		{
			g_apFFDevice[i] = NULL;
			DebugWriteA("Didn't get FF device %d\n", i);
		}
	}

	if( fMouse )
	{
		if( !g_sysMouse.didHandle )
		{
			if( GetInputDevice( g_strEmuInfo.hMainWindow, g_sysMouse.didHandle, GUID_SysMouse, DI8DEVTYPE_MOUSE, g_bExclusiveMouse ? DIB_MOUSE : DIB_KEYBOARD ))
			{
				AcquireDevice( g_sysMouse.didHandle );
			}
		}
	}
	else
	{
		g_bExclusiveMouse = false;
	}

	return true;
}

bool IsAdaptoidCommandSupported( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD cmd )
{
    DIEFFESCAPE esc;
    DWORD inbuf, outbuf;
    HRESULT hr;

	esc.dwSize = sizeof(esc);
	esc.dwCommand = ADAPT_TEST;   // command to determine if a command is supported
	esc.lpvInBuffer = &inbuf;
	esc.cbInBuffer = 4;
	esc.lpvOutBuffer = &outbuf;   
	esc.cbOutBuffer = 4;
	inbuf = cmd;                  // command that we are asking is supported
	outbuf = 0;                   

	hr = lpDirectInputDevice->Escape(&esc);

    return( SUCCEEDED(hr) && esc.cbOutBuffer == 4 && outbuf == 0xB0CAB0CA );
}

#ifdef _DEBUG
// Direct Adaptoid debugging stuff.
void _debugAd( LPCSTR szMessage, HRESULT res )
{
	LPCSTR suc = (SUCCEEDED(res)) ? "OK" : "FAILED";

	DebugWriteA( "%s: %s (RC:%08X)\n", szMessage, suc, res );
}
#endif // #ifdef _DEBUG

HRESULT DirectRumbleCommand( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD cmd )
{
    DIEFFESCAPE esc;

    esc.dwSize = sizeof(esc);
    esc.dwCommand = ADAPT_RUMBLE;   // send rumble command
    esc.lpvInBuffer = &cmd;  // 1=go, 0=stop
    esc.cbInBuffer = 4;
    esc.lpvOutBuffer = NULL;
    esc.cbOutBuffer = 0;

	HRESULT hr = lpDirectInputDevice->Escape(&esc);

#ifdef _DEBUG
	_debugAd( "Direct Adaptoid RumbleCommand", hr );
#endif // #ifdef _DEBUG

    return hr;
}

HRESULT InitializeAdaptoid( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, LPBYTE status )
{
    DIEFFESCAPE esc;

    esc.dwSize = sizeof(esc);
    esc.dwCommand = ADAPT_INIT;   // Initialize Pak
    esc.lpvInBuffer = NULL;
    esc.cbInBuffer = 0;
    esc.lpvOutBuffer = status;   
    esc.cbOutBuffer = 1;

	HRESULT hr = lpDirectInputDevice->Escape(&esc);

#ifdef _DEBUG
	_debugAd( "Direct Adaptoid InitPak", hr );
#endif // #ifdef _DEBUG

    return hr;
}

HRESULT ReadAdaptoidPak( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD addr, LPBYTE data )
{
    DIEFFESCAPE esc;

    esc.dwSize = sizeof(esc);
    esc.dwCommand = ADAPT_READPAK;   // Read 32 bytes from pak
    esc.lpvInBuffer = &addr;
    esc.cbInBuffer = 4;
    esc.lpvOutBuffer = data;   
    esc.cbOutBuffer = 32;

	HRESULT hr = lpDirectInputDevice->Escape(&esc);

#ifdef _DEBUG
	LPCSTR suc = (SUCCEEDED(hr)) ? "OK" : "FAILED";

	DebugWriteA( "Direct Adaptoid ReadPak(Addr:%04X): %s (RC:%08X)\n", addr, suc, hr );
#endif // #ifdef _DEBUG

    return hr;
}

HRESULT WriteAdaptoidPak( LPDIRECTINPUTDEVICE8 lpDirectInputDevice, DWORD addr, LPBYTE data )
{
    DIEFFESCAPE esc;
    struct
    {
        DWORD addr;
        BYTE data[32];
    } buf;

    buf.addr = addr;
    CopyMemory( buf.data, data, 32 );

    esc.dwSize = sizeof(esc);
    esc.dwCommand = ADAPT_WRITEPAK;   // Write 32 bytes to pak
    esc.lpvInBuffer = &buf;
    esc.cbInBuffer = 36;
    esc.lpvOutBuffer = NULL;   
    esc.cbOutBuffer = 0;
	
	HRESULT hr = lpDirectInputDevice->Escape(&esc);

#ifdef _DEBUG
	LPCSTR suc = (SUCCEEDED(hr)) ? "OK" : "FAILED";

	DebugWriteA( "Direct Adaptoid WritePak(Addr:%04X): %s (RC:%08X)\n", addr, suc, hr );
#endif // #ifdef _DEBUG

    return hr;
}

BYTE GetAdaptoidStatus( LPDIRECTINPUTDEVICE8 lpDirectInputDevice )
{
	HRESULT hr;
	BYTE bStatus = 0;
	hr = InitializeAdaptoid( lpDirectInputDevice, &bStatus );

	int iRetrys = 10;
	while( FAILED( hr ) && iRetrys > 0 )
	{
		Sleep( 5 );
		hr = AcquireDevice( lpDirectInputDevice );
		hr = InitializeAdaptoid( lpDirectInputDevice, &bStatus );
		iRetrys--;
	}

	return (SUCCEEDED(hr)) ? bStatus : RD_NOPLUGIN | RD_NOTINITIALIZED;
}

// Fill the handle for g_sysMouse properly
void InitMouse()
{
	if (GetInputDevice( g_strEmuInfo.hMainWindow, g_sysMouse.didHandle, GUID_SysMouse, DI8DEVTYPE_MOUSE, DIB_KEYBOARD ))
	{
		g_sysMouse.guidInstance = GUID_SysMouse;
		g_sysMouse.dwDevType = DI8DEVTYPE_MOUSE;
		_tcsncpy(g_sysMouse.szProductName, STRING_GUID_SYSMOUSE, ARRAYSIZE(g_sysMouse.szProductName));
	}
	else
		g_sysMouse.didHandle = NULL;
}

// treat btnButton as a b_Value, and return whether it is pressed or not
bool IsBtnPressed(BUTTON btnButton)
{
	long l_Value;
	LPLONG plRawState = (LPLONG)&btnButton.parentDevice->stateAs.joyState;

	switch ( btnButton.bBtnType )
	{
	case DT_JOYBUTTON:
		return ( btnButton.parentDevice->stateAs.joyState.rgbButtons[btnButton.bOffset] & 0x80 ) != 0;

	case DT_JOYSLIDER:
	case DT_JOYAXE:
		l_Value = plRawState[btnButton.bOffset] - ZEROVALUE;

		if ( btnButton.bAxisID )
			return ( l_Value <= -ABSTHRESHOLD );
		else
			return ( l_Value >= ABSTHRESHOLD );

	case DT_JOYPOV:
		return GetJoyPadPOV( (PDWORD)&plRawState[btnButton.bOffset] , btnButton.bAxisID );

	case DT_KEYBUTTON:
		return ( btnButton.parentDevice->stateAs.rgbButtons[btnButton.bOffset] & 0x80 ) != 0;

	case DT_MOUSEBUTTON:
		return ( btnButton.parentDevice->stateAs.mouseState.rgbButtons[btnButton.bOffset] & 0x80 ) != 0;

	case DT_MOUSEAXE:
		l_Value = MOUSEMOVE; // a.k.a. lvalue is button threshold

		if( btnButton.bAxisID )
			return ( ((LPLONG)(&btnButton.parentDevice->stateAs.mouseState))[btnButton.bOffset] < -l_Value );
		else
			return ( ((LPLONG)(&btnButton.parentDevice->stateAs.mouseState))[btnButton.bOffset] > l_Value );
		break;

	case DT_UNASSIGNED:
	default:
		return false;
	}
}
