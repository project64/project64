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
#pragma once

#include <Common/stdtypes.h>
#include <Project64-video/Renderer/Renderer.h>

extern char out_buf[2048];

extern uint32_t frame_count; // frame counter

//GlideHQ support
#include "Ext_TxFilter.h"

enum
{
    MAX_CACHE = 1024 * 4,
    MAX_TRI_CACHE = 768, // this is actually # of vertices, not triangles
    MAX_VTX = 256,
    MAX_TMU = 2,
};

#define MAXCMD 0x100000
const unsigned int maxCMDMask = MAXCMD - 1;

#define TEXMEM_2MB_EDGE 2097152

// Supported flags
#define SUP_TEXMIRROR 0x00000001

// Clipping flags
#define CLIP_XMAX 0x00000001
#define CLIP_XMIN 0x00000002
#define CLIP_YMAX 0x00000004
#define CLIP_YMIN 0x00000008
#define CLIP_WMIN 0x00000010
#define CLIP_ZMAX 0x00000020
#define CLIP_ZMIN 0x00000040

// Flags
#define ZBUF_ENABLED  0x00000001
#define ZBUF_DECAL    0x00000002
#define ZBUF_COMPARE  0x00000004
#define ZBUF_UPDATE   0x00000008
#define ALPHA_COMPARE 0x00000010
#define FORCE_BL      0x00000020
#define CULL_FRONT    0x00001000  // * must be here
#define CULL_BACK     0x00002000  // * must be here
#define FOG_ENABLED   0x00010000

#define CULLMASK    0x00003000
#define CULLSHIFT   12

// Update flags
#define UPDATE_ZBUF_ENABLED 0x00000001

#define UPDATE_TEXTURE    0x00000002  // \ Same thing!
#define UPDATE_COMBINE    0x00000002  // /

#define UPDATE_CULL_MODE  0x00000004
#define UPDATE_LIGHTS     0x00000010
#define UPDATE_BIASLEVEL  0x00000020
#define UPDATE_ALPHA_COMPARE  0x00000040
#define UPDATE_VIEWPORT   0x00000080
#define UPDATE_MULT_MAT   0x00000100
#define UPDATE_SCISSOR    0x00000200
#define UPDATE_FOG_ENABLED  0x00010000

#define CMB_MULT    0x00000001
#define CMB_SET     0x00000002
#define CMB_SUB     0x00000004
#define CMB_ADD     0x00000008
#define CMB_A_MULT  0x00000010
#define CMB_A_SET   0x00000020
#define CMB_A_SUB   0x00000040
#define CMB_A_ADD   0x00000080
#define CMB_SETSHADE_SHADEALPHA 0x00000100
#define CMB_INTER   0x00000200
#define CMB_MULT_OWN_ALPHA  0x00000400
#define CMB_COL_SUB_OWN  0x00000800

#define uc(x) coord[x<<1]
#define vc(x) coord[(x<<1)+1]

#if defined(_MSC_VER)
#define DECLAREALIGN16VAR(var) __declspec(align(16)) float var
#elif defined(__GNUG__)
#define DECLAREALIGN16VAR(var) float (var) __attribute__ ((aligned(16)))
#endif

// Clipping (scissors)
typedef struct
{
    uint32_t ul_x;
    uint32_t ul_y;
    uint32_t lr_x;
    uint32_t lr_y;
} SCISSOR;

typedef struct {
    uint16_t tile_ul_s;
    uint16_t tile_ul_t;
    uint16_t tile_width;
    uint16_t tile_height;
    uint16_t tex_width;
    uint16_t tex_size;
    uint32_t dxt;
} LOAD_TILE_INFO;

typedef struct
{
    int gamma_correction;
    int32_t gamma_table_size;
    uint32_t *gamma_table_r;
    uint32_t *gamma_table_g;
    uint32_t *gamma_table_b;
    uint32_t tmem_ptr[MAX_TMU];
    uint32_t tex_min_addr[MAX_TMU];
    uint32_t tex_max_addr[MAX_TMU];
} VOODOO;

