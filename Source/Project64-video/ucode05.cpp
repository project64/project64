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
#include <Project64-video/rdp.h>
#include <Project64-video/Gfx_1.3.h>
#include <Project64-video/trace.h>
#include <Project64-video/ucode.h>
#include <math.h>
#include "3dmath.h"
#include "ucode00.h"
#include "util.h"

int cur_mtx = 0;
int billboarding = 0;
int vtx_last = 0;
uint32_t dma_offset_mtx = 0;
uint32_t dma_offset_vtx = 0;

void uc5_dma_offsets()
{
    dma_offset_mtx = rdp.cmd0 & 0x00FFFFFF;
    dma_offset_vtx = rdp.cmd1 & 0x00FFFFFF;
    vtx_last = 0;
    WriteTrace(TraceRDP, TraceDebug, "uc5:dma_offsets - mtx: %08lx, vtx: %08lx", dma_offset_mtx, dma_offset_vtx);
}

void uc5_matrix()
{
    // Use segment offset to get the address
    uint32_t addr = dma_offset_mtx + (segoffset(rdp.cmd1) & BMASK);

    uint8_t n = (uint8_t)((rdp.cmd0 >> 16) & 0xF);
    uint8_t multiply;

    if (n == 0) //DKR
    {
        n = (uint8_t)((rdp.cmd0 >> 22) & 0x3);
        multiply = 0;
    }
    else //JF
    {
        multiply = (uint8_t)((rdp.cmd0 >> 23) & 0x1);
    }

    cur_mtx = n;

    WriteTrace(TraceRDP, TraceDebug, "uc5:matrix - #%d, addr: %08lx", n, addr);

    if (multiply)
    {
        DECLAREALIGN16VAR(m[4][4]);
        load_matrix(m, addr);
        DECLAREALIGN16VAR(m_src[4][4]);
        memcpy(m_src, rdp.dkrproj[0], 64);
        MulMatrices(m, m_src, rdp.dkrproj[n]);
    }
    else
    {
        load_matrix(rdp.dkrproj[n], addr);
    }
    rdp.update |= UPDATE_MULT_MAT;

    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[n][0][0], rdp.dkrproj[n][0][1], rdp.dkrproj[n][0][2], rdp.dkrproj[n][0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[n][1][0], rdp.dkrproj[n][1][1], rdp.dkrproj[n][1][2], rdp.dkrproj[n][1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[n][2][0], rdp.dkrproj[n][2][1], rdp.dkrproj[n][2][2], rdp.dkrproj[n][2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[n][3][0], rdp.dkrproj[n][3][1], rdp.dkrproj[n][3][2], rdp.dkrproj[n][3][3]);

    for (int i = 0; i < 3; i++)
    {
        WriteTrace(TraceRDP, TraceVerbose, "proj %d", i);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[i][0][0], rdp.dkrproj[i][0][1], rdp.dkrproj[i][0][2], rdp.dkrproj[i][0][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[i][1][0], rdp.dkrproj[i][1][1], rdp.dkrproj[i][1][2], rdp.dkrproj[i][1][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[i][2][0], rdp.dkrproj[i][2][1], rdp.dkrproj[i][2][2], rdp.dkrproj[i][2][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.dkrproj[i][3][0], rdp.dkrproj[i][3][1], rdp.dkrproj[i][3][2], rdp.dkrproj[i][3][3]);
    }
}

void uc5_vertex()
{
    uint32_t addr = dma_offset_vtx + (segoffset(rdp.cmd1) & BMASK);

    // | cccc cccc 1111 1??? 0000 0002 2222 2222 | cmd1 = address |
    // c = vtx command
    // 1 = method #1 of getting count
    // 2 = method #2 of getting count
    // ? = unknown, but used
    // 0 = unused

    int n = ((rdp.cmd0 >> 19) & 0x1F);// + 1;
    if (g_settings->hacks(CSettings::hack_Diddy))
        n++;

    if (rdp.cmd0 & 0x00010000)
    {
        if (billboarding)
            vtx_last = 1;
    }
    else
        vtx_last = 0;

    int first = ((rdp.cmd0 >> 9) & 0x1F) + vtx_last;
    WriteTrace(TraceRDP, TraceDebug, "uc5:vertex - addr: %08lx, first: %d, count: %d, matrix: %08lx", addr, first, n, cur_mtx);

    int prj = cur_mtx;

    int start = 0;
    float x, y, z;
    for (int i = first; i < first + n; i++)
    {
        start = (i - first) * 10;
        gfxVERTEX &v = rdp.vtx(i);
        x = (float)((short*)gfx.RDRAM)[(((addr + start) >> 1) + 0) ^ 1];
        y = (float)((short*)gfx.RDRAM)[(((addr + start) >> 1) + 1) ^ 1];
        z = (float)((short*)gfx.RDRAM)[(((addr + start) >> 1) + 2) ^ 1];

        v.x = x*rdp.dkrproj[prj][0][0] + y*rdp.dkrproj[prj][1][0] + z*rdp.dkrproj[prj][2][0] + rdp.dkrproj[prj][3][0];
        v.y = x*rdp.dkrproj[prj][0][1] + y*rdp.dkrproj[prj][1][1] + z*rdp.dkrproj[prj][2][1] + rdp.dkrproj[prj][3][1];
        v.z = x*rdp.dkrproj[prj][0][2] + y*rdp.dkrproj[prj][1][2] + z*rdp.dkrproj[prj][2][2] + rdp.dkrproj[prj][3][2];
        v.w = x*rdp.dkrproj[prj][0][3] + y*rdp.dkrproj[prj][1][3] + z*rdp.dkrproj[prj][2][3] + rdp.dkrproj[prj][3][3];

        if (billboarding)
        {
            v.x += rdp.vtx(0).x;
            v.y += rdp.vtx(0).y;
            v.z += rdp.vtx(0).z;
            v.w += rdp.vtx(0).w;
        }

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
        if (fabs(v.z_w) > 1.0) v.scr_off |= 32;

        v.r = ((uint8_t*)gfx.RDRAM)[(addr + start + 6) ^ 3];
        v.g = ((uint8_t*)gfx.RDRAM)[(addr + start + 7) ^ 3];
        v.b = ((uint8_t*)gfx.RDRAM)[(addr + start + 8) ^ 3];
        v.a = ((uint8_t*)gfx.RDRAM)[(addr + start + 9) ^ 3];
        CalculateFog(v);

        WriteTrace(TraceRDP, TraceVerbose, "v%d - x: %f, y: %f, z: %f, w: %f, z_w: %f, r=%d, g=%d, b=%d, a=%d", i, v.x, v.y, v.z, v.w, v.z_w, v.r, v.g, v.b, v.a);
    }

    vtx_last += n;
}

void uc5_tridma()
{
    vtx_last = 0;    // we've drawn something, so the vertex index needs resetting
    if (rdp.skip_drawing)
        return;

    // | cccc cccc 2222 0000 1111 1111 1111 0000 | cmd1 = address |
    // c = tridma command
    // 1 = method #1 of getting count
    // 2 = method #2 of getting count
    // 0 = unused

    uint32_t addr = segoffset(rdp.cmd1) & BMASK;
    int num = (rdp.cmd0 & 0xFFF0) >> 4;
    //int num = ((rdp.cmd0 & 0x00F00000) >> 20) + 1;  // same thing!
    WriteTrace(TraceRDP, TraceDebug, "uc5:tridma #%d - addr: %08lx, count: %d", rdp.tri_n, addr, num);

    int start, v0, v1, v2, flags;
    for (int i = 0; i < num; i++)
    {
        start = i << 4;
        v0 = gfx.RDRAM[addr + start];
        v1 = gfx.RDRAM[addr + start + 1];
        v2 = gfx.RDRAM[addr + start + 2];

        WriteTrace(TraceRDP, TraceDebug, "tri #%d - %d, %d, %d", rdp.tri_n, v0, v1, v2);

        gfxVERTEX *vtx[3] = {
            &rdp.vtx(v0),
            &rdp.vtx(v1),
            &rdp.vtx(v2)
        };

        flags = gfx.RDRAM[addr + start + 3];

        if (flags & 0x40) { // no cull
            rdp.flags &= ~CULLMASK;
            gfxCullMode(GFX_CULL_DISABLE);
        }
        else {        // front cull
            rdp.flags &= ~CULLMASK;
            if (rdp.view_scale[0] < 0) {
                rdp.flags |= CULL_BACK;   // agh, backwards culling
                gfxCullMode(GFX_CULL_POSITIVE);
            }
            else {
                rdp.flags |= CULL_FRONT;
                gfxCullMode(GFX_CULL_NEGATIVE);
            }
        }
        start += 4;

        vtx[0]->ou = (float)((short*)gfx.RDRAM)[((addr + start) >> 1) + 5] / 32.0f;
        vtx[0]->ov = (float)((short*)gfx.RDRAM)[((addr + start) >> 1) + 4] / 32.0f;
        vtx[1]->ou = (float)((short*)gfx.RDRAM)[((addr + start) >> 1) + 3] / 32.0f;
        vtx[1]->ov = (float)((short*)gfx.RDRAM)[((addr + start) >> 1) + 2] / 32.0f;
        vtx[2]->ou = (float)((short*)gfx.RDRAM)[((addr + start) >> 1) + 1] / 32.0f;
        vtx[2]->ov = (float)((short*)gfx.RDRAM)[((addr + start) >> 1) + 0] / 32.0f;

        vtx[0]->uv_calculated = 0xFFFFFFFF;
        vtx[1]->uv_calculated = 0xFFFFFFFF;
        vtx[2]->uv_calculated = 0xFFFFFFFF;

        if (cull_tri(vtx))
            rdp.tri_n++;
        else
        {
            update();

            draw_tri(vtx);
            rdp.tri_n++;
        }
    }
}

void uc5_dl_in_mem()
{
    uint32_t addr = segoffset(rdp.cmd1) & BMASK;
    int count = (rdp.cmd0 & 0x00FF0000) >> 16;
    WriteTrace(TraceRDP, TraceDebug, "uc5:dl_in_mem - addr: %08lx, count: %d", addr, count);

    if (rdp.pc_i >= 9)
    {
        WriteTrace(TraceRDP, TraceWarning, "** DL stack overflow **");
        return;
    }
    rdp.pc_i++;  // go to the next PC in the stack
    rdp.pc[rdp.pc_i] = addr;  // jump to the address
    rdp.dl_count = count + 1;
}

void uc5_moveword()
{
    WriteTrace(TraceRDP, TraceDebug, "uc5:moveword ");

    // Find which command this is (lowest byte of cmd0)
    switch (rdp.cmd0 & 0xFF)
    {
    case 0x02:  // moveword matrix 2 billboard
        billboarding = (rdp.cmd1 & 1);
        WriteTrace(TraceRDP, TraceDebug, "matrix billboard - %s", str_offon[billboarding]);
        break;

    case 0x04:  // clip (verified same)
        if (((rdp.cmd0 >> 8) & 0xFFFF) == 0x04)
        {
            rdp.clip_ratio = sqrt((float)rdp.cmd1);
            rdp.update |= UPDATE_VIEWPORT;
        }
        WriteTrace(TraceRDP, TraceDebug, "clip %08lx, %08lx", rdp.cmd0, rdp.cmd1);
        break;

    case 0x06:  // segment (verified same)
        WriteTrace(TraceRDP, TraceDebug, "segment: %08lx -> seg%d", rdp.cmd1, (rdp.cmd0 >> 10) & 0x0F);
        rdp.segment[(rdp.cmd0 >> 10) & 0x0F] = rdp.cmd1;
        break;

    case 0x08:
    {
        rdp.fog_multiplier = (short)(rdp.cmd1 >> 16);
        rdp.fog_offset = (short)(rdp.cmd1 & 0x0000FFFF);
        WriteTrace(TraceRDP, TraceDebug, "fog: multiplier: %f, offset: %f", rdp.fog_multiplier, rdp.fog_offset);
        //	  rdp.update |= UPDATE_FOG_ENABLED;
    }
    break;

    case 0x0a:  // moveword matrix select
        cur_mtx = (rdp.cmd1 >> 6) & 3;
        WriteTrace(TraceRDP, TraceDebug, "matrix select - mtx: %d", cur_mtx);
        break;

    default:
        WriteTrace(TraceRDP, TraceDebug, "(unknown) %02lx - IGNORED", rdp.cmd0 & 0xFF);
    }
}

void uc5_setgeometrymode()
{
    WriteTrace(TraceRDP, TraceDebug, "uc0:setgeometrymode %08lx", rdp.cmd1);

    rdp.geom_mode |= rdp.cmd1;

    if (rdp.cmd1 & 0x00000001)  // Z-Buffer enable
    {
        if (!(rdp.flags & ZBUF_ENABLED))
        {
            rdp.flags |= ZBUF_ENABLED;
            rdp.update |= UPDATE_ZBUF_ENABLED;
        }
    }

    //Added by Gonetz
    if (rdp.cmd1 & 0x00010000)      // Fog enable
    {
        if (!(rdp.flags & FOG_ENABLED))
        {
            rdp.flags |= FOG_ENABLED;
            rdp.update |= UPDATE_FOG_ENABLED;
        }
    }
}

void uc5_cleargeometrymode()
{
    WriteTrace(TraceRDP, TraceDebug, "uc0:cleargeometrymode %08lx", rdp.cmd1);

    rdp.geom_mode &= (~rdp.cmd1);

    if (rdp.cmd1 & 0x00000001)  // Z-Buffer enable
    {
        if (rdp.flags & ZBUF_ENABLED)
        {
            rdp.flags ^= ZBUF_ENABLED;
            rdp.update |= UPDATE_ZBUF_ENABLED;
        }
    }
    //Added by Gonetz
    if (rdp.cmd1 & 0x00010000)      // Fog enable
    {
        if (rdp.flags & FOG_ENABLED)
        {
            rdp.flags ^= FOG_ENABLED;
            rdp.update |= UPDATE_FOG_ENABLED;
        }
    }
}