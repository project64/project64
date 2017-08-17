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
#include <Project64-video/Gfx_1.3.h>

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
#include <Common/Util.h>

/*
 * `GetSystemSetting` and `FindSystemSettingId` from Project64 debugger
 * used only in g_Notify->DisplayError when OpenGL extension loading fails on WGL
 */
#include <Settings/Settings.h>

int screen_width, screen_height;

static inline void opt_glCopyTexImage2D(GLenum target,
    GLint level,
    GLint internalFormat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border)

{
    int w, h, fmt;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
    //printf("copyteximage %dx%d fmt %x oldfmt %x\n", width, height, internalFormat, fmt);
    if (w == width && h == height && fmt == internalFormat) {
        if (x + width >= screen_width) {
            width = screen_width - x;
            //printf("resizing w --> %d\n", width);
        }
        if (y + height >= screen_height + g_viewport_offset) {
            height = screen_height + g_viewport_offset - y;
            //printf("resizing h --> %d\n", height);
        }
        glCopyTexSubImage2D(target, level, 0, 0, x, y, width, height);
    }
    else {
        //printf("copyteximage %dx%d fmt %x old %dx%d oldfmt %x\n", width, height, internalFormat, w, h, fmt);
        //       glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, 0);
        //       glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
        //       printf("--> %dx%d newfmt %x\n", width, height, fmt);
        glCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
    }
    grDisplayGLError("opt_glCopyTexImage2D");
}
#define glCopyTexImage2D opt_glCopyTexImage2D

#ifdef _WIN32
/*
 * Some post-1.1 OpenGL functions can fail to be loaded through GL extensions
 * when running primitive OpenGL contexts on Microsoft Windows, specifically.
 *
 * As of the Project64 Glide64 version, Glitch64 now assigns these GL
 * functions to dummy functions to prevent access violations, while also
 * displaying error information showing the missing OpenGL support.
 */

PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLBLENDFUNCSEPARATEEXTPROC glBlendFuncSeparateEXT;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLFOGCOORDFPROC glFogCoordfEXT;
void APIENTRY dummy_glActiveTexture(GLenum/*texture*/)
{ /* GLX render opcode 197, req. OpenGL 1.3 (1.2 w/ ARB_multitexture) */
    g_Notify->DisplayError("glActiveTexture");
}
void APIENTRY dummy_glMultiTexCoord2f(GLenum/*target*/, GLfloat/*s*/, GLfloat/*t*/)
{ /* GLX render opcode 203, req. OpenGL 1.3 (1.2 w/ ARB_multitexture) */
    g_Notify->DisplayError("glMultiTexCoord2f");
}
void APIENTRY dummy_glFogCoordf(GLfloat/*coord*/)
{ /* GLX render opcode 4124, req. OpenGL 1.4 (1.1 w/ EXT_fog_coord) */
    g_Notify->DisplayError("glFogCoordf");
}
void APIENTRY dummy_glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum)
{ /* GLX render opcode 4134, req. OpenGL 1.0 w/ EXT_blend_func_separate */
    g_Notify->DisplayError("glBlendFuncSeparate");
}

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
const char * APIENTRY dummy_wglGetExtensionsString(HDC)
{
    g_Notify->DisplayError("wglGetExtensionsString");
    return NULL;
}

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
void APIENTRY dummy_glGenRenderbuffers(GLsizei/*n*/, GLuint* /*renderbuffers*/)
{ /* GLX vendor opcode 1423, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glGenRenderbuffers");
}
void APIENTRY dummy_glGenFramebuffers(GLsizei/*n*/, GLuint* /*framebuffers*/)
{ /* GLX vendor opcode 1426, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glGenFramebuffers");
}
GLenum APIENTRY dummy_glCheckFramebufferStatus(GLenum/*target*/)
{ /* GLX vendor opcode 1427, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glCheckFramebufferStatus");
    return 0x00008CDD; /* GL_FRAMEBUFFER_UNSUPPORTED */
}
void APIENTRY dummy_glBindRenderbuffer(GLenum/*target*/, GLuint/*renderbuffer*/)
{ /* GLX render opcode 4316, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glBindRenderbuffer");
}
void APIENTRY dummy_glDeleteRenderbuffers(GLsizei/*n*/, const GLuint* /*renderbuffers*/)
{ /* GLX render opcode 4317, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glDeleteRenderbuffers");
}
void APIENTRY dummy_glRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei)
{ /* GLX render opcode 4318, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glRenderbufferStorage");
}
void APIENTRY dummy_glBindFramebuffer(GLenum/*target*/, GLuint/*framebuffer*/)
{ /* GLX render opcode 4319, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glBindFramebuffer");
}
void APIENTRY dummy_glDeleteFramebuffers(GLsizei/*n*/, const GLuint* /*framebuffers*/)
{ /* GLX render opcode 4320, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glDeleteFramebuffers");
}
void APIENTRY dummy_glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint)
{ /* GLX render opcode 4322, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glFramebufferTexture2D");
}
void APIENTRY dummy_glFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint)
{ /* GLX render opcode 4324, req. OpenGL 1.2 w/ EXT_framebuffer_object */
    g_Notify->DisplayError("glFramebufferRenderbuffer");
}

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
void APIENTRY dummy_glSecondaryColor3f(GLfloat/*red*/, GLfloat/*green*/, GLfloat/*blue*/)
{ /* GLX render opcode 4129, req. OpenGL 1.4 (1.1 w/ EXT_secondary_color) */
    g_Notify->DisplayError("glSecondaryColor3f");
}
GLuint APIENTRY dummy_glCreateShader(GLenum/*type*/)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glCreateShader");
    return ((GLuint)(NULL));
}
void APIENTRY dummy_glShaderSource(GLuint, GLsizei, const GLchar **, GLint *)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glShaderSource");
}
void APIENTRY dummy_glCompileShader(GLuint/*shader*/)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glCompileShader");
}
GLuint APIENTRY dummy_glCreateProgram(void)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glCreateProgram");
    return ((GLuint)(NULL));
}
void APIENTRY dummy_glAttachObject(GLhandleARB, GLhandleARB)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glAttachObject");
}
void APIENTRY dummy_glLinkProgram(GLuint/*program*/)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glLinkProgram");
}
void APIENTRY dummy_glUseProgram(GLuint/*program*/)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glUseProgram");
}
GLint APIENTRY dummy_glGetUniformLocation(GLuint/*program*/, GLchar* /*name*/)
{ /* GLX single opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glGetUniformLocation");
    return -1;
}
void APIENTRY dummy_glUniform1i(GLint/*location*/, GLint/*v0*/)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glUniform1i");
}
void APIENTRY dummy_glUniform4i(GLint/*location*/, GLint, GLint, GLint, GLint)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glUniform4i");
}
void APIENTRY dummy_glUniform1f(GLint/*location*/, GLfloat/*v0*/)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glUniform1f");
}
void APIENTRY dummy_glUniform4f(GLint/*location*/, GLfloat, GLfloat, GLfloat, GLfloat)
{ /* GLX render opcode ?, req. OpenGL 2.0 (1.2 w/ ARB_shader_objects) */
    g_Notify->DisplayError("glUniform4f");
}
void APIENTRY dummy_glDeleteObject(GLhandleARB/*obj*/)
{ /* GLX render opcode ?, req. OpenGL 1.2 w/ ARB_shader_objects */
    g_Notify->DisplayError("glDeleteObject");
}
void APIENTRY dummy_glGetInfoLog(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)
{ /* GLX single opcode ?, req. OpenGL 1.2 w/ ARB_shader_objects */
    g_Notify->DisplayError("glGetInfoLog");
}
void APIENTRY dummy_glGetObjectParameteriv(GLhandleARB, GLenum, GLint *)
{ /* GLX single opcode ?, req. OpenGL 1.2 w/ ARB_shader_objects */
    g_Notify->DisplayError("glGetObjectParameteriv");
}

// FXT1,DXT1,DXT5 support - Hiroshi Morii <koolsmoky(at)users.sourceforge.net>
// NOTE: Glide64 + GlideHQ use the following formats
// GL_COMPRESSED_RGB_S3TC_DXT1_EXT
// GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
// GL_COMPRESSED_RGB_FXT1_3DFX
// GL_COMPRESSED_RGBA_FXT1_3DFX
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2DARB;
void APIENTRY dummy_glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)
{ /* GLX render opcode 215, req. OpenGL 1.3 (1.2 w/ ARB_texture_compression) */
    g_Notify->DisplayError("glCompressedTexImage2D");
}
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

static int savedWidtho, savedHeighto;
static int savedWidth, savedHeight;
unsigned int pBufferAddress;
static int pBufferFmt;
static int pBufferWidth, pBufferHeight;
static fb fbs[100];
static int nb_fb = 0;
static unsigned int curBufferAddr = 0;

struct TMU_USAGE { unsigned long min, max; } tmu_usage[2] = {
    { 0x0FFFFFFFul, 0x00000000ul },
    { 0x0FFFFFFFul, 0x00000000ul },
};

struct texbuf_t {
    uint32_t start, end;
    int fmt;
};
#define NB_TEXBUFS 128 // MUST be a power of two
static texbuf_t texbufs[NB_TEXBUFS];
static int texbuf_i;

unsigned short frameBuffer[2048 * 2048];
unsigned short depthBuffer[2048 * 2048];

