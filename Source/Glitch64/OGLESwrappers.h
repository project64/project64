#pragma once
#include "opengl.h"
void vbo_draw();

void cache_glActiveTexture (GLenum texture);
void cache_glBindTexture (GLenum target, GLuint texture);
void cache_glBlendEquation ( GLenum mode );
void cache_glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha);
void cache_glBlendFunc (GLenum sfactor, GLenum dfactor);
void cache_glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void cache_glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void cache_glClearDepthf (GLclampf depth);
void cache_glCullFace (GLenum mode);
void cache_glDepthFunc (GLenum func);
void cache_glDepthMask (GLboolean flag);
void cache_glDepthRangef (GLclampf zNear, GLclampf zFar);
void cache_glEnableDisable (GLenum cap, bool enable);
void cache_glPolygonOffset (GLfloat factor, GLfloat units);
void cache_glScissor (GLint x, GLint y, GLsizei width, GLsizei height);
void cache_glUseProgram (GLuint program);
void cache_glViewport (GLint x, GLint y, GLsizei width, GLsizei height);

#define glActiveTexture(texture) cache_glActiveTexture(texture)
#define glBindTexture(target, texture) cache_glBindTexture(target, texture)
#define glBlendEquation(mode) cache_glBlendEquation(mode)
#define glBlendEquationSeparate(modeRGB, modeAlpha) cache_glBlendEquationSeparate(modeRGB, modeAlpha)
#define glBlendFunc(sfactor, dfactor) cache_glBlendFunc(sfactor, dfactor)
#define glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha) cache_glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha)
#define glClearColor(red, green, blue, alpha) cache_glClearColor(red, green, blue, alpha)
#define glClearDepthf(depth) cache_glClearDepthf(depth)
#define glCullFace(mode) cache_glCullFace(mode)
#define glDepthFunc(func) cache_glDepthFunc(func)
#define glDepthMask(flag) cache_glDepthMask(flag)
#define glDepthRangef(zNear, zFar) cache_glDepthRangef(zNear, zFar)
#define glDisable(cap) cache_glEnableDisable(cap, false)
#define glEnable(cap) cache_glEnableDisable(cap, true)
#define glFrontFace(mode) cache_glFrontFace(mode)
#define glPolygonOffset(factor, units) cache_glPolygonOffset(factor, units)
#define glScissor(x, y, width, height) cache_glScissor(x, y, width, height)
#define glUseProgram(program) cache_glUseProgram(program)
#define glViewport(x, y, width, height) cache_glViewport(x, y, width, height)

