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
#ifndef COMBINE_H
#define COMBINE_H

// texture MOD types
#define TMOD_TEX_INTER_COLOR_USING_FACTOR			1
#define TMOD_TEX_INTER_COL_USING_COL1				2
#define TMOD_FULL_COLOR_SUB_TEX						3
#define TMOD_COL_INTER_COL1_USING_TEX				4
#define TMOD_COL_INTER_COL1_USING_TEXA				5
#define TMOD_COL_INTER_COL1_USING_TEXA__MUL_TEX		6
#define TMOD_COL_INTER_TEX_USING_TEXA				7
#define TMOD_COL2_INTER__COL_INTER_COL1_USING_TEX__USING_TEXA	8
#define TMOD_TEX_SCALE_FAC_ADD_FAC					9
#define TMOD_TEX_SUB_COL_MUL_FAC_ADD_TEX			10
#define TMOD_TEX_SCALE_COL_ADD_COL					11
#define TMOD_TEX_ADD_COL							12
#define TMOD_TEX_SUB_COL							13
#define TMOD_TEX_SUB_COL_MUL_FAC					14
#define TMOD_COL_INTER_TEX_USING_COL1				15
#define TMOD_COL_MUL_TEXA_ADD_TEX					16
#define TMOD_COL_INTER_TEX_USING_TEX				17
#define TMOD_TEX_INTER_NOISE_USING_COL				18
#define TMOD_TEX_INTER_COL_USING_TEXA				19
#define TMOD_TEX_MUL_COL				            20
#define TMOD_TEX_SCALE_FAC_ADD_COL					21

#define COMBINE_EXT_COLOR     1
#define COMBINE_EXT_ALPHA     2
#define TEX_COMBINE_EXT_COLOR 1
#define TEX_COMBINE_EXT_ALPHA 2

typedef struct
{
    uint32_t ccolor;  // constant color to set at the end, color and alpha
    gfxCombineFunction_t c_fnc;
    gfxCombineFactor_t c_fac;
    gfxCombineLocal_t c_loc;
    gfxCombineOther_t c_oth;  // grColorCombine flags
    gfxCombineFunction_t a_fnc;
    gfxCombineFactor_t a_fac;
    gfxCombineLocal_t a_loc;
    gfxCombineOther_t a_oth;  // gfxAlphaCombine flags
    gfxCombineFunction_t tmu0_func;
    gfxCombineFactor_t tmu0_fac;
    gfxCombineFunction_t tmu1_func;
    gfxCombineFactor_t tmu1_fac;
    gfxCombineFunction_t tmu0_a_func;
    gfxCombineFactor_t tmu0_a_fac;
    gfxCombineFunction_t tmu1_a_func;
    gfxCombineFactor_t tmu1_a_fac;
    uint32_t tex, tmu0_invert, tmu1_invert;
    uint32_t tmu0_a_invert, tmu1_a_invert;
    int   dc0_lodbias, dc1_lodbias;
    uint8_t  dc0_detailscale, dc1_detailscale;
    float dc0_detailmax, dc1_detailmax;
    float lodbias0, lodbias1;
    gfxAlphaBlendFnc_t abf1, abf2;
    uint32_t mod_0, modcolor_0, modcolor1_0, modcolor2_0, modfactor_0;
    uint32_t mod_1, modcolor_1, modcolor1_1, modcolor2_1, modfactor_1;
    //combine extensions
    uint32_t c_ext_a, c_ext_b, c_ext_c, c_ext_d;
    gfxCombineMode_t c_ext_a_mode;
    gfxCombineMode_t c_ext_b_mode;
    int  c_ext_c_invert, c_ext_d_invert;
    uint32_t a_ext_a, a_ext_b, a_ext_c, a_ext_d;
    gfxCombineMode_t a_ext_a_mode;
    gfxCombineMode_t a_ext_b_mode;
    int  a_ext_c_invert, a_ext_d_invert;
    uint32_t t0c_ext_a, t0c_ext_b, t0c_ext_c, t0c_ext_d;
    gfxCombineMode_t t0c_ext_a_mode;
    gfxCombineMode_t t0c_ext_b_mode;
    int  t0c_ext_c_invert, t0c_ext_d_invert;
    uint32_t t0a_ext_a, t0a_ext_b, t0a_ext_c, t0a_ext_d;
    gfxCombineMode_t t0a_ext_a_mode;
    gfxCombineMode_t t0a_ext_b_mode;
    int  t0a_ext_c_invert, t0a_ext_d_invert;
    uint32_t t1c_ext_a, t1c_ext_b, t1c_ext_c, t1c_ext_d;
    gfxCombineMode_t t1c_ext_a_mode;
    gfxCombineMode_t t1c_ext_b_mode;
    int  t1c_ext_c_invert, t1c_ext_d_invert;
    uint32_t t1a_ext_a, t1a_ext_b, t1a_ext_c, t1a_ext_d;
    gfxCombineMode_t t1a_ext_a_mode;
    gfxCombineMode_t t1a_ext_b_mode;
    int  t1a_ext_c_invert, t1a_ext_d_invert;
    uint32_t tex_ccolor;
    int combine_ext;
    uint8_t cmb_ext_use;
    uint8_t tex_cmb_ext_use;
    uint32_t shade_mod_hash;
} COMBINE;

extern COMBINE cmb;

void Combine();
void CombineBlender();
void CountCombine();
void InitCombine();
void ColorCombinerToExtension();
void AlphaCombinerToExtension();
void TexColorCombinerToExtension(gfxChipID_t tmu);
void TexAlphaCombinerToExtension(gfxChipID_t tmu);

#endif //COMBINE _H
