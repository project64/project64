#include "Gfx #1.3.h"

/*****************************************************************
;
;             ******** Textures conversion ********
;
;*****************************************************************/
void asmTexConv_ARGB1555_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
push ebx
push esi
push edi

mov esi,[src]
mov edi,[dst]
mov ecx,[isize]

tc1_loop:
mov eax,[esi]
add esi,4

; arrr rrgg gggb bbbb
; aaaa rrrr gggg bbbb
mov edx,eax
and eax,0x80008000
mov ebx,eax                             ; ebx = 0xa000000000000000
shr eax,1
or ebx,eax                              ; ebx = 0xaa00000000000000
shr eax,1
or ebx,eax                              ; ebx = 0xaaa0000000000000
shr eax,1
or ebx,eax                              ; ebx = 0xaaaa000000000000

mov eax,edx
and eax,0x78007800              ; eax = 0x0rrrr00000000000
shr eax,3                               ; eax = 0x0000rrrr00000000
or ebx,eax                              ; ebx = 0xaaaarrrr00000000

mov eax,edx
and eax,0x03c003c0              ; eax = 0x000000gggg000000
shr eax,2                               ; eax = 0x00000000gggg0000
or ebx,eax                              ; ebx = 0xaaaarrrrgggg0000

and edx,0x001e001e              ; edx = 0x00000000000bbbb0
shr edx,1                               ; edx = 0x000000000000bbbb
or ebx,edx                              ; ebx = 0xaaaarrrrggggbbbb

mov [edi],ebx
add edi,4

dec ecx
jnz tc1_loop

pop edi
pop esi
pop ebx
	}
}

void asmTexConv_AI88_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
push ebx
push esi
push edi

mov esi,[src]
mov edi,[dst]
mov ecx,[isize]

tc1_loop:
mov eax,[esi]
add esi,4

; aaaa aaaa iiii iiii
; aaaa rrrr gggg bbbb
mov edx,eax
and eax,0xF000F000              ; eax = 0xaaaa000000000000
mov ebx,eax                             ; ebx = 0xaaaa000000000000

and edx,0x00F000F0              ; edx = 0x00000000iiii0000
shl edx,4                               ; edx = 0x0000iiii00000000
or ebx,edx                              ; ebx = 0xaaaaiiii00000000
shr edx,4                               ; edx = 0x00000000iiii0000
or ebx,edx                              ; ebx = 0xaaaaiiiiiiii0000
shr edx,4                               ; edx = 0x000000000000iiii
or ebx,edx                              ; ebx = 0xaaaaiiiiiiiiiiii

mov [edi],ebx
add edi,4

dec ecx
jnz tc1_loop

pop edi
pop esi
pop ebx
	}
}

void asmTexConv_AI44_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
push ebx
push esi
push edi

mov esi,[src]
mov edi,[dst]
mov ecx,[isize]

tc1_loop:
mov eax,[esi]
add esi,4

; aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
; aaaa1 rrrr1 gggg1 bbbb1 aaaa0 rrrr0 gggg0 bbbb0
; aaaa3 rrrr3 gggg3 bbbb3 aaaa2 rrrr2 gggg2 bbbb2
mov edx,eax                             ; eax = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
shl eax,16                              ; eax = aaaa1 iiii1 aaaa0 iiii0 0000  0000  0000  0000
and eax,0xFF000000              ; eax = aaaa1 iiii1 0000  0000  0000  0000  0000  0000
mov ebx,eax                             ; ebx = aaaa1 iiii1 0000  0000  0000  0000  0000  0000
and eax,0x0F000000              ; eax = 0000  iiii1 0000  0000  0000  0000  0000  0000
shr eax,4                               ; eax = 0000  0000  iiii1 0000  0000  0000  0000  0000
or ebx,eax                              ; ebx = aaaa1 iiii1 iiii1 0000  0000  0000  0000  0000
shr eax,4                               ; eax = 0000  0000  0000  iiii1 0000  0000  0000  0000
or ebx,eax                              ; ebx = aaaa1 iiii1 iiii1 iiii1 0000  0000  0000  0000

mov eax,edx                             ; eax = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
shl eax,8                               ; eax = aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0 0000  0000
and eax,0x0000FF00              ; eax = 0000  0000  0000  0000  aaaa0 iiii0 0000  0000
or ebx,eax                              ; ebx = aaaa1 iiii1 iiii1 iiii1 aaaa0 iiii0 0000  0000
and eax,0x00000F00              ; eax = 0000  0000  0000  0000  0000  iiii0 0000  0000
shr eax,4                               ; eax = 0000  0000  0000  0000  0000  0000  iiii0 0000
or ebx,eax                              ; ebx = aaaa1 iiii1 iiii1 iiii1 aaaa0 iiii0 iiii0 0000
shr eax,4                               ; eax = 0000  0000  0000  0000  0000  0000  0000  iiii0
or ebx,eax                              ; ebx = aaaa1 iiii1 iiii1 iiii1 aaaa0 iiii0 iiii0 iiii0

