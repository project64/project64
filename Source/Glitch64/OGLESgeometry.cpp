/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include "glide.h"
#include "glitchmain.h"
#include "../Glide64/winlnxdefs.h"
#include "../Glide64/rdp.h"
#include <Glide64/trace.h>

#define Z_MAX (65536.0f)
#define VERTEX_SIZE sizeof(VERTEX) //Size of vertex struct

static int xy_off;
static int xy_en;
static int z_en;
static int z_off;
static int q_off;
static int q_en;
static int pargb_off;
static int pargb_en;
static int st0_off;
static int st0_en;
static int st1_off;
static int st1_en;
static int fog_ext_off;
static int fog_ext_en;

int w_buffer_mode;
int inverted_culling;
int culling_mode;

#define VERTEX_BUFFER_SIZE 1500 //Max amount of vertices to buffer, this seems large enough.
static VERTEX vertex_buffer[VERTEX_BUFFER_SIZE];
static int vertex_buffer_count = 0;
static GLenum vertex_draw_mode;
static bool vertex_buffer_enabled = false;

void vbo_init()
{
}

void vbo_draw()
{
    if (vertex_buffer_count)
    {
        WriteTrace(TraceGlide64, TraceDebug, "vertex_draw_mode: %d vertex_buffer_count: %d", vertex_draw_mode, vertex_buffer_count);
        glDrawArrays(vertex_draw_mode, 0, vertex_buffer_count);
        vertex_buffer_count = 0;
        WriteTrace(TraceGlide64, TraceDebug, "done (glGetError() = %X)", glGetError());
    }
}

//Buffer vertices instead of glDrawArrays(...)
void vbo_buffer(GLenum mode, GLint first, GLsizei count, void* pointers)
{
    if ((count != 3 && mode != GL_TRIANGLES) || vertex_buffer_count + count > VERTEX_BUFFER_SIZE)
    {
        vbo_draw();
    }

    memcpy(&vertex_buffer[vertex_buffer_count], pointers, count * VERTEX_SIZE);
    vertex_buffer_count += count;

    if (count == 3 || mode == GL_TRIANGLES)
    {
        vertex_draw_mode = GL_TRIANGLES;
    }
    else
    {
        vertex_draw_mode = mode;
        vbo_draw(); //Triangle fans and strips can't be joined as easily, just draw them straight away.
    }
}

void vbo_enable()
{
    if (vertex_buffer_enabled)
        return;

    vertex_buffer_enabled = true;
    glEnableVertexAttribArray(POSITION_ATTR);
    glVertexAttribPointer(POSITION_ATTR, 4, GL_FLOAT, GL_FALSE, VERTEX_SIZE, &vertex_buffer[0].x); //Position

    glEnableVertexAttribArray(COLOUR_ATTR);
    glVertexAttribPointer(COLOUR_ATTR, 4, GL_UNSIGNED_BYTE, GL_TRUE, VERTEX_SIZE, &vertex_buffer[0].b); //Colour

    glEnableVertexAttribArray(TEXCOORD_0_ATTR);
    glVertexAttribPointer(TEXCOORD_0_ATTR, 2, GL_FLOAT, false, VERTEX_SIZE, &vertex_buffer[0].coord[2]); //Tex0

    glEnableVertexAttribArray(TEXCOORD_1_ATTR);
    glVertexAttribPointer(TEXCOORD_1_ATTR, 2, GL_FLOAT, false, VERTEX_SIZE, &vertex_buffer[0].coord[0]); //Tex1

    glEnableVertexAttribArray(FOG_ATTR);
    glVertexAttribPointer(FOG_ATTR, 1, GL_FLOAT, false, VERTEX_SIZE, &vertex_buffer[0].f); //Fog
}

void vbo_disable()
{
    vbo_draw();
    vertex_buffer_enabled = false;
}

inline float ZCALC(const float & z, const float & q) {
    float res = z_en ? ((z) / Z_MAX) / (q) : 1.0f;
    return res;
}

/*
#define zclamp (1.0f-1.0f/zscale)
static inline void zclamp_glVertex4f(float a, float b, float c, float d)
{
if (c<zclamp) c = zclamp;
glVertex4f(a,b,c,d);
}
#define glVertex4f(a,b,c,d) zclamp_glVertex4f(a,b,c,d)
*/

static inline float ytex(int tmu, float y) {
    if (invtex[tmu])
        return invtex[tmu] - y;
    else
        return y;
}

void init_geometry()
{
    xy_en = q_en = pargb_en = st0_en = st1_en = z_en = 0;
    w_buffer_mode = 0;
    inverted_culling = 0;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    vbo_init();
}

FX_ENTRY void FX_CALL
grCoordinateSpace(GrCoordinateSpaceMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    switch (mode)
    {
    case GR_WINDOW_COORDS:
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknwown coordinate space : %x", mode);
    }
}

