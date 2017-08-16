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
#include "Util.h"
#include "3dmath.h"
#include "ucode00.h"

void uc9_rpdcmd();

typedef float M44[4][4];

struct ZSORTRDP {
    float view_scale[2];
    float view_trans[2];
    float scale_x;
    float scale_y;
} zSortRdp = { { 0, 0 }, { 0, 0 }, 0, 0 };

//RSP command VRCPL
static int Calc_invw(int w) {
    int count, neg;
    union {
        int32_t		  W;
        uint32_t	  UW;
        int16_t			HW[2];
        uint16_t		UHW[2];
    } Result;
    Result.W = w;
    if (Result.UW == 0) {
        Result.UW = 0x7FFFFFFF;
    }
    else {
        if (Result.W < 0) {
            neg = TRUE;
            if (Result.UHW[1] == 0xFFFF && Result.HW[0] < 0) {
                Result.W = ~Result.W + 1;
            }
            else {
                Result.W = ~Result.W;
            }
        }
        else {
            neg = FALSE;
        }
        for (count = 31; count > 0; count--) {
            if ((Result.W & (1 << count))) {
                Result.W &= (0xFFC00000 >> (31 - count));
                count = 0;
            }
        }
        Result.W = 0x7FFFFFFF / Result.W;
        for (count = 31; count > 0; count--) {
            if ((Result.W & (1 << count))) {
                Result.W &= (0xFFFF8000 >> (31 - count));
                count = 0;
            }
        }
        if (neg == TRUE) {
            Result.W = ~Result.W;
        }
    }
    return Result.W;
}

static void uc9_draw_object(uint8_t * addr, uint32_t type)
{
    uint32_t textured, vnum, vsize;
    switch (type) {
    case 0: //null
        textured = vnum = vsize = 0;
        break;
    case 1: //sh tri
        textured = 0;
        vnum = 3;
        vsize = 8;
        break;
    case 2: //tx tri
        textured = 1;
        vnum = 3;
        vsize = 16;
        break;
    case 3: //sh quad
        textured = 0;
        vnum = 4;
        vsize = 8;
        break;
    case 4: //tx quad
        textured = 1;
        vnum = 4;
        vsize = 16;
        break;
    default:
        WriteTrace(TraceRDP, TraceWarning, "Unknown geometric primitive type %u.", type);
        textured = vnum = vsize = 0;
        break;
    }
    gfxVERTEX vtx[4];
    for (uint32_t i = 0; i < vnum; i++)
    {
        gfxVERTEX &v = vtx[i];
        v.sx = zSortRdp.scale_x * ((short*)addr)[0 ^ 1];
        v.sy = zSortRdp.scale_y * ((short*)addr)[1 ^ 1];
        v.sz = 1.0f;
        v.r = addr[4 ^ 3];
        v.g = addr[5 ^ 3];
        v.b = addr[6 ^ 3];
        v.a = addr[7 ^ 3];
        v.flags = 0;
        v.uv_scaled = 0;
        v.uv_calculated = 0xFFFFFFFF;
        v.shade_mod = 0;
        v.scr_off = 0;
        v.screen_translated = 2;
        if (textured)
        {
            v.ou = ((short*)addr)[4 ^ 1];
            v.ov = ((short*)addr)[5 ^ 1];
            v.w = Calc_invw(((int*)addr)[3]) / 31.0f;
            v.oow = 1.0f / v.w;
            WriteTrace(TraceRDP, TraceDebug, "v%d - sx: %f, sy: %f ou: %f, ov: %f, w: %f, r=%d, g=%d, b=%d, a=%d", i, v.sx / rdp.scale_x, v.sy / rdp.scale_y, v.ou*rdp.tiles(rdp.cur_tile).s_scale, v.ov*rdp.tiles(rdp.cur_tile).t_scale, v.w, v.r, v.g, v.b, v.a);
        }
        else
        {
            v.oow = v.w = 1.0f;
            WriteTrace(TraceRDP, TraceDebug, "v%d - sx: %f, sy: %f r=%d, g=%d, b=%d, a=%d", i, v.sx / rdp.scale_x, v.sy / rdp.scale_y, v.r, v.g, v.b, v.a);
        }
        addr += vsize;
    }
    //*
    gfxVERTEX *pV[4] = {
        &vtx[0],
        &vtx[1],
        &vtx[2],
        &vtx[3]
    };
    if (vnum == 3)
    {
        WriteTrace(TraceRDP, TraceDebug, "uc9:Tri #%d, #%d", rdp.tri_n, rdp.tri_n + 1);
        draw_tri(pV, 0);
        rdp.tri_n++;
    }
    else
    {
        WriteTrace(TraceRDP, TraceDebug, "uc9:Quad #%d, #%d", rdp.tri_n, rdp.tri_n + 1);
        draw_tri(pV, 0);
        draw_tri(pV + 1, 0);
        rdp.tri_n += 2;
    }
}

