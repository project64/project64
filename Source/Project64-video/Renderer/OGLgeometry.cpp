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
#include <Project64-video/Settings.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include "glitchmain.h"
#include <Project64-video/trace.h>

#define Z_MAX (65536.0f)

static int xy_off = offsetof(VERTEX, x);
static int z_off = offsetof(VERTEX, z);
static int q_off = offsetof(VERTEX, q);
static int pargb_off = offsetof(VERTEX, b);
static int st0_off = offsetof(VERTEX, coord[0]);
static int st1_off = offsetof(VERTEX, coord[2]);
static int fog_ext_off = offsetof(VERTEX, f);

int w_buffer_mode;
int inverted_culling;
gfxCullMode_t culling_mode;
extern int fog_enabled;

inline float ZCALC(const float & z, const float & q) {
    float res = ((z) / Z_MAX) / (q);
    return res;
}

#define zclamp (1.0f-1.0f/zscale)
static inline void zclamp_glVertex4f(float a, float b, float c, float d)
{
    if (c < zclamp) c = zclamp;
    glVertex4f(a, b, c, d);
}
#define glVertex4f(a,b,c,d) zclamp_glVertex4f(a,b,c,d)

static inline float ytex(int tmu, float y) {
    if (invtex[tmu])
        return invtex[tmu] - y;
    else
        return y;
}

void init_geometry()
{
    w_buffer_mode = 0;
    inverted_culling = 0;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    grDisplayGLError("init_geometry");
}

void gfxCullMode(gfxCullMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    static int oldmode = -1, oldinv = -1;
    culling_mode = mode;
    if (inverted_culling == oldinv && oldmode == mode)
        return;
    oldmode = mode;
    oldinv = inverted_culling;
    switch (mode)
    {
    case GFX_CULL_DISABLE:
        glDisable(GL_CULL_FACE);
        break;
    case GFX_CULL_NEGATIVE:
        if (!inverted_culling)
            glCullFace(GL_FRONT);
        else
            glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        break;
    case GFX_CULL_POSITIVE:
        if (!inverted_culling)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown cull mode : %x", mode);
    }
    grDisplayGLError("gfxCullMode");
}

// Depth buffer
void gfxDepthBufferMode(gfxDepthBufferMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    switch (mode)
    {
    case GFX_DEPTHBUFFER_DISABLE:
        glDisable(GL_DEPTH_TEST);
        w_buffer_mode = 0;
        return;
    case GFX_DEPTHBUFFER_WBUFFER:
    case GFX_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS:
        glEnable(GL_DEPTH_TEST);
        w_buffer_mode = 1;
        break;
    case GFX_DEPTHBUFFER_ZBUFFER:
    case GFX_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS:
        glEnable(GL_DEPTH_TEST);
        w_buffer_mode = 0;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown depth buffer mode : %x", mode);
    }
    grDisplayGLError("gfxDepthBufferMode");
}

void gfxDepthBufferFunction(gfxCmpFnc_t function)
{
    WriteTrace(TraceGlitch, TraceDebug, "function: %d", function);
    switch (function)
    {
    case GFX_CMP_GEQUAL:
        if (w_buffer_mode)
            glDepthFunc(GL_LEQUAL);
        else
            glDepthFunc(GL_GEQUAL);
        break;
    case GFX_CMP_LEQUAL:
        if (w_buffer_mode)
            glDepthFunc(GL_GEQUAL);
        else
            glDepthFunc(GL_LEQUAL);
        break;
    case GFX_CMP_LESS:
        if (w_buffer_mode)
            glDepthFunc(GL_GREATER);
        else
            glDepthFunc(GL_LESS);
        break;
    case GFX_CMP_ALWAYS:
        glDepthFunc(GL_ALWAYS);
        break;
    case GFX_CMP_EQUAL:
        glDepthFunc(GL_EQUAL);
        break;
    case GFX_CMP_GREATER:
        if (w_buffer_mode)
            glDepthFunc(GL_LESS);
        else
            glDepthFunc(GL_GREATER);
        break;
    case GFX_CMP_NEVER:
        glDepthFunc(GL_NEVER);
        break;
    case GFX_CMP_NOTEQUAL:
        glDepthFunc(GL_NOTEQUAL);
        break;

    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown depth buffer function : %x", function);
    }
    grDisplayGLError("gfxDepthBufferFunction");
}

void gfxDepthMask(bool mask)
{
    WriteTrace(TraceGlitch, TraceDebug, "mask: %d", mask);
    glDepthMask((GLboolean)mask);
    grDisplayGLError("gfxDepthMask");
}

