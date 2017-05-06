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
static void mod_tex_inter_color_using_factor_CI(uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    float percent_i = 1 - percent;
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (uint8_t)((color >> 24) & 0xFF);
    cg = (uint8_t)((color >> 16) & 0xFF);
    cb = (uint8_t)((color >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = (uint8_t)(minval(255, percent_i * r + percent * cr));
        g = (uint8_t)(minval(255, percent_i * g + percent * cg));
        b = (uint8_t)(minval(255, percent_i * b + percent * cb));
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_tex_inter_col_using_col1_CI(uint32_t color0, uint32_t color1)
{
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    float percent_r = ((color1 >> 24) & 0xFF) / 255.0f;
    float percent_g = ((color1 >> 16) & 0xFF) / 255.0f;
    float percent_b = ((color1 >> 8) & 0xFF) / 255.0f;
    float percent_r_i = 1.0f - percent_r;
    float percent_g_i = 1.0f - percent_g;
    float percent_b_i = 1.0f - percent_b;

    cr = (uint8_t)((color0 >> 24) & 0xFF);
    cg = (uint8_t)((color0 >> 16) & 0xFF);
    cb = (uint8_t)((color0 >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = (uint8_t)(minval(255, percent_r_i * r + percent_r * cr));
        g = (uint8_t)(minval(255, percent_g_i * g + percent_g * cg));
        b = (uint8_t)(minval(255, percent_b_i * b + percent_b * cb));
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_full_color_sub_tex_CI(uint32_t color)
{
    uint8_t cr, cg, cb, ca;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (uint8_t)((color >> 24) & 0xFF);
    cg = (uint8_t)((color >> 16) & 0xFF);
    cb = (uint8_t)((color >> 8) & 0xFF);
    ca = (uint8_t)(color & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        a = maxval(0, ca - a);
        r = maxval(0, cr - r);
        g = maxval(0, cg - g);
        b = maxval(0, cb - b);
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_col_inter_col1_using_tex_CI(uint32_t color0, uint32_t color1)
{
    uint32_t cr0, cg0, cb0, cr1, cg1, cb1;
    uint16_t col;
    uint8_t a, r, g, b;
    float percent_r, percent_g, percent_b;

    cr0 = (uint8_t)((color0 >> 24) & 0xFF);
    cg0 = (uint8_t)((color0 >> 16) & 0xFF);
    cb0 = (uint8_t)((color0 >> 8) & 0xFF);
    cr1 = (uint8_t)((color1 >> 24) & 0xFF);
    cg1 = (uint8_t)((color1 >> 16) & 0xFF);
    cb1 = (uint8_t)((color1 >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        percent_r = ((col & 0xF800) >> 11) / 31.0f;
        percent_g = ((col & 0x07C0) >> 6) / 31.0f;
        percent_b = ((col & 0x003E) >> 1) / 31.0f;
        r = (uint8_t)(minval((1.0f - percent_r) * cr0 + percent_r * cr1, 255));
        g = (uint8_t)(minval((1.0f - percent_g) * cg0 + percent_g * cg1, 255));
        b = (uint8_t)(minval((1.0f - percent_b) * cb0 + percent_b * cb1, 255));
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_tex_sub_col_mul_fac_add_tex_CI(uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    uint8_t cr, cg, cb, a;
    uint16_t col;
    float r, g, b;

    cr = (uint8_t)((color >> 24) & 0xFF);
    cg = (uint8_t)((color >> 16) & 0xFF);
    cb = (uint8_t)((color >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = (r - cr) * percent + r;
        if (r > 255.0f) r = 255.0f;
        if (r < 0.0f) r = 0.0f;
        g = (g - cg) * percent + g;
        if (g > 255.0f) g = 255.0f;
        if (g < 0.0f) g = 0.0f;
        b = (b - cb) * percent + b;
        if (b > 255.0f) g = 255.0f;
        if (b < 0.0f) b = 0.0f;
        rdp.pal_8[i] = (uint16_t)(((uint16_t)((uint8_t)(r) >> 3) << 11) |
            ((uint16_t)((uint8_t)(g) >> 3) << 6) |
            ((uint16_t)((uint8_t)(b) >> 3) << 1) |
            (uint16_t)(a));
    }
}

static void mod_tex_scale_col_add_col_CI(uint32_t color0, uint32_t color1)
{
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    float percent_r = ((color0 >> 24) & 0xFF) / 255.0f;
    float percent_g = ((color0 >> 16) & 0xFF) / 255.0f;
    float percent_b = ((color0 >> 8) & 0xFF) / 255.0f;
    cr = (uint8_t)((color1 >> 24) & 0xFF);
    cg = (uint8_t)((color1 >> 16) & 0xFF);
    cb = (uint8_t)((color1 >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = (uint8_t)(minval(255, percent_r * r + cr));
        g = (uint8_t)(minval(255, percent_g * g + cg));
        b = (uint8_t)(minval(255, percent_b * b + cb));
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_tex_add_col_CI(uint32_t color)
{
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (uint8_t)((color >> 24) & 0xFF);
    cg = (uint8_t)((color >> 16) & 0xFF);
    cb = (uint8_t)((color >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = minval(cr + r, 255);
        g = minval(cg + g, 255);
        b = minval(cb + b, 255);
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_tex_sub_col_CI(uint32_t color)
{
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (uint8_t)((color >> 24) & 0xFF);
    cg = (uint8_t)((color >> 16) & 0xFF);
    cb = (uint8_t)((color >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = maxval(r - cr, 0);
        g = maxval(g - cg, 0);
        b = maxval(b - cb, 0);
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_tex_sub_col_mul_fac_CI(uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a;
    float r, g, b;

    cr = (uint8_t)((color >> 24) & 0xFF);
    cg = (uint8_t)((color >> 16) & 0xFF);
    cb = (uint8_t)((color >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);
        r = (float)((col & 0xF800) >> 11) / 31.0f * 255.0f;
        g = (float)((col & 0x07C0) >> 6) / 31.0f * 255.0f;
        b = (float)((col & 0x003E) >> 1) / 31.0f * 255.0f;
        r = (r - cr) * percent;
        if (r > 255.0f) r = 255.0f;
        if (r < 0.0f) r = 0.0f;
        g = (g - cg) * percent;
        if (g > 255.0f) g = 255.0f;
        if (g < 0.0f) g = 0.0f;
        b = (b - cb) * percent;
        if (b > 255.0f) g = 255.0f;
        if (b < 0.0f) b = 0.0f;

        rdp.pal_8[i] = (uint16_t)(((uint16_t)((uint8_t)(r) >> 3) << 11) |
            ((uint16_t)((uint8_t)(g) >> 3) << 6) |
            ((uint16_t)((uint8_t)(b) >> 3) << 1) |
            (uint16_t)(a));
    }
}

static void mod_col_inter_tex_using_col1_CI(uint32_t color0, uint32_t color1)
{
    uint8_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    float percent_r = ((color1 >> 24) & 0xFF) / 255.0f;
    float percent_g = ((color1 >> 16) & 0xFF) / 255.0f;
    float percent_b = ((color1 >> 8) & 0xFF) / 255.0f;
    float percent_r_i = 1.0f - percent_r;
    float percent_g_i = 1.0f - percent_g;
    float percent_b_i = 1.0f - percent_b;

    cr = (uint8_t)((color0 >> 24) & 0xFF);
    cg = (uint8_t)((color0 >> 16) & 0xFF);
    cb = (uint8_t)((color0 >> 8) & 0xFF);

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) / 31.0f * 255.0f);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) / 31.0f * 255.0f);
        b = (uint8_t)((float)((col & 0x003E) >> 1) / 31.0f * 255.0f);
        r = (uint8_t)(minval(255, percent_r * r + percent_r_i * cr));
        g = (uint8_t)(minval(255, percent_g * g + percent_g_i * cg));
        b = (uint8_t)(minval(255, percent_b * b + percent_b_i * cb));
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void mod_tex_inter_col_using_texa_CI(uint32_t color)
{
    uint8_t a, r, g, b;

    r = (uint8_t)((float)((color >> 24) & 0xFF) / 255.0f * 31.0f);
    g = (uint8_t)((float)((color >> 16) & 0xFF) / 255.0f * 31.0f);
    b = (uint8_t)((float)((color >> 8) & 0xFF) / 255.0f * 31.0f);
    a = (color & 0xFF) ? 1 : 0;
    uint16_t col16 = (uint16_t)((r << 11) | (g << 6) | (b << 1) | a);

    for (int i = 0; i < 256; i++)
    {
        if (rdp.pal_8[i] & 1)
            rdp.pal_8[i] = col16;
    }
}

static void mod_tex_mul_col_CI(uint32_t color)
{
    uint8_t a, r, g, b;
    uint16_t col;
    float cr, cg, cb;

    cr = (float)((color >> 24) & 0xFF) / 255.0f;
    cg = (float)((color >> 16) & 0xFF) / 255.0f;
    cb = (float)((color >> 8) & 0xFF) / 255.0f;

    for (int i = 0; i < 256; i++)
    {
        col = rdp.pal_8[i];
        a = (uint8_t)(col & 0x0001);;
        r = (uint8_t)((float)((col & 0xF800) >> 11) * cr);
        g = (uint8_t)((float)((col & 0x07C0) >> 6) * cg);
        b = (uint8_t)((float)((col & 0x003E) >> 1) * cb);
        rdp.pal_8[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
            ((uint16_t)(g >> 3) << 6) |
            ((uint16_t)(b >> 3) << 1) |
            ((uint16_t)(a) << 0));
    }
}

static void ModifyPalette(uint32_t mod, uint32_t modcolor, uint32_t modcolor1, uint32_t modfactor)
{
    switch (mod)
    {
    case TMOD_TEX_INTER_COLOR_USING_FACTOR:
        mod_tex_inter_color_using_factor_CI(modcolor, modfactor);
        break;
    case TMOD_TEX_INTER_COL_USING_COL1:
        mod_tex_inter_col_using_col1_CI(modcolor, modcolor1);
        break;
    case TMOD_FULL_COLOR_SUB_TEX:
        mod_full_color_sub_tex_CI(modcolor);
        break;
    case TMOD_COL_INTER_COL1_USING_TEX:
        mod_col_inter_col1_using_tex_CI(modcolor, modcolor1);
        break;
    case TMOD_TEX_SUB_COL_MUL_FAC_ADD_TEX:
        mod_tex_sub_col_mul_fac_add_tex_CI(modcolor, modfactor);
        break;
    case TMOD_TEX_SCALE_COL_ADD_COL:
        mod_tex_scale_col_add_col_CI(modcolor, modcolor1);
        break;
    case TMOD_TEX_ADD_COL:
        mod_tex_add_col_CI(modcolor);
        break;
    case TMOD_TEX_SUB_COL:
        mod_tex_sub_col_CI(modcolor);
        break;
    case TMOD_TEX_SUB_COL_MUL_FAC:
        mod_tex_sub_col_mul_fac_CI(modcolor, modfactor);
        break;
    case TMOD_COL_INTER_TEX_USING_COL1:
        mod_col_inter_tex_using_col1_CI(modcolor, modcolor1);
        break;
    case TMOD_TEX_INTER_COL_USING_TEXA:
        mod_tex_inter_col_using_texa_CI(modcolor);
        break;
    case TMOD_TEX_MUL_COL:
        mod_tex_mul_col_CI(modcolor);
        break;
    default:
        ;
    }
}
