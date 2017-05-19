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
static void mod_tex_inter_color_using_factor(uint16_t *dst, int size, uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    float percent_i = 1 - percent;
    uint32_t cr, cg, cb;
    uint16_t col, a;
    uint8_t r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        r = (uint8_t)(percent_i * ((col >> 8) & 0xF) + percent * cr);
        g = (uint8_t)(percent_i * ((col >> 4) & 0xF) + percent * cg);
        b = (uint8_t)(percent_i * (col & 0xF) + percent * cb);
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_inter_col_using_col1(uint16_t *dst, int size, uint32_t color0, uint32_t color1)
{
    uint32_t cr, cg, cb;
    uint16_t col, a;
    uint8_t r, g, b;

    float percent_r = ((color1 >> 12) & 0xF) / 15.0f;
    float percent_g = ((color1 >> 8) & 0xF) / 15.0f;
    float percent_b = ((color1 >> 4) & 0xF) / 15.0f;
    float percent_r_i = 1.0f - percent_r;
    float percent_g_i = 1.0f - percent_g;
    float percent_b_i = 1.0f - percent_b;

    cr = (color0 >> 12) & 0xF;
    cg = (color0 >> 8) & 0xF;
    cb = (color0 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        r = (uint8_t)(percent_r_i * ((col >> 8) & 0xF) + percent_r * cr);
        g = (uint8_t)(percent_g_i * ((col >> 4) & 0xF) + percent_g * cg);
        b = (uint8_t)(percent_b_i * (col & 0xF) + percent_b * cb);
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_full_color_sub_tex(uint16_t *dst, int size, uint32_t color)
{
    uint32_t cr, cg, cb, ca;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;
    ca = color & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = (uint8_t)(ca - ((col >> 12) & 0xF));
        r = (uint8_t)(cr - ((col >> 8) & 0xF));
        g = (uint8_t)(cg - ((col >> 4) & 0xF));
        b = (uint8_t)(cb - (col & 0xF));
        *(dst++) = (a << 12) | (r << 8) | (g << 4) | b;
    }
}

static void mod_col_inter_col1_using_tex(uint16_t *dst, int size, uint32_t color0, uint32_t color1)
{
    uint32_t cr0, cg0, cb0, cr1, cg1, cb1;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent_r, percent_g, percent_b;

    cr0 = (color0 >> 12) & 0xF;
    cg0 = (color0 >> 8) & 0xF;
    cb0 = (color0 >> 4) & 0xF;
    cr1 = (color1 >> 12) & 0xF;
    cg1 = (color1 >> 8) & 0xF;
    cb1 = (color1 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent_r = ((col >> 8) & 0xF) / 15.0f;
        percent_g = ((col >> 4) & 0xF) / 15.0f;
        percent_b = (col & 0xF) / 15.0f;
        r = minval(15, (uint8_t)((1.0f - percent_r) * cr0 + percent_r * cr1 + 0.0001f));
        g = minval(15, (uint8_t)((1.0f - percent_g) * cg0 + percent_g * cg1 + 0.0001f));
        b = minval(15, (uint8_t)((1.0f - percent_b) * cb0 + percent_b * cb1 + 0.0001f));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_col_inter_col1_using_texa(uint16_t *dst, int size, uint32_t color0, uint32_t color1)
{
    uint32_t cr0, cg0, cb0, cr1, cg1, cb1;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent, percent_i;

    cr0 = (color0 >> 12) & 0xF;
    cg0 = (color0 >> 8) & 0xF;
    cb0 = (color0 >> 4) & 0xF;
    cr1 = (color1 >> 12) & 0xF;
    cg1 = (color1 >> 8) & 0xF;
    cb1 = (color1 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent = (a >> 12) / 15.0f;
        percent_i = 1.0f - percent;
        r = (uint8_t)(percent_i * cr0 + percent * cr1);
        g = (uint8_t)(percent_i * cg0 + percent * cg1);
        b = (uint8_t)(percent_i * cb0 + percent * cb1);
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_col_inter_col1_using_texa__mul_tex(uint16_t *dst, int size, uint32_t color0, uint32_t color1)
{
    uint32_t cr0, cg0, cb0, cr1, cg1, cb1;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent, percent_i;

    cr0 = (color0 >> 12) & 0xF;
    cg0 = (color0 >> 8) & 0xF;
    cb0 = (color0 >> 4) & 0xF;
    cr1 = (color1 >> 12) & 0xF;
    cg1 = (color1 >> 8) & 0xF;
    cb1 = (color1 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent = (a >> 12) / 15.0f;
        percent_i = 1.0f - percent;
        r = (uint8_t)(((percent_i * cr0 + percent * cr1) / 15.0f) * (((col & 0x0F00) >> 8) / 15.0f) * 15.0f);
        g = (uint8_t)(((percent_i * cg0 + percent * cg1) / 15.0f) * (((col & 0x00F0) >> 4) / 15.0f) * 15.0f);
        b = (uint8_t)(((percent_i * cb0 + percent * cb1) / 15.0f) * ((col & 0x000F) / 15.0f) * 15.0f);
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_col_inter_tex_using_tex(uint16_t *dst, int size, uint32_t color)
{
    uint32_t cr, cg, cb;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent_r, percent_g, percent_b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent_r = ((col >> 8) & 0xF) / 15.0f;
        percent_g = ((col >> 4) & 0xF) / 15.0f;
        percent_b = (col & 0xF) / 15.0f;
        r = (uint8_t)((1.0f - percent_r) * cr + percent_r * ((col & 0x0F00) >> 8));
        g = (uint8_t)((1.0f - percent_g) * cg + percent_g * ((col & 0x00F0) >> 4));
        b = (uint8_t)((1.0f - percent_b) * cb + percent_b * (col & 0x000F));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_col_inter_tex_using_texa(uint16_t *dst, int size, uint32_t color)
{
    uint32_t cr, cg, cb;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent, percent_i;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent = (a >> 12) / 15.0f;
        percent_i = 1.0f - percent;
        r = (uint8_t)(percent_i * cr + percent * ((col & 0x0F00) >> 8));
        g = (uint8_t)(percent_i * cg + percent * ((col & 0x00F0) >> 4));
        b = (uint8_t)(percent_i * cb + percent * (col & 0x000F));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_col2_inter__col_inter_col1_using_tex__using_texa(uint16_t *dst, int size,
    uint32_t color0, uint32_t color1,
    uint32_t color2)
{
    uint32_t cr0, cg0, cb0, cr1, cg1, cb1, cr2, cg2, cb2;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent_r, percent_g, percent_b, percent_a;

    cr0 = (color0 >> 12) & 0xF;
    cg0 = (color0 >> 8) & 0xF;
    cb0 = (color0 >> 4) & 0xF;
    cr1 = (color1 >> 12) & 0xF;
    cg1 = (color1 >> 8) & 0xF;
    cb1 = (color1 >> 4) & 0xF;
    cr2 = (color2 >> 12) & 0xF;
    cg2 = (color2 >> 8) & 0xF;
    cb2 = (color2 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent_a = (a >> 12) / 15.0f;
        percent_r = ((col >> 8) & 0xF) / 15.0f;
        percent_g = ((col >> 4) & 0xF) / 15.0f;
        percent_b = (col & 0xF) / 15.0f;
        r = (uint8_t)(((1.0f - percent_r) * cr0 + percent_r * cr1) * percent_a + cr2 * (1.0f - percent_a));
        g = (uint8_t)(((1.0f - percent_g) * cg0 + percent_g * cg1) * percent_a + cg2 * (1.0f - percent_a));
        b = (uint8_t)(((1.0f - percent_b) * cb0 + percent_b * cb1) * percent_a + cb2 * (1.0f - percent_a));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_scale_fac_add_fac(uint16_t *dst, int size, uint32_t factor)
{
    float percent = factor / 255.0f;
    uint16_t col;
    uint8_t a;
    float base_a = (1.0f - percent) * 15.0f;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = (uint8_t)(base_a + percent * (col >> 12));
        *(dst++) = (a << 12) | (col & 0x0FFF);
    }
}

static void mod_tex_sub_col_mul_fac_add_tex(uint16_t *dst, int size, uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    uint32_t cr, cg, cb;
    uint16_t col, a;
    float r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        r = (float)((col >> 8) & 0xF);
        r = /*maxval(*/(r - cr) * percent/*, 0.0f)*/ + r;
        if (r > 15.0f) r = 15.0f;
        if (r < 0.0f) r = 0.0f;
        g = (float)((col >> 4) & 0xF);
        g = /*maxval(*/(g - cg) * percent/*, 0.0f)*/ + g;
        if (g > 15.0f) g = 15.0f;
        if (g < 0.0f) g = 0.0f;
        b = (float)(col & 0xF);
        b = /*maxval(*/(b - cb) * percent/*, 0.0f)*/ + b;
        if (b > 15.0f) b = 15.0f;
        if (b < 0.0f) b = 0.0f;

        *(dst++) = a | ((uint16_t)r << 8) | ((uint16_t)g << 4) | (uint16_t)b;
    }
}

static void mod_tex_scale_col_add_col(uint16_t *dst, int size, uint32_t color0, uint32_t color1)
{
    uint32_t cr0, cg0, cb0, cr1, cg1, cb1;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent_r, percent_g, percent_b;

    cr0 = (color0 >> 12) & 0xF;
    cg0 = (color0 >> 8) & 0xF;
    cb0 = (color0 >> 4) & 0xF;
    cr1 = (color1 >> 12) & 0xF;
    cg1 = (color1 >> 8) & 0xF;
    cb1 = (color1 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent_r = ((col >> 8) & 0xF) / 15.0f;
        percent_g = ((col >> 4) & 0xF) / 15.0f;
        percent_b = (col & 0xF) / 15.0f;
        r = minval(15, (uint8_t)(percent_r * cr0 + cr1 + 0.0001f));
        g = minval(15, (uint8_t)(percent_g * cg0 + cg1 + 0.0001f));
        b = minval(15, (uint8_t)(percent_b * cb0 + cb1 + 0.0001f));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_add_col(uint16_t *dst, int size, uint32_t color)
{
    uint32_t cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = (uint8_t)((col >> 12) & 0xF);
        //		a = col & 0xF000;
        r = (uint8_t)(cr + ((col >> 8) & 0xF)) & 0xF;
        g = (uint8_t)(cg + ((col >> 4) & 0xF)) & 0xF;
        b = (uint8_t)(cb + (col & 0xF)) & 0xF;
        *(dst++) = (a << 12) | (r << 8) | (g << 4) | b;
    }
}

static void mod_col_mul_texa_add_tex(uint16_t *dst, int size, uint32_t color)
{
    uint32_t cr, cg, cb;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float factor;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        factor = (a >> 12) / 15.0f;
        r = (uint8_t)(cr*factor + ((col >> 8) & 0xF)) & 0xF;
        g = (uint8_t)(cg*factor + ((col >> 4) & 0xF)) & 0xF;
        b = (uint8_t)(cb*factor + (col & 0xF)) & 0xF;
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_sub_col(uint16_t *dst, int size, uint32_t color)
{
    int cr, cg, cb;
    uint16_t col;
    uint8_t a, r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = (uint8_t)(col & 0xF000);
        r = (uint8_t)maxval((((col >> 8) & 0xF) - cr), 0);
        g = (uint8_t)maxval((((col >> 4) & 0xF) - cg), 0);
        b = (uint8_t)maxval(((col & 0xF) - cb), 0);
        *(dst++) = (a << 12) | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_sub_col_mul_fac(uint16_t *dst, int size, uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    uint32_t cr, cg, cb;
    uint16_t col, a;
    float r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = (uint8_t)((col >> 12) & 0xF);
        r = (float)((col >> 8) & 0xF);
        r = (r - cr) * percent;
        if (r > 15.0f) r = 15.0f;
        if (r < 0.0f) r = 0.0f;
        g = (float)((col >> 4) & 0xF);
        g = (g - cg) * percent;
        if (g > 15.0f) g = 15.0f;
        if (g < 0.0f) g = 0.0f;
        b = (float)(col & 0xF);
        b = (b - cb) * percent;
        if (b > 15.0f) b = 15.0f;
        if (b < 0.0f) b = 0.0f;

        *(dst++) = (a << 12) | ((uint16_t)r << 8) | ((uint16_t)g << 4) | (uint16_t)b;
    }
}

static void mod_col_inter_tex_using_col1(uint16_t *dst, int size, uint32_t color0, uint32_t color1)
{
    uint32_t cr, cg, cb;
    uint16_t col, a;
    uint8_t r, g, b;

    float percent_r = ((color1 >> 12) & 0xF) / 15.0f;
    float percent_g = ((color1 >> 8) & 0xF) / 15.0f;
    float percent_b = ((color1 >> 4) & 0xF) / 15.0f;
    float percent_r_i = 1.0f - percent_r;
    float percent_g_i = 1.0f - percent_g;
    float percent_b_i = 1.0f - percent_b;

    cr = (color0 >> 12) & 0xF;
    cg = (color0 >> 8) & 0xF;
    cb = (color0 >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = (uint8_t)((col >> 12) & 0xF);
        r = (uint8_t)(percent_r * ((col >> 8) & 0xF) + percent_r_i * cr);
        g = (uint8_t)(percent_g * ((col >> 4) & 0xF) + percent_g_i * cg);
        b = (uint8_t)(percent_b * (col & 0xF) + percent_b_i * cb);
        *(dst++) = (a << 12) | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_inter_noise_using_col(uint16_t *dst, int size, uint32_t color)
{
    uint16_t col, a;
    uint8_t r, g, b, noise;

    float percent_r = ((color >> 12) & 0xF) / 15.0f;
    float percent_g = ((color >> 8) & 0xF) / 15.0f;
    float percent_b = ((color >> 4) & 0xF) / 15.0f;
    float percent_r_i = 1.0f - percent_r;
    float percent_g_i = 1.0f - percent_g;
    float percent_b_i = 1.0f - percent_b;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        noise = rand() % 16;
        r = (uint8_t)(percent_r_i * ((col >> 8) & 0xF) + percent_r * noise);
        g = (uint8_t)(percent_g_i * ((col >> 4) & 0xF) + percent_g * noise);
        b = (uint8_t)(percent_b_i * (col & 0xF) + percent_b * noise);
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_inter_col_using_texa(uint16_t *dst, int size, uint32_t color)
{
    uint32_t cr, cg, cb;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;
    float percent, percent_i;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        percent = (a >> 12) / 15.0f;
        percent_i = 1.0f - percent;
        r = (uint8_t)(percent * cr + percent_i * ((col & 0x0F00) >> 8));
        g = (uint8_t)(percent * cg + percent_i * ((col & 0x00F0) >> 4));
        b = (uint8_t)(percent * cb + percent_i * (col & 0x000F));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_mul_col(uint16_t *dst, int size, uint32_t color)
{
    float cr, cg, cb;
    uint16_t col;
    uint8_t r, g, b;
    uint16_t a;

    cr = (float)((color >> 12) & 0xF) / 16.0f;
    cg = (float)((color >> 8) & 0xF) / 16.0f;
    cb = (float)((color >> 4) & 0xF) / 16.0f;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        a = col & 0xF000;
        r = (uint8_t)(cr * ((col & 0x0F00) >> 8));
        g = (uint8_t)(cg * ((col & 0x00F0) >> 4));
        b = (uint8_t)(cb * (col & 0x000F));
        *(dst++) = a | (r << 8) | (g << 4) | b;
    }
}

static void mod_tex_scale_fac_add_col(uint16_t *dst, int size, uint32_t color, uint32_t factor)
{
    float percent = factor / 255.0f;
    uint32_t cr, cg, cb;
    uint16_t col;
    float r, g, b;

    cr = (color >> 12) & 0xF;
    cg = (color >> 8) & 0xF;
    cb = (color >> 4) & 0xF;

    for (int i = 0; i < size; i++)
    {
        col = *dst;
        r = cr + percent * (float)((col >> 8) & 0xF);
        g = cg + percent * (float)((col >> 4) & 0xF);
        b = cb + percent * (float)(col & 0xF);
        *(dst++) = (col & 0xF000) | ((uint8_t)r << 8) | ((uint8_t)g << 4) | (uint8_t)b;
    }
}
