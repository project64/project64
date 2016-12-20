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

void cache_glActiveTexture (GLenum texture)
{
    static GLenum cached_texture;

    if(texture != cached_texture)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "texture: %d",texture);
        vbo_draw();
        glActiveTexture(texture);
        cached_texture = texture;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - texture: %d",texture);
    }
}

void cache_glBindTexture (GLenum target, GLuint texture)
{
    WriteTrace(TraceOGLWrapper, TraceDebug, "target: %d texture: %d",target, texture);
    vbo_draw();
    glBindTexture(target, texture);
}

void cache_glBlendEquation ( GLenum mode )
{
    static GLenum cached_mode;

    if(mode != cached_mode)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "mode: %d",mode);
        vbo_draw();
        glBlendEquation(mode);
        cached_mode = mode;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - mode: %d",mode);
    }
}

void cache_glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
    static GLenum cached_modeRGB;
    static GLenum cached_modeAlpha;

    if(modeRGB != cached_modeRGB || modeAlpha != cached_modeAlpha)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "modeRGB: %d cached_modeAlpha: %d",modeRGB, cached_modeAlpha);
        vbo_draw();
        glBlendEquationSeparate(modeRGB, modeAlpha);
        cached_modeRGB = modeRGB;
        cached_modeAlpha = modeAlpha;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - modeRGB: %d cached_modeAlpha: %d",modeRGB, cached_modeAlpha);
    }

}

void cache_glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    static GLenum cached_sfactor;
    static GLenum cached_dfactor;

    if(sfactor != cached_sfactor || dfactor != cached_dfactor)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "sfactor: %d dfactor: %d",sfactor, dfactor);
        vbo_draw();
        glBlendFunc(sfactor, dfactor);
        cached_sfactor = sfactor;
        cached_dfactor = dfactor;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - sfactor: %d dfactor: %d",sfactor, dfactor);
    }
}

void cache_glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    static GLenum cached_BlendFuncSeparate_srcRGB;
    static GLenum cached_BlendFuncSeparate_dstRGB;
    static GLenum cached_BlendFuncSeparate_srcAlpha;
    static GLenum cached_BlendFuncSeparate_dstAlpha;

    if(srcRGB != cached_BlendFuncSeparate_srcRGB || dstRGB != cached_BlendFuncSeparate_dstRGB || srcAlpha != cached_BlendFuncSeparate_srcAlpha || dstAlpha != cached_BlendFuncSeparate_dstAlpha)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "srcRGB: %d dstRGB: %d srcAlpha: %d dstAlpha: %d",srcRGB, dstRGB, srcAlpha, dstAlpha);
        vbo_draw();
        glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
        cached_BlendFuncSeparate_srcRGB = srcRGB;
        cached_BlendFuncSeparate_dstRGB = dstRGB;
        cached_BlendFuncSeparate_srcAlpha = srcAlpha;
        cached_BlendFuncSeparate_dstAlpha = dstAlpha;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - srcRGB: %d dstRGB: %d srcAlpha: %d dstAlpha: %d",srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

void cache_glClearDepthf (GLclampf depth)
{
    static GLclampf cached_depth;

    if(depth != cached_depth)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "depth: %d",depth);
        vbo_draw();
        glClearDepthf(depth);
        cached_depth = depth;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - depth: %d",depth);
    }
}

void cache_glCullFace (GLenum mode)
{
    static GLenum cached_CullFace_mode;

    if(mode != cached_CullFace_mode)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "mode: %d",mode);
        vbo_draw();
        glCullFace(mode);
        cached_CullFace_mode = mode;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - mode: %d",mode);
    }
}

void cache_glDepthFunc (GLenum func)
{
    static GLenum cached_func;

    if(func != cached_func)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "func: %d",func);
        vbo_draw();
        glDepthFunc(func);
        cached_func = func;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - func: %d",func);
    }
}

void cache_glDepthMask (GLboolean flag)
{
    static GLboolean cached_DepthMask_flag;

    if(flag != cached_DepthMask_flag)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "flag: %d",(int)flag);
        vbo_draw();
        glDepthMask(flag);
        cached_DepthMask_flag = flag;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - flag: %d",(int)flag);
    }
}

void cache_glDepthRangef (GLclampf zNear, GLclampf zFar)
{
    static GLclampf cached_zNear;
    static GLclampf cached_zFar;
    if(zNear != cached_zNear || zFar != cached_zFar)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "zNear: %d zFar: %d",zNear, zFar);
        vbo_draw();
        glDepthRangef(zNear, zFar);
        cached_zNear = zNear;
        cached_zFar = zFar;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - zNear: %d zFar: %d",zNear, zFar);
    }
}


void cache_glEnableDisableItem (GLenum cap, bool enable, bool & cached_state, const char * StateName)
{
    if (enable)
    {
        if(!cached_state)
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glEnable(%s)",StateName);
            vbo_draw();
            glEnable(cap);
            cached_state = true;
        }
        else
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - glEnable(%s)",StateName);
        }
    }
    else
    {
        if (cached_state)
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glDisable(%s)",StateName);
            vbo_draw();
            glDisable(cap);
            cached_state = false;
        }
        else
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - glEnable(%s)",StateName);
        }
    }
}

