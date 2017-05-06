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
#include <GLES2/gl2.h>
#include "glitchmain.h"

#undef glActiveTexture
#undef glBindTexture
#undef glBlendEquation
#undef glBlendEquationSeparate
#undef glBlendFunc
#undef glBlendFuncSeparate
#undef glClearColor
#undef glClearDepthf
#undef glCullFace
#undef glDepthFunc
#undef glDepthMask
#undef glDepthRangef
#undef glDisable
#undef glEnable
#undef glFrontFace
#undef glPolygonOffset
#undef glScissor
#undef glUseProgram
#undef glViewport

void vbo_draw();

GLenum GLCache::m_cached_texture;
GLenum GLCache::m_cached_mode;
GLenum GLCache::m_cached_modeRGB;
GLenum GLCache::m_cached_modeAlpha;
GLenum GLCache::m_cached_sfactor;
GLenum GLCache::m_cached_dfactor;
GLenum GLCache::m_cached_BlendFuncSeparate_srcRGB;
GLenum GLCache::m_cached_BlendFuncSeparate_dstRGB;
GLenum GLCache::m_cached_BlendFuncSeparate_srcAlpha;
GLenum GLCache::m_cached_BlendFuncSeparate_dstAlpha;
GLclampf GLCache::m_cached_depth;
GLenum GLCache::m_cached_CullFace_mode;
GLenum GLCache::m_cached_func;
GLboolean GLCache::m_cached_DepthMask_flag;
GLclampf GLCache::m_cached_zNear;
GLclampf GLCache::m_cached_zFar;
bool GLCache::m_cached_BLEND = false;
bool GLCache::m_cached_CULL_FACE = false;
bool GLCache::m_cached_DEPTH_TEST = false;
bool GLCache::m_cached_DITHER = false;
bool GLCache::m_cached_POLYGON_OFFSET_FILL = false;
bool GLCache::m_cached_SAMPLE_ALPHA_TO_COVERAGE = false;
bool GLCache::m_cached_SAMPLE_COVERAGE = false;
bool GLCache::m_cached_SCISSOR_TEST = false;
bool GLCache::m_cached_STENCIL_TEST = false;
GLenum GLCache::m_cached_FrontFace_mode;
GLfloat GLCache::m_cached_factor;
GLfloat GLCache::m_cached_units;
GLclampf GLCache::m_cached_red, GLCache::m_cached_green, GLCache::m_cached_blue, GLCache::m_cached_alpha;
GLint GLCache::m_cached_x, GLCache::m_cached_y;
GLsizei GLCache::m_cached_width, GLCache::m_cached_height;
GLuint GLCache::m_cached_program;
GLint GLCache::m_Viewport_cached_x = 0, GLCache::m_Viewport_cached_y = 0;
GLsizei GLCache::m_Viewport_cached_width = 0, GLCache::m_Viewport_cached_height = 0;

void GLCache::ResetCache(void)
{
    m_cached_texture = 0;
    m_cached_mode = 0;
    m_cached_modeRGB = 0;
    m_cached_modeAlpha = 0;
    m_cached_sfactor = 0;
    m_cached_dfactor = 0;
    m_cached_BlendFuncSeparate_srcRGB = 0;
    m_cached_BlendFuncSeparate_dstRGB = 0;
    m_cached_BlendFuncSeparate_srcAlpha = 0;
    m_cached_BlendFuncSeparate_dstAlpha = 0;
    m_cached_depth = 0;
    m_cached_CullFace_mode = 0;
    m_cached_func = 0;
    m_cached_DepthMask_flag = 0;
    m_cached_zNear = 0;
    m_cached_zFar = 0;
    m_cached_BLEND = false;
    m_cached_CULL_FACE = false;
    m_cached_DEPTH_TEST = false;
    m_cached_DITHER = false;
    m_cached_POLYGON_OFFSET_FILL = false;
    m_cached_SAMPLE_ALPHA_TO_COVERAGE = false;
    m_cached_SAMPLE_COVERAGE = false;
    m_cached_SCISSOR_TEST = false;
    m_cached_STENCIL_TEST = false;
    m_cached_FrontFace_mode = 0;
    m_cached_factor = 0;
    m_cached_units = 0;
    m_cached_red = 0;
    m_cached_green = 0;
    m_cached_blue = 0;
    m_cached_alpha = 0;
    m_cached_x = 0;
    m_cached_y = 0;
    m_cached_width = 0;
    m_cached_height = 0;
    m_cached_program = 0;
    m_Viewport_cached_x = 0;
    m_Viewport_cached_y = 0;
    m_Viewport_cached_width = 0;
    m_Viewport_cached_height = 0;
}

