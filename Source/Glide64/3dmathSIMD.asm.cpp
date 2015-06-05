;/*
;* Glide64 - Glide video plugin for Nintendo 64 emulators.
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
;****************************************************************
;
; Glide64 - Glide Plugin for Nintendo 64 emulators
; Project started on December 29th, 2001
;
; Authors:
; Dave2001, original author, founded the project in 2001, left it in 2002
; Gugaman, joined the project in 2002, left it in 2002
; Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
; Hiroshi 'KoolSmoky' Morii, joined the project in 2007
;
;****************************************************************
;
; To modify Glide64:
; * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
; * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
;
;****************************************************************

%include "inc/c32.mac"

segment .text
*/

extern "C" void __declspec(naked) DetectSIMD(int func, int * iedx, int * iecx)
{
	_asm {
		push ebp
		mov ebp,esp
		mov       eax,[func]
		cpuid
		mov       eax,[iedx]
		mov       [eax],edx
		mov       eax,[iecx]
		mov       [eax],ecx
		leave
		ret
	}
}

/****************************************************************
;
;                     ******** SSE ********
;
;****************************************************************/

extern "C" void __declspec(naked) NormalizeVectorSSE (float *v)
{
	_asm
	{
		push ebp
		mov ebp,esp

      mov edx, [v]
      movaps xmm0, [edx]      ; x y z 0
      movaps xmm2, xmm0       ; x y z 0
      mulps  xmm0, xmm0       ; x*x y*y z*z 0
      movaps xmm1, xmm0       ; x*x y*y z*z 0
      shufps xmm0, xmm1, 0x4e ; z*z 0 x*x y*y
      addps  xmm0, xmm1       ; x*x+z*z y*y z*z+x*x y*y
      movaps xmm1, xmm0       ; x*x+z*z y*y z*z+x*x y*y
      shufps xmm1, xmm1, 0x11 ; y*y z*z+x*x y*y z*z+x*x
      addps  xmm0, xmm1       ; x*x+z*z+y*y
      rsqrtps xmm0, xmm0      ; 1.0/sqrt(x*x+z*z+y*y)
      mulps  xmm2, xmm0       ; x/sqrt(x*x+z*z+y*y) y/sqrt(x*x+z*z+y*y) z/sqrt(x*x+z*z+y*y) 0
      movaps [edx], xmm2
	  leave
	  ret
	}
}

/*****************************************************************
;
;                     ******** SSE3 ********
;
;****************************************************************/

float __declspec(naked) DotProductSSE3(register float *v1, register float *v2)
{
	_asm {
		push ebp
		mov ebp,esp

      mov eax,[v1]
      mov edx,[v2]
      movaps xmm0, [eax]
      mulps xmm0, [edx]
      haddps xmm0, xmm0
      haddps xmm0, xmm0
;      movss eax, xmm0
		leave
		ret
	}
}

/****************************************************************
;
;                     ******** 3DNOW ********
;
;****************************************************************/

extern "C" void __declspec(naked) InverseTransformVector3DNOW(float *src, float *dst, float mat[4][4])
{
	_asm {
		push ebp
		mov ebp,esp

    femms
      mov         ecx,[src]
      mov         eax,[dst]
      mov         edx,[mat]
      movq        mm0,[ecx]     ; src[1] src[0]
      movd        mm4,[ecx+8]   ; 0 src[2]
      movq        mm1,mm0       ; src[1] src[0]
      pfmul       mm0,[edx]     ; src[1]*mat[0][1] src[0]*mat[0][0]
      movq        mm5,mm4       ; 0 src[2]
      pfmul       mm4,[edx+8]   ; 0 src[2]*mat[0][2]
      movq        mm2,mm1       ; src[1] src[0]
      pfmul       mm1,[edx+16]  ; src[1]*mat[1][1] src[0]*mat[1][0]
      movq        mm6,mm5       ; 0 src[2]
      pfmul       mm5,[edx+24]  ; 0 src[2]*mat[1][2]
      movq        mm3,mm2       ; src[1] src[0]
      pfmul       mm2,[edx+32]  ; src[1]*mat[2][1] src[0]*mat[2][0]
      movq        mm7,mm6       ; 0 src[2]
      pfmul       mm6,[edx+40]  ; 0 src[2]*mat[2][2]
      pfacc       mm0,mm4       ; src[2]*mat[0][2] src[1]*mat[0][1]+src[0]*mat[0][0]
      pfacc       mm1,mm5       ; src[2]*mat[1][2] src[1]*mat[1][1]+src[0]*mat[1][0]
      pfacc       mm2,mm6       ; src[2]*mat[2][2] src[1]*mat[2][1]+src[0]*mat[2][0]
      pfacc       mm0,mm1       ; src[2]*mat[1][2]+src[1]*mat[1][1]+src[0]*mat[1][0] src[2]*mat[0][2]+src[1]*mat[0][1]+src[0]*mat[0][0]
      pfacc       mm2,mm3       ; 0 src[2]*mat[2][2]+src[1]*mat[2][1]+src[0]*mat[2][0]
      movq        [eax],mm0     ; mat[1][0]*src[0]+mat[1][1]*src[1]+mat[1][2]*src[2] mat[0][0]*src[0]+mat[0][1]*src[1]+mat[0][2]*src[2]
      movd        [eax+8],mm2   ; mat[2][0]*src[0]+mat[2][1]*src[1]+mat[2][2]*src[2]
      femms                    
		  leave
		  ret
	}
}

extern "C" float  __declspec(naked) DotProduct3DNOW(register float *v1, register float *v2)
{
	_asm {
		push ebp
		mov ebp,esp
      
      femms
      mov         edx,[v1]
      mov         eax,[v2]
      movq        mm0,[edx]
      movq        mm3,[eax]
      pfmul       mm0,mm3
      movq        mm2,[edx+8]
      movq        mm1,[eax+8]
      pfacc       mm0,mm0
      pfmul       mm1,mm2
      pfadd       mm0,mm1
      movd        eax,mm0
      femms
	  leave
	  ret
	}
}

extern "C" void __declspec(naked) NormalizeVector3DNOW(float *v)
{
	_asm {
		push ebp
		mov ebp,esp      
      femms
      mov          edx,[v]
      movq         mm0,[edx]
      movq         mm3,[edx+8]
      movq         mm1,mm0
      movq         mm2,mm3
      pfmul        mm0,mm0
      pfmul        mm3,mm3
      pfacc        mm0,mm0
      pfadd        mm0,mm3
      ;movq mm4,mm0 ; prepare for 24bit precision
      ;punpckldq mm4,mm4 ; prepare for 24bit precision
      pfrsqrt      mm0,mm0 ; 15bit precision 1/sqrtf(v)
      ;movq mm3,mm0
      ;pfmul mm0,mm0
      ;pfrsqit1 mm0,mm4
      ;pfrcpit2 mm0,mm3 ; 24bit precision 1/sqrtf(v)
      pfmul        mm1,mm0
      pfmul        mm2,mm0
      movq         [edx],mm1
      movq         [edx+8],mm2
      femms
	  leave
	  ret
	}
}
