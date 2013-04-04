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

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include "Gfx #1.3.h"
#include "3dmath.h"

void calc_light (VERTEX *v)
{
  float light_intensity = 0.0f;
  register float color[3] = {rdp.light[rdp.num_lights].r, rdp.light[rdp.num_lights].g, rdp.light[rdp.num_lights].b};
  for (wxUint32 l=0; l<rdp.num_lights; l++)
  {
    light_intensity = DotProduct (rdp.light_vector[l], v->vec);
    
    if (light_intensity > 0.0f) 
    {
      color[0] += rdp.light[l].r * light_intensity;
      color[1] += rdp.light[l].g * light_intensity;
      color[2] += rdp.light[l].b * light_intensity;
    }
  }
  
  if (color[0] > 1.0f) color[0] = 1.0f;
  if (color[1] > 1.0f) color[1] = 1.0f;
  if (color[2] > 1.0f) color[2] = 1.0f;
  
  v->r = (wxUint8)(color[0]*255.0f);
  v->g = (wxUint8)(color[1]*255.0f);
  v->b = (wxUint8)(color[2]*255.0f);
}

//*
void calc_linear (VERTEX *v)
{
  if (settings.force_calc_sphere)
  {
    calc_sphere(v);
    return;
  }
  DECLAREALIGN16VAR(vec[3]);
  
  TransformVector (v->vec, vec, rdp.model);
  //    TransformVector (v->vec, vec, rdp.combined);
  NormalizeVector (vec);
  float x, y;
  if (!rdp.use_lookat)
  {
    x = vec[0];
    y = vec[1];
  }
  else
  {
    x = DotProduct (rdp.lookat[0], vec);
    y = DotProduct (rdp.lookat[1], vec);
  }
  
  if (x > 1.0f)
    x = 1.0f;
  else if (x < -1.0f)
    x = -1.0f;
  if (y > 1.0f)
    y = 1.0f;
  else if (y < -1.0f)
    y = -1.0f;
  
  if (rdp.cur_cache[0])
  {
    // scale >> 6 is size to map to
    v->ou = (acosf(x)/3.141592654f) * (rdp.tiles[rdp.cur_tile].org_s_scale >> 6);
    v->ov = (acosf(y)/3.141592654f) * (rdp.tiles[rdp.cur_tile].org_t_scale >> 6);
  }
  v->uv_scaled = 1;
#ifdef EXTREME_LOGGING
  FRDP ("calc linear u: %f, v: %f\n", v->ou, v->ov);
#endif
}

void calc_sphere (VERTEX *v)
{
//  LRDP("calc_sphere\n");
  DECLAREALIGN16VAR(vec[3]);
  int s_scale, t_scale;
  if (settings.hacks&hack_Chopper)
  {
    s_scale = min(rdp.tiles[rdp.cur_tile].org_s_scale >> 6, rdp.tiles[rdp.cur_tile].lr_s);
    t_scale = min(rdp.tiles[rdp.cur_tile].org_t_scale >> 6, rdp.tiles[rdp.cur_tile].lr_t);
  }
  else
  {
    s_scale = rdp.tiles[rdp.cur_tile].org_s_scale >> 6;
    t_scale = rdp.tiles[rdp.cur_tile].org_t_scale >> 6;
  }
  TransformVector (v->vec, vec, rdp.model);
  //    TransformVector (v->vec, vec, rdp.combined);
  NormalizeVector (vec);
  float x, y;
  if (!rdp.use_lookat)
  {
    x = vec[0];
    y = vec[1];
  }
  else
  {
    x = DotProduct (rdp.lookat[0], vec);
    y = DotProduct (rdp.lookat[1], vec);
  }
  v->ou = (x * 0.5f + 0.5f) * s_scale;
  v->ov = (y * 0.5f + 0.5f) * t_scale;
  v->uv_scaled = 1;
#ifdef EXTREME_LOGGING
  FRDP ("calc sphere u: %f, v: %f\n", v->ou, v->ov);
#endif
}

float DotProductC(register float *v1, register float *v2)
{
    register float result;
    result = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    return(result);
}

