/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/

#pragma once
#include <Common/stdtypes.h>

typedef uint8_t gfxAlpha_t;
enum gfxTextureFormat_t
{
    GFX_TEXFMT_8BIT = 0x0,
    GFX_TEXFMT_RGB_332 = GFX_TEXFMT_8BIT,
    GFX_TEXFMT_YIQ_422 = 0x1,
    GFX_TEXFMT_ALPHA_8 = 0x2, /* (0..0xFF) alpha     */
    GFX_TEXFMT_INTENSITY_8 = 0x3, /* (0..0xFF) intensity */
    GFX_TEXFMT_ALPHA_INTENSITY_44 = 0x4,
    GFX_TEXFMT_P_8 = 0x5, /* 8-bit palette */
    GFX_TEXFMT_RSVD0 = 0x6, /* GFX_TEXFMT_P_8_RGBA */
    GFX_TEXFMT_P_8_6666 = GFX_TEXFMT_RSVD0,
    GFX_TEXFMT_P_8_6666_EXT = GFX_TEXFMT_RSVD0,
    GFX_TEXFMT_RSVD1 = 0x7,
    GFX_TEXFMT_16BIT = 0x8,
    GFX_TEXFMT_ARGB_8332 = GFX_TEXFMT_16BIT,
    GFX_TEXFMT_AYIQ_8422 = 0x9,
    GFX_TEXFMT_RGB_565 = 0xa,
    GFX_TEXFMT_ARGB_1555 = 0xb,
    GFX_TEXFMT_ARGB_4444 = 0xc,
    GFX_TEXFMT_ALPHA_INTENSITY_88 = 0xd,
    GFX_TEXFMT_AP_88 = 0xe, /* 8-bit alpha 8-bit palette */
    GFX_TEXFMT_RSVD2 = 0xf,
    GFX_TEXFMT_RSVD4 = GFX_TEXFMT_RSVD2,
    GFX_TEXFMT_ARGB_CMP_FXT1 = 0x11,
    GFX_TEXFMT_ARGB_8888 = 0x12,
    GFX_TEXFMT_YUYV_422 = 0x13,
    GFX_TEXFMT_UYVY_422 = 0x14,
    GFX_TEXFMT_AYUV_444 = 0x15,
    GFX_TEXFMT_ARGB_CMP_DXT1 = 0x16,
    GFX_TEXFMT_ARGB_CMP_DXT2 = 0x17,
    GFX_TEXFMT_ARGB_CMP_DXT3 = 0x18,
    GFX_TEXFMT_ARGB_CMP_DXT4 = 0x19,
    GFX_TEXFMT_ARGB_CMP_DXT5 = 0x1A,
    GFX_TEXFMT_RGB_888 = 0xFF,
    GFX_TEXFMT_GZ = 0x8000,
};

