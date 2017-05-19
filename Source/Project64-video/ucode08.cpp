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
#include "Util.h"

uint32_t uc8_normale_addr = 0;
float uc8_coord_mod[16];

void uc8_vertex()
{
    if (rdp.update & UPDATE_MULT_MAT)
    {
        rdp.update ^= UPDATE_MULT_MAT;
        MulMatrices(rdp.model, rdp.proj, rdp.combined);
    }

    uint32_t addr = segoffset(rdp.cmd1);
    int v0, i, n;
    float x, y, z;

    rdp.vn = n = (rdp.cmd0 >> 12) & 0xFF;
    rdp.v0 = v0 = ((rdp.cmd0 >> 1) & 0x7F) - n;

    WriteTrace(TraceRDP, TraceDebug, "uc8:vertex n: %d, v0: %d, from: %08lx", n, v0, addr);

    if (v0 < 0)
    {
        WriteTrace(TraceRDP, TraceWarning, "** ERROR: uc2:vertex v0 < 0");
        return;
    }
    //*
    // This is special, not handled in update()
    if (rdp.update & UPDATE_LIGHTS)
    {
        rdp.update ^= UPDATE_LIGHTS;

        // Calculate light vectors
        for (uint32_t l = 0; l < rdp.num_lights; l++)
        {
            InverseTransformVector(&rdp.light[l].dir_x, rdp.light_vector[l], rdp.model);
            NormalizeVector(rdp.light_vector[l]);
            WriteTrace(TraceRDP, TraceVerbose, "light_vector[%d] x: %f, y: %f, z: %f", l, rdp.light_vector[l][0], rdp.light_vector[l][1], rdp.light_vector[l][2]);
        }
    }
    //*/
    for (i = 0; i < (n << 4); i += 16)
    {
        VERTEX & v = rdp.vtx(v0 + (i >> 4));
        x = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 0) ^ 1];
        y = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 1) ^ 1];
        z = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 2) ^ 1];
        v.flags = ((uint16_t*)gfx.RDRAM)[(((addr + i) >> 1) + 3) ^ 1];
        v.ou = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 4) ^ 1];
        v.ov = (float)((short*)gfx.RDRAM)[(((addr + i) >> 1) + 5) ^ 1];
        v.uv_scaled = 0;
        v.a = ((uint8_t*)gfx.RDRAM)[(addr + i + 15) ^ 3];

        WriteTrace(TraceRDP, TraceVerbose, "before v%d - x: %f, y: %f, z: %f", i >> 4, x, y, z);
        v.x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
        v.y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
        v.z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
        v.w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

        WriteTrace(TraceRDP, TraceVerbose, "v%d - x: %f, y: %f, z: %f, w: %f, u: %f, v: %f, flags: %d", i >> 4, v.x, v.y, v.z, v.w, v.ou, v.ov, v.flags);

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
        ///*
        v.r = ((uint8_t*)gfx.RDRAM)[(addr + i + 12) ^ 3];
        v.g = ((uint8_t*)gfx.RDRAM)[(addr + i + 13) ^ 3];
        v.b = ((uint8_t*)gfx.RDRAM)[(addr + i + 14) ^ 3];
        WriteTrace(TraceRDP, TraceVerbose, "r: %02lx, g: %02lx, b: %02lx, a: %02lx", v.r, v.g, v.b, v.a);

        if ((rdp.geom_mode & 0x00020000))
        {
            uint32_t shift = v0 << 1;
            v.vec[0] = ((char*)gfx.RDRAM)[(uc8_normale_addr + (i >> 3) + shift + 0) ^ 3];
            v.vec[1] = ((char*)gfx.RDRAM)[(uc8_normale_addr + (i >> 3) + shift + 1) ^ 3];
            v.vec[2] = (char)(v.flags & 0xff);

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
            WriteTrace(TraceRDP, TraceDebug, "v[%d] calc light. r: 0x%02lx, g: 0x%02lx, b: 0x%02lx", i >> 4, v.r, v.g, v.b);
            float color[3] = { rdp.light[rdp.num_lights].r, rdp.light[rdp.num_lights].g, rdp.light[rdp.num_lights].b };
            WriteTrace(TraceRDP, TraceDebug, "ambient light. r: %f, g: %f, b: %f", color[0], color[1], color[2]);
            float light_intensity = 0.0f;
            uint32_t l;
            if (rdp.geom_mode & 0x00400000)
            {
                NormalizeVector(v.vec);
                for (l = 0; l < rdp.num_lights - 1; l++)
                {
                    if (!rdp.light[l].nonblack)
                        continue;
                    light_intensity = DotProduct(rdp.light_vector[l], v.vec);
                    WriteTrace(TraceRDP, TraceDebug, "light %d, intensity : %f", l, light_intensity);
                    if (light_intensity < 0.0f)
                        continue;
                    //*
                    if (rdp.light[l].ca > 0.0f)
                    {
                        float vx = (v.x + uc8_coord_mod[8])*uc8_coord_mod[12] - rdp.light[l].x;
                        float vy = (v.y + uc8_coord_mod[9])*uc8_coord_mod[13] - rdp.light[l].y;
                        float vz = (v.z + uc8_coord_mod[10])*uc8_coord_mod[14] - rdp.light[l].z;
                        float vw = (v.w + uc8_coord_mod[11])*uc8_coord_mod[15] - rdp.light[l].w;
                        float len = (vx*vx + vy*vy + vz*vz + vw*vw) / 65536.0f;
                        float p_i = rdp.light[l].ca / len;
                        if (p_i > 1.0f) p_i = 1.0f;
                        light_intensity *= p_i;
                        WriteTrace(TraceRDP, TraceDebug, "light %d, len: %f, p_intensity : %f", l, len, p_i);
                    }
                    //*/
                    color[0] += rdp.light[l].r * light_intensity;
                    color[1] += rdp.light[l].g * light_intensity;
                    color[2] += rdp.light[l].b * light_intensity;
                    WriteTrace(TraceRDP, TraceDebug, "light %d r: %f, g: %f, b: %f", l, color[0], color[1], color[2]);
                }
                light_intensity = DotProduct(rdp.light_vector[l], v.vec);
                WriteTrace(TraceRDP, TraceDebug, "light %d, intensity : %f", l, light_intensity);
                if (light_intensity > 0.0f)
                {
                    color[0] += rdp.light[l].r * light_intensity;
                    color[1] += rdp.light[l].g * light_intensity;
                    color[2] += rdp.light[l].b * light_intensity;
                }
                WriteTrace(TraceRDP, TraceDebug, "light %d r: %f, g: %f, b: %f", l, color[0], color[1], color[2]);
            }
            else
            {
                for (l = 0; l < rdp.num_lights; l++)
                {
                    if (rdp.light[l].nonblack && rdp.light[l].nonzero)
                    {
                        float vx = (v.x + uc8_coord_mod[8])*uc8_coord_mod[12] - rdp.light[l].x;
                        float vy = (v.y + uc8_coord_mod[9])*uc8_coord_mod[13] - rdp.light[l].y;
                        float vz = (v.z + uc8_coord_mod[10])*uc8_coord_mod[14] - rdp.light[l].z;
                        float vw = (v.w + uc8_coord_mod[11])*uc8_coord_mod[15] - rdp.light[l].w;
                        float len = (vx*vx + vy*vy + vz*vz + vw*vw) / 65536.0f;
                        light_intensity = rdp.light[l].ca / len;
                        if (light_intensity > 1.0f) light_intensity = 1.0f;
                        WriteTrace(TraceRDP, TraceDebug, "light %d, p_intensity : %f", l, light_intensity);
                        color[0] += rdp.light[l].r * light_intensity;
                        color[1] += rdp.light[l].g * light_intensity;
                        color[2] += rdp.light[l].b * light_intensity;
                        //WriteTrace(TraceRDP, TraceDebug, "light %d r: %f, g: %f, b: %f", l, color[0], color[1], color[2]);
                    }
                }
            }
            if (color[0] > 1.0f) color[0] = 1.0f;
            if (color[1] > 1.0f) color[1] = 1.0f;
            if (color[2] > 1.0f) color[2] = 1.0f;
            v.r = (uint8_t)(((float)v.r)*color[0]);
            v.g = (uint8_t)(((float)v.g)*color[1]);
            v.b = (uint8_t)(((float)v.b)*color[2]);
            WriteTrace(TraceRDP, TraceVerbose, "color after light: r: 0x%02lx, g: 0x%02lx, b: 0x%02lx", v.r, v.g, v.b);
        }
    }
}

