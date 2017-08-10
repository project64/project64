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
#include <math.h>
#include <string.h>

#include "Gfx_1.3.h"
#include "Util.h"
#include "Combine.h"
#include "3dmath.h"
#include "Debugger.h"
#include "TexCache.h"
#include "DepthBufferRender.h"
#include <Project64-video/trace.h>

#define Vj rdp.vtxbuf2[j]
#define Vi rdp.vtxbuf2[i]

VERTEX *vtx_list1[32];  // vertex indexing
VERTEX *vtx_list2[32];

//
// util_init - initialize data for the functions in this file
//

void util_init()
{
    for (int i = 0; i < 32; i++)
    {
        vtx_list1[i] = &rdp.vtx1[i];
        vtx_list2[i] = &rdp.vtx2[i];
    }
}

static uint32_t u_cull_mode = 0;

//software backface culling. Gonetz
// mega modifications by Dave2001
int cull_tri(VERTEX **v) // type changed to VERTEX** [Dave2001]
{
    int i;

    if (v[0]->scr_off & v[1]->scr_off & v[2]->scr_off)
    {
        WriteTrace(TraceRDP, TraceDebug, " clipped");
        return TRUE;
    }

    // Triangle can't be culled, if it need clipping
    int draw = FALSE;

    for (i = 0; i < 3; i++)
    {
        if (!v[i]->screen_translated)
        {
            v[i]->sx = rdp.view_trans[0] + v[i]->x_w * rdp.view_scale[0] + rdp.offset_x;
            v[i]->sy = rdp.view_trans[1] + v[i]->y_w * rdp.view_scale[1] + rdp.offset_y;
            v[i]->sz = rdp.view_trans[2] + v[i]->z_w * rdp.view_scale[2];
            v[i]->screen_translated = 1;
        }
        if (v[i]->w < 0.01f) //need clip_z. can't be culled now
            draw = 1;
    }

    u_cull_mode = (rdp.flags & CULLMASK);
    if (draw || u_cull_mode == 0 || u_cull_mode == CULLMASK) //no culling set
    {
        u_cull_mode >>= CULLSHIFT;
        return FALSE;
    }

#define SW_CULLING
#ifdef SW_CULLING
#if 1 // H.Morii - faster float comparisons with zero area check added

    const float x1 = v[0]->sx - v[1]->sx;
    const float y1 = v[0]->sy - v[1]->sy;
    const float x2 = v[2]->sx - v[1]->sx;
    const float y2 = v[2]->sy - v[1]->sy;
    const float area = y1*x2 - x1*y2;

    const int iarea = *(int*)&area;
    const unsigned int mode = (u_cull_mode << 19UL);
    u_cull_mode >>= CULLSHIFT;

    if ((iarea & 0x7FFFFFFF) == 0)
    {
        WriteTrace(TraceRDP, TraceDebug, " zero area triangles");
        return TRUE;
    }

    if ((rdp.flags & CULLMASK) && ((int)(iarea ^ mode)) >= 0)
    {
        WriteTrace(TraceRDP, TraceDebug, " culled");
        return TRUE;
    }
#else

    float x1 = v[0]->sx - v[1]->sx;
    float y1 = v[0]->sy - v[1]->sy;
    float x2 = v[2]->sx - v[1]->sx;
    float y2 = v[2]->sy - v[1]->sy;

    u_cull_mode >>= CULLSHIFT;
    switch (u_cull_mode)
    {
    case 1: // cull front
        //    if ((x1*y2 - y1*x2) < 0.0f) //counter-clockwise, positive
        if ((y1*x2 - x1*y2) < 0.0f) //counter-clockwise, positive
        {
            WriteTrace(TraceRDP, TraceDebug, " culled!");
            return TRUE;
        }
        return FALSE;
    case 2: // cull back
        //    if ((x1*y2 - y1*x2) >= 0.0f) //clockwise, negative
        if ((y1*x2 - x1*y2) >= 0.0f) //clockwise, negative
        {
            WriteTrace(TraceRDP, TraceDebug, " culled!");
            return TRUE;
        }
        return FALSE;
    }
#endif
#endif

    return FALSE;
}

void apply_shade_mods(VERTEX *v)
{
    float col[4];
    uint32_t mod;
    memcpy(col, rdp.col, 16);

    if (rdp.cmb_flags)
    {
        if (v->shade_mod == 0)
            v->color_backup = *(uint32_t*)(&(v->b));
        else
            *(uint32_t*)(&(v->b)) = v->color_backup;
        mod = rdp.cmb_flags;
        if (mod & CMB_SET)
        {
            if (col[0] > 1.0f) col[0] = 1.0f;
            if (col[1] > 1.0f) col[1] = 1.0f;
            if (col[2] > 1.0f) col[2] = 1.0f;
            if (col[0] < 0.0f) col[0] = 0.0f;
            if (col[1] < 0.0f) col[1] = 0.0f;
            if (col[2] < 0.0f) col[2] = 0.0f;
            v->r = (uint8_t)(255.0f * col[0]);
            v->g = (uint8_t)(255.0f * col[1]);
            v->b = (uint8_t)(255.0f * col[2]);
        }
        if (mod & CMB_A_SET)
        {
            if (col[3] > 1.0f) col[3] = 1.0f;
            if (col[3] < 0.0f) col[3] = 0.0f;
            v->a = (uint8_t)(255.0f * col[3]);
        }
        if (mod & CMB_SETSHADE_SHADEALPHA)
        {
            v->r = v->g = v->b = v->a;
        }
        if (mod & CMB_MULT_OWN_ALPHA)
        {
            float percent = v->a / 255.0f;
            v->r = (uint8_t)(v->r * percent);
            v->g = (uint8_t)(v->g * percent);
            v->b = (uint8_t)(v->b * percent);
        }
        if (mod & CMB_MULT)
        {
            if (col[0] > 1.0f) col[0] = 1.0f;
            if (col[1] > 1.0f) col[1] = 1.0f;
            if (col[2] > 1.0f) col[2] = 1.0f;
            if (col[0] < 0.0f) col[0] = 0.0f;
            if (col[1] < 0.0f) col[1] = 0.0f;
            if (col[2] < 0.0f) col[2] = 0.0f;
            v->r = (uint8_t)(v->r * col[0]);
            v->g = (uint8_t)(v->g * col[1]);
            v->b = (uint8_t)(v->b * col[2]);
        }
        if (mod & CMB_A_MULT)
        {
            if (col[3] > 1.0f) col[3] = 1.0f;
            if (col[3] < 0.0f) col[3] = 0.0f;
            v->a = (uint8_t)(v->a * col[3]);
        }
        if (mod & CMB_SUB)
        {
            int r = v->r - (int)(255.0f * rdp.coladd[0]);
            int g = v->g - (int)(255.0f * rdp.coladd[1]);
            int b = v->b - (int)(255.0f * rdp.coladd[2]);
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            v->r = (uint8_t)r;
            v->g = (uint8_t)g;
            v->b = (uint8_t)b;
        }
        if (mod & CMB_A_SUB)
        {
            int a = v->a - (int)(255.0f * rdp.coladd[3]);
            if (a < 0) a = 0;
            v->a = (uint8_t)a;
        }
        if (mod & CMB_ADD)
        {
            int r = v->r + (int)(255.0f * rdp.coladd[0]);
            int g = v->g + (int)(255.0f * rdp.coladd[1]);
            int b = v->b + (int)(255.0f * rdp.coladd[2]);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            v->r = (uint8_t)r;
            v->g = (uint8_t)g;
            v->b = (uint8_t)b;
        }
        if (mod & CMB_A_ADD)
        {
            int a = v->a + (int)(255.0f * rdp.coladd[3]);
            if (a > 255) a = 255;
            v->a = (uint8_t)a;
        }
        if (mod & CMB_COL_SUB_OWN)
        {
            int r = (uint8_t)(255.0f * rdp.coladd[0]) - v->r;
            int g = (uint8_t)(255.0f * rdp.coladd[1]) - v->g;
            int b = (uint8_t)(255.0f * rdp.coladd[2]) - v->b;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            v->r = (uint8_t)r;
            v->g = (uint8_t)g;
            v->b = (uint8_t)b;
        }
        v->shade_mod = cmb.shade_mod_hash;
    }
    if (rdp.cmb_flags_2 & CMB_INTER)
    {
        v->r = (uint8_t)(rdp.col_2[0] * rdp.shade_factor * 255.0f + v->r * (1.0f - rdp.shade_factor));
        v->g = (uint8_t)(rdp.col_2[1] * rdp.shade_factor * 255.0f + v->g * (1.0f - rdp.shade_factor));
        v->b = (uint8_t)(rdp.col_2[2] * rdp.shade_factor * 255.0f + v->b * (1.0f - rdp.shade_factor));
        v->shade_mod = cmb.shade_mod_hash;
    }
}

static int dzdx = 0;
static int deltaZ = 0;
VERTEX **org_vtx;