float biasFactor = 0;
void FindBestDepthBias()
{
    GLfloat vertices[4][3];
    float f, bestz = 0.25f;
    int x;

    if (biasFactor)
        return;

    biasFactor = 64.0f; // default value
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glColor4ub(255, 255, 255, 255);
    glDepthMask(GL_TRUE);

    for (x = 0; x < 4; x++)
        vertices[x][2] = 0.5;

    for (x = 0, f = 1.0f; f <= 65536.0f; x += 4, f *= 2.0f) {
        float z;

        vertices[0][0] = float(x + 4 - widtho) / (g_width / 2);
        vertices[0][1] = float(0 + 0 - heighto) / (g_height / 2);
        vertices[1][0] = float(x + 0 - widtho) / (g_width / 2);
        vertices[1][1] = float(0 + 0 - heighto) / (g_height / 2);
        vertices[2][0] = float(x + 4 - widtho) / (g_width / 2);
        vertices[2][1] = float(0 + 4 - heighto) / (g_height / 2);
        vertices[3][0] = float(x + 0 - widtho) / (g_width / 2);
        vertices[3][1] = float(0 + 4 - heighto) / (g_height / 2);
        glPolygonOffset(0, f);

        glBegin(GL_TRIANGLE_STRIP);
        glVertex3fv(vertices[0]);
        glVertex3fv(vertices[1]);
        glVertex3fv(vertices[2]);
        glVertex3fv(vertices[3]);
        glEnd();

        glReadPixels(x + 2, 2 + g_viewport_offset, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
        z -= 0.75f + 8e-6f;
        if (z < 0.0f) z = -z;
        if (z > 0.01f) continue;
        if (z < bestz) {
            bestz = z;
            biasFactor = f;
        }
        //printf("f %g z %g\n", f, z);
    }
    //printf(" --> bias factor %g\n", biasFactor);
    glPopAttrib();
    grDisplayGLError("FindBestDepthBias");
}

void gfxDepthBiasLevel(int32_t level)
{
    WriteTrace(TraceGlitch, TraceDebug, "level: %d", level);
    if (level)
    {
        if (w_buffer_mode)
            glPolygonOffset(1.0f, -(float)level*zscale / 255.0f);
        else
            glPolygonOffset(0, (float)level*biasFactor);
        glEnable(GL_POLYGON_OFFSET_FILL);
    }
    else
    {
        glPolygonOffset(0, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    grDisplayGLError("gfxDepthBiasLevel");
}

// draw
void gfxDrawTriangle(const void *a, const void *b, const void *c)
{
    float *a_x = (float*)a + xy_off / sizeof(float);
    float *a_y = (float*)a + xy_off / sizeof(float) + 1;
    float *a_z = (float*)a + z_off / sizeof(float);
    float *a_q = (float*)a + q_off / sizeof(float);
    unsigned char *a_pargb = (unsigned char*)a + pargb_off;
    float *a_s0 = (float*)a + st0_off / sizeof(float);
    float *a_t0 = (float*)a + st0_off / sizeof(float) + 1;
    float *a_s1 = (float*)a + st1_off / sizeof(float);
    float *a_t1 = (float*)a + st1_off / sizeof(float) + 1;
    float *a_fog = (float*)a + fog_ext_off / sizeof(float);

    float *b_x = (float*)b + xy_off / sizeof(float);
    float *b_y = (float*)b + xy_off / sizeof(float) + 1;
    float *b_z = (float*)b + z_off / sizeof(float);
    float *b_q = (float*)b + q_off / sizeof(float);
    unsigned char *b_pargb = (unsigned char*)b + pargb_off;
    float *b_s0 = (float*)b + st0_off / sizeof(float);
    float *b_t0 = (float*)b + st0_off / sizeof(float) + 1;
    float *b_s1 = (float*)b + st1_off / sizeof(float);
    float *b_t1 = (float*)b + st1_off / sizeof(float) + 1;
    float *b_fog = (float*)b + fog_ext_off / sizeof(float);

    float *c_x = (float*)c + xy_off / sizeof(float);
    float *c_y = (float*)c + xy_off / sizeof(float) + 1;
    float *c_z = (float*)c + z_off / sizeof(float);
    float *c_q = (float*)c + q_off / sizeof(float);
    unsigned char *c_pargb = (unsigned char*)c + pargb_off;
    float *c_s0 = (float*)c + st0_off / sizeof(float);
    float *c_t0 = (float*)c + st0_off / sizeof(float) + 1;
    float *c_s1 = (float*)c + st1_off / sizeof(float);
    float *c_t1 = (float*)c + st1_off / sizeof(float) + 1;
    float *c_fog = (float*)c + fog_ext_off / sizeof(float);
    WriteTrace(TraceGlitch, TraceDebug, "-");

    // ugly ? i know but nvidia drivers are losing the viewport otherwise

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    glBegin(GL_TRIANGLES);

    if (nbTextureUnits > 2)
    {
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *a_s0 / *a_q / (float)tex1_width, ytex(0, *a_t0 / *a_q / (float)tex1_height));
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *a_s1 / *a_q / (float)tex0_width, ytex(1, *a_t1 / *a_q / (float)tex0_height));
    }
    else
    {
        glTexCoord2f(*a_s0 / *a_q / (float)tex0_width, ytex(0, *a_t0 / *a_q / (float)tex0_height));
    }
    glColor4f(a_pargb[2] / 255.0f, a_pargb[1] / 255.0f, a_pargb[0] / 255.0f, a_pargb[3] / 255.0f);
    if (fog_enabled && fog_coord_support)
    {
        if (!g_settings->fog() || fog_enabled != 2)
            glSecondaryColor3f((1.0f / *a_q) / 255.0f, 0.0f, 0.0f);
        else
            glSecondaryColor3f((1.0f / *a_fog) / 255.0f, 0.0f, 0.0f);
    }
    glVertex4f((*a_x - (float)widtho) / (float)(g_width / 2) / *a_q,
        -(*a_y - (float)heighto) / (float)(g_height / 2) / *a_q, ZCALC(*a_z, *a_q), 1.0f / *a_q);

    if (nbTextureUnits > 2)
    {
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *b_s0 / *b_q / (float)tex1_width, ytex(0, *b_t0 / *b_q / (float)tex1_height));
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *b_s1 / *b_q / (float)tex0_width, ytex(1, *b_t1 / *b_q / (float)tex0_height));
    }
    else
    {
        glTexCoord2f(*b_s0 / *b_q / (float)tex0_width, ytex(0, *b_t0 / *b_q / (float)tex0_height));
    }
    glColor4f(b_pargb[2] / 255.0f, b_pargb[1] / 255.0f, b_pargb[0] / 255.0f, b_pargb[3] / 255.0f);
    if (fog_enabled && fog_coord_support)
    {
        if (!g_settings->fog() || fog_enabled != 2)
            glSecondaryColor3f((1.0f / *b_q) / 255.0f, 0.0f, 0.0f);
        else
            glSecondaryColor3f((1.0f / *b_fog) / 255.0f, 0.0f, 0.0f);
    }

    glVertex4f((*b_x - (float)widtho) / (float)(g_width / 2) / *b_q,
        -(*b_y - (float)heighto) / (float)(g_height / 2) / *b_q, ZCALC(*b_z, *b_q), 1.0f / *b_q);

    if (nbTextureUnits > 2)
    {
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *c_s0 / *c_q / (float)tex1_width, ytex(0, *c_t0 / *c_q / (float)tex1_height));
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *c_s1 / *c_q / (float)tex0_width, ytex(1, *c_t1 / *c_q / (float)tex0_height));
    }
    else
    {
        glTexCoord2f(*c_s0 / *c_q / (float)tex0_width, ytex(0, *c_t0 / *c_q / (float)tex0_height));
    }
    glColor4f(c_pargb[2] / 255.0f, c_pargb[1] / 255.0f, c_pargb[0] / 255.0f, c_pargb[3] / 255.0f);
    if (fog_enabled && fog_coord_support)
    {
        if (!g_settings->fog() || fog_enabled != 2)
            glSecondaryColor3f((1.0f / *c_q) / 255.0f, 0.0f, 0.0f);
        else
            glSecondaryColor3f((1.0f / *c_fog) / 255.0f, 0.0f, 0.0f);
    }
    glVertex4f((*c_x - (float)widtho) / (float)(g_width / 2) / *c_q,
        -(*c_y - (float)heighto) / (float)(g_height / 2) / *c_q, ZCALC(*c_z, *c_q), 1.0f / *c_q);

    glEnd();
    grDisplayGLError("gfxDrawTriangle");
}

