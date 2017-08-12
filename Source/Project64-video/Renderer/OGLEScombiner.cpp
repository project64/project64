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

#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <string.h>
#include <stdlib.h>
#endif // _WIN32
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "glitchmain.h"
#include <Project64-video/trace.h>
#include <Project64-video/Settings.h>
#include <vector>

void vbo_draw();

static gfxCmpFnc_t g_alpha_func;
static int g_alpha_ref;
static bool g_alpha_test = 0;

static float g_texture_env_color[4];
static float g_ccolor0[4];
static float g_ccolor1[4];
static float g_chroma_color[4];
static int g_fog_enabled;
static bool g_chroma_enabled;
static int chroma_other_color;
static int chroma_other_alpha;
static int dither_enabled;
int blackandwhite0;
int blackandwhite1;

float fogStart, fogEnd;
float fogColor[4];

int need_lambda[2];
float lambda_color[2][4];

// shaders variables
int need_to_compile;

static GLuint g_program_object_default = 0;
static int constant_color_location;
static int ccolor0_location;
static int ccolor1_location;
static int first_color = 1;
static int first_alpha = 1;
static int first_texture0 = 1;
static int first_texture1 = 1;
static int tex0_combiner_ext = 0;
static int tex1_combiner_ext = 0;
static int c_combiner_ext = 0;
static int a_combiner_ext = 0;

#define GLSL_VERSION "100"

#define SHADER_HEADER \
"#version " GLSL_VERSION "          \n"

#define SHADER_VARYING \
"varying highp vec4 vFrontColor;  \n" \
"varying highp vec4 vTexCoord[4]; \n"

static const char* g_fragment_shader_header =
SHADER_HEADER
"precision lowp float;             \n"
"uniform sampler2D texture0;       \n"
"uniform sampler2D texture1;       \n"
"uniform sampler2D ditherTex;      \n"
"uniform vec4 constant_color;      \n"
"uniform vec4 ccolor0;             \n"
"uniform vec4 ccolor1;             \n"
"uniform vec4 chroma_color;        \n"
"uniform float lambda;             \n"
"uniform vec3 fogColor;            \n"
"uniform float alphaRef;           \n"
SHADER_VARYING
"                                  \n"
"void test_chroma(vec4 ctexture1); \n"
"                                  \n"
"                                  \n"
"void main()                       \n"
"{                                 \n"
;

// using gl_FragCoord is terribly slow on ATI and varying variables don't work for some unknown
// reason, so we use the unused components of the texture2 coordinates
static const char* g_fragment_shader_dither =
"  float dithx = (vTexCoord[2].b + 1.0)*0.5*1000.0; \n"
"  float dithy = (vTexCoord[2].a + 1.0)*0.5*1000.0; \n"
"  if(texture2D(ditherTex, vec2((dithx-32.0*floor(dithx/32.0))/32.0, \n"
"                               (dithy-32.0*floor(dithy/32.0))/32.0)).a > 0.5) discard; \n"
;

static const char* g_fragment_shader_default =
"  gl_FragColor = texture2D(texture0, vec2(vTexCoord[0])); \n"
;

static const char* g_fragment_shader_readtex0color =
"  vec4 readtex0 = texture2D(texture0, vec2(vTexCoord[0])); \n"
;

static const char* g_fragment_shader_readtex0bw =
"  vec4 readtex0 = texture2D(texture0, vec2(vTexCoord[0])); \n"
"  readtex0 = vec4(vec3(readtex0.b),                          \n"
"                  readtex0.r + readtex0.g * 8.0 / 256.0);    \n"
;
static const char* g_fragment_shader_readtex0bw_2 =
"  vec4 readtex0 = vec4(dot(texture2D(texture0, vec2(vTexCoord[0])), vec4(1.0/3, 1.0/3, 1.0/3, 0)));                        \n"
;

static const char* g_fragment_shader_readtex1color =
"  vec4 readtex1 = texture2D(texture1, vec2(vTexCoord[1])); \n"
;

static const char* g_fragment_shader_readtex1bw =
"  vec4 readtex1 = texture2D(texture1, vec2(vTexCoord[1])); \n"
"  readtex1 = vec4(vec3(readtex1.b),                          \n"
"                  readtex1.r + readtex1.g * 8.0 / 256.0);    \n"
;
static const char* g_fragment_shader_readtex1bw_2 =
"  vec4 readtex1 = vec4(dot(texture2D(texture1, vec2(vTexCoord[1])), vec4(1.0/3, 1.0/3, 1.0/3, 0)));                        \n"
;

static const char* g_fragment_shader_fog =
"  float fog;                                                                         \n"
"  fog = vTexCoord[0].b;                                                            \n"
"  gl_FragColor.rgb = mix(fogColor, gl_FragColor.rgb, fog); \n"
;

static const char* g_fragment_shader_end =
"if(gl_FragColor.a <= alphaRef) {discard;}   \n"
"                                \n"
"}                               \n"
;

static const char* g_vertex_shader =
SHADER_HEADER
"#define Z_MAX 65536.0                                          \n"
"attribute highp vec4 aPosition;                                \n"
"attribute highp vec4 aColor;                                   \n"
"attribute highp vec4 aMultiTexCoord0;                          \n"
"attribute highp vec4 aMultiTexCoord1;                          \n"
"attribute float aFog;                                          \n"
"uniform vec3 vertexOffset;                                     \n" //Moved some calculations from grDrawXXX to shader
"uniform vec4 textureSizes;                                     \n"
"uniform vec3 fogModeEndScale;                                  \n" //0 = Mode, 1 = gl_Fog.end, 2 = gl_Fog.scale
"uniform mat4 rotation_matrix;                                  \n"
SHADER_VARYING
"                                                               \n"
"void main()                                                    \n"
"{                                                              \n"
"  float q = aPosition.w;                                                   \n"
"  float invertY = vertexOffset.z;                                          \n" //Usually 1.0 but -1.0 when rendering to a texture (see inverted_culling gfxRenderBuffer)
"  gl_Position.x = (aPosition.x - vertexOffset.x) / vertexOffset.x;         \n"
"  gl_Position.y = invertY *-(aPosition.y - vertexOffset.y) / vertexOffset.y;\n"
"  gl_Position.z = aPosition.z / Z_MAX;                                     \n"
"  gl_Position.w = 1.0;                                                     \n"
"  gl_Position /= q;                                                        \n"
"  gl_Position = rotation_matrix * gl_Position;                             \n"
"  vFrontColor = aColor.bgra;                                               \n"
"                                                                           \n"
"  vTexCoord[0] = vec4(aMultiTexCoord0.xy / q / textureSizes.xy,0,1);       \n"
"  vTexCoord[1] = vec4(aMultiTexCoord1.xy / q / textureSizes.zw,0,1);       \n"
"                                                                           \n"
"  float fogV = (1.0 / mix(q,aFog,fogModeEndScale[0])) / 255.0;             \n"
"  //if(fogMode == 2) {                                                     \n"
"  //  fogV = 1.0 / aFog / 255                                              \n"
"  //}                                                                      \n"
"                                                                           \n"
"  float f = (fogModeEndScale[1] - fogV) * fogModeEndScale[2];              \n"
"  f = clamp(f, 0.0, 1.0);                                                  \n"
"  vTexCoord[0].b = f;                                                      \n"
"  vTexCoord[2].b = aPosition.x;                                            \n"
"  vTexCoord[2].a = aPosition.y;                                            \n"
"}                                                                          \n"
;

static char fragment_shader_color_combiner[1024];
static char fragment_shader_alpha_combiner[1024];
static char fragment_shader_texture1[1024];
static char fragment_shader_texture0[1024];
static char fragment_shader_chroma[1024];

GLuint CompileShader(GLenum type, const std::string &source)
{
    GLuint shader = glCreateShader(type);

    const char *sourceArray[1] = { source.c_str() };
    glShaderSource(shader, 1, sourceArray, NULL);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

    if (compileResult == 0)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetShaderInfoLog(shader, (GLsizei)infoLog.size(), NULL, infoLog.data());

        WriteTrace(TraceGlitch, TraceError, "Shader compilation failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
        return 0;
    }

    return shader;
}

void check_link(GLuint program)
{
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(program, 1024, NULL, log);
        //LOGINFO(log);
    }
}