void gfxClipWindow(uint32_t minx, uint32_t miny, uint32_t maxx, uint32_t maxy)
{
    WriteTrace(TraceGlitch, TraceDebug, "minx = %d, miny: %d maxy: %d", minx, miny, maxy);

    if (use_fbo && render_to_texture)
    {
        if (int(minx) < 0) minx = 0;
        if (int(miny) < 0) miny = 0;
        if (maxx < minx) maxx = minx;
        if (maxy < miny) maxy = miny;
        glScissor(minx, miny, maxx - minx, maxy - miny);
        glEnable(GL_SCISSOR_TEST);
        grDisplayGLError("gfxClipWindow :: use_fbo");
        return;
    }

    if (!use_fbo) {
        int th = g_height;
        if (th > screen_height)
            th = screen_height;
        maxy = th - maxy;
        miny = th - miny;
        uint32_t tmp = maxy; maxy = miny; miny = tmp;
        if ((int32_t)maxx > g_width) maxx = g_width;
        if ((int32_t)maxy > g_height) maxy = g_height;
        if (int(minx) < 0) minx = 0;
        if (int(miny) < 0) miny = 0;
        if (maxx < minx) maxx = minx;
        if (maxy < miny) maxy = miny;
        glScissor(minx, miny + g_viewport_offset, maxx - minx, maxy - miny);
        //printf("gl scissor %d %d %d %d\n", minx, miny, maxx, maxy);
    }
    else
    {
        glScissor(minx, (g_viewport_offset)+g_height - maxy, maxx - minx, maxy - miny);
    }
    glEnable(GL_SCISSOR_TEST);
    grDisplayGLError("gfxClipWindow");
}

void gfxColorMask(bool rgb, bool a)
{
    WriteTrace(TraceGlitch, TraceDebug, "rgb = %d, a: %d", rgb, a);
    glColorMask((GLboolean)rgb, (GLboolean)rgb, (GLboolean)rgb, (GLboolean)a);
    grDisplayGLError("gfxColorMask");
}

int isExtensionSupported(const char *extension)
{
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

bool gfxSstWinOpen(gfxColorFormat_t color_format, gfxOriginLocation_t origin_location, int nColBuffers, int nAuxBuffers)
{
    static int show_warning = 1;

    // ZIGGY
    // allocate static texture names
    // the initial value should be big enough to support the maximal resolution
    free_texture = 32 * 2048 * 2048;
    default_texture = free_texture++;
    color_texture = free_texture++;
    depth_texture = free_texture++;

#ifdef _WIN32
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cAuxBuffers = 1;

    int pfm;
#else
    fputs("ERROR:  No GLX yet to start GL on [Free]BSD, Linux etc.\n", stderr);
#endif // _WIN32

    WriteTrace(TraceGlitch, TraceDebug, "color_format: %d, origin_location: %d, nColBuffers: %d, nAuxBuffers: %d", color_format, origin_location, nColBuffers, nAuxBuffers);

#ifdef _WIN32
    TMU_SIZE = ((g_settings->wrpVRAM() * 1024 * 1024) - g_width * g_height * 4 * 3) / 2;

    // save screen resolution for hwfbe, after resolution enumeration
    screen_width = g_width;
    screen_height = g_height;

    if ((HWND)gfx.hWnd != NULL)
    {
        hDC = GetDC((HWND)gfx.hWnd);
    }
    if (hDC == NULL)
    {
        WriteTrace(TraceGlitch, TraceWarning, "GetDC on main window failed");
        return false;
    }

    if ((pfm = ChoosePixelFormat(hDC, &pfd)) == 0) {
        //printf("disabling auxiliary buffers\n");
        pfd.cAuxBuffers = 0;
        pfm = ChoosePixelFormat(hDC, &pfd);
    }
    if (pfm == 0)
    {
        WriteTrace(TraceGlitch, TraceWarning, "ChoosePixelFormat failed");
        return false;
    }
    if (SetPixelFormat(hDC, pfm, &pfd) == 0)
    {
        WriteTrace(TraceGlitch, TraceWarning, "SetPixelFormat failed");
        return false;
    }

    if ((hGLRC = wglCreateContext(hDC)) == 0)
    {
        WriteTrace(TraceGlitch, TraceWarning, "wglCreateContext failed!");
        gfxSstWinClose();
        return false;
    }

    HGLRC CurrenthGLRC = wglGetCurrentContext();

    if (CurrenthGLRC == NULL || CurrenthGLRC == hGLRC)
    {
        if (!wglMakeCurrent(hDC, hGLRC))
        {
            WriteTrace(TraceGlitch, TraceWarning, "wglMakeCurrent failed!");
            gfxSstWinClose();
            return false;
        }
    }
#endif // _WIN32
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

    if (glActiveTextureARB == NULL)
        glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)dummy_glActiveTexture;
    if (glMultiTexCoord2fARB == NULL)
        glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)dummy_glMultiTexCoord2f;