void gfxDrawLine(const void *a, const void *b)
{
    float *a_x = (float*)a + xy_off / sizeof(float);
    float *a_y = (float*)a + xy_off / sizeof(float) + 1;
    float *a_z = (float*)a + z_off / sizeof(float);
    float *a_q = (float*)a + q_off / sizeof(float);
    unsigned char *a_pargb = (unsigned char*)a + pargb_off;
    float *a_s0 = (float*)a + st0_off / sizeof(float);
    float *a_t0 = (float*)a + st0_off / sizeof(float) + 1;
    float *a_s1 = (float*)a + st1_off / sizeof(float);
    float *a_t1 = (float*)a + st1_off / sizeof(float) + 1;
    float *a_fog = (float*)a + fog_ext_off / sizeof(float);

    float *b_x = (float*)b + xy_off / sizeof(float);
    float *b_y = (float*)b + xy_off / sizeof(float) + 1;
    float *b_z = (float*)b + z_off / sizeof(float);
    float *b_q = (float*)b + q_off / sizeof(float);
    unsigned char *b_pargb = (unsigned char*)b + pargb_off;
    float *b_s0 = (float*)b + st0_off / sizeof(float);
    float *b_t0 = (float*)b + st0_off / sizeof(float) + 1;
    float *b_s1 = (float*)b + st1_off / sizeof(float);
    float *b_t1 = (float*)b + st1_off / sizeof(float) + 1;
    float *b_fog = (float*)b + fog_ext_off / sizeof(float);
    WriteTrace(TraceGlitch, TraceDebug, "-");

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    glBegin(GL_LINES);

    if (nbTextureUnits > 2)
    {
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *a_s0 / *a_q / (float)tex1_width, ytex(0, *a_t0 / *a_q / (float)tex1_height));
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *a_s1 / *a_q / (float)tex0_width, ytex(1, *a_t1 / *a_q / (float)tex0_height));
    }
    else
    {
        glTexCoord2f(*a_s0 / *a_q / (float)tex0_width, ytex(0, *a_t0 / *a_q / (float)tex0_height));
    }
    glColor4f(a_pargb[2] / 255.0f, a_pargb[1] / 255.0f, a_pargb[0] / 255.0f, a_pargb[3] / 255.0f);
    if (fog_enabled && fog_coord_support)
    {
        if (!g_settings->fog() || fog_enabled != 2)
            glSecondaryColor3f((1.0f / *a_q) / 255.0f, 0.0f, 0.0f);
        else
            glSecondaryColor3f((1.0f / *a_fog) / 255.0f, 0.0f, 0.0f);
    }
    glVertex4f((*a_x - (float)widtho) / (float)(g_width / 2) / *a_q,
        -(*a_y - (float)heighto) / (float)(g_height / 2) / *a_q, ZCALC(*a_z, *a_q), 1.0f / *a_q);

    if (nbTextureUnits > 2)
    {
        glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *b_s0 / *b_q / (float)tex1_width, ytex(0, *b_t0 / *b_q / (float)tex1_height));
        glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *b_s1 / *b_q / (float)tex0_width, ytex(1, *b_t1 / *b_q / (float)tex0_height));
    }
    else
    {
        glTexCoord2f(*b_s0 / *b_q / (float)tex0_width, ytex(0, *b_t0 / *b_q / (float)tex0_height));
    }
    glColor4f(b_pargb[2] / 255.0f, b_pargb[1] / 255.0f, b_pargb[0] / 255.0f, b_pargb[3] / 255.0f);
    if (fog_enabled && fog_coord_support)
    {
        if (!g_settings->fog() || fog_enabled != 2)
            glSecondaryColor3f((1.0f / *b_q) / 255.0f, 0.0f, 0.0f);
        else
            glSecondaryColor3f((1.0f / *b_fog) / 255.0f, 0.0f, 0.0f);
    }
    glVertex4f((*b_x - (float)widtho) / (float)(g_width / 2) / *b_q,
        -(*b_y - (float)heighto) / (float)(g_height / 2) / *b_q, ZCALC(*b_z, *b_q), 1.0f / *b_q);

    glEnd();
    grDisplayGLError("gfxDrawLine");
}