void draw_tri(VERTEX **vtx, uint16_t linew)
{
    deltaZ = dzdx = 0;
    if (linew == 0 && (g_settings->fb_depth_render_enabled() || (rdp.rm & 0xC00) == 0xC00))
    {
        double X0 = vtx[0]->sx / rdp.scale_x;
        double Y0 = vtx[0]->sy / rdp.scale_y;
        double X1 = vtx[1]->sx / rdp.scale_x;
        double Y1 = vtx[1]->sy / rdp.scale_y;
        double X2 = vtx[2]->sx / rdp.scale_x;
        double Y2 = vtx[2]->sy / rdp.scale_y;
        double diffy_02 = Y0 - Y2;
        double diffy_12 = Y1 - Y2;
        double diffx_02 = X0 - X2;
        double diffx_12 = X1 - X2;

        double denom = (diffx_02 * diffy_12 - diffx_12 * diffy_02);
        if (denom*denom > 0.0)
        {
            double diffz_02 = vtx[0]->sz - vtx[2]->sz;
            double diffz_12 = vtx[1]->sz - vtx[2]->sz;
            double fdzdx = (diffz_02 * diffy_12 - diffz_12 * diffy_02) / denom;
            if ((rdp.rm & 0xC00) == 0xC00)
            {
                // Calculate deltaZ per polygon for Decal z-mode
                double fdzdy = (diffz_02 * diffx_12 - diffz_12 * diffx_02) / denom;
                double fdz = fabs(fdzdx) + fabs(fdzdy);
                if (g_settings->hacks(CSettings::hack_Zelda) && (rdp.rm & 0x800))
                {
                    fdz *= 4.0;  // Decal mode in Zelda sometimes needs mutiplied deltaZ to work correct, e.g. roads
                }
                deltaZ = maxval(8, (int)fdz);
            }
            dzdx = (int)(fdzdx * 65536.0);
        }
    }

    org_vtx = vtx;

    for (int i = 0; i < 3; i++)
    {
        VERTEX *v = vtx[i];

        if (v->uv_calculated != rdp.tex_ctr)
        {
            WriteTrace(TraceRDP, TraceVerbose, " * CALCULATING VERTEX U/V: %d", v->number);
            v->uv_calculated = rdp.tex_ctr;

            if (!(rdp.geom_mode & 0x00020000))
            {
                if (!(rdp.geom_mode & 0x00000200))
                {
                    if (rdp.geom_mode & 0x00000004) // flat shading
                    {
                        int flag = minval(2, (rdp.cmd1 >> 24) & 3);
                        v->a = vtx[flag]->a;
                        v->b = vtx[flag]->b;
                        v->g = vtx[flag]->g;
                        v->r = vtx[flag]->r;
                        WriteTrace(TraceRDP, TraceVerbose, " * Flat shaded, flag%d - r: %d, g: %d, b: %d, a: %d", flag, v->r, v->g, v->b, v->a);
                    }
                    else  // prim color
                    {
                        WriteTrace(TraceRDP, TraceVerbose, " * Prim shaded %08lx", rdp.prim_color);
                        v->a = (uint8_t)(rdp.prim_color & 0xFF);
                        v->b = (uint8_t)((rdp.prim_color >> 8) & 0xFF);
                        v->g = (uint8_t)((rdp.prim_color >> 16) & 0xFF);
                        v->r = (uint8_t)((rdp.prim_color >> 24) & 0xFF);
                    }
                }
            }

            // Fix texture coordinates
            if (!v->uv_scaled)
            {
                v->ou *= rdp.tiles(rdp.cur_tile).s_scale;
                v->ov *= rdp.tiles(rdp.cur_tile).t_scale;
                v->uv_scaled = 1;
                if (!rdp.Persp_en)
                {
                    //          v->oow = v->w = 1.0f;
                    v->ou *= 0.5f;
                    v->ov *= 0.5f;
                }
            }
            v->u1 = v->u0 = v->ou;
            v->v1 = v->v0 = v->ov;

            if (rdp.tex >= 1 && rdp.cur_cache[0])
            {
                if (rdp.aTBuffTex[0])
                {
                    v->u0 += rdp.aTBuffTex[0]->u_shift + rdp.aTBuffTex[0]->tile_uls;
                    v->v0 += rdp.aTBuffTex[0]->v_shift + rdp.aTBuffTex[0]->tile_ult;
                }

                if (rdp.tiles(rdp.cur_tile).shift_s)
                {
                    if (rdp.tiles(rdp.cur_tile).shift_s > 10)
                        v->u0 *= (float)(1 << (16 - rdp.tiles(rdp.cur_tile).shift_s));
                    else
                        v->u0 /= (float)(1 << rdp.tiles(rdp.cur_tile).shift_s);
                }
                if (rdp.tiles(rdp.cur_tile).shift_t)
                {
                    if (rdp.tiles(rdp.cur_tile).shift_t > 10)
                        v->v0 *= (float)(1 << (16 - rdp.tiles(rdp.cur_tile).shift_t));
                    else
                        v->v0 /= (float)(1 << rdp.tiles(rdp.cur_tile).shift_t);
                }

                if (rdp.aTBuffTex[0])
                {
                    if (rdp.aTBuffTex[0]->tile_uls != (int)rdp.tiles(rdp.cur_tile).f_ul_s)
                        v->u0 -= rdp.tiles(rdp.cur_tile).f_ul_s;
                    if (rdp.aTBuffTex[0]->tile_ult != (int)rdp.tiles(rdp.cur_tile).f_ul_t || g_settings->hacks(CSettings::hack_Megaman))
                        v->v0 -= rdp.tiles(rdp.cur_tile).f_ul_t; //required for megaman (boss special attack)
                    v->u0 *= rdp.aTBuffTex[0]->u_scale;
                    v->v0 *= rdp.aTBuffTex[0]->v_scale;
                    WriteTrace(TraceRDP, TraceVerbose, "tbuff_tex t0: (%f, %f)->(%f, %f)", v->ou, v->ov, v->u0, v->v0);
                }
                else
                {
                    v->u0 -= rdp.tiles(rdp.cur_tile).f_ul_s;
                    v->v0 -= rdp.tiles(rdp.cur_tile).f_ul_t;
                    v->u0 = rdp.cur_cache[0]->c_off + rdp.cur_cache[0]->c_scl_x * v->u0;
                    v->v0 = rdp.cur_cache[0]->c_off + rdp.cur_cache[0]->c_scl_y * v->v0;
                }
                v->u0_w = v->u0 / v->w;
                v->v0_w = v->v0 / v->w;
            }

            if (rdp.tex >= 2 && rdp.cur_cache[1])
            {
                if (rdp.aTBuffTex[1])
                {
                    v->u1 += rdp.aTBuffTex[1]->u_shift + rdp.aTBuffTex[1]->tile_uls;
                    v->v1 += rdp.aTBuffTex[1]->v_shift + rdp.aTBuffTex[1]->tile_ult;
                }
                if (rdp.tiles(rdp.cur_tile + 1).shift_s)
                {
                    if (rdp.tiles(rdp.cur_tile + 1).shift_s > 10)
                        v->u1 *= (float)(1 << (16 - rdp.tiles(rdp.cur_tile + 1).shift_s));
                    else
                        v->u1 /= (float)(1 << rdp.tiles(rdp.cur_tile + 1).shift_s);
                }
                if (rdp.tiles(rdp.cur_tile + 1).shift_t)
                {
                    if (rdp.tiles(rdp.cur_tile + 1).shift_t > 10)
                        v->v1 *= (float)(1 << (16 - rdp.tiles(rdp.cur_tile + 1).shift_t));
                    else
                        v->v1 /= (float)(1 << rdp.tiles(rdp.cur_tile + 1).shift_t);
                }

                if (rdp.aTBuffTex[1])
                {
                    if (rdp.aTBuffTex[1]->tile_uls != (int)rdp.tiles(rdp.cur_tile).f_ul_s)
                        v->u1 -= rdp.tiles(rdp.cur_tile).f_ul_s;
                    v->u1 *= rdp.aTBuffTex[1]->u_scale;
                    v->v1 *= rdp.aTBuffTex[1]->v_scale;
                    WriteTrace(TraceRDP, TraceVerbose, "tbuff_tex t1: (%f, %f)->(%f, %f)", v->ou, v->ov, v->u1, v->v1);
                }
                else
                {
                    v->u1 -= rdp.tiles(rdp.cur_tile + 1).f_ul_s;
                    v->v1 -= rdp.tiles(rdp.cur_tile + 1).f_ul_t;
                    v->u1 = rdp.cur_cache[1]->c_off + rdp.cur_cache[1]->c_scl_x * v->u1;
                    v->v1 = rdp.cur_cache[1]->c_off + rdp.cur_cache[1]->c_scl_y * v->v1;
                }

                v->u1_w = v->u1 / v->w;
                v->v1_w = v->v1 / v->w;
            }
            //      WriteTrace(TraceRDP, TraceDebug, " * CALCULATING VERTEX U/V: %d  u0: %f, v0: %f, u1: %f, v1: %f", v->number, v->u0, v->v0, v->u1, v->v1);
        }
        WriteTrace(TraceRDP, TraceVerbose, "draw_tri. v[%d] ou=%f, ov = %f", i, v->ou, v->ov);
        if (v->shade_mod != cmb.shade_mod_hash)
            apply_shade_mods(v);
    } //for

    rdp.clip = 0;

    if ((vtx[0]->scr_off & 16) ||
        (vtx[1]->scr_off & 16) ||
        (vtx[2]->scr_off & 16))
        rdp.clip |= CLIP_WMIN;

    vtx[0]->not_zclipped = vtx[1]->not_zclipped = vtx[2]->not_zclipped = 1;

    if (rdp.cur_cache[0] && (rdp.tex & 1) && (rdp.cur_cache[0]->splits > 1) && !rdp.aTBuffTex[0] && !rdp.clip)
    {
        int index, i, j, min_256, max_256, cur_256, left_256, right_256;
        float percent;

        min_256 = minval((int)vtx[0]->u0, (int)vtx[1]->u0); // bah, don't put two mins on one line
        min_256 = minval(min_256, (int)vtx[2]->u0) >> 8;  // or it will be calculated twice

        max_256 = maxval((int)vtx[0]->u0, (int)vtx[1]->u0); // not like it makes much difference
        max_256 = maxval(max_256, (int)vtx[2]->u0) >> 8;  // anyway :P

        for (cur_256 = min_256; cur_256 <= max_256; cur_256++)
        {
            left_256 = cur_256 << 8;
            right_256 = (cur_256 + 1) << 8;

            // Set vertex buffers
            rdp.vtxbuf = rdp.vtx1;  // copy from v to rdp.vtx1
            rdp.vtxbuf2 = rdp.vtx2;
            rdp.vtx_buffer = 0;
            rdp.n_global = 3;
            index = 0;

            // ** Left plane **
            for (i = 0; i < 3; i++)
            {
                j = i + 1;
                if (j == 3) j = 0;

                VERTEX *v1 = vtx[i];
                VERTEX *v2 = vtx[j];

                if (v1->u0 >= left_256)
                {
                    if (v2->u0 >= left_256)   // Both are in, save the last one
                    {
                        rdp.vtxbuf[index] = *v2;
                        rdp.vtxbuf[index].u0 -= left_256;
                        rdp.vtxbuf[index++].v0 += cur_256 * rdp.cur_cache[0]->splitheight;
                    }
                    else      // First is in, second is out, save intersection
                    {
                        percent = (left_256 - v1->u0) / (v2->u0 - v1->u0);
                        rdp.vtxbuf[index].x = v1->x + (v2->x - v1->x) * percent;
                        rdp.vtxbuf[index].y = v1->y + (v2->y - v1->y) * percent;
                        rdp.vtxbuf[index].z = v1->z + (v2->z - v1->z) * percent;
                        rdp.vtxbuf[index].w = v1->w + (v2->w - v1->w) * percent;
                        rdp.vtxbuf[index].f = v1->f + (v2->f - v1->f) * percent;
                        rdp.vtxbuf[index].u0 = 0.5f;
                        rdp.vtxbuf[index].v0 = v1->v0 + (v2->v0 - v1->v0) * percent +
                            cur_256 * rdp.cur_cache[0]->splitheight;
                        rdp.vtxbuf[index].u1 = v1->u1 + (v2->u1 - v1->u1) * percent;
                        rdp.vtxbuf[index].v1 = v1->v1 + (v2->v1 - v1->v1) * percent;
                        rdp.vtxbuf[index].b = (uint8_t)(v1->b + (v2->b - v1->b) * percent);
                        rdp.vtxbuf[index].g = (uint8_t)(v1->g + (v2->g - v1->g) * percent);
                        rdp.vtxbuf[index].r = (uint8_t)(v1->r + (v2->r - v1->r) * percent);
                        rdp.vtxbuf[index++].a = (uint8_t)(v1->a + (v2->a - v1->a) * percent);
                    }
                }
                else
                {
                    //if (v2->u0 < left_256)  // Both are out, save nothing
                    if (v2->u0 >= left_256) // First is out, second is in, save intersection & in point
                    {
                        percent = (left_256 - v2->u0) / (v1->u0 - v2->u0);
                        rdp.vtxbuf[index].x = v2->x + (v1->x - v2->x) * percent;
                        rdp.vtxbuf[index].y = v2->y + (v1->y - v2->y) * percent;
                        rdp.vtxbuf[index].z = v2->z + (v1->z - v2->z) * percent;
                        rdp.vtxbuf[index].w = v2->w + (v1->w - v2->w) * percent;
                        rdp.vtxbuf[index].f = v2->f + (v1->f - v2->f) * percent;
                        rdp.vtxbuf[index].u0 = 0.5f;
                        rdp.vtxbuf[index].v0 = v2->v0 + (v1->v0 - v2->v0) * percent +
                            cur_256 * rdp.cur_cache[0]->splitheight;
                        rdp.vtxbuf[index].u1 = v2->u1 + (v1->u1 - v2->u1) * percent;
                        rdp.vtxbuf[index].v1 = v2->v1 + (v1->v1 - v2->v1) * percent;
                        rdp.vtxbuf[index].b = (uint8_t)(v2->b + (v1->b - v2->b) * percent);
                        rdp.vtxbuf[index].g = (uint8_t)(v2->g + (v1->g - v2->g) * percent);
                        rdp.vtxbuf[index].r = (uint8_t)(v2->r + (v1->r - v2->r) * percent);
                        rdp.vtxbuf[index++].a = (uint8_t)(v2->a + (v1->a - v2->a) * percent);

                        // Save the in point
                        rdp.vtxbuf[index] = *v2;
                        rdp.vtxbuf[index].u0 -= left_256;
                        rdp.vtxbuf[index++].v0 += cur_256 * rdp.cur_cache[0]->splitheight;
                    }
                }
            }
            rdp.n_global = index;

            rdp.vtxbuf = rdp.vtx2;  // now vtx1 holds the value, & vtx2 is the destination
            rdp.vtxbuf2 = rdp.vtx1;
            rdp.vtx_buffer ^= 1;
            index = 0;

            for (i = 0; i < rdp.n_global; i++)
            {
                j = i + 1;
                if (j == rdp.n_global) j = 0;

                VERTEX *v1 = &rdp.vtxbuf2[i];
                VERTEX *v2 = &rdp.vtxbuf2[j];

                // ** Right plane **
                if (v1->u0 <= right_256)
                {
                    if (v2->u0 <= right_256)   // Both are in, save the last one
                    {
                        rdp.vtxbuf[index] = *v2;
                        rdp.vtxbuf[index++].not_zclipped = 0;
                    }
                    else      // First is in, second is out, save intersection
                    {
                        percent = (right_256 - v1->u0) / (v2->u0 - v1->u0);
                        rdp.vtxbuf[index].x = v1->x + (v2->x - v1->x) * percent;
                        rdp.vtxbuf[index].y = v1->y + (v2->y - v1->y) * percent;
                        rdp.vtxbuf[index].z = v1->z + (v2->z - v1->z) * percent;
                        rdp.vtxbuf[index].w = v1->w + (v2->w - v1->w) * percent;
                        rdp.vtxbuf[index].f = v1->f + (v2->f - v1->f) * percent;
                        rdp.vtxbuf[index].u0 = 255.5f;
                        rdp.vtxbuf[index].v0 = v1->v0 + (v2->v0 - v1->v0) * percent;
                        rdp.vtxbuf[index].u1 = v1->u1 + (v2->u1 - v1->u1) * percent;
                        rdp.vtxbuf[index].v1 = v1->v1 + (v2->v1 - v1->v1) * percent;
                        rdp.vtxbuf[index].b = (uint8_t)(v1->b + (v2->b - v1->b) * percent);
                        rdp.vtxbuf[index].g = (uint8_t)(v1->g + (v2->g - v1->g) * percent);
                        rdp.vtxbuf[index].r = (uint8_t)(v1->r + (v2->r - v1->r) * percent);
                        rdp.vtxbuf[index].a = (uint8_t)(v1->a + (v2->a - v1->a) * percent);
                        rdp.vtxbuf[index++].not_zclipped = 0;
                    }
                }
                else
                {
                    //if (v2->u0 > 256.0f)  // Both are out, save nothing
                    if (v2->u0 <= right_256) // First is out, second is in, save intersection & in point
                    {
                        percent = (right_256 - v2->u0) / (v1->u0 - v2->u0);
                        rdp.vtxbuf[index].x = v2->x + (v1->x - v2->x) * percent;
                        rdp.vtxbuf[index].y = v2->y + (v1->y - v2->y) * percent;
                        rdp.vtxbuf[index].z = v2->z + (v1->z - v2->z) * percent;
                        rdp.vtxbuf[index].w = v2->w + (v1->w - v2->w) * percent;
                        rdp.vtxbuf[index].f = v2->f + (v1->f - v2->f) * percent;
                        rdp.vtxbuf[index].u0 = 255.5f;
                        rdp.vtxbuf[index].v0 = v2->v0 + (v1->v0 - v2->v0) * percent;
                        rdp.vtxbuf[index].u1 = v2->u1 + (v1->u1 - v2->u1) * percent;
                        rdp.vtxbuf[index].v1 = v2->v1 + (v1->v1 - v2->v1) * percent;
                        rdp.vtxbuf[index].b = (uint8_t)(v2->b + (v1->b - v2->b) * percent);
                        rdp.vtxbuf[index].g = (uint8_t)(v2->g + (v1->g - v2->g) * percent);
                        rdp.vtxbuf[index].r = (uint8_t)(v2->r + (v1->r - v2->r) * percent);
                        rdp.vtxbuf[index].a = (uint8_t)(v2->a + (v1->a - v2->a) * percent);
                        rdp.vtxbuf[index++].not_zclipped = 0;

                        // Save the in point
                        rdp.vtxbuf[index] = *v2;
                        rdp.vtxbuf[index++].not_zclipped = 0;
                    }
                }
            }
            rdp.n_global = index;

            do_triangle_stuff(linew, TRUE);
        }
    }
    else
    {
        // Set vertex buffers
        rdp.vtxbuf = rdp.vtx1;  // copy from v to rdp.vtx1
        rdp.vtxbuf2 = rdp.vtx2;
        rdp.vtx_buffer = 0;
        rdp.n_global = 3;

        rdp.vtxbuf[0] = *vtx[0];
        rdp.vtxbuf[0].number = 1;
        rdp.vtxbuf[1] = *vtx[1];
        rdp.vtxbuf[1].number = 2;
        rdp.vtxbuf[2] = *vtx[2];
        rdp.vtxbuf[2].number = 4;

        do_triangle_stuff(linew, FALSE);
    }
}

