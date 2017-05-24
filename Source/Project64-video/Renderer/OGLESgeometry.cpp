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

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include "glide.h"
#include "glitchmain.h"
#include <Project64-video/rdp.h>
#include <Project64-video/trace.h>

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
}

void gfxVertexLayout(FxU32 param, FxI32 offset, FxU32 mode)
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
        WriteTrace(TraceGlitch, TraceWarning, "unknown gfxVertexLayout parameter : %x", param);
    }
}

void gfxCullMode(GrCullMode_t mode)
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

void gfxDepthBufferMode(GrDepthBufferMode_t mode)
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

void gfxDepthBufferFunction(GrCmpFnc_t function)
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

void gfxDepthMask(FxBool mask)
{
    WriteTrace(TraceGlitch, TraceDebug, "mask: %d", mask);
    glDepthMask(mask);
}

float biasFactor = 0;
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
void gfxDrawTriangle(const void *a, const void *b, const void *c)
{
    WriteTrace(TraceGlitch, TraceDebug, "start");
    vbo_enable();
    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
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
}

FX_ENTRY void FX_CALL
grDrawLine(const void *a, const void *b)
{
}

FX_ENTRY void FX_CALL
grDrawVertexArray(FxU32 mode, FxU32 Count, void *pointers2)
{
    void **pointers = (void**)pointers2;
    WriteTrace(TraceGlitch, TraceDebug, "grDrawVertexArray(%d,%d)\r\n", mode, Count);

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
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
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
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