void GLCache::glActiveTexture(GLenum texture)
{
    if (texture != m_cached_texture)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "texture: %d", texture);
        vbo_draw();
        ::glActiveTexture(texture);
        m_cached_texture = texture;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - texture: %d", texture);
    }
}

void GLCache::glBindTexture(GLenum target, GLuint texture)
{
    WriteTrace(TraceOGLWrapper, TraceDebug, "target: %d texture: %d", target, texture);
    vbo_draw();
    ::glBindTexture(target, texture);
}

void GLCache::glBlendEquation(GLenum mode)
{
    if (mode != m_cached_mode)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "mode: %d", mode);
        vbo_draw();
        ::glBlendEquation(mode);
        m_cached_mode = mode;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - mode: %d", mode);
    }
}

void GLCache::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    if (modeRGB != m_cached_modeRGB || modeAlpha != m_cached_modeAlpha)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "modeRGB: %d cached_modeAlpha: %d", modeRGB, m_cached_modeAlpha);
        vbo_draw();
        ::glBlendEquationSeparate(modeRGB, modeAlpha);
        m_cached_modeRGB = modeRGB;
        m_cached_modeAlpha = modeAlpha;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - modeRGB: %d cached_modeAlpha: %d", modeRGB, m_cached_modeAlpha);
    }
}

void GLCache::glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    if (sfactor != m_cached_sfactor || dfactor != m_cached_dfactor)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "sfactor: %d dfactor: %d", sfactor, dfactor);
        vbo_draw();
        ::glBlendFunc(sfactor, dfactor);
        m_cached_sfactor = sfactor;
        m_cached_dfactor = dfactor;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - sfactor: %d dfactor: %d", sfactor, dfactor);
    }
}

void GLCache::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    if (srcRGB != m_cached_BlendFuncSeparate_srcRGB || dstRGB != m_cached_BlendFuncSeparate_dstRGB || srcAlpha != m_cached_BlendFuncSeparate_srcAlpha || dstAlpha != m_cached_BlendFuncSeparate_dstAlpha)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "srcRGB: %d dstRGB: %d srcAlpha: %d dstAlpha: %d", srcRGB, dstRGB, srcAlpha, dstAlpha);
        vbo_draw();
        ::glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
        m_cached_BlendFuncSeparate_srcRGB = srcRGB;
        m_cached_BlendFuncSeparate_dstRGB = dstRGB;
        m_cached_BlendFuncSeparate_srcAlpha = srcAlpha;
        m_cached_BlendFuncSeparate_dstAlpha = dstAlpha;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - srcRGB: %d dstRGB: %d srcAlpha: %d dstAlpha: %d", srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

void GLCache::glClearDepthf(GLclampf depth)
{
    if (depth != m_cached_depth)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "depth: %d", depth);
        vbo_draw();
        ::glClearDepthf(depth);
        m_cached_depth = depth;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - depth: %d", depth);
    }
}

void GLCache::glCullFace(GLenum mode)
{
    if (mode != m_cached_CullFace_mode)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "mode: %d", mode);
        vbo_draw();
        ::glCullFace(mode);
        m_cached_CullFace_mode = mode;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - mode: %d", mode);
    }
}

void GLCache::glDepthFunc(GLenum func)
{
    if (func != m_cached_func)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "func: %d", func);
        vbo_draw();
        ::glDepthFunc(func);
        m_cached_func = func;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - func: %d", func);
    }
}

void GLCache::glDepthMask(GLboolean flag)
{
    if (flag != m_cached_DepthMask_flag)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "flag: %d", (int)flag);
        vbo_draw();
        ::glDepthMask(flag);
        m_cached_DepthMask_flag = flag;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - flag: %d", (int)flag);
    }
}

void GLCache::glDepthRangef(GLclampf zNear, GLclampf zFar)
{
    if (zNear != m_cached_zNear || zFar != m_cached_zFar)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "zNear: %d zFar: %d", zNear, zFar);
        vbo_draw();
        ::glDepthRangef(zNear, zFar);
        m_cached_zNear = zNear;
        m_cached_zFar = zFar;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - zNear: %d zFar: %d", zNear, zFar);
    }
}

void GLCache::glEnableDisableItem(GLenum cap, bool enable, bool & cached_state, const char * StateName)
{
    if (enable)
    {
        if (!cached_state)
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glEnable(%s)", StateName);
            vbo_draw();
            ::glEnable(cap);
            cached_state = true;
        }
        else
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - glEnable(%s)", StateName);
        }
    }
    else
    {
        if (cached_state)
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glDisable(%s)", StateName);
            vbo_draw();
            ::glDisable(cap);
            cached_state = false;
        }
        else
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - glEnable(%s)", StateName);
        }
    }
}

