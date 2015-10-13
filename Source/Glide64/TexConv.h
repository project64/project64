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

extern "C" void  __declspec(naked) asmTexConv_ARGB1555_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[isize]

tc1_loop:
        mov eax,[esi]
        add esi,4

        // arrr rrgg gggb bbbb
        // aaaa rrrr gggg bbbb
        mov edx,eax
        and eax,0x80008000
        mov ebx,eax                             // ebx = 0xa000000000000000
        shr eax,1
        or ebx,eax                              // ebx = 0xaa00000000000000
        shr eax,1
        or ebx,eax                              // ebx = 0xaaa0000000000000
        shr eax,1
        or ebx,eax                              // ebx = 0xaaaa000000000000

        mov eax,edx
        and eax,0x78007800              // eax = 0x0rrrr00000000000
        shr eax,3                               // eax = 0x0000rrrr00000000
        or ebx,eax                              // ebx = 0xaaaarrrr00000000

        mov eax,edx
        and eax,0x03c003c0              // eax = 0x000000gggg000000
        shr eax,2                               // eax = 0x00000000gggg0000
        or ebx,eax                              // ebx = 0xaaaarrrrgggg0000

        and edx,0x001e001e              // edx = 0x00000000000bbbb0
        shr edx,1                               // edx = 0x000000000000bbbb
        or ebx,edx                              // ebx = 0xaaaarrrrggggbbbb

        mov [edi],ebx
        add edi,4

        dec ecx
        jnz tc1_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmTexConv_AI88_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[isize]

tc1_loop:
        mov eax,[esi]
        add esi,4

        // aaaa aaaa iiii iiii
        // aaaa rrrr gggg bbbb
        mov edx,eax
        and eax,0xF000F000              // eax = 0xaaaa000000000000
        mov ebx,eax                             // ebx = 0xaaaa000000000000

        and edx,0x00F000F0              // edx = 0x00000000iiii0000
        shl edx,4                               // edx = 0x0000iiii00000000
        or ebx,edx                              // ebx = 0xaaaaiiii00000000
        shr edx,4                               // edx = 0x00000000iiii0000
        or ebx,edx                              // ebx = 0xaaaaiiiiiiii0000
        shr edx,4                               // edx = 0x000000000000iiii
        or ebx,edx                              // ebx = 0xaaaaiiiiiiiiiiii

        mov [edi],ebx
        add edi,4

        dec ecx
        jnz tc1_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmTexConv_AI44_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[isize]

tc1_loop:
        mov eax,[esi]
        add esi,4

        // aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
        // aaaa1 rrrr1 gggg1 bbbb1 aaaa0 rrrr0 gggg0 bbbb0
        // aaaa3 rrrr3 gggg3 bbbb3 aaaa2 rrrr2 gggg2 bbbb2
        mov edx,eax                             // eax = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
        shl eax,16                              // eax = aaaa1 iiii1 aaaa0 iiii0 0000  0000  0000  0000
        and eax,0xFF000000              // eax = aaaa1 iiii1 0000  0000  0000  0000  0000  0000
        mov ebx,eax                             // ebx = aaaa1 iiii1 0000  0000  0000  0000  0000  0000
        and eax,0x0F000000              // eax = 0000  iiii1 0000  0000  0000  0000  0000  0000
        shr eax,4                               // eax = 0000  0000  iiii1 0000  0000  0000  0000  0000
        or ebx,eax                              // ebx = aaaa1 iiii1 iiii1 0000  0000  0000  0000  0000
        shr eax,4                               // eax = 0000  0000  0000  iiii1 0000  0000  0000  0000
        or ebx,eax                              // ebx = aaaa1 iiii1 iiii1 iiii1 0000  0000  0000  0000

        mov eax,edx                             // eax = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
        shl eax,8                               // eax = aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0 0000  0000
        and eax,0x0000FF00              // eax = 0000  0000  0000  0000  aaaa0 iiii0 0000  0000
        or ebx,eax                              // ebx = aaaa1 iiii1 iiii1 iiii1 aaaa0 iiii0 0000  0000
        and eax,0x00000F00              // eax = 0000  0000  0000  0000  0000  iiii0 0000  0000
        shr eax,4                               // eax = 0000  0000  0000  0000  0000  0000  iiii0 0000
        or ebx,eax                              // ebx = aaaa1 iiii1 iiii1 iiii1 aaaa0 iiii0 iiii0 0000
        shr eax,4                               // eax = 0000  0000  0000  0000  0000  0000  0000  iiii0
        or ebx,eax                              // ebx = aaaa1 iiii1 iiii1 iiii1 aaaa0 iiii0 iiii0 iiii0

        mov [edi],ebx
        add edi,4

        mov eax,edx                             // eax = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
        and eax,0xFF000000              // eax = aaaa3 iiii3 0000  0000  0000  0000  0000  0000
        mov ebx,eax                             // ebx = aaaa3 iiii3 0000  0000  0000  0000  0000  0000
        and eax,0x0F000000              // eax = 0000  iiii3 0000  0000  0000  0000  0000  0000
        shr eax,4                               // eax = 0000  0000  iiii3 0000  0000  0000  0000  0000
        or ebx,eax                              // ebx = aaaa3 iiii3 iiii3 0000  0000  0000  0000  0000
        shr eax,4                               // eax = 0000  0000  0000  iiii3 0000  0000  0000  0000
        or ebx,eax                              // ebx = aaaa3 iiii3 iiii3 iiii3 0000  0000  0000  0000

                                                        // edx = aaaa3 iiii3 aaaa2 iiii2 aaaa1 iiii1 aaaa0 iiii0
        shr edx,8                               // edx = 0000  0000  aaaa3 aaaa3 aaaa2 iiii2 aaaa1 iiii1
        and edx,0x0000FF00              // edx = 0000  0000  0000  0000  aaaa2 iiii2 0000  0000
        or ebx,edx                              // ebx = aaaa3 iiii3 iiii3 iiii3 aaaa2 iiii2 0000  0000
        and edx,0x00000F00              // edx = 0000  0000  0000  0000  0000  iiii2 0000  0000
        shr edx,4                               // edx = 0000  0000  0000  0000  0000  0000  iiii2 0000
        or ebx,edx                              // ebx = aaaa3 iiii3 iiii3 iiii3 aaaa2 iiii2 iiii2 0000
        shr edx,4                               // edx = 0000  0000  0000  0000  0000  0000  0000  iiii2
        or ebx,edx                              // ebx = aaaa3 iiii3 iiii3 iiii3 aaaa2 iiii2 iiii2 iiii2

        mov [edi],ebx
        add edi,4

        dec ecx
        jnz tc1_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmTexConv_A8_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int isize)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[isize]