static uint32_t uc9_load_object(uint32_t zHeader, uint32_t * rdpcmds)
{
    uint32_t type = zHeader & 7;
    uint8_t * addr = gfx.RDRAM + (zHeader & 0xFFFFFFF8);
    switch (type) {
    case 1: //sh tri
    case 3: //sh quad
    {
        rdp.cmd1 = ((uint32_t*)addr)[1];
        if (rdp.cmd1 != rdpcmds[0])
        {
            rdpcmds[0] = rdp.cmd1;
            uc9_rpdcmd();
        }
        update();
        uc9_draw_object(addr + 8, type);
    }
    break;
    case 0: //null
    case 2: //tx tri
    case 4: //tx quad
    {
        rdp.cmd1 = ((uint32_t*)addr)[1];
        if (rdp.cmd1 != rdpcmds[0])
        {
            rdpcmds[0] = rdp.cmd1;
            uc9_rpdcmd();
        }
        rdp.cmd1 = ((uint32_t*)addr)[2];
        if (rdp.cmd1 != rdpcmds[1])
        {
            uc9_rpdcmd();
            rdpcmds[1] = rdp.cmd1;
        }
        rdp.cmd1 = ((uint32_t*)addr)[3];
        if (rdp.cmd1 != rdpcmds[2])
        {
            uc9_rpdcmd();
            rdpcmds[2] = rdp.cmd1;
        }
        if (type)
        {
            update();
            uc9_draw_object(addr + 16, type);
        }
    }
    break;
    }
    return segoffset(((uint32_t*)addr)[0]);
}

void uc9_object()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:object");
    uint32_t rdpcmds[3] = { 0, 0, 0 };
    uint32_t cmd1 = rdp.cmd1;
    uint32_t zHeader = segoffset(rdp.cmd0);
    while (zHeader)
        zHeader = uc9_load_object(zHeader, rdpcmds);
    zHeader = segoffset(cmd1);
    while (zHeader)
        zHeader = uc9_load_object(zHeader, rdpcmds);
}

void uc9_mix()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:mix IGNORED");
}

