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
// Size: 1, Format: 2
//

wxUint32 Load8bCI (wxUIntPtr dst, wxUIntPtr src, int wid_64, int height, int line, int real_width, int /*tile*/)
{
  if (wid_64 < 1) wid_64 = 1;
  if (height < 1) height = 1;
  int ext = (real_width - (wid_64 << 3));
  wxUIntPtr pal = wxPtrToUInt(rdp.pal_8);

  switch (rdp.tlut_mode) {
    case 0: //palette is not used
      //in tlut DISABLE mode load CI texture as plain intensity texture instead of palette dereference. 
      //Thanks to angrylion for the advice
      asmLoad8bI (src, dst, wid_64, height, line, ext);	
      return /*(0 << 16) | */GR_TEXFMT_ALPHA_8;
    case 2: //color palette
      ext <<= 1;
      asmLoad8bCI (src, dst, wid_64, height, line, ext, pal);
      return (1 << 16) | GR_TEXFMT_ARGB_1555;
    default: //IA palette
      ext <<= 1;
      asmLoad8bIA8 (src, dst, wid_64, height, line, ext, pal);
      return (1 << 16) | GR_TEXFMT_ALPHA_INTENSITY_88;
  }
}

//****************************************************************
// Size: 1, Format: 3
//

wxUint32 Load8bIA (wxUIntPtr dst, wxUIntPtr src, int wid_64, int height, int line, int real_width, int tile)  
{ 
  if (rdp.tlut_mode != 0)
    return Load8bCI (dst, src, wid_64, height, line, real_width, tile);

  if (wid_64 < 1) wid_64 = 1;  
  if (height < 1) height = 1;  
  int ext = (real_width - (wid_64 << 3));  
  asmLoad8bIA4 (src, dst, wid_64, height, line, ext);
  return /*(0 << 16) | */GR_TEXFMT_ALPHA_INTENSITY_44;  
} 

//****************************************************************
// Size: 1, Format: 4
//

wxUint32 Load8bI (wxUIntPtr dst, wxUIntPtr src, int wid_64, int height, int line, int real_width, int tile)  
{ 
  if (rdp.tlut_mode != 0)
    return Load8bCI (dst, src, wid_64, height, line, real_width, tile);

  if (wid_64 < 1) wid_64 = 1;  
  if (height < 1) height = 1;  
  int ext = (real_width - (wid_64 << 3));  
  asmLoad8bI (src, dst, wid_64, height, line, ext);	
  return /*(0 << 16) | */GR_TEXFMT_ALPHA_8;  
}
