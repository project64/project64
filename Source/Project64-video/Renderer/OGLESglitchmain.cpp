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

#define SAVE_CBUFFER

#ifdef _WIN32
#include <windows.h>
#else
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#endif // _WIN32
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include "glide.h"
#include "g3ext.h"
#include "glitchmain.h"
#include <Project64-video/trace.h>

#define OPENGL_CHECK_ERRORS { const GLenum errcode = glGetError(); if (errcode != GL_NO_ERROR) LOG("OpenGL Error code %i in '%s' line %i\n", errcode, __FILE__, __LINE__-1); }

#ifdef VPDEBUG
#include <IL/il.h>
#endif

extern void(*renderCallback)(int);

wrapper_config config = { 0, 0, 0 };
int screen_width, screen_height;

void Android_JNI_SwapWindow(void);

/*
static inline void opt_glCopyTexImage2D( GLenum target,
GLint level,
GLenum internalFormat,
GLint x,
GLint y,
GLsizei width,
GLsizei height,
GLint border )

{
int w, h, fmt;
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
//printf("copyteximage %dx%d fmt %x oldfmt %x\n", width, height, internalFormat, fmt);
if (w == (int) width && h == (int) height && fmt == (int) internalFormat) {
if (x+width >= screen_width) {
width = screen_width - x;
//printf("resizing w --> %d\n", width);
}
if (y+height >= screen_height+g_viewport_offset) {
height = screen_height+g_viewport_offset - y;
//printf("resizing h --> %d\n", height);
}
glCopyTexSubImage2D(target, level, 0, 0, x, y, width, height);
} else {
//printf("copyteximage %dx%d fmt %x old %dx%d oldfmt %x\n", width, height, internalFormat, w, h, fmt);
//       glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, 0);
//       glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
//       printf("--> %dx%d newfmt %x\n", width, height, fmt);
glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
}
}
#define glCopyTexImage2D opt_glCopyTexImage2D
*/

#ifdef _WIN32
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLBLENDFUNCSEPARATEEXTPROC glBlendFuncSeparateEXT;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLFOGCOORDFPROC glFogCoordfEXT;

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;

PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;

PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
PFNGLUNIFORM1IARBPROC glUniform1iARB;
PFNGLUNIFORM4IARBPROC glUniform4iARB;
PFNGLUNIFORM4FARBPROC glUniform4fARB;
PFNGLUNIFORM1FARBPROC glUniform1fARB;
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
PFNGLSECONDARYCOLOR3FPROC glSecondaryColor3f;

// FXT1,DXT1,DXT5 support - Hiroshi Morii <koolsmoky(at)users.sourceforge.net>
// NOTE: Glide64 + GlideHQ use the following formats
// GL_COMPRESSED_RGB_S3TC_DXT1_EXT
// GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
// GL_COMPRESSED_RGB_FXT1_3DFX
// GL_COMPRESSED_RGBA_FXT1_3DFX
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2DARB;
#endif // _WIN32

typedef struct
{
    unsigned int address;
    int width;
    int height;
    unsigned int fbid;
    unsigned int zbid;
    unsigned int texid;
    int buff_clear;
} fb;

int nbTextureUnits;
int nbAuxBuffers, current_buffer;
int g_scr_res_x, g_width, widtho, heighto, g_scr_res_y, g_height;
int g_res_x, g_res_y;
int saved_width, saved_height;
int blend_func_separate_support;
int npot_support;
int fog_coord_support;
int render_to_texture = 0;
int texture_unit;
int use_fbo;
int buffer_cleared;
// ZIGGY
// to allocate a new static texture name, take the value (free_texture++)
int free_texture;
int default_texture; // the infamous "32*1024*1024" is now configurable
int current_texture;
int depth_texture, color_texture;
int glsl_support = 1;
int viewport_width, viewport_height, g_viewport_offset = 0, nvidia_viewport_hack = 0;
int save_w, save_h;
int lfb_color_fmt;
float invtex[2];
//Gonetz
int UMAmode = 0; //support for VSA-100 UMA mode;

#ifdef _WIN32
static HDC hDC = NULL;
static HGLRC hGLRC = NULL;
static HWND hToolBar = NULL;
#endif // _WIN32
static unsigned long fullscreen;

static int savedWidtho, savedHeighto;
static int savedWidth, savedHeight;
unsigned int pBufferAddress;
static int pBufferFmt;
static int pBufferWidth, pBufferHeight;
static fb fbs[100];
static int nb_fb = 0;
static unsigned int curBufferAddr = 0;

struct TMU_USAGE { int min, max; } tmu_usage[2] = { { 0xfffffff, 0 }, { 0xfffffff, 0 } };

struct texbuf_t {
    FxU32 start, end;
    int fmt;
};
#define NB_TEXBUFS 128 // MUST be a power of two
static texbuf_t texbufs[NB_TEXBUFS];
static int texbuf_i;

unsigned short frameBuffer[2048 * 2048 * 2]; // Support 2048x2048 screen resolution at 32 bits (RGBA) per pixel
unsigned short depthBuffer[2048 * 2048];   // Support 2048x2048 screen resolution at 16 bits (depth) per pixel

//#define VOODOO1

void display_warning(const char *text, ...)
{
    static int first_message = 100;
    if (first_message)
    {
        char buf[4096];

        va_list ap;

        va_start(ap, text);
        vsprintf(buf, text, ap);
        va_end(ap);
        first_message--;
        //LOGINFO(buf);
    }
}

FX_ENTRY void FX_CALL
grSstOrigin(GrOriginLocation_t  origin)
{
    WriteTrace(TraceGlitch, TraceDebug, "origin = %d", origin);
    if (origin != GR_ORIGIN_UPPER_LEFT)
        WriteTrace(TraceGlitch, TraceWarning, "grSstOrigin : %x", origin);
}

FX_ENTRY void FX_CALL
grClipWindow(FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy)
{
    WriteTrace(TraceGlitch, TraceDebug, "minx = %d, miny: %d maxx: %d maxy: %d", minx, miny, maxx, maxy);

    if (use_fbo && render_to_texture) {
        if (int(minx) < 0) minx = 0;
        if (int(miny) < 0) miny = 0;
        if (maxx < minx) maxx = minx;
        if (maxy < miny) maxy = miny;
        glScissor(minx, miny, maxx - minx, maxy - miny);
        glEnable(GL_SCISSOR_TEST);
        return;
    }

    if (!use_fbo) {
        int th = g_height;
        if (th > screen_height)
            th = screen_height;
        maxy = th - maxy;
        miny = th - miny;
        FxU32 tmp = maxy; maxy = miny; miny = tmp;
        if (maxx > (FxU32)g_width) maxx = g_width;
        if (maxy > (FxU32)g_height) maxy = g_height;
        if (int(minx) < 0) minx = 0;
        if (int(miny) < 0) miny = 0;
        if (maxx < minx) maxx = minx;
        if (maxy < miny) maxy = miny;
        glScissor(minx, miny + g_viewport_offset, maxx - minx, maxy - miny);
        //printf("gl scissor %d %d %d %d\n", minx, miny, maxx, maxy);
    }
    else {
        glScissor(minx, (g_viewport_offset)+g_height - maxy, maxx - minx, maxy - miny);
    }
    glEnable(GL_SCISSOR_TEST);
}

FX_ENTRY void FX_CALL
grColorMask(FxBool rgb, FxBool a)
{
    WriteTrace(TraceGlitch, TraceDebug, "rgb = %d, a: %d", rgb, a);
    glColorMask(rgb, rgb, rgb, a);
}

FX_ENTRY void FX_CALL
grGlideInit(void)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
}

int isExtensionSupported(const char *extension)
{
    return 0;
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    where = (GLubyte *)strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    extensions = glGetString(GL_EXTENSIONS);

    start = extensions;
    for (;;)
    {
        where = (GLubyte *)strstr((const char *)start, extension);
        if (!where)
            break;

        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return 1;

        start = terminator;
    }

    return 0;
}

#ifdef _WIN32
int isWglExtensionSupported(const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    where = (GLubyte *)strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    extensions = (GLubyte*)wglGetExtensionsStringARB(wglGetCurrentDC());

    start = extensions;
    for (;;)
    {
        where = (GLubyte *)strstr((const char *)start, extension);
        if (!where)
            break;

        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return 1;

        start = terminator;
    }

    return 0;
}
#endif // _WIN32

#define GrPixelFormat_t int

FX_ENTRY GrContext_t FX_CALL grSstWinOpenExt(GrColorFormat_t color_format, GrOriginLocation_t origin_location, GrPixelFormat_t pixelformat, int nColBuffers, int nAuxBuffers)
{
    WriteTrace(TraceGlitch, TraceDebug, "color_format: %d, origin_location: %d, nColBuffers: %d, nAuxBuffers: %d", color_format, origin_location, nColBuffers, nAuxBuffers);
    return grSstWinOpen(color_format, origin_location, nColBuffers, nAuxBuffers);
}

#ifdef _WIN32
# include <fcntl.h>
# ifndef ATTACH_PARENT_PROCESS
#  define ATTACH_PARENT_PROCESS ((FxU32)-1)
# endif
#endif

