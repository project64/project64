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

//****************************************************************
// Size: 2, Format: 0
//
// Load 32bit RGBA texture
// Based on sources of angrylion's software plugin.
//
uint32_t Load32bRGBA(uintptr_t dst, uintptr_t src, int wid_64, int height, int line, int real_width, int tile)
{
    if (height < 1) height = 1;
    const uint16_t *tmem16 = (uint16_t*)rdp.tmem;
    const uint32_t tbase = (src - (uintptr_t)rdp.tmem) >> 1;
    const uint32_t width = maxval(1, wid_64 << 1);
    const int ext = real_width - width;
    line = width + (line >> 2);
    uint32_t s, t, c;
    uint32_t * tex = (uint32_t*)dst;
    uint16_t rg, ba;
    for (t = 0; t < (uint32_t)height; t++)
    {
        uint32_t tline = tbase + line * t;
        uint32_t xorval = (t & 1) ? 3 : 1;
        for (s = 0; s < width; s++)
        {
            uint32_t taddr = ((tline + s) ^ xorval) & 0x3ff;
            rg = tmem16[taddr];
            ba = tmem16[taddr | 0x400];
            c = ((ba & 0xFF) << 24) | (rg << 8) | (ba >> 8);
            *tex++ = c;
        }
        tex += ext;
    }
    int id = tile - rdp.cur_tile;
    uint32_t mod = (id == 0) ? cmb.mod_0 : cmb.mod_1;
    if (mod || !voodoo.sup_32bit_tex)
    {
        //convert to ARGB_4444
        const uint32_t tex_size = real_width * height;
        tex = (uint32_t *)dst;
        uint16_t *tex16 = (uint16_t*)dst;
        uint16_t a, r, g, b;
        for (uint32_t i = 0; i < tex_size; i++) {
            c = tex[i];
            a = (c >> 28) & 0xF;
            r = (c >> 20) & 0xF;
            g = (c >> 12) & 0xF;
            b = (c >> 4) & 0xF;
            tex16[i] = (a << 12) | (r << 8) | (g << 4) | b;
        }
        return (1 << 16) | GFX_TEXFMT_ARGB_4444;
    }
    return (2 << 16) | GFX_TEXFMT_ARGB_8888;
}

//****************************************************************
// LoadTile for 32bit RGBA texture
// Based on sources of angrylion's software plugin.
//
void LoadTile32b(uint32_t tile, uint32_t ul_s, uint32_t ul_t, uint32_t width, uint32_t height)
{
    const uint32_t line = rdp.tiles(tile).line << 2;
    const uint32_t tbase = rdp.tiles(tile).t_mem << 2;
    const uint32_t addr = rdp.timg.addr >> 2;
    const uint32_t* src = (const uint32_t*)gfx.RDRAM;
    uint16_t *tmem16 = (uint16_t*)rdp.tmem;
    uint32_t c, ptr, tline, s, xorval;

    for (uint32_t j = 0; j < height; j++)
    {
        tline = tbase + line * j;
        s = ((j + ul_t) * rdp.timg.width) + ul_s;
        xorval = (j & 1) ? 3 : 1;
        for (uint32_t i = 0; i < width; i++)
        {
            c = src[addr + s + i];
            ptr = ((tline + i) ^ xorval) & 0x3ff;
            tmem16[ptr] = c >> 16;
            tmem16[ptr | 0x400] = c & 0xffff;
        }
    }
}

//****************************************************************
// LoadBlock for 32bit RGBA texture
// Based on sources of angrylion's software plugin.
//
void LoadBlock32b(uint32_t tile, uint32_t ul_s, uint32_t ul_t, uint32_t lr_s, uint32_t dxt)
{
    const uint32_t * src = (const uint32_t*)gfx.RDRAM;
    const uint32_t tb = rdp.tiles(tile).t_mem << 2;
    const uint32_t tiwindwords = rdp.timg.width;
    const uint32_t slindwords = ul_s;
    const uint32_t line = rdp.tiles(tile).line << 2;

    uint16_t *tmem16 = (uint16_t*)rdp.tmem;
    uint32_t addr = rdp.timg.addr >> 2;
    uint32_t width = (lr_s - ul_s + 1) << 2;
    if (width & 7)
        width = (width & (~7)) + 8;

    if (dxt != 0)
    {
        uint32_t j = 0;
        uint32_t t = 0;
        uint32_t oldt = 0;
        uint32_t ptr;

        addr += (ul_t * tiwindwords) + slindwords;
        uint32_t c = 0;
        for (uint32_t i = 0; i < width; i += 2)
        {
            oldt = t;
            t = ((j >> 11) & 1) ? 3 : 1;
            if (t != oldt)
                i += line;
            ptr = ((tb + i) ^ t) & 0x3ff;
            c = src[addr + i];
            tmem16[ptr] = c >> 16;
            tmem16[ptr | 0x400] = c & 0xffff;
            ptr = ((tb + i + 1) ^ t) & 0x3ff;
            c = src[addr + i + 1];
            tmem16[ptr] = c >> 16;
            tmem16[ptr | 0x400] = c & 0xffff;
            j += dxt;
        }
    }
    else
    {
        addr += (ul_t * tiwindwords) + slindwords;
        uint32_t c, ptr;
        for (uint32_t i = 0; i < width; i++)
        {
            ptr = ((tb + i) ^ 1) & 0x3ff;
            c = src[addr + i];
            tmem16[ptr] = c >> 16;
            tmem16[ptr | 0x400] = c & 0xffff;
        }
    }
}
