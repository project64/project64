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
// 8-bit Horizontal Mirror

static inline void mirror8bS(uint8_t *tex, uint8_t *start, int width, int height, int mask, int line, int full, int count)
{
    uint8_t *v8;
    int v9;
    int v10;

    v8 = start;
    v9 = height;
    do
    {
        v10 = 0;
        do
        {
            if (width & (v10 + width))
                *v8++ = *(&tex[mask] - (mask & v10));
            else
                *v8++ = tex[mask & v10];
            ++v10;
        } while (v10 != count);
        v8 += line;
        tex += full;
        --v9;
    } while (v9);
}

static inline void wrap8bS(uint8_t *tex, uint8_t *start, int height, int mask, int line, int full, int count)
{
    uint32_t *v7;
    int v8;
    int v9;

    v7 = (uint32_t *)start;
    v8 = height;
    do
    {
        v9 = 0;
        do
        {
            *v7 = *(uint32_t *)&tex[4 * (mask & v9)];
            ++v7;
            ++v9;
        } while (v9 != count);
        v7 = (uint32_t *)((char *)v7 + line);
        tex += full;
        --v8;
    } while (v8);
}

static inline void clamp8bS(uint8_t *tex, uint8_t *constant, int height, int line, int full, int count)
{
    uint8_t *v6;
    uint8_t *v7;
    int v8;
    uint8_t v9;
    int v10;

    v6 = constant;
    v7 = tex;
    v8 = height;
    do
    {
        v9 = *v6;
        v10 = count;
        do
        {
            *v7++ = v9;
            --v10;
        } while (v10);
        v6 += full;
        v7 += line;
        --v8;
    } while (v8);
}

void Mirror8bS(unsigned char * tex, uint32_t mask, uint32_t max_width, uint32_t real_width, uint32_t height)
{
    if (mask == 0) return;

    uint32_t mask_width = (1 << mask);
    uint32_t mask_mask = (mask_width - 1);
    if (mask_width >= max_width) return;
    int count = max_width - mask_width;
    if (count <= 0) return;
    int line_full = real_width;
    int line = line_full - (count);
    if (line < 0) return;
    unsigned char * start = tex + (mask_width);
    mirror8bS(tex, start, mask_width, height, mask_mask, line, line_full, count);
}

//****************************************************************
// 8-bit Horizontal Wrap (like mirror) ** UNTESTED **

void Wrap8bS(unsigned char * tex, uint32_t mask, uint32_t max_width, uint32_t real_width, uint32_t height)
{
    if (mask == 0) return;

    uint32_t mask_width = (1 << mask);
    uint32_t mask_mask = (mask_width - 1) >> 2;
    if (mask_width >= max_width) return;
    int count = (max_width - mask_width) >> 2;
    if (count <= 0) return;
    int line_full = real_width;
    int line = line_full - (count << 2);
    if (line < 0) return;
    unsigned char * start = tex + (mask_width);
    wrap8bS(tex, start, height, mask_mask, line, line_full, count);
}

//****************************************************************
// 8-bit Horizontal Clamp

void Clamp8bS(unsigned char * tex, uint32_t width, uint32_t clamp_to, uint32_t real_width, uint32_t real_height)
{
    if (real_width <= width) return;

    unsigned char * dest = tex + (width);
    unsigned char * constant = dest - 1;
    int count = clamp_to - width;

    int line_full = real_width;
    int line = width;
    clamp8bS(dest, constant, real_height, line, line_full, count);
}

//****************************************************************
// 8-bit Vertical Mirror

void Mirror8bT(unsigned char * tex, uint32_t mask, uint32_t max_height, uint32_t real_width)
{
    if (mask == 0) return;

    uint32_t mask_height = (1 << mask);
    uint32_t mask_mask = mask_height - 1;
    if (max_height <= mask_height) return;
    int line_full = real_width;

    unsigned char * dst = tex + mask_height * line_full;

    for (uint32_t y = mask_height; y < max_height; y++)
    {
        if (y & mask_height)
        {
            // mirrored
            memcpy((void*)dst, (void*)(tex + (mask_mask - (y & mask_mask)) * line_full), line_full);
        }
        else
        {
            // not mirrored
            memcpy((void*)dst, (void*)(tex + (y & mask_mask) * line_full), line_full);
        }

        dst += line_full;
    }
}

//****************************************************************
// 8-bit Vertical Wrap

void Wrap8bT(unsigned char * tex, uint32_t mask, uint32_t max_height, uint32_t real_width)
{
    if (mask == 0) return;

    uint32_t mask_height = (1 << mask);
    uint32_t mask_mask = mask_height - 1;
    if (max_height <= mask_height) return;
    int line_full = real_width;

    unsigned char * dst = tex + mask_height * line_full;

    for (uint32_t y = mask_height; y < max_height; y++)
    {
        // not mirrored
        memcpy((void*)dst, (void*)(tex + (y & mask_mask) * line_full), line_full);

        dst += line_full;
    }
}

//****************************************************************
// 8-bit Vertical Clamp

void Clamp8bT(unsigned char * tex, uint32_t height, uint32_t real_width, uint32_t clamp_to)
{
    int line_full = real_width;
    unsigned char * dst = tex + height * line_full;
    unsigned char * const_line = dst - line_full;

    for (uint32_t y = height; y < clamp_to; y++)
    {
        memcpy((void*)dst, (void*)const_line, line_full);
        dst += line_full;
    }
}
