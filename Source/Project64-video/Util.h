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

#define NOT_TMU0	0x00
#define NOT_TMU1	0x01
#define NOT_TMU2	0x02

void util_init();

int cull_tri(gfxVERTEX **v);
void draw_tri(gfxVERTEX **v, uint16_t linew = 0);
void do_triangle_stuff(uint16_t linew = 0, int old_interpolate = TRUE);
void do_triangle_stuff_2(uint16_t linew = 0);
void apply_shade_mods(gfxVERTEX *v);

void update();
void update_scissor();

void set_message_combiner(void);

float ScaleZ(float z);

// positional and texel coordinate clipping
#define CCLIP(ux,lx,ut,lt,uc,lc) \
		if (ux > lx || lx < uc || ux > lc) { rdp.tri_n += 2; return; } \
		if (ux < uc) { \
			float p = (uc-ux)/(lx-ux); \
			ut = p*(lt-ut)+ut; \
			ux = uc; \
                		} \
		if (lx > lc) { \
			float p = (lc-ux)/(lx-ux); \
			lt = p*(lt-ut)+ut; \
			lx = lc; \
                		}

#define CCLIP2(ux,lx,ut,lt,un,ln,uc,lc) \
		if (ux > lx || lx < uc || ux > lc) { rdp.tri_n += 2; return; } \
		if (ux < uc) { \
			float p = (uc-ux)/(lx-ux); \
			ut = p*(lt-ut)+ut; \
			un = p*(ln-un)+un; \
			ux = uc; \
                		} \
		if (lx > lc) { \
			float p = (lc-ux)/(lx-ux); \
			lt = p*(lt-ut)+ut; \
			ln = p*(ln-un)+un; \
			lx = lc; \
                		}

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <stdlib.h>
#define bswap32(x) _byteswap_ulong(x)
#else
static inline uint32_t bswap32(uint32_t val)
{
    return (((val & 0xff000000) >> 24) |
        ((val & 0x00ff0000) >> 8) |
        ((val & 0x0000ff00) << 8) |
        ((val & 0x000000ff) << 24));
}
#endif

#define ALOWORD(x)   (*((uint16_t*)&(x)))   // low word

static inline uint16_t __ROR__(uint16_t value, unsigned int count)
{
    const unsigned int nbits = sizeof(uint16_t) * 8;
    count %= nbits;

    uint16_t low = (value << (nbits - count)) & 0xFFFF;
    value >>= count;
    value |= low;
    return value;
}

template<class T> static inline T __ROR__(T value, unsigned int count)
{
    const unsigned int nbits = sizeof(T) * 8;
    count %= nbits;

    T low = value << (nbits - count);
    value >>= count;
    value |= low;
    return value;
}

// rotate left
template<class T> static T __ROL__(T value, unsigned int count)
{
    const unsigned int nbits = sizeof(T) * 8;
    count %= nbits;

    T high = value >> (nbits - count);
    value <<= count;
    value |= high;
    return value;
}