void uc8_moveword()
{
    uint8_t index = (uint8_t)((rdp.cmd0 >> 16) & 0xFF);
    uint16_t offset = (uint16_t)(rdp.cmd0 & 0xFFFF);
    uint32_t data = rdp.cmd1;

    WriteTrace(TraceRDP, TraceDebug, "uc8:moveword ");

    switch (index)
    {
        // NOTE: right now it's assuming that it sets the integer part first.  This could
        //  be easily fixed, but only if i had something to test with.

    case 0x02:
        rdp.num_lights = (data / 48);
        rdp.update |= UPDATE_LIGHTS;
        WriteTrace(TraceRDP, TraceDebug, "numlights: %d", rdp.num_lights);
        break;

    case 0x04:
        if (offset == 0x04)
        {
            rdp.clip_ratio = sqrt((float)rdp.cmd1);
            rdp.update |= UPDATE_VIEWPORT;
        }
        WriteTrace(TraceRDP, TraceDebug, "mw_clip %08lx, %08lx", rdp.cmd0, rdp.cmd1);
        break;

    case 0x06:  // moveword SEGMENT
    {
        WriteTrace(TraceRDP, TraceDebug, "SEGMENT %08lx -> seg%d", data, offset >> 2);
        rdp.segment[(offset >> 2) & 0xF] = data;
    }
    break;

    case 0x08:
    {
        rdp.fog_multiplier = (short)(rdp.cmd1 >> 16);
        rdp.fog_offset = (short)(rdp.cmd1 & 0x0000FFFF);
        WriteTrace(TraceRDP, TraceDebug, "fog: multiplier: %f, offset: %f", rdp.fog_multiplier, rdp.fog_offset);
    }
    break;

    case 0x0c:

        WriteTrace(TraceRDP, TraceWarning, "uc8:moveword forcemtx - IGNORED");
        break;

    case 0x0e:
        WriteTrace(TraceRDP, TraceDebug, "perspnorm - IGNORED");
        break;

    case 0x10:  // moveword coord mod
    {
        uint8_t n = offset >> 2;

        WriteTrace(TraceRDP, TraceDebug, "coord mod:%d, %08lx", n, data);
        if (rdp.cmd0 & 8)
            return;
        uint32_t idx = (rdp.cmd0 >> 1) & 3;
        uint32_t pos = rdp.cmd0 & 0x30;
        if (pos == 0)
        {
            uc8_coord_mod[0 + idx] = (short)(rdp.cmd1 >> 16);
            uc8_coord_mod[1 + idx] = (short)(rdp.cmd1 & 0xffff);
        }
        else if (pos == 0x10)
        {
            uc8_coord_mod[4 + idx] = (rdp.cmd1 >> 16) / 65536.0f;
            uc8_coord_mod[5 + idx] = (rdp.cmd1 & 0xffff) / 65536.0f;
            uc8_coord_mod[12 + idx] = uc8_coord_mod[0 + idx] + uc8_coord_mod[4 + idx];
            uc8_coord_mod[13 + idx] = uc8_coord_mod[1 + idx] + uc8_coord_mod[5 + idx];
        }
        else if (pos == 0x20)
        {
            uc8_coord_mod[8 + idx] = (short)(rdp.cmd1 >> 16);
            uc8_coord_mod[9 + idx] = (short)(rdp.cmd1 & 0xffff);
            if (idx)
            {
                for (int k = 8; k < 16; k++)
                {
                    WriteTrace(TraceRDP, TraceVerbose, "coord_mod[%d]=%f", k, uc8_coord_mod[k]);
                }
            }
        }
    }
    break;

    default:
        WriteTrace(TraceRDP, TraceWarning, "uc8:moveword unknown (index: 0x%08lx, offset 0x%08lx)", index, offset);
    }
}

