/****************************************************************************
 *                                                                          *
 * Project64 - A Nintendo 64 emulator.                                      *
 * http://www.pj64-emu.com/                                                 *
 * Copyright (C) 2016 Project64. All rights reserved.                       *
 * Copyright (C) 2014 Bobby Smiles                                          *
 *                                                                          *
 * License:                                                                 *
 * GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
 * version 2 of the License, or (at your option) any later version.         *
 *                                                                          *
 ****************************************************************************/
#pragma once

static inline int16_t clamp_s16(int_fast32_t x)
{
    x = (x < INT16_MIN) ? INT16_MIN : x;
    x = (x > INT16_MAX) ? INT16_MAX : x;

    return (int16_t)x;
}

static inline int32_t vmulf(int16_t x, int16_t y)
{
    return (((int32_t)(x))*((int32_t)(y))+0x4000)>>15;
}