void uc9_fmlight()
{
    int mid = rdp.cmd0 & 0xFF;
    rdp.num_lights = 1 + ((rdp.cmd1 >> 12) & 0xFF);
    uint32_t a = -1024 + (rdp.cmd1 & 0xFFF);
    WriteTrace(TraceRDP, TraceDebug, "uc9:fmlight matrix: %d, num: %d, dmem: %04lx", mid, rdp.num_lights, a);

    M44 *m = NULL;
    switch (mid) {
    case 4:
        m = (M44*)rdp.model;
        break;
    case 6:
        m = (M44*)rdp.proj;
        break;
    case 8:
        m = (M44*)rdp.combined;
        break;
    default:
        m = NULL; /* allowing segfaults to debug in case of PJGlide64 bugs */
        WriteTrace(TraceRDP, TraceWarning, "Invalid FM light matrix ID %u.", mid);
        break;
    }

    rdp.light[rdp.num_lights].r = (float)(((uint8_t*)gfx.DMEM)[(a + 0) ^ 3]) / 255.0f;
    rdp.light[rdp.num_lights].g = (float)(((uint8_t*)gfx.DMEM)[(a + 1) ^ 3]) / 255.0f;
    rdp.light[rdp.num_lights].b = (float)(((uint8_t*)gfx.DMEM)[(a + 2) ^ 3]) / 255.0f;
    rdp.light[rdp.num_lights].a = 1.0f;
    WriteTrace(TraceRDP, TraceDebug, "ambient light: r: %.3f, g: %.3f, b: %.3f", rdp.light[rdp.num_lights].r, rdp.light[rdp.num_lights].g, rdp.light[rdp.num_lights].b);
    a += 8;
    uint32_t i;
    for (i = 0; i < rdp.num_lights; i++)
    {
        rdp.light[i].r = (float)(((uint8_t*)gfx.DMEM)[(a + 0) ^ 3]) / 255.0f;
        rdp.light[i].g = (float)(((uint8_t*)gfx.DMEM)[(a + 1) ^ 3]) / 255.0f;
        rdp.light[i].b = (float)(((uint8_t*)gfx.DMEM)[(a + 2) ^ 3]) / 255.0f;
        rdp.light[i].a = 1.0f;
        rdp.light[i].dir_x = (float)(((char*)gfx.DMEM)[(a + 8) ^ 3]) / 127.0f;
        rdp.light[i].dir_y = (float)(((char*)gfx.DMEM)[(a + 9) ^ 3]) / 127.0f;
        rdp.light[i].dir_z = (float)(((char*)gfx.DMEM)[(a + 10) ^ 3]) / 127.0f;
        WriteTrace(TraceRDP, TraceDebug, "light: n: %d, r: %.3f, g: %.3f, b: %.3f, x: %.3f, y: %.3f, z: %.3f",
            i, rdp.light[i].r, rdp.light[i].g, rdp.light[i].b,
            rdp.light[i].dir_x, rdp.light[i].dir_y, rdp.light[i].dir_z);
        //    TransformVector(&rdp.light[i].dir_x, rdp.light_vector[i], *m);
        InverseTransformVector(&rdp.light[i].dir_x, rdp.light_vector[i], *m);
        NormalizeVector(rdp.light_vector[i]);
        WriteTrace(TraceRDP, TraceDebug, "light vector: n: %d, x: %.3f, y: %.3f, z: %.3f",
            i, rdp.light_vector[i][0], rdp.light_vector[i][1], rdp.light_vector[i][2]);
        a += 24;
    }
    for (i = 0; i < 2; i++)
    {
        float dir_x = (float)(((char*)gfx.DMEM)[(a + 8) ^ 3]) / 127.0f;
        float dir_y = (float)(((char*)gfx.DMEM)[(a + 9) ^ 3]) / 127.0f;
        float dir_z = (float)(((char*)gfx.DMEM)[(a + 10) ^ 3]) / 127.0f;
        if (sqrt(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z) < 0.98)
        {
            rdp.use_lookat = FALSE;
            return;
        }
        rdp.lookat[i][0] = dir_x;
        rdp.lookat[i][1] = dir_y;
        rdp.lookat[i][2] = dir_z;
        a += 24;
    }
    rdp.use_lookat = TRUE;
}