FX_ENTRY GrContext_t FX_CALL grSstWinOpen(GrColorFormat_t color_format, GrOriginLocation_t origin_location, int nColBuffers, int nAuxBuffers)
{
    static int show_warning = 1;

    GLCache::ResetCache();
    
    // ZIGGY
    // allocate static texture names
    // the initial value should be big enough to support the maximal resolution
    free_texture = 32 * 2048 * 2048;
    default_texture = free_texture++;
    color_texture = free_texture++;
    depth_texture = free_texture++;

    WriteTrace(TraceGlitch, TraceDebug, "color_format: %d, origin_location: %d, nColBuffers: %d, nAuxBuffers: %d", color_format, origin_location, nColBuffers, nAuxBuffers);
    WriteTrace(TraceGlitch, TraceDebug, "g_width: %d, g_height: %d fullscreen: %d", g_width, g_height, fullscreen);

    //g_viewport_offset = ((screen_resolution>>2) > 20) ? screen_resolution >> 2 : 20;
    // ZIGGY g_viewport_offset is WIN32 specific, with SDL just set it to zero
    g_viewport_offset = 0; //-10 //-20;

    printf("(II) Setting video mode %dx%d...\n", g_width, g_height);

    glViewport(0, g_viewport_offset, g_width, g_height);
    lfb_color_fmt = color_format;
    if (origin_location != GR_ORIGIN_UPPER_LEFT) WriteTrace(TraceGlitch, TraceWarning, "origin must be in upper left corner");
    if (nColBuffers != 2) WriteTrace(TraceGlitch, TraceWarning, "number of color buffer is not 2");
    if (nAuxBuffers != 1) WriteTrace(TraceGlitch, TraceWarning, "number of auxiliary buffer is not 1");

    if (isExtensionSupported("GL_ARB_texture_env_combine") == 0 &&
        isExtensionSupported("GL_EXT_texture_env_combine") == 0 &&
        show_warning)
        WriteTrace(TraceGlitch, TraceWarning, "Your video card doesn't support GL_ARB_texture_env_combine extension");
    if (isExtensionSupported("GL_ARB_multitexture") == 0 && show_warning)
        WriteTrace(TraceGlitch, TraceWarning, "Your video card doesn't support GL_ARB_multitexture extension");
    if (isExtensionSupported("GL_ARB_texture_mirrored_repeat") == 0 && show_warning)
        WriteTrace(TraceGlitch, TraceWarning, "Your video card doesn't support GL_ARB_texture_mirrored_repeat extension");
    show_warning = 0;

#ifdef _WIN32
    glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
    glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
#endif // _WIN32

    nbTextureUnits = 4;
    //glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &nbTextureUnits);
    if (nbTextureUnits == 1) WriteTrace(TraceGlitch, TraceWarning, "You need a video card that has at least 2 texture units");

    nbAuxBuffers = 4;
    //glGetIntegerv(GL_AUX_BUFFERS, &nbAuxBuffers);
    if (nbAuxBuffers > 0)
        printf("Congratulations, you have %d auxilliary buffers, we'll use them wisely !\n", nbAuxBuffers);
#ifdef VOODOO1
    nbTextureUnits = 2;
#endif

    blend_func_separate_support = 1;
    packed_pixels_support = 0;
    /*
      if (isExtensionSupported("GL_EXT_blend_func_separate") == 0)
      blend_func_separate_support = 0;
      else
      blend_func_separate_support = 1;

      if (isExtensionSupported("GL_EXT_packed_pixels") == 0)
      packed_pixels_support = 0;
      else {
      printf("packed pixels extension used\n");
      packed_pixels_support = 1;
      }
      */

    if (isExtensionSupported("GL_ARB_texture_non_power_of_two") == 0)
        npot_support = 0;
    else {
        printf("NPOT extension used\n");
        npot_support = 1;
    }

#ifdef _WIN32
    glBlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)wglGetProcAddress("glBlendFuncSeparateEXT");
#endif // _WIN32

    if (isExtensionSupported("GL_EXT_fog_coord") == 0)
        fog_coord_support = 0;
    else
        fog_coord_support = 1;

#ifdef _WIN32
    glFogCoordfEXT = (PFNGLFOGCOORDFPROC)wglGetProcAddress("glFogCoordfEXT");
#endif // _WIN32

#ifdef _WIN32
    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
#endif // _WIN32

#ifdef _WIN32
    glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
    glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
    glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");

    glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
    glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
    glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
    glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
    use_fbo = config.fbo && (glFramebufferRenderbufferEXT != NULL);
#else
    use_fbo = config.fbo;
#endif // _WIN32

    //LOGINFO("use_fbo %d\n", use_fbo);

    if (isExtensionSupported("GL_ARB_shading_language_100") &&
        isExtensionSupported("GL_ARB_shader_objects") &&
        isExtensionSupported("GL_ARB_fragment_shader") &&
        isExtensionSupported("GL_ARB_vertex_shader"))
    {
#ifdef _WIN32
        glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)wglGetProcAddress("glCreateShaderObjectARB");
        glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)wglGetProcAddress("glShaderSourceARB");
        glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)wglGetProcAddress("glCompileShaderARB");
        glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)wglGetProcAddress("glCreateProgramObjectARB");
        glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)wglGetProcAddress("glAttachObjectARB");
        glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)wglGetProcAddress("glLinkProgramARB");
        glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)wglGetProcAddress("glUseProgramObjectARB");
        glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)wglGetProcAddress("glGetUniformLocationARB");
        glUniform1iARB = (PFNGLUNIFORM1IARBPROC)wglGetProcAddress("glUniform1iARB");
        glUniform4iARB = (PFNGLUNIFORM4IARBPROC)wglGetProcAddress("glUniform4iARB");
        glUniform4fARB = (PFNGLUNIFORM4FARBPROC)wglGetProcAddress("glUniform4fARB");
        glUniform1fARB = (PFNGLUNIFORM1FARBPROC)wglGetProcAddress("glUniform1fARB");
        glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)wglGetProcAddress("glDeleteObjectARB");
        glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)wglGetProcAddress("glGetInfoLogARB");
        glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)wglGetProcAddress("glGetObjectParameterivARB");

        glSecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC)wglGetProcAddress("glSecondaryColor3f");
#endif // _WIN32
    }

    if (isExtensionSupported("GL_EXT_texture_compression_s3tc") == 0 && show_warning)
        WriteTrace(TraceGlitch, TraceWarning, "Your video card doesn't support GL_EXT_texture_compression_s3tc extension");
    if (isExtensionSupported("GL_3DFX_texture_compression_FXT1") == 0 && show_warning)
        WriteTrace(TraceGlitch, TraceWarning, "Your video card doesn't support GL_3DFX_texture_compression_FXT1 extension");

#ifdef _WIN32
    glCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2DARB");
#endif

#ifdef _WIN32
    glViewport(0, g_viewport_offset, width, height);
    viewport_width = width;
    viewport_height = height;
    nvidia_viewport_hack = 1;
#else
    glViewport(0, g_viewport_offset, g_width, g_height);
    viewport_width = g_width;
    viewport_height = g_height;
#endif // _WIN32

    //   void do_benchmarks();
    //   do_benchmarks();

    // VP try to resolve z precision issues
    //  glMatrixMode(GL_MODELVIEW);
    //  glLoadIdentity();
    //  glTranslatef(0, 0, 1-zscale);
    //  glScalef(1, 1, zscale);

    widtho = g_width / 2;
    heighto = g_height / 2;

    pBufferWidth = pBufferHeight = -1;

    current_buffer = GL_BACK;

    texture_unit = GL_TEXTURE0;

    {
        int i;
        for (i = 0; i < NB_TEXBUFS; i++)
            texbufs[i].start = texbufs[i].end = 0xffffffff;
    }

    if (!use_fbo && nbAuxBuffers == 0) {
        // create the framebuffer saving texture
        int w = g_width, h = g_height;
        glBindTexture(GL_TEXTURE_2D, color_texture);
        if (!npot_support) {
            w = h = 1;
            while (w < g_width) w *= 2;
            while (h < g_height) h *= 2;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        save_w = save_h = 0;
    }

    //void FindBestDepthBias();
    //FindBestDepthBias();

    init_geometry();
    init_textures();
    init_combiner();

    /*
      // Aniso filter check
      if (config.anisofilter > 0 )
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);

      // ATI hack - certain texture formats are slow on ATI?
      // Hmm, perhaps the internal format need to be specified explicitly...
      {
      GLint ifmt;
      glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, NULL);
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &ifmt);
      if (ifmt != GL_RGB5_A1) {
      WriteTrace(TraceGlitch, TraceWarning, "ATI SUCKS %x\n", ifmt);
      ati_sucks = 1;
      } else
      ati_sucks = 0;
      }
      */

    return 1;
}

FX_ENTRY void FX_CALL
grGlideShutdown(void)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
}

