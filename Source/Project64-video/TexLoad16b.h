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

static inline void load16bRGBA(uint8_t *src, uint8_t *dst, int wid_64, int height, int line, int ext)
{
    uint32_t *v6;
    uint32_t *v7;
    int v8;
    int v9;
    uint32_t v10;
    uint32_t v11;
    uint32_t *v12;
    uint32_t *v13;
    int v14;
    uint32_t v15;
    uint32_t v16;
    int v17;
    int v18;

    v6 = (uint32_t *)src;
    v7 = (uint32_t *)dst;
    v8 = height;
    do
    {
        v17 = v8;
        v9 = wid_64;
        do
        {
            v10 = bswap32(*v6);
            v11 = bswap32(v6[1]);
            ALOWORD(v10) = __ROR__((uint16_t)(v10 & 0xFFFF), 1);
            ALOWORD(v11) = __ROR__((uint16_t)(v11 & 0xFFFF), 1);
            v10 = __ROR__(v10, 16);
            v11 = __ROR__(v11, 16);
            ALOWORD(v10) = __ROR__((uint16_t)(v10 & 0xFFFF), 1);
            ALOWORD(v11) = __ROR__((uint16_t)(v11 & 0xFFFF), 1);
            *v7 = v10;
            v7[1] = v11;
            v6 += 2;
            v7 += 2;
            --v9;
        } while (v9);
        if (v17 == 1)
            break;
        v18 = v17 - 1;
        v12 = (uint32_t *)&src[(line + (uintptr_t)v6 - (uintptr_t)src) & 0xFFF];
        v13 = (uint32_t *)((char *)v7 + ext);
        v14 = wid_64;
        do
        {
            v15 = bswap32(v12[1]);
            v16 = bswap32(*v12);
            ALOWORD(v15) = __ROR__((uint16_t)(v15 & 0xFFFF), 1);
            ALOWORD(v16) = __ROR__((uint16_t)(v16 & 0xFFFF), 1);
            v15 = __ROR__(v15, 16);
            v16 = __ROR__(v16, 16);
            ALOWORD(v15) = __ROR__((uint16_t)(v15 & 0xFFFF), 1);
            ALOWORD(v16) = __ROR__((uint16_t)(v16 & 0xFFFF), 1);
            *v13 = v15;
            v13[1] = v16;
            v12 += 2;
            v13 += 2;
            --v14;
        } while (v14);
        v6 = (uint32_t *)&src[(line + (uintptr_t)v12 - (uintptr_t)src) & 0xFFF];
        v7 = (uint32_t *)((char *)v13 + ext);
        v8 = v18 - 1;
    } while (v18 != 1);
}

static inline void load16bIA(uint8_t *src, uint8_t *dst, int wid_64, int height, int line, int ext)
{
    uint32_t *v6;
    uint32_t *v7;
    int v8;
    int v9;
    uint32_t v10;
    uint32_t *v11;
    uint32_t *v12;
    int v13;
    uint32_t v14;
    int v15;
    int v16;

    v6 = (uint32_t *)src;
    v7 = (uint32_t *)dst;
    v8 = height;
    do
    {
        v15 = v8;
        v9 = wid_64;
        do
        {
            v10 = v6[1];
            *v7 = *v6;
            v7[1] = v10;
            v6 += 2;
            v7 += 2;
            --v9;
        } while (v9);
        if (v15 == 1)
            break;
        v16 = v15 - 1;
        v11 = (uint32_t *)((char *)v6 + line);
        v12 = (uint32_t *)((char *)v7 + ext);
        v13 = wid_64;
        do
        {
            v14 = *v11;
            *v12 = v11[1];
            v12[1] = v14;
            v11 += 2;
            v12 += 2;
            --v13;
        } while (v13);
        v6 = (uint32_t *)((char *)v11 + line);
        v7 = (uint32_t *)((char *)v12 + ext);
        v8 = v16 - 1;
    } while (v16 != 1);
}

//****************************************************************
// Size: 2, Format: 0
//

uint32_t Load16bRGBA(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int /*tile*/)
{
    if (wid_64 < 1) wid_64 = 1;
    if (height < 1) height = 1;
    int ext = (real_width - (wid_64 << 2)) << 1;

    load16bRGBA((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext);

    return (1 << 16) | GFX_TEXFMT_ARGB_1555;
}

//****************************************************************
// Size: 2, Format: 3
//

uint32_t Load16bIA(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int /*tile*/)
{
    if (wid_64 < 1) wid_64 = 1;
    if (height < 1) height = 1;
    int ext = (real_width - (wid_64 << 2)) << 1;

    load16bIA((uint8_t *)src, (uint8_t *)dst, wid_64, height, line, ext);

    return (1 << 16) | GFX_TEXFMT_ALPHA_INTENSITY_88;
}

//****************************************************************
// Size: 2, Format: 1
//

uint16_t yuv_to_rgb565(uint8_t y, uint8_t u, uint8_t v)
{
    //*
    float r = y + (1.370705f * (v - 128));
    float g = y - (0.698001f * (v - 128)) - (0.337633f * (u - 128));
    float b = y + (1.732446f * (u - 128));
    r *= 0.125f;
    g *= 0.25f;
    b *= 0.125f;
    //clipping the result
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    uint16_t c = (uint16_t)(((uint16_t)(r) << 11) |
        ((uint16_t)(g) << 5) |
        (uint16_t)(b));
    return c;
    //*/
    /*
    const uint32_t c = y - 16;
    const uint32_t d = u - 128;
    const uint32_t e = v - 128;

    uint32_t r =  (298 * c           + 409 * e + 128) & 0xf800;
    uint32_t g = ((298 * c - 100 * d - 208 * e + 128) >> 5) & 0x7e0;
    uint32_t b = ((298 * c + 516 * d           + 128) >> 11) & 0x1f;

    WORD texel = (WORD)(r | g | b);

    return texel;
    */
}

//****************************************************************
// Size: 2, Format: 1
//

uint32_t Load16bYUV(uintptr_t dst, uintptr_t /*src*/, int /*wid_64*/, int /*height*/, int /*line*/, int /*real_width*/, int tile)
{
    uint32_t * mb = (uint32_t*)(gfx.RDRAM + rdp.addr[rdp.tiles(tile).t_mem]); //pointer to the macro block
    uint16_t * tex = (uint16_t*)dst;
    uint16_t i;
    for (i = 0; i < 128; i++)
    {
        uint32_t t = mb[i]; //each uint32_t contains 2 pixels
        uint8_t y1 = (uint8_t)t & 0xFF;
        uint8_t v = (uint8_t)(t >> 8) & 0xFF;
        uint8_t y0 = (uint8_t)(t >> 16) & 0xFF;
        uint8_t u = (uint8_t)(t >> 24) & 0xFF;
        uint16_t c = yuv_to_rgb565(y0, u, v);
        *(tex++) = c;
        c = yuv_to_rgb565(y1, u, v);
        *(tex++) = c;
    }
    return (1 << 16) | GFX_TEXFMT_RGB_565;
}