void uc9_light()
{
    uint32_t csrs = -1024 + ((rdp.cmd0 >> 12) & 0xFFF);
    uint32_t nsrs = -1024 + (rdp.cmd0 & 0xFFF);
    uint32_t num = 1 + ((rdp.cmd1 >> 24) & 0xFF);
    uint32_t cdest = -1024 + ((rdp.cmd1 >> 12) & 0xFFF);
    uint32_t tdest = -1024 + (rdp.cmd1 & 0xFFF);
    int use_material = (csrs != 0x0ff0);
    tdest >>= 1;
    WriteTrace(TraceRDP, TraceDebug, "uc9:light n: %d, colsrs: %04lx, normales: %04lx, coldst: %04lx, texdst: %04lx", num, csrs, nsrs, cdest, tdest);
    gfxVERTEX v;
    for (uint32_t i = 0; i < num; i++)
    {
        v.vec[0] = ((char*)gfx.DMEM)[(nsrs++) ^ 3];
        v.vec[1] = ((char*)gfx.DMEM)[(nsrs++) ^ 3];
        v.vec[2] = ((char*)gfx.DMEM)[(nsrs++) ^ 3];
        calc_sphere(v);
        //    calc_linear (&v);
        NormalizeVector(v.vec);
        calc_light(v);
        v.a = 0xFF;
        if (use_material)
        {
            v.r = (uint8_t)(((uint32_t)v.r * gfx.DMEM[(csrs++) ^ 3]) >> 8);
            v.g = (uint8_t)(((uint32_t)v.g * gfx.DMEM[(csrs++) ^ 3]) >> 8);
            v.b = (uint8_t)(((uint32_t)v.b * gfx.DMEM[(csrs++) ^ 3]) >> 8);
            v.a = gfx.DMEM[(csrs++) ^ 3];
        }
        gfx.DMEM[(cdest++) ^ 3] = v.r;
        gfx.DMEM[(cdest++) ^ 3] = v.g;
        gfx.DMEM[(cdest++) ^ 3] = v.b;
        gfx.DMEM[(cdest++) ^ 3] = v.a;
        ((short*)gfx.DMEM)[(tdest++) ^ 1] = (short)v.ou;
        ((short*)gfx.DMEM)[(tdest++) ^ 1] = (short)v.ov;
    }
}

void uc9_mtxtrnsp()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:mtxtrnsp - ignored");
    /*
    WriteTrace(TraceRDP, TraceDebug, "uc9:mtxtrnsp ");
    M44 *s;
    switch (rdp.cmd1&0xF) {
    case 4:
    s = (M44*)rdp.model;
    WriteTrace(TraceRDP, TraceDebug, "Model");
    break;
    case 6:
    s = (M44*)rdp.proj;
    WriteTrace(TraceRDP, TraceDebug, "Proj");
    break;
    case 8:
    s = (M44*)rdp.combined;
    WriteTrace(TraceRDP, TraceDebug, "Comb");
    break;
    }
    float m = *s[1][0];
    *s[1][0] = *s[0][1];
    *s[0][1] = m;
    m = *s[2][0];
    *s[2][0] = *s[0][2];
    *s[0][2] = m;
    m = *s[2][1];
    *s[2][1] = *s[1][2];
    *s[1][2] = m;
    */
}

