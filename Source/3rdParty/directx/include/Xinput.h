/***************************************************************************
*                                                                          *
*   XInput.h -- This module defines XBOX controller APIs                   *
*               and constansts for the Windows platform.                   *
*                                                                          *
*   Copyright (c) Microsoft Corp. All rights reserved.                     *
*                                                                          *
***************************************************************************/
#ifndef _XINPUT_H_
#define _XINPUT_H_

#include <windef.h>

// Current name of the DLL shipped in the same SDK as this header.
// The name reflects the current version
#define XINPUT_DLL_A  "xinput9_1_0.dll"
#define XINPUT_DLL_W L"xinput9_1_0.dll"
#ifdef UNICODE
    #define XINPUT_DLL XINPUT_DLL_W
#else
    #define XINPUT_DLL XINPUT_DLL_A
#endif 

//
// Device types available in XINPUT_CAPABILITIES
//
#define XINPUT_DEVTYPE_GAMEPAD          0x01

//
// Device subtypes available in XINPUT_CAPABILITIES
//
#define XINPUT_DEVSUBTYPE_GAMEPAD       0x01

//
// Flags for XINPUT_CAPABILITIES
//
#define XINPUT_CAPS_VOICE_SUPPORTED     0x0004

//
// Constants for gamepad buttons
//
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

//
// Gamepad thresholds
//
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

//
// Flags to pass to XInputGetCapabilities
//
#define XINPUT_FLAG_GAMEPAD             0x00000001


//
// Structures used by XInput APIs
//
typedef struct _XINPUT_GAMEPAD
{
    WORD                                wButtons;
    BYTE                                bLeftTrigger;
    BYTE                                bRightTrigger;
    SHORT                               sThumbLX;
    SHORT                               sThumbLY;
    SHORT                               sThumbRX;
    SHORT                               sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE
{
    DWORD                               dwPacketNumber;
    XINPUT_GAMEPAD                      Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION
{
    WORD                                wLeftMotorSpeed;
    WORD                                wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

typedef struct _XINPUT_CAPABILITIES
{
    BYTE                                Type;
    BYTE                                SubType;
    WORD                                Flags;
    XINPUT_GAMEPAD                      Gamepad;
    XINPUT_VIBRATION                    Vibration;
} XINPUT_CAPABILITIES, *PXINPUT_CAPABILITIES;


//
// XInput APIs
//
#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI XInputGetState
(
    DWORD         dwUserIndex,  // [in] Index of the gamer associated with the device
    XINPUT_STATE* pState        // [out] Receives the current state
);

DWORD WINAPI XInputSetState
(
    DWORD             dwUserIndex,  // [in] Index of the gamer associated with the device
    XINPUT_VIBRATION* pVibration    // [in, out] The vibration information to send to the controller
);

DWORD WINAPI XInputGetCapabilities
(
    DWORD                dwUserIndex,   // [in] Index of the gamer associated with the device
    DWORD                dwFlags,       // [in] Input flags that identify the device type
    XINPUT_CAPABILITIES* pCapabilities  // [out] Receives the capabilities
);

DWORD WINAPI XInputGetDSoundAudioDeviceGuids
(
    DWORD dwUserIndex,          // [in] Index of the gamer associated with the device
    GUID* pDSoundRenderGuid,    // [out] DSound device ID for render
    GUID* pDSoundCaptureGuid    // [out] DSound device ID for capture
);

#ifdef __cplusplus
}
#endif

#endif  //_XINPUT_H_