void gfxDrawVertexArray(gfxDrawMode_t mode, uint32_t Count, void *pointers2)
{
    unsigned int i;
    float *x, *y, *q, *s0, *t0, *s1, *t1, *z, *fog;
    unsigned char *pargb;
    void **pointers = (void**)pointers2;
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d Count: %d", mode, Count);

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    switch (mode)
    {
    case GFX_TRIANGLE_FAN:
        glBegin(GL_TRIANGLE_FAN);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxDrawVertexArray : unknown mode : %x", mode);
    }

    for (i = 0; i < Count; i++)
    {
        x = (float*)pointers[i] + xy_off / sizeof(float);
        y = (float*)pointers[i] + xy_off / sizeof(float) + 1;
        z = (float*)pointers[i] + z_off / sizeof(float);
        q = (float*)pointers[i] + q_off / sizeof(float);
        pargb = (unsigned char*)pointers[i] + pargb_off;
        s0 = (float*)pointers[i] + st0_off / sizeof(float);
        t0 = (float*)pointers[i] + st0_off / sizeof(float) + 1;
        s1 = (float*)pointers[i] + st1_off / sizeof(float);
        t1 = (float*)pointers[i] + st1_off / sizeof(float) + 1;
        fog = (float*)pointers[i] + fog_ext_off / sizeof(float);

        if (nbTextureUnits > 2)
        {
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *s0 / *q / (float)tex1_width, ytex(0, *t0 / *q / (float)tex1_height));
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *s1 / *q / (float)tex0_width, ytex(1, *t1 / *q / (float)tex0_height));
        }
        else
        {
            glTexCoord2f(*s0 / *q / (float)tex0_width, ytex(0, *t0 / *q / (float)tex0_height));
        }
        glColor4f(pargb[2] / 255.0f, pargb[1] / 255.0f, pargb[0] / 255.0f, pargb[3] / 255.0f);
        if (fog_enabled && fog_coord_support)
        {
            if (!g_settings->fog() || fog_enabled != 2)
                glSecondaryColor3f((1.0f / *q) / 255.0f, 0.0f, 0.0f);
            else
                glSecondaryColor3f((1.0f / *fog) / 255.0f, 0.0f, 0.0f);
        }
        glVertex4f((*x - (float)widtho) / (float)(g_width / 2) / *q,
            -(*y - (float)heighto) / (float)(g_height / 2) / *q, ZCALC(*z, *q), 1.0f / *q);
    }
    glEnd();

    grDisplayGLError("gfxDrawVertexArray");
}