// This structure is what is passed in by rdp:settextureimage
typedef struct {
    uint8_t format;  // format: ARGB, IA, ...
    uint8_t size;    // size: 4,8,16, or 32 bit
    uint16_t width;   // used in settextureimage
    uint32_t addr;   // address in RDRAM to load the texture from
    int set_by;  // 0-loadblock 1-loadtile
} TEXTURE_IMAGE;

// This structure is a tile descriptor (as used by rdp:settile and rdp:settilesize)
typedef struct
{
    // rdp:settile
    uint8_t format;  // format: ARGB, IA, ...
    uint8_t size;    // size: 4,8,16, or 32 bit
    uint16_t line;    // size of one row (x axis) in 64 bit words
    uint16_t t_mem;   // location in texture memory (in 64 bit words, max 512 (4MB))
    uint8_t palette; // palette # to use
    uint8_t clamp_t; // clamp or wrap (y axis)?
    uint8_t mirror_t;  // mirroring on (y axis)?
    uint8_t mask_t;  // mask to wrap around (ex: 5 would wrap around 32) (y axis)
    uint8_t shift_t; // ??? (scaling)
    uint8_t clamp_s; // clamp or wrap (x axis)?
    uint8_t mirror_s;  // mirroring on (x axis)?
    uint8_t mask_s;  // mask to wrap around (x axis)
    uint8_t shift_s; // ??? (scaling)

    // rdp:settilesize
    uint16_t ul_s;    // upper left s coordinate
    uint16_t ul_t;    // upper left t coordinate
    uint16_t lr_s;    // lower right s coordinate
    uint16_t lr_t;    // lower right t coordinate

    float f_ul_s;
    float f_ul_t;

    // these are set by loadtile
    uint16_t t_ul_s;    // upper left s coordinate
    uint16_t t_ul_t;    // upper left t coordinate
    uint16_t t_lr_s;    // lower right s coordinate
    uint16_t t_lr_t;    // lower right t coordinate

    uint32_t width;
    uint32_t height;

    // uc0:texture
    uint8_t on;
    float s_scale;
    float t_scale;

    uint16_t org_s_scale;
    uint16_t org_t_scale;
} TILE;

// This structure forms the lookup table for cached textures
typedef struct {
    uint32_t addr;     // address in RDRAM
    uint32_t crc;      // CRC check
    uint32_t palette;    // Palette #
    uint32_t width;    // width
    uint32_t height;   // height
    uint32_t format;   // format
    uint32_t size;     // size
    uint32_t last_used;  // what frame # was this texture last used (used for replacing)

    uint32_t line;

    uint32_t flags;    // clamp/wrap/mirror flags

    uint32_t realwidth;  // width of actual texture
    uint32_t realheight; // height of actual texture
    uint32_t lod;
    uint32_t aspect;

    uint8_t set_by;
    uint8_t texrecting;

    int f_mirror_s;
    int f_mirror_t;
    int f_wrap_s;
    int f_wrap_t;

    float scale_x;    // texture scaling
    float scale_y;
    float scale;    // general scale to 256

    gfxTexInfo t_info; // texture info (glide)
    uint32_t tmem_addr;  // addres in texture memory (glide)

    int uses;   // 1 triangle that uses this texture

    int splits;   // number of splits
    int splitheight;

    float c_off;  // ul center texel offset (both x and y)
    float c_scl_x;  // scale to lower-right center-texel x
    float c_scl_y;  // scale to lower-right center-texel y

    uint32_t mod, mod_color, mod_color1, mod_color2, mod_factor;
    uint64_t ricecrc;
    int is_hires_tex;
} CACHE_LUT;

