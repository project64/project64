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
#include "ucode00.h"

static void calc_point_light(VERTEX & v, float * vpos)
{
    float light_intensity = 0.0f;
    register float color[3] = { rdp.light[rdp.num_lights].r, rdp.light[rdp.num_lights].g, rdp.light[rdp.num_lights].b };
    for (uint32_t l = 0; l < rdp.num_lights; l++)
    {
        if (rdp.light[l].nonblack)
        {
            float lvec[3] = { rdp.light[l].x, rdp.light[l].y, rdp.light[l].z };
            lvec[0] -= vpos[0];
            lvec[1] -= vpos[1];
            lvec[2] -= vpos[2];
            float light_len2 = lvec[0] * lvec[0] + lvec[1] * lvec[1] + lvec[2] * lvec[2];
            float light_len = sqrtf(light_len2);
            WriteTrace(TraceRDP, TraceVerbose, "calc_point_light: len: %f, len2: %f", light_len, light_len2);
            float at = rdp.light[l].ca + light_len / 65535.0f*rdp.light[l].la + light_len2 / 65535.0f*rdp.light[l].qa;
            if (at > 0.0f)
                light_intensity = 1 / at;//DotProduct (lvec, nvec) / (light_len * normal_len * at);
            else
                light_intensity = 0.0f;
        }
        else
        {
            light_intensity = 0.0f;
        }
        if (light_intensity > 0.0f)
        {
            color[0] += rdp.light[l].r * light_intensity;
            color[1] += rdp.light[l].g * light_intensity;
            color[2] += rdp.light[l].b * light_intensity;
        }
    }
    if (color[0] > 1.0f) color[0] = 1.0f;
    if (color[1] > 1.0f) color[1] = 1.0f;
    if (color[2] > 1.0f) color[2] = 1.0f;

    v.r = (uint8_t)(color[0] * 255.0f);
    v.g = (uint8_t)(color[1] * 255.0f);
    v.b = (uint8_t)(color[2] * 255.0f);
}

void uc6_obj_rectangle();