void set_rotation_matrix(GLuint loc, CSettings::ScreenRotate_t rotate)
{
    GLfloat mat[16];

    /* first setup everything which is the same everytime */
    /* (X, X, 0, 0)
     * (X, X, 0, 0)
     * (0, 0, 1, 0)
     * (0, 0, 0, 1)
     */

    mat[0] = 1;
    mat[1] = 0;
    mat[2] = 0;
    mat[3] = 0;

    mat[4] = 0;
    mat[5] = 1;
    mat[6] = 0;
    mat[7] = 0;

    mat[8] = 0;
    mat[9] = 0;
    mat[10] = 1;
    mat[11] = 0;

    mat[12] = 0;
    mat[13] = 0;
    mat[14] = 0;
    mat[15] = 1;

    /* now set the actual rotation */
    if (rotate == CSettings::Rotate_90)
    {
        mat[0] = 0;
        mat[1] = 1;
        mat[4] = -1;
        mat[5] = 0;
    }
    else if (rotate == CSettings::Rotate_180)
    {
        mat[0] = -1;
        mat[1] = 0;
        mat[4] = 0;
        mat[5] = -1;
    }
    else if (rotate == CSettings::Rotate_270)
    {
        mat[0] = 0;
        mat[1] = -1;
        mat[4] = 1;
        mat[5] = 0;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
}

void init_combiner()
{
    int texture[4] = { 0, 0, 0, 0 };

    glActiveTexture(GL_TEXTURE0);

    // creating a fake texture
    glBindTexture(GL_TEXTURE_2D, default_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, default_texture);

    int texture0_location;
    int texture1_location;

    // default shader
    std::string fragment_shader = g_fragment_shader_header;
    fragment_shader += g_fragment_shader_default;
    fragment_shader += g_fragment_shader_end;

    GLuint fragment_shader_object = CompileShader(GL_FRAGMENT_SHADER, fragment_shader);
    GLuint vertex_shader_object = CompileShader(GL_VERTEX_SHADER, g_vertex_shader);

    // default program
    g_program_object_default = glCreateProgram();
    glAttachShader(g_program_object_default, fragment_shader_object);
    glAttachShader(g_program_object_default, vertex_shader_object);
    glDeleteShader(fragment_shader_object);
    glDeleteShader(vertex_shader_object);

    glBindAttribLocation(g_program_object_default, POSITION_ATTR, "aPosition");
    glBindAttribLocation(g_program_object_default, COLOUR_ATTR, "aColor");
    glBindAttribLocation(g_program_object_default, TEXCOORD_0_ATTR, "aMultiTexCoord0");
    glBindAttribLocation(g_program_object_default, TEXCOORD_1_ATTR, "aMultiTexCoord1");
    glBindAttribLocation(g_program_object_default, FOG_ATTR, "aFog");

    glLinkProgram(g_program_object_default);
    check_link(g_program_object_default);
    glUseProgram(g_program_object_default);
    int rotation_matrix_location = glGetUniformLocation(g_program_object_default, "rotation_matrix");
    set_rotation_matrix(rotation_matrix_location, g_settings->rotate());

    texture0_location = glGetUniformLocation(g_program_object_default, "texture0");
    texture1_location = glGetUniformLocation(g_program_object_default, "texture1");
    glUniform1i(texture0_location, 0);
    glUniform1i(texture1_location, 1);

    strcpy(fragment_shader_color_combiner, "");
    strcpy(fragment_shader_alpha_combiner, "");
    strcpy(fragment_shader_texture1, "vec4 ctexture1 = texture2D(texture0, vec2(vTexCoord[0])); \n");
    strcpy(fragment_shader_texture0, "");

    first_color = 1;
    first_alpha = 1;
    first_texture0 = 1;
    first_texture1 = 1;
    need_to_compile = 0;
    g_fog_enabled = 0;
    g_chroma_enabled = false;
    dither_enabled = 0;
    blackandwhite0 = 0;
    blackandwhite1 = 0;
}

void compile_chroma_shader()
{
    strcpy(fragment_shader_chroma, "\nvoid test_chroma(vec4 ctexture1)\n{\n");

    switch (chroma_other_alpha)
    {
    case GFX_COMBINE_OTHER_ITERATED:
        strcat(fragment_shader_chroma, "float alpha = vFrontColor.a; \n");
        break;
    case GFX_COMBINE_OTHER_TEXTURE:
        strcat(fragment_shader_chroma, "float alpha = ctexture1.a; \n");
        break;
    case GFX_COMBINE_OTHER_CONSTANT:
        strcat(fragment_shader_chroma, "float alpha = constant_color.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown compile_choma_shader_alpha : %x", chroma_other_alpha);
    }

    switch (chroma_other_color)
    {
    case GFX_COMBINE_OTHER_ITERATED:
        strcat(fragment_shader_chroma, "vec4 color = vec4(vec3(vFrontColor),alpha); \n");
        break;
    case GFX_COMBINE_OTHER_TEXTURE:
        strcat(fragment_shader_chroma, "vec4 color = vec4(vec3(ctexture1),alpha); \n");
        break;
    case GFX_COMBINE_OTHER_CONSTANT:
        strcat(fragment_shader_chroma, "vec4 color = vec4(vec3(constant_color),alpha); \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown compile_choma_shader_alpha : %x", chroma_other_color);
    }

    strcat(fragment_shader_chroma, "if (color.rgb == chroma_color.rgb) discard; \n");
    strcat(fragment_shader_chroma, "}");
}

typedef struct _shader_program_key
{
    int color_combiner;
    int alpha_combiner;
    int texture0_combiner;
    int texture1_combiner;
    int texture0_combinera;
    int texture1_combinera;
    int fog_enabled;
    bool chroma_enabled;
    int dither_enabled;
    int blackandwhite0;
    int blackandwhite1;
    GLuint program_object;
    int texture0_location;
    int texture1_location;
    int vertexOffset_location;
    int textureSizes_location;
    int fogModeEndScale_location;
    int fogColor_location;
    int alphaRef_location;
    int ditherTex_location;
    int chroma_color_location;
} shader_program_key;

static std::vector<shader_program_key> g_shader_programs;
static int color_combiner_key;
static int alpha_combiner_key;
static int texture0_combiner_key;
static int texture1_combiner_key;
static int texture0_combinera_key;
static int texture1_combinera_key;

void update_uniforms(GLuint program_object, const shader_program_key & prog)
{
    glUniform1i(prog.texture0_location, 0);
    glUniform1i(prog.texture1_location, 1);

    glUniform3f(prog.vertexOffset_location, widtho, heighto, inverted_culling ? -1.0f : 1.0f);
    glUniform4f(prog.textureSizes_location, tex0_width, tex0_height, tex1_width, tex1_height);

    glUniform3f(prog.fogModeEndScale_location,
        g_fog_enabled != 2 ? 0.0f : 1.0f,
        fogEnd,
        1.0f / (fogEnd - fogStart)
    );

    if (prog.fogColor_location != -1)
    {
        glUniform3f(prog.fogColor_location, fogColor[0], fogColor[1], fogColor[2]);
    }

    glUniform1f(prog.alphaRef_location, g_alpha_test ? g_alpha_ref / 255.0f : -1.0f);

    constant_color_location = glGetUniformLocation(program_object, "constant_color");
    glUniform4f(constant_color_location, g_texture_env_color[0], g_texture_env_color[1],
        g_texture_env_color[2], g_texture_env_color[3]);

    ccolor0_location = glGetUniformLocation(program_object, "ccolor0");
    glUniform4f(ccolor0_location, g_ccolor0[0], g_ccolor0[1], g_ccolor0[2], g_ccolor0[3]);

    ccolor1_location = glGetUniformLocation(program_object, "ccolor1");
    glUniform4f(ccolor1_location, g_ccolor1[0], g_ccolor1[1], g_ccolor1[2], g_ccolor1[3]);

    glUniform4f(prog.chroma_color_location, g_chroma_color[0], g_chroma_color[1],
        g_chroma_color[2], g_chroma_color[3]);

    if (dither_enabled)
    {
        glUniform1i(prog.ditherTex_location, 2);
    }

    GLuint rotation_matrix_location = glGetUniformLocation(program_object, "rotation_matrix");
    set_rotation_matrix(rotation_matrix_location, g_settings->rotate());
    set_lambda();
}

void disable_textureSizes()
{
    int textureSizes_location = glGetUniformLocation(g_program_object_default, "textureSizes");
    glUniform4f(textureSizes_location, 1, 1, 1, 1);
}

void compile_shader()
{
    need_to_compile = 0;

    for (size_t i = 0; i < g_shader_programs.size(); i++)
    {
        shader_program_key & prog = g_shader_programs[i];
        if (prog.color_combiner == color_combiner_key &&
            prog.alpha_combiner == alpha_combiner_key &&
            prog.texture0_combiner == texture0_combiner_key &&
            prog.texture1_combiner == texture1_combiner_key &&
            prog.texture0_combinera == texture0_combinera_key &&
            prog.texture1_combinera == texture1_combinera_key &&
            prog.fog_enabled == g_fog_enabled &&
            prog.chroma_enabled == g_chroma_enabled &&
            prog.dither_enabled == dither_enabled &&
            prog.blackandwhite0 == blackandwhite0 &&
            prog.blackandwhite1 == blackandwhite1)
        {
            glUseProgram(prog.program_object);
            update_uniforms(prog.program_object, prog);
            return;
        }
    }

    shader_program_key shader_program;
    shader_program.color_combiner = color_combiner_key;
    shader_program.alpha_combiner = alpha_combiner_key;
    shader_program.texture0_combiner = texture0_combiner_key;
    shader_program.texture1_combiner = texture1_combiner_key;
    shader_program.texture0_combinera = texture0_combinera_key;
    shader_program.texture1_combinera = texture1_combinera_key;
    shader_program.fog_enabled = g_fog_enabled;
    shader_program.chroma_enabled = g_chroma_enabled;
    shader_program.dither_enabled = dither_enabled;
    shader_program.blackandwhite0 = blackandwhite0;
    shader_program.blackandwhite1 = blackandwhite1;

    if (g_chroma_enabled)
    {
        strcat(fragment_shader_texture1, "test_chroma(ctexture1); \n");
        compile_chroma_shader();
    }

    std::string fragment_shader = g_fragment_shader_header;

    if (dither_enabled)
    {
        fragment_shader += g_fragment_shader_dither;
    }
    switch (blackandwhite0)
    {
    case 1: fragment_shader += g_fragment_shader_readtex0bw; break;
    case 2: fragment_shader += g_fragment_shader_readtex0bw_2; break;
    default: fragment_shader += g_fragment_shader_readtex0color;
    }
    switch (blackandwhite1)
    {
    case 1: fragment_shader += g_fragment_shader_readtex1bw; break;
    case 2: fragment_shader += g_fragment_shader_readtex1bw_2; break;
    default: fragment_shader += g_fragment_shader_readtex1color;
    }
    fragment_shader += fragment_shader_texture0;
    fragment_shader += fragment_shader_texture1;
    fragment_shader += fragment_shader_color_combiner;
    fragment_shader += fragment_shader_alpha_combiner;
    if (g_fog_enabled)
    {
        fragment_shader += g_fragment_shader_fog;
    }
    fragment_shader += g_fragment_shader_end;
    if (g_chroma_enabled)
    {
        fragment_shader += fragment_shader_chroma;
    }

    GLuint fragment_shader_object = CompileShader(GL_FRAGMENT_SHADER, fragment_shader);
    GLuint vertex_shader_object = CompileShader(GL_VERTEX_SHADER, g_vertex_shader);

    GLuint program_object = glCreateProgram();
    shader_program.program_object = program_object;

    glBindAttribLocation(program_object, POSITION_ATTR, "aPosition");
    glBindAttribLocation(program_object, COLOUR_ATTR, "aColor");
    glBindAttribLocation(program_object, TEXCOORD_0_ATTR, "aMultiTexCoord0");
    glBindAttribLocation(program_object, TEXCOORD_1_ATTR, "aMultiTexCoord1");
    glBindAttribLocation(program_object, FOG_ATTR, "aFog");

    glAttachShader(shader_program.program_object, fragment_shader_object);
    glDeleteShader(fragment_shader_object);

    glAttachShader(shader_program.program_object, vertex_shader_object);
    glDeleteShader(vertex_shader_object);

    glLinkProgram(program_object);
    check_link(program_object);
    glUseProgram(program_object);

    shader_program.texture0_location = glGetUniformLocation(program_object, "texture0");
    shader_program.texture1_location = glGetUniformLocation(program_object, "texture1");
    shader_program.vertexOffset_location = glGetUniformLocation(program_object, "vertexOffset");
    shader_program.textureSizes_location = glGetUniformLocation(program_object, "textureSizes");
    shader_program.fogModeEndScale_location = glGetUniformLocation(program_object, "fogModeEndScale");
    shader_program.fogColor_location = glGetUniformLocation(program_object, "fogColor");
    shader_program.alphaRef_location = glGetUniformLocation(program_object, "alphaRef");
    shader_program.chroma_color_location = glGetUniformLocation(program_object, "chroma_color");

    update_uniforms(shader_program.program_object, shader_program);
    g_shader_programs.push_back(shader_program);
}

void free_combiners()
{
    if (g_program_object_default != 0)
    {
        glDeleteProgram(g_program_object_default);
        g_program_object_default = 0;
    }
    for (size_t i = 0; i < g_shader_programs.size(); i++)
    {
        glDeleteProgram(g_shader_programs[i].program_object);
        g_shader_programs[i].program_object = 0;
    }
    g_shader_programs.clear();

    g_alpha_ref = 0;
    g_alpha_func = GFX_CMP_NEVER;
    g_alpha_test = 0;

    memset(g_texture_env_color, 0, sizeof(g_texture_env_color));
    memset(g_ccolor0, 0, sizeof(g_ccolor0));
    memset(g_ccolor1, 0, sizeof(g_ccolor1));
    memset(g_chroma_color, 0, sizeof(g_chroma_color));
    g_fog_enabled = 0;
    g_chroma_enabled = false;
    chroma_other_color = 0;
    chroma_other_alpha = 0;
    dither_enabled = 0;
    blackandwhite0 = 0;
    blackandwhite1 = 0;

    fogStart = 0.0f;
    fogEnd = 0.0f;
    for (int i = 0; i < (sizeof(fogColor) / sizeof(fogColor[0])); i++)
    {
        fogColor[i] = 0.0f;
    }
    memset(need_lambda, 0, sizeof(need_lambda));
    for (int i = 0; i < (sizeof(lambda_color) / sizeof(lambda_color[0])); i++)
    {
        for (int z = 0; z < (sizeof(lambda_color[i]) / sizeof(lambda_color[i][0])); z++)
        {
            lambda_color[i][z] = 0.0f;
        }
    }
    need_to_compile = 0;

    g_program_object_default = 0;
    constant_color_location = 0;
    ccolor0_location = 0;
    ccolor1_location = 0;
    first_color = 1;
    first_alpha = 1;
    first_texture0 = 1;
    first_texture1 = 1;
    tex0_combiner_ext = 0;
    tex1_combiner_ext = 0;
    c_combiner_ext = 0;
    a_combiner_ext = 0;
}

void set_copy_shader()
{
    int texture0_location;
    int alphaRef_location;

    glUseProgram(g_program_object_default);
    texture0_location = glGetUniformLocation(g_program_object_default, "texture0");
    glUniform1i(texture0_location, 0);

    alphaRef_location = glGetUniformLocation(g_program_object_default, "alphaRef");
    if (alphaRef_location != -1)
    {
        glUniform1f(alphaRef_location, g_alpha_test ? g_alpha_ref / 255.0f : -1.0f);
    }
}

void set_depth_shader()
{
}

void set_lambda()
{
    int lambda_location = glGetUniformLocation(g_program_object_default, "lambda");
    glUniform1f(lambda_location, lambda);
}

void gfxConstantColorValue(gfxColor_t value)
{
    WriteTrace(TraceGlitch, TraceDebug, "value: %d", value);
    switch (lfb_color_fmt)
    {
    case GFX_COLORFORMAT_ARGB:
        g_texture_env_color[3] = ((value >> 24) & 0xFF) / 255.0f;
        g_texture_env_color[0] = ((value >> 16) & 0xFF) / 255.0f;
        g_texture_env_color[1] = ((value >> 8) & 0xFF) / 255.0f;
        g_texture_env_color[2] = (value & 0xFF) / 255.0f;
        break;
    case GFX_COLORFORMAT_RGBA:
        g_texture_env_color[0] = ((value >> 24) & 0xFF) / 255.0f;
        g_texture_env_color[1] = ((value >> 16) & 0xFF) / 255.0f;
        g_texture_env_color[2] = ((value >> 8) & 0xFF) / 255.0f;
        g_texture_env_color[3] = (value & 0xFF) / 255.0f;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxConstantColorValue: unknown color format : %x", lfb_color_fmt);
    }

    vbo_draw();

    constant_color_location = glGetUniformLocation(g_program_object_default, "constant_color");
    glUniform4f(constant_color_location, g_texture_env_color[0], g_texture_env_color[1], g_texture_env_color[2], g_texture_env_color[3]);
}

void writeGLSLColorOther(int other)
{
    switch (other)
    {
    case GFX_COMBINE_OTHER_ITERATED:
        strcat(fragment_shader_color_combiner, "vec4 color_other = vFrontColor; \n");
        break;
    case GFX_COMBINE_OTHER_TEXTURE:
        strcat(fragment_shader_color_combiner, "vec4 color_other = ctexture1; \n");
        break;
    case GFX_COMBINE_OTHER_CONSTANT:
        strcat(fragment_shader_color_combiner, "vec4 color_other = constant_color; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLColorOther : %x", other);
    }
}

void writeGLSLColorLocal(int local)
{
    switch (local)
    {
    case GFX_COMBINE_LOCAL_ITERATED:
        strcat(fragment_shader_color_combiner, "vec4 color_local = vFrontColor; \n");
        break;
    case GFX_COMBINE_LOCAL_CONSTANT:
        strcat(fragment_shader_color_combiner, "vec4 color_local = constant_color; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLColorLocal : %x", local);
    }
}

void writeGLSLColorFactor(int factor, int local, int need_local, int other, int need_other)
{
    switch (factor)
    {
    case GFX_COMBINE_FACTOR_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(0.0); \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL:
        if (need_local) writeGLSLColorLocal(local);
        strcat(fragment_shader_color_combiner, "vec4 color_factor = color_local; \n");
        break;
    case GFX_COMBINE_FACTOR_OTHER_ALPHA:
        if (need_other) writeGLSLColorOther(other);
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(color_other.a); \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL_ALPHA:
        if (need_local) writeGLSLColorLocal(local);
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(color_local.a); \n");
        break;
    case GFX_COMBINE_FACTOR_TEXTURE_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(ctexture1.a); \n");
        break;
    case GFX_COMBINE_FACTOR_TEXTURE_RGB:
        strcat(fragment_shader_color_combiner, "vec4 color_factor = ctexture1; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE:
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL:
        if (need_local) writeGLSLColorLocal(local);
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - color_local; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
        if (need_other) writeGLSLColorOther(other);
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - vec4(color_other.a); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
        if (need_local) writeGLSLColorLocal(local);
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - vec4(color_local.a); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - vec4(ctexture1.a); \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLColorFactor : %x", factor);
    }
}

void gfxColorCombine(gfxCombineFunction_t function, gfxCombineFactor_t factor, gfxCombineLocal_t local, gfxCombineOther_t other, bool invert)
{
    WriteTrace(TraceGlitch, TraceDebug, "function: %d factor: %d local: %d other: %d invert: %d", function, factor, local, other, invert);
    static int last_function = 0;
    static int last_factor = 0;
    static int last_local = 0;
    static int last_other = 0;

    if (last_function == function && last_factor == factor &&
        last_local == local && last_other == other && first_color == 0 && !c_combiner_ext) return;
    first_color = 0;
    c_combiner_ext = 0;

    last_function = function;
    last_factor = factor;
    last_local = local;
    last_other = other;

    if (invert) WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombine : inverted result");

    color_combiner_key = function | (factor << 4) | (local << 8) | (other << 10);
    chroma_other_color = other;

    strcpy(fragment_shader_color_combiner, "");
    switch (function)
    {
    case GFX_COMBINE_FUNCTION_ZERO:
        strcat(fragment_shader_color_combiner, "gl_FragColor = vec4(0.0); \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL:
        writeGLSLColorLocal(local);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL_ALPHA:
        writeGLSLColorLocal(local);
        strcat(fragment_shader_color_combiner, "gl_FragColor = vec4(color_local.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER:
        writeGLSLColorOther(other);
        writeGLSLColorFactor(factor, local, 1, other, 0);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * color_other; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
        writeGLSLColorLocal(local);
        writeGLSLColorOther(other);
        writeGLSLColorFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * color_other + color_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
        writeGLSLColorLocal(local);
        writeGLSLColorOther(other);
        writeGLSLColorFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * color_other + vec4(color_local.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
        writeGLSLColorLocal(local);
        writeGLSLColorOther(other);
        writeGLSLColorFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (color_other - color_local); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLColorLocal(local);
        writeGLSLColorOther(other);
        writeGLSLColorFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (color_other - color_local) + color_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLColorLocal(local);
        writeGLSLColorOther(other);
        writeGLSLColorFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (color_other - color_local) + vec4(color_local.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLColorLocal(local);
        writeGLSLColorFactor(factor, local, 0, other, 1);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (-color_local) + color_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLColorLocal(local);
        writeGLSLColorFactor(factor, local, 0, other, 1);
        strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (-color_local) + vec4(color_local.a); \n");
        break;
    default:
        strcpy(fragment_shader_color_combiner, g_fragment_shader_default);
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombine : unknown function : %x", function);
    }
    //compile_shader();
    need_to_compile = 1;
}

/*
int setOtherAlphaSource(int other)
{
switch(other)
{
case GFX_COMBINE_OTHER_ITERATED:
return GL_PRIMARY_COLOR_ARB;
break;
case GFX_COMBINE_OTHER_TEXTURE:
return GL_PREVIOUS_ARB;
break;
case GFX_COMBINE_OTHER_CONSTANT:
return GL_CONSTANT_ARB;
break;
default:
WriteTrace(TraceGlitch, TraceWarning, "unknwown other alpha source : %x", other);
}
return 0;
}

int setLocalAlphaSource(int local)
{
switch(local)
{
case GFX_COMBINE_LOCAL_ITERATED:
return GL_PRIMARY_COLOR_ARB;
break;
case GFX_COMBINE_LOCAL_CONSTANT:
return GL_CONSTANT_ARB;
break;
default:
WriteTrace(TraceGlitch, TraceWarning, "unknwown local alpha source : %x", local);
}
return 0;
}
*/

void writeGLSLAlphaOther(int other)
{
    switch (other)
    {
    case GFX_COMBINE_OTHER_ITERATED:
        strcat(fragment_shader_alpha_combiner, "float alpha_other = vFrontColor.a; \n");
        break;
    case GFX_COMBINE_OTHER_TEXTURE:
        strcat(fragment_shader_alpha_combiner, "float alpha_other = ctexture1.a; \n");
        break;
    case GFX_COMBINE_OTHER_CONSTANT:
        strcat(fragment_shader_alpha_combiner, "float alpha_other = constant_color.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLAlphaOther : %x", other);
    }
}

void writeGLSLAlphaLocal(int local)
{
    switch (local)
    {
    case GFX_COMBINE_LOCAL_ITERATED:
        strcat(fragment_shader_alpha_combiner, "float alpha_local = vFrontColor.a; \n");
        break;
    case GFX_COMBINE_LOCAL_CONSTANT:
        strcat(fragment_shader_alpha_combiner, "float alpha_local = constant_color.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLAlphaLocal : %x", local);
    }
}

void writeGLSLAlphaFactor(int factor, int local, int need_local, int other, int need_other)
{
    switch (factor)
    {
    case GFX_COMBINE_FACTOR_ZERO:
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = 0.0; \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL:
        if (need_local) writeGLSLAlphaLocal(local);
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = alpha_local; \n");
        break;
    case GFX_COMBINE_FACTOR_OTHER_ALPHA:
        if (need_other) writeGLSLAlphaOther(other);
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = alpha_other; \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL_ALPHA:
        if (need_local) writeGLSLAlphaLocal(local);
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = alpha_local; \n");
        break;
    case GFX_COMBINE_FACTOR_TEXTURE_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = ctexture1.a; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE:
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL:
        if (need_local) writeGLSLAlphaLocal(local);
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - alpha_local; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
        if (need_other) writeGLSLAlphaOther(other);
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - alpha_other; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
        if (need_local) writeGLSLAlphaLocal(local);
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - alpha_local; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - ctexture1.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLAlphaFactor : %x", factor);
    }
}

void gfxAlphaCombine(gfxCombineFunction_t function, gfxCombineFactor_t factor, gfxCombineLocal_t local, gfxCombineOther_t other, bool invert)
{
    WriteTrace(TraceGlitch, TraceDebug, "function: %d factor: %d local: %d other: %d invert: %d", function, factor, local, other, invert);
    static int last_function = 0;
    static int last_factor = 0;
    static int last_local = 0;
    static int last_other = 0;

    if (last_function == function && last_factor == factor &&
        last_local == local && last_other == other && first_alpha == 0 && !a_combiner_ext) return;
    first_alpha = 0;
    a_combiner_ext = 0;

    last_function = function;
    last_factor = factor;
    last_local = local;
    last_other = other;

    if (invert) WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombine : inverted result");

    alpha_combiner_key = function | (factor << 4) | (local << 8) | (other << 10);
    chroma_other_alpha = other;

    strcpy(fragment_shader_alpha_combiner, "");

    switch (function)
    {
    case GFX_COMBINE_FUNCTION_ZERO:
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = 0.0; \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL:
        writeGLSLAlphaLocal(local);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL_ALPHA:
        writeGLSLAlphaLocal(local);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER:
        writeGLSLAlphaOther(other);
        writeGLSLAlphaFactor(factor, local, 1, other, 0);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * alpha_other; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaOther(other);
        writeGLSLAlphaFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * alpha_other + alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaOther(other);
        writeGLSLAlphaFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * alpha_other + alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaOther(other);
        writeGLSLAlphaFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (alpha_other - alpha_local); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaOther(other);
        writeGLSLAlphaFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (alpha_other - alpha_local) + alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaOther(other);
        writeGLSLAlphaFactor(factor, local, 0, other, 0);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (alpha_other - alpha_local) + alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaFactor(factor, local, 0, other, 1);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (-alpha_local) + alpha_local; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLAlphaLocal(local);
        writeGLSLAlphaFactor(factor, local, 0, other, 1);
        strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (-alpha_local) + alpha_local; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombine : unknown function : %x", function);
    }

    //compile_shader();
    need_to_compile = 1;
}

void writeGLSLTextureColorFactor(int num_tex, int factor)
{
    switch (factor)
    {
    case GFX_COMBINE_FACTOR_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(0.0); \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = readtex1; \n");
        break;
    case GFX_COMBINE_FACTOR_OTHER_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(ctexture0.a); \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(readtex1.a); \n");
        break;
    case GFX_COMBINE_FACTOR_DETAIL_FACTOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(lambda); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(lambda); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - readtex1; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - vec4(ctexture0.a); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - vec4(readtex1.a); \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - vec4(lambda); \n");
        else
            strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - vec4(lambda); \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLTextureColorFactor : %x", factor);
    }
}

void writeGLSLTextureAlphaFactor(int num_tex, int factor)
{
    switch (factor)
    {
    case GFX_COMBINE_FACTOR_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = 0.0; \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = readtex1.a; \n");
        break;
    case GFX_COMBINE_FACTOR_OTHER_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = ctexture0.a; \n");
        break;
    case GFX_COMBINE_FACTOR_LOCAL_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = readtex1.a; \n");
        break;
    case GFX_COMBINE_FACTOR_DETAIL_FACTOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = lambda; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = lambda; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - readtex1.a; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - 0.0; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - ctexture0.a; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - readtex1.a; \n");
        break;
    case GFX_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - lambda; \n");
        else
            strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - lambda; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "unknown writeGLSLTextureAlphaFactor : %x", factor);
    }
}

void gfxTexCombine(gfxChipID_t tmu, gfxCombineFunction_t rgb_function, gfxCombineFactor_t rgb_factor, gfxCombineFunction_t alpha_function, gfxCombineFactor_t alpha_factor, bool rgb_invert, bool alpha_invert)
{
    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d rgb_function: %d rgb_factor: %d alpha_function: %d alpha_factor: %d rgb_invert: %d alpha_invert: %d", tmu, rgb_function, rgb_factor, alpha_function, alpha_factor, rgb_invert, alpha_invert);
    int num_tex;

    if (tmu == GFX_TMU0) num_tex = 1;
    else num_tex = 0;

    if (num_tex == 0)
    {
        static int last_function = 0;
        static int last_factor = 0;
        static int last_afunction = 0;
        static int last_afactor = 0;
        static int last_rgb_invert = 0;

        if (last_function == rgb_function && last_factor == rgb_factor &&
            last_afunction == alpha_function && last_afactor == alpha_factor &&
            last_rgb_invert == rgb_invert && first_texture0 == 0 && !tex0_combiner_ext) return;
        first_texture0 = 0;
        tex0_combiner_ext = 0;

        last_function = rgb_function;
        last_factor = rgb_factor;
        last_afunction = alpha_function;
        last_afactor = alpha_factor;
        last_rgb_invert = rgb_invert;
        texture0_combiner_key = rgb_function | (rgb_factor << 4) |
            (alpha_function << 8) | (alpha_factor << 12) |
            (rgb_invert << 16);
        texture0_combinera_key = 0;
        strcpy(fragment_shader_texture0, "");
    }
    else
    {
        static int last_function = 0;
        static int last_factor = 0;
        static int last_afunction = 0;
        static int last_afactor = 0;
        static int last_rgb_invert = 0;

        if (last_function == rgb_function && last_factor == rgb_factor &&
            last_afunction == alpha_function && last_afactor == alpha_factor &&
            last_rgb_invert == rgb_invert && first_texture1 == 0 && !tex1_combiner_ext) return;
        first_texture1 = 0;
        tex1_combiner_ext = 0;

        last_function = rgb_function;
        last_factor = rgb_factor;
        last_afunction = alpha_function;
        last_afactor = alpha_factor;
        last_rgb_invert = rgb_invert;

        texture1_combiner_key = rgb_function | (rgb_factor << 4) |
            (alpha_function << 8) | (alpha_factor << 12) |
            (rgb_invert << 16);
        texture1_combinera_key = 0;
        strcpy(fragment_shader_texture1, "");
    }

    switch (rgb_function)
    {
    case GFX_COMBINE_FUNCTION_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = vec4(0.0); \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = readtex1; \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = vec4(readtex1.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * ctexture0; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * vec4(0.0) + readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * ctexture0 + readtex1; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * vec4(0.0) + vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * ctexture0 + vec4(readtex1.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (vec4(0.0) - readtex0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (ctexture0 - readtex1); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (vec4(0.0) - readtex0) + readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (ctexture0 - readtex1) + readtex1; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (vec4(0.0) - readtex0) + vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (ctexture0 - readtex1) + vec4(readtex1.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (-readtex0) + readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (-readtex1) + readtex1; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLTextureColorFactor(num_tex, rgb_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (-readtex0) + vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (-readtex1) + vec4(readtex1.a); \n");
        break;
    default:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctexture0 = readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctexture1 = readtex1; \n");
        WriteTrace(TraceGlitch, TraceWarning, "grTextCombine : unknown rgb function : %x", rgb_function);
    }

    if (rgb_invert)
    {
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0 = vec4(1.0) - ctexture0; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1 = vec4(1.0) - ctexture1; \n");
    }

    switch (alpha_function)
    {
    case GFX_COMBINE_FACTOR_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = 0.0; \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_LOCAL_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * ctexture0.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * 0.0 + readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * ctexture0.a + readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * 0.0 + readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * ctexture0.a + readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (0.0 - readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (ctexture0.a - readtex1.a); \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (0.0 - readtex0.a) + readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (ctexture0.a - readtex1.a) + readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (0.0 - readtex0.a) + readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (ctexture0.a - readtex1.a) + readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (-readtex0.a) + readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (-readtex1.a) + readtex1.a; \n");
        break;
    case GFX_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
        writeGLSLTextureAlphaFactor(num_tex, alpha_factor);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (-readtex0.a) + readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (-readtex1.a) + readtex1.a; \n");
        break;
    default:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = ctexture0.a; \n");
        WriteTrace(TraceGlitch, TraceWarning, "grTextCombine : unknown alpha function : %x", alpha_function);
    }

    if (alpha_invert)
    {
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctexture0.a = 1.0 - ctexture0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctexture1.a = 1.0 - ctexture1.a; \n");
    }
    need_to_compile = 1;
}

void gfxAlphaBlendFunction(gfxAlphaBlendFnc_t rgb_sf, gfxAlphaBlendFnc_t rgb_df, gfxAlphaBlendFnc_t alpha_sf, gfxAlphaBlendFnc_t alpha_df)
{
    int sfactorRGB = 0, dfactorRGB = 0, sfactorAlpha = 0, dfactorAlpha = 0;
    WriteTrace(TraceGlitch, TraceDebug, "rgb_sf: %d rgb_df: %d alpha_sf: %d alpha_df: %d", rgb_sf, rgb_df, alpha_sf, alpha_df);

    switch (rgb_sf)
    {
    case GFX_BLEND_ZERO:
        sfactorRGB = GL_ZERO;
        break;
    case GFX_BLEND_SRC_ALPHA:
        sfactorRGB = GL_SRC_ALPHA;
        break;
    case GFX_BLEND_ONE:
        sfactorRGB = GL_ONE;
        break;
    case GFX_BLEND_ONE_MINUS_SRC_ALPHA:
        sfactorRGB = GL_ONE_MINUS_SRC_ALPHA;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaBlendFunction : rgb_sf = %x", rgb_sf);
    }

    switch (rgb_df)
    {
    case GFX_BLEND_ZERO:
        dfactorRGB = GL_ZERO;
        break;
    case GFX_BLEND_SRC_ALPHA:
        dfactorRGB = GL_SRC_ALPHA;
        break;
    case GFX_BLEND_ONE:
        dfactorRGB = GL_ONE;
        break;
    case GFX_BLEND_ONE_MINUS_SRC_ALPHA:
        dfactorRGB = GL_ONE_MINUS_SRC_ALPHA;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaBlendFunction : rgb_df = %x", rgb_df);
    }

    switch (alpha_sf)
    {
    case GFX_BLEND_ZERO:
        sfactorAlpha = GL_ZERO;
        break;
    case GFX_BLEND_ONE:
        sfactorAlpha = GL_ONE;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaBlendFunction : alpha_sf = %x", alpha_sf);
    }

    switch (alpha_df)
    {
    case GFX_BLEND_ZERO:
        dfactorAlpha = GL_ZERO;
        break;
    case GFX_BLEND_ONE:
        dfactorAlpha = GL_ONE;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaBlendFunction : alpha_df = %x", alpha_df);
    }
    glEnable(GL_BLEND);
    glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

void gfxAlphaTestReferenceValue(gfxAlpha_t value)
{
    WriteTrace(TraceGlitch, TraceDebug, "value: %d", value);
    g_alpha_ref = value;
    gfxAlphaTestFunction(g_alpha_func);
}

void gfxAlphaTestFunction(gfxCmpFnc_t function)
{
    WriteTrace(TraceGlitch, TraceDebug, "function: %d", function);
    g_alpha_func = function;
    switch (function)
    {
    case GFX_CMP_GREATER:
        //glAlphaFunc(GL_GREATER, g_alpha_ref/255.0f);
        break;
    case GFX_CMP_GEQUAL:
        //glAlphaFunc(GL_GEQUAL, g_alpha_ref/255.0f);
        break;
    case GFX_CMP_ALWAYS:
        //glAlphaFunc(GL_ALWAYS, g_alpha_ref/255.0f);
        //glDisable(GL_ALPHA_TEST);
        g_alpha_test = false;
        return;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaTestFunction : unknown function : %x", function);
    }
    //glEnable(GL_ALPHA_TEST);
    g_alpha_test = true;
}

// fog

void gfxFogMode(gfxFogMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    switch (mode)
    {
    case GFX_FOG_DISABLE:
        //glDisable(GL_FOG);
        g_fog_enabled = 0;
        break;
    case GFX_FOG_WITH_TABLE_ON_Q:
        //glEnable(GL_FOG);
        //glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
        g_fog_enabled = 1;
        break;
    case GFX_FOG_WITH_TABLE_ON_FOGCOORD_EXT:
        //glEnable(GL_FOG);
        //glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
        g_fog_enabled = 2;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxFogMode : unknown mode : %x", mode);
    }
    need_to_compile = 1;
}

void gfxFogGenerateLinear(float nearZ, float farZ)
{
    WriteTrace(TraceGlitch, TraceDebug, "nearZ: %f farZ: %f", nearZ, farZ);
    fogStart = nearZ / 255.0f;
    fogEnd = farZ / 255.0f;
}

void gfxFogColorValue(gfxColor_t fogcolor)
{
    WriteTrace(TraceGlitch, TraceDebug, "fogcolor: %x", fogcolor);

    switch (lfb_color_fmt)
    {
    case GFX_COLORFORMAT_ARGB:
        fogColor[3] = ((fogcolor >> 24) & 0xFF) / 255.0f;
        fogColor[0] = ((fogcolor >> 16) & 0xFF) / 255.0f;
        fogColor[1] = ((fogcolor >> 8) & 0xFF) / 255.0f;
        fogColor[2] = (fogcolor & 0xFF) / 255.0f;
        break;
    case GFX_COLORFORMAT_RGBA:
        fogColor[0] = ((fogcolor >> 24) & 0xFF) / 255.0f;
        fogColor[1] = ((fogcolor >> 16) & 0xFF) / 255.0f;
        fogColor[2] = ((fogcolor >> 8) & 0xFF) / 255.0f;
        fogColor[3] = (fogcolor & 0xFF) / 255.0f;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxFogColorValue: unknown color format : %x", lfb_color_fmt);
    }
}

// chroma
void gfxChromakeyMode(gfxChromakeyMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    switch (mode)
    {
    case GFX_CHROMAKEY_DISABLE:
        g_chroma_enabled = false;
        break;
    case GFX_CHROMAKEY_ENABLE:
        g_chroma_enabled = true;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxChromakeyMode : unknown mode : %x", mode);
    }
    need_to_compile = 1;
}

void gfxChromakeyValue(gfxColor_t value)
{
    WriteTrace(TraceGlitch, TraceDebug, "value: %d", value);
    int chroma_color_location;

    switch (lfb_color_fmt)
    {
    case GFX_COLORFORMAT_ARGB:
        g_chroma_color[3] = 1.0;//((value >> 24) & 0xFF) / 255.0f;
        g_chroma_color[0] = ((value >> 16) & 0xFF) / 255.0f;
        g_chroma_color[1] = ((value >> 8) & 0xFF) / 255.0f;
        g_chroma_color[2] = (value & 0xFF) / 255.0f;
        break;
    case GFX_COLORFORMAT_RGBA:
        g_chroma_color[0] = ((value >> 24) & 0xFF) / 255.0f;
        g_chroma_color[1] = ((value >> 16) & 0xFF) / 255.0f;
        g_chroma_color[2] = ((value >> 8) & 0xFF) / 255.0f;
        g_chroma_color[3] = 1.0;//(value & 0xFF) / 255.0f;
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxChromakeyValue: unknown color format : %x", lfb_color_fmt);
    }
    vbo_draw();
    chroma_color_location = glGetUniformLocation(g_program_object_default, "chroma_color");
    glUniform4f(chroma_color_location, g_chroma_color[0], g_chroma_color[1],
        g_chroma_color[2], g_chroma_color[3]);
}

void setPattern()
{
    int i;
    GLubyte stip[32 * 4];
    for (i = 0; i < 32; i++)
    {
        unsigned int val = (rand() << 17) | ((rand() & 1) << 16) | (rand() << 1) | (rand() & 1);
        stip[i * 4 + 0] = (val >> 24) & 0xFF;
        stip[i * 4 + 1] = (val >> 16) & 0xFF;
        stip[i * 4 + 2] = (val >> 8) & 0xFF;
        stip[i * 4 + 3] = val & 0xFF;
    }
    GLubyte texture[32 * 32 * 4];
    for (i = 0; i < 32; i++)
    {
        int j;
        for (j = 0; j < 4; j++)
        {
            texture[(i * 32 + j * 8 + 0) * 4 + 3] = ((stip[i * 4 + j] >> 7) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 1) * 4 + 3] = ((stip[i * 4 + j] >> 6) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 2) * 4 + 3] = ((stip[i * 4 + j] >> 5) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 3) * 4 + 3] = ((stip[i * 4 + j] >> 4) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 4) * 4 + 3] = ((stip[i * 4 + j] >> 3) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 5) * 4 + 3] = ((stip[i * 4 + j] >> 2) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 6) * 4 + 3] = ((stip[i * 4 + j] >> 1) & 1) ? 255 : 0;
            texture[(i * 32 + j * 8 + 7) * 4 + 3] = ((stip[i * 4 + j] >> 0) & 1) ? 255 : 0;
        }
    }
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 33 * 1024 * 1024);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void gfxStippleMode(gfxStippleMode_t mode)
{
    WriteTrace(TraceGlitch, TraceDebug, "mode: %d", mode);
    switch (mode)
    {
    case GFX_STIPPLE_DISABLE:
        dither_enabled = 0;
        glActiveTexture(GL_TEXTURE2);
        break;
    case GFX_STIPPLE_PATTERN:
        setPattern();
        dither_enabled = 1;
        glActiveTexture(GL_TEXTURE2);
        break;
    case GFX_STIPPLE_ROTATE:
        setPattern();
        dither_enabled = 1;
        glActiveTexture(GL_TEXTURE2);
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxStippleMode:%x", mode);
    }
    need_to_compile = 1;
}

void gfxColorCombineExt(gfxCCUColor_t a, gfxCombineMode_t a_mode, gfxCCUColor_t b, gfxCombineMode_t b_mode, gfxCCUColor_t c, bool c_invert, gfxCCUColor_t d, bool d_invert, uint32_t shift, bool invert)
{
    WriteTrace(TraceGlitch, TraceDebug, "a: %d a_mode: %d b: %d b_mode: %d c: %d c_invert: %d d: %d d_invert: %d shift: %d invert: %d", a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);
    if (invert) WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : inverted result");
    if (shift) WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : shift = %d", shift);

    color_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
        ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
        ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
        ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
    c_combiner_ext = 1;
    strcpy(fragment_shader_color_combiner, "");

    switch (a)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(0.0); \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(ctexture1.a); \n");
        break;
    case GFX_CMBX_CONSTANT_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(constant_color.a); \n");
        break;
    case GFX_CMBX_CONSTANT_COLOR:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = constant_color; \n");
        break;
    case GFX_CMBX_ITALPHA:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(vFrontColor.a); \n");
        break;
    case GFX_CMBX_ITRGB:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = vFrontColor; \n");
        break;
    case GFX_CMBX_TEXTURE_RGB:
        strcat(fragment_shader_color_combiner, "vec4 cs_a = ctexture1; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : a = %x", a);
        strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(0.0); \n");
    }

    switch (a_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 c_a = vec4(0.0); \n");
        break;
    case GFX_FUNC_MODE_X:
        strcat(fragment_shader_color_combiner, "vec4 c_a = cs_a; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        strcat(fragment_shader_color_combiner, "vec4 c_a = vec4(1.0) - cs_a; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        strcat(fragment_shader_color_combiner, "vec4 c_a = -cs_a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : a_mode = %x", a_mode);
        strcat(fragment_shader_color_combiner, "vec4 c_a = vec4(0.0); \n");
    }

    switch (b)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(0.0); \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(ctexture1.a); \n");
        break;
    case GFX_CMBX_CONSTANT_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(constant_color.a); \n");
        break;
    case GFX_CMBX_CONSTANT_COLOR:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = constant_color; \n");
        break;
    case GFX_CMBX_ITALPHA:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(vFrontColor.a); \n");
        break;
    case GFX_CMBX_ITRGB:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = vFrontColor; \n");
        break;
    case GFX_CMBX_TEXTURE_RGB:
        strcat(fragment_shader_color_combiner, "vec4 cs_b = ctexture1; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : b = %x", b);
        strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(0.0); \n");
    }

    switch (b_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 c_b = vec4(0.0); \n");
        break;
    case GFX_FUNC_MODE_X:
        strcat(fragment_shader_color_combiner, "vec4 c_b = cs_b; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        strcat(fragment_shader_color_combiner, "vec4 c_b = vec4(1.0) - cs_b; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        strcat(fragment_shader_color_combiner, "vec4 c_b = -cs_b; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : b_mode = %x", b_mode);
        strcat(fragment_shader_color_combiner, "vec4 c_b = vec4(0.0); \n");
    }

    switch (c)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(0.0); \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(ctexture1.a); \n");
        break;
    case GFX_CMBX_ALOCAL:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(c_b.a); \n");
        break;
    case GFX_CMBX_AOTHER:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(c_a.a); \n");
        break;
    case GFX_CMBX_B:
        strcat(fragment_shader_color_combiner, "vec4 c_c = cs_b; \n");
        break;
    case GFX_CMBX_CONSTANT_ALPHA:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(constant_color.a); \n");
        break;
    case GFX_CMBX_CONSTANT_COLOR:
        strcat(fragment_shader_color_combiner, "vec4 c_c = constant_color; \n");
        break;
    case GFX_CMBX_ITALPHA:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(vFrontColor.a); \n");
        break;
    case GFX_CMBX_ITRGB:
        strcat(fragment_shader_color_combiner, "vec4 c_c = vFrontColor; \n");
        break;
    case GFX_CMBX_TEXTURE_RGB:
        strcat(fragment_shader_color_combiner, "vec4 c_c = ctexture1; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : c = %x", c);
        strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(0.0); \n");
    }

    if (c_invert)
        strcat(fragment_shader_color_combiner, "c_c = vec4(1.0) - c_c; \n");

    switch (d)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_color_combiner, "vec4 c_d = vec4(0.0); \n");
        break;
    case GFX_CMBX_ALOCAL:
        strcat(fragment_shader_color_combiner, "vec4 c_d = vec4(c_b.a); \n");
        break;
    case GFX_CMBX_B:
        strcat(fragment_shader_color_combiner, "vec4 c_d = cs_b; \n");
        break;
    case GFX_CMBX_TEXTURE_RGB:
        strcat(fragment_shader_color_combiner, "vec4 c_d = ctexture1; \n");
        break;
    case GFX_CMBX_ITRGB:
        strcat(fragment_shader_color_combiner, "vec4 c_d = vFrontColor; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxColorCombineExt : d = %x", d);
        strcat(fragment_shader_color_combiner, "vec4 c_d = vec4(0.0); \n");
    }

    if (d_invert)
        strcat(fragment_shader_color_combiner, "c_d = vec4(1.0) - c_d; \n");

    strcat(fragment_shader_color_combiner, "gl_FragColor = (c_a + c_b) * c_c + c_d; \n");

    need_to_compile = 1;
}

void gfxAlphaCombineExt(gfxACUColor_t a, gfxCombineMode_t a_mode, gfxACUColor_t b, gfxCombineMode_t b_mode, gfxACUColor_t c, bool c_invert, gfxACUColor_t d, bool d_invert, uint32_t shift, bool invert)
{
    WriteTrace(TraceGlitch, TraceDebug, "a: %d a_mode: %d b: %d b_mode: %d c: %d c_invert: %d d: %d d_invert: %d shift: %d invert: %d", a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);
    if (invert) WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : inverted result");
    if (shift) WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : shift = %d", shift);

    alpha_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
        ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
        ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
        ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
    a_combiner_ext = 1;
    strcpy(fragment_shader_alpha_combiner, "");

    switch (a)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_alpha_combiner, "float as_a = 0.0; \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float as_a = ctexture1.a; \n");
        break;
    case GFX_CMBX_CONSTANT_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float as_a = constant_color.a; \n");
        break;
    case GFX_CMBX_ITALPHA:
        strcat(fragment_shader_alpha_combiner, "float as_a = vFrontColor.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : a = %x", a);
        strcat(fragment_shader_alpha_combiner, "float as_a = 0.0; \n");
    }

    switch (a_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        strcat(fragment_shader_alpha_combiner, "float a_a = 0.0; \n");
        break;
    case GFX_FUNC_MODE_X:
        strcat(fragment_shader_alpha_combiner, "float a_a = as_a; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        strcat(fragment_shader_alpha_combiner, "float a_a = 1.0 - as_a; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        strcat(fragment_shader_alpha_combiner, "float a_a = -as_a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : a_mode = %x", a_mode);
        strcat(fragment_shader_alpha_combiner, "float a_a = 0.0; \n");
    }

    switch (b)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_alpha_combiner, "float as_b = 0.0; \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float as_b = ctexture1.a; \n");
        break;
    case GFX_CMBX_CONSTANT_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float as_b = constant_color.a; \n");
        break;
    case GFX_CMBX_ITALPHA:
        strcat(fragment_shader_alpha_combiner, "float as_b = vFrontColor.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : b = %x", b);
        strcat(fragment_shader_alpha_combiner, "float as_b = 0.0; \n");
    }

    switch (b_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        strcat(fragment_shader_alpha_combiner, "float a_b = 0.0; \n");
        break;
    case GFX_FUNC_MODE_X:
        strcat(fragment_shader_alpha_combiner, "float a_b = as_b; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        strcat(fragment_shader_alpha_combiner, "float a_b = 1.0 - as_b; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        strcat(fragment_shader_alpha_combiner, "float a_b = -as_b; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : b_mode = %x", b_mode);
        strcat(fragment_shader_alpha_combiner, "float a_b = 0.0; \n");
    }

    switch (c)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_alpha_combiner, "float a_c = 0.0; \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float a_c = ctexture1.a; \n");
        break;
    case GFX_CMBX_ALOCAL:
        strcat(fragment_shader_alpha_combiner, "float a_c = as_b; \n");
        break;
    case GFX_CMBX_AOTHER:
        strcat(fragment_shader_alpha_combiner, "float a_c = as_a; \n");
        break;
    case GFX_CMBX_B:
        strcat(fragment_shader_alpha_combiner, "float a_c = as_b; \n");
        break;
    case GFX_CMBX_CONSTANT_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float a_c = constant_color.a; \n");
        break;
    case GFX_CMBX_ITALPHA:
        strcat(fragment_shader_alpha_combiner, "float a_c = vFrontColor.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : c = %x", c);
        strcat(fragment_shader_alpha_combiner, "float a_c = 0.0; \n");
    }

    if (c_invert)
        strcat(fragment_shader_alpha_combiner, "a_c = 1.0 - a_c; \n");

    switch (d)
    {
    case GFX_CMBX_ZERO:
        strcat(fragment_shader_alpha_combiner, "float a_d = 0.0; \n");
        break;
    case GFX_CMBX_TEXTURE_ALPHA:
        strcat(fragment_shader_alpha_combiner, "float a_d = ctexture1.a; \n");
        break;
    case GFX_CMBX_ALOCAL:
        strcat(fragment_shader_alpha_combiner, "float a_d = as_b; \n");
        break;
    case GFX_CMBX_B:
        strcat(fragment_shader_alpha_combiner, "float a_d = as_b; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxAlphaCombineExt : d = %x", d);
        strcat(fragment_shader_alpha_combiner, "float a_d = 0.0; \n");
    }

    if (d_invert)
        strcat(fragment_shader_alpha_combiner, "a_d = 1.0 - a_d; \n");

    strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = (a_a + a_b) * a_c + a_d; \n");

    need_to_compile = 1;
}