#define interp2p(a, b, r)  (a + (b - a) * r)

//*
static void InterpolateColors(VERTEX & va, VERTEX & vb, VERTEX & res, float percent)
{
    res.b = (uint8_t)interp2p(va.b, vb.b, percent);
    res.g = (uint8_t)interp2p(va.g, vb.g, percent);;
    res.r = (uint8_t)interp2p(va.r, vb.r, percent);;
    res.a = (uint8_t)interp2p(va.a, vb.a, percent);;
    res.f = interp2p(va.f, vb.f, percent);;
}
//*/
//
// clip_w - clips aint the z-axis
//
static void clip_w(int interpolate_colors)
{
    int i, j, index, n = rdp.n_global;
    float percent;
    // Swap vertex buffers
    VERTEX *tmp = rdp.vtxbuf2;
    rdp.vtxbuf2 = rdp.vtxbuf;
    rdp.vtxbuf = tmp;
    rdp.vtx_buffer ^= 1;
    index = 0;

    // Check the vertices for clipping
    for (i = 0; i < n; i++)
    {
        j = i + 1;
        if (j == 3) j = 0;

        if (Vi.w >= 0.01f)
        {
            if (Vj.w >= 0.01f)    // Both are in, save the last one
            {
                rdp.vtxbuf[index] = Vj;
                rdp.vtxbuf[index++].not_zclipped = 1;
            }
            else      // First is in, second is out, save intersection
            {
                percent = (-Vi.w) / (Vj.w - Vi.w);
                rdp.vtxbuf[index].not_zclipped = 0;
                rdp.vtxbuf[index].x = Vi.x + (Vj.x - Vi.x) * percent;
                rdp.vtxbuf[index].y = Vi.y + (Vj.y - Vi.y) * percent;
                rdp.vtxbuf[index].z = Vi.z + (Vj.z - Vi.z) * percent;
                rdp.vtxbuf[index].w = 0.01f;
                rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
                rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
                rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
                rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
                if (interpolate_colors)
                    InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
                else
                    rdp.vtxbuf[index++].number = Vi.number | Vj.number;
            }
        }
        else
        {
            //if (Vj.w < 0.01f) // Both are out, save nothing
            if (Vj.w >= 0.01f)  // First is out, second is in, save intersection & in point
            {
                percent = (-Vj.w) / (Vi.w - Vj.w);
                rdp.vtxbuf[index].not_zclipped = 0;
                rdp.vtxbuf[index].x = Vj.x + (Vi.x - Vj.x) * percent;
                rdp.vtxbuf[index].y = Vj.y + (Vi.y - Vj.y) * percent;
                rdp.vtxbuf[index].z = Vj.z + (Vi.z - Vj.z) * percent;
                rdp.vtxbuf[index].w = 0.01f;
                rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
                rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
                rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
                rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
                if (interpolate_colors)
                    InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
                else
                    rdp.vtxbuf[index++].number = Vi.number | Vj.number;

                // Save the in point
                rdp.vtxbuf[index] = Vj;
                rdp.vtxbuf[index++].not_zclipped = 1;
            }
        }
    }
    rdp.n_global = index;
}