FX_ENTRY FxBool FX_CALL
grSstWinClose(GrContext_t context)
{
    int i, clear_texbuff = use_fbo;
    WriteTrace(TraceGlitch, TraceDebug, "context: %d", context);

    for (i = 0; i < 2; i++) {
        tmu_usage[i].min = 0xfffffff;
        tmu_usage[i].max = 0;
        invtex[i] = 0;
    }

    free_combiners();
#ifndef _WIN32
    try // I don't know why, but opengl can be killed before this function call when emulator is closed (Gonetz).
        // ZIGGY : I found the problem : it is a function pointer, when the extension isn't supported , it is then zero, so just need to check the pointer prior to do the call.
    {
        if (use_fbo)
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    catch (...)
    {
        clear_texbuff = 0;
    }

    if (clear_texbuff)
    {
        for (i = 0; i < nb_fb; i++)
        {
            glDeleteTextures(1, &(fbs[i].texid));
            glDeleteFramebuffers(1, &(fbs[i].fbid));
            glDeleteRenderbuffers(1, &(fbs[i].zbid));
        }
    }
#endif
    nb_fb = 0;

    free_textures();
#ifndef _WIN32
    // ZIGGY for some reasons, Pj64 doesn't like remove_tex on exit
    remove_tex(0, 0xfffffff);
#endif

    //*/
#ifdef _WIN32
    if (hGLRC)
    {
        wglMakeCurrent(hDC, NULL);
        wglDeleteContext(hGLRC);
        hGLRC = NULL;
    }
    ExitFullScreen();
#endif
    return FXTRUE;
}

FX_ENTRY void FX_CALL grTextureBufferExt(GrChipID_t  		tmu,
    FxU32 				startAddress,
    GrLOD_t 			lodmin,
    GrLOD_t 			lodmax,
    GrAspectRatio_t 	aspect,
    GrTextureFormat_t 	fmt,
    FxU32 				evenOdd)
{
    int i;
    static int fbs_init = 0;

    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d startAddress: %d lodmin: %d lodmax: %d aspect: %d fmt: %d evenOdd: %d", tmu, startAddress, lodmin, lodmax, aspect, fmt, evenOdd);
    if (lodmin != lodmax) WriteTrace(TraceGlitch, TraceWarning, "grTextureBufferExt : loading more than one LOD");
    if (!use_fbo) {
        if (!render_to_texture) { //initialization
            return;
        }

        render_to_texture = 2;

        if (aspect < 0)
        {
            pBufferHeight = 1 << lodmin;
            pBufferWidth = pBufferHeight >> -aspect;
        }
        else
        {
            pBufferWidth = 1 << lodmin;
            pBufferHeight = pBufferWidth >> aspect;
        }

        if (curBufferAddr && startAddress + 1 != curBufferAddr)
            updateTexture();
#ifdef SAVE_CBUFFER
        //printf("saving %dx%d\n", pBufferWidth, pBufferHeight);
        // save color buffer
        if (nbAuxBuffers > 0) {
            //glDrawBuffer(GL_AUX0);
            //current_buffer = GL_AUX0;
        }
        else {
            int tw, th;
            if (pBufferWidth < screen_width)
                tw = pBufferWidth;
            else
                tw = screen_width;
            if (pBufferHeight < screen_height)
                th = pBufferHeight;
            else
                th = screen_height;
            //glReadBuffer(GL_BACK);
            glActiveTexture(texture_unit);
            glBindTexture(GL_TEXTURE_2D, color_texture);
            // save incrementally the framebuffer
            if (save_w) {
                if (tw > save_w && th > save_h) {
                    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, save_h,
                        0, g_viewport_offset + save_h, tw, th - save_h);
                    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, save_w, 0,
                        save_w, g_viewport_offset, tw - save_w, save_h);
                    save_w = tw;
                    save_h = th;
                }
                else if (tw > save_w) {
                    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, save_w, 0,
                        save_w, g_viewport_offset, tw - save_w, save_h);
                    save_w = tw;
                }
                else if (th > save_h) {
                    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, save_h,
                        0, g_viewport_offset + save_h, save_w, th - save_h);
                    save_h = th;
                }
            }
            else {
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    0, g_viewport_offset, tw, th);
                save_w = tw;
                save_h = th;
            }
            glBindTexture(GL_TEXTURE_2D, default_texture);
        }
#endif

        if (startAddress + 1 != curBufferAddr ||
            (curBufferAddr == 0L && nbAuxBuffers == 0))
            buffer_cleared = 0;

        curBufferAddr = pBufferAddress = startAddress + 1;
        pBufferFmt = fmt;

        int rtmu = startAddress < grTexMinAddress(GR_TMU1) ? 0 : 1;
        int size = pBufferWidth*pBufferHeight * 2; //grTexFormatSize(fmt);
        if ((unsigned int)tmu_usage[rtmu].min > pBufferAddress)
            tmu_usage[rtmu].min = pBufferAddress;
        if ((unsigned int)tmu_usage[rtmu].max < pBufferAddress + size)
            tmu_usage[rtmu].max = pBufferAddress + size;
        //   printf("tmu %d usage now %gMb - %gMb\n",
        //          rtmu, tmu_usage[rtmu].min/1024.0f, tmu_usage[rtmu].max/1024.0f);

        g_width = pBufferWidth;
        g_height = pBufferHeight;

        widtho = g_width / 2;
        heighto = g_height / 2;

        // this could be improved, but might be enough as long as the set of
        // texture buffer addresses stay small
        for (i = (texbuf_i - 1)&(NB_TEXBUFS - 1); i != texbuf_i; i = (i - 1)&(NB_TEXBUFS - 1))
            if (texbufs[i].start == pBufferAddress)
                break;
        texbufs[i].start = pBufferAddress;
        texbufs[i].end = pBufferAddress + size;
        texbufs[i].fmt = fmt;
        if (i == texbuf_i)
            texbuf_i = (texbuf_i + 1)&(NB_TEXBUFS - 1);
        //printf("texbuf %x fmt %x\n", pBufferAddress, fmt);

        // ZIGGY it speeds things up to not delete the buffers
        // a better thing would be to delete them *sometimes*
        //   remove_tex(pBufferAddress+1, pBufferAddress + size);
        add_tex(pBufferAddress);

        //printf("viewport %dx%d\n", width, height);
        if (g_height > screen_height) {
            glViewport(0, g_viewport_offset + screen_height - g_height, g_width, g_height);
        }
        else
            glViewport(0, g_viewport_offset, g_width, g_height);

        glScissor(0, g_viewport_offset, g_width, g_height);
    }
    else {
        if (!render_to_texture) //initialization
        {
            if (!fbs_init)
            {
                for (i = 0; i < 100; i++) fbs[i].address = 0;
                fbs_init = 1;
                nb_fb = 0;
            }
            return; //no need to allocate FBO if render buffer is not texture buffer
        }

        render_to_texture = 2;

        if (aspect < 0)
        {
            pBufferHeight = 1 << lodmin;
            pBufferWidth = pBufferHeight >> -aspect;
        }
        else
        {
            pBufferWidth = 1 << lodmin;
            pBufferHeight = pBufferWidth >> aspect;
        }
        pBufferAddress = startAddress + 1;

        g_width = pBufferWidth;
        g_height = pBufferHeight;

        widtho = g_width / 2;
        heighto = g_height / 2;

        for (i = 0; i < nb_fb; i++)
        {
            if (fbs[i].address == pBufferAddress)
            {
                if (fbs[i].width == g_width && fbs[i].height == g_height) //select already allocated FBO
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbs[i].fbid);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbs[i].texid, 0);
                    glBindRenderbuffer(GL_RENDERBUFFER, fbs[i].zbid);
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbs[i].zbid);
                    glViewport(0, 0, g_width, g_height);
                    glScissor(0, 0, g_width, g_height);
                    if (fbs[i].buff_clear)
                    {
                        glDepthMask(1);
                        glClear(GL_DEPTH_BUFFER_BIT); //clear z-buffer only. we may need content, stored in the frame buffer
                        fbs[i].buff_clear = 0;
                    }
                    CHECK_FRAMEBUFFER_STATUS();
                    curBufferAddr = pBufferAddress;
                    return;
                }
                else //create new FBO at the same address, delete old one
                {
                    glDeleteFramebuffers(1, &(fbs[i].fbid));
                    glDeleteRenderbuffers(1, &(fbs[i].zbid));
                    if (nb_fb > 1)
                        memmove(&(fbs[i]), &(fbs[i + 1]), sizeof(fb)*(nb_fb - i));
                    nb_fb--;
                    break;
                }
            }
        }

        remove_tex(pBufferAddress, pBufferAddress + g_width*g_height * 2/*grTexFormatSize(fmt)*/);
        //create new FBO
        glGenFramebuffers(1, &(fbs[nb_fb].fbid));
        glGenRenderbuffers(1, &(fbs[nb_fb].zbid));
        glBindRenderbuffer(GL_RENDERBUFFER, fbs[nb_fb].zbid);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, g_width, g_height);
        fbs[nb_fb].address = pBufferAddress;
        fbs[nb_fb].width = g_width;
        fbs[nb_fb].height = g_height;
        fbs[nb_fb].texid = pBufferAddress;
        fbs[nb_fb].buff_clear = 0;
        add_tex(fbs[nb_fb].texid);
        glBindTexture(GL_TEXTURE_2D, fbs[nb_fb].texid);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_width, g_height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fbs[nb_fb].fbid);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbs[nb_fb].texid, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbs[nb_fb].zbid);
        glViewport(0, 0, g_width, g_height);
        glScissor(0, 0, g_width, g_height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glDepthMask(1);
        glClear(GL_DEPTH_BUFFER_BIT);
        CHECK_FRAMEBUFFER_STATUS();
        curBufferAddr = pBufferAddress;
        nb_fb++;
    }
}