FX_ENTRY void FX_CALL
grVertexLayout(FxU32 param, FxI32 offset, FxU32 mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "param: %d offset: %d mode: %d", param, offset, mode);
    switch (param)
    {
    case GR_PARAM_XY:
        xy_en = mode;
        xy_off = offset;
        break;
    case GR_PARAM_Z:
        z_en = mode;
        z_off = offset;
        break;
    case GR_PARAM_Q:
        q_en = mode;
        q_off = offset;
        break;
    case GR_PARAM_FOG_EXT:
        fog_ext_en = mode;
        fog_ext_off = offset;
        break;
    case GR_PARAM_PARGB:
        pargb_en = mode;
        pargb_off = offset;
        break;
    case GR_PARAM_ST0:
        st0_en = mode;
        st0_off = offset;
        break;
    case GR_PARAM_ST1:
        st1_en = mode;
        st1_off = offset;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown grVertexLayout parameter : %x", param);
    }
}

FX_ENTRY void FX_CALL
grCullMode(GrCullMode_t mode)
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
    case GR_CULL_DISABLE:
        glDisable(GL_CULL_FACE);
        break;
    case GR_CULL_NEGATIVE:
        if (!inverted_culling)
            glCullFace(GL_FRONT);
        else
            glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        break;
    case GR_CULL_POSITIVE:
        if (!inverted_culling)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown cull mode : %x", mode);
    }
}

// Depth buffer

FX_ENTRY void FX_CALL
grDepthBufferMode(GrDepthBufferMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    switch (mode)
    {
    case GR_DEPTHBUFFER_DISABLE:
        glDisable(GL_DEPTH_TEST);
        w_buffer_mode = 0;
        return;
    case GR_DEPTHBUFFER_WBUFFER:
    case GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS:
        glEnable(GL_DEPTH_TEST);
        w_buffer_mode = 1;
        break;
    case GR_DEPTHBUFFER_ZBUFFER:
    case GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS:
        glEnable(GL_DEPTH_TEST);
        w_buffer_mode = 0;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown depth buffer mode : %x", mode);
    }
}

FX_ENTRY void FX_CALL
grDepthBufferFunction(GrCmpFnc_t function)
{
    WriteTrace(TraceGlitch, TraceDebug, "function: %d", function);
    switch (function)
    {
    case GR_CMP_GEQUAL:
        if (w_buffer_mode)
            glDepthFunc(GL_LEQUAL);
        else
            glDepthFunc(GL_GEQUAL);
        break;
    case GR_CMP_LEQUAL:
        if (w_buffer_mode)
            glDepthFunc(GL_GEQUAL);
        else
            glDepthFunc(GL_LEQUAL);
        break;
    case GR_CMP_LESS:
        if (w_buffer_mode)
            glDepthFunc(GL_GREATER);
        else
            glDepthFunc(GL_LESS);
        break;
    case GR_CMP_ALWAYS:
        glDepthFunc(GL_ALWAYS);
        break;
    case GR_CMP_EQUAL:
        glDepthFunc(GL_EQUAL);
        break;
    case GR_CMP_GREATER:
        if (w_buffer_mode)
            glDepthFunc(GL_LESS);
        else
            glDepthFunc(GL_GREATER);
        break;
    case GR_CMP_NEVER:
        glDepthFunc(GL_NEVER);
        break;
    case GR_CMP_NOTEQUAL:
        glDepthFunc(GL_NOTEQUAL);
        break;

    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown depth buffer function : %x", function);
    }
}