// Lights
typedef struct {
    float r, g, b, a;       // color
    float dir_x, dir_y, dir_z;  // direction towards the light source
    float x, y, z, w;  // light position
    float ca, la, qa;
    uint32_t nonblack;
    uint32_t nonzero;
} LIGHT;

typedef enum {
    ci_main,      //0, main color image
    ci_zimg,      //1, depth image
    ci_unknown,   //2, status is unknown
    ci_useless,   //3, status is unclear
    ci_old_copy,  //4, auxiliary color image, copy of last color image from previous frame
    ci_copy,      //5, auxiliary color image, copy of previous color image
    ci_copy_self, //6, main color image, it's content will be used to draw into itself
    ci_zcopy,     //7, auxiliary color image, copy of depth image
    ci_aux,       //8, auxiliary color image
    ci_aux_copy   //9, auxiliary color image, partial copy of previous color image
} CI_STATUS;

// Frame buffers
typedef struct
{
    uint32_t addr;   //color image address
    uint8_t format;
    uint8_t size;
    uint16_t width;
    uint16_t height;
    CI_STATUS status;
    int   changed;
} COLOR_IMAGE;

typedef struct
{
    gfxChipID_t tmu;
    uint32_t addr;  //address of color image
    uint32_t end_addr;
    uint32_t tex_addr; //address in video memory
    uint32_t width;    //width of color image
    uint32_t height;   //height of color image
    uint8_t  format;   //format of color image
    uint8_t  size;   //format of color image
    uint8_t  clear;  //flag. texture buffer must be cleared
    uint8_t  drawn;  //flag. if equal to 1, this image was already drawn in current frame
    uint32_t crc; //checksum of the color image
    float scr_width; //width of rendered image
    float scr_height; //height of rendered image
    uint32_t tex_width;  //width of texture buffer
    uint32_t tex_height; //height of texture buffer
    int   tile;     //
    uint16_t  tile_uls; //shift from left bound of the texture
    uint16_t  tile_ult; //shift from top of the texture
    uint32_t v_shift; //shift from top of the texture
    uint32_t u_shift; //shift from left of the texture
    float lr_u;
    float lr_v;
    float u_scale; //used to map vertex u,v coordinates into hires texture
    float v_scale; //used to map vertex u,v coordinates into hires texture
    CACHE_LUT * cache; //pointer to texture cache item
    gfxTexInfo info;
    uint16_t t_mem;
} TBUFF_COLOR_IMAGE;

typedef struct
{
    gfxChipID_t tmu;
    uint32_t begin; //start of the block in video memory
    uint32_t end;   //end of the block in video memory
    uint8_t count;  //number of allocated texture buffers
    int clear_allowed; //stack of buffers can be cleared
    TBUFF_COLOR_IMAGE images[256];
} TEXTURE_BUFFER;

#define NUMTEXBUF 92

class CRDP
{
public:
    CRDP();
    ~CRDP();

    bool init();
    void free();

    inline gfxVERTEX & vtx(int index) const { return m_vtx[index]; }
    inline TILE & tiles(int index) { return m_tiles[index]; }
    // Clipping
    int clip;     // clipping flags
    gfxVERTEX *vtx1; //[256] copy vertex buffer #1 (used for clipping)
    gfxVERTEX *vtx2; //[256] copy vertex buffer #2
    gfxVERTEX *vtxbuf;   // current vertex buffer (reset to vtx, used to determine current vertex buffer)
    gfxVERTEX *vtxbuf2;
    int n_global;   // Used to pass the number of vertices from clip_z to clip_tri
    int vtx_buffer;

    CACHE_LUT *cache[MAX_TMU]; //[MAX_CACHE]
    CACHE_LUT *cur_cache[MAX_TMU];
    uint32_t   cur_cache_n[MAX_TMU];
    int     n_cached[MAX_TMU];

    // Vertices
private:
    gfxVERTEX * m_vtx; //[MAX_VTX]
public:
    int v0, vn;

