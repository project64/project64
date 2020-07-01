#pragma once
#include <Common\stdtypes.h>
#include <guiddef.h>

enum BtnType
{
    BTNTYPE_UNASSIGNED = 0,

    // Joystick
    BTNTYPE_JOYBUTTON = 1,
    BTNTYPE_JOYAXE = 2,
    BTNTYPE_JOYPOV = 3,
    BTNTYPE_JOYSLIDER = 4,

    // Keyboard
    BTNTYPE_KEYBUTTON = 5,

    // Mouse
    BTNTYPE_MOUSEBUTTON = 6,
    BTNTYPE_MOUSEAXE = 7,
};

typedef struct _BUTTON
{
    uint8_t Offset;
    uint8_t AxisID;
    BtnType BtnType;
    GUID DeviceGuid;
    void * Device;
} BUTTON;
