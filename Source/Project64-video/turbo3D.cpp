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
#include <math.h>
#include "ucode00.h"
#include "util.h"

/******************Turbo3D microcode*************************/
struct t3dGlobState
{
    uint16_t		pad0;
    uint16_t		perspNorm;
    uint32_t		flag;
    uint32_t		othermode0;
    uint32_t		othermode1;
    uint32_t		segBases[16];
    /* the viewport to use */
    short     vsacle1;
    short     vsacle0;
    short     vsacle3;
    short     vsacle2;
    short     vtrans1;
    short     vtrans0;
    short     vtrans3;
    short     vtrans2;
    uint32_t  rdpCmds;
};

struct t3dState {
    uint32_t	renderState;	/* render state */
    uint32_t	textureState;	/* texture state */
    uint8_t	flag;
    uint8_t	triCount;	/* how many tris? */
    uint8_t	vtxV0;		/* where to load verts? */
    uint8_t	vtxCount;	/* how many verts? */
    uint32_t	rdpCmds;	/* ptr (segment address) to RDP DL */
    uint32_t	othermode0;
    uint32_t	othermode1;
};

struct t3dTriN {
    uint8_t	flag, v2, v1, v0;	/* flag is which one for flat shade */
};

static void t3dProcessRDP(uint32_t a)
{
    if (a)
    {
        rdp.LLE = 1;
        rdp.cmd0 = ((uint32_t*)gfx.RDRAM)[a++];
        rdp.cmd1 = ((uint32_t*)gfx.RDRAM)[a++];
        while (rdp.cmd0 + rdp.cmd1) {
            gfx_instruction[0][rdp.cmd0 >> 24]();
            rdp.cmd0 = ((uint32_t*)gfx.RDRAM)[a++];
            rdp.cmd1 = ((uint32_t*)gfx.RDRAM)[a++];
            uint32_t cmd = rdp.cmd0 >> 24;
            if (cmd == 0xE4 || cmd == 0xE5)
            {
                rdp.cmd2 = ((uint32_t*)gfx.RDRAM)[a++];
                rdp.cmd3 = ((uint32_t*)gfx.RDRAM)[a++];
            }
        }
        rdp.LLE = 0;
    }
}

static void t3dLoadGlobState(uint32_t pgstate)
{
    t3dGlobState *gstate = (t3dGlobState*)&gfx.RDRAM[segoffset(pgstate)];
    WriteTrace(TraceRDP, TraceDebug, "Global state. pad0: %04lx, perspNorm: %04lx, flag: %08lx", gstate->pad0, gstate->perspNorm, gstate->flag);
    rdp.cmd0 = gstate->othermode0;
    rdp.cmd1 = gstate->othermode1;
    rdp_setothermode();

    for (int s = 0; s < 16; s++)
    {
        rdp.segment[s] = gstate->segBases[s];
        WriteTrace(TraceRDP, TraceDebug, "segment: %08lx -> seg%d", rdp.segment[s], s);
    }

    short scale_x = gstate->vsacle0 / 4;
    short scale_y = gstate->vsacle1 / 4;;
    short scale_z = gstate->vsacle2;
    short trans_x = gstate->vtrans0 / 4;
    short trans_y = gstate->vtrans1 / 4;
    short trans_z = gstate->vtrans2;
    rdp.view_scale[0] = scale_x * rdp.scale_x;
    rdp.view_scale[1] = -scale_y * rdp.scale_y;
    rdp.view_scale[2] = 32.0f * scale_z;
    rdp.view_trans[0] = trans_x * rdp.scale_x;
    rdp.view_trans[1] = trans_y * rdp.scale_y;
    rdp.view_trans[2] = 32.0f * trans_z;
    rdp.update |= UPDATE_VIEWPORT;
    WriteTrace(TraceRDP, TraceDebug, "viewport scale(%d, %d, %d), trans(%d, %d, %d)", scale_x, scale_y, scale_z,
        trans_x, trans_y, trans_z);

    t3dProcessRDP(segoffset(gstate->rdpCmds) >> 2);
}

static void t3d_vertex(uint32_t addr, uint32_t v0, uint32_t n)
{
    float x, y, z;

    rdp.v0 = v0; // Current vertex
    rdp.vn = n; // Number of vertices to copy
    n <<= 4;

    for (uint32_t i = 0; i < n; i += 16)
    {
        VERTEX &v = rdp.vtx(v0 + (i >> 4));
        x = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 0) ^ 1];
        y = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 1) ^ 1];
        z = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 2) ^ 1];
        v.flags = ((uint16_t*)gfx.RDRAM)[(((addr + i) >> 1) + 3) ^ 1];
        v.ou = 2.0f * (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 4) ^ 1];
        v.ov = 2.0f * (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 5) ^ 1];
        v.uv_scaled = 0;
        v.r = ((uint8_t*)gfx.RDRAM)[(addr + i + 12) ^ 3];
        v.g = ((uint8_t*)gfx.RDRAM)[(addr + i + 13) ^ 3];
        v.b = ((uint8_t*)gfx.RDRAM)[(addr + i + 14) ^ 3];
        v.a = ((uint8_t*)gfx.RDRAM)[(addr + i + 15) ^ 3];

        v.x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
        v.y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
        v.z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
        v.w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

        if (fabs(v.w) < 0.001) v.w = 0.001f;
        v.oow = 1.0f / v.w;
        v.x_w = v.x * v.oow;
        v.y_w = v.y * v.oow;
        v.z_w = v.z * v.oow;

        v.uv_calculated = 0xFFFFFFFF;
        v.screen_translated = 0;
        v.shade_mod = 0;

        v.scr_off = 0;
        if (v.x < -v.w) v.scr_off |= 1;
        if (v.x > v.w) v.scr_off |= 2;
        if (v.y < -v.w) v.scr_off |= 4;
        if (v.y > v.w) v.scr_off |= 8;
        if (v.w < 0.1f) v.scr_off |= 16;
        WriteTrace(TraceRDP, TraceVerbose, "v%d - x: %f, y: %f, z: %f, w: %f, u: %f, v: %f, f: %f, z_w: %f, r=%d, g=%d, b=%d, a=%d", i >> 4, v.x, v.y, v.z, v.w, v.ou*rdp.tiles(rdp.cur_tile).s_scale, v.ov*rdp.tiles(rdp.cur_tile).t_scale, v.f, v.z_w, v.r, v.g, v.b, v.a);
    }
}