static void render_tri(uint16_t linew, int old_interpolate);

void do_triangle_stuff(uint16_t linew, int old_interpolate) // what else?? do the triangle stuff :P (to keep from writing code twice)
{
    int i;

    if (rdp.clip & CLIP_WMIN)
        clip_w(old_interpolate);

    float maxZ = (rdp.zsrc != 1) ? rdp.view_trans[2] + rdp.view_scale[2] : rdp.prim_depth;

    uint8_t no_clip = 2;
    for (i = 0; i < rdp.n_global; i++)
    {
        if (rdp.vtxbuf[i].not_zclipped)// && rdp.zsrc != 1)
        {
            WriteTrace(TraceRDP, TraceVerbose, " * NOT ZCLIPPPED: %d", rdp.vtxbuf[i].number);
            rdp.vtxbuf[i].x = rdp.vtxbuf[i].sx;
            rdp.vtxbuf[i].y = rdp.vtxbuf[i].sy;
            rdp.vtxbuf[i].z = rdp.vtxbuf[i].sz;
            rdp.vtxbuf[i].q = rdp.vtxbuf[i].oow;
            rdp.vtxbuf[i].u0 = rdp.vtxbuf[i].u0_w;
            rdp.vtxbuf[i].v0 = rdp.vtxbuf[i].v0_w;
            rdp.vtxbuf[i].u1 = rdp.vtxbuf[i].u1_w;
            rdp.vtxbuf[i].v1 = rdp.vtxbuf[i].v1_w;
        }
        else
        {
            WriteTrace(TraceRDP, TraceVerbose, " * ZCLIPPED: %d", rdp.vtxbuf[i].number);
            rdp.vtxbuf[i].q = 1.0f / rdp.vtxbuf[i].w;
            rdp.vtxbuf[i].x = rdp.view_trans[0] + rdp.vtxbuf[i].x * rdp.vtxbuf[i].q * rdp.view_scale[0] + rdp.offset_x;
            rdp.vtxbuf[i].y = rdp.view_trans[1] + rdp.vtxbuf[i].y * rdp.vtxbuf[i].q * rdp.view_scale[1] + rdp.offset_y;
            rdp.vtxbuf[i].z = rdp.view_trans[2] + rdp.vtxbuf[i].z * rdp.vtxbuf[i].q * rdp.view_scale[2];
            if (rdp.tex >= 1)
            {
                rdp.vtxbuf[i].u0 *= rdp.vtxbuf[i].q;
                rdp.vtxbuf[i].v0 *= rdp.vtxbuf[i].q;
            }
            if (rdp.tex >= 2)
            {
                rdp.vtxbuf[i].u1 *= rdp.vtxbuf[i].q;
                rdp.vtxbuf[i].v1 *= rdp.vtxbuf[i].q;
            }
        }

        if (rdp.zsrc == 1)
            rdp.vtxbuf[i].z = rdp.prim_depth;

        // Don't remove clipping, or it will freeze
        if (rdp.vtxbuf[i].x > rdp.clip_max_x) rdp.clip |= CLIP_XMAX;
        if (rdp.vtxbuf[i].x < rdp.clip_min_x) rdp.clip |= CLIP_XMIN;
        if (rdp.vtxbuf[i].y > rdp.clip_max_y) rdp.clip |= CLIP_YMAX;
        if (rdp.vtxbuf[i].y < rdp.clip_min_y) rdp.clip |= CLIP_YMIN;
        if (rdp.vtxbuf[i].z > maxZ)           rdp.clip |= CLIP_ZMAX;
        if (rdp.vtxbuf[i].z < 0.0f)           rdp.clip |= CLIP_ZMIN;
        no_clip &= rdp.vtxbuf[i].screen_translated;
    }
    if (no_clip)
        rdp.clip = 0;
    else
    {
        if (!g_settings->clip_zmin())
        {
            rdp.clip &= ~CLIP_ZMIN;
        }
        if (!g_settings->clip_zmax())
        {
            rdp.clip &= ~CLIP_ZMAX;
        }
    }
    render_tri(linew, old_interpolate);
}

void do_triangle_stuff_2(uint16_t linew)
{
    rdp.clip = 0;

    for (int i = 0; i < rdp.n_global; i++)
    {
        // Don't remove clipping, or it will freeze
        if (rdp.vtxbuf[i].x > rdp.clip_max_x) rdp.clip |= CLIP_XMAX;
        if (rdp.vtxbuf[i].x < rdp.clip_min_x) rdp.clip |= CLIP_XMIN;
        if (rdp.vtxbuf[i].y > rdp.clip_max_y) rdp.clip |= CLIP_YMAX;
        if (rdp.vtxbuf[i].y < rdp.clip_min_y) rdp.clip |= CLIP_YMIN;
    }

    render_tri(linew, TRUE);
}

__inline uint8_t real_to_char(double x)
{
    return (uint8_t)(((int)floor(x + 0.5)) & 0xFF);
}

//*
static void InterpolateColors2(VERTEX & va, VERTEX & vb, VERTEX & res, float percent)
{
    float w = 1.0f / (va.oow + (vb.oow - va.oow) * percent);
    //   res.oow = va.oow + (vb.oow-va.oow) * percent;
    //   res.q = res.oow;
    float ba = va.b * va.oow;
    float bb = vb.b * vb.oow;
    res.b = real_to_char(interp2p(ba, bb, percent) * w);
    float ga = va.g * va.oow;
    float gb = vb.g * vb.oow;
    res.g = real_to_char(interp2p(ga, gb, percent) * w);
    float ra = va.r * va.oow;
    float rb = vb.r * vb.oow;
    res.r = real_to_char(interp2p(ra, rb, percent) * w);
    float aa = va.a * va.oow;
    float ab = vb.a * vb.oow;
    res.a = real_to_char(interp2p(aa, ab, percent) * w);
    float fa = va.f * va.oow;
    float fb = vb.f * vb.oow;
    res.f = interp2p(fa, fb, percent) * w;
    /*
    float u0a = va.u0_w * va.oow;
    float u0b = vb.u0_w * vb.oow;
    res.u0 = (u0a + (u0b - u0a) * percent) * w;
    float v0a = va.v0_w * va.oow;
    float v0b = vb.v0_w * vb.oow;
    res.v0 = (v0a + (v0b - v0a) * percent) * w;
    float u1a = va.u1_w * va.oow;
    float u1b = vb.u1_w * vb.oow;
    res.u1 = (u1a + (u1b - u1a) * percent) * w;
    float v1a = va.v1_w * va.oow;
    float v1b = vb.v1_w * vb.oow;
    res.v1 = (v1a + (v1b - v1a) * percent) * w;
    */
}
//*/

typedef struct {
    double d;
    double x;
    double y;
} LineEuqationType;

static double EvaLine(LineEuqationType &li, double x, double y)
{
    return li.x*x + li.y*y + li.d;
}

static void Create1LineEq(LineEuqationType &l, VERTEX &v1, VERTEX &v2, VERTEX &v3)
{
    // Line between (x1,y1) to (x2,y2)
    l.x = v2.sy - v1.sy;
    l.y = v1.sx - v2.sx;
    l.d = -(l.x*v2.sx + (l.y)*v2.sy);
    if (EvaLine(l, v3.sx, v3.sy)*v3.oow < 0)
    {
        l.x = -l.x;
        l.y = -l.y;
        l.d = -l.d;
    }
}

__inline double interp3p(float a, float b, float c, double r1, double r2)
{
    return (a)+(((b)+((c)-(b))*(r2)) - (a))*(r1);
}
/*
#define interp3p(a, b, c, r1, r2) \
  (a+(((b)+((c)-(b))*(r2))-(a))*(r1))
  */

static void InterpolateColors3(VERTEX &v1, VERTEX &v2, VERTEX &v3, VERTEX &out)
{
    LineEuqationType line;
    Create1LineEq(line, v2, v3, v1);

    double aDot = (out.x*line.x + out.y*line.y);
    double bDot = (v1.sx*line.x + v1.sy*line.y);

    double scale1 = (-line.d - aDot) / (bDot - aDot);

    double tx = out.x + scale1 * (v1.sx - out.x);
    double ty = out.y + scale1 * (v1.sy - out.y);

    double s1 = 101.0, s2 = 101.0;
    double den = tx - v1.sx;
    if (fabs(den) > 1.0)
        s1 = (out.x - v1.sx) / den;
    if (s1 > 100.0f)
        s1 = (out.y - v1.sy) / (ty - v1.sy);

    den = v3.sx - v2.sx;
    if (fabs(den) > 1.0)
        s2 = (tx - v2.sx) / den;
    if (s2 > 100.0f)
        s2 = (ty - v2.sy) / (v3.sy - v2.sy);

    double w = 1.0 / interp3p(v1.oow, v2.oow, v3.oow, s1, s2);

    out.r = real_to_char(interp3p(v1.r*v1.oow, v2.r*v2.oow, v3.r*v3.oow, s1, s2)*w);
    out.g = real_to_char(interp3p(v1.g*v1.oow, v2.g*v2.oow, v3.g*v3.oow, s1, s2)*w);
    out.b = real_to_char(interp3p(v1.b*v1.oow, v2.b*v2.oow, v3.b*v3.oow, s1, s2)*w);
    out.a = real_to_char(interp3p(v1.a*v1.oow, v2.a*v2.oow, v3.a*v3.oow, s1, s2)*w);
    out.f = (float)(interp3p(v1.f*v1.oow, v2.f*v2.oow, v3.f*v3.oow, s1, s2)*w);
    /*
    out.u0 = interp3p(v1.u0_w*v1.oow,v2.u0_w*v2.oow,v3.u0_w*v3.oow,s1,s2)/oow;
    out.v0 = interp3p(v1.v0_w*v1.oow,v2.v0_w*v2.oow,v3.v0_w*v3.oow,s1,s2)/oow;
    out.u1 = interp3p(v1.u1_w*v1.oow,v2.u1_w*v2.oow,v3.u1_w*v3.oow,s1,s2)/oow;
    out.v1 = interp3p(v1.v1_w*v1.oow,v2.v1_w*v2.oow,v3.v1_w*v3.oow,s1,s2)/oow;
    */
}

