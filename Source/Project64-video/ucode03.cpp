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
#include <Project64-video/rdp.h>
#include <Project64-video/Gfx_1.3.h>
#include <Project64-video/trace.h>
#include <Project64-video/ucode.h>
#include "ucode00.h"

//
// vertex - loads vertices
//

void uc3_vertex()
{
    int v0 = ((rdp.cmd0 >> 16) & 0xFF) / 5;      // Current vertex
    int n = (uint16_t)((rdp.cmd0 & 0xFFFF) + 1) / 0x210;    // Number to copy

    if (v0 >= 32)
        v0 = 31;

    if ((v0 + n) > 32)
        n = 32 - v0;

    rsp_vertex(v0, n);
}

//
// tri1 - renders a triangle
//

void uc3_tri1()
{
    WriteTrace(TraceRDP, TraceDebug, "uc3:tri1 #%d - %d, %d, %d - %08lx - %08lx", rdp.tri_n,
        ((rdp.cmd1 >> 16) & 0xFF) / 5,
        ((rdp.cmd1 >> 8) & 0xFF) / 5,
        ((rdp.cmd1) & 0xFF) / 5, rdp.cmd0, rdp.cmd1);

    gfxVERTEX *vtx[3] = {
        &rdp.vtx(((rdp.cmd1 >> 16) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 8) & 0xFF) / 5),
        &rdp.vtx((rdp.cmd1 & 0xFF) / 5)
    };

    rsp_tri1(vtx);
}

void uc3_tri2()
{
    WriteTrace(TraceRDP, TraceDebug, "uc3:tri2 #%d, #%d - %d, %d, %d - %d, %d, %d", rdp.tri_n, rdp.tri_n + 1,
        ((rdp.cmd0 >> 16) & 0xFF) / 5,
        ((rdp.cmd0 >> 8) & 0xFF) / 5,
        ((rdp.cmd0) & 0xFF) / 5,
        ((rdp.cmd1 >> 16) & 0xFF) / 5,
        ((rdp.cmd1 >> 8) & 0xFF) / 5,
        ((rdp.cmd1) & 0xFF) / 5);

    gfxVERTEX *vtx[6] = {
        &rdp.vtx(((rdp.cmd0 >> 16) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd0 >> 8) & 0xFF) / 5),
        &rdp.vtx((rdp.cmd0 & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 16) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 8) & 0xFF) / 5),
        &rdp.vtx((rdp.cmd1 & 0xFF) / 5)
    };

    rsp_tri2(vtx);
}

void uc3_quad3d()
{
    WriteTrace(TraceRDP, TraceDebug, "uc3:quad3d #%d, #%d", rdp.tri_n, rdp.tri_n + 1);

    gfxVERTEX *vtx[6] = {
        &rdp.vtx(((rdp.cmd1 >> 24) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 16) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 8) & 0xFF) / 5),
        &rdp.vtx((rdp.cmd1 & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 24) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 8) & 0xFF) / 5)
    };

    rsp_tri2(vtx);
}