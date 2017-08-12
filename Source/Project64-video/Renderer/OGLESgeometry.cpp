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
#include "glitchmain.h"
#include <Project64-video/rdp.h>
#include <Project64-video/trace.h>

#define Z_MAX (65536.0f)
#define VERTEX_SIZE sizeof(VERTEX) //Size of vertex struct

int w_buffer_mode;
int inverted_culling;
gfxCullMode_t culling_mode;

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
    float res = ((z) / Z_MAX) / (q);
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
    w_buffer_mode = 0;
    inverted_culling = 0;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
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
}

void gfxDepthMask(bool mask)
{
    WriteTrace(TraceGlitch, TraceDebug, "mask: %d", mask);
    glDepthMask(mask);
}

float biasFactor = 0;
void gfxDepthBiasLevel(int32_t level)
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

void gfxDrawLine(const void *a, const void *b)
{
}

void gfxDrawVertexArray(gfxDrawMode_t mode, uint32_t Count, void *pointers2)
{
    void **pointers = (void**)pointers2;
    WriteTrace(TraceGlitch, TraceDebug, "gfxDrawVertexArray(%d,%d)\r\n", mode, Count);

    if (nvidia_viewport_hack && !render_to_texture)
    {
        glViewport(0, g_viewport_offset, viewport_width, viewport_height);
        nvidia_viewport_hack = 0;
    }

    reloadTexture();

    if (need_to_compile) compile_shader();

    if (mode != GFX_TRIANGLE_FAN)
    {
        WriteTrace(TraceGlitch, TraceWarning, "gfxDrawVertexArray : unknown mode : %x", mode);
    }

    vbo_enable();
    vbo_buffer(GL_TRIANGLE_FAN, 0, Count, pointers[0]);
}

void gfxDrawVertexArrayContiguous(gfxDrawMode_t mode, uint32_t Count, void *pointers, uint32_t stride)
{
    WriteTrace(TraceGlitch, TraceDebug, "gfxDrawVertexArrayContiguous(%d,%d,%d)\r\n", mode, Count, stride);

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
    case GFX_TRIANGLE_STRIP:
        vbo_buffer(GL_TRIANGLE_STRIP, 0, Count, pointers);
        break;
    case GFX_TRIANGLE_FAN:
        vbo_buffer(GL_TRIANGLE_FAN, 0, Count, pointers);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxDrawVertexArrayContiguous : unknown mode : %x", mode);
    }
}