static void CalculateLOD(VERTEX *v, int n)
{
    float deltaS, deltaT;
    float deltaX, deltaY;
    double deltaTexels, deltaPixels, lodFactor = 0;
    double intptr;
    float s_scale = rdp.tiles(rdp.cur_tile).width / 255.0f;
    float t_scale = rdp.tiles(rdp.cur_tile).height / 255.0f;
    if (g_settings->lodmode() == CSettings::LOD_Fast)
    {
        deltaS = (v[1].u0 / v[1].q - v[0].u0 / v[0].q) * s_scale;
        deltaT = (v[1].v0 / v[1].q - v[0].v0 / v[0].q) * t_scale;
        deltaTexels = sqrt(deltaS * deltaS + deltaT * deltaT);

        deltaX = (v[1].x - v[0].x) / rdp.scale_x;
        deltaY = (v[1].y - v[0].y) / rdp.scale_y;
        deltaPixels = sqrt(deltaX * deltaX + deltaY * deltaY);

        lodFactor = deltaTexels / deltaPixels;
    }
    else
    {
        int i, j;
        for (i = 0; i < n; i++)
        {
            j = (i < n - 1) ? i + 1 : 0;

            deltaS = (v[j].u0 / v[j].q - v[i].u0 / v[i].q) * s_scale;
            deltaT = (v[j].v0 / v[j].q - v[i].v0 / v[i].q) * t_scale;
            //    deltaS = v[j].ou - v[i].ou;
            //    deltaT = v[j].ov - v[i].ov;
            deltaTexels = sqrt(deltaS * deltaS + deltaT * deltaT);

            deltaX = (v[j].x - v[i].x) / rdp.scale_x;
            deltaY = (v[j].y - v[i].y) / rdp.scale_y;
            deltaPixels = sqrt(deltaX * deltaX + deltaY * deltaY);

            lodFactor += deltaTexels / deltaPixels;
        }
        // Divide by n (n edges) to find average
        lodFactor = lodFactor / n;
    }
    int ilod = (int)lodFactor;
    int lod_tile = minval((int)(log10f((float)ilod) / log10f(2.0f)), rdp.cur_tile + rdp.mipmap_level);
    float lod_fraction = 1.0f;
    if (lod_tile < rdp.cur_tile + rdp.mipmap_level)
    {
        lod_fraction = maxval((float)modf(lodFactor / pow(2., lod_tile), &intptr), rdp.prim_lodmin / 255.0f);
    }
    float detailmax;
    if (cmb.dc0_detailmax < 0.5f)
        detailmax = lod_fraction;
    else
        detailmax = 1.0f - lod_fraction;
    gfxTexDetailControl(GFX_TMU0, cmb.dc0_lodbias, cmb.dc0_detailscale, detailmax);
    if ((nbTextureUnits > 2 ? 2 : 1) == 2)
        gfxTexDetailControl(GFX_TMU1, cmb.dc1_lodbias, cmb.dc1_detailscale, detailmax);
    WriteTrace(TraceRDP, TraceDebug, "CalculateLOD factor: %f, tile: %d, lod_fraction: %f", (float)lodFactor, lod_tile, lod_fraction);
}

float ScaleZ(float z)
{
    if (g_settings->n64_z_scale())
    {
        int iz = (int)(z*8.0f + 0.5f);
        if (iz < 0) iz = 0;
        else if (iz >= 0x40000) iz = 0x40000 - 1;
        return (float)zLUT[iz];
    }
    if (z < 0.0f) return 0.0f;
    z *= 1.9f;
    if (z > 65534.0f) return 65534.0f;
    return z;
}

static void DepthBuffer(VERTEX * vtx, int n)
{
    if (g_settings->fb_depth_render_enabled() && !g_settings->hacks(CSettings::hack_RE2) && dzdx && (rdp.flags & ZBUF_UPDATE))
    {
        vertexi v[12];
        if (u_cull_mode == 1) //cull front
        {
            for (int i = 0; i < n; i++)
            {
                v[i].x = (int)((vtx[n - i - 1].x - rdp.offset_x) / rdp.scale_x * 65536.0);
                v[i].y = (int)((vtx[n - i - 1].y - rdp.offset_y) / rdp.scale_y * 65536.0);
                v[i].z = (int)(vtx[n - i - 1].z * 65536.0);
            }
        }
        else
        {
            for (int i = 0; i < n; i++)
            {
                v[i].x = (int)((vtx[i].x - rdp.offset_x) / rdp.scale_x * 65536.0);
                v[i].y = (int)((vtx[i].y - rdp.offset_y) / rdp.scale_y * 65536.0);
                v[i].z = (int)(vtx[i].z * 65536.0);
            }
        }
        Rasterize(v, n, dzdx);
    }
    for (int i = 0; i < n; i++)
        vtx[i].z = ScaleZ(vtx[i].z);
}

