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
#include "3dmath.h"

uint32_t pd_col_addr = 0;

void uc7_colorbase()
{
    WriteTrace(TraceRDP, TraceDebug, "uc7_colorbase");
    pd_col_addr = segoffset(rdp.cmd1);
}

typedef struct
{
    short y;
    short x;
    uint16_t idx;

    short z;

    short t;
    short s;
} vtx_uc7;

void uc7_vertex()
{
    if (rdp.update & UPDATE_MULT_MAT)
    {
        rdp.update ^= UPDATE_MULT_MAT;
        MulMatrices(rdp.model, rdp.proj, rdp.combined);
    }

    // This is special, not handled in update()
    if (rdp.update & UPDATE_LIGHTS)
    {
        rdp.update ^= UPDATE_LIGHTS;

        // Calculate light vectors
        for (uint32_t l = 0; l < rdp.num_lights; l++)
        {
            InverseTransformVector(&rdp.light[l].dir_x, rdp.light_vector[l], rdp.model);
            NormalizeVector(rdp.light_vector[l]);
        }
    }

    uint32_t addr = segoffset(rdp.cmd1);
    uint32_t v0, i, n;
    float x, y, z;

    rdp.v0 = v0 = (rdp.cmd0 & 0x0F0000) >> 16;
    rdp.vn = n = ((rdp.cmd0 & 0xF00000) >> 20) + 1;

    WriteTrace(TraceRDP, TraceDebug, "uc7:vertex n: %d, v0: %d, from: %08lx", n, v0, addr);

    vtx_uc7 *vertex = (vtx_uc7 *)&gfx.RDRAM[addr];

    for (i = 0; i < n; i++)
    {
        gfxVERTEX &v = rdp.vtx(v0 + i);
        x = (float)vertex->x;
        y = (float)vertex->y;
        z = (float)vertex->z;
        v.flags = 0;
        v.ou = (float)vertex->s;
        v.ov = (float)vertex->t;
        v.uv_scaled = 0;

        WriteTrace(TraceRDP, TraceVerbose, "before: v%d - x: %f, y: %f, z: %f, flags: %04lx, ou: %f, ov: %f", i >> 4, x, y, z, v.flags, v.ou, v.ov);

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

        v.scr_off = 0;
        if (v.x < -v.w) v.scr_off |= 1;
        if (v.x > v.w) v.scr_off |= 2;
        if (v.y < -v.w) v.scr_off |= 4;
        if (v.y > v.w) v.scr_off |= 8;
        if (v.w < 0.1f) v.scr_off |= 16;

        uint8_t *color = &gfx.RDRAM[pd_col_addr + (vertex->idx & 0xff)];

        v.a = color[0];
        CalculateFog(v);

        if (rdp.geom_mode & 0x00020000)
        {
            v.vec[0] = (char)color[3];
            v.vec[1] = (char)color[2];
            v.vec[2] = (char)color[1];

            if (rdp.geom_mode & 0x80000)
            {
                calc_linear(v);
                WriteTrace(TraceRDP, TraceVerbose, "calc linear: v%d - u: %f, v: %f", i >> 4, v.ou, v.ov);
            }
            else if (rdp.geom_mode & 0x40000)
            {
                calc_sphere(v);
                WriteTrace(TraceRDP, TraceVerbose, "calc sphere: v%d - u: %f, v: %f", i >> 4, v.ou, v.ov);
            }

            NormalizeVector(v.vec);

            calc_light(v);
        }
        else
        {
            v.r = color[3];
            v.g = color[2];
            v.b = color[1];
        }
        WriteTrace(TraceRDP, TraceVerbose, "v%d - x: %f, y: %f, z: %f, w: %f, u: %f, v: %f", i >> 4, v.x, v.y, v.z, v.w, v.ou, v.ov);
        vertex++;
    }
}