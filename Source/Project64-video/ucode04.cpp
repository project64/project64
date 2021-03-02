// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html

//****************************************************************
// uCode 4 - RSP SW 2.0D EXT
//****************************************************************
#include <Project64-video/rdp.h>
#include <Project64-video/Gfx_1.3.h>
#include <Project64-video/trace.h>
#include <Project64-video/ucode.h>
#include "ucode00.h"

void uc4_vertex()
{
    int v0 = 0;     // Current vertex
    int n = ((rdp.cmd0 >> 4) & 0xFFF) / 33 + 1; // Number of vertices to copy
    rsp_vertex(v0, n);
}

void uc4_tri1()
{
    int v1 = ((rdp.cmd1 >> 16) & 0xFF) / 5;
    int v2 = ((rdp.cmd1 >> 8) & 0xFF) / 5;
    int v3 = (rdp.cmd1 & 0xFF) / 5;
    WriteTrace(TraceRDP, TraceDebug, "uc4:tri1 #%d - %d, %d, %d", rdp.tri_n,
        v1, v2, v3);

    gfxVERTEX *vtx[3] = {
        &rdp.vtx(v1),
        &rdp.vtx(v2),
        &rdp.vtx(v3)
    };

    rsp_tri1(vtx);
}

void uc4_quad3d()
{
    WriteTrace(TraceRDP, TraceDebug, "uc4:quad3d #%d, #%d", rdp.tri_n, rdp.tri_n + 1);

    gfxVERTEX *vtx[6] = {
        &rdp.vtx(((rdp.cmd1 >> 24) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 16) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 8) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 24) & 0xFF) / 5),
        &rdp.vtx(((rdp.cmd1 >> 8) & 0xFF) / 5),
        &rdp.vtx((rdp.cmd1 & 0xFF) / 5)
    };

    rsp_tri2(vtx);
}