void clip_tri(int interpolate_colors)
{
    int i, j, index, n = rdp.n_global;
    float percent;

    // Check which clipping is needed
    if (rdp.clip & CLIP_XMAX) // right of the screen
    {
        // Swap vertex buffers
        VERTEX *tmp = rdp.vtxbuf2;
        rdp.vtxbuf2 = rdp.vtxbuf;
        rdp.vtxbuf = tmp;
        rdp.vtx_buffer ^= 1;
        index = 0;

        // Check the vertices for clipping
        for (i = 0; i < n; i++)
        {
            j = i + 1;
            if (j == n) j = 0;

            if (Vi.x <= rdp.clip_max_x)
            {
                if (Vj.x <= rdp.clip_max_x)   // Both are in, save the last one
                {
                    rdp.vtxbuf[index++] = Vj;
                }
                else      // First is in, second is out, save intersection
                {
                    percent = (rdp.clip_max_x - Vi.x) / (Vj.x - Vi.x);
                    rdp.vtxbuf[index].x = rdp.clip_max_x;
                    rdp.vtxbuf[index].y = Vi.y + (Vj.y - Vi.y) * percent;
                    rdp.vtxbuf[index].z = Vi.z + (Vj.z - Vi.z) * percent;
                    rdp.vtxbuf[index].q = Vi.q + (Vj.q - Vi.q) * percent;
                    rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 8;
                }
            }
            else
            {
                //if (Vj.x > rdp.clip_max_x)  // Both are out, save nothing
                if (Vj.x <= rdp.clip_max_x) // First is out, second is in, save intersection & in point
                {
                    percent = (rdp.clip_max_x - Vj.x) / (Vi.x - Vj.x);
                    rdp.vtxbuf[index].x = rdp.clip_max_x;
                    rdp.vtxbuf[index].y = Vj.y + (Vi.y - Vj.y) * percent;
                    rdp.vtxbuf[index].z = Vj.z + (Vi.z - Vj.z) * percent;
                    rdp.vtxbuf[index].q = Vj.q + (Vi.q - Vj.q) * percent;
                    rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 8;

                    // Save the in point
                    rdp.vtxbuf[index++] = Vj;
                }
            }
        }
        n = index;
    }
    if (rdp.clip & CLIP_XMIN) // left of the screen
    {
        // Swap vertex buffers
        VERTEX *tmp = rdp.vtxbuf2;
        rdp.vtxbuf2 = rdp.vtxbuf;
        rdp.vtxbuf = tmp;
        rdp.vtx_buffer ^= 1;
        index = 0;

        // Check the vertices for clipping
        for (i = 0; i < n; i++)
        {
            j = i + 1;
            if (j == n) j = 0;

            if (Vi.x >= rdp.clip_min_x)
            {
                if (Vj.x >= rdp.clip_min_x)   // Both are in, save the last one
                {
                    rdp.vtxbuf[index++] = Vj;
                }
                else      // First is in, second is out, save intersection
                {
                    percent = (rdp.clip_min_x - Vi.x) / (Vj.x - Vi.x);
                    rdp.vtxbuf[index].x = rdp.clip_min_x;
                    rdp.vtxbuf[index].y = Vi.y + (Vj.y - Vi.y) * percent;
                    rdp.vtxbuf[index].z = Vi.z + (Vj.z - Vi.z) * percent;
                    rdp.vtxbuf[index].q = Vi.q + (Vj.q - Vi.q) * percent;
                    rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 8;
                }
            }
            else
            {
                //if (Vj.x < rdp.clip_min_x)  // Both are out, save nothing
                if (Vj.x >= rdp.clip_min_x) // First is out, second is in, save intersection & in point
                {
                    percent = (rdp.clip_min_x - Vj.x) / (Vi.x - Vj.x);
                    rdp.vtxbuf[index].x = rdp.clip_min_x;
                    rdp.vtxbuf[index].y = Vj.y + (Vi.y - Vj.y) * percent;
                    rdp.vtxbuf[index].z = Vj.z + (Vi.z - Vj.z) * percent;
                    rdp.vtxbuf[index].q = Vj.q + (Vi.q - Vj.q) * percent;
                    rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 8;

                    // Save the in point
                    rdp.vtxbuf[index++] = Vj;
                }
            }
        }
        n = index;
    }
    if (rdp.clip & CLIP_YMAX) // top of the screen
    {
        // Swap vertex buffers
        VERTEX *tmp = rdp.vtxbuf2;
        rdp.vtxbuf2 = rdp.vtxbuf;
        rdp.vtxbuf = tmp;
        rdp.vtx_buffer ^= 1;
        index = 0;

        // Check the vertices for clipping
        for (i = 0; i < n; i++)
        {
            j = i + 1;
            if (j == n) j = 0;

            if (Vi.y <= rdp.clip_max_y)
            {
                if (Vj.y <= rdp.clip_max_y)   // Both are in, save the last one
                {
                    rdp.vtxbuf[index++] = Vj;
                }
                else      // First is in, second is out, save intersection
                {
                    percent = (rdp.clip_max_y - Vi.y) / (Vj.y - Vi.y);
                    rdp.vtxbuf[index].x = Vi.x + (Vj.x - Vi.x) * percent;
                    rdp.vtxbuf[index].y = rdp.clip_max_y;
                    rdp.vtxbuf[index].z = Vi.z + (Vj.z - Vi.z) * percent;
                    rdp.vtxbuf[index].q = Vi.q + (Vj.q - Vi.q) * percent;
                    rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 16;
                }
            }
            else
            {
                //if (Vj.y > rdp.clip_max_y)  // Both are out, save nothing
                if (Vj.y <= rdp.clip_max_y) // First is out, second is in, save intersection & in point
                {
                    percent = (rdp.clip_max_y - Vj.y) / (Vi.y - Vj.y);
                    rdp.vtxbuf[index].x = Vj.x + (Vi.x - Vj.x) * percent;
                    rdp.vtxbuf[index].y = rdp.clip_max_y;
                    rdp.vtxbuf[index].z = Vj.z + (Vi.z - Vj.z) * percent;
                    rdp.vtxbuf[index].q = Vj.q + (Vi.q - Vj.q) * percent;
                    rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 16;

                    // Save the in point
                    rdp.vtxbuf[index++] = Vj;
                }
            }
        }
        n = index;
    }
    if (rdp.clip & CLIP_YMIN) // bottom of the screen
    {
        // Swap vertex buffers
        VERTEX *tmp = rdp.vtxbuf2;
        rdp.vtxbuf2 = rdp.vtxbuf;
        rdp.vtxbuf = tmp;
        rdp.vtx_buffer ^= 1;
        index = 0;

        // Check the vertices for clipping
        for (i = 0; i < n; i++)
        {
            j = i + 1;
            if (j == n) j = 0;

            if (Vi.y >= rdp.clip_min_y)
            {
                if (Vj.y >= rdp.clip_min_y)   // Both are in, save the last one
                {
                    rdp.vtxbuf[index++] = Vj;
                }
                else      // First is in, second is out, save intersection
                {
                    percent = (rdp.clip_min_y - Vi.y) / (Vj.y - Vi.y);
                    rdp.vtxbuf[index].x = Vi.x + (Vj.x - Vi.x) * percent;
                    rdp.vtxbuf[index].y = rdp.clip_min_y;
                    rdp.vtxbuf[index].z = Vi.z + (Vj.z - Vi.z) * percent;
                    rdp.vtxbuf[index].q = Vi.q + (Vj.q - Vi.q) * percent;
                    rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 16;
                }
            }
            else
            {
                //if (Vj.y < rdp.clip_min_y)  // Both are out, save nothing
                if (Vj.y >= rdp.clip_min_y) // First is out, second is in, save intersection & in point
                {
                    percent = (rdp.clip_min_y - Vj.y) / (Vi.y - Vj.y);
                    rdp.vtxbuf[index].x = Vj.x + (Vi.x - Vj.x) * percent;
                    rdp.vtxbuf[index].y = rdp.clip_min_y;
                    rdp.vtxbuf[index].z = Vj.z + (Vi.z - Vj.z) * percent;
                    rdp.vtxbuf[index].q = Vj.q + (Vi.q - Vj.q) * percent;
                    rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number | 16;

                    // Save the in point
                    rdp.vtxbuf[index++] = Vj;
                }
            }
        }
        n = index;
    }
    if (rdp.clip & CLIP_ZMAX) // far plane
    {
        // Swap vertex buffers
        VERTEX *tmp = rdp.vtxbuf2;
        rdp.vtxbuf2 = rdp.vtxbuf;
        rdp.vtxbuf = tmp;
        rdp.vtx_buffer ^= 1;
        index = 0;
        float maxZ = rdp.view_trans[2] + rdp.view_scale[2];

        // Check the vertices for clipping
        for (i = 0; i < n; i++)
        {
            j = i + 1;
            if (j == n) j = 0;

            if (Vi.z < maxZ)
            {
                if (Vj.z < maxZ)   // Both are in, save the last one
                {
                    rdp.vtxbuf[index++] = Vj;
                }
                else      // First is in, second is out, save intersection
                {
                    percent = (maxZ - Vi.z) / (Vj.z - Vi.z);
                    rdp.vtxbuf[index].x = Vi.x + (Vj.x - Vi.x) * percent;
                    rdp.vtxbuf[index].y = Vi.y + (Vj.y - Vi.y) * percent;
                    rdp.vtxbuf[index].z = maxZ - 0.001f;
                    rdp.vtxbuf[index].q = Vi.q + (Vj.q - Vi.q) * percent;
                    rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number;
                }
            }
            else
            {
                //if (Vj.z > maxZ)  // Both are out, save nothing
                if (Vj.z < maxZ) // First is out, second is in, save intersection & in point
                {
                    percent = (maxZ - Vj.z) / (Vi.z - Vj.z);
                    rdp.vtxbuf[index].x = Vj.x + (Vi.x - Vj.x) * percent;
                    rdp.vtxbuf[index].y = Vj.y + (Vi.y - Vj.y) * percent;
                    rdp.vtxbuf[index].z = maxZ - 0.001f;;
                    rdp.vtxbuf[index].q = Vj.q + (Vi.q - Vj.q) * percent;
                    rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
                    rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
                    rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
                    rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
                    if (interpolate_colors)
                        InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
                    else
                        rdp.vtxbuf[index++].number = Vi.number | Vj.number;

                    // Save the in point
                    rdp.vtxbuf[index++] = Vj;
                }
            }
        }
        n = index;
    }
    /*
      if (rdp.clip & CLIP_ZMIN) // near Z
      {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
      j = i+1;
      if (j == n) j = 0;

      if (Vi.z >= 0.0f)
      {
      if (Vj.z >= 0.0f)   // Both are in, save the last one
      {
      rdp.vtxbuf[index++] = Vj;
      }
      else      // First is in, second is out, save intersection
      {
      percent = (-Vi.z) / (Vj.z - Vi.z);
      rdp.vtxbuf[index].x = Vi.x + (Vj.x - Vi.x) * percent;
      rdp.vtxbuf[index].y = Vi.y + (Vj.y - Vi.y) * percent;
      rdp.vtxbuf[index].z = 0.0f;
      rdp.vtxbuf[index].q = Vi.q + (Vj.q - Vi.q) * percent;
      rdp.vtxbuf[index].u0 = Vi.u0 + (Vj.u0 - Vi.u0) * percent;
      rdp.vtxbuf[index].v0 = Vi.v0 + (Vj.v0 - Vi.v0) * percent;
      rdp.vtxbuf[index].u1 = Vi.u1 + (Vj.u1 - Vi.u1) * percent;
      rdp.vtxbuf[index].v1 = Vi.v1 + (Vj.v1 - Vi.v1) * percent;
      if (interpolate_colors)
      InterpolateColors(Vi, Vj, rdp.vtxbuf[index++], percent);
      else
      rdp.vtxbuf[index++].number = Vi.number | Vj.number;
      }
      }
      else
      {
      //if (Vj.z < 0.0f)  // Both are out, save nothing
      if (Vj.z >= 0.0f) // First is out, second is in, save intersection & in point
      {
      percent = (-Vj.z) / (Vi.z - Vj.z);
      rdp.vtxbuf[index].x = Vj.x + (Vi.x - Vj.x) * percent;
      rdp.vtxbuf[index].y = Vj.y + (Vi.y - Vj.y) * percent;
      rdp.vtxbuf[index].z = 0.0f;;
      rdp.vtxbuf[index].q = Vj.q + (Vi.q - Vj.q) * percent;
      rdp.vtxbuf[index].u0 = Vj.u0 + (Vi.u0 - Vj.u0) * percent;
      rdp.vtxbuf[index].v0 = Vj.v0 + (Vi.v0 - Vj.v0) * percent;
      rdp.vtxbuf[index].u1 = Vj.u1 + (Vi.u1 - Vj.u1) * percent;
      rdp.vtxbuf[index].v1 = Vj.v1 + (Vi.v1 - Vj.v1) * percent;
      if (interpolate_colors)
      InterpolateColors(Vj, Vi, rdp.vtxbuf[index++], percent);
      else
      rdp.vtxbuf[index++].number = Vi.number | Vj.number;

      // Save the in point
      rdp.vtxbuf[index++] = Vj;
      }
      }
      }
      n = index;
      }
      */
    rdp.n_global = n;
}