FX_ENTRY void FX_CALL
grDepthMask(FxBool mask)
{
    WriteTrace(TraceGlitch, TraceDebug, "mask: %d", mask);
    glDepthMask(mask);
}
float biasFactor = 0;
#if 0
void FindBestDepthBias()
{
    float f, bestz = 0.25f;
    int x;
    if (biasFactor) return;
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
    for (x = 0, f = 1.0f; f <= 65536.0f; x += 4, f *= 2.0f) {
        float z;
        glPolygonOffset(0, f);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(float(x + 4 - widtho) / (width / 2), float(0 - heighto) / (height / 2), 0.5);
        glVertex3f(float(x - widtho) / (width / 2), float(0 - heighto) / (height / 2), 0.5);
        glVertex3f(float(x + 4 - widtho) / (width / 2), float(4 - heighto) / (height / 2), 0.5);
        glVertex3f(float(x - widtho) / (width / 2), float(4 - heighto) / (height / 2), 0.5);
        glEnd();
        glReadPixels(x + 2, 2 + viewport_offset, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
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
}
#endif

FX_ENTRY void FX_CALL
grDepthBiasLevel(FxI32 level)
{
    WriteTrace(TraceGlitch, TraceDebug, "level: %d", level);
    if (level)
    {
        if (w_buffer_mode)
        {
            glPolygonOffset(1.0f, -(float)level*zscale / 255.0f);
        }
        else
        {
            glPolygonOffset(0, (float)level*biasFactor);
        }
        glEnable(GL_POLYGON_OFFSET_FILL);
    }
    else
    {
        glPolygonOffset(0, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

// draw

FX_ENTRY void FX_CALL
grDrawTriangle(const void *a, const void *b, const void *c)
{
    WriteTrace(TraceGlitch, TraceDebug, "start");
    vbo_enable();
    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    if (vertex_buffer_count + 3 > VERTEX_BUFFER_SIZE)
    {
        vbo_draw();
    }
    vertex_draw_mode = GL_TRIANGLES;
    memcpy(&vertex_buffer[vertex_buffer_count], a, VERTEX_SIZE);
    memcpy(&vertex_buffer[vertex_buffer_count + 1], b, VERTEX_SIZE);
    memcpy(&vertex_buffer[vertex_buffer_count + 2], c, VERTEX_SIZE);
    vertex_buffer_count += 3;

    WriteTrace(TraceGlitch, TraceDebug, "Done");
}

FX_ENTRY void FX_CALL
grDrawPoint(const void *pt)
{
    /*
      float *x = (float*)pt + xy_off/sizeof(float);
      float *y = (float*)pt + xy_off/sizeof(float) + 1;
      float *z = (float*)pt + z_off/sizeof(float);
      float *q = (float*)pt + q_off/sizeof(float);
      unsigned char *pargb = (unsigned char*)pt + pargb_off;
      float *s0 = (float*)pt + st0_off/sizeof(float);
      float *t0 = (float*)pt + st0_off/sizeof(float) + 1;
      float *s1 = (float*)pt + st1_off/sizeof(float);
      float *t1 = (float*)pt + st1_off/sizeof(float) + 1;
      float *fog = (float*)pt + fog_ext_off/sizeof(float);
      WriteTrace(TraceGlitch, TraceDebug,"grDrawPoint()\r\n");

      if(nvidia_viewport_hack && !render_to_texture)
      {
      glViewport(0, viewport_offset, viewport_width, viewport_height);
      nvidia_viewport_hack = 0;
      }

      reloadTexture();

      if(need_to_compile) compile_shader();

      glBegin(GL_POINTS);

      if (nbTextureUnits > 2)
      {
      if (st0_en)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *s0 / *q / (float)tex1_width,
      ytex(0, *t0 / *q / (float)tex1_height));
      if (st1_en)
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *s1 / *q / (float)tex0_width,
      ytex(1, *t1 / *q / (float)tex0_height));
      }
      else
      {
      if (st0_en)
      glTexCoord2f(*s0 / *q / (float)tex0_width,
      ytex(0, *t0 / *q / (float)tex0_height));
      }
      if (pargb_en)
      glColor4f(pargb[2]/255.0f, pargb[1]/255.0f, pargb[0]/255.0f, pargb[3]/255.0f);
      if (fog_enabled && fog_coord_support)
      {
      if(!fog_ext_en || fog_enabled != 2)
      glSecondaryColor3f((1.0f / *q) / 255.0f, 0.0f, 0.0f);
      else
      glSecondaryColor3f((1.0f / *fog) / 255.0f, 0.0f, 0.0f);
      }
      glVertex4f((*x - (float)widtho) / (float)(width/2) / *q,
      -(*y - (float)heighto) / (float)(height/2) / *q, ZCALC(*z ,*q), 1.0f / *q);

      glEnd();
      */
}

FX_ENTRY void FX_CALL
grDrawLine(const void *a, const void *b)
{
    /*
      float *a_x = (float*)a + xy_off/sizeof(float);
      float *a_y = (float*)a + xy_off/sizeof(float) + 1;
      float *a_z = (float*)a + z_off/sizeof(float);
      float *a_q = (float*)a + q_off/sizeof(float);
      unsigned char *a_pargb = (unsigned char*)a + pargb_off;
      float *a_s0 = (float*)a + st0_off/sizeof(float);
      float *a_t0 = (float*)a + st0_off/sizeof(float) + 1;
      float *a_s1 = (float*)a + st1_off/sizeof(float);
      float *a_t1 = (float*)a + st1_off/sizeof(float) + 1;
      float *a_fog = (float*)a + fog_ext_off/sizeof(float);

      float *b_x = (float*)b + xy_off/sizeof(float);
      float *b_y = (float*)b + xy_off/sizeof(float) + 1;
      float *b_z = (float*)b + z_off/sizeof(float);
      float *b_q = (float*)b + q_off/sizeof(float);
      unsigned char *b_pargb = (unsigned char*)b + pargb_off;
      float *b_s0 = (float*)b + st0_off/sizeof(float);
      float *b_t0 = (float*)b + st0_off/sizeof(float) + 1;
      float *b_s1 = (float*)b + st1_off/sizeof(float);
      float *b_t1 = (float*)b + st1_off/sizeof(float) + 1;
      float *b_fog = (float*)b + fog_ext_off/sizeof(float);
      WriteTrace(TraceGlitch, TraceDebug,"grDrawLine()\r\n");

      if(nvidia_viewport_hack && !render_to_texture)
      {
      glViewport(0, viewport_offset, viewport_width, viewport_height);
      nvidia_viewport_hack = 0;
      }

      reloadTexture();

      if(need_to_compile) compile_shader();

      glBegin(GL_LINES);

      if (nbTextureUnits > 2)
      {
      if (st0_en)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *a_s0 / *a_q / (float)tex1_width, ytex(0, *a_t0 / *a_q / (float)tex1_height));
      if (st1_en)
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *a_s1 / *a_q / (float)tex0_width, ytex(1, *a_t1 / *a_q / (float)tex0_height));
      }
      else
      {
      if (st0_en)
      glTexCoord2f(*a_s0 / *a_q / (float)tex0_width, ytex(0, *a_t0 / *a_q / (float)tex0_height));
      }
      if (pargb_en)
      glColor4f(a_pargb[2]/255.0f, a_pargb[1]/255.0f, a_pargb[0]/255.0f, a_pargb[3]/255.0f);
      if (fog_enabled && fog_coord_support)
      {
      if(!fog_ext_en || fog_enabled != 2)
      glSecondaryColor3f((1.0f / *a_q) / 255.0f, 0.0f, 0.0f);
      else
      glSecondaryColor3f((1.0f / *a_fog) / 255.0f, 0.0f, 0.0f);
      }
      glVertex4f((*a_x - (float)widtho) / (float)(width/2) / *a_q,
      -(*a_y - (float)heighto) / (float)(height/2) / *a_q, ZCALC(*a_z, *a_q), 1.0f / *a_q);

      if (nbTextureUnits > 2)
      {
      if (st0_en)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, *b_s0 / *b_q / (float)tex1_width,
      ytex(0, *b_t0 / *b_q / (float)tex1_height));
      if (st1_en)
      glMultiTexCoord2fARB(GL_TEXTURE0_ARB, *b_s1 / *b_q / (float)tex0_width,
      ytex(1, *b_t1 / *b_q / (float)tex0_height));
      }
      else
      {
      if (st0_en)
      glTexCoord2f(*b_s0 / *b_q / (float)tex0_width,
      ytex(0, *b_t0 / *b_q / (float)tex0_height));
      }
      if (pargb_en)
      glColor4f(b_pargb[2]/255.0f, b_pargb[1]/255.0f, b_pargb[0]/255.0f, b_pargb[3]/255.0f);
      if (fog_enabled && fog_coord_support)
      {
      if(!fog_ext_en || fog_enabled != 2)
      glSecondaryColor3f((1.0f / *b_q) / 255.0f, 0.0f, 0.0f);
      else
      glSecondaryColor3f((1.0f / *b_fog) / 255.0f, 0.0f, 0.0f);
      }
      glVertex4f((*b_x - (float)widtho) / (float)(width/2) / *b_q,
      -(*b_y - (float)heighto) / (float)(height/2) / *b_q, ZCALC(*b_z, *b_q), 1.0f / *b_q);

      glEnd();
      */
}

FX_ENTRY void FX_CALL
grDrawVertexArray(FxU32 mode, FxU32 Count, void *pointers2)
{
    void **pointers = (void**)pointers2;
    WriteTrace(TraceGlitch, TraceDebug, "grDrawVertexArray(%d,%d)\r\n", mode, Count);

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    if (mode != GR_TRIANGLE_FAN)
    {
        WriteTrace(TraceGlitch, TraceWarning, "grDrawVertexArray : unknown mode : %x", mode);
    }

    vbo_enable();
    vbo_buffer(GL_TRIANGLE_FAN, 0, Count, pointers[0]);
}

FX_ENTRY void FX_CALL
grDrawVertexArrayContiguous(FxU32 mode, FxU32 Count, void *pointers, FxU32 stride)
{
    WriteTrace(TraceGlitch, TraceDebug, "grDrawVertexArrayContiguous(%d,%d,%d)\r\n", mode, Count, stride);

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    if (stride != 156)
    {
        //LOGINFO("Incompatible stride\n");
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    vbo_enable();

    switch (mode)
    {
    case GR_TRIANGLE_STRIP:
        vbo_buffer(GL_TRIANGLE_STRIP, 0, Count, pointers);
        break;
    case GR_TRIANGLE_FAN:
        vbo_buffer(GL_TRIANGLE_FAN, 0, Count, pointers);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "grDrawVertexArrayContiguous : unknown mode : %x", mode);
    }
}