#endif // _WIN32

    nbTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, (GLint *)&nbTextureUnits);
    if (nbTextureUnits == 1) WriteTrace(TraceGlitch, TraceWarning, "You need a video card that has at least 2 texture units");

    nbAuxBuffers = 0;
    glGetIntegerv(GL_AUX_BUFFERS, &nbAuxBuffers);
    if (nbAuxBuffers > 0)
        printf("Congratulations, you have %d auxilliary buffers, we'll use them wisely !\n", nbAuxBuffers);

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

    if (isExtensionSupported("GL_ARB_texture_non_power_of_two") == 0)
        npot_support = 0;
    else {
        printf("NPOT extension used\n");
        npot_support = 1;
    }

#ifdef _WIN32
    glBlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)wglGetProcAddress("glBlendFuncSeparateEXT");
    if (glBlendFuncSeparateEXT == NULL)
        glBlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)dummy_glBlendFuncSeparate;
#endif // _WIN32

    if (isExtensionSupported("GL_EXT_fog_coord") == 0)
        fog_coord_support = 0;
    else
        fog_coord_support = 1;

#ifdef _WIN32
    glFogCoordfEXT = (PFNGLFOGCOORDFPROC)wglGetProcAddress("glFogCoordfEXT");
    if (glFogCoordfEXT == NULL)
        glFogCoordfEXT = (PFNGLFOGCOORDFPROC)dummy_glFogCoordf;
#endif // _WIN32

#ifdef _WIN32
    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if (wglGetExtensionsStringARB == NULL)
        wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)dummy_wglGetExtensionsString;
#endif // _WIN32

#ifdef _WIN32
    glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
    glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
    glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");

    if (glBindFramebufferEXT == NULL)
        glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)dummy_glBindFramebuffer;
    if (glFramebufferTexture2DEXT == NULL)
        glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)dummy_glFramebufferTexture2D;
    if (glGenFramebuffersEXT == NULL)
        glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)dummy_glGenFramebuffers;
    if (glCheckFramebufferStatusEXT == NULL)
        glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)dummy_glCheckFramebufferStatus;
    if (glDeleteFramebuffersEXT == NULL)
        glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)dummy_glDeleteFramebuffers;

    glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
    glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
    glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
    glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");

    if (glBindRenderbufferEXT == NULL)
        glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)dummy_glBindRenderbuffer;
    if (glDeleteRenderbuffersEXT == NULL)
        glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)dummy_glDeleteRenderbuffers;
    if (glGenRenderbuffersEXT == NULL)
        glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)dummy_glGenRenderbuffers;
    if (glRenderbufferStorageEXT == NULL)
        glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)dummy_glRenderbufferStorage;
    if (glFramebufferRenderbufferEXT == NULL)
        glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)dummy_glFramebufferRenderbuffer;
#endif // _WIN32

    use_fbo = g_settings->wrpFBO() && glFramebufferRenderbufferEXT;

    printf("use_fbo %d\n", use_fbo);

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

    if (glCreateShaderObjectARB == NULL)
        glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)dummy_glCreateShader;
    if (glShaderSourceARB == NULL)
        glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)dummy_glShaderSource;
    if (glCompileShaderARB == NULL)
        glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)dummy_glCompileShader;
    if (glCreateProgramObjectARB == NULL)
        glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)dummy_glCreateProgram;
    if (glAttachObjectARB == NULL)
        glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)dummy_glAttachObject;
    if (glLinkProgramARB == NULL)
        glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)dummy_glLinkProgram;
    if (glUseProgramObjectARB == NULL)
        glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)dummy_glUseProgram;
    if (glGetUniformLocationARB == NULL)
        glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)dummy_glGetUniformLocation;
    if (glUniform1iARB == NULL)
        glUniform1iARB = (PFNGLUNIFORM1IARBPROC)dummy_glUniform1i;
    if (glUniform4iARB == NULL)
        glUniform4iARB = (PFNGLUNIFORM4IARBPROC)dummy_glUniform4i;
    if (glUniform4fARB == NULL)
        glUniform4fARB = (PFNGLUNIFORM4FARBPROC)dummy_glUniform4f;
    if (glUniform1fARB == NULL)
        glUniform1fARB = (PFNGLUNIFORM1FARBPROC)dummy_glUniform1f;
    if (glDeleteObjectARB == NULL)
        glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)dummy_glDeleteObject;
    if (glGetInfoLogARB == NULL)
        glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)dummy_glGetInfoLog;
    if (glGetObjectParameterivARB == NULL)
        glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)dummy_glGetObjectParameteriv;

    if (glSecondaryColor3f == NULL)
        glSecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC)dummy_glSecondaryColor3f;
    if (glCompressedTexImage2DARB == NULL)
        glCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)dummy_glCompressedTexImage2D;
#endif

#ifndef ANDROID
    glViewport(0, g_viewport_offset, g_width, g_height);
    viewport_width = g_width;
    viewport_height = g_height;
    nvidia_viewport_hack = 1;
#else
    glViewport(0, g_viewport_offset, width, height);
    viewport_width = width;
    viewport_height = height;