void NormalizeVectorC(float *v)
{
    register float len;
    len = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len > 0.0f)
    {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

void TransformVectorC(float *src, float *dst, float mat[4][4])
{
  dst[0] = mat[0][0]*src[0] + mat[1][0]*src[1] + mat[2][0]*src[2];
  dst[1] = mat[0][1]*src[0] + mat[1][1]*src[1] + mat[2][1]*src[2];
  dst[2] = mat[0][2]*src[0] + mat[1][2]*src[1] + mat[2][2]*src[2];
}

void InverseTransformVectorC (float *src, float *dst, float mat[4][4])
{
  dst[0] = mat[0][0]*src[0] + mat[0][1]*src[1] + mat[0][2]*src[2];
  dst[1] = mat[1][0]*src[0] + mat[1][1]*src[1] + mat[1][2]*src[2];
  dst[2] = mat[2][0]*src[0] + mat[2][1]*src[1] + mat[2][2]*src[2];
}

void MulMatricesC(float m1[4][4],float m2[4][4],float r[4][4])
{
  for (int i=0; i<4; i++)
  {
    for (int j=0; j<4; j++)
    {
      r[i][j] = m1[i][0] * m2[0][j] +
                m1[i][1] * m2[1][j] +
                m1[i][2] * m2[2][j] +
                m1[i][3] * m2[3][j];
    }
  }
}

// 2008.03.29 H.Morii - added SSE 3DNOW! 3x3 1x3 matrix multiplication
//                      and 3DNOW! 4x4 4x4 matrix multiplication
MULMATRIX MulMatrices = MulMatricesC;
TRANSFORMVECTOR TransformVector = TransformVectorC;
TRANSFORMVECTOR InverseTransformVector = InverseTransformVectorC;
DOTPRODUCT DotProduct = DotProductC;
NORMALIZEVECTOR NormalizeVector = NormalizeVectorC;

extern "C" void  TransformVectorSSE(float *src, float *dst, float mat[4][4]);
extern "C" void  TransformVector3DNOW(float *src, float *dst, float mat[4][4]);
extern "C" void  InverseTransformVector3DNOW(float *src, float *dst, float mat[4][4]);
extern "C" void  MulMatricesSSE(float m1[4][4],float m2[4][4],float r[4][4]);
extern "C" void  MulMatrices3DNOW(float m1[4][4],float m2[4][4],float r[4][4]);
extern "C" float DotProductSSE3(register float *v1, register float *v2);
extern "C" float DotProduct3DNOW(register float *v1, register float *v2);
extern "C" void NormalizeVectorSSE(float *v);
extern "C" void NormalizeVector3DNOW(float *v);

extern "C" void DetectSIMD(int function, int * iedx, int * iecx);

void math_init()
{
#ifndef _DEBUG
  int iecx = 0, iedx = 0;

  GLIDE64_TRY
  {
    DetectSIMD(0x0000001, &iedx, &iecx);
  }
  GLIDE64_CATCH
  {
    return;
  }
  if (iedx & 0x2000000) //SSE
  {
    MulMatrices = MulMatricesSSE;
    TransformVector = TransformVectorSSE;
    //InverseTransformVector = InverseTransformVectorSSE;
    //NormalizeVector = NormalizeVectorSSE; /* not ready yet */
    LOG("SSE detected.\n");
  }
  if (iedx & 0x4000000) // SSE2
  {
    LOG("SSE2 detected.\n");
  }
  if (iecx & 0x1) // SSE3
  {
    //DotProduct = DotProductSSE3; /* not ready yet */
    LOG("SSE3 detected.\n");
  }
  // the 3dnow version is faster than sse
  iecx = 0;
  iedx = 0;
  GLIDE64_TRY
  {
    DetectSIMD(0x80000001, &iedx, &iecx);
  }
  GLIDE64_CATCH
  {
    return;
  }
  if (iedx & 0x80000000) //3DNow!
  {
    MulMatrices = MulMatrices3DNOW;
    TransformVector = TransformVector3DNOW;
    InverseTransformVector = InverseTransformVector3DNOW;
    //DotProduct = DotProduct3DNOW;  //not ready yet 
    NormalizeVector = NormalizeVector3DNOW; // not ready yet 
    LOG("3DNOW! detected.\n");
  }
#endif //_DEBUG
}