void uc2_vertex()
{
    if (!(rdp.cmd0 & 0x00FFFFFF))
    {
        uc6_obj_rectangle();
        return;
    }

    // This is special, not handled in update(), but here
    // * Matrix Pre-multiplication idea by Gonetz (Gonetz@ngs.ru)
    if (rdp.update & UPDATE_MULT_MAT)
    {
        rdp.update ^= UPDATE_MULT_MAT;
        MulMatrices(rdp.model, rdp.proj, rdp.combined);
    }
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
    int v0, i, n;
    float x, y, z;

    rdp.vn = n = (rdp.cmd0 >> 12) & 0xFF;
    rdp.v0 = v0 = ((rdp.cmd0 >> 1) & 0x7F) - n;

    WriteTrace(TraceRDP, TraceDebug, "uc2:vertex n: %d, v0: %d, from: %08lx", n, v0, addr);

    if (v0 < 0)
    {
        WriteTrace(TraceRDP, TraceWarning, "** ERROR: uc2:vertex v0 < 0");
        return;
    }

    uint32_t geom_mode = rdp.geom_mode;
    if (g_settings->hacks(CSettings::hack_Fzero) && (rdp.geom_mode & 0x40000))
    {
        if (((short*)gfx.RDRAM)[(((addr) >> 1) + 4) ^ 1] || ((short*)gfx.RDRAM)[(((addr) >> 1) + 5) ^ 1])
            rdp.geom_mode ^= 0x40000;
    }
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

        v.x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
        v.y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
        v.z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
        v.w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

        if (fabs(v.w) < 0.001) v.w = 0.001f;
        v.oow = 1.0f / v.w;
        v.x_w = v.x * v.oow;
        v.y_w = v.y * v.oow;
        v.z_w = v.z * v.oow;
        CalculateFog(v);

        v.uv_calculated = 0xFFFFFFFF;
        v.screen_translated = 0;
        v.shade_mod = 0;

        v.scr_off = 0;
        if (v.x < -v.w) v.scr_off |= 1;
        if (v.x > v.w) v.scr_off |= 2;
        if (v.y < -v.w) v.scr_off |= 4;
        if (v.y > v.w) v.scr_off |= 8;
        if (v.w < 0.1f) v.scr_off |= 16;
        //    if (v.z_w > 1.0f) v.scr_off |= 32;

        if (rdp.geom_mode & 0x00020000)
        {
            v.vec[0] = ((char*)gfx.RDRAM)[(addr + i + 12) ^ 3];
            v.vec[1] = ((char*)gfx.RDRAM)[(addr + i + 13) ^ 3];
            v.vec[2] = ((char*)gfx.RDRAM)[(addr + i + 14) ^ 3];
            //	  WriteTrace(TraceRDP, TraceDebug, "Calc light. x: %f, y: %f z: %f", v.vec[0], v.vec[1], v.vec[2]);
            //      if (!(rdp.geom_mode & 0x800000))
            {
                if (rdp.geom_mode & 0x40000)
                {
                    if (rdp.geom_mode & 0x80000)
                    {
                        calc_linear(v);
                        WriteTrace(TraceRDP, TraceVerbose, "calc linear: v%d - u: %f, v: %f", i >> 4, v.ou, v.ov);
                    }
                    else
                    {
                        calc_sphere(v);
                        WriteTrace(TraceRDP, TraceVerbose, "calc sphere: v%d - u: %f, v: %f", i >> 4, v.ou, v.ov);
                    }
                }
            }
            if (rdp.geom_mode & 0x00400000)
            {
                float tmpvec[3] = { x, y, z };
                calc_point_light(v, tmpvec);
            }
            else
            {
                NormalizeVector(v.vec);
                calc_light(v);
            }
        }
        else
        {
            v.r = ((uint8_t*)gfx.RDRAM)[(addr + i + 12) ^ 3];
            v.g = ((uint8_t*)gfx.RDRAM)[(addr + i + 13) ^ 3];
            v.b = ((uint8_t*)gfx.RDRAM)[(addr + i + 14) ^ 3];
        }
        WriteTrace(TraceRDP, TraceVerbose, "v%d - x: %f, y: %f, z: %f, w: %f, u: %f, v: %f, f: %f, z_w: %f, r=%d, g=%d, b=%d, a=%d", i >> 4, v.x, v.y, v.z, v.w, v.ou*rdp.tiles(rdp.cur_tile).s_scale, v.ov*rdp.tiles(rdp.cur_tile).t_scale, v.f, v.z_w, v.r, v.g, v.b, v.a);
    }
    rdp.geom_mode = geom_mode;
}

void uc2_modifyvtx()
{
    uint8_t where = (uint8_t)((rdp.cmd0 >> 16) & 0xFF);
    uint16_t vtx = (uint16_t)((rdp.cmd0 >> 1) & 0xFFFF);

    WriteTrace(TraceRDP, TraceDebug, "uc2:modifyvtx: vtx: %d, where: 0x%02lx, val: %08lx - ", vtx, where, rdp.cmd1);
    uc0_modifyvtx(where, vtx, rdp.cmd1);
}