void gfxTexColorCombineExt(gfxChipID_t tmu, gfxTCCUColor_t a, gfxCombineMode_t a_mode, gfxTCCUColor_t b, gfxCombineMode_t b_mode, gfxTCCUColor_t c, bool c_invert, gfxTCCUColor_t d, bool d_invert, uint32_t shift, bool invert)
{
    int num_tex;
    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d a: %d a_mode: %d b: %d b_mode: %d c: %d c_invert: %d d: %d d_invert: %d shift: %d invert: %d", tmu, a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);

    if (invert) WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : inverted result");
    if (shift) WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : shift = %d", shift);

    if (tmu == GFX_TMU0) num_tex = 1;
    else num_tex = 0;

    if (num_tex == 0)
    {
        texture0_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
            ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
            ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
            ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
        tex0_combiner_ext = 1;
        strcpy(fragment_shader_texture0, "");
    }
    else
    {
        texture1_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
            ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
            ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
            ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
        tex1_combiner_ext = 1;
        strcpy(fragment_shader_texture1, "");
    }

    switch (a)
    {
    case GFX_CMBX_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(0.0); \n");
        break;
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(vFrontColor.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(vFrontColor.a); \n");
        break;
    case GFX_CMBX_ITRGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vFrontColor; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vFrontColor; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(readtex1.a); \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_RGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = readtex1; \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(ctexture0.a); \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_RGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = ctexture0; \n");
        break;
    case GFX_CMBX_TMU_CCOLOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = ccolor0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = ccolor1; \n");
        break;
    case GFX_CMBX_TMU_CALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(ccolor0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(ccolor1.a); \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : a = %x", a);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(0.0); \n");
    }

    switch (a_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_a = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_a = vec4(0.0); \n");
        break;
    case GFX_FUNC_MODE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_a = ctex0s_a; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_a = ctex1s_a; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_a = vec4(1.0) - ctex0s_a; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_a = vec4(1.0) - ctex1s_a; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_a = -ctex0s_a; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_a = -ctex1s_a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : a_mode = %x", a_mode);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_a = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_a = vec4(0.0); \n");
    }

    switch (b)
    {
    case GFX_CMBX_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(0.0); \n");
        break;
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(vFrontColor.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(vFrontColor.a); \n");
        break;
    case GFX_CMBX_ITRGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vFrontColor; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vFrontColor; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(readtex1.a); \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_RGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = readtex1; \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(ctexture0.a); \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_RGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = ctexture0; \n");
        break;
    case GFX_CMBX_TMU_CALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(ccolor0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(ccolor1.a); \n");
        break;
    case GFX_CMBX_TMU_CCOLOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = ccolor0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = ccolor1; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : b = %x", b);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(0.0); \n");
    }

    switch (b_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_b = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_b = vec4(0.0); \n");
        break;
    case GFX_FUNC_MODE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_b = ctex0s_b; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_b = ctex1s_b; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_b = vec4(1.0) - ctex0s_b; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_b = vec4(1.0) - ctex1s_b; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_b = -ctex0s_b; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_b = -ctex1s_b; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : b_mode = %x", b_mode);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_b = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_b = vec4(0.0); \n");
    }

    switch (c)
    {
    case GFX_CMBX_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(0.0); \n");
        break;
    case GFX_CMBX_B:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = ctex0s_b; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = ctex1s_b; \n");
        break;
    case GFX_CMBX_DETAIL_FACTOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(lambda); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(lambda); \n");
        break;
    case GFX_CMBX_ITRGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vFrontColor; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vFrontColor; \n");
        break;
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(vFrontColor.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(vFrontColor.a); \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(readtex1.a); \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_RGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = readtex0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = readtex1; \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(ctexture0.a); \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_RGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = ctexture0; \n");
        break;
    case GFX_CMBX_TMU_CALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(ccolor0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(ccolor1.a); \n");
        break;
    case GFX_CMBX_TMU_CCOLOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = ccolor0; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = ccolor1; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : c = %x", c);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(0.0); \n");
    }

    if (c_invert)
    {
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c = vec4(1.0) - ctex0_c; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c = vec4(1.0) - ctex1_c; \n");
    }

    switch (d)
    {
    case GFX_CMBX_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vec4(0.0); \n");
        break;
    case GFX_CMBX_B:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_d = ctex0s_b; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_d = ctex1s_b; \n");
        break;
    case GFX_CMBX_ITRGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vFrontColor; \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vFrontColor; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vec4(readtex0.a); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vec4(readtex1.a); \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexColorCombineExt : d = %x", d);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vec4(0.0); \n");
        else
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vec4(0.0); \n");
    }

    if (d_invert)
    {
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d = vec4(1.0) - ctex0_d; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d = vec4(1.0) - ctex1_d; \n");
    }

    if (num_tex == 0)
        strcat(fragment_shader_texture0, "vec4 ctexture0 = (ctex0_a + ctex0_b) * ctex0_c + ctex0_d; \n");
    else
        strcat(fragment_shader_texture1, "vec4 ctexture1 = (ctex1_a + ctex1_b) * ctex1_c + ctex1_d; \n");
    need_to_compile = 1;
}