#endif // _WIN32

    //   void do_benchmarks();
    //   do_benchmarks();

    // VP try to resolve z precision issues
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, 1 - zscale);
    glScalef(1, 1, zscale);

    widtho = g_width / 2;
    heighto = g_height / 2;

    pBufferWidth = pBufferHeight = -1;

    current_buffer = GL_BACK;

    texture_unit = GL_TEXTURE0_ARB;

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

    void FindBestDepthBias();
    FindBestDepthBias();

    init_geometry();
    init_textures();
    init_combiner();

    // Aniso filter check
    if (g_settings->wrpAnisotropic())
    {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);
    }

    // ATI hack - certain texture formats are slow on ATI?
    // Hmm, perhaps the internal format need to be specified explicitly...
    {
        GLint ifmt;
        glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, NULL);
        glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &ifmt);
        if (ifmt != GL_RGB5_A1) {
            WriteTrace(TraceGlitch, TraceWarning, "ATI SUCKS %x\n", ifmt);
            ati_sucks = 1;
        }
        else
            ati_sucks = 0;
    }

    grDisplayGLError("gfxSstWinOpen");
    return 1;
}

bool gfxSstWinClose()
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    int i;
#ifndef WIN32
    int clear_texbuff = use_fbo;
#endif

    for (i = 0; i < 2; i++) {
        tmu_usage[i].min = 0x0FFFFFFFul;
        tmu_usage[i].max = 0x00000000ul;
        invtex[i] = 0;
    }

    free_combiners();
#ifndef WIN32
    try // I don't know why, but opengl can be killed before this function call when emulator is closed (Gonetz).
        // ZIGGY : I found the problem : it is a function pointer, when the extension isn't supported , it is then zero, so just need to check the pointer prior to do the call.
    {
        if (use_fbo && glBindFramebufferEXT)
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
            glDeleteFramebuffersEXT(1, &(fbs[i].fbid));
            glDeleteRenderbuffersEXT(1, &(fbs[i].zbid));
        }
}
#endif
    nb_fb = 0;

    free_textures();
#ifndef WIN32
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
#else
    //SDL_QuitSubSystem(SDL_INIT_VIDEO);
    //sleep(2);
    //m_pScreen = NULL;
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
            glDrawBuffer(GL_AUX0);
            current_buffer = GL_AUX0;
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
            glReadBuffer(GL_BACK);
            glActiveTextureARB(texture_unit);
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
        if (tmu_usage[rtmu].min > pBufferAddress + 0)
            tmu_usage[rtmu].min = pBufferAddress + 0;
        if (tmu_usage[rtmu].max < pBufferAddress + size)
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

        grDisplayGLError("gfxTextureBufferExt :: A");
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
                    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
                    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbs[i].fbid);
                    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbs[i].texid, 0);
                    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbs[i].zbid);
                    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbs[i].zbid);
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
                    grDisplayGLError("gfxTextureBufferExt :: C");
                    return;
                }
                else //create new FBO at the same address, delete old one
                {
                    glDeleteFramebuffersEXT(1, &(fbs[i].fbid));
                    glDeleteRenderbuffersEXT(1, &(fbs[i].zbid));
                    if (nb_fb > 1)
                        memmove(&(fbs[i]), &(fbs[i + 1]), sizeof(fb)*(nb_fb - i));
                    nb_fb--;
                    break;
                }
            }
        }

        remove_tex(pBufferAddress, pBufferAddress + g_width*g_height * 2/*grTexFormatSize(fmt)*/);
        //create new FBO
        glGenFramebuffersEXT(1, &(fbs[nb_fb].fbid));
        glGenRenderbuffersEXT(1, &(fbs[nb_fb].zbid));
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbs[nb_fb].zbid);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, g_width, g_height);
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

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbs[nb_fb].fbid);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
            GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbs[nb_fb].texid, 0);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbs[nb_fb].zbid);
        glViewport(0, 0, g_width, g_height);
        glScissor(0, 0, g_width, g_height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glDepthMask(1);
        glClear(GL_DEPTH_BUFFER_BIT);
        CHECK_FRAMEBUFFER_STATUS();
        curBufferAddr = pBufferAddress;
        nb_fb++;
        grDisplayGLError("gfxTextureBufferExt :: B");
    }
}