void uc2_culldl()
{
    uint16_t vStart = (uint16_t)(rdp.cmd0 & 0xFFFF) >> 1;
    uint16_t vEnd = (uint16_t)(rdp.cmd1 & 0xFFFF) >> 1;
    uint32_t cond = 0;
    WriteTrace(TraceRDP, TraceDebug, "uc2:culldl start: %d, end: %d", vStart, vEnd);

    if (vEnd < vStart) return;
    for (uint16_t i = vStart; i <= vEnd; i++)
    {
        VERTEX & v = rdp.vtx(i);
        /*
        // Check if completely off the screen (quick frustrum clipping for 90 FOV)
        if (v.x >= -v.w)
        cond |= 0x01;
        if (v.x <= v.w)
        cond |= 0x02;
        if (v.y >= -v.w)
        cond |= 0x04;
        if (v.y <= v.w)
        cond |= 0x08;
        if (v.w >= 0.1f)
        cond |= 0x10;

        if (cond == 0x1F)
        return;
        //*/

        WriteTrace(TraceRDP, TraceVerbose, " v[%d] = (%02f, %02f, %02f, 0x%02lx)", i, v.x, v.y, v.w, v.scr_off);

        cond |= (~v.scr_off) & 0x1F;
        if (cond == 0x1F)
        {
            return;
        }
    }

    WriteTrace(TraceRDP, TraceDebug, " - ");  // specify that the enddl is not a real command
    uc0_enddl();
}

void uc6_obj_loadtxtr();

void uc2_tri1()
{
    if ((rdp.cmd0 & 0x00FFFFFF) == 0x17)
    {
        uc6_obj_loadtxtr();
        return;
    }
    if (rdp.skip_drawing)
    {
        WriteTrace(TraceRDP, TraceDebug, "uc2:tri1. skipped");
        return;
    }

    WriteTrace(TraceRDP, TraceDebug, "uc2:tri1 #%d - %d, %d, %d", rdp.tri_n,
        ((rdp.cmd0 >> 17) & 0x7F),
        ((rdp.cmd0 >> 9) & 0x7F),
        ((rdp.cmd0 >> 1) & 0x7F));

    VERTEX *vtx[3] = {
        &rdp.vtx((rdp.cmd0 >> 17) & 0x7F),
        &rdp.vtx((rdp.cmd0 >> 9) & 0x7F),
        &rdp.vtx((rdp.cmd0 >> 1) & 0x7F)
    };

    rsp_tri1(vtx);
}

void uc6_obj_ldtx_sprite();
void uc6_obj_ldtx_rect();

void uc2_quad()
{
    if ((rdp.cmd0 & 0x00FFFFFF) == 0x2F)
    {
        uint32_t command = rdp.cmd0 >> 24;
        if (command == 0x6)
        {
            uc6_obj_ldtx_sprite();
            return;
        }
        if (command == 0x7)
        {
            uc6_obj_ldtx_rect();
            return;
        }
    }

    if (rdp.skip_drawing)
    {
        WriteTrace(TraceRDP, TraceDebug, "uc2_quad. skipped");
        return;
    }

    WriteTrace(TraceRDP, TraceDebug, "uc2:quad");

    WriteTrace(TraceRDP, TraceDebug, " #%d, #%d - %d, %d, %d - %d, %d, %d", rdp.tri_n, rdp.tri_n + 1,
        ((rdp.cmd0 >> 17) & 0x7F),
        ((rdp.cmd0 >> 9) & 0x7F),
        ((rdp.cmd0 >> 1) & 0x7F),
        ((rdp.cmd1 >> 17) & 0x7F),
        ((rdp.cmd1 >> 9) & 0x7F),
        ((rdp.cmd1 >> 1) & 0x7F));

    VERTEX *vtx[6] =
    {
        &rdp.vtx((rdp.cmd0 >> 17) & 0x7F),
        &rdp.vtx((rdp.cmd0 >> 9) & 0x7F),
        &rdp.vtx((rdp.cmd0 >> 1) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 17) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 9) & 0x7F),
        &rdp.vtx((rdp.cmd1 >> 1) & 0x7F)
    };

    rsp_tri2(vtx);
}

void uc6_ldtx_rect_r();

