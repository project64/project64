// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
void calc_light(gfxVERTEX &v);
void calc_linear(gfxVERTEX &v);
void calc_sphere(gfxVERTEX &v);

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