int CheckTextureBufferFormat(gfxChipID_t tmu, uint32_t startAddress, gfxTexInfo *info)
{
    int found, i;
    if (!use_fbo) {
        for (found = i = 0; i < 2; i++)
            if (tmu_usage[i].min <= startAddress && tmu_usage[i].max > startAddress) {
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
    GLfloat planar_vertices[5][2];

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    planar_vertices[0][0] = ((int)dst_x - widtho) / (float)(g_width / 2);
    planar_vertices[0][1] = -((int)dst_y - heighto) / (float)(g_height / 2) * invert;
    planar_vertices[1][0] = ((int)dst_x - widtho) / (float)(g_width / 2);
    planar_vertices[1][1] = -((int)dst_y + (int)src_height - heighto) / (float)(g_height / 2) * invert;
    planar_vertices[2][0] = ((int)dst_x + (int)src_width - widtho) / (float)(g_width / 2);
    planar_vertices[2][1] = -((int)dst_y + (int)src_height - heighto) / (float)(g_height / 2) * invert;
    planar_vertices[3][0] = ((int)dst_x + (int)src_width - widtho) / (float)(g_width / 2);
    planar_vertices[3][1] = -((int)dst_y - heighto) / (float)(g_height / 2) * invert;
    planar_vertices[4][0] = ((int)dst_x - widtho) / (float)(g_width / 2);
    planar_vertices[4][1] = -((int)dst_y - heighto) / (float)(g_height / 2) * invert;

    glBegin(GL_QUADS);

    glMultiTexCoord2fARB(texture_number, 0.0f, 0.0f);
    glVertex2fv(planar_vertices[0]);

    glMultiTexCoord2fARB(texture_number, 0.0f, (float)src_height / (float)tex_height);
    glVertex2fv(planar_vertices[1]);

    glMultiTexCoord2fARB(texture_number, (float)src_width / (float)tex_width, (float)src_height / (float)tex_height);
    glVertex2fv(planar_vertices[2]);

    glMultiTexCoord2fARB(texture_number, (float)src_width / (float)tex_width, 0.0f);
    glVertex2fv(planar_vertices[3]);

    glMultiTexCoord2fARB(texture_number, 0.0f, 0.0f);
    glVertex2fv(planar_vertices[4]);

    glEnd();

    compile_shader();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    grDisplayGLError("render_rectangle");
}

void reloadTexture()
{
    if (use_fbo || !render_to_texture || buffer_cleared)
        return;

    WriteTrace(TraceGlitch, TraceDebug, "width: %d height: %d", g_width, g_height);

    buffer_cleared = 1;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glActiveTextureARB(texture_unit);
    glBindTexture(GL_TEXTURE_2D, pBufferAddress);
    glDisable(GL_ALPHA_TEST);
    glDrawBuffer(current_buffer);
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
    glPopAttrib();
    grDisplayGLError("reloadTexture");
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

        glPushAttrib(GL_ALL_ATTRIB_BITS);

        // save result of render to texture into actual texture
        glReadBuffer(current_buffer);
        glActiveTextureARB(texture_unit);
        // ZIGGY
        // deleting the texture before resampling it increases speed on certain old
        // nvidia cards (geforce 2 for example), unfortunatly it slows down a lot
        // on newer cards.
        //glDeleteTextures( 1, &pBufferAddress );
        glBindTexture(GL_TEXTURE_2D, pBufferAddress);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
            0, g_viewport_offset, g_width, g_height, 0);

        glBindTexture(GL_TEXTURE_2D, default_texture);
        glPopAttrib();
    }
    grDisplayGLError("updateTexture");
}

void gfxRenderBuffer(gfxBuffer_t buffer)
{
#ifdef _WIN32
    static HANDLE region = NULL;
    //int realWidth = pBufferWidth, realHeight = pBufferHeight;
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
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glTranslatef(0, 0, 1 - zscale);
            glScalef(1, 1, zscale);
            inverted_culling = 0;
            gfxCullMode(culling_mode);

            g_width = savedWidth;
            g_height = savedHeight;
            widtho = savedWidtho;
            heighto = savedHeighto;
            if (use_fbo)
            {
                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
                glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
            }
            curBufferAddr = 0;

            glViewport(0, g_viewport_offset, g_width, viewport_height);
            glScissor(0, g_viewport_offset, g_width, g_height);

#ifdef SAVE_CBUFFER
            if (!use_fbo && render_to_texture == 2) {
                // restore color buffer
                if (nbAuxBuffers > 0) {
                    glDrawBuffer(GL_BACK);
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

                    glPushAttrib(GL_ALL_ATTRIB_BITS);
                    glDisable(GL_ALPHA_TEST);
                    glDrawBuffer(GL_BACK);
                    glActiveTextureARB(texture_unit);
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
                    glPopAttrib();

                    save_w = save_h = 0;
                }
            }
#endif
            render_to_texture = 0;
        }
        glDrawBuffer(GL_BACK);
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
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glTranslatef(0, 0, 1 - zscale);
                glScalef(1, 1, zscale);
                inverted_culling = 0;
            }
            else {
                float m[4 * 4] = { 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, -1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f };
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(m);
                // VP z fix
                glTranslatef(0, 0, 1 - zscale);
                glScalef(1, 1 * 1, zscale);
                inverted_culling = 1;
                gfxCullMode(culling_mode);
            }
        }
        render_to_texture = 1;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxRenderBuffer : unknown buffer : %x", buffer);
    }
    grDisplayGLError("gfxRenderBuffer");
}