int CheckTextureBufferFormat(GrChipID_t tmu, FxU32 startAddress, GrTexInfo *info)
{
    int found, i;
    if (!use_fbo) {
        for (found = i = 0; i < 2; i++)
            if ((FxU32)tmu_usage[i].min <= startAddress && (FxU32)tmu_usage[i].max > startAddress) {
                //printf("tmu %d == framebuffer %x\n", tmu, startAddress);
                found = 1;
                break;
            }
    }
    else {
        found = i = 0;
        while (i < nb_fb)
        {
            unsigned int end = fbs[i].address + fbs[i].width*fbs[i].height * 2;
            if (startAddress >= fbs[i].address &&  startAddress < end)
            {
                found = 1;
                break;
            }
            i++;
        }
    }

    if (!use_fbo && found) {
        int tw, th, rh, cw, ch;
        if (info->aspectRatioLog2 < 0)
        {
            th = 1 << info->largeLodLog2;
            tw = th >> -info->aspectRatioLog2;
        }
        else
        {
            tw = 1 << info->largeLodLog2;
            th = tw >> info->aspectRatioLog2;
        }

        if (info->aspectRatioLog2 < 0)
        {
            ch = 256;
            cw = ch >> -info->aspectRatioLog2;
        }
        else
        {
            cw = 256;
            ch = cw >> info->aspectRatioLog2;
        }

        if (use_fbo || th < screen_height)
            rh = th;
        else
            rh = screen_height;

        //printf("th %d rh %d ch %d\n", th, rh, ch);

        invtex[tmu] = 1.0f - (th - rh) / (float)th;
    }
    else
        invtex[tmu] = 0;

    if (info->format == GR_TEXFMT_ALPHA_INTENSITY_88) {
        if (!found) {
            return 0;
        }
        if (tmu == 0)
        {
            if (blackandwhite1 != found)
            {
                blackandwhite1 = found;
                need_to_compile = 1;
            }
        }
        else
        {
            if (blackandwhite0 != found)
            {
                blackandwhite0 = found;
                need_to_compile = 1;
            }
        }
        return 1;
    }
    return 0;
}

FX_ENTRY void FX_CALL
grTextureAuxBufferExt(GrChipID_t tmu,
    FxU32      startAddress,
    GrLOD_t    thisLOD,
    GrLOD_t    largeLOD,
    GrAspectRatio_t aspectRatio,
    GrTextureFormat_t format,
    FxU32      odd_even_mask)
{
    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d startAddress: %d thisLOD: %d largeLOD: %d aspectRatio: %d format: %d odd_even_mask: %d", tmu, startAddress, thisLOD, largeLOD, aspectRatio, format, odd_even_mask);
    //WriteTrace(TraceGlitch, TraceWarning, "grTextureAuxBufferExt");
}

FX_ENTRY void FX_CALL grAuxBufferExt(GrBuffer_t buffer);

FX_ENTRY GrProc FX_CALL
grGetProcAddress(char *procName)
{
    WriteTrace(TraceGlitch, TraceDebug, "procName: %s", procName);
    if (!strcmp(procName, "grSstWinOpenExt"))
        return (GrProc)grSstWinOpenExt;
    if (!strcmp(procName, "grTextureBufferExt"))
        return (GrProc)grTextureBufferExt;
    if (!strcmp(procName, "grChromaRangeExt"))
        return (GrProc)grChromaRangeExt;
    if (!strcmp(procName, "grChromaRangeModeExt"))
        return (GrProc)grChromaRangeModeExt;
    if (!strcmp(procName, "grTexChromaRangeExt"))
        return (GrProc)grTexChromaRangeExt;
    if (!strcmp(procName, "grTexChromaModeExt"))
        return (GrProc)grTexChromaModeExt;
    // ZIGGY framebuffer copy extension
    if (!strcmp(procName, "grFramebufferCopyExt"))
        return (GrProc)grFramebufferCopyExt;
    if (!strcmp(procName, "grColorCombineExt"))
        return (GrProc)grColorCombineExt;
    if (!strcmp(procName, "grAlphaCombineExt"))
        return (GrProc)grAlphaCombineExt;
    if (!strcmp(procName, "grTexColorCombineExt"))
        return (GrProc)grTexColorCombineExt;
    if (!strcmp(procName, "grTexAlphaCombineExt"))
        return (GrProc)grTexAlphaCombineExt;
    if (!strcmp(procName, "grConstantColorValueExt"))
        return (GrProc)grConstantColorValueExt;
    if (!strcmp(procName, "grTextureAuxBufferExt"))
        return (GrProc)grTextureAuxBufferExt;
    if (!strcmp(procName, "grAuxBufferExt"))
        return (GrProc)grAuxBufferExt;
    if (!strcmp(procName, "grConfigWrapperExt"))
        return (GrProc)grConfigWrapperExt;
    if (!strcmp(procName, "grKeyPressedExt"))
        return (GrProc)grKeyPressedExt;
    if (!strcmp(procName, "grQueryResolutionsExt"))
        return (GrProc)grQueryResolutionsExt;
    if (!strcmp(procName, "grGetGammaTableExt"))
        return (GrProc)grGetGammaTableExt;
    WriteTrace(TraceGlitch, TraceWarning, "grGetProcAddress : %s", procName);
    return 0;
}

FX_ENTRY FxU32 FX_CALL
grGet(FxU32 pname, FxU32 plength, FxI32 *params)
{
    WriteTrace(TraceGlitch, TraceDebug, "pname: %d plength: %d", pname, plength);
    switch (pname)
    {
    case GR_MAX_TEXTURE_SIZE:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 2048;
        return 4;
        break;
    case GR_NUM_TMU:
        if (plength < 4 || params == NULL) return 0;
        if (!nbTextureUnits)
        {
            grSstWinOpen(GR_COLORFORMAT_ARGB, GR_ORIGIN_UPPER_LEFT, 2, 1);
            grSstWinClose(0);
        }
#ifdef VOODOO1
        params[0] = 1;
#else
        if (nbTextureUnits > 2)
            params[0] = 2;
        else
            params[0] = 1;
#endif
        return 4;
        break;
    case GR_NUM_BOARDS:
    case GR_NUM_FB:
    case GR_REVISION_FB:
    case GR_REVISION_TMU:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 1;
        return 4;
        break;
    case GR_MEMORY_FB:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 16 * 1024 * 1024;
        return 4;
        break;
    case GR_MEMORY_TMU:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 16 * 1024 * 1024;
        return 4;
        break;
    case GR_MEMORY_UMA:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 16 * 1024 * 1024 * nbTextureUnits;
        return 4;
        break;
    case GR_BITS_RGBA:
        if (plength < 16 || params == NULL) return 0;
        params[0] = 8;
        params[1] = 8;
        params[2] = 8;
        params[3] = 8;
        return 16;
        break;
    case GR_BITS_DEPTH:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 16;
        return 4;
        break;
    case GR_BITS_GAMMA:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 8;
        return 4;
        break;
    case GR_GAMMA_TABLE_ENTRIES:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 256;
        return 4;
        break;
    case GR_FOG_TABLE_ENTRIES:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 64;
        return 4;
        break;
    case GR_WDEPTH_MIN_MAX:
        if (plength < 8 || params == NULL) return 0;
        params[0] = 0;
        params[1] = 65528;
        return 8;
        break;
    case GR_ZDEPTH_MIN_MAX:
        if (plength < 8 || params == NULL) return 0;
        params[0] = 0;
        params[1] = 65535;
        return 8;
        break;
    case GR_LFB_PIXEL_PIPE:
        if (plength < 4 || params == NULL) return 0;
        params[0] = FXFALSE;
        return 4;
        break;
    case GR_MAX_TEXTURE_ASPECT_RATIO:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 3;
        return 4;
        break;
    case GR_NON_POWER_OF_TWO_TEXTURES:
        if (plength < 4 || params == NULL) return 0;
        params[0] = FXFALSE;
        return 4;
        break;
    case GR_TEXTURE_ALIGN:
        if (plength < 4 || params == NULL) return 0;
        params[0] = 0;
        return 4;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown pname in grGet : %x", pname);
    }
    return 0;
}

FX_ENTRY const char * FX_CALL
grGetString(FxU32 pname)
{
    WriteTrace(TraceGlitch, TraceDebug, "pname: %d", pname);
    switch (pname)
    {
    case GR_EXTENSION:
    {
        static char extension[] = "CHROMARANGE TEXCHROMA TEXMIRROR PALETTE6666 FOGCOORD EVOODOO TEXTUREBUFFER TEXUMA TEXFMT COMBINE GETGAMMA";
        return extension;
    }
    break;
    case GR_HARDWARE:
    {
        static char hardware[] = "Voodoo5 (tm)";
        return hardware;
    }
    break;
    case GR_VENDOR:
    {
        static char vendor[] = "3Dfx Interactive";
        return vendor;
    }
    break;
    case GR_RENDERER:
    {
        static char renderer[] = "Glide";
        return renderer;
    }
    break;
    case GR_VERSION:
    {
        static char version[] = "3.0";
        return version;
    }
    break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown grGetString selector : %x", pname);
    }
    return NULL;
}