    COLOR_IMAGE *frame_buffers; //[NUMTEXBUF+2]
    TEXTURE_BUFFER texbufs[2];
    char RomName[21];
    float vi_width;
    float vi_height;

    int window_changed;

    float offset_x, offset_y, offset_x_bak, offset_y_bak;

    float scale_x, scale_x_bak;
    float scale_y, scale_y_bak;

    float view_scale[3];
    float view_trans[3];
    float clip_min_x, clip_max_x, clip_min_y, clip_max_y;
    float clip_ratio;

    int updatescreen;

    uint32_t tri_n;  // triangle counter
    uint32_t debug_n;

    // Program counter
    uint32_t pc[10]; // DList PC stack
    uint32_t pc_i;   // current PC index in the stack
    int dl_count; // number of instructions before returning
    int LLE;

    // Segments
    uint32_t segment[16];  // Segment pointer

    // Marks the end of DList execution (done in uc?:enddl)
    bool halt;

    // Next command
    uint32_t cmd0;
    uint32_t cmd1;
    uint32_t cmd2;
    uint32_t cmd3;

    // Clipping
    SCISSOR scissor_o;
    SCISSOR scissor;
    int scissor_set;

    // Colors
    uint32_t fog_color;
    uint32_t fill_color;
    uint32_t prim_color;
    uint32_t blend_color;
    uint32_t env_color;
    uint32_t SCALE;
    uint32_t CENTER;
    uint32_t prim_lodmin, prim_lodfrac;
    uint16_t prim_depth;
    uint16_t prim_dz;
    uint8_t K4;
    uint8_t K5;
    enum {
        noise_none,
        noise_combine,
        noise_texture
    } noise;

    float col[4];   // color multiplier
    float coladd[4];  // color add/subtract
    float shade_factor;

    float col_2[4];

    uint32_t cmb_flags, cmb_flags_2;

    // othermode_l flags
    int acmp; // 0 = none, 1 = threshold, 2 = dither
    int zsrc; // 0 = pixel, 1 = prim
    uint8_t alpha_dither_mode;

    // Matrices
#pragma warning(push)
#pragma warning(disable:4324) //structure was padded due to __declspec(align())
    DECLAREALIGN16VAR(model[4][4]);
#pragma warning(pop)
    DECLAREALIGN16VAR(proj[4][4]);
    DECLAREALIGN16VAR(combined[4][4]);
    DECLAREALIGN16VAR(dkrproj[3][4][4]);

    DECLAREALIGN16VAR(model_stack[32][4][4]);  // 32 deep, will warn if overflow
    int model_i;          // index in the model matrix stack
    int model_stack_size;

    // Textures
    TEXTURE_IMAGE timg;       // 1 for each tmem address
private:
    TILE m_tiles[8];          // 8 tile descriptors
public:
    uint8_t tmem[4096];        // 4k tmem
    uint32_t addr[512];        // 512 addresses (used to determine address loaded from)
    LOAD_TILE_INFO load_info[512];    // 512 addresses. inforamation about tile loading.

    int     cur_tile;   // current tile
    int     mipmap_level;
    int     last_tile;   // last tile set
    int     last_tile_size;   // last tile size set

    gfxChipID_t t0, t1;
    int     best_tex; // if no 2-tmus, which texture? (0 or 1)
    int     tex;
    int     filter_mode;

    // Texture palette
    uint16_t pal_8[256];
    uint32_t pal_8_crc[16];
    uint32_t pal_256_crc;
    uint8_t tlut_mode;
    int LOD_en;
    int Persp_en;
    int persp_supported;
    int force_wrap;
    uint16_t pal_8_rice[512];

    // Lighting
    uint32_t num_lights;
    LIGHT light[12];
    float light_vector[12][3];
    float lookat[2][3];
    int  use_lookat;