void gfxTexAlphaCombineExt(gfxChipID_t tmu, gfxTACUColor_t a, gfxCombineMode_t a_mode, gfxTACUColor_t b, gfxCombineMode_t b_mode, gfxTACUColor_t c, bool c_invert, gfxTACUColor_t d, bool d_invert, uint32_t shift, bool invert)
{
    int num_tex;
    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d a: %d a_mode: %d b: %d b_mode: %d c: %d c_invert: %d d: %d d_invert: %d shift, invert: %d", tmu, a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);

    if (invert) WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : inverted result");
    if (shift) WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : shift = %d", shift);

    if (tmu == GFX_TMU0) num_tex = 1;
    else num_tex = 0;

    if (num_tex == 0)
    {
        texture0_combinera_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
            ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
            ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
            ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
    }
    else
    {
        texture1_combinera_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
            ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
            ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
            ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
    }

    switch (a)
    {
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_a.a = vFrontColor.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_a.a = vFrontColor.a; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_a.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_a.a = readtex1.a; \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_a.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_a.a = ctexture0.a; \n");
        break;
    case GFX_CMBX_TMU_CALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_a.a = ccolor0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_a.a = ccolor1.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : a = %x", a);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_a.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_a.a = 0.0; \n");
    }

    switch (a_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_a.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_a.a = 0.0; \n");
        break;
    case GFX_FUNC_MODE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_a.a = ctex0s_a.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_a.a = ctex1s_a.a; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_a.a = 1.0 - ctex0s_a.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_a.a = 1.0 - ctex1s_a.a; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_a.a = -ctex0s_a.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_a.a = -ctex1s_a.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : a_mode = %x", a_mode);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_a.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_a.a = 0.0; \n");
    }

    switch (b)
    {
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_b.a = vFrontColor.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_b.a = vFrontColor.a; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_b.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_b.a = readtex1.a; \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_b.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_b.a = ctexture0.a; \n");
        break;
    case GFX_CMBX_TMU_CALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_b.a = ccolor0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_b.a = ccolor1.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : b = %x", b);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0s_b.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1s_b.a = 0.0; \n");
    }

    switch (b_mode)
    {
    case GFX_FUNC_MODE_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_b.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_b.a = 0.0; \n");
        break;
    case GFX_FUNC_MODE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_b.a = ctex0s_b.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_b.a = ctex1s_b.a; \n");
        break;
    case GFX_FUNC_MODE_ONE_MINUS_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_b.a = 1.0 - ctex0s_b.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_b.a = 1.0 - ctex1s_b.a; \n");
        break;
    case GFX_FUNC_MODE_NEGATIVE_X:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_b.a = -ctex0s_b.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_b.a = -ctex1s_b.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : b_mode = %x", b_mode);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_b.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_b.a = 0.0; \n");
    }

    switch (c)
    {
    case GFX_CMBX_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = 0.0; \n");
        break;
    case GFX_CMBX_B:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = ctex0s_b.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = ctex1s_b.a; \n");
        break;
    case GFX_CMBX_DETAIL_FACTOR:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = lambda; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = lambda; \n");
        break;
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = vFrontColor.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = vFrontColor.a; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = readtex1.a; \n");
        break;
    case GFX_CMBX_OTHER_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = ctexture0.a; \n");
        break;
    case GFX_CMBX_TMU_CALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = ccolor0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = ccolor1.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : c = %x", c);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = 0.0; \n");
    }

    if (c_invert)
    {
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_c.a = 1.0 - ctex0_c.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_c.a = 1.0 - ctex1_c.a; \n");
    }

    switch (d)
    {
    case GFX_CMBX_ZERO:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = 0.0; \n");
        break;
    case GFX_CMBX_B:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = ctex0s_b.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = ctex1s_b.a; \n");
        break;
    case GFX_CMBX_ITALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = vFrontColor.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = vFrontColor.a; \n");
        break;
    case GFX_CMBX_ITRGB:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = vFrontColor.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = vFrontColor.a; \n");
        break;
    case GFX_CMBX_LOCAL_TEXTURE_ALPHA:
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = readtex0.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = readtex1.a; \n");
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxTexAlphaCombineExt : d = %x", d);
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = 0.0; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = 0.0; \n");
    }

    if (d_invert)
    {
        if (num_tex == 0)
            strcat(fragment_shader_texture0, "ctex0_d.a = 1.0 - ctex0_d.a; \n");
        else
            strcat(fragment_shader_texture1, "ctex1_d.a = 1.0 - ctex1_d.a; \n");
    }

    if (num_tex == 0)
        strcat(fragment_shader_texture0, "ctexture0.a = (ctex0_a.a + ctex0_b.a) * ctex0_c.a + ctex0_d.a; \n");
    else
        strcat(fragment_shader_texture1, "ctexture1.a = (ctex1_a.a + ctex1_b.a) * ctex1_c.a + ctex1_d.a; \n");

    need_to_compile = 1;
}