mov [edi],ebx
add edi,4

mov eax,edx                             ; eax = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
and eax,0xFF000000              ; eax = aaaa3 iiii3 0000  0000  0000  0000  0000  0000
mov ebx,eax                             ; ebx = aaaa3 iiii3 0000  0000  0000  0000  0000  0000
and eax,0x0F000000              ; eax = 0000  iiii3 0000  0000  0000  0000  0000  0000
shr eax,4                               ; eax = 0000  0000  iiii3 0000  0000  0000  0000  0000
or ebx,eax                              ; ebx = aaaa3 iiii3 iiii3 0000  0000  0000  0000  0000
shr eax,4                               ; eax = 0000  0000  0000  iiii3 0000  0000  0000  0000
or ebx,eax                              ; ebx = aaaa3 iiii3 iiii3 iiii3 0000  0000  0000  0000

; edx = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
shr edx,8                               ; edx = 0000  0000  aaaa3 aaaa3 aaaa2 iiii2 aaaa1 iiii1
and edx,0x0000FF00              ; edx = 0000  0000  0000  0000  aaaa2 iiii2 0000  0000
or ebx,edx                              ; ebx = aaaa3 iiii3 iiii3 iiii3 aaaa2 iiii2 0000  0000
and edx,0x00000F00              ; edx = 0000  0000  0000  0000  0000  iiii2 0000  0000
shr edx,4                               ; edx = 0000  0000  0000  0000  0000  0000  iiii2 0000
or ebx,edx                              ; ebx = aaaa3 iiii3 iiii3 iiii3 aaaa2 iiii2 iiii2 0000
shr edx,4                               ; edx = 0000  0000  0000  0000  0000  0000  0000  iiii2
or ebx,edx                              ; ebx = aaaa3 iiii3 iiii3 iiii3 aaaa2 iiii2 iiii2 iiii2

mov [edi],ebx
add edi,4

dec ecx
jnz tc1_loop

pop edi
pop esi
pop ebx
	}
}

void asmTexConv_A8_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
push ebx
push esi
push edi

mov esi,[src]
mov edi,[dst]
mov ecx,[isize]

tc1_loop:
mov eax,[esi]
add esi,4

; aaaa3 aaaa3 aaaa2 aaaa2 aaaa1 aaaa1 aaaa0 aaaa0
; aaaa1 rrrr1 gggg1 bbbb1 aaaa0 rrrr0 gggg0 bbbb0
; aaaa3 rrrr3 gggg3 bbbb3 aaaa2 rrrr2 gggg2 bbbb2
mov edx,eax
and eax,0x0000F000              ; eax = 00 00 00 00 a1 00 00 00
shl eax,16                              ; eax = a1 00 00 00 00 00 00 00
mov ebx,eax                             ; ebx = a1 00 00 00 00 00 00 00
shr eax,4
or ebx,eax                              ; ebx = a1 a1 00 00 00 00 00 00
shr eax,4
or ebx,eax                              ; ebx = a1 a1 a1 00 00 00 00 00
shr eax,4
or ebx,eax                              ; ebx = a1 a1 a1 a1 00 00 00 00

mov eax,edx
and eax,0x000000F0              ; eax = 00 00 00 00 00 00 a0 00
shl eax,8                               ; eax = 00 00 00 00 a0 00 00 00
or ebx,eax
shr eax,4
or ebx,eax
shr eax,4
or ebx,eax
shr eax,4
or ebx,eax                              ; ebx = a1 a1 a1 a1 a0 a0 a0 a0

mov [edi],ebx
add edi,4

mov eax,edx                             ; eax = a3 a3 a2 a2 a1 a1 a0 a0
and eax,0xF0000000              ; eax = a3 00 00 00 00 00 00 00
mov ebx,eax                             ; ebx = a3 00 00 00 00 00 00 00
shr eax,4
or ebx,eax                              ; ebx = a3 a3 00 00 00 00 00 00
shr eax,4
or ebx,eax                              ; ebx = a3 a3 a3 00 00 00 00 00
shr eax,4
or ebx,eax                              ; ebx = a3 a3 a3 a3 00 00 00 00

and edx,0x00F00000              ; eax = 00 00 a2 00 00 00 00 00
shr edx,8                               ; eax = 00 00 00 00 a2 00 00 00
or ebx,edx
shr edx,4
or ebx,edx
shr edx,4
or ebx,edx
shr edx,4
or ebx,edx                              ; ebx = a3 a3 a3 a3 a2 a2 a2 a2

mov [edi],ebx
add edi,4

dec ecx
jnz tc1_loop

pop edi
pop esi
pop ebx
	}
}

