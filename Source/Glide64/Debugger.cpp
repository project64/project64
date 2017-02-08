/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include <stdarg.h>
#include <string.h>

#include "Gfx_1.3.h"
#include "Util.h"
#include "Debugger.h"

#define SX(x) ((x)*rdp.scale_1024)
#define SY(x) ((x)*rdp.scale_768)

//
// output - output debugger text
//

void output (float x, float y, int scale, const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  vsprintf(out_buf, fmt, ap);
  va_end(ap);

  uint8_t c,r;
  for (uint32_t i=0; i<strlen(out_buf); i++)
  {
    c = ((out_buf[i]-32) & 0x1F) * 8;//<< 3;
    r = (((out_buf[i]-32) & 0xE0) >> 5) * 16;//<< 4;
    VERTEX v[4] = { { SX(x), SY(768-y), 1, 1,   (float)c, r+16.0f, 0, 0, {0, 0, 0, 0} },
      { SX(x+8), SY(768-y), 1, 1,   c+8.0f, r+16.0f, 0, 0, {0, 0, 0, 0} },
      { SX(x), SY(768-y-16), 1, 1,  (float)c, (float)r, 0, 0, {0, 0, 0, 0} },
      { SX(x+8), SY(768-y-16), 1, 1,  c+8.0f, (float)r, 0, 0, {0, 0, 0, 0} }
      };
    if (!scale)
    {
      v[0].x = x;
      v[0].y = y;
      v[1].x = x+8;
      v[1].y = y;
      v[2].x = x;
      v[2].y = y-16;
      v[3].x = x+8;
      v[3].y = y-16;
    }

    ConvertCoordsKeep (v, 4);

    grDrawTriangle (&v[0], &v[1], &v[2]);
    grDrawTriangle (&v[1], &v[3], &v[2]);

    x+=8;
  }
}
