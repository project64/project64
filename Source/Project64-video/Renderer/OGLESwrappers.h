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
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
void vbo_draw();

class GLCache
{
public:
    static void ResetCache(void);

    static void glActiveTexture(GLenum texture);
    static void glBindTexture(GLenum target, GLuint texture);
    static void glBlendEquation(GLenum mode);
    static void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    static void glBlendFunc(GLenum sfactor, GLenum dfactor);
    static void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    static void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    static void glClearDepthf(GLclampf depth);
    static void glCullFace(GLenum mode);
    static void glDepthFunc(GLenum func);
    static void glDepthMask(GLboolean flag);
    static void glDepthRangef(GLclampf zNear, GLclampf zFar);
    static void glEnableDisable(GLenum cap, bool enable);
    static void glPolygonOffset(GLfloat factor, GLfloat units);
    static void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
    static void glUseProgram(GLuint program);
    static void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    static void glFrontFace(GLenum mode);

private:
    static void glEnableDisableItem(GLenum cap, bool enable, bool & cached_state, const char * StateName);

    static GLenum m_cached_texture;
    static GLenum m_cached_mode;
    static GLenum m_cached_modeRGB;
    static GLenum m_cached_modeAlpha;
    static GLenum m_cached_sfactor;
    static GLenum m_cached_dfactor;
    static GLenum m_cached_BlendFuncSeparate_srcRGB;
    static GLenum m_cached_BlendFuncSeparate_dstRGB;
    static GLenum m_cached_BlendFuncSeparate_srcAlpha;
    static GLenum m_cached_BlendFuncSeparate_dstAlpha;
    static GLclampf m_cached_depth;
    static GLenum m_cached_CullFace_mode;
    static GLenum m_cached_func;
    static GLboolean m_cached_DepthMask_flag;
    static GLclampf m_cached_zNear;
    static GLclampf m_cached_zFar;
    static bool m_cached_BLEND;
    static bool m_cached_CULL_FACE;
    static bool m_cached_DEPTH_TEST;
    static bool m_cached_DITHER;
    static bool m_cached_POLYGON_OFFSET_FILL;
    static bool m_cached_SAMPLE_ALPHA_TO_COVERAGE;
    static bool m_cached_SAMPLE_COVERAGE;
    static bool m_cached_SCISSOR_TEST;
    static bool m_cached_STENCIL_TEST;
    static GLenum m_cached_FrontFace_mode;
    static GLfloat m_cached_factor;
    static GLfloat m_cached_units;
    static GLclampf m_cached_red, m_cached_green, m_cached_blue, m_cached_alpha;
    static GLint m_cached_x, m_cached_y;
    static GLsizei m_cached_width, m_cached_height;
    static GLuint m_cached_program;
    static GLint m_Viewport_cached_x, m_Viewport_cached_y;
    static GLsizei m_Viewport_cached_width, m_Viewport_cached_height;
};

#define glActiveTexture(texture) GLCache::glActiveTexture(texture)
#define glBindTexture(target, texture) GLCache::glBindTexture(target, texture)
#define glBlendEquation(mode) GLCache::glBlendEquation(mode)
#define glBlendEquationSeparate(modeRGB, modeAlpha) GLCache::glBlendEquationSeparate(modeRGB, modeAlpha)
#define glBlendFunc(sfactor, dfactor) GLCache::glBlendFunc(sfactor, dfactor)
#define glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha) GLCache::glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha)
#define glClearColor(red, green, blue, alpha) GLCache::glClearColor(red, green, blue, alpha)
#define glClearDepthf(depth) GLCache::glClearDepthf(depth)
#define glCullFace(mode) GLCache::glCullFace(mode)
#define glDepthFunc(func) GLCache::glDepthFunc(func)
#define glDepthMask(flag) GLCache::glDepthMask(flag)
#define glDepthRangef(zNear, zFar) GLCache::glDepthRangef(zNear, zFar)
#define glDisable(cap) GLCache::glEnableDisable(cap, false)
#define glEnable(cap) GLCache::glEnableDisable(cap, true)
#define glFrontFace(mode) GLCache::glFrontFace(mode)
#define glPolygonOffset(factor, units) GLCache::glPolygonOffset(factor, units)
#define glScissor(x, y, width, height) GLCache::glScissor(x, y, width, height)
#define glUseProgram(program) GLCache::glUseProgram(program)
#define glViewport(x, y, width, height) GLCache::glViewport(x, y, width, height)