void uc8_movemem()
{
    int idx = rdp.cmd0 & 0xFF;
    uint32_t addr = segoffset(rdp.cmd1);
    int ofs = (rdp.cmd0 >> 5) & 0x3FFF;

    WriteTrace(TraceRDP, TraceDebug, "uc8:movemem ofs:%d ", ofs);

    switch (idx)
    {
    case 8:   // VIEWPORT
    {
        uint32_t a = addr >> 1;
        short scale_x = ((short*)gfx.RDRAM)[(a + 0) ^ 1] >> 2;
        short scale_y = ((short*)gfx.RDRAM)[(a + 1) ^ 1] >> 2;
        short scale_z = ((short*)gfx.RDRAM)[(a + 2) ^ 1];
        short trans_x = ((short*)gfx.RDRAM)[(a + 4) ^ 1] >> 2;
        short trans_y = ((short*)gfx.RDRAM)[(a + 5) ^ 1] >> 2;
        short trans_z = ((short*)gfx.RDRAM)[(a + 6) ^ 1];
        rdp.view_scale[0] = scale_x * rdp.scale_x;
        rdp.view_scale[1] = -scale_y * rdp.scale_y;
        rdp.view_scale[2] = 32.0f * scale_z;
        rdp.view_trans[0] = trans_x * rdp.scale_x;
        rdp.view_trans[1] = trans_y * rdp.scale_y;
        rdp.view_trans[2] = 32.0f * trans_z;

        rdp.update |= UPDATE_VIEWPORT;

        WriteTrace(TraceRDP, TraceDebug, "viewport scale(%d, %d), trans(%d, %d), from:%08lx", scale_x, scale_y,
            trans_x, trans_y, a);
    }
    break;

    case 10:  // LIGHT
    {
        int n = (ofs / 48);
        if (n < 2)
        {
            char dir_x = ((char*)gfx.RDRAM)[(addr + 8) ^ 3];
            rdp.lookat[n][0] = (float)(dir_x) / 127.0f;
            char dir_y = ((char*)gfx.RDRAM)[(addr + 9) ^ 3];
            rdp.lookat[n][1] = (float)(dir_y) / 127.0f;
            char dir_z = ((char*)gfx.RDRAM)[(addr + 10) ^ 3];
            rdp.lookat[n][2] = (float)(dir_z) / 127.0f;
            rdp.use_lookat = TRUE;
            if (n == 1)
            {
                if (!dir_x && !dir_y)
                    rdp.use_lookat = FALSE;
            }
            WriteTrace(TraceRDP, TraceDebug, "lookat_%d (%f, %f, %f)", n, rdp.lookat[n][0], rdp.lookat[n][1], rdp.lookat[n][2]);
            return;
        }
        n -= 2;
        uint8_t col = gfx.RDRAM[(addr + 0) ^ 3];
        rdp.light[n].r = (float)col / 255.0f;
        rdp.light[n].nonblack = col;
        col = gfx.RDRAM[(addr + 1) ^ 3];
        rdp.light[n].g = (float)col / 255.0f;
        rdp.light[n].nonblack += col;
        col = gfx.RDRAM[(addr + 2) ^ 3];
        rdp.light[n].b = (float)col / 255.0f;
        rdp.light[n].nonblack += col;
        rdp.light[n].a = 1.0f;
        rdp.light[n].dir_x = (float)(((char*)gfx.RDRAM)[(addr + 8) ^ 3]) / 127.0f;
        rdp.light[n].dir_y = (float)(((char*)gfx.RDRAM)[(addr + 9) ^ 3]) / 127.0f;
        rdp.light[n].dir_z = (float)(((char*)gfx.RDRAM)[(addr + 10) ^ 3]) / 127.0f;
        // **
        uint32_t a = addr >> 1;
        rdp.light[n].x = (float)(((short*)gfx.RDRAM)[(a + 16) ^ 1]);
        rdp.light[n].y = (float)(((short*)gfx.RDRAM)[(a + 17) ^ 1]);
        rdp.light[n].z = (float)(((short*)gfx.RDRAM)[(a + 18) ^ 1]);
        rdp.light[n].w = (float)(((short*)gfx.RDRAM)[(a + 19) ^ 1]);
        rdp.light[n].nonzero = gfx.RDRAM[(addr + 12) ^ 3];
        rdp.light[n].ca = (float)rdp.light[n].nonzero / 16.0f;
        //rdp.light[n].la = rdp.light[n].ca * 1.0f;
        WriteTrace(TraceRDP, TraceVerbose, "light: n: %d, pos: x: %f, y: %f, z: %f, w: %f, ca: %f",
            n, rdp.light[n].x, rdp.light[n].y, rdp.light[n].z, rdp.light[n].w, rdp.light[n].ca);
        WriteTrace(TraceRDP, TraceDebug, "light: n: %d, r: %f, g: %f, b: %f. dir: x: %.3f, y: %.3f, z: %.3f",
            n, rdp.light[n].r, rdp.light[n].g, rdp.light[n].b,
            rdp.light[n].dir_x, rdp.light[n].dir_y, rdp.light[n].dir_z);
        for (int t = 0; t < 24; t++)
        {
            WriteTrace(TraceRDP, TraceVerbose, "light[%d] = 0x%04lx ", t, ((uint16_t*)gfx.RDRAM)[(a + t) ^ 1]);
        }
    }
    break;

    case 14: //Normales
    {
        uc8_normale_addr = segoffset(rdp.cmd1);
        WriteTrace(TraceRDP, TraceVerbose, "Normale - addr: %08lx", uc8_normale_addr);
        int i;
        for (i = 0; i < 32; i++)
        {
            char x = ((char*)gfx.RDRAM)[uc8_normale_addr + ((i << 1) + 0) ^ 3];
            char y = ((char*)gfx.RDRAM)[uc8_normale_addr + ((i << 1) + 1) ^ 3];
            WriteTrace(TraceRDP, TraceVerbose, "#%d x = %d, y = %d", i, x, y);
        }
        uint32_t a = uc8_normale_addr >> 1;
        for (i = 0; i < 32; i++)
        {
            WriteTrace(TraceRDP, TraceVerbose, "n[%d] = 0x%04lx ", i, ((uint16_t*)gfx.RDRAM)[(a + i) ^ 1]);
        }
    }
    break;

    default:
        WriteTrace(TraceRDP, TraceDebug, "uc8:movemem unknown (%d)", idx);
    }
}