void uc2_line3d()
{
    if ((rdp.cmd0 & 0xFF) == 0x2F)
        uc6_ldtx_rect_r();
    else
    {
        WriteTrace(TraceRDP, TraceDebug, "uc2:line3d #%d, #%d - %d, %d", rdp.tri_n, rdp.tri_n + 1,
            (rdp.cmd0 >> 17) & 0x7F,
            (rdp.cmd0 >> 9) & 0x7F);

        VERTEX *vtx[3] =
        {
            &rdp.vtx((rdp.cmd0 >> 17) & 0x7F),
            &rdp.vtx((rdp.cmd0 >> 9) & 0x7F),
            &rdp.vtx((rdp.cmd0 >> 9) & 0x7F)
        };
        uint16_t width = (uint16_t)(rdp.cmd0 + 3) & 0xFF;
        uint32_t cull_mode = (rdp.flags & CULLMASK) >> CULLSHIFT;
        rdp.flags |= CULLMASK;
        rdp.update |= UPDATE_CULL_MODE;
        rsp_tri1(vtx, width);
        rdp.flags ^= CULLMASK;
        rdp.flags |= cull_mode << CULLSHIFT;
        rdp.update |= UPDATE_CULL_MODE;
    }
}

void uc2_special3()
{
    WriteTrace(TraceRDP, TraceDebug, "uc2:special3");
}

void uc2_special2()
{
    WriteTrace(TraceRDP, TraceDebug, "uc2:special2");
}

void uc2_dma_io()
{
    WriteTrace(TraceRDP, TraceDebug, "uc2:dma_io");
}

void uc2_pop_matrix()
{
    WriteTrace(TraceRDP, TraceDebug, "uc2:pop_matrix %08lx, %08lx", rdp.cmd0, rdp.cmd1);

    // Just pop the modelview matrix
    modelview_pop(rdp.cmd1 >> 6);
}

void uc2_geom_mode()
{
    // Switch around some things
    uint32_t clr_mode = (rdp.cmd0 & 0x00DFC9FF) |
        ((rdp.cmd0 & 0x00000600) << 3) |
        ((rdp.cmd0 & 0x00200000) >> 12) | 0xFF000000;
    uint32_t set_mode = (rdp.cmd1 & 0xFFDFC9FF) |
        ((rdp.cmd1 & 0x00000600) << 3) |
        ((rdp.cmd1 & 0x00200000) >> 12);

    WriteTrace(TraceRDP, TraceDebug, "uc2:geom_mode c:%08lx, s:%08lx ", clr_mode, set_mode);

    rdp.geom_mode &= clr_mode;
    rdp.geom_mode |= set_mode;

    WriteTrace(TraceRDP, TraceDebug, "result:%08lx", rdp.geom_mode);

    if (rdp.geom_mode & 0x00000001) // Z-Buffer enable
    {
        if (!(rdp.flags & ZBUF_ENABLED))
        {
            rdp.flags |= ZBUF_ENABLED;
            rdp.update |= UPDATE_ZBUF_ENABLED;
        }
    }
    else
    {
        if ((rdp.flags & ZBUF_ENABLED))
        {
            if (!g_settings->flame_corona() || (rdp.rm != 0x00504341)) //hack for flame's corona
            {
                rdp.flags ^= ZBUF_ENABLED;
            }
            rdp.update |= UPDATE_ZBUF_ENABLED;
        }
    }
    if (rdp.geom_mode & 0x00001000) // Front culling
    {
        if (!(rdp.flags & CULL_FRONT))
        {
            rdp.flags |= CULL_FRONT;
            rdp.update |= UPDATE_CULL_MODE;
        }
    }
    else
    {
        if (rdp.flags & CULL_FRONT)
        {
            rdp.flags ^= CULL_FRONT;
            rdp.update |= UPDATE_CULL_MODE;
        }
    }
    if (rdp.geom_mode & 0x00002000) // Back culling
    {
        if (!(rdp.flags & CULL_BACK))
        {
            rdp.flags |= CULL_BACK;
            rdp.update |= UPDATE_CULL_MODE;
        }
    }
    else
    {
        if (rdp.flags & CULL_BACK)
        {
            rdp.flags ^= CULL_BACK;
            rdp.update |= UPDATE_CULL_MODE;
        }
    }

    //Added by Gonetz
    if (rdp.geom_mode & 0x00010000)      // Fog enable
    {
        if (!(rdp.flags & FOG_ENABLED))
        {
            rdp.flags |= FOG_ENABLED;
            rdp.update |= UPDATE_FOG_ENABLED;
        }
    }
    else
    {
        if (rdp.flags & FOG_ENABLED)
        {
            rdp.flags ^= FOG_ENABLED;
            rdp.update |= UPDATE_FOG_ENABLED;
        }
    }
}