static void render_rectangle(int texture_number,
    int dst_x, int dst_y,
    int src_width, int src_height,
    int tex_width, int tex_height, int invert)
{
    //LOGINFO("render_rectangle(%d,%d,%d,%d,%d,%d,%d,%d)",texture_number,dst_x,dst_y,src_width,src_height,tex_width,tex_height,invert);
    static float data[] = {
        (float)((int)dst_x),                      //X 0
        (float)(invert*-((int)dst_y)),            //Y 0
        0.0f,                                     //U 0
        0.0f,                                     //V 0

        (float)((int)dst_x),                      //X 1
        (float)(invert*-((int)dst_y + (int)src_height)),   //Y 1
        0.0f,                                     //U 1
        (float)src_height / (float)tex_height,    //V 1

        (float)((int)dst_x + (int)src_width),
        (float)(invert*-((int)dst_y + (int)src_height)),
        (float)src_width / (float)tex_width,
        (float)src_height / (float)tex_height,

        (float)((int)dst_x),
        (float)(invert*-((int)dst_y)),
        0.0f,
        0.0f
    };

    vbo_disable();
    glDisableVertexAttribArray(COLOUR_ATTR);
    glDisableVertexAttribArray(TEXCOORD_1_ATTR);
    glDisableVertexAttribArray(FOG_ATTR);

    glVertexAttribPointer(POSITION_ATTR, 2, GL_FLOAT, false, 2, data); //Position
    glVertexAttribPointer(TEXCOORD_0_ATTR, 2, GL_FLOAT, false, 2, &data[2]); //Tex

    glEnableVertexAttribArray(COLOUR_ATTR);
    glEnableVertexAttribArray(TEXCOORD_1_ATTR);
    glEnableVertexAttribArray(FOG_ATTR);

    disable_textureSizes();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vbo_enable();

    /*
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glBegin(GL_QUADS);
      glMultiTexCoord2fARB(texture_number, 0.0f, 0.0f);
      glVertex2f(((int)dst_x - widtho) / (float)(width/2),
      invert*-((int)dst_y - heighto) / (float)(height/2));
      glMultiTexCoord2fARB(texture_number, 0.0f, (float)src_height / (float)tex_height);
      glVertex2f(((int)dst_x - widtho) / (float)(width/2),
      invert*-((int)dst_y + (int)src_height - heighto) / (float)(height/2));
      glMultiTexCoord2fARB(texture_number, (float)src_width / (float)tex_width, (float)src_height / (float)tex_height);
      glVertex2f(((int)dst_x + (int)src_width - widtho) / (float)(width/2),
      invert*-((int)dst_y + (int)src_height - heighto) / (float)(height/2));
      glMultiTexCoord2fARB(texture_number, (float)src_width / (float)tex_width, 0.0f);
      glVertex2f(((int)dst_x + (int)src_width - widtho) / (float)(width/2),
      invert*-((int)dst_y - heighto) / (float)(height/2));
      glMultiTexCoord2fARB(texture_number, 0.0f, 0.0f);
      glVertex2f(((int)dst_x - widtho) / (float)(width/2),
      invert*-((int)dst_y - heighto) / (float)(height/2));
      glEnd();
      */

    compile_shader();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

void reloadTexture()
{
    if (use_fbo || !render_to_texture || buffer_cleared)
        return;

    WriteTrace(TraceGlitch, TraceDebug, "width: %d height: %d", g_width, g_height);
    //printf("reload texture %dx%d\n", width, height);

    buffer_cleared = 1;

    //glPushAttrib(GL_ALL_ATTRIB_BITS);
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, pBufferAddress);
    //glDisable(GL_ALPHA_TEST);
    //glDrawBuffer(current_buffer);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    set_copy_shader();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    int w = 0, h = 0;
    if (g_height > screen_height) h = screen_height - g_height;
    render_rectangle(texture_unit,
        -w, -h,
        g_width, g_height,
        g_width, g_height, -1);
    glBindTexture(GL_TEXTURE_2D, default_texture);
    //glPopAttrib();
}

void updateTexture()
{
    if (!use_fbo && render_to_texture == 2) {
        WriteTrace(TraceGlitch, TraceDebug, "pBufferAddress: %x", pBufferAddress);
        //printf("update texture %x\n", pBufferAddress);

        // nothing changed, don't update the texture
        if (!buffer_cleared) {
            WriteTrace(TraceGlitch, TraceDebug, "update cancelled");
            return;
        }

        //glPushAttrib(GL_ALL_ATTRIB_BITS);

        // save result of render to texture into actual texture
        //glReadBuffer(current_buffer);
        glActiveTexture(texture_unit);
        // ZIGGY
        // deleting the texture before resampling it increases speed on certain old
        // nvidia cards (geforce 2 for example), unfortunatly it slows down a lot
        // on newer cards.
        //glDeleteTextures( 1, &pBufferAddress );
        glBindTexture(GL_TEXTURE_2D, pBufferAddress);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
            0, g_viewport_offset, g_width, g_height, 0);

        glBindTexture(GL_TEXTURE_2D, default_texture);
        //glPopAttrib();
    }
}

FX_ENTRY void FX_CALL grFramebufferCopyExt(int x, int y, int w, int h,
    int from, int to, int mode)
{
    if (mode == GR_FBCOPY_MODE_DEPTH) {
        int tw = 1, th = 1;
        if (npot_support) {
            tw = g_width; th = g_height;
        }
        else {
            while (tw < g_width) tw <<= 1;
            while (th < g_height) th <<= 1;
        }

        if (from == GR_FBCOPY_BUFFER_BACK && to == GR_FBCOPY_BUFFER_FRONT) {
            //printf("save depth buffer %d\n", render_to_texture);
            // save the depth image in a texture
            //glReadBuffer(current_buffer);
            glBindTexture(GL_TEXTURE_2D, depth_texture);
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                0, g_viewport_offset, tw, th, 0);
            glBindTexture(GL_TEXTURE_2D, default_texture);
            return;
        }
        if (from == GR_FBCOPY_BUFFER_FRONT && to == GR_FBCOPY_BUFFER_BACK) {
            //printf("writing to depth buffer %d\n", render_to_texture);
            //glPushAttrib(GL_ALL_ATTRIB_BITS);
            //glDisable(GL_ALPHA_TEST);
            //glDrawBuffer(current_buffer);
            glActiveTexture(texture_unit);
            glBindTexture(GL_TEXTURE_2D, depth_texture);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            set_depth_shader();
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);
            glDisable(GL_CULL_FACE);
            render_rectangle(texture_unit,
                0, 0,
                g_width, g_height,
                tw, th, -1);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glBindTexture(GL_TEXTURE_2D, default_texture);
            //glPopAttrib();
            return;
        }
    }
}

FX_ENTRY void FX_CALL
grRenderBuffer(GrBuffer_t buffer)
{
#ifdef _WIN32
    static HANDLE region = NULL;
    int realWidth = pBufferWidth, realHeight = pBufferHeight;
#endif // _WIN32
    WriteTrace(TraceGlitch, TraceDebug, "buffer: %d", buffer);
    //printf("grRenderBuffer(%d)\n", buffer);

    switch (buffer)
    {
    case GR_BUFFER_BACKBUFFER:
        if (render_to_texture)
        {
            updateTexture();

            // VP z fix
            //glMatrixMode(GL_MODELVIEW);
            //glLoadIdentity();
            //glTranslatef(0, 0, 1-zscale);
            //glScalef(1, 1, zscale);
            inverted_culling = 0;
            grCullMode(culling_mode);

            g_width = savedWidth;
            g_height = savedHeight;
            widtho = savedWidtho;
            heighto = savedHeighto;
            if (use_fbo) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }
            curBufferAddr = 0;

            glViewport(0, g_viewport_offset, g_width, viewport_height);
            glScissor(0, g_viewport_offset, g_width, g_height);

#ifdef SAVE_CBUFFER
            if (!use_fbo && render_to_texture == 2) {
                // restore color buffer
                if (nbAuxBuffers > 0) {
                    //glDrawBuffer(GL_BACK);
                    current_buffer = GL_BACK;
                }
                else if (save_w) {
                    int tw = 1, th = 1;
                    //printf("restore %dx%d\n", save_w, save_h);
                    if (npot_support) {
                        tw = screen_width;
                        th = screen_height;
                    }
                    else {
                        while (tw < screen_width) tw <<= 1;
                        while (th < screen_height) th <<= 1;
                    }

                    //glPushAttrib(GL_ALL_ATTRIB_BITS);
                    //glDisable(GL_ALPHA_TEST);
                    //glDrawBuffer(GL_BACK);
                    glActiveTexture(texture_unit);
                    glBindTexture(GL_TEXTURE_2D, color_texture);
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    set_copy_shader();
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_CULL_FACE);
                    render_rectangle(texture_unit,
                        0, 0,
                        save_w, save_h,
                        tw, th, -1);
                    glBindTexture(GL_TEXTURE_2D, default_texture);
                    //glPopAttrib();

                    save_w = save_h = 0;
                }
            }
#endif
            render_to_texture = 0;
        }
        //glDrawBuffer(GL_BACK);
        break;
    case 6: // RENDER TO TEXTURE
        if (!render_to_texture)
        {
            savedWidth = g_width;
            savedHeight = g_height;
            savedWidtho = widtho;
            savedHeighto = heighto;
        }

        {
            if (!use_fbo) {
                //glMatrixMode(GL_MODELVIEW);
                //glLoadIdentity();
                //glTranslatef(0, 0, 1-zscale);
                //glScalef(1, 1, zscale);
                inverted_culling = 0;
            }
            else {
                /*
                        float m[4*4] = {1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f,-1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f};
                        glMatrixMode(GL_MODELVIEW);
                        glLoadMatrixf(m);
                        // VP z fix
                        glTranslatef(0, 0, 1-zscale);
                        glScalef(1, 1*1, zscale);
                        */
                inverted_culling = 1;
                grCullMode(culling_mode);
            }
        }
        render_to_texture = 1;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "grRenderBuffer : unknown buffer : %x", buffer);
    }
}