void gfxAuxBufferExt(gfxBuffer_t buffer)
{
    WriteTrace(TraceGlitch, TraceDebug, "buffer: %d", buffer);

    if (buffer == GFX_BUFFER_AUXBUFFER)
    {
        invtex[0] = 0;
        invtex[1] = 0;
        need_to_compile = 0;
        set_depth_shader();
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glDisable(GL_CULL_FACE);
        glDisable(GL_ALPHA_TEST);
        glDepthMask(GL_TRUE);
        gfxTexFilterMode(GFX_TMU1, GFX_TEXTUREFILTER_POINT_SAMPLED, GFX_TEXTUREFILTER_POINT_SAMPLED);
    }
    else {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        need_to_compile = 1;
    }
    grDisplayGLError("gfxAuxBufferExt");
}

void gfxBufferClear(gfxColor_t color, gfxAlpha_t alpha, uint32_t depth)
{
    WriteTrace(TraceGlitch, TraceDebug, "color: %X alpha: %X depth: %X", color, alpha, depth);
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

    if (w_buffer_mode)
        glClearDepth(1.0f - ((1.0f + (depth >> 4) / 4096.0f) * (1 << (depth & 0xF))) / 65528.0);
    else
        glClearDepth(depth / 65535.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ZIGGY TODO check that color mask is on
    buffer_cleared = 1;

    grDisplayGLError("gfxBufferClear");
}

void gfxBufferSwap(uint32_t swap_interval)
{
    int i;
    WriteTrace(TraceGlitch, TraceDebug, "swap_interval: %d", swap_interval);
    //printf("swap\n");
    if (render_to_texture) {
        WriteTrace(TraceGlitch, TraceWarning, "swap while render_to_texture\n");
        return;
    }

#ifdef _WIN32
    SwapBuffers(wglGetCurrentDC());
#else // _WIN32
#endif // _WIN32
    for (i = 0; i < nb_fb; i++)
        fbs[i].buff_clear = 1;

    // VP debugging
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
            glReadBuffer(GL_FRONT);
            break;
        case GFX_BUFFER_BACKBUFFER:
            glReadBuffer(GL_BACK);
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
                glReadPixels(0, g_viewport_offset, g_width, g_height, GL_BGRA, GL_UNSIGNED_BYTE, frameBuffer);
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

    grDisplayGLError("gfxLfbLock");
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
        glReadBuffer(GL_FRONT);
        break;
    case GFX_BUFFER_BACKBUFFER:
        glReadBuffer(GL_BACK);
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

    grDisplayGLError("gfxLfbReadRegion");
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

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    while (tex_width < src_width) tex_width <<= 1;
    while (tex_height < src_height) tex_height <<= 1;

    switch (dst_buffer)
    {
    case GFX_BUFFER_BACKBUFFER:
        glDrawBuffer(GL_BACK);
        break;
    case GFX_BUFFER_AUXBUFFER:
        glDrawBuffer(current_buffer);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxLfbWriteRegion : unknown buffer : %x", dst_buffer);
    }

    if (dst_buffer != GFX_BUFFER_AUXBUFFER)
    {
        buf = (unsigned char*)malloc(tex_width*tex_height * 4);

        texture_number = GL_TEXTURE0_ARB;
        glActiveTextureARB(texture_number);

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

        glDrawBuffer(GL_BACK);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDepthMask(1);
        glDrawPixels(src_width, src_height + (g_viewport_offset), GL_DEPTH_COMPONENT, GL_FLOAT, buf);

        free(buf);
    }
    glDrawBuffer(current_buffer);
    glPopAttrib();

    grDisplayGLError("gfxLfbWriteRegion");
    return true;
}

/* wrapper-specific glide extensions */
#ifdef _WIN32
static void CorrectGamma(LPVOID apGammaRamp)
{
    HDC hdc = GetDC(NULL);
    if (hdc != NULL)
    {
        SetDeviceGammaRamp(hdc, apGammaRamp);
        ReleaseDC(NULL, hdc);
    }
}
#else
static void CorrectGamma(const uint16_t aGammaRamp[3][256])
{
    int res;

    /* res = SDL_SetGammaRamp(aGammaRamp[0], aGammaRamp[1], aGammaRamp[2]); */
    res = -1;
    fputs("ERROR:  Replacement for SDL_SetGammaRamp unimplemented.\n", stderr);
    WriteTrace(TraceGlitch, TraceDebug, "SDL_SetGammaRamp returned %d\r\n", res);
}
#endif

void gfxLoadGammaTable(uint32_t /*nentries*/, uint32_t *red, uint32_t *green, uint32_t *blue)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    uint16_t aGammaRamp[3][256];
    for (int i = 0; i < 256; i++)
    {
        aGammaRamp[0][i] = (uint16_t)((red[i] << 8) & 0xFFFF);
        aGammaRamp[1][i] = (uint16_t)((green[i] << 8) & 0xFFFF);
        aGammaRamp[2][i] = (uint16_t)((blue[i] << 8) & 0xFFFF);
    }
    CorrectGamma(aGammaRamp);
}

