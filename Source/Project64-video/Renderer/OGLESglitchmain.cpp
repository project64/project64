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
#include "glitchmain.h"
#include <Project64-video/trace.h>
#include <Project64-video/Settings.h>

#define OPENGL_CHECK_ERRORS { const GLenum errcode = glGetError(); if (errcode != GL_NO_ERROR) LOG("OpenGL Error code %i in '%s' line %i\n", errcode, __FILE__, __LINE__-1); }

extern void(*renderCallback)(int);

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

int nbAuxBuffers, current_buffer;
int g_width, widtho, heighto, g_height;
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
    uint32_t start, end;
    int fmt;
};
#define NB_TEXBUFS 128 // MUST be a power of two
static texbuf_t texbufs[NB_TEXBUFS];
static int texbuf_i;

unsigned short frameBuffer[2048 * 2048 * 2]; // Support 2048x2048 screen resolution at 32 bits (RGBA) per pixel
unsigned short depthBuffer[2048 * 2048];   // Support 2048x2048 screen resolution at 16 bits (depth) per pixel

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

void gfxClipWindow(uint32_t minx, uint32_t miny, uint32_t maxx, uint32_t maxy)
{
    WriteTrace(TraceGlitch, TraceDebug, "minx = %d, miny: %d maxx: %d maxy: %d", minx, miny, maxx, maxy);

    if (use_fbo && render_to_texture)
    {
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
        uint32_t tmp = maxy; maxy = miny; miny = tmp;
        if (maxx > (uint32_t)g_width) maxx = g_width;
        if (maxy > (uint32_t)g_height) maxy = g_height;
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

void gfxColorMask(bool rgb, bool a)
{
    WriteTrace(TraceGlitch, TraceDebug, "rgb = %d, a: %d", rgb, a);
    glColorMask(rgb, rgb, rgb, a);
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

#ifdef _WIN32
# include <fcntl.h>
# ifndef ATTACH_PARENT_PROCESS
#  define ATTACH_PARENT_PROCESS ((uint32_t)-1)
# endif
#endif

bool gfxSstWinOpen(gfxColorFormat_t color_format, gfxOriginLocation_t origin_location, int nColBuffers, int nAuxBuffers)
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
    if (origin_location != GFX_ORIGIN_UPPER_LEFT) WriteTrace(TraceGlitch, TraceWarning, "origin must be in upper left corner");
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
    nbAuxBuffers = 4;
    //glGetIntegerv(GL_AUX_BUFFERS, &nbAuxBuffers);
    if (nbAuxBuffers > 0)
        printf("Congratulations, you have %d auxilliary buffers, we'll use them wisely !\n", nbAuxBuffers);

    blend_func_separate_support = 1;
    packed_pixels_support = 0;

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
    use_fbo = g_settings->wrpFBO() && (glFramebufferRenderbufferEXT != NULL);
#else
    use_fbo = g_settings->wrpFBO();
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

    return 1;
}

bool gfxSstWinClose()
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    int i, clear_texbuff = use_fbo;

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

#ifdef _WIN32
    if (hGLRC)
    {
        wglMakeCurrent(hDC, NULL);
        wglDeleteContext(hGLRC);
        hGLRC = NULL;
    }
    ExitFullScreen();
#endif
    return true;
}

void gfxTextureBufferExt(gfxChipID_t tmu, uint32_t startAddress, gfxLOD_t lodmin, gfxLOD_t lodmax, gfxAspectRatio_t aspect, gfxTextureFormat_t fmt, uint32_t evenOdd)
{
    int i;
    static int fbs_init = 0;

    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d startAddress: %d lodmin: %d lodmax: %d aspect: %d fmt: %d evenOdd: %d", tmu, startAddress, lodmin, lodmax, aspect, fmt, evenOdd);
    if (lodmin != lodmax) WriteTrace(TraceGlitch, TraceWarning, "gfxTextureBufferExt : loading more than one LOD");
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

        int rtmu = startAddress < gfxTexMinAddress(GFX_TMU1) ? 0 : 1;
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

int CheckTextureBufferFormat(gfxChipID_t tmu, uint32_t startAddress, gfxTexInfo *info)
{
    int found, i;
    if (!use_fbo) {
        for (found = i = 0; i < 2; i++)
            if ((uint32_t)tmu_usage[i].min <= startAddress && (uint32_t)tmu_usage[i].max > startAddress) {
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

    if (info->format == GFX_TEXFMT_ALPHA_INTENSITY_88) {
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

void gfxRenderBuffer(gfxBuffer_t buffer)
{
#ifdef _WIN32
    static HANDLE region = NULL;
    int realWidth = pBufferWidth, realHeight = pBufferHeight;
#endif // _WIN32
    WriteTrace(TraceGlitch, TraceDebug, "buffer: %d", buffer);
    //printf("gfxRenderBuffer(%d)\n", buffer);

    switch (buffer)
    {
    case GFX_BUFFER_BACKBUFFER:
        if (render_to_texture)
        {
            updateTexture();

            // VP z fix
            //glMatrixMode(GL_MODELVIEW);
            //glLoadIdentity();
            //glTranslatef(0, 0, 1-zscale);
            //glScalef(1, 1, zscale);
            inverted_culling = 0;
            gfxCullMode(culling_mode);

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
                gfxCullMode(culling_mode);
            }
        }
        render_to_texture = 1;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxRenderBuffer : unknown buffer : %x", buffer);
    }
}

void gfxAuxBufferExt(gfxBuffer_t buffer)
{
    WriteTrace(TraceGlitch, TraceDebug, "buffer: %d", buffer);
    //WriteTrace(TraceGlitch, TraceWarning, "gfxAuxBufferExt");

    if (buffer == GFX_BUFFER_AUXBUFFER) {
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
        gfxTexFilterMode(GFX_TMU1, GFX_TEXTUREFILTER_POINT_SAMPLED, GFX_TEXTUREFILTER_POINT_SAMPLED);
    }
    else {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        need_to_compile = 1;
    }
}

void gfxBufferClear(gfxColor_t color, gfxAlpha_t alpha, uint32_t depth)
{
    WriteTrace(TraceGlitch, TraceDebug, "color: %d alpha: %d depth: %d", color, alpha, depth);
    vbo_draw();
    switch (lfb_color_fmt)
    {
    case GFX_COLORFORMAT_ARGB:
        glClearColor(((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f,
            (color & 0xFF) / 255.0f,
            alpha / 255.0f);
        break;
    case GFX_COLORFORMAT_RGBA:
        glClearColor(((color >> 24) & 0xFF) / 255.0f,
            ((color >> 16) & 0xFF) / 255.0f,
            (color & 0xFF) / 255.0f,
            alpha / 255.0f);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxBufferClear: unknown color format : %x", lfb_color_fmt);
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

void gfxBufferSwap(uint32_t swap_interval)
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
}

// frame buffer
bool gfxLfbLock(gfxLock_t type, gfxBuffer_t buffer, gfxLfbWriteMode_t writeMode, gfxOriginLocation_t origin, bool pixelPipeline, gfxLfbInfo_t *info)
{
    WriteTrace(TraceGlitch, TraceDebug, "type: %d buffer: %d writeMode: %d origin: %d pixelPipeline: %d", type, buffer, writeMode, origin, pixelPipeline);
    if (type == GFX_LFB_WRITE_ONLY)
    {
        WriteTrace(TraceGlitch, TraceWarning, "gfxLfbLock : write only");
    }
    else
    {
        unsigned char *buf;
        int i, j;

        switch (buffer)
        {
        case GFX_BUFFER_FRONTBUFFER:
            //glReadBuffer(GL_FRONT);
            break;
        case GFX_BUFFER_BACKBUFFER:
            //glReadBuffer(GL_BACK);
            break;
        default:
            WriteTrace(TraceGlitch, TraceWarning, "gfxLfbLock : unknown buffer : %x", buffer);
        }

        if (buffer != GFX_BUFFER_AUXBUFFER)
        {
            if (writeMode == GFX_LFBWRITEMODE_888) {
                //printf("LfbLock GFX_LFBWRITEMODE_888\n");
                info->lfbPtr = frameBuffer;
                info->strideInBytes = g_width * 4;
                info->writeMode = GFX_LFBWRITEMODE_888;
                info->origin = origin;
                glReadPixels(0, g_viewport_offset, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer);
            }
            else {
                buf = (unsigned char*)malloc(g_width*g_height * 4);

                info->lfbPtr = frameBuffer;
                info->strideInBytes = g_width * 2;
                info->writeMode = GFX_LFBWRITEMODE_565;
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
            info->writeMode = GFX_LFBWRITEMODE_ZA16;
            info->origin = origin;
            glReadPixels(0, g_viewport_offset, g_width, g_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, depthBuffer);
        }
    }

    return true;
}

bool gfxLfbUnlock(gfxLock_t type, gfxBuffer_t buffer)
{
    WriteTrace(TraceGlitch, TraceDebug, "type: %d, buffer: %d", type, buffer);
    if (type == GFX_LFB_WRITE_ONLY)
    {
        WriteTrace(TraceGlitch, TraceWarning, "gfxLfbUnlock : write only");
    }
    return true;
}

bool gfxLfbReadRegion(gfxBuffer_t src_buffer, uint32_t src_x, uint32_t src_y, uint32_t src_width, uint32_t src_height, uint32_t dst_stride, void *dst_data)
{
    unsigned char *buf;
    unsigned int i, j;
    unsigned short *frameBuffer = (unsigned short*)dst_data;
    unsigned short *depthBuffer = (unsigned short*)dst_data;
    WriteTrace(TraceGlitch, TraceDebug, "src_buffer: %d src_x: %d src_y: %d src_width: %d src_height: %d dst_stride: %d", src_buffer, src_x, src_y, src_width, src_height, dst_stride);

    switch (src_buffer)
    {
    case GFX_BUFFER_FRONTBUFFER:
        //glReadBuffer(GL_FRONT);
        break;
    case GFX_BUFFER_BACKBUFFER:
        //glReadBuffer(GL_BACK);
        break;
        /*case GFX_BUFFER_AUXBUFFER:
        glReadBuffer(current_buffer);
        break;*/
    default:
        WriteTrace(TraceGlitch, TraceWarning, "grReadRegion : unknown buffer : %x", src_buffer);
    }

    if (src_buffer != GFX_BUFFER_AUXBUFFER)
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

    return true;
}

bool gfxLfbWriteRegion(gfxBuffer_t dst_buffer, uint32_t dst_x, uint32_t dst_y, gfxLfbSrcFmt_t src_format, uint32_t src_width, uint32_t src_height, bool pixelPipeline, int32_t src_stride, void *src_data)
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
    case GFX_BUFFER_BACKBUFFER:
        //glDrawBuffer(GL_BACK);
        break;
    case GFX_BUFFER_AUXBUFFER:
        //glDrawBuffer(current_buffer);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxLfbWriteRegion : unknown buffer : %x", dst_buffer);
    }

    if (dst_buffer != GFX_BUFFER_AUXBUFFER)
    {
        buf = (unsigned char*)malloc(tex_width*tex_height * 4);

        texture_number = GL_TEXTURE0;
        glActiveTexture(texture_number);

        const unsigned int half_stride = src_stride / 2;
        switch (src_format)
        {
        case GFX_LFB_SRC_FMT_1555:
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
        case GFX_LFBWRITEMODE_555:
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
        case GFX_LFBWRITEMODE_565:
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
            WriteTrace(TraceGlitch, TraceWarning, "gfxLfbWriteRegion : unknown format : %d", src_format);
        }

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

        if (src_format != GFX_LFBWRITEMODE_ZA16)
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
    return true;
}

/* wrapper-specific glide extensions */
void gfxLoadGammaTable(uint32_t nentries, uint32_t *red, uint32_t *green, uint32_t *blue)
{
}

void gfxGetGammaTableExt(uint32_t nentries, uint32_t *red, uint32_t *green, uint32_t *blue)
{
    return;
}

void gfxGammaCorrectionRGB(float gammaR, float gammaG, float gammaB)
{
}

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