FX_ENTRY void FX_CALL
grAuxBufferExt(GrBuffer_t buffer)
{
    WriteTrace(TraceGlitch, TraceDebug, "buffer: %d", buffer);
    //WriteTrace(TraceGlitch, TraceWarning, "grAuxBufferExt");

    if (buffer == GR_BUFFER_AUXBUFFER) {
        invtex[0] = 0;
        invtex[1] = 0;
        need_to_compile = 0;
        set_depth_shader();
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glDisable(GL_CULL_FACE);
        //glDisable(GL_ALPHA_TEST);
        glDepthMask(GL_TRUE);
        grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
    }
    else {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        need_to_compile = 1;
    }
}

FX_ENTRY void FX_CALL
grBufferClear(GrColor_t color, GrAlpha_t alpha, FxU32 depth)
{
    WriteTrace(TraceGlitch, TraceDebug, "color: %d alpha: %d depth: %d", color, alpha, depth);
    vbo_draw();
    switch (lfb_color_fmt)
    {
    case GR_COLORFORMAT_ARGB:
        glClearColor(((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f,
            (color & 0xFF) / 255.0f,
            alpha / 255.0f);
        break;
    case GR_COLORFORMAT_RGBA:
        glClearColor(((color >> 24) & 0xFF) / 255.0f,
            ((color >> 16) & 0xFF) / 255.0f,
            (color & 0xFF) / 255.0f,
            alpha / 255.0f);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "grBufferClear: unknown color format : %x", lfb_color_fmt);
    }

    glClear(GL_COLOR_BUFFER_BIT);

    if (w_buffer_mode)
        glClearDepthf(1.0f - ((1.0f + (depth >> 4) / 4096.0f) * (1 << (depth & 0xF))) / 65528.0);
    else
        glClearDepthf(depth / 65535.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ZIGGY TODO check that color mask is on
    buffer_cleared = 1;
}

// #include <unistd.h>
FX_ENTRY void FX_CALL
grBufferSwap(FxU32 swap_interval)
{
    //   GLuint program;

    vbo_draw();
    //	glFinish();
    //  printf("rendercallback is %p\n", renderCallback);
    //if (renderCallback) {
    //      glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
    //      glUseProgramObjectARB(0);
    //(*renderCallback)(1);
    //      if (program)
    //         glUseProgramObjectARB(program);
    //}
    int i;
    WriteTrace(TraceGlitch, TraceDebug, "swap_interval: %d", swap_interval);
    //printf("swap\n");
    if (render_to_texture) {
        WriteTrace(TraceGlitch, TraceWarning, "swap while render_to_texture\n");
        return;
    }

#ifdef ANDROID
    Android_JNI_SwapWindow();
#else
    SwapBuffers();
#endif
    for (i = 0; i < nb_fb; i++)
        fbs[i].buff_clear = 1;

    // VP debugging
#ifdef VPDEBUG
    dump_stop();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case 'd':
                printf("Dumping !\n");
                dump_start();
                break;
            case 'w': {
                static int wireframe;
                wireframe = !wireframe;
                glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
                break;
            }
            }
            break;
        }
    }
#endif
        }

// frame buffer

