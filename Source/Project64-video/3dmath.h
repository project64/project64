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
void calc_light(VERTEX &v);
void calc_linear(VERTEX &v);
void calc_sphere(VERTEX &v);

void math_init();

typedef void(*MULMATRIX)(float m1[4][4], float m2[4][4], float r[4][4]);
extern MULMATRIX MulMatrices;
typedef void(*TRANSFORMVECTOR)(float *src, float *dst, float mat[4][4]);
extern TRANSFORMVECTOR TransformVector;
extern TRANSFORMVECTOR InverseTransformVector;
typedef float(*DOTPRODUCT)(register float *v1, register float *v2);
extern DOTPRODUCT DotProduct;
typedef void(*NORMALIZEVECTOR)(float *v);
extern NORMALIZEVECTOR NormalizeVector;
