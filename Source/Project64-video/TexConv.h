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
static inline void texConv_ARGB1555_ARGB4444(uint8_t *src, uint8_t *dst, int size)
{
    uint32_t *v3;
    uint32_t *v4;
    int v5;
    uint32_t v6;
    uint32_t v7;

    v3 = (uint32_t *)src;
    v4 = (uint32_t *)dst;
    v5 = size;
    do
    {
        v6 = *v3;
        ++v3;
        v7 = v6;
        *v4 = ((v7 & 0x1E001E) >> 1) | ((v6 & 0x3C003C0) >> 2) | ((v6 & 0x78007800) >> 3) | ((v6 & 0x80008000) >> 3) | ((v6 & 0x80008000) >> 2) | ((v6 & 0x80008000) >> 1) | (v6 & 0x80008000);
        ++v4;
        --v5;
    } while (v5);
}

static inline void texConv_AI88_ARGB4444(uint8_t *src, uint8_t *dst, int size)
{
    uint32_t *v3;
    uint32_t *v4;
    int v5;
    uint32_t v6;
    uint32_t v7;

    v3 = (uint32_t *)src;
    v4 = (uint32_t *)dst;
    v5 = size;
    do
    {
        v6 = *v3;
        ++v3;
        v7 = v6;
        *v4 = (16 * (v7 & 0xF000F0) >> 8) | (v7 & 0xF000F0) | (16 * (v7 & 0xF000F0)) | (v6 & 0xF000F000);
        ++v4;
        --v5;
    } while (v5);
}

static inline void texConv_AI44_ARGB4444(uint8_t *src, uint8_t *dst, int size)
{
    uint32_t *v3;
    uint32_t *v4;
    int v5;
    uint32_t v6;
    uint32_t *v7;

    v3 = (uint32_t *)src;
    v4 = (uint32_t *)dst;
    v5 = size;
    do
    {
        v6 = *v3;
        ++v3;
        *v4 = ((((uint16_t)v6 << 8) & 0xFF00 & 0xF00u) >> 8) | ((((uint16_t)v6 << 8) & 0xFF00 & 0xF00u) >> 4) | (uint16_t)(((uint16_t)v6 << 8) & 0xFF00) | (((v6 << 16) & 0xF000000) >> 8) | (((v6 << 16) & 0xF000000) >> 4) | ((v6 << 16) & 0xFF000000);
        v7 = v4 + 1;
        *v7 = (((v6 >> 8) & 0xF00) >> 8) | (((v6 >> 8) & 0xF00) >> 4) | ((v6 >> 8) & 0xFF00) | ((v6 & 0xF000000) >> 8) | ((v6 & 0xF000000) >> 4) | (v6 & 0xFF000000);
        v4 = v7 + 1;
        --v5;
    } while (v5);
}

static inline void texConv_A8_ARGB4444(uint8_t *src, uint8_t *dst, int size)
{
    uint32_t *v3;
    uint32_t *v4;
    int v5;
    uint32_t v6;
    uint32_t v7;
    uint32_t *v8;

    v3 = (uint32_t *)src;
    v4 = (uint32_t *)dst;
    v5 = size;
    do
    {
        v6 = *v3;
        ++v3;
        v7 = v6;
        *v4 = ((v6 & 0xF0) << 8 >> 12) | (uint8_t)(v6 & 0xF0) | (16 * (uint8_t)(v6 & 0xF0) & 0xFFFFFFF) | ((uint8_t)(v6 & 0xF0) << 8) | (16 * (uint16_t)(v6 & 0xF000) & 0xFFFFF) | (((uint16_t)(v6 & 0xF000) << 8) & 0xFFFFFF) | (((uint16_t)(v6 & 0xF000) << 12) & 0xFFFFFFF) | ((uint16_t)(v6 & 0xF000) << 16);
        v8 = v4 + 1;
        *v8 = ((v7 & 0xF00000) >> 20) | ((v7 & 0xF00000) >> 16) | ((v7 & 0xF00000) >> 12) | ((v7 & 0xF00000) >> 8) | ((v6 & 0xF0000000) >> 12) | ((v6 & 0xF0000000) >> 8) | ((v6 & 0xF0000000) >> 4) | (v6 & 0xF0000000);
        v4 = v8 + 1;
        --v5;
    } while (v5);
}

void TexConv_ARGB1555_ARGB4444(unsigned char * src, unsigned char * dst, int width, int height)
{
    int size = (width * height) >> 1;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
    // 2 pixels are converted in one loop
    // NOTE: width * height must be a multiple of 2
    texConv_ARGB1555_ARGB4444(src, dst, size);
}

void TexConv_AI88_ARGB4444(unsigned char * src, unsigned char * dst, int width, int height)
{
    int size = (width * height) >> 1;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
    // 2 pixels are converted in one loop
    // NOTE: width * height must be a multiple of 2
    texConv_AI88_ARGB4444(src, dst, size);
}

void TexConv_AI44_ARGB4444(unsigned char * src, unsigned char * dst, int width, int height)
{
    int size = (width * height) >> 2;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
    // 4 pixels are converted in one loop
    // NOTE: width * height must be a multiple of 4
    texConv_AI44_ARGB4444(src, dst, size);
}

void TexConv_A8_ARGB4444(unsigned char * src, unsigned char * dst, int width, int height)
{
    int size = (width * height) >> 2;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
    // 4 pixels are converted in one loop
    // NOTE: width * height must be a multiple of 4
    texConv_A8_ARGB4444(src, dst, size);
}