FX_ENTRY FxBool FX_CALL
grLfbLock(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t writeMode,
    GrOriginLocation_t origin, FxBool pixelPipeline,
    GrLfbInfo_t *info)
{
    WriteTrace(TraceGlitch, TraceDebug, "type: %d buffer: %d writeMode: %d origin: %d pixelPipeline: %d", type, buffer, writeMode, origin, pixelPipeline);
    if (type == GR_LFB_WRITE_ONLY)
    {
        WriteTrace(TraceGlitch, TraceWarning, "grLfbLock : write only");
    }
    else
    {
        unsigned char *buf;
        int i, j;

        switch (buffer)
        {
        case GR_BUFFER_FRONTBUFFER:
            //glReadBuffer(GL_FRONT);
            break;
        case GR_BUFFER_BACKBUFFER:
            //glReadBuffer(GL_BACK);
            break;
        default:
            WriteTrace(TraceGlitch, TraceWarning, "grLfbLock : unknown buffer : %x", buffer);
    }

        if (buffer != GR_BUFFER_AUXBUFFER)
        {
            if (writeMode == GR_LFBWRITEMODE_888) {
                //printf("LfbLock GR_LFBWRITEMODE_888\n");
                info->lfbPtr = frameBuffer;
                info->strideInBytes = g_width * 4;
                info->writeMode = GR_LFBWRITEMODE_888;
                info->origin = origin;
                glReadPixels(0, g_viewport_offset, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer);
            }
            else {
                buf = (unsigned char*)malloc(g_width*g_height * 4);

                info->lfbPtr = frameBuffer;
                info->strideInBytes = g_width * 2;
                info->writeMode = GR_LFBWRITEMODE_565;
                info->origin = origin;
                glReadPixels(0, g_viewport_offset, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, buf);

                for (j = 0; j < g_height; j++)
                {
                    for (i = 0; i < g_width; i++)
                    {
                        frameBuffer[(g_height - j - 1)*g_width + i] =
                            ((buf[j*g_width * 4 + i * 4 + 0] >> 3) << 11) |
                            ((buf[j*g_width * 4 + i * 4 + 1] >> 2) << 5) |
                            (buf[j*g_width * 4 + i * 4 + 2] >> 3);
                    }
                }
                free(buf);
            }
        }
        else
        {
            info->lfbPtr = depthBuffer;
            info->strideInBytes = g_width * 2;
            info->writeMode = GR_LFBWRITEMODE_ZA16;
            info->origin = origin;
            glReadPixels(0, g_viewport_offset, g_width, g_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, depthBuffer);
        }
}

    return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
grLfbUnlock(GrLock_t type, GrBuffer_t buffer)
{
    WriteTrace(TraceGlitch, TraceDebug, "type: %d, buffer: %d", type, buffer);
    if (type == GR_LFB_WRITE_ONLY)
    {
        WriteTrace(TraceGlitch, TraceWarning, "grLfbUnlock : write only");
    }
    return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
grLfbReadRegion(GrBuffer_t src_buffer,
    FxU32 src_x, FxU32 src_y,
    FxU32 src_width, FxU32 src_height,
    FxU32 dst_stride, void *dst_data)
{
    unsigned char *buf;
    unsigned int i, j;
    unsigned short *frameBuffer = (unsigned short*)dst_data;
    unsigned short *depthBuffer = (unsigned short*)dst_data;
    WriteTrace(TraceGlitch, TraceDebug, "src_buffer: %d src_x: %d src_y: %d src_width: %d src_height: %d dst_stride: %d", src_buffer, src_x, src_y, src_width, src_height, dst_stride);

    switch (src_buffer)
    {
    case GR_BUFFER_FRONTBUFFER:
        //glReadBuffer(GL_FRONT);
        break;
    case GR_BUFFER_BACKBUFFER:
        //glReadBuffer(GL_BACK);
        break;
        /*case GR_BUFFER_AUXBUFFER:
        glReadBuffer(current_buffer);
        break;*/
    default:
        WriteTrace(TraceGlitch, TraceWarning, "grReadRegion : unknown buffer : %x", src_buffer);
    }

    if (src_buffer != GR_BUFFER_AUXBUFFER)
    {
        buf = (unsigned char*)malloc(src_width*src_height * 4);

        glReadPixels(src_x, (g_viewport_offset)+g_height - src_y - src_height, src_width, src_height, GL_RGBA, GL_UNSIGNED_BYTE, buf);

        for (j = 0; j < src_height; j++)
        {
            for (i = 0; i < src_width; i++)
            {
                frameBuffer[j*(dst_stride / 2) + i] =
                    ((buf[(src_height - j - 1)*src_width * 4 + i * 4 + 0] >> 3) << 11) |
                    ((buf[(src_height - j - 1)*src_width * 4 + i * 4 + 1] >> 2) << 5) |
                    (buf[(src_height - j - 1)*src_width * 4 + i * 4 + 2] >> 3);
            }
        }
        free(buf);
    }
    else
    {
        buf = (unsigned char*)malloc(src_width*src_height * 2);

        glReadPixels(src_x, (g_viewport_offset)+g_height - src_y - src_height, src_width, src_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, depthBuffer);

        for (j = 0; j < src_height; j++)
        {
            for (i = 0; i < src_width; i++)
            {
                depthBuffer[j*(dst_stride / 2) + i] =
                    ((unsigned short*)buf)[(src_height - j - 1)*src_width * 4 + i * 4];
            }
        }
        free(buf);
    }

    return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
grLfbWriteRegion(GrBuffer_t dst_buffer,
    FxU32 dst_x, FxU32 dst_y,
    GrLfbSrcFmt_t src_format,
    FxU32 src_width, FxU32 src_height,
    FxBool pixelPipeline,
    FxI32 src_stride, void *src_data)
{
    unsigned char *buf;
    unsigned int i, j;
    unsigned short *frameBuffer = (unsigned short*)src_data;
    int texture_number;
    unsigned int tex_width = 1, tex_height = 1;
    WriteTrace(TraceGlitch, TraceDebug, "dst_buffer: %d dst_x: %d dst_y: %d src_format: %d src_width: %d src_height: %d pixelPipeline: %d src_stride: %d", dst_buffer, dst_x, dst_y, src_format, src_width, src_height, pixelPipeline, src_stride);

    //glPushAttrib(GL_ALL_ATTRIB_BITS);

    while (tex_width < src_width) tex_width <<= 1;
    while (tex_height < src_height) tex_height <<= 1;

    switch (dst_buffer)
    {
    case GR_BUFFER_BACKBUFFER:
        //glDrawBuffer(GL_BACK);
        break;
    case GR_BUFFER_AUXBUFFER:
        //glDrawBuffer(current_buffer);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "grLfbWriteRegion : unknown buffer : %x", dst_buffer);
    }

    if (dst_buffer != GR_BUFFER_AUXBUFFER)
    {
        buf = (unsigned char*)malloc(tex_width*tex_height * 4);

        texture_number = GL_TEXTURE0;
        glActiveTexture(texture_number);

        const unsigned int half_stride = src_stride / 2;
        switch (src_format)
        {
        case GR_LFB_SRC_FMT_1555:
            for (j = 0; j < src_height; j++)
            {
                for (i = 0; i < src_width; i++)
                {
                    const unsigned int col = frameBuffer[j*half_stride + i];
                    buf[j*tex_width * 4 + i * 4 + 0] = ((col >> 10) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 1] = ((col >> 5) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 2] = ((col >> 0) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 3] = (col >> 15) ? 0xFF : 0;
                }
            }
            break;
        case GR_LFBWRITEMODE_555:
            for (j = 0; j < src_height; j++)
            {
                for (i = 0; i < src_width; i++)
                {
                    const unsigned int col = frameBuffer[j*half_stride + i];
                    buf[j*tex_width * 4 + i * 4 + 0] = ((col >> 10) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 1] = ((col >> 5) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 2] = ((col >> 0) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 3] = 0xFF;
                }
            }
            break;
        case GR_LFBWRITEMODE_565:
            for (j = 0; j < src_height; j++)
            {
                for (i = 0; i < src_width; i++)
                {
                    const unsigned int col = frameBuffer[j*half_stride + i];
                    buf[j*tex_width * 4 + i * 4 + 0] = ((col >> 11) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 1] = ((col >> 5) & 0x3F) << 2;
                    buf[j*tex_width * 4 + i * 4 + 2] = ((col >> 0) & 0x1F) << 3;
                    buf[j*tex_width * 4 + i * 4 + 3] = 0xFF;
                }
            }
            break;
        default:
            WriteTrace(TraceGlitch, TraceWarning, "grLfbWriteRegion : unknown format : %d", src_format);
        }

#ifdef VPDEBUG
        if (dumping) {
            ilTexImage(tex_width, tex_height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, buf);
            char name[128];
            static int id;
            sprintf(name, "dump/writecolor%d.png", id++);
            ilSaveImage(name);
            //printf("dumped gdLfbWriteRegion %s\n", name);
        }
#endif

        glBindTexture(GL_TEXTURE_2D, default_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        free(buf);

        set_copy_shader();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        render_rectangle(texture_number,
            dst_x, dst_y,
            src_width, src_height,
            tex_width, tex_height, +1);
    }
    else
    {
        float *buf = (float*)malloc(src_width*(src_height + (g_viewport_offset)) * sizeof(float));

        if (src_format != GR_LFBWRITEMODE_ZA16)
            WriteTrace(TraceGlitch, TraceWarning, "unknown depth buffer write format:%x", src_format);

        if (dst_x || dst_y)
            WriteTrace(TraceGlitch, TraceWarning, "dst_x:%d, dst_y:%d\n", dst_x, dst_y);

        for (j = 0; j < src_height; j++)
        {
            for (i = 0; i < src_width; i++)
            {
                buf[(j + (g_viewport_offset))*src_width + i] =
                    (frameBuffer[(src_height - j - 1)*(src_stride / 2) + i] / (65536.0f*(2.0f / zscale))) + 1 - zscale / 2.0f;
            }
    }

#ifdef VPDEBUG
        if (dumping) {
            unsigned char * buf2 = (unsigned char *)malloc(src_width*(src_height + (g_viewport_offset)));
            for (i = 0; i < src_width*src_height; i++)
                buf2[i] = buf[i] * 255.0f;
            ilTexImage(src_width, src_height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, buf2);
            char name[128];
            static int id;
            sprintf(name, "dump/writedepth%d.png", id++);
            ilSaveImage(name);
            //printf("dumped gdLfbWriteRegion %s\n", name);
            free(buf2);
        }
#endif

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        //glDrawBuffer(GL_BACK);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDepthMask(1);
        //glDrawPixels(src_width, src_height+(g_viewport_offset), GL_DEPTH_COMPONENT, GL_FLOAT, buf);

        free(buf);
}
    //glDrawBuffer(current_buffer);
    //glPopAttrib();
    return FXTRUE;
}

/* wrapper-specific glide extensions */

FX_ENTRY char ** FX_CALL
grQueryResolutionsExt(int32_t * Size)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    return 0;
}

FX_ENTRY FxBool FX_CALL grKeyPressedExt(FxU32 key)
{
    return 0;
}

void grConfigWrapperExt(FxI32 vram, FxBool fbo, FxBool aniso)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    config.vram_size = vram;
    config.fbo = fbo;
    config.anisofilter = aniso;
}

// unused by glide64

FX_ENTRY FxI32 FX_CALL
grQueryResolutions(const GrResolution *resTemplate, GrResolution *output)
{
    int res_inf = 0;
    int res_sup = 0xf;
    int i;
    int n = 0;
    WriteTrace(TraceGlitch, TraceDebug, "-");
    WriteTrace(TraceGlitch, TraceWarning, "grQueryResolutions");
    if ((unsigned int)resTemplate->resolution != GR_QUERY_ANY)
    {
        res_inf = res_sup = resTemplate->resolution;
    }
    if ((unsigned int)resTemplate->refresh == GR_QUERY_ANY) WriteTrace(TraceGlitch, TraceWarning, "querying any refresh rate");
    if ((unsigned int)resTemplate->numAuxBuffers == GR_QUERY_ANY) WriteTrace(TraceGlitch, TraceWarning, "querying any numAuxBuffers");
    if ((unsigned int)resTemplate->numColorBuffers == GR_QUERY_ANY) WriteTrace(TraceGlitch, TraceWarning, "querying any numColorBuffers");

    if (output == NULL) return res_sup - res_inf + 1;
    for (i = res_inf; i <= res_sup; i++)
    {
        output[n].resolution = i;
        output[n].refresh = resTemplate->refresh;
        output[n].numAuxBuffers = resTemplate->numAuxBuffers;
        output[n].numColorBuffers = resTemplate->numColorBuffers;
        n++;
    }
    return res_sup - res_inf + 1;
}

FX_ENTRY FxBool FX_CALL
grReset(FxU32 what)
{
    WriteTrace(TraceGlitch, TraceWarning, "grReset");
    return 1;
}

FX_ENTRY void FX_CALL
grEnable(GrEnableMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    if (mode == GR_TEXTURE_UMA_EXT)
        UMAmode = 1;
}

FX_ENTRY void FX_CALL
grDisable(GrEnableMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    if (mode == GR_TEXTURE_UMA_EXT)
        UMAmode = 0;
}

FX_ENTRY void FX_CALL
grDisableAllEffects(void)
{
    WriteTrace(TraceGlitch, TraceWarning, "grDisableAllEffects");
}

FX_ENTRY void FX_CALL
grErrorSetCallback(GrErrorCallbackFnc_t fnc)
{
    WriteTrace(TraceGlitch, TraceWarning, "grErrorSetCallback");
}

FX_ENTRY void FX_CALL
grFinish(void)
{
    WriteTrace(TraceGlitch, TraceWarning, "grFinish");
}

FX_ENTRY void FX_CALL
grFlush(void)
{
    WriteTrace(TraceGlitch, TraceWarning, "grFlush");
}

FX_ENTRY void FX_CALL
grTexMultibase(GrChipID_t tmu,
    FxBool     enable)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexMultibase");
}

FX_ENTRY void FX_CALL
grTexMipMapMode(GrChipID_t     tmu,
    GrMipMapMode_t mode,
    FxBool         lodBlend)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexMipMapMode");
}

FX_ENTRY void FX_CALL
grTexDownloadTablePartial(GrTexTable_t type,
    void         *data,
    int          start,
    int          end)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexDownloadTablePartial");
}

FX_ENTRY void FX_CALL
grTexDownloadTable(GrTexTable_t type,
    void         *data)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexDownloadTable");
}

FX_ENTRY FxBool FX_CALL
grTexDownloadMipMapLevelPartial(GrChipID_t        tmu,
    FxU32             startAddress,
    GrLOD_t           thisLod,
    GrLOD_t           largeLod,
    GrAspectRatio_t   aspectRatio,
    GrTextureFormat_t format,
    FxU32             evenOdd,
    void              *data,
    int               start,
    int               end)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexDownloadMipMapLevelPartial");
    return 1;
}

FX_ENTRY void FX_CALL
grTexDownloadMipMapLevel(GrChipID_t        tmu,
    FxU32             startAddress,
    GrLOD_t           thisLod,
    GrLOD_t           largeLod,
    GrAspectRatio_t   aspectRatio,
    GrTextureFormat_t format,
    FxU32             evenOdd,
    void              *data)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexDownloadMipMapLevel");
}

FX_ENTRY void FX_CALL
grTexNCCTable(GrNCCTable_t table)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexNCCTable");
}

FX_ENTRY void FX_CALL
grViewport(FxI32 x, FxI32 y, FxI32 width, FxI32 height)
{
    WriteTrace(TraceGlitch, TraceWarning, "grViewport");
}