static void t3dLoadObject(uint32_t pstate, uint32_t pvtx, uint32_t ptri)
{
    WriteTrace(TraceRDP, TraceDebug, "Loading Turbo3D object");
    t3dState *ostate = (t3dState*)&gfx.RDRAM[segoffset(pstate)];
    rdp.cur_tile = (ostate->textureState) & 7;
    WriteTrace(TraceRDP, TraceDebug, "tile: %d", rdp.cur_tile);
    if (rdp.tiles(rdp.cur_tile).s_scale < 0.001f)
        rdp.tiles(rdp.cur_tile).s_scale = 0.015625;
    if (rdp.tiles(rdp.cur_tile).t_scale < 0.001f)
        rdp.tiles(rdp.cur_tile).t_scale = 0.015625;

    WriteTrace(TraceRDP, TraceVerbose, "renderState: %08lx, textureState: %08lx, othermode0: %08lx, othermode1: %08lx, rdpCmds: %08lx, triCount : %d, v0: %d, vn: %d", ostate->renderState, ostate->textureState,
        ostate->othermode0, ostate->othermode1, ostate->rdpCmds, ostate->triCount, ostate->vtxV0, ostate->vtxCount);

    rdp.cmd0 = ostate->othermode0;
    rdp.cmd1 = ostate->othermode1;
    rdp_setothermode();

    rdp.cmd1 = ostate->renderState;
    uc0_setgeometrymode();

    if (!(ostate->flag & 1)) //load matrix
    {
        uint32_t addr = segoffset(pstate + sizeof(t3dState)) & BMASK;
        load_matrix(rdp.combined, addr);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[0][0], rdp.combined[0][1], rdp.combined[0][2], rdp.combined[0][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[1][0], rdp.combined[1][1], rdp.combined[1][2], rdp.combined[1][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[2][0], rdp.combined[2][1], rdp.combined[2][2], rdp.combined[2][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[3][0], rdp.combined[3][1], rdp.combined[3][2], rdp.combined[3][3]);
    }

    rdp.geom_mode &= ~0x00020000;
    rdp.geom_mode |= 0x00000200;
    if (pvtx) //load vtx
        t3d_vertex(segoffset(pvtx) & BMASK, ostate->vtxV0, ostate->vtxCount);

    t3dProcessRDP(segoffset(ostate->rdpCmds) >> 2);

    if (ptri)
    {
        update();
        uint32_t a = segoffset(ptri);
        for (int t = 0; t < ostate->triCount; t++)
        {
            t3dTriN * tri = (t3dTriN*)&gfx.RDRAM[a];
            a += 4;
            WriteTrace(TraceRDP, TraceDebug, "tri #%d - %d, %d, %d", t, tri->v0, tri->v1, tri->v2);
            VERTEX *vtx[3] = { &rdp.vtx(tri->v0), &rdp.vtx(tri->v1), &rdp.vtx(tri->v2) };
            if (cull_tri(vtx))
                rdp.tri_n++;
            else
            {
                draw_tri(vtx);
                rdp.tri_n++;
            }
        }
    }
}

void Turbo3D()
{
    WriteTrace(TraceRDP, TraceDebug, "Start Turbo3D microcode");
    g_settings->SetUcode(CSettings::ucode_Fast3D);
    uint32_t a = 0, pgstate = 0, pstate = 0, pvtx = 0, ptri = 0;
    do
    {
        a = rdp.pc[rdp.pc_i] & BMASK;
        pgstate = ((uint32_t*)gfx.RDRAM)[a >> 2];
        pstate = ((uint32_t*)gfx.RDRAM)[(a >> 2) + 1];
        pvtx = ((uint32_t*)gfx.RDRAM)[(a >> 2) + 2];
        ptri = ((uint32_t*)gfx.RDRAM)[(a >> 2) + 3];
        WriteTrace(TraceRDP, TraceDebug, "GlobalState: %08lx, Object: %08lx, Vertices: %08lx, Triangles: %08lx", pgstate, pstate, pvtx, ptri);
        if (!pstate)
        {
            rdp.halt = true;
            break;
        }
        if (pgstate)
        {
            t3dLoadGlobState(pgstate);
        }
        t3dLoadObject(pstate, pvtx, ptri);
        // Go to the next instruction
        rdp.pc[rdp.pc_i] += 16;
    } while (pstate);
    g_settings->SetUcode(CSettings::ucode_Turbo3d);
}