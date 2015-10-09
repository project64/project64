/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
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

/****************************************************************

 Glide64 - Glide Plugin for Nintendo 64 emulators
 Project started on December 29th, 2001

 Authors:
 Dave2001, original author, founded the project in 2001, left it in 2002
 Gugaman, joined the project in 2002, left it in 2002
 Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
 Hiroshi 'KoolSmoky' Morii, joined the project in 2007

****************************************************************

 To modify Glide64:
 * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
 * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.

****************************************************************
*/

#include "Gfx_1.3.h"

/****************************************************************

               ******** Textures load ********

****************************************************************/


/*****************************************************************
4b textures load
*****************************************************************/


/****************************************************************
 Size: 0, Format: 2
 2009 ported to NASM - Sergey (Gonetz) Lipski
 *****************************************************************/
extern "C" void __declspec(naked) asmLoad4bCI (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
{
	_asm {
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov ebx,[pal]
        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        push ecx

        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz near end_y_loop
        push ecx

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax,esi
        and eax,0x7FF
        add esi,eax
        add edi,[ext]

        mov ecx,[wid_64]
 x_loop_2:
        push ecx

        mov eax,[esi+4]         // read all 8 pixels
        bswap eax
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        mov edx,esi
        add edx,8
        mov esi,[src]
        sub edx,esi
        and edx,0x7FF
        add esi,edx
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop_2

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax,esi
        and eax,0x7FF
        add esi,eax
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmLoad4bIAPal (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
{
	_asm {
		push ebp
		mov ebp, esp
		push ebx
        push esi
        push edi

        mov ebx,[pal]
        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        push ecx

        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz near end_y_loop
        push ecx

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax,esi
        and eax,0x7FF
        add esi,eax
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        push ecx

        mov eax,[esi+4]         // read all 8 pixels
        bswap eax
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        mov edx,esi
        add edx,8
        mov esi,[src]
        sub edx,esi
        and edx,0x7FF
        add esi,edx
        mov edx,eax

        // 1st dword output {
        shr eax,23
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,27
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shr eax,15
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,19
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 3rd dword output {
        mov eax,edx
        shr eax,7
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,11
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 4th dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1E
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,3
        and edx,0x1E
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop_2

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax,esi
        and eax,0x7FF
        add esi,eax
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

/*****************************************************************
 Size: 0, Format: 3

 ** BY GUGAMAN **
 2009 ported to NASM - Sergey (Gonetz) Lipski
*****************************************************************/
extern "C" void  __declspec(naked) asmLoad4bIA (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
{
	_asm {
		push ebp
		mov ebp, esp
		push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        push ecx

        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword {
        xor ecx,ecx

        // pixel #1
        //       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,24 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,28 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #2
        //       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        mov eax,edx
        shr eax,12 //Alpha
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,16 // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #3
        //       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,4 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #4
        //       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,12 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,8 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax


        mov [edi],ecx
        add edi,4
        // }

// 2nd dword {
        xor ecx,ecx

        // pixel #5
        //       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,8 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,12 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #6
        //       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,4
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx     // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #7
        //       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,16
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,12 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #8
        //       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,28 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,24 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword {
        xor ecx,ecx

        // pixel #1
        //       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,24 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,28 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #2
        //       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        mov eax,edx
        shr eax,12 //Alpha
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,16 // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #3
        //       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,4 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #4
        //       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,12 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,8 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax


        mov [edi],ecx
        add edi,4
        // }

// 2nd dword {
        xor ecx,ecx

        // pixel #5
        //       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,8 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,12 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #6
        //       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,4
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx     // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #7
        //       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,16
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,12 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #8
        //       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,28 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,24 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // *

        pop ecx
        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz near end_y_loop
        push ecx

        add esi,[line]
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        push ecx

        mov eax,[esi+4]         // read all 8 pixels
        bswap eax
        mov edx,eax

        // 1st dword {
        xor ecx,ecx

        // pixel #1
        //       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,24 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,28 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #2
        //       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        mov eax,edx
        shr eax,12 //Alpha
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,16 // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #3
        //       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,4 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #4
        //       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,12 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,8 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax


        mov [edi],ecx
        add edi,4
        // }

// 2nd dword {
        xor ecx,ecx

        // pixel #5
        //       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,8 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,12 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #6
        //       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,4
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx     // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #7
        //       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,16
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,12 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #8
        //       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,28 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,24 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,8
        mov edx,eax

// 1st dword {
        xor ecx,ecx

        // pixel #1
        //       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,24 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,28 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #2
        //       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        mov eax,edx
        shr eax,12 //Alpha
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,16 // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #3
        //       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,4 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #4
        //       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,12 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,8 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax


        mov [edi],ecx
        add edi,4
        // }

// 2nd dword {
        xor ecx,ecx

        // pixel #5
        //       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
        //       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
        mov eax,edx
        shr eax,8 //Alpha
        and eax,0x00000010
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shr eax,12 // Intensity
        and eax,0x0000000E
        or ecx,eax
        shr eax,3
        or ecx,eax

        // pixel #6
        //       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
        //       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,4
        and eax,0x00001000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx     // Intensity
        and eax,0x00000E00
        or ecx,eax
        shr eax,3
        and eax,0x00000100
        or ecx,eax

        // pixel #7
        //       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
        //       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
        //Alpha
        mov eax,edx
        shl eax,16
        and eax,0x00100000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,12 // Intensity
        and eax,0x000E0000
        or ecx,eax
        shr eax,3
        and eax,0x00010000
        or ecx,eax

        // pixel #8
        //       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
        //       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
        mov eax,edx
        shl eax,28 //Alpha
        and eax,0x10000000
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        shl eax,1
        or ecx,eax
        mov eax,edx
        shl eax,24 // Intensity
        and eax,0x0E000000
        or ecx,eax
        shr eax,3
        and eax,0x01000000
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx
        dec ecx
        jnz x_loop_2

        add esi,[line]
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
// Size: 0, Format: 4
// 2009 ported to NASM - Sergey (Gonetz) Lipski

extern "C" void  __declspec(naked) asmLoad4bI (wxUIntPtr src, int dst, wxUIntPtr wid_64, int height, int line, int ext)
{
	_asm {
		push ebp
		mov ebp, esp
		push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        push ecx

        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword {
        xor ecx,ecx
        shr eax,28              // 0xF0000000 -> 0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x0F000000 -> 0x00000F00
        shr eax,16
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shr eax,4               // 0x00F00000 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,8               // 0x000F0000 -> 0x0F000000
        and eax,0x0F000000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword {
        xor ecx,ecx
        mov eax,edx
        shr eax,12              // 0x0000F000 -> 0x0000000F
        and eax,0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x00000F00 -> 0x00000F00
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,12              // 0x000000F0 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        shl edx,24              // 0x0000000F -> 0x0F000000
        and edx,0x0F000000
        or ecx,edx
        shl edx,4
        or ecx,edx

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword {
        xor ecx,ecx
        shr eax,28              // 0xF0000000 -> 0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x0F000000 -> 0x00000F00
        shr eax,16
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shr eax,4               // 0x00F00000 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,8               // 0x000F0000 -> 0x0F000000
        and eax,0x0F000000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword {
        xor ecx,ecx
        mov eax,edx
        shr eax,12              // 0x0000F000 -> 0x0000000F
        and eax,0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x00000F00 -> 0x00000F00
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,12              // 0x000000F0 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        shl edx,24              // 0x0000000F -> 0x0F000000
        and edx,0x0F000000
        or ecx,edx
        shl edx,4
        or ecx,edx

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx
        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz near end_y_loop
        push ecx

        add esi,[line]
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        push ecx

        mov eax,[esi+4]         // read all 8 pixels
        bswap eax
        mov edx,eax

        // 1st dword {
        xor ecx,ecx
        shr eax,28              // 0xF0000000 -> 0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x0F000000 -> 0x00000F00
        shr eax,16
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shr eax,4               // 0x00F00000 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,8               // 0x000F0000 -> 0x0F000000
        and eax,0x0F000000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword {
        xor ecx,ecx
        mov eax,edx
        shr eax,12              // 0x0000F000 -> 0x0000000F
        and eax,0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x00000F00 -> 0x00000F00
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,12              // 0x000000F0 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        shl edx,24              // 0x0000000F -> 0x0F000000
        and edx,0x0F000000
        or ecx,edx
        shl edx,4
        or ecx,edx

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 8 pixels
        bswap eax
        add esi,8
        mov edx,eax

        // 1st dword {
        xor ecx,ecx
        shr eax,28              // 0xF0000000 -> 0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x0F000000 -> 0x00000F00
        shr eax,16
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shr eax,4               // 0x00F00000 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,8               // 0x000F0000 -> 0x0F000000
        and eax,0x0F000000
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword {
        xor ecx,ecx
        mov eax,edx
        shr eax,12              // 0x0000F000 -> 0x0000000F
        and eax,0x0000000F
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx             // 0x00000F00 -> 0x00000F00
        and eax,0x00000F00
        or ecx,eax
        shl eax,4
        or ecx,eax

        mov eax,edx
        shl eax,12              // 0x000000F0 -> 0x000F0000
        and eax,0x000F0000
        or ecx,eax
        shl eax,4
        or ecx,eax

        shl edx,24              // 0x0000000F -> 0x0F000000
        and edx,0x0F000000
        or ecx,edx
        shl edx,4
        or ecx,edx

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx
        dec ecx
        jnz x_loop_2

        add esi,[line]
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}
//****************************************************************
//8b textures load
//****************************************************************

//****************************************************************
// Size: 1, Format: 2
//
// 2008.03.29 cleaned up - H.Morii
// 2009 ported to NASM - Sergey (Gonetz) Lipski

extern "C" void  __declspec(naked) asmLoad8bCI (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
{
	_asm {
		push ebp
		mov ebp, esp
		push ebx
        push esi
        push edi

        mov ebx,[pal]
        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        push ecx

        mov eax,[esi]           // read all 4 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 4 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz near end_y_loop
        push ecx

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax,esi
        and eax,0x7FF
        add esi,eax
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        push ecx

        mov eax,[esi+4]         // read all 4 pixels
        bswap eax
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 4 pixels
        bswap eax
        mov edx,esi
        add edx,8
        mov esi,[src]
        sub edx,esi
        and edx,0x7FF
        add esi,edx
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,1
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,1

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop_2

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax,esi
        and eax,0x7FF
        add esi,eax
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmLoad8bIA8 (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
{
	_asm {
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov ebx,[pal]
        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        push ecx

        mov eax,[esi]           // read all 4 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 4 pixels
        bswap eax
        add esi,4
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz near end_y_loop
        push ecx

        add esi,[line]
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        push ecx

        mov eax,[esi+4]         // read all 4 pixels
        bswap eax
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // * copy
        mov eax,[esi]           // read all 4 pixels
        bswap eax
        add esi,8
        mov edx,eax

        // 1st dword output {
        shr eax,15
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        mov eax,edx
        shr eax,23
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }

        // 2nd dword output {
        mov eax,edx
        shl eax,1
        and eax,0x1FE
        mov cx,[ebx+eax]
        ror cx,8
        shl ecx,16

        shr edx,7
        and edx,0x1FE
        mov cx,[ebx+edx]
        ror cx,8

        mov [edi],ecx
        add edi,4
        // }
        // *

        pop ecx

        dec ecx
        jnz x_loop_2

        add esi,[line]
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
// Size: 1, Format: 3
//
// ** by Gugaman **
//
// 2008.03.29 cleaned up - H.Morii
// 2009 ported to NASM - Sergey (Gonetz) Lipski

extern "C" void  __declspec(naked) asmLoad8bIA4 (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
{
	_asm {
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        mov eax,[esi] // read all 4 pixels
        mov edx,eax

        shr eax,4     //all alpha
        shl edx,4
        and eax,0x0F0F0F0F
        and edx,0xF0F0F0F0
        add esi,4
        or eax,edx

        mov [edi],eax // save dword
        add edi,4

        mov eax,[esi] // read all 4 pixels
        mov edx,eax

        shr eax,4     //all alpha
        shl edx,4
        and eax,0x0F0F0F0F
        and edx,0xF0F0F0F0
        add esi,4
        or eax,edx

        mov [edi],eax // save dword
        add edi,4
        // *

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz end_y_loop
        push ecx

        add esi,[line]
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        mov eax,[esi+4] // read both pixels
        mov edx,eax

        shr eax,4       //all alpha
        shl edx,4
        and eax,0x0F0F0F0F
        and edx,0xF0F0F0F0
        or eax,edx

        mov [edi],eax //save dword
        add edi,4

        mov eax,[esi] // read both pixels
        add esi,8
        mov edx,eax

        shr eax,4     //all alpha
        shl edx,4
        and eax,0x0F0F0F0F
        and edx,0xF0F0F0F0
        or eax,edx

        mov [edi],eax //save dword
        add edi,4
        // *

        dec ecx
        jnz x_loop_2

        add esi,[line]
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
// Size: 1, Format: 4
//
// ** by Gugaman **
// 2009 ported to NASM - Sergey (Gonetz) Lipski

extern "C" void  __declspec(naked) asmLoad8bI (wxUIntPtr src, int dst, wxUIntPtr wid_64, int height, int line, int ext)
{
	_asm {
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        mov eax,[esi] // read all 4 pixels
        add esi,4

        mov [edi],eax // save dword
        add edi,4

        mov eax,[esi] // read all 4 pixels
        add esi,4

        mov [edi],eax // save dword
        add edi,4
        // *

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz end_y_loop
        push ecx

        add esi,[line]
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        mov eax,[esi+4] // read both pixels

        mov [edi],eax //save dword
        add edi,4

        mov eax,[esi] // read both pixels
        add esi,8

        mov [edi],eax //save dword
        add edi,4
        // *

        dec ecx
        jnz x_loop_2

        add esi,[line]
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}


//****************************************************************
//16b textures load
//****************************************************************

//****************************************************************
// Size: 2, Format: 0
//
// 2008.03.29 cleaned up - H.Morii
// 2009 ported to NASM - Sergey (Gonetz) Lipski

extern "C" void __declspec(naked) asmLoad16bRGBA (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
{
	_asm {
		align 4
		push        ebp  
		mov         ebp,esp 
        push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        mov eax,[esi]   // read both pixels
        mov ebx,[esi+4] // read both pixels
        bswap eax
        bswap ebx

        ror ax,1
        ror bx,1
        ror eax,16
        ror ebx,16
        ror ax,1
        ror bx,1

        mov  [edi],eax
        mov  [edi+4],ebx
        add esi,8
        add edi,8

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz end_y_loop
        push ecx

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax, esi
        and eax, 0xFFF
        add esi, eax
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        mov eax,[esi+4] // read both pixels
        mov ebx,[esi]   // read both pixels
        bswap eax
        bswap ebx

        ror ax,1
        ror bx,1
        ror eax,16
        ror ebx,16
        ror ax,1
        ror bx,1

        mov [edi],eax
        mov [edi+4],ebx
        add esi,8
        add edi,8

        dec ecx
        jnz x_loop_2

        mov eax,esi
        add eax,[line]
        mov esi,[src]
        sub eax, esi
        and eax, 0xFFF
        add esi, eax
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}



//****************************************************************
// Size: 2, Format: 3
//
// ** by Gugaman/Dave2001 **
//
// 2008.03.29 cleaned up - H.Morii
// 2009 ported to NASM - Sergey (Gonetz) Lipski

extern "C" void  __declspec(naked) asmLoad16bIA (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
{
	_asm {
		ALIGN 4

		push ebp
		mov ebp, esp
		push ebx
        push esi
        push edi

        mov esi,[src]
        mov edi,[dst]
        mov ecx,[height]
y_loop:
        push ecx
        mov ecx,[wid_64]
x_loop:
        mov eax,[esi]   // read both pixels
        mov ebx,[esi+4] // read both pixels
        mov [edi],eax
        mov [edi+4],ebx
        add esi,8
        add edi,8

        dec ecx
        jnz x_loop

        pop ecx
        dec ecx
        jz end_y_loop
        push ecx

        add esi,[line]
        add edi,[ext]

        mov ecx,[wid_64]
x_loop_2:
        mov eax,[esi+4] // read both pixels
        mov ebx,[esi]   // read both pixels
        mov [edi],eax
        mov [edi+4],ebx
        add esi,8
        add edi,8

        dec ecx
        jnz x_loop_2

        add esi,[line]
        add edi,[ext]

        pop ecx
        dec ecx
        jnz y_loop

end_y_loop:
        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
//
//            ******** Textures mirror/clamp/wrap ********
//
//****************************************************************

//****************************************************************
//8b textures mirror/clamp/wrap
//****************************************************************

extern "C" void  __declspec(naked) asmMirror8bS (int tex, int start, int width, int height, int mask, int line, int full, int count)
{
	_asm{
		ALIGN 4

		push ebp
		mov ebp, esp
		push ebx
        push esi
        push edi

        mov edi,[start]
        mov ecx,[height]
loop_y:

        xor edx,edx
loop_x:
        mov esi,[tex]
        mov ebx,[width]
        add ebx,edx
        and ebx,[width]
        jnz is_mirrored

        mov eax,edx
        and eax,[mask]
        add esi,eax
        mov al,[esi]
        mov [edi],al
        inc edi
        jmp end_mirror_check
is_mirrored:
        add esi,[mask]
        mov eax,edx
        and eax,[mask]
        sub esi,eax
        mov al,[esi]
        mov [edi],al
        inc edi
end_mirror_check:

        inc edx
        cmp edx,[count]
        jne loop_x

        add edi,[line]
        mov eax,[tex]
        add eax,[full]
        mov [tex],eax

        dec ecx
        jnz loop_y

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmWrap8bS (int tex, int start, int height, int mask, int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov edi,[start]
        mov ecx,[height]
loop_y:

        xor edx,edx
loop_x:

        mov esi,[tex]
        mov eax,edx
        and eax,[mask]
        shl eax,2
        add esi,eax
        mov eax,[esi]
        mov [edi],eax
        add edi,4

        inc edx
        cmp edx,[count]
        jne loop_x

        add edi,[line]
        mov eax,[tex]
        add eax,[full]
        mov [tex],eax

        dec ecx
        jnz loop_y

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmClamp8bS (int tex, int constant, int height,int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[constant]
        mov edi,[tex]

        mov ecx,[height]
y_loop:

        mov al,[esi]

        mov edx,[count]
x_loop:

        mov [edi],al            // don't unroll or make dword, it may go into next line (doesn't have to be multiple of two)
        inc edi

        dec edx
        jnz x_loop

        add esi,[full]
        add edi,[line]

        dec ecx
        jnz y_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
//16b textures mirror/clamp/wrap
//****************************************************************

extern "C" void  __declspec(naked) asmMirror16bS (int tex, int start, int width, int height, int mask, int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov edi,[start]
        mov ecx,[height]
loop_y:

        xor edx,edx
loop_x:
        mov esi,[tex]
        mov ebx,[width]
        add ebx,edx
        and ebx,[width]
        jnz is_mirrored

        mov eax,edx
        shl eax,1
        and eax,[mask]
        add esi,eax
        mov ax,[esi]
        mov [edi],ax
        add edi,2
        jmp end_mirror_check
is_mirrored:
        add esi,[mask]
        mov eax,edx
        shl eax,1
        and eax,[mask]
        sub esi,eax
        mov ax,[esi]
        mov [edi],ax
        add edi,2
end_mirror_check:

        inc edx
        cmp edx,[count]
        jne loop_x

        add edi,[line]
        mov eax,[tex]
        add eax,[full]
        mov [tex],eax

        dec ecx
        jnz loop_y

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmWrap16bS (int tex, int start, int height, int mask, int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov edi,[start]
        mov ecx,[height]
loop_y:

        xor edx,edx
loop_x:

        mov esi,[tex]
        mov eax,edx
        and eax,[mask]
        shl eax,2
        add esi,eax
        mov eax,[esi]
        mov [edi],eax
        add edi,4

        inc edx
        cmp edx,[count]
        jne loop_x

        add edi,[line]
        mov eax,[tex]
        add eax,[full]
        mov [tex],eax

        dec ecx
        jnz loop_y

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmClamp16bS (int tex, int constant, int height,int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[constant]
        mov edi,[tex]

        mov ecx,[height]
y_loop:

        mov ax,[esi]

        mov edx,[count]
x_loop:

        mov [edi],ax            // don't unroll or make dword, it may go into next line (doesn't have to be multiple of two)
        add edi,2

        dec edx
        jnz x_loop

        add esi,[full]
        add edi,[line]

        dec ecx
        jnz y_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
//32b textures mirror/clamp/wrap
//****************************************************************

extern "C" void  __declspec(naked) asmMirror32bS (int tex, int start, int width, int height, int mask, int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov edi,[start]
        mov ecx,[height]
loop_y:

        xor edx,edx
loop_x:
        mov esi,[tex]
        mov ebx,[width]
        add ebx,edx
        and ebx,[width]
        jnz is_mirrored

        mov eax,edx
        shl eax,2
        and eax,[mask]
        add esi,eax
        mov eax,[esi]
        mov [edi],eax
        add edi,4
        jmp end_mirror_check
is_mirrored:
        add esi,[mask]
        mov eax,edx
        shl eax,2
        and eax,[mask]
        sub esi,eax
        mov eax,[esi]
        mov [edi],eax
        add edi,4
end_mirror_check:

        inc edx
        cmp edx,[count]
        jne loop_x

        add edi,[line]
        mov eax,[tex]
        add eax,[full]
        mov [tex],eax

        dec ecx
        jnz loop_y

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmWrap32bS (int tex, int start, int height, int mask, int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov edi,[start]
        mov ecx,[height]
loop_y:

        xor edx,edx
loop_x:

        mov esi,[tex]
        mov eax,edx
        and eax,[mask]
        shl eax,2
        add esi,eax
        mov eax,[esi]
        mov [edi],eax
        add edi,4

        inc edx
        cmp edx,[count]
        jne loop_x

        add edi,[line]
        mov eax,[tex]
        add eax,[full]
        mov [tex],eax

        dec ecx
        jnz loop_y

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" void  __declspec(naked) asmClamp32bS (int tex, int constant, int height,int line, int full, int count)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push ebx
        push esi
        push edi

        mov esi,[constant]
        mov edi,[tex]

        mov ecx,[height]
y_loop:

        mov eax,[esi]

        mov edx,[count]
x_loop:

        mov [edi],eax           // don't unroll or make dword, it may go into next line (doesn't have to be multiple of two)
        add edi,4

        dec edx
        jnz x_loop

        add esi,[full]
        add edi,[line]

        dec ecx
        jnz y_loop

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
//
//             ******** Textures conversion ********
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

//****************************************************************
//
//                ******** Tmem functions ********
//
//****************************************************************

//****************************************************************
// CopyBlock - copies a block from base_addr+offset to dest_addr, while unswapping the
//  data within.
//
// edi = dest_addr -> end of dest
// ecx = num_words
// esi = base_addr (preserved)
// edx = offset (preserved)
//****************************************************************
void __declspec(naked) CopyBlock ( void )
{
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push eax
        push ebx
        push esi
        push edx

        or ecx,ecx
        jz near copyblock_end

        push ecx

        // first, set the source address and check if not on a dword boundary
        push esi
        push edx
        mov ebx,edx
        and edx,0FFFFFFFCh
        add esi,edx

        and ebx,3                               // ebx = # we DON'T need to copy
        jz copyblock_copy

        mov edx,4                               // ecx = # we DO need to copy
        sub edx,ebx

        // load the first word, accounting for swapping

        mov eax,[esi]
        add esi,4
copyblock_precopy_skip:
        rol eax,8
        dec ebx
        jnz copyblock_precopy_skip

copyblock_precopy_copy:
        rol eax,8
        mov [edi],al
        inc edi
        dec edx
        jnz copyblock_precopy_copy

        mov eax,[esi]
        add esi,4
        bswap eax
        mov [edi],eax
        add edi,4

        dec ecx         // 1 less word to copy
        jz copyblock_postcopy

copyblock_copy:
        mov eax,[esi]
        bswap eax
        mov [edi],eax

        mov eax,[esi+4]
        bswap eax
        mov [edi+4],eax

        add esi,8
        add edi,8

        dec ecx
        jnz copyblock_copy

copyblock_postcopy:
        pop edx
        pop esi
        pop ecx

        // check again if on dword boundary
        mov ebx,edx     // ebx = # we DO need to copy

        and ebx,3
        jz copyblock_end

        shl ecx,3       // ecx = num_words * 8
        add edx,ecx
        and edx,0FFFFFFFCh
        add esi,edx

        mov eax,[esi]

copyblock_postcopy_copy:
        rol eax,8
        mov [edi],al
        inc edi
        dec ebx
        jnz copyblock_postcopy_copy

copyblock_end:
        pop edx
        pop esi
        pop ebx
        pop eax
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" __declspec(naked) void SwapBlock32 ( void )
{
//****************************************************************
// SwapBlock - swaps every other 32-bit word at addr
//
// ecx = num_words -> 0
// edi = addr -> end of dest
//****************************************************************
	_asm {
		align 4
		push ebp
		mov ebp, esp
        push eax
        push ebx
        or ecx,ecx
        jz swapblock32_end
swapblock32_loop:
        mov eax,[edi]
        mov ebx,[edi+4]
        mov [edi],ebx
        mov [edi+4],eax
        add edi,8
        dec ecx
        jnz swapblock32_loop
swapblock32_end:
        pop ebx
        pop eax
		mov esp, ebp
		pop ebp
		ret
	}
}

//****************************************************************
//
//               ******** Load block/tile ********
//
//****************************************************************

extern "C" __declspec(naked) void asmLoadBlock(int src, int dst, int off, int dxt, int cnt, wxUIntPtr swp)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp

        push ebx
        push esi
        push edi

        // copy the data
        mov esi,[src]
        mov edi,[dst]
        mov ecx,[cnt]
        mov edx,[off]
        call CopyBlock

        // now swap it
        mov eax,[cnt]   // eax = count remaining
        xor edx,edx         // edx = dxt counter
        mov edi,[dst]
        mov ebx,[dxt]

        xor ecx,ecx     // ecx = how much to copy
dxt_test:
        add edi,8
        dec eax
        jz end_dxt_test
        add edx,ebx
        jns dxt_test

dxt_s_test:
        inc ecx
        dec eax
        jz end_dxt_test
        add edx,ebx
        js dxt_s_test

        // swap this data (ecx set, dst set)
        call [swp] // (ecx reset to 0 after)

        jmp dxt_test  // and repeat

end_dxt_test:
        // swap any remaining data
        call [swp]

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

extern "C" __declspec(naked) void asmLoadTile(int src, int dst, int width, int height, int line, int off, int end)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp

        push ebx
        push esi
        push edi

        // set initial values
        mov edi,[dst]
        mov ecx,[width]
        mov esi,[src]
        mov edx,[off]
        xor ebx,ebx         // swap this line?
        mov eax,[height]

loadtile_loop:
        cmp [end],edi   // end of tmem: error
        jc loadtile_end

        // copy this line
        push edi
        push ecx
        call CopyBlock
        pop ecx

        // swap it?
        xor ebx,1
        jnz loadtile_no_swap

        // (ecx set, restore edi)
        pop edi
        push ecx
        call SwapBlock32
        pop ecx
        jmp loadtile_swap_end
loadtile_no_swap:
        add sp,4  // forget edi, we are already at the next position
loadtile_swap_end:

        add edx,[line]

        dec eax
        jnz loadtile_loop

loadtile_end:

        pop edi
        pop esi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}


//****************************************************************
//
//               ******** Texture CRC ********
//
//****************************************************************
extern "C" __declspec(naked) int asmTextureCRC(int addr, int width, int height, int line)
{
	_asm {
		align 4
		push ebp
		mov ebp, esp

        push ebx
        push edi

        xor eax,eax                             // eax is final result
        mov ebx,[line]
        mov ecx,[height]                // ecx is height counter
        mov edi,[addr]                  // edi is ptr to texture memory
crc_loop_y:
        push ecx

        mov ecx,[width]
crc_loop_x:

        add eax,[edi]           // MUST be 64-bit aligned, so manually unroll
        add eax,[edi+4]
        mov edx,ecx
        mul edx
        add eax,edx
        add edi,8

        dec ecx
        jnz crc_loop_x

        pop ecx

        mov edx,ecx
        mul edx
        add eax,edx

        add edi,ebx

        dec ecx
        jnz crc_loop_y

        pop edi
        pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}