    // Combine modes
    uint32_t cycle1, cycle2, cycle_mode;
    uint8_t c_a0, c_b0, c_c0, c_d0, c_Aa0, c_Ab0, c_Ac0, c_Ad0;
    uint8_t c_a1, c_b1, c_c1, c_d1, c_Aa1, c_Ab1, c_Ac1, c_Ad1;

    uint8_t fbl_a0, fbl_b0, fbl_c0, fbl_d0;
    uint8_t fbl_a1, fbl_b1, fbl_c1, fbl_d1;

    uint8_t uncombined;  // which is uncombined: 0x01=color 0x02=alpha 0x03=both

    //  float YUV_C0, YUV_C1, YUV_C2, YUV_C3, YUV_C4; //YUV textures conversion coefficients

    // What needs updating
    uint32_t update;
    uint32_t flags;

    int first;

    uint32_t tex_ctr;    // incremented every time textures are updated

    int allow_combine; // allow combine updating?

    int s2dex_tex_loaded;
    uint16_t bg_image_height;

    // Debug stuff
    uint32_t rm; // use othermode_l instead, this just as a check for changes
    uint32_t render_mode_changed;
    uint32_t geom_mode;

    uint32_t othermode_h;
    uint32_t othermode_l;

    // used to check if in texrect while loading texture
    uint8_t texrecting;

    //frame buffer related slots. Added by Gonetz
    uint32_t cimg, ocimg, zimg, tmpzimg, vi_org_reg;
    COLOR_IMAGE maincimg[2];
    uint32_t last_drawn_ci_addr;
    uint32_t main_ci, main_ci_end, main_ci_bg, main_ci_last_tex_addr, zimg_end, last_bg;
    uint32_t ci_width, ci_height, ci_size, ci_end;
    uint32_t zi_width;
    int zi_lrx, zi_lry;
    uint8_t  ci_count, num_of_ci, main_ci_index, copy_ci_index, copy_zi_index;
    int swap_ci_index, black_ci_index;
    uint32_t ci_upper_bound, ci_lower_bound;
    int  motionblur, fb_drawn, fb_drawn_front, read_previous_ci, read_whole_frame;
    CI_STATUS ci_status;
    TBUFF_COLOR_IMAGE * cur_image;  //image currently being drawn
    TBUFF_COLOR_IMAGE * tbuff_tex;  //image, which corresponds to currently selected texture
    TBUFF_COLOR_IMAGE * aTBuffTex[2];
    uint8_t  cur_tex_buf;
    uint8_t  acc_tex_buf;
    int skip_drawing; //rendering is not required. used for frame buffer emulation

    //fog related slots. Added by Gonetz
    float fog_multiplier, fog_offset;
    enum {
        fog_disabled,
        fog_enabled,
        fog_blend,
        fog_blend_inverse
    }
    fog_mode;
};

void SetWireframeCol();
void ChangeSize();
void GoToFullScreen();

extern CRDP rdp;
extern VOODOO voodoo;

extern gfxTexInfo  fontTex;
extern gfxTexInfo  cursorTex;
extern uint32_t   offset_font;
extern uint32_t   offset_cursor;
extern uint32_t   offset_textures;
extern uint32_t   offset_texbuf1;

extern bool	g_ucode_error_report;

// RDP functions
void rdp_reset();

extern const char *ACmp[];
extern const char *Mode0[];
extern const char *Mode1[];
extern const char *Mode2[];
extern const char *Mode3[];
extern const char *Alpha0[];
#define Alpha1 Alpha0
extern const char *Alpha2[];
#define Alpha3 Alpha0
extern const char *FBLa[];
extern const char *FBLb[];
extern const char *FBLc[];
extern const char *FBLd[];
extern const char *str_zs[];
extern const char *str_yn[];
extern const char *str_offon[];
extern const char *str_cull[];
// I=intensity probably
extern const char *str_format[];
extern const char *str_size[];
extern const char *str_cm[];
extern const char *str_lod[];
extern const char *str_aspect[];
extern const char *str_filter[];
extern const char *str_tlut[];
extern const char *CIStatus[];