void uc6_obj_rectangle_r();

void uc2_matrix()
{
    if (!(rdp.cmd0 & 0x00FFFFFF))
    {
        uc6_obj_rectangle_r();
        return;
    }
    WriteTrace(TraceRDP, TraceDebug, "uc2:matrix");

    DECLAREALIGN16VAR(m[4][4]);
    load_matrix(m, segoffset(rdp.cmd1));

    uint8_t command = (uint8_t)((rdp.cmd0 ^ 1) & 0xFF);
    switch (command)
    {
    case 0: // modelview mul nopush
        WriteTrace(TraceRDP, TraceDebug, "modelview mul");
        modelview_mul(m);
        break;

    case 1: // modelview mul push
        WriteTrace(TraceRDP, TraceDebug, "modelview mul push");
        modelview_mul_push(m);
        break;

    case 2: // modelview load nopush
        WriteTrace(TraceRDP, TraceDebug, "modelview load");
        modelview_load(m);
        break;

    case 3: // modelview load push
        WriteTrace(TraceRDP, TraceDebug, "modelview load push");
        modelview_load_push(m);
        break;

    case 4: // projection mul nopush
    case 5: // projection mul push, can't push projection
        WriteTrace(TraceRDP, TraceDebug, "projection mul");
        projection_mul(m);
        break;

    case 6: // projection load nopush
    case 7: // projection load push, can't push projection
        WriteTrace(TraceRDP, TraceDebug, "projection load");
        projection_load(m);
        break;

    default:
        WriteTrace(TraceRDP, TraceWarning, "Unknown matrix command, %02lx", command);
    }

    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", m[0][0], m[0][1], m[0][2], m[0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", m[1][0], m[1][1], m[1][2], m[1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", m[2][0], m[2][1], m[2][2], m[2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", m[3][0], m[3][1], m[3][2], m[3][3]);
    WriteTrace(TraceRDP, TraceVerbose, "\nmodel\n{%f,%f,%f,%f}", rdp.model[0][0], rdp.model[0][1], rdp.model[0][2], rdp.model[0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.model[1][0], rdp.model[1][1], rdp.model[1][2], rdp.model[1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.model[2][0], rdp.model[2][1], rdp.model[2][2], rdp.model[2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.model[3][0], rdp.model[3][1], rdp.model[3][2], rdp.model[3][3]);
    WriteTrace(TraceRDP, TraceVerbose, "\nproj\n{%f,%f,%f,%f}", rdp.proj[0][0], rdp.proj[0][1], rdp.proj[0][2], rdp.proj[0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.proj[1][0], rdp.proj[1][1], rdp.proj[1][2], rdp.proj[1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.proj[2][0], rdp.proj[2][1], rdp.proj[2][2], rdp.proj[2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.proj[3][0], rdp.proj[3][1], rdp.proj[3][2], rdp.proj[3][3]);
}

void uc2_moveword()
{
    uint8_t index = (uint8_t)((rdp.cmd0 >> 16) & 0xFF);
    uint16_t offset = (uint16_t)(rdp.cmd0 & 0xFFFF);
    uint32_t data = rdp.cmd1;

    WriteTrace(TraceRDP, TraceDebug, "uc2:moveword ");

    switch (index)
    {
        // NOTE: right now it's assuming that it sets the integer part first.  This could
        //  be easily fixed, but only if i had something to test with.

    case 0x00:  // moveword matrix
    {
        // do matrix pre-mult so it's re-updated next time
        if (rdp.update & UPDATE_MULT_MAT)
        {
            rdp.update ^= UPDATE_MULT_MAT;
            MulMatrices(rdp.model, rdp.proj, rdp.combined);
        }

        if (rdp.cmd0 & 0x20)  // fractional part
        {
            int index_x = (rdp.cmd0 & 0x1F) >> 1;
            int index_y = index_x >> 2;
            index_x &= 3;

            float fpart = (rdp.cmd1 >> 16) / 65536.0f;
            rdp.combined[index_y][index_x] = (float)(int)rdp.combined[index_y][index_x];
            rdp.combined[index_y][index_x] += fpart;

            fpart = (rdp.cmd1 & 0xFFFF) / 65536.0f;
            rdp.combined[index_y][index_x + 1] = (float)(int)rdp.combined[index_y][index_x + 1];
            rdp.combined[index_y][index_x + 1] += fpart;
        }
        else
        {
            int index_x = (rdp.cmd0 & 0x1F) >> 1;
            int index_y = index_x >> 2;
            index_x &= 3;

            rdp.combined[index_y][index_x] = (short)(rdp.cmd1 >> 16);
            rdp.combined[index_y][index_x + 1] = (short)(rdp.cmd1 & 0xFFFF);
        }

        WriteTrace(TraceRDP, TraceDebug, "matrix");
    }
    break;

    case 0x02:
        rdp.num_lights = data / 24;
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
        if ((data&BMASK) < BMASK)
            rdp.segment[(offset >> 2) & 0xF] = data;
    }
    break;

    case 0x08:
    {
        rdp.fog_multiplier = (short)(rdp.cmd1 >> 16);
        rdp.fog_offset = (short)(rdp.cmd1 & 0x0000FFFF);
        WriteTrace(TraceRDP, TraceDebug, "fog: multiplier: %f, offset: %f", rdp.fog_multiplier, rdp.fog_offset);

        //offset must be 0 for move_fog, but it can be non zero in Nushi Zuri 64 - Shiokaze ni Notte
        //low-level display list has setothermode commands in this place, so this is obviously not move_fog.
        if (offset == 0x04)
            rdp.tlut_mode = (data == 0xffffffff) ? 0 : 2;
    }
    break;

    case 0x0a:  // moveword LIGHTCOL
    {
        int n = offset / 24;
        WriteTrace(TraceRDP, TraceDebug, "lightcol light:%d, %08lx", n, data);

        rdp.light[n].r = (float)((data >> 24) & 0xFF) / 255.0f;
        rdp.light[n].g = (float)((data >> 16) & 0xFF) / 255.0f;
        rdp.light[n].b = (float)((data >> 8) & 0xFF) / 255.0f;
        rdp.light[n].a = 255;
    }
    break;

    case 0x0c:
        WriteTrace(TraceRDP, TraceWarning, "uc2:moveword forcemtx - IGNORED");
        break;

    case 0x0e:
        WriteTrace(TraceRDP, TraceDebug, "perspnorm - IGNORED");
        break;

    default:
        WriteTrace(TraceRDP, TraceWarning, "uc2:moveword unknown (index: 0x%08lx, offset 0x%08lx)", index, offset);
    }
}

void uc6_obj_movemem();

void uc2_movemem()
{
    int idx = rdp.cmd0 & 0xFF;
    uint32_t addr = segoffset(rdp.cmd1);
    int ofs = (rdp.cmd0 >> 5) & 0x7F8;

    WriteTrace(TraceRDP, TraceDebug, "uc2:movemem ofs:%d ", ofs);

    switch (idx)
    {
    case 0:
    case 2:
        uc6_obj_movemem();
        break;

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

        WriteTrace(TraceRDP, TraceDebug, "viewport scale(%d, %d, %d), trans(%d, %d, %d), from:%08lx", scale_x, scale_y, scale_z,
            trans_x, trans_y, trans_z, a);
    }
    break;

    case 10:  // LIGHT
    {
        int n = ofs / 24;

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
        if (n > 7) return;

        // Get the data
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
        // ** Thanks to Icepir8 for pointing this out **
        // Lighting must be signed byte instead of byte
        rdp.light[n].dir_x = (float)(((char*)gfx.RDRAM)[(addr + 8) ^ 3]) / 127.0f;
        rdp.light[n].dir_y = (float)(((char*)gfx.RDRAM)[(addr + 9) ^ 3]) / 127.0f;
        rdp.light[n].dir_z = (float)(((char*)gfx.RDRAM)[(addr + 10) ^ 3]) / 127.0f;
        uint32_t a = addr >> 1;
        rdp.light[n].x = (float)(((short*)gfx.RDRAM)[(a + 4) ^ 1]);
        rdp.light[n].y = (float)(((short*)gfx.RDRAM)[(a + 5) ^ 1]);
        rdp.light[n].z = (float)(((short*)gfx.RDRAM)[(a + 6) ^ 1]);
        rdp.light[n].ca = (float)(gfx.RDRAM[(addr + 3) ^ 3]) / 16.0f;
        rdp.light[n].la = (float)(gfx.RDRAM[(addr + 7) ^ 3]);
        rdp.light[n].qa = (float)(gfx.RDRAM[(addr + 14) ^ 3]) / 8.0f;
        WriteTrace(TraceRDP, TraceVerbose, "light: n: %d, pos: x: %f, y: %f, z: %f, ca: %f, la:%f, qa: %f",
            n, rdp.light[n].x, rdp.light[n].y, rdp.light[n].z, rdp.light[n].ca, rdp.light[n].la, rdp.light[n].qa);
        WriteTrace(TraceRDP, TraceDebug, "light: n: %d, r: %.3f, g: %.3f, b: %.3f. dir: x: %.3f, y: %.3f, z: %.3f",
            n, rdp.light[n].r, rdp.light[n].g, rdp.light[n].b,
            rdp.light[n].dir_x, rdp.light[n].dir_y, rdp.light[n].dir_z);
    }
    break;

    case 14:  // matrix
    {
        // do not update the combined matrix!
        rdp.update &= ~UPDATE_MULT_MAT;
        load_matrix(rdp.combined, segoffset(rdp.cmd1));

        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[0][0], rdp.combined[0][1], rdp.combined[0][2], rdp.combined[0][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[1][0], rdp.combined[1][1], rdp.combined[1][2], rdp.combined[1][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[2][0], rdp.combined[2][1], rdp.combined[2][2], rdp.combined[2][3]);
        WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[3][0], rdp.combined[3][1], rdp.combined[3][2], rdp.combined[3][3]);
    }
    break;

    default:
        WriteTrace(TraceRDP, TraceDebug, "uc2:matrix unknown (%d)", idx);
        WriteTrace(TraceRDP, TraceDebug, "** UNKNOWN %d", idx);
    }
}

void uc2_load_ucode()
{
    WriteTrace(TraceRDP, TraceDebug, "uc2:load_ucode");
}

void uc2_rdphalf_2()
{
    WriteTrace(TraceRDP, TraceDebug, "uc2:rdphalf_2");
}

void uc2_dlist_cnt()
{
    uint32_t addr = segoffset(rdp.cmd1) & BMASK;
    int count = rdp.cmd0 & 0x000000FF;
    WriteTrace(TraceRDP, TraceDebug, "dl_count - addr: %08lx, count: %d", addr, count);
    if (addr == 0)
        return;

    if (rdp.pc_i >= 9)
    {
        WriteTrace(TraceRDP, TraceWarning, "** DL stack overflow **");
        return;
    }
    rdp.pc_i++;  // go to the next PC in the stack
    rdp.pc[rdp.pc_i] = addr;  // jump to the address
    rdp.dl_count = count + 1;
}