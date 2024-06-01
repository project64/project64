// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#include <Project64-video/rdp.h>
#include <Project64-video/Gfx_1.3.h>
#include <Project64-video/trace.h>
#include <Project64-video/ucode.h>
#include <math.h>
#include "ucode00.h"

//
// vertex - loads vertices
//

void uc1_vertex()
{
    int v0 = (rdp.cmd0 >> 17) & 0x7F;     // Current vertex
    int n = (rdp.cmd0 >> 10) & 0x3F;    // Number to copy
    rsp_vertex(v0, n);
}

//
// tri1 - renders a triangle
//

void uc1_tri1()
{
    if (rdp.skip_drawing)
    {
        WriteTrace(TraceRDP, TraceDebug, "uc1:tri1. skipped");
        return;
    }
    WriteTrace(TraceRDP, TraceDebug, "uc1:tri1 #%d - %d, %d, %d - %08lx - %08lx", rdp.tri_n,
        ((rdp.cmd1 >> 17) & 0x7F),
        ((rdp.cmd1 >> 9) & 0x7F),
        ((rdp.cmd1 >> 1) & 0x7F), rdp.cmd0, rdp.cmd1);

    gfxVERTEX *vtx[3] = {
        &rdp.vtx((rdp.cmd1 >> 17) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 9) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 1) & 0x7F)
    };

    rsp_tri1(vtx);
}

void uc1_tri2()
{
    if (rdp.skip_drawing)
    {
        WriteTrace(TraceRDP, TraceDebug, "uc1:tri2. skipped");
        return;
    }
    WriteTrace(TraceRDP, TraceDebug, "uc1:tri2");

    WriteTrace(TraceRDP, TraceDebug, " #%d, #%d - %d, %d, %d - %d, %d, %d", rdp.tri_n, rdp.tri_n + 1,
        ((rdp.cmd0 >> 17) & 0x7F),
        ((rdp.cmd0 >> 9) & 0x7F),
        ((rdp.cmd0 >> 1) & 0x7F),
        ((rdp.cmd1 >> 17) & 0x7F),
        ((rdp.cmd1 >> 9) & 0x7F),
        ((rdp.cmd1 >> 1) & 0x7F));

    gfxVERTEX *vtx[6] = {
        &rdp.vtx((rdp.cmd0 >> 17) & 0x7F),
        &rdp.vtx((rdp.cmd0 >> 9) & 0x7F),
        &rdp.vtx((rdp.cmd0 >> 1) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 17) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 9) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 1) & 0x7F)
    };

    rsp_tri2(vtx);
}

void uc1_quad3d()
{
        WriteTrace(TraceRDP, TraceDebug, "uc1:quad3d #%d, #%d", rdp.tri_n, rdp.tri_n + 1);

        gfxVERTEX *vtx[6] = {
            &rdp.vtx((rdp.cmd1 >> 25) & 0x7F),
            &rdp.vtx((rdp.cmd1 >> 17) & 0x7F),
            &rdp.vtx((rdp.cmd1 >> 9) & 0x7F),
            &rdp.vtx((rdp.cmd1 >> 1) & 0x7F),
            &rdp.vtx((rdp.cmd1 >> 25) & 0x7F),
            &rdp.vtx((rdp.cmd1 >> 9) & 0x7F)
        };

        rsp_tri2(vtx);
}

uint32_t branch_dl = 0;

void uc1_rdphalf_1()
{
    WriteTrace(TraceRDP, TraceDebug, "uc1:rdphalf_1");
    branch_dl = rdp.cmd1;
    rdphalf_1();
}

void uc1_branch_z()
{
    uint32_t addr = segoffset(branch_dl);
    WriteTrace(TraceRDP, TraceDebug, "uc1:branch_less_z, addr: %08lx", addr);
    uint32_t vtx = (rdp.cmd0 & 0xFFF) >> 1;
    if (fabs(rdp.vtx(vtx).z) <= (rdp.cmd1/*&0xFFFF*/))
    {
        rdp.pc[rdp.pc_i] = addr;
    }
}