#define FBL_D_1 2
#define FBL_D_0 3

#ifndef maxval
#define maxval(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#ifndef minval
#define minval(a, b)       (((a) < (b)) ? (a) : (b))
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef HIWORD
#define HIWORD(a) ((unsigned int)(a) >> 16)
#endif
#ifndef LOWORD
#define LOWORD(a) ((a) & 0xFFFF)
#endif

// Convert from u0/v0/u1/v1 to the real coordinates without regard to tmu
__inline void ConvertCoordsKeep(gfxVERTEX *v, int n)
{
    for (int i = 0; i < n; i++)
    {
        v[i].uc(0) = v[i].u0;
        v[i].vc(0) = v[i].v0;
        v[i].uc(1) = v[i].u1;
        v[i].vc(1) = v[i].v1;
    }
}

// Convert from u0/v0/u1/v1 to the real coordinates based on the tmu they are on
__inline void ConvertCoordsConvert(gfxVERTEX *v, int n)
{
    for (int i = 0; i < n; i++)
    {
        v[i].uc(rdp.t0) = v[i].u0;
        v[i].vc(rdp.t0) = v[i].v0;
        v[i].uc(rdp.t1) = v[i].u1;
        v[i].vc(rdp.t1) = v[i].v1;
    }
}

__inline void AllowShadeMods(gfxVERTEX *v, int n)
{
    for (int i = 0; i < n; i++)
    {
        v[i].shade_mod = 0;
    }
}

__inline void AddOffset(gfxVERTEX *v, int n)
{
    for (int i = 0; i < n; i++)
    {
        v[i].x += rdp.offset_x;
        v[i].y += rdp.offset_y;
    }
}

__inline void CalculateFog(gfxVERTEX &v)
{
    if (rdp.flags & FOG_ENABLED)
    {
        if (v.w < 0.0f)
            v.f = 0.0f;
        else
            v.f = minval(255.0f, maxval(0.0f, v.z_w * rdp.fog_multiplier + rdp.fog_offset));
        v.a = (uint8_t)v.f;
    }
    else
    {
        v.f = 1.0f;
    }
}

void newSwapBuffers();
extern int SwapOK;

// ** utility functions
void load_palette(uint32_t addr, uint16_t start, uint16_t count);
void setTBufTex(uint16_t t_mem, uint32_t cnt);
// ** RDP graphics functions **
void undef();
void spnoop();

void rdp_noop();
void rdp_texrect();
//void rdp_texrectflip();
void rdp_loadsync();
void rdp_pipesync();
void rdp_tilesync();
void rdp_fullsync();
void rdp_setkeygb();
void rdp_setkeyr();
void rdp_setconvert();
void rdp_setscissor();
void rdp_setprimdepth();
void rdp_loadtlut();
void rdp_settilesize();
void rdp_loadblock();
void rdp_loadtile();
void rdp_settile();
void rdp_fillrect();
void rdp_setfillcolor();
void rdp_setfogcolor();
void rdp_setblendcolor();
void rdp_setprimcolor();
void rdp_setenvcolor();
void rdp_setcombine();
void rdp_settextureimage();
void rdp_setdepthimage();
void rdp_setcolorimage();
void rdp_setothermode();
void rdp_trifill();
void rdp_trishade();
void rdp_tritxtr();
void rdp_trishadetxtr();
void rdp_trifillz();
void rdp_trishadez();
void rdp_tritxtrz();
void rdp_trishadetxtrz();
void rdphalf_1();
void rdphalf_2();
void rdphalf_cont();

void rsp_reserved0();
void rsp_reserved1();
void rsp_reserved2();
void rsp_reserved3();

void ys_memrect();
void microcheck();

extern const char *str_dither[];
extern uint8_t microcode[4096];
