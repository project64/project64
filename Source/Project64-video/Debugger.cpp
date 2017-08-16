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
#include <Project64-video/Renderer/Renderer.h>
#include <stdarg.h>
#include <string.h>

#include "Gfx_1.3.h"
#include "Util.h"
#include "Debugger.h"

//
// output - output debugger text
//
void output(float x, float y, int scale, const char *fmt, ...)
{
    float scale_1024 = g_scr_res_x / 1024.0f;
    float scale_768 = g_scr_res_y / 768.0f;

    va_list ap;
    va_start(ap, fmt);
    vsprintf(out_buf, fmt, ap);
    va_end(ap);

    uint8_t c, r;
    for (uint32_t i = 0; i < strlen(out_buf); i++)
    {
        c = ((out_buf[i] - 32) & 0x1F) * 8;//<< 3;
        r = (((out_buf[i] - 32) & 0xE0) >> 5) * 16;//<< 4;
        gfxVERTEX v[4] = { { x * scale_1024, (768 - y) * scale_768, 1, 1,   (float)c, r + 16.0f, 0, 0,{ 0, 0, 0, 0 } },
        { (x + 8) * scale_1024, (768 - y) * scale_768, 1, 1,   c + 8.0f, r + 16.0f, 0, 0,{ 0, 0, 0, 0 } },
        { x * scale_1024, (768 - y - 16) * scale_768, 1, 1,  (float)c, (float)r, 0, 0,{ 0, 0, 0, 0 } },
        { (x + 8) * scale_1024, (768 - y - 16) * scale_768, 1, 1,  c + 8.0f, (float)r, 0, 0,{ 0, 0, 0, 0 } }
        };
        if (!scale)
        {
            v[0].x = x;
            v[0].y = y;
            v[1].x = x + 8;
            v[1].y = y;
            v[2].x = x;
            v[2].y = y - 16;
            v[3].x = x + 8;
            v[3].y = y - 16;
        }

        ConvertCoordsKeep(v, 4);

        gfxDrawTriangle(&v[0], &v[1], &v[2]);
        gfxDrawTriangle(&v[1], &v[3], &v[2]);

        x += 8;
    }
}