void uc8_tri4() //by Gugaman Apr 19 2002
{
    if (rdp.skip_drawing)
    {
        WriteTrace(TraceRDP, TraceDebug, "uc8:tri4. skipped");
        return;
    }

    WriteTrace(TraceRDP, TraceDebug, "uc8:tri4 (#%d - #%d), %d-%d-%d, %d-%d-%d, %d-%d-%d, %d-%d-%d",
        rdp.tri_n,
        rdp.tri_n + 3,
        ((rdp.cmd0 >> 23) & 0x1F),
        ((rdp.cmd0 >> 18) & 0x1F),
        ((((rdp.cmd0 >> 15) & 0x7) << 2) | ((rdp.cmd1 >> 30) & 0x3)),
        ((rdp.cmd0 >> 10) & 0x1F),
        ((rdp.cmd0 >> 5) & 0x1F),
        ((rdp.cmd0 >> 0) & 0x1F),
        ((rdp.cmd1 >> 25) & 0x1F),
        ((rdp.cmd1 >> 20) & 0x1F),
        ((rdp.cmd1 >> 15) & 0x1F),
        ((rdp.cmd1 >> 10) & 0x1F),
        ((rdp.cmd1 >> 5) & 0x1F),
        ((rdp.cmd1 >> 0) & 0x1F));

    VERTEX *v[12] = {
        &rdp.vtx((rdp.cmd0 >> 23) & 0x1F),
        &rdp.vtx((rdp.cmd0 >> 18) & 0x1F),
        &rdp.vtx(((((rdp.cmd0 >> 15) & 0x7) << 2) | ((rdp.cmd1 >> 30) & 0x3))),
        &rdp.vtx((rdp.cmd0 >> 10) & 0x1F),
        &rdp.vtx((rdp.cmd0 >> 5) & 0x1F),
        &rdp.vtx((rdp.cmd0 >> 0) & 0x1F),
        &rdp.vtx((rdp.cmd1 >> 25) & 0x1F),
        &rdp.vtx((rdp.cmd1 >> 20) & 0x1F),
        &rdp.vtx((rdp.cmd1 >> 15) & 0x1F),
        &rdp.vtx((rdp.cmd1 >> 10) & 0x1F),
        &rdp.vtx((rdp.cmd1 >> 5) & 0x1F),
        &rdp.vtx((rdp.cmd1 >> 0) & 0x1F)
    };

    int updated = 0;

    if (cull_tri(v))
        rdp.tri_n++;
    else
    {
        updated = 1;
        update();

        draw_tri(v);
        rdp.tri_n++;
    }

    if (cull_tri(v + 3))
        rdp.tri_n++;
    else
    {
        if (!updated)
        {
            updated = 1;
            update();
        }

        draw_tri(v + 3);
        rdp.tri_n++;
    }

    if (cull_tri(v + 6))
        rdp.tri_n++;
    else
    {
        if (!updated)
        {
            updated = 1;
            update();
        }

        draw_tri(v + 6);
        rdp.tri_n++;
    }

    if (cull_tri(v + 9))
        rdp.tri_n++;
    else
    {
        if (!updated)
        {
            updated = 1;
            update();
        }

        draw_tri(v + 9);
        rdp.tri_n++;
    }
}