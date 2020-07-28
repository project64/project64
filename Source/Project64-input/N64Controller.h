#pragma once
#include "Button.h"

typedef struct
{
    BUTTON U_DPAD;
    BUTTON D_DPAD;
    BUTTON L_DPAD;
    BUTTON R_DPAD;
    BUTTON A_BUTTON;
    BUTTON B_BUTTON;
    BUTTON U_CBUTTON;
    BUTTON D_CBUTTON;
    BUTTON L_CBUTTON;
    BUTTON R_CBUTTON;
    BUTTON START_BUTTON;
    BUTTON Z_TRIG;
    BUTTON R_TRIG;
    BUTTON L_TRIG;
    BUTTON U_ANALOG;
    BUTTON D_ANALOG;
    BUTTON L_ANALOG;
    BUTTON R_ANALOG;
    uint8_t Range;
    uint8_t DeadZone;
    bool RealN64Range;
    bool RemoveDuplicate;
} N64CONTROLLER;
