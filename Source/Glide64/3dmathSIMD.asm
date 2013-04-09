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
;*/
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

;****************************************************************
;
;                     ******** SSE ********
;
;****************************************************************


proc MulMatricesSSE
CPU P3 
endproc ;MulMatricesSSE

proc NormalizeVectorSSE
CPU P3
      %$v arg
      
      mov edx, [ebp + %$v]
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
      
endproc ;NormalizeVectorSSE

;****************************************************************
;
;                     ******** SSE3 ********
;
;****************************************************************


;****************************************************************
;
;                     ******** 3DNOW ********
;
;****************************************************************




proc DotProduct3DNOW
CPU 586
      %$v1        arg
      %$v2        arg
      
      femms
      mov         edx,[ebp + %$v1]
      mov         eax,[ebp + %$v2]
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

endproc ;DotProduct3DNOW