tc1_loop:
        mov eax,[esi]
        add esi,4

        // aaaa3 aaaa3 aaaa2 aaaa2 aaaa1 aaaa1 aaaa0 aaaa0
        // aaaa1 rrrr1 gggg1 bbbb1 aaaa0 rrrr0 gggg0 bbbb0
        // aaaa3 rrrr3 gggg3 bbbb3 aaaa2 rrrr2 gggg2 bbbb2
        mov edx,eax
        and eax,0x0000F000              // eax = 00 00 00 00 a1 00 00 00
        shl eax,16                              // eax = a1 00 00 00 00 00 00 00
        mov ebx,eax                             // ebx = a1 00 00 00 00 00 00 00
        shr eax,4
        or ebx,eax                              // ebx = a1 a1 00 00 00 00 00 00
        shr eax,4
        or ebx,eax                              // ebx = a1 a1 a1 00 00 00 00 00
        shr eax,4
        or ebx,eax                              // ebx = a1 a1 a1 a1 00 00 00 00

        mov eax,edx
        and eax,0x000000F0              // eax = 00 00 00 00 00 00 a0 00
        shl eax,8                               // eax = 00 00 00 00 a0 00 00 00
        or ebx,eax
        shr eax,4
        or ebx,eax
        shr eax,4
        or ebx,eax
        shr eax,4
        or ebx,eax                              // ebx = a1 a1 a1 a1 a0 a0 a0 a0

        mov [edi],ebx
        add edi,4

        mov eax,edx                             // eax = a3 a3 a2 a2 a1 a1 a0 a0
        and eax,0xF0000000              // eax = a3 00 00 00 00 00 00 00
        mov ebx,eax                             // ebx = a3 00 00 00 00 00 00 00
        shr eax,4
        or ebx,eax                              // ebx = a3 a3 00 00 00 00 00 00
        shr eax,4
        or ebx,eax                              // ebx = a3 a3 a3 00 00 00 00 00
        shr eax,4
        or ebx,eax                              // ebx = a3 a3 a3 a3 00 00 00 00

        and edx,0x00F00000              // eax = 00 00 a2 00 00 00 00 00
        shr edx,8                               // eax = 00 00 00 00 a2 00 00 00
        or ebx,edx
        shr edx,4
        or ebx,edx
        shr edx,4
        or ebx,edx
        shr edx,4
        or ebx,edx                              // ebx = a3 a3 a3 a3 a2 a2 a2 a2

        mov [edi],ebx
        add edi,4

        dec ecx
        jnz tc1_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

void TexConv_ARGB1555_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 1;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 2 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 2
  asmTexConv_ARGB1555_ARGB4444(src, dst, size);
}

void TexConv_AI88_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 1;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 2 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 2
  asmTexConv_AI88_ARGB4444(src, dst, size);
}

void TexConv_AI44_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 2;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 4 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 4
  asmTexConv_AI44_ARGB4444(src, dst, size);
}

void TexConv_A8_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 2;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 4 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 4
  asmTexConv_A8_ARGB4444(src, dst, size);
}