void gfxGetGammaTableExt(uint32_t /*nentries*/, uint32_t *red, uint32_t *green, uint32_t *blue)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");
    uint16_t aGammaRamp[3][256];
#ifdef _WIN32
    HDC hdc = GetDC(NULL);
    if (hdc == NULL)
        return;
    if (GetDeviceGammaRamp(hdc, aGammaRamp) == TRUE)
    {
        ReleaseDC(NULL, hdc);
#else
    fputs("ERROR:  Replacement for SDL_GetGammaRamp unimplemented.\n", stderr);
    /* if (SDL_GetGammaRamp(aGammaRamp[0], aGammaRamp[1], aGammaRamp[2]) != -1) */
    {
#endif
        for (int i = 0; i < 256; i++)
        {
            red[i] = aGammaRamp[0][i] >> 8;
            green[i] = aGammaRamp[1][i] >> 8;
            blue[i] = aGammaRamp[2][i] >> 8;
        }
    }
    }

void gfxGammaCorrectionRGB(float gammaR, float gammaG, float gammaB)
{
    WriteTrace(TraceGlitch, TraceDebug, "-");

    uint16_t aGammaRamp[3][256];
    for (int i = 0; i < 256; i++)
    {
        aGammaRamp[0][i] = (((uint16_t)((pow(i / 255.0F, 1.0F / gammaR)) * 255.0F + 0.5F)) << 8) & 0xFFFF;
        aGammaRamp[1][i] = (((uint16_t)((pow(i / 255.0F, 1.0F / gammaG)) * 255.0F + 0.5F)) << 8) & 0xFFFF;
        aGammaRamp[2][i] = (((uint16_t)((pow(i / 255.0F, 1.0F / gammaB)) * 255.0F + 0.5F)) << 8) & 0xFFFF;
    }
    CorrectGamma(aGammaRamp);
}

static const char * GL_errors[7 + 1] = {
    "GL_NO_ERROR", /* "There is no current error." */
    "GL_INVALID_ENUM", /* "Invalid parameter." */
    "GL_INVALID_VALUE", /* "Invalid enum parameter value." */
    "GL_INVALID_OPERATION", /* "Illegal call." */
    "GL_STACK_OVERFLOW",
    "GL_STACK_UNDERFLOW",
    "GL_OUT_OF_MEMORY", /* "Unable to allocate memory." */

    "GL_UNKNOWN_ERROR" /* ??? */
};

#ifndef _DEBUG
int grDisplayGLError(const char* /*unused*/)
{
    return -1;
}
#else
int grDisplayGLError(const char* message)
{
    GLenum status;
    unsigned int error_index;
    int failure;

    status = glGetError();
    failure = 1;

    if (status == GL_NO_ERROR)
        error_index = failure = 0;
    else
        error_index =
        (status < GL_INVALID_ENUM) /* to avoid underflow when subtracting */
        ? (7) /* our own, made-up "GL_UNKNOWN_ERROR" error */
        : (status - GL_INVALID_ENUM) + 1;

    if (error_index > 7)
        error_index = 7;

#if !0
    /*
     * In most cases, we don't want to spam the screen to repeatedly say that
     * there were no OpenGL errors yet, though sometimes one may need verbosity.
     */
    if (failure == 0)
        return (failure);
#endif

#ifdef _WIN32
    MessageBoxA(NULL, message, GL_errors[error_index], MB_ICONERROR);
#else
    fprintf(stderr, "%s\n%s\n\n", GL_errors[error_index], message);
#endif
    return (failure);
}
#endif

void CHECK_FRAMEBUFFER_STATUS()
{
    GLenum status;
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    WriteTrace(TraceGlitch, TraceDebug, "status: %X", status);
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        /*WriteTrace(TraceGlitch, TraceWarning, "framebuffer complete!\n");*/
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT\n");
        /* you gotta choose different formats */
        /*assert(0);*/
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer INCOMPLETE_ATTACHMENT\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer FRAMEBUFFER_MISSING_ATTACHMENT\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer FRAMEBUFFER_DIMENSIONS\n");
        break;
        /*case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
          WriteTrace(TraceGlitch, TraceWarning, "framebuffer INCOMPLETE_DUPLICATE_ATTACHMENT\n");
          break;*/
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer INCOMPLETE_FORMATS\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer INCOMPLETE_DRAW_BUFFER\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer INCOMPLETE_READ_BUFFER\n");
        break;
    case GL_FRAMEBUFFER_BINDING_EXT:
        WriteTrace(TraceGlitch, TraceWarning, "framebuffer BINDING_EXT\n");
        break;
    default:
        break;
        /* programming error; will fail on all hardware */
        /*assert(0);*/
    }
}