void GLCache::glEnableDisable(GLenum cap, bool enable)
{
    if (cap == GL_BLEND) { GLCache::glEnableDisableItem(cap, enable, m_cached_BLEND, "GL_BLEND"); }
    else if (cap == GL_CULL_FACE) { GLCache::glEnableDisableItem(cap, enable, m_cached_CULL_FACE, "GL_CULL_FACE"); }
    else if (cap == GL_DEPTH_TEST) { GLCache::glEnableDisableItem(cap, enable, m_cached_DEPTH_TEST, "GL_DEPTH_TEST"); }
    else if (cap == GL_DITHER) { GLCache::glEnableDisableItem(cap, enable, m_cached_DITHER, "GL_DITHER"); }
    else if (cap == GL_POLYGON_OFFSET_FILL) { GLCache::glEnableDisableItem(cap, enable, m_cached_POLYGON_OFFSET_FILL, "GL_POLYGON_OFFSET_FILL"); }
    else if (cap == GL_SAMPLE_ALPHA_TO_COVERAGE) { GLCache::glEnableDisableItem(cap, enable, m_cached_SAMPLE_ALPHA_TO_COVERAGE, "GL_SAMPLE_ALPHA_TO_COVERAGE"); }
    else if (cap == GL_SAMPLE_COVERAGE) { GLCache::glEnableDisableItem(cap, enable, m_cached_SAMPLE_COVERAGE, "GL_SAMPLE_COVERAGE"); }
    else if (cap == GL_SCISSOR_TEST) { GLCache::glEnableDisableItem(cap, enable, m_cached_SCISSOR_TEST, "GL_SCISSOR_TEST"); }
    else if (cap == GL_STENCIL_TEST) { GLCache::glEnableDisableItem(cap, enable, m_cached_STENCIL_TEST, "GL_STENCIL_TEST"); }
    else
    {
        if (enable)
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glEnable(%d)", cap);
            vbo_draw();
            ::glEnable(cap);
        }
        else
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glDisable(%d)", cap);
            vbo_draw();
            ::glDisable(cap);
        }
    }
}

void GLCache::glFrontFace(GLenum mode)
{
    if (mode != m_cached_FrontFace_mode)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "mode: %d", mode);
        vbo_draw();
        ::glFrontFace(mode);
        m_cached_FrontFace_mode = mode;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - mode: %d", mode);
    }
}

void GLCache::glPolygonOffset(GLfloat factor, GLfloat units)
{
    if (factor != m_cached_factor || units != m_cached_units)
    {
        vbo_draw();
        WriteTrace(TraceOGLWrapper, TraceDebug, "factor: %f units: %f", factor, units);
        ::glPolygonOffset(factor, units);
        m_cached_factor = factor;
        m_cached_units = units;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - factor: %f units: %f", factor, units);
    }
}

void GLCache::glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    if (red != m_cached_red || green != m_cached_green || blue != m_cached_blue || alpha != m_cached_alpha)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "red: %f, green: %f, blue: %f, alpha: %f", red, green, blue, alpha);
        vbo_draw();
        ::glClearColor(red, green, blue, alpha);
        m_cached_red = red;
        m_cached_green = green;
        m_cached_blue = blue;
        m_cached_alpha = alpha;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - red: %f, green: %f, blue: %f, alpha: %f", red, green, blue, alpha);
    }
}

void GLCache::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (x != m_cached_x || y != m_cached_y || width != m_cached_width || height != m_cached_height)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "x: %d, y: %d, width: %d, height: %d", x, y, width, height);
        vbo_draw();
        ::glScissor(x, y, width, height);
        m_cached_x = x;
        m_cached_y = y;
        m_cached_width = width;
        m_cached_height = height;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    }
}

void GLCache::glUseProgram(GLuint program)
{
    if (program != m_cached_program)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "program: %d", program);
        vbo_draw();
        ::glUseProgram(program);
        m_cached_program = program;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - program: %d", program);
    }
}

void GLCache::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (x != m_Viewport_cached_x || y != m_Viewport_cached_y || width != m_Viewport_cached_width || height != m_Viewport_cached_height)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "x: %d, y: %d, width: %d, height: %d", x, y, width, height);
        vbo_draw();
        ::glViewport(x, y, width, height);
        m_Viewport_cached_x = x;
        m_Viewport_cached_y = y;
        m_Viewport_cached_width = width;
        m_Viewport_cached_height = height;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "ignored x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    }
}