void gfxConstantColorValueExt(gfxChipID_t tmu, gfxColor_t value)
{
    int num_tex;
    WriteTrace(TraceGlitch, TraceDebug, "tmu: %d value: %d", tmu, value);

    if (tmu == GFX_TMU0) num_tex = 1;
    else num_tex = 0;

    switch (lfb_color_fmt)
    {
    case GFX_COLORFORMAT_ARGB:
        if (num_tex == 0)
        {
            g_ccolor0[3] = ((value >> 24) & 0xFF) / 255.0f;
            g_ccolor0[0] = ((value >> 16) & 0xFF) / 255.0f;
            g_ccolor0[1] = ((value >> 8) & 0xFF) / 255.0f;
            g_ccolor0[2] = (value & 0xFF) / 255.0f;
        }
        else
        {
            g_ccolor1[3] = ((value >> 24) & 0xFF) / 255.0f;
            g_ccolor1[0] = ((value >> 16) & 0xFF) / 255.0f;
            g_ccolor1[1] = ((value >> 8) & 0xFF) / 255.0f;
            g_ccolor1[2] = (value & 0xFF) / 255.0f;
        }
        break;
    case GFX_COLORFORMAT_RGBA:
        if (num_tex == 0)
        {
            g_ccolor0[0] = ((value >> 24) & 0xFF) / 255.0f;
            g_ccolor0[1] = ((value >> 16) & 0xFF) / 255.0f;
            g_ccolor0[2] = ((value >> 8) & 0xFF) / 255.0f;
            g_ccolor0[3] = (value & 0xFF) / 255.0f;
        }
        else
        {
            g_ccolor1[0] = ((value >> 24) & 0xFF) / 255.0f;
            g_ccolor1[1] = ((value >> 16) & 0xFF) / 255.0f;
            g_ccolor1[2] = ((value >> 8) & 0xFF) / 255.0f;
            g_ccolor1[3] = (value & 0xFF) / 255.0f;
        }
        break;
    default:
        WriteTrace(TraceGlitch, TraceWarning, "gfxConstantColorValue: unknown color format : %x", lfb_color_fmt);
    }

    vbo_draw();
    if (num_tex == 0)
    {
        ccolor0_location = glGetUniformLocation(g_program_object_default, "ccolor0");
        glUniform4f(ccolor0_location, g_ccolor0[0], g_ccolor0[1], g_ccolor0[2], g_ccolor0[3]);
    }
    else
    {
        ccolor1_location = glGetUniformLocation(g_program_object_default, "ccolor1");
        glUniform4f(ccolor1_location, g_ccolor1[0], g_ccolor1[1], g_ccolor1[2], g_ccolor1[3]);
    }
}