void uc9_mtxcat()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:mtxcat ");
    M44 *s = NULL;
    M44 *t = NULL;
    uint32_t S = rdp.cmd0 & 0xF;
    uint32_t T = (rdp.cmd1 >> 16) & 0xF;
    uint32_t D = rdp.cmd1 & 0xF;
    switch (S) {
    case 4:
        s = (M44*)rdp.model;
        WriteTrace(TraceRDP, TraceDebug, "Model * ");
        break;
    case 6:
        s = (M44*)rdp.proj;
        WriteTrace(TraceRDP, TraceDebug, "Proj * ");
        break;
    case 8:
        s = (M44*)rdp.combined;
        WriteTrace(TraceRDP, TraceDebug, "Comb * ");
        break;
    default:
        WriteTrace(TraceRDP, TraceWarning, "Invalid mutex S-coordinate:  %u", S);
        s = NULL; /* intentional segfault to alert for bugs in PJGlide64 (cxd4) */
        break;
    }
    switch (T) {
    case 4:
        t = (M44*)rdp.model;
        WriteTrace(TraceRDP, TraceDebug, "Model -> ");
        break;
    case 6:
        t = (M44*)rdp.proj;
        WriteTrace(TraceRDP, TraceDebug, "Proj -> ");
        break;
    case 8:
        WriteTrace(TraceRDP, TraceDebug, "Comb -> ");
        t = (M44*)rdp.combined;
        break;
    default:
        WriteTrace(TraceRDP, TraceWarning, "Invalid mutex T-coordinate:  %u", T);
        t = NULL; /* intentional segfault to alert for bugs in PJGlide64 (cxd4) */
        break;
    }
    DECLAREALIGN16VAR(m[4][4]);
    MulMatrices(*s, *t, m);

    switch (D) {
    case 4:
        memcpy(rdp.model, m, 64);;
        WriteTrace(TraceRDP, TraceDebug, "Model");
        break;
    case 6:
        memcpy(rdp.proj, m, 64);;
        WriteTrace(TraceRDP, TraceDebug, "Proj");
        break;
    case 8:
        memcpy(rdp.combined, m, 64);;
        WriteTrace(TraceRDP, TraceDebug, "Comb");
        break;
    }
    WriteTrace(TraceRDP, TraceVerbose, "\nmodel\n{%f,%f,%f,%f}", rdp.model[0][0], rdp.model[0][1], rdp.model[0][2], rdp.model[0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.model[1][0], rdp.model[1][1], rdp.model[1][2], rdp.model[1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.model[2][0], rdp.model[2][1], rdp.model[2][2], rdp.model[2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.model[3][0], rdp.model[3][1], rdp.model[3][2], rdp.model[3][3]);
    WriteTrace(TraceRDP, TraceVerbose, "\nproj\n{%f,%f,%f,%f}", rdp.proj[0][0], rdp.proj[0][1], rdp.proj[0][2], rdp.proj[0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.proj[1][0], rdp.proj[1][1], rdp.proj[1][2], rdp.proj[1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.proj[2][0], rdp.proj[2][1], rdp.proj[2][2], rdp.proj[2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.proj[3][0], rdp.proj[3][1], rdp.proj[3][2], rdp.proj[3][3]);
    WriteTrace(TraceRDP, TraceVerbose, "\ncombined\n{%f,%f,%f,%f}", rdp.combined[0][0], rdp.combined[0][1], rdp.combined[0][2], rdp.combined[0][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[1][0], rdp.combined[1][1], rdp.combined[1][2], rdp.combined[1][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[2][0], rdp.combined[2][1], rdp.combined[2][2], rdp.combined[2][3]);
    WriteTrace(TraceRDP, TraceVerbose, "{%f,%f,%f,%f}", rdp.combined[3][0], rdp.combined[3][1], rdp.combined[3][2], rdp.combined[3][3]);
}

typedef struct {
    short sy;
    short sx;
    int   invw;
    short yi;
    short xi;
    short wi;
    uint8_t fog;
    uint8_t cc;
} zSortVDest;

void uc9_mult_mpmtx()
{
    //int id = rdp.cmd0&0xFF;
    int num = 1 + ((rdp.cmd1 >> 24) & 0xFF);
    int src = -1024 + ((rdp.cmd1 >> 12) & 0xFFF);
    int dst = -1024 + (rdp.cmd1 & 0xFFF);
    WriteTrace(TraceRDP, TraceDebug, "uc9:mult_mpmtx from: %04lx  to: %04lx n: %d", src, dst, num);
    short * saddr = (short*)(gfx.DMEM + src);
    zSortVDest * daddr = (zSortVDest*)(gfx.DMEM + dst);
    int idx = 0;
    zSortVDest v;
    memset(&v, 0, sizeof(zSortVDest));
    //float scale_x = 4.0f/rdp.scale_x;
    //float scale_y = 4.0f/rdp.scale_y;
    for (int i = 0; i < num; i++)
    {
        short sx = saddr[(idx++) ^ 1];
        short sy = saddr[(idx++) ^ 1];
        short sz = saddr[(idx++) ^ 1];
        float x = sx*rdp.combined[0][0] + sy*rdp.combined[1][0] + sz*rdp.combined[2][0] + rdp.combined[3][0];
        float y = sx*rdp.combined[0][1] + sy*rdp.combined[1][1] + sz*rdp.combined[2][1] + rdp.combined[3][1];
        float z = sx*rdp.combined[0][2] + sy*rdp.combined[1][2] + sz*rdp.combined[2][2] + rdp.combined[3][2];
        float w = sx*rdp.combined[0][3] + sy*rdp.combined[1][3] + sz*rdp.combined[2][3] + rdp.combined[3][3];
        v.sx = (short)(zSortRdp.view_trans[0] + x / w * zSortRdp.view_scale[0]);
        v.sy = (short)(zSortRdp.view_trans[1] + y / w * zSortRdp.view_scale[1]);

        v.xi = (short)x;
        v.yi = (short)y;
        v.wi = (short)w;
        v.invw = Calc_invw((int)(w * 31.0));

        if (w < 0.0f)
            v.fog = 0;
        else
        {
            int fog = (int)(z / w * rdp.fog_multiplier + rdp.fog_offset);
            if (fog > 255)
                fog = 255;
            v.fog = (fog >= 0) ? (uint8_t)fog : 0;
        }

        v.cc = 0;
        if (x < -w) v.cc |= 0x10;
        if (x > w) v.cc |= 0x01;
        if (y < -w) v.cc |= 0x20;
        if (y > w) v.cc |= 0x02;
        if (w < 0.1f) v.cc |= 0x04;

        daddr[i] = v;
        //memcpy(gfx.DMEM+dst+sizeof(zSortVDest)*i, &v, sizeof(zSortVDest));
        //    WriteTrace(TraceRDP, TraceDebug, "v%d x: %d, y: %d, z: %d -> sx: %d, sy: %d, w: %d, xi: %d, yi: %d, wi: %d, fog: %d", i, sx, sy, sz, v.sx, v.sy, v.invw, v.xi, v.yi, v.wi, v.fog);
        WriteTrace(TraceRDP, TraceDebug, "v%d x: %d, y: %d, z: %d -> sx: %04lx, sy: %04lx, invw: %08lx - %f, xi: %04lx, yi: %04lx, wi: %04lx, fog: %04lx", i, sx, sy, sz, v.sx, v.sy, v.invw, w, v.xi, v.yi, v.wi, v.fog);
    }
}

void uc9_link_subdl()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:link_subdl IGNORED");
}

void uc9_set_subdl()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:set_subdl IGNORED");
}

void uc9_wait_signal()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:wait_signal IGNORED");
}

void uc9_send_signal()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:send_signal IGNORED");
}

void uc9_movemem()
{
    WriteTrace(TraceRDP, TraceDebug, "uc9:movemem");
    int idx = rdp.cmd0 & 0x0E;
    int ofs = ((rdp.cmd0 >> 6) & 0x1ff) << 3;
    int len = (1 + ((rdp.cmd0 >> 15) & 0x1ff)) << 3;
    WriteTrace(TraceRDP, TraceDebug, "uc9:movemem ofs: %d, len: %d. ", ofs, len);
    int flag = rdp.cmd0 & 0x01;
    uint32_t addr = segoffset(rdp.cmd1);
    switch (idx)
    {
    case 0: //save/load
        if (flag == 0)
        {
            int dmem_addr = (idx << 3) + ofs;
            WriteTrace(TraceRDP, TraceDebug, "Load to DMEM. %08lx -> %08lx", addr, dmem_addr);
            memcpy(gfx.DMEM + dmem_addr, gfx.RDRAM + addr, len);
        }
        else
        {
            int dmem_addr = (idx << 3) + ofs;
            WriteTrace(TraceRDP, TraceDebug, "Load from DMEM. %08lx -> %08lx", dmem_addr, addr);
            memcpy(gfx.RDRAM + addr, gfx.DMEM + dmem_addr, len);
        }
        break;

    case 4:  // model matrix
    case 6:  // projection matrix
    case 8:  // combined matrix
    {
        DECLAREALIGN16VAR(m[4][4]);
        load_matrix(m, addr);
        switch (idx)
        {
        case 4:  // model matrix
            WriteTrace(TraceRDP, TraceDebug, "Modelview load");
            modelview_load(m);
            break;
        case 6:  // projection matrix
            WriteTrace(TraceRDP, TraceDebug, "Projection load");
            projection_load(m);
            break;
        case 8:  // projection matrix
            WriteTrace(TraceRDP, TraceDebug, "Combined load");
            rdp.update &= ~UPDATE_MULT_MAT;
            memcpy(rdp.combined, m, 64);;
            break;
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
    break;

    case 10:
        WriteTrace(TraceRDP, TraceDebug, "Othermode - IGNORED");
        break;

    case 12:   // VIEWPORT
    {
        uint32_t a = addr >> 1;
        short scale_x = ((short*)gfx.RDRAM)[(a + 0) ^ 1] >> 2;
        short scale_y = ((short*)gfx.RDRAM)[(a + 1) ^ 1] >> 2;
        short scale_z = ((short*)gfx.RDRAM)[(a + 2) ^ 1];
        rdp.fog_multiplier = ((short*)gfx.RDRAM)[(a + 3) ^ 1];
        short trans_x = ((short*)gfx.RDRAM)[(a + 4) ^ 1] >> 2;
        short trans_y = ((short*)gfx.RDRAM)[(a + 5) ^ 1] >> 2;
        short trans_z = ((short*)gfx.RDRAM)[(a + 6) ^ 1];
        rdp.fog_offset = ((short*)gfx.RDRAM)[(a + 7) ^ 1];
        rdp.view_scale[0] = scale_x * rdp.scale_x;
        rdp.view_scale[1] = scale_y * rdp.scale_y;
        rdp.view_scale[2] = 32.0f * scale_z;
        rdp.view_trans[0] = trans_x * rdp.scale_x;
        rdp.view_trans[1] = trans_y * rdp.scale_y;
        rdp.view_trans[2] = 32.0f * trans_z;
        zSortRdp.view_scale[0] = (float)(scale_x * 4);
        zSortRdp.view_scale[1] = (float)(scale_y * 4);
        zSortRdp.view_trans[0] = (float)(trans_x * 4);
        zSortRdp.view_trans[1] = (float)(trans_y * 4);
        zSortRdp.scale_x = rdp.scale_x / 4.0f;
        zSortRdp.scale_y = rdp.scale_y / 4.0f;

        rdp.update |= UPDATE_VIEWPORT;

        rdp.mipmap_level = 0;
        rdp.cur_tile = 0;
        TILE *tmp_tile = &rdp.tiles(0);
        tmp_tile->on = 1;
        tmp_tile->org_s_scale = 0xFFFF;
        tmp_tile->org_t_scale = 0xFFFF;
        tmp_tile->s_scale = 0.031250f;
        tmp_tile->t_scale = 0.031250f;

        rdp.geom_mode |= 0x0200;

        WriteTrace(TraceRDP, TraceDebug, "viewport scale(%d, %d, %d), trans(%d, %d, %d), from:%08lx", scale_x, scale_y, scale_z,
            trans_x, trans_y, trans_z, a);
        WriteTrace(TraceRDP, TraceDebug, "fog: multiplier: %f, offset: %f", rdp.fog_multiplier, rdp.fog_offset);
    }
    break;

    default:
        WriteTrace(TraceRDP, TraceDebug, "** UNKNOWN %d", idx);
    }
}

void uc9_setscissor()
{
    rdp_setscissor();

    if ((rdp.scissor_o.lr_x - rdp.scissor_o.ul_x) > (zSortRdp.view_scale[0] - zSortRdp.view_trans[0]))
    {
        float w = (rdp.scissor_o.lr_x - rdp.scissor_o.ul_x) / 2.0f;
        float h = (rdp.scissor_o.lr_y - rdp.scissor_o.ul_y) / 2.0f;
        rdp.view_scale[0] = w * rdp.scale_x;
        rdp.view_scale[1] = h * rdp.scale_y;
        rdp.view_trans[0] = w * rdp.scale_x;
        rdp.view_trans[1] = h * rdp.scale_y;
        zSortRdp.view_scale[0] = w * 4.0f;
        zSortRdp.view_scale[1] = h * 4.0f;
        zSortRdp.view_trans[0] = w * 4.0f;
        zSortRdp.view_trans[1] = h * 4.0f;
        zSortRdp.scale_x = rdp.scale_x / 4.0f;
        zSortRdp.scale_y = rdp.scale_y / 4.0f;
        rdp.update |= UPDATE_VIEWPORT;

        rdp.mipmap_level = 0;
        rdp.cur_tile = 0;
        TILE *tmp_tile = &rdp.tiles(0);
        tmp_tile->on = 1;
        tmp_tile->org_s_scale = 0xFFFF;
        tmp_tile->org_t_scale = 0xFFFF;
        tmp_tile->s_scale = 0.031250f;
        tmp_tile->t_scale = 0.031250f;

        rdp.geom_mode |= 0x0200;
    }
}