void gfxDrawVertexArrayContiguous(gfxDrawMode_t mode, uint32_t Count, void *pointers, uint32_t stride)
{
    unsigned int i;
    float *x, *y, *q, *s0, *t0, *s1, *t1, *z, *fog;
    unsigned char *pargb;
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d Count: %d stride: %d", mode, Count, stride);

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    switch (mode)
    {
    case GFX_TRIANGLE_STRIP:
        glBegin(GL_TRIANGLE_STRIP);
        break;
    case GFX_TRIANGLE_FAN:
        glBegin(GL_TRIANGLE_FAN);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxDrawVertexArrayContiguous : unknown mode : %x", mode);
    }

    for (i = 0; i < Count; i++)
    {
        x = (float*)((unsigned char*)pointers + stride*i) + xy_off / sizeof(float);
        y = (float*)((unsigned char*)pointers + stride*i) + xy_off / sizeof(float) + 1;
        z = (float*)((unsigned char*)pointers + stride*i) + z_off / sizeof(float);
        q = (float*)((unsigned char*)pointers + stride*i) + q_off / sizeof(float);
        pargb = (unsigned char*)pointers + stride*i + pargb_off;
        s0 = (float*)((unsigned char*)pointers + stride*i) + st0_off / sizeof(float);
        t0 = (float*)((unsigned char*)pointers + stride*i) + st0_off / sizeof(float) + 1;
        s1 = (float*)((unsigned char*)pointers + stride*i) + st1_off / sizeof(float);
        t1 = (float*)((unsigned char*)pointers + stride*i) + st1_off / sizeof(float) + 1;
        fog = (float*)((unsigned char*)pointers + stride*i) + fog_ext_off / sizeof(float);

        if (nbTextureUnits > 2)
        {
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *s0 / *q / (float)tex1_width, ytex(0, *t0 / *q / (float)tex1_height));
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *s1 / *q / (float)tex0_width, ytex(1, *t1 / *q / (float)tex0_height));
        }
        else
        {
            glTexCoord2f(*s0 / *q / (float)tex0_width, ytex(0, *t0 / *q / (float)tex0_height));
        }
        glColor4f(pargb[2] / 255.0f, pargb[1] / 255.0f, pargb[0] / 255.0f, pargb[3] / 255.0f);
        if (fog_enabled && fog_coord_support)
        {
            if (!g_settings->fog() || fog_enabled != 2)
                glSecondaryColor3f((1.0f / *q) / 255.0f, 0.0f, 0.0f);
            else
                glSecondaryColor3f((1.0f / *fog) / 255.0f, 0.0f, 0.0f);
        }

        glVertex4f((*x - (float)widtho) / (float)(g_width / 2) / *q,
            -(*y - (float)heighto) / (float)(g_height / 2) / *q, ZCALC(*z, *q), 1.0f / *q);
    }
    glEnd();

    grDisplayGLError("gfxDrawVertexArrayContiguous");
}