void cache_glEnableDisable (GLenum cap, bool enable)
{
    static bool cached_BLEND = false;
    static bool cached_CULL_FACE = false;
    static bool cached_DEPTH_TEST = false;
    static bool cached_DITHER = false;
    static bool cached_POLYGON_OFFSET_FILL = false;
    static bool cached_SAMPLE_ALPHA_TO_COVERAGE = false;
    static bool cached_SAMPLE_COVERAGE = false;
    static bool cached_SCISSOR_TEST = false;
    static bool cached_STENCIL_TEST = false;
    
    if(cap == GL_BLEND) { cache_glEnableDisableItem(cap, enable, cached_BLEND, "GL_BLEND"); } 
    else if(cap == GL_CULL_FACE) { cache_glEnableDisableItem(cap, enable, cached_CULL_FACE, "GL_CULL_FACE"); } 
    else if(cap == GL_DEPTH_TEST) { cache_glEnableDisableItem(cap, enable, cached_DEPTH_TEST, "GL_DEPTH_TEST"); } 
    else if(cap == GL_DITHER) { cache_glEnableDisableItem(cap, enable, cached_DITHER, "GL_DITHER"); } 
    else if(cap == GL_POLYGON_OFFSET_FILL) { cache_glEnableDisableItem(cap, enable, cached_POLYGON_OFFSET_FILL, "GL_POLYGON_OFFSET_FILL"); } 
    else if(cap == GL_SAMPLE_ALPHA_TO_COVERAGE) { cache_glEnableDisableItem(cap, enable, cached_SAMPLE_ALPHA_TO_COVERAGE, "GL_SAMPLE_ALPHA_TO_COVERAGE"); } 
    else if(cap == GL_SAMPLE_COVERAGE) { cache_glEnableDisableItem(cap, enable, cached_SAMPLE_COVERAGE, "GL_SAMPLE_COVERAGE"); } 
    else if(cap == GL_SCISSOR_TEST) { cache_glEnableDisableItem(cap, enable, cached_SCISSOR_TEST, "GL_SCISSOR_TEST"); } 
    else if(cap == GL_STENCIL_TEST) { cache_glEnableDisableItem(cap, enable, cached_STENCIL_TEST, "GL_STENCIL_TEST"); } 
    else
    {
        if (enable)
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glEnable(%d)",cap);
            vbo_draw();
            glEnable(cap);
        }
        else
        {
            WriteTrace(TraceOGLWrapper, TraceDebug, "glDisable(%d)",cap);
            vbo_draw();
            glDisable(cap);
        }
    }
}

void cache_glFrontFace (GLenum mode)
{
    static GLenum cached_FrontFace_mode;
    if(mode != cached_FrontFace_mode)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "mode: %d",mode);
        vbo_draw();
        glFrontFace(mode);
        cached_FrontFace_mode = mode;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - mode: %d",mode);
    }
}

void cache_glPolygonOffset (GLfloat factor, GLfloat units)
{
    static GLfloat cached_factor;
    static GLfloat cached_units;
    if(factor != cached_factor || units != cached_units)
    {
        vbo_draw();
        WriteTrace(TraceOGLWrapper, TraceDebug, "factor: %f units: %f",factor, units);
        glPolygonOffset(factor, units);
        cached_factor = factor;
        cached_units = units;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - factor: %f units: %f",factor, units);
    }
}

void cache_glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    static GLclampf cached_red, cached_green, cached_blue, cached_alpha;

    if(red != cached_red || green != cached_green || blue != cached_blue || alpha != cached_alpha)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "red: %f, green: %f, blue: %f, alpha: %f", red, green, blue, alpha);
        vbo_draw();
        glClearColor(red, green, blue, alpha);
        cached_red = red;
        cached_green = green;
        cached_blue = blue;
        cached_alpha = alpha;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - red: %f, green: %f, blue: %f, alpha: %f", red, green, blue, alpha);
    }
}

void cache_glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    static GLint cached_x, cached_y;
    static GLsizei cached_width, cached_height;

    if(x != cached_x || y != cached_y || width != cached_width || height != cached_height)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "x: %d, y: %d, width: %d, height: %d", x, y, width, height);
        vbo_draw();
        glScissor(x, y, width, height);
        cached_x = x;
        cached_y = y;
        cached_width = width;
        cached_height = height;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    }
}

void cache_glUseProgram (GLuint program)
{
    static GLuint cached_program;
    if(program != cached_program)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "program: %d", program);
        vbo_draw();
        glUseProgram(program);
        cached_program = program;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "Ignored - program: %d", program);
    }
}

void cache_glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    static GLint cached_x = 0, cached_y = 0;
    static GLsizei cached_width = 0, cached_height = 0;

    if(x != cached_x || y != cached_y || width != cached_width || height != cached_height)
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "x: %d, y: %d, width: %d, height: %d", x, y, width, height);
        vbo_draw();
        glViewport(x, y, width, height);
        cached_x = x;
        cached_y = y;
        cached_width = width;
        cached_height = height;
    }
    else
    {
        WriteTrace(TraceOGLWrapper, TraceDebug, "ignored x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    }
}