FX_ENTRY void FX_CALL
grDepthRange(FxFloat n, FxFloat f)
{
    WriteTrace(TraceGlitch, TraceWarning, "grDepthRange");
}

FX_ENTRY void FX_CALL
grSplash(float x, float y, float width, float height, FxU32 frame)
{
    WriteTrace(TraceGlitch, TraceWarning, "grSplash");
}

FX_ENTRY FxBool FX_CALL
grSelectContext(GrContext_t context)
{
    WriteTrace(TraceGlitch, TraceWarning, "grSelectContext");
    return 1;
}

FX_ENTRY void FX_CALL
grAADrawTriangle(
    const void *a, const void *b, const void *c,
    FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias
)
{
    WriteTrace(TraceGlitch, TraceWarning, "grAADrawTriangle");
}

FX_ENTRY void FX_CALL
grAlphaControlsITRGBLighting(FxBool enable)
{
    WriteTrace(TraceGlitch, TraceWarning, "grAlphaControlsITRGBLighting");
}

FX_ENTRY void FX_CALL
grGlideSetVertexLayout(const void *layout)
{
    WriteTrace(TraceGlitch, TraceWarning, "grGlideSetVertexLayout");
}

FX_ENTRY void FX_CALL
grGlideGetVertexLayout(void *layout)
{
    WriteTrace(TraceGlitch, TraceWarning, "grGlideGetVertexLayout");
}

FX_ENTRY void FX_CALL
grGlideSetState(const void *state)
{
    WriteTrace(TraceGlitch, TraceWarning, "grGlideSetState");
}

FX_ENTRY void FX_CALL
grGlideGetState(void *state)
{
    WriteTrace(TraceGlitch, TraceWarning, "grGlideGetState");
}

FX_ENTRY void FX_CALL
grLfbWriteColorFormat(GrColorFormat_t colorFormat)
{
    WriteTrace(TraceGlitch, TraceWarning, "grLfbWriteColorFormat");
}

FX_ENTRY void FX_CALL
grLfbWriteColorSwizzle(FxBool swizzleBytes, FxBool swapWords)
{
    WriteTrace(TraceGlitch, TraceWarning, "grLfbWriteColorSwizzle");
}

FX_ENTRY void FX_CALL
grLfbConstantDepth(FxU32 depth)
{
    WriteTrace(TraceGlitch, TraceWarning, "grLfbConstantDepth");
}

FX_ENTRY void FX_CALL
grLfbConstantAlpha(GrAlpha_t alpha)
{
    WriteTrace(TraceGlitch, TraceWarning, "grLfbConstantAlpha");
}

FX_ENTRY void FX_CALL
grTexMultibaseAddress(GrChipID_t       tmu,
    GrTexBaseRange_t range,
    FxU32            startAddress,
    FxU32            evenOdd,
    GrTexInfo        *info)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexMultibaseAddress");
}

FX_ENTRY void FX_CALL
grLoadGammaTable(FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue)
{
}

FX_ENTRY void FX_CALL
grGetGammaTableExt(FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue)
{
    return;
    //TODO?
    /*
    LOG("grGetGammaTableExt()\r\n");
    FxU16 aGammaRamp[3][256];
    #ifdef _WIN32
    HDC hdc = GetDC(NULL);
    if (hdc == NULL)
    return;
    if (GetDeviceGammaRamp(hdc, aGammaRamp) == TRUE)
    {
    ReleaseDC(NULL, hdc);
    #else
    if (SDL_GetGammaRamp(aGammaRamp[0], aGammaRamp[1], aGammaRamp[2]) != -1)
    {
    #endif
    for (int i = 0; i < 256; i++)
    {
    red[i] = aGammaRamp[0][i] >> 8;
    green[i] = aGammaRamp[1][i] >> 8;
    blue[i] = aGammaRamp[2][i] >> 8;
    }
    }
    */
}

FX_ENTRY void FX_CALL
guGammaCorrectionRGB(FxFloat gammaR, FxFloat gammaG, FxFloat gammaB)
{
    //TODO?
    /*
    LOG("guGammaCorrectionRGB()\r\n");
    if (!fullscreen)
    return;
    FxU16 aGammaRamp[3][256];
    for (int i = 0; i < 256; i++)
    {
    aGammaRamp[0][i] = (((FxU16)((pow(i/255.0F, 1.0F/gammaR)) * 255.0F + 0.5F)) << 8) & 0xFFFF;
    aGammaRamp[1][i] = (((FxU16)((pow(i/255.0F, 1.0F/gammaG)) * 255.0F + 0.5F)) << 8) & 0xFFFF;
    aGammaRamp[2][i] = (((FxU16)((pow(i/255.0F, 1.0F/gammaB)) * 255.0F + 0.5F)) << 8) & 0xFFFF;
    }
    CorrectGamma(aGammaRamp);*/
}

FX_ENTRY void FX_CALL
grDitherMode(GrDitherMode_t mode)
{
    WriteTrace(TraceGlitch, TraceWarning, "grDitherMode");
}

void grChromaRangeExt(GrColor_t color0, GrColor_t color1, FxU32 mode)
{
    WriteTrace(TraceGlitch, TraceWarning, "grChromaRangeExt");
}

void grChromaRangeModeExt(GrChromakeyMode_t mode)
{
    WriteTrace(TraceGlitch, TraceWarning, "grChromaRangeModeExt");
}

void grTexChromaRangeExt(GrChipID_t tmu, GrColor_t color0, GrColor_t color1, GrTexChromakeyMode_t mode)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexChromaRangeExt");
}

void grTexChromaModeExt(GrChipID_t tmu, GrChromakeyMode_t mode)
{
    WriteTrace(TraceGlitch, TraceWarning, "grTexChromaRangeModeExt");
}

// VP debug
#ifdef VPDEBUG
int dumping = 0;
static int tl_i;
static int tl[10240];

void dump_start()
{
    static int init;
    if (!init) {
        init = 1;
        ilInit();
        ilEnable(IL_FILE_OVERWRITE);
    }
    dumping = 1;
    tl_i = 0;
}

void dump_stop()
{
    if (!dumping) return;

    int i, j;
    for (i = 0; i < nb_fb; i++) {
        dump_tex(fbs[i].texid);
    }
    dump_tex(default_texture);
    dump_tex(depth_texture);

    dumping = 0;

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer);
    ilTexImage(width, height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, frameBuffer);
    ilSaveImage("dump/framecolor.png");
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, depthBuffer);
    //   FILE * fp = fopen("glide_depth1.bin", "rb");
    //   fread(depthBuffer, 2, width*height, fp);
    //   fclose(fp);
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            //uint16_t d = ( (uint16_t *)depthBuffer )[i+(height-1-j)*width]/2 + 0x8000;
            uint16_t d = ((uint16_t *)depthBuffer)[i + j*width];
            uint32_t c = ((uint32_t *)frameBuffer)[i + j*width];
            ((unsigned char *)frameBuffer)[(i + j*width) * 3] = d & 0xff;
            ((unsigned char *)frameBuffer)[(i + j*width) * 3 + 1] = d >> 8;
            ((unsigned char *)frameBuffer)[(i + j*width) * 3 + 2] = c & 0xff;
        }
    }
    ilTexImage(width, height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, frameBuffer);
    ilSaveImage("dump/framedepth.png");

    for (i = 0; i < tl_i; i++) {
        glBindTexture(GL_TEXTURE_2D, tl[i]);
        GLint w, h, fmt;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
        fprintf(stderr, "Texture %d %dx%d fmt %x\n", tl[i], (int)w, (int)h, (int)fmt);

        uint32_t * pixels = (uint32_t *)malloc(w*h * 4);
        // 0x1902 is another constant meaning GL_DEPTH_COMPONENT
        // (but isn't defined in gl's headers !!)
        if (fmt != GL_DEPTH_COMPONENT && fmt != 0x1902) {
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            ilTexImage(w, h, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, pixels);
        }
        else {
            glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, pixels);
            int i;
            for (i = 0; i < w*h; i++)
                ((unsigned char *)frameBuffer)[i] = ((unsigned short *)pixels)[i] / 256;
            ilTexImage(w, h, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, frameBuffer);
        }
        char name[128];
        //     sprintf(name, "mkdir -p dump ; rm -f dump/tex%04d.png", i);
        //     system(name);
        sprintf(name, "dump/tex%04d.png", i);
        fprintf(stderr, "Writing '%s'\n", name);
        ilSaveImage(name);

        //     SDL_FreeSurface(surf);
        free(pixels);
    }
    glBindTexture(GL_TEXTURE_2D, default_texture);
}

void dump_tex(int id)
{
    if (!dumping) return;

    int n;
    // yes, it's inefficient
    for (n = 0; n < tl_i; n++)
        if (tl[n] == id)
            return;

    tl[tl_i++] = id;

    int i = tl_i - 1;
}

#endif

void CHECK_FRAMEBUFFER_STATUS(void)
{
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    WriteTrace(TraceGlitch, TraceDebug, "status: %X", status);
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        /*WriteTrace(TraceGlitch, TraceWarning, "framebuffer complete!\n");*/
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT\n");
        /* you gotta choose different formats */
        /*assert(0);*/
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer INCOMPLETE_ATTACHMENT\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer FRAMEBUFFER_MISSING_ATTACHMENT\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer FRAMEBUFFER_DIMENSIONS\n");
        break;
    default:
        break;
        /* programming error; will fail on all hardware */
        /*assert(0);*/
}
}