static void render_tri(uint16_t linew, int old_interpolate)
{
    if (rdp.clip)
        clip_tri(old_interpolate);
    int n = rdp.n_global;
    if (n < 3)
    {
        WriteTrace(TraceRDP, TraceDebug, " * render_tri: n < 3");
        return;
    }
    int i, j;
    //*
    if ((rdp.clip & CLIP_ZMIN) && (rdp.othermode_l & 0x00000030))
    {
        int to_render = FALSE;
        for (i = 0; i < n; i++)
        {
            if (rdp.vtxbuf[i].z >= 0.0f)
            {
                to_render = TRUE;
                break;
            }
        }
        if (!to_render) //all z < 0
        {
            WriteTrace(TraceRDP, TraceDebug, " * render_tri: all z < 0");
            return;
        }
    }
    //*/
    if (rdp.clip && !old_interpolate)
    {
        for (i = 0; i < n; i++)
        {
            float percent = 101.0f;
            VERTEX * v1 = 0, *v2 = 0;
            switch (rdp.vtxbuf[i].number & 7)
            {
            case 1:
            case 2:
            case 4:
                continue;
                break;
            case 3:
                v1 = org_vtx[0];
                v2 = org_vtx[1];
                break;
            case 5:
                v1 = org_vtx[0];
                v2 = org_vtx[2];
                break;
            case 6:
                v1 = org_vtx[1];
                v2 = org_vtx[2];
                break;
            case 7:
                InterpolateColors3(*org_vtx[0], *org_vtx[1], *org_vtx[2], rdp.vtxbuf[i]);
                continue;
                break;
            }
            switch (rdp.vtxbuf[i].number & 24)
            {
            case 8:
                percent = (rdp.vtxbuf[i].x - v1->sx) / (v2->sx - v1->sx);
                break;
            case 16:
                percent = (rdp.vtxbuf[i].y - v1->sy) / (v2->sy - v1->sy);
                break;
            default:
            {
                float d = (v2->sx - v1->sx);
                if (fabs(d) > 1.0)
                    percent = (rdp.vtxbuf[i].x - v1->sx) / d;
                if (percent > 100.0f)
                    percent = (rdp.vtxbuf[i].y - v1->sy) / (v2->sy - v1->sy);
            }
            }
            InterpolateColors2(*v1, *v2, rdp.vtxbuf[i], percent);
        }
    }

    ConvertCoordsConvert(rdp.vtxbuf, n);
    if (rdp.fog_mode == CRDP::fog_enabled)
    {
        for (i = 0; i < n; i++)
        {
            rdp.vtxbuf[i].f = 1.0f / maxval(4.0f, rdp.vtxbuf[i].f);
        }
    }
    else if (rdp.fog_mode == CRDP::fog_blend)
    {
        float fog = 1.0f / maxval(1, rdp.fog_color & 0xFF);
        for (i = 0; i < n; i++)
        {
            rdp.vtxbuf[i].f = fog;
        }
    }
    else if (rdp.fog_mode == CRDP::fog_blend_inverse)
    {
        float fog = 1.0f / maxval(1, (~rdp.fog_color) & 0xFF);
        for (i = 0; i < n; i++)
        {
            rdp.vtxbuf[i].f = fog;
        }
    }

    if (g_settings->lodmode() != CSettings::LOD_Off && rdp.cur_tile < rdp.mipmap_level)
    {
        CalculateLOD(rdp.vtxbuf, n);
    }

    cmb.cmb_ext_use = cmb.tex_cmb_ext_use = 0;

    if (g_settings->wireframe())
    {
        SetWireframeCol();
        for (i = 0; i < n; i++)
        {
            j = i + 1;
            if (j == n) j = 0;
            gfxDrawLine(&rdp.vtxbuf[i], &rdp.vtxbuf[j]);
        }
    }
    else
    {
        if (linew > 0)
        {
            VERTEX *V0 = &rdp.vtxbuf[0];
            VERTEX *V1 = &rdp.vtxbuf[1];
            if (fabs(V0->x - V1->x) < 0.01 && fabs(V0->y - V1->y) < 0.01)
                V1 = &rdp.vtxbuf[2];
            V0->z = ScaleZ(V0->z);
            V1->z = ScaleZ(V1->z);
            VERTEX v[4];
            v[0] = *V0;
            v[1] = *V0;
            v[2] = *V1;
            v[3] = *V1;
            float width = linew * 0.25f;
            if (fabs(V0->y - V1->y) < 0.0001)
            {
                v[0].x = v[1].x = V0->x;
                v[2].x = v[3].x = V1->x;

                width *= rdp.scale_y;
                v[0].y = v[2].y = V0->y - width;
                v[1].y = v[3].y = V0->y + width;
            }
            else if (fabs(V0->x - V1->x) < 0.0001)
            {
                v[0].y = v[1].y = V0->y;
                v[2].y = v[3].y = V1->y;

                width *= rdp.scale_x;
                v[0].x = v[2].x = V0->x - width;
                v[1].x = v[3].x = V0->x + width;
            }
            else
            {
                float dx = V1->x - V0->x;
                float dy = V1->y - V0->y;
                float len = sqrtf(dx*dx + dy*dy);
                float wx = dy * width * rdp.scale_x / len;
                float wy = dx * width * rdp.scale_y / len;
                v[0].x = V0->x + wx;
                v[0].y = V0->y - wy;
                v[1].x = V0->x - wx;
                v[1].y = V0->y + wy;
                v[2].x = V1->x + wx;
                v[2].y = V1->y - wy;
                v[3].x = V1->x - wx;
                v[3].y = V1->y + wy;
            }
            gfxDrawTriangle(&v[0], &v[1], &v[2]);
            gfxDrawTriangle(&v[1], &v[2], &v[3]);
        }
        else
        {
            DepthBuffer(rdp.vtxbuf, n);
            if ((rdp.rm & 0xC10) == 0xC10)
                gfxDepthBiasLevel(-deltaZ);
            gfxDrawVertexArray(GFX_TRIANGLE_FAN, n, rdp.vtx_buffer ? (&vtx_list2) : (&vtx_list1));
        }
    }
}

