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

#ifndef _WIN32
//#define VPDEBUG
#endif
#ifdef VPDEBUG
void dump_tex(int id);
void dump_start();
void dump_stop();
extern int dumping;
#endif

#include <Project64-video/trace.h>
#include "types.h"

#define zscale 1.0f

// VP added this utility function
// returns the bytes per pixel of a given GR texture format
int grTexFormatSize(int fmt);

/* 2015.03.07 cxd4 -- regulated GL state machine debugging using glGetError */
extern int grDisplayGLError(const char* message);

extern int packed_pixels_support;
extern int ati_sucks;
extern float largest_supported_anisotropy;

extern int default_texture; // the infamous "32*1024*1024" is now configurable
extern int depth_texture;
void set_depth_shader();
void set_bw_shader();
extern float invtex[2];
extern int buffer_cleared; // mark that the buffer has been cleared, used to check if we need to reload the texture buffer content

#ifdef _WIN32
#include <windows.h>
typedef const char * (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC hdc);
#else
#include <stdio.h>
#endif

#if defined(__ANDROID__) || defined(ANDROID)
#include "OGLESwrappers.h"
#else
#include "opengl.h"

extern "C" {
#ifndef GL_VERSION_1_3
    extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
    extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
#endif
    extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
    extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
    extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
    extern PFNGLBLENDFUNCSEPARATEEXTPROC glBlendFuncSeparateEXT;
    extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
    extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
    extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
    extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
    extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
    extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
    extern PFNGLFOGCOORDFEXTPROC glFogCoordfEXT;
    extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
    extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
    extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
    extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
    extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
    extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
    extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
    extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
    extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
    extern PFNGLSECONDARYCOLOR3FPROC glSecondaryColor3f;
    extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
    extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
    extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
    extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
    extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
    extern PFNGLGETHANDLEARBPROC glGetHandleARB;
}
#endif

void init_geometry();
void init_textures();
void init_combiner();
void free_textures();
void updateCombiner(int i);
void updateCombinera(int i);
void remove_tex(unsigned int idmin, unsigned int idmax);
void add_tex(unsigned int id);

#ifdef _WIN32
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLBLENDFUNCSEPARATEEXTPROC glBlendFuncSeparateEXT;
extern PFNGLFOGCOORDFPROC glFogCoordfEXT;

extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLGETHANDLEARBPROC glGetHandleARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
extern PFNGLUNIFORM4IARBPROC glUniform4iARB;
extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
extern PFNGLSECONDARYCOLOR3FPROC glSecondaryColor3f;
#endif
void check_link(GLuint program);
void vbo_enable();
void vbo_disable();

//Vertex Attribute Locations
#define POSITION_ATTR 0
#define COLOUR_ATTR 1
#define TEXCOORD_0_ATTR 2
#define TEXCOORD_1_ATTR 3
#define FOG_ATTR 4

extern int w_buffer_mode;
extern int g_width, g_height, widtho, heighto;
extern int tex0_width, tex0_height, tex1_width, tex1_height;
extern float lambda;
extern int need_lambda[2];
extern float lambda_color[2][4];
extern int inverted_culling;
extern gfxCullMode_t culling_mode;
extern int render_to_texture;
extern int lfb_color_fmt;
extern int need_to_compile;
extern int blackandwhite0;
extern int blackandwhite1;
extern int TMU_SIZE;

extern int blend_func_separate_support;
extern int fog_coord_support;
extern int glsl_support;
extern unsigned int pBufferAddress;
extern int viewport_width, viewport_height, g_viewport_offset, nvidia_viewport_hack;
extern int UMAmode;

void updateTexture();
void reloadTexture();
void free_combiners();
void compile_shader();
void set_lambda();
void set_copy_shader();
void disable_textureSizes();
void ExitFullScreen();

void CHECK_FRAMEBUFFER_STATUS(void);