void update_scissor()
{
    if (rdp.update & UPDATE_SCISSOR)
    {
        rdp.update ^= UPDATE_SCISSOR;

        // KILL the floating point error with 0.01f
        rdp.scissor.ul_x = (uint32_t)maxval(minval((rdp.scissor_o.ul_x * rdp.scale_x + rdp.offset_x + 0.01f), g_scr_res_x), 0);
        rdp.scissor.lr_x = (uint32_t)maxval(minval((rdp.scissor_o.lr_x * rdp.scale_x + rdp.offset_x + 0.01f), g_scr_res_x), 0);
        rdp.scissor.ul_y = (uint32_t)maxval(minval((rdp.scissor_o.ul_y * rdp.scale_y + rdp.offset_y + 0.01f), g_scr_res_y), 0);
        rdp.scissor.lr_y = (uint32_t)maxval(minval((rdp.scissor_o.lr_y * rdp.scale_y + rdp.offset_y + 0.01f), g_scr_res_y), 0);
        //gfxClipWindow specifies the hardware clipping window. Any pixels outside the clipping window are rejected.
        //Values are inclusive for minimum x and y values and exclusive for maximum x and y values.
        //    gfxClipWindow (rdp.scissor.ul_x?rdp.scissor.ul_x+1:0, rdp.scissor.ul_y?rdp.scissor.ul_y+1:0, rdp.scissor.lr_x, rdp.scissor.lr_y);
        gfxClipWindow(rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
        WriteTrace(TraceRDP, TraceDebug, " |- scissor - (%d, %d) -> (%d, %d)", rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
    }
}

//
// update - update states if they need it
//

typedef struct
{
    unsigned int	c2_m2b : 2;
    unsigned int	c1_m2b : 2;
    unsigned int	c2_m2a : 2;
    unsigned int	c1_m2a : 2;
    unsigned int	c2_m1b : 2;
    unsigned int	c1_m1b : 2;
    unsigned int	c2_m1a : 2;
    unsigned int	c1_m1a : 2;
} rdp_blender_setting;

void update()
{
    WriteTrace(TraceRDP, TraceDebug, "-+ update called");
    // Check for rendermode changes
    // Z buffer
    if (rdp.render_mode_changed & 0x00000C30)
    {
        WriteTrace(TraceRDP, TraceDebug, " |- render_mode_changed zbuf - decal: %s, update: %s, compare: %s",
            str_yn[(rdp.othermode_l & 0x00000400) ? 1 : 0],
            str_yn[(rdp.othermode_l & 0x00000020) ? 1 : 0],
            str_yn[(rdp.othermode_l & 0x00000010) ? 1 : 0]);

        rdp.render_mode_changed &= ~0x00000C30;
        rdp.update |= UPDATE_ZBUF_ENABLED;

        // Update?
        if ((rdp.othermode_l & 0x00000020))
            rdp.flags |= ZBUF_UPDATE;
        else
            rdp.flags &= ~ZBUF_UPDATE;

        // Compare?
        if (rdp.othermode_l & 0x00000010)
            rdp.flags |= ZBUF_COMPARE;
        else
            rdp.flags &= ~ZBUF_COMPARE;
    }

    // Alpha compare
    if (rdp.render_mode_changed & 0x00001000)
    {
        WriteTrace(TraceRDP, TraceDebug, " |- render_mode_changed alpha compare - on: %s",
            str_yn[(rdp.othermode_l & 0x00001000) ? 1 : 0]);
        rdp.render_mode_changed &= ~0x00001000;
        rdp.update |= UPDATE_ALPHA_COMPARE;

        if (rdp.othermode_l & 0x00001000)
            rdp.flags |= ALPHA_COMPARE;
        else
            rdp.flags &= ~ALPHA_COMPARE;
    }

    if (rdp.render_mode_changed & 0x00002000) // alpha cvg sel
    {
        WriteTrace(TraceRDP, TraceDebug, " |- render_mode_changed alpha cvg sel - on: %s",
            str_yn[(rdp.othermode_l & 0x00002000) ? 1 : 0]);
        rdp.render_mode_changed &= ~0x00002000;
        rdp.update |= UPDATE_COMBINE;
        rdp.update |= UPDATE_ALPHA_COMPARE;
    }

    // Force blend
    if (rdp.render_mode_changed & 0xFFFF0000)
    {
        WriteTrace(TraceRDP, TraceDebug, " |- render_mode_changed force_blend - %08lx", rdp.othermode_l & 0xFFFF0000);
        rdp.render_mode_changed &= 0x0000FFFF;

        rdp.fbl_a0 = (uint8_t)((rdp.othermode_l >> 30) & 0x3);
        rdp.fbl_b0 = (uint8_t)((rdp.othermode_l >> 26) & 0x3);
        rdp.fbl_c0 = (uint8_t)((rdp.othermode_l >> 22) & 0x3);
        rdp.fbl_d0 = (uint8_t)((rdp.othermode_l >> 18) & 0x3);
        rdp.fbl_a1 = (uint8_t)((rdp.othermode_l >> 28) & 0x3);
        rdp.fbl_b1 = (uint8_t)((rdp.othermode_l >> 24) & 0x3);
        rdp.fbl_c1 = (uint8_t)((rdp.othermode_l >> 20) & 0x3);
        rdp.fbl_d1 = (uint8_t)((rdp.othermode_l >> 16) & 0x3);

        rdp.update |= UPDATE_COMBINE;
    }

    // Combine MUST go before texture
    if ((rdp.update & UPDATE_COMBINE) && rdp.allow_combine)
    {
        TBUFF_COLOR_IMAGE * aTBuff[2] = { 0, 0 };
        if (rdp.aTBuffTex[0])
            aTBuff[rdp.aTBuffTex[0]->tile] = rdp.aTBuffTex[0];
        if (rdp.aTBuffTex[1])
            aTBuff[rdp.aTBuffTex[1]->tile] = rdp.aTBuffTex[1];
        rdp.aTBuffTex[0] = aTBuff[0];
        rdp.aTBuffTex[1] = aTBuff[1];

        WriteTrace(TraceRDP, TraceDebug, " |-+ update_combine");
        Combine();
    }

    if (rdp.update & UPDATE_TEXTURE)  // note: UPDATE_TEXTURE and UPDATE_COMBINE are the same
    {
        rdp.tex_ctr++;
        if (rdp.tex_ctr == 0xFFFFFFFF)
            rdp.tex_ctr = 0;

        TexCache();
        if (rdp.noise == CRDP::noise_none)
            rdp.update ^= UPDATE_TEXTURE;
    }

    // Z buffer
    if (rdp.update & UPDATE_ZBUF_ENABLED)
    {
        // already logged above
        rdp.update ^= UPDATE_ZBUF_ENABLED;

        if (((rdp.flags & ZBUF_ENABLED) || rdp.zsrc == 1) && rdp.cycle_mode < 2)
        {
            if (rdp.flags & ZBUF_COMPARE)
            {
                switch ((rdp.rm & 0xC00) >> 10) {
                case 0:
                    gfxDepthBiasLevel(0);
                    gfxDepthBufferFunction(g_settings->zmode_compare_less() ? GFX_CMP_LESS : GFX_CMP_LEQUAL);
                    break;
                case 1:
                    gfxDepthBiasLevel(-4);
                    gfxDepthBufferFunction(g_settings->zmode_compare_less() ? GFX_CMP_LESS : GFX_CMP_LEQUAL);
                    break;
                case 2:
                    gfxDepthBiasLevel(g_settings->ucode() == CSettings::ucode_PerfectDark ? -4 : 0);
                    gfxDepthBufferFunction(GFX_CMP_LESS);
                    break;
                case 3:
                    // will be set dynamically per polygon
                    //gfxDepthBiasLevel(-deltaZ);
                    gfxDepthBufferFunction(GFX_CMP_LEQUAL);
                    break;
                }
            }
            else
            {
                gfxDepthBiasLevel(0);
                gfxDepthBufferFunction(GFX_CMP_ALWAYS);
            }

            if (rdp.flags & ZBUF_UPDATE)
                gfxDepthMask(true);
            else
                gfxDepthMask(false);
        }
        else
        {
            gfxDepthBiasLevel(0);
            gfxDepthBufferFunction(GFX_CMP_ALWAYS);
            gfxDepthMask(false);
        }
    }

    // Alpha compare
    if (rdp.update & UPDATE_ALPHA_COMPARE)
    {
        // already logged above
        rdp.update ^= UPDATE_ALPHA_COMPARE;

        //	  if (rdp.acmp == 1 && !(rdp.othermode_l & 0x00002000) && !force_full_alpha)
        //      if (rdp.acmp == 1 && !(rdp.othermode_l & 0x00002000) && (rdp.blend_color&0xFF))
        if (rdp.acmp == 1 && !(rdp.othermode_l & 0x00002000) && (!(rdp.othermode_l & 0x00004000) || (rdp.blend_color & 0xFF)))
        {
            uint8_t reference = (uint8_t)(rdp.blend_color & 0xFF);
            gfxAlphaTestFunction(reference ? GFX_CMP_GEQUAL : GFX_CMP_GREATER);
            gfxAlphaTestReferenceValue(reference);
            WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: blend: %02lx", reference);
        }
        else
        {
            if (rdp.flags & ALPHA_COMPARE)
            {
                if ((rdp.othermode_l & 0x5000) != 0x5000)
                {
                    gfxAlphaTestFunction(GFX_CMP_GEQUAL);
                    gfxAlphaTestReferenceValue(0x20);//0xA0);
                    WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: 0x20");
                }
                else
                {
                    gfxAlphaTestFunction(GFX_CMP_GREATER);
                    if (rdp.acmp == 3)
                    {
                        gfxAlphaTestReferenceValue((uint8_t)(rdp.blend_color & 0xFF));
                        WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: blend: %02lx", rdp.blend_color & 0xFF);
                    }
                    else
                    {
                        gfxAlphaTestReferenceValue(0x00);
                        WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: 0x00");
                    }
                }
            }
            else
            {
                gfxAlphaTestFunction(GFX_CMP_ALWAYS);
                WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: none");
            }
        }
        if (rdp.acmp == 3 && rdp.cycle_mode < 2)
        {
            if (g_settings->old_style_adither() || rdp.alpha_dither_mode != 3)
            {
                WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: dither");
                gfxStippleMode(g_settings->stipple_mode());
            }
            else
            {
                gfxStippleMode(GFX_STIPPLE_DISABLE);
            }
        }
        else
        {
            //WriteTrace(TraceRDP, TraceDebug, " |- alpha compare: dither disabled");
            gfxStippleMode(GFX_STIPPLE_DISABLE);
        }
    }
    // Cull mode (leave this in for z-clipped triangles)
    if (rdp.update & UPDATE_CULL_MODE)
    {
        rdp.update ^= UPDATE_CULL_MODE;
        uint32_t mode = (rdp.flags & CULLMASK) >> CULLSHIFT;
        WriteTrace(TraceRDP, TraceDebug, " |- cull_mode - mode: %s", str_cull[mode]);
        switch (mode)
        {
        case 0: // cull none
        case 3: // cull both
            gfxCullMode(GFX_CULL_DISABLE);
            break;
        case 1: // cull front
            //        gfxCullMode(GFX_CULL_POSITIVE);
            gfxCullMode(GFX_CULL_NEGATIVE);
            break;
        case 2: // cull back
            //        gfxCullMode (GFX_CULL_NEGATIVE);
            gfxCullMode(GFX_CULL_POSITIVE);
            break;
        }
    }

    //Added by Gonetz.
    if (g_settings->fog() && (rdp.update & UPDATE_FOG_ENABLED))
    {
        rdp.update ^= UPDATE_FOG_ENABLED;

        uint16_t blender = (uint16_t)(rdp.othermode_l >> 16);
        if (rdp.flags & FOG_ENABLED)
        {
            rdp_blender_setting &bl = *(rdp_blender_setting*)(&(blender));
            if ((rdp.fog_multiplier > 0) && (bl.c1_m1a == 3 || bl.c1_m2a == 3 || bl.c2_m1a == 3 || bl.c2_m2a == 3))
            {
                gfxFogColorValue(rdp.fog_color);
                gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
                rdp.fog_mode = CRDP::fog_enabled;
                WriteTrace(TraceRDP, TraceDebug, "fog enabled ");
            }
            else
            {
                WriteTrace(TraceRDP, TraceDebug, "fog disabled in blender");
                rdp.fog_mode = CRDP::fog_disabled;
                gfxFogMode(GFX_FOG_DISABLE);
            }
        }
        else if (blender == 0xc410 || blender == 0xc411 || blender == 0xf500)
        {
            gfxFogColorValue(rdp.fog_color);
            gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
            rdp.fog_mode = CRDP::fog_blend;
            WriteTrace(TraceRDP, TraceDebug, "fog blend ");
        }
        else if (blender == 0x04d1)
        {
            gfxFogColorValue(rdp.fog_color);
            gfxFogMode(GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
            rdp.fog_mode = CRDP::fog_blend_inverse;
            WriteTrace(TraceRDP, TraceDebug, "fog blend ");
        }
        else
        {
            WriteTrace(TraceRDP, TraceDebug, "fog disabled");
            rdp.fog_mode = CRDP::fog_disabled;
            gfxFogMode(GFX_FOG_DISABLE);
        }
    }

    if (rdp.update & UPDATE_VIEWPORT)
    {
        rdp.update ^= UPDATE_VIEWPORT;
        float scale_x = (float)fabs(rdp.view_scale[0]);
        float scale_y = (float)fabs(rdp.view_scale[1]);

        rdp.clip_min_x = maxval((rdp.view_trans[0] - scale_x + rdp.offset_x) / rdp.clip_ratio, 0.0f);
        rdp.clip_min_y = maxval((rdp.view_trans[1] - scale_y + rdp.offset_y) / rdp.clip_ratio, 0.0f);
        rdp.clip_max_x = minval((rdp.view_trans[0] + scale_x + rdp.offset_x) * rdp.clip_ratio, g_scr_res_x);
        rdp.clip_max_y = minval((rdp.view_trans[1] + scale_y + rdp.offset_y) * rdp.clip_ratio, g_scr_res_y);

        WriteTrace(TraceRDP, TraceDebug, " |- viewport - (%d, %d, %d, %d)", (uint32_t)rdp.clip_min_x, (uint32_t)rdp.clip_min_y, (uint32_t)rdp.clip_max_x, (uint32_t)rdp.clip_max_y);
        if (!rdp.scissor_set)
        {
            rdp.scissor.ul_x = (uint32_t)rdp.clip_min_x;
            rdp.scissor.lr_x = (uint32_t)rdp.clip_max_x;
            rdp.scissor.ul_y = (uint32_t)rdp.clip_min_y;
            rdp.scissor.lr_y = (uint32_t)rdp.clip_max_y;
            gfxClipWindow(rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
        }
    }

    if (rdp.update & UPDATE_SCISSOR)
    {
        update_scissor();
    }

    WriteTrace(TraceRDP, TraceDebug, " + update end");
}

void set_message_combiner()
{
    gfxColorCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_TEXTURE,
        false);
    gfxAlphaCombine(GFX_COMBINE_FUNCTION_SCALE_OTHER,
        GFX_COMBINE_FACTOR_ONE,
        GFX_COMBINE_LOCAL_NONE,
        GFX_COMBINE_OTHER_TEXTURE,
        false);
    gfxAlphaBlendFunction(GFX_BLEND_ONE,
        GFX_BLEND_ZERO,
        GFX_BLEND_ZERO,
        GFX_BLEND_ZERO);
    gfxAlphaTestFunction(GFX_CMP_ALWAYS);
    gfxStippleMode(GFX_STIPPLE_DISABLE);
    gfxTexFilterMode(GFX_TMU0, GFX_TEXTUREFILTER_BILINEAR, GFX_TEXTUREFILTER_BILINEAR);
    gfxTexCombine(GFX_TMU1,
        GFX_COMBINE_FUNCTION_NONE,
        GFX_COMBINE_FACTOR_NONE,
        GFX_COMBINE_FUNCTION_NONE,
        GFX_COMBINE_FACTOR_NONE,
        false, false);
    gfxTexCombine(GFX_TMU0,
        GFX_COMBINE_FUNCTION_LOCAL,
        GFX_COMBINE_FACTOR_NONE,
        GFX_COMBINE_FUNCTION_LOCAL,
        GFX_COMBINE_FACTOR_NONE,
        false, false);
    gfxTexSource(GFX_TMU0,
        voodoo.tex_min_addr[GFX_TMU0] + offset_font,
        GFX_MIPMAPLEVELMASK_BOTH,
        &fontTex);
    gfxFogMode(GFX_FOG_DISABLE);
}