#include "Gfx #1.3.h"

/****************************************************************
;4b textures load
;***************************************************************/
void asmLoad4bCI (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
{
	_asm{
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

		mov eax,[esi]           ; read all 8 pixels
		bswap eax
		add esi,4
		mov edx,eax

		; 1st dword output {
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
		; }

		; 2nd dword output {
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
		; }

		; 3rd dword output {
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
		; }

		; 4th dword output {
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
		; }

		; * copy
		mov eax,[esi]           ; read all 8 pixels
		bswap eax
		add esi,4
		mov edx,eax

		; 1st dword output {
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
		; }

		; 2nd dword output {
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
		; }

		; 3rd dword output {
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
		; }

		; 4th dword output {
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
		; }
		; *

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

		mov eax,[esi+4]         ; read all 8 pixels
		bswap eax
		mov edx,eax

			; 1st dword output {
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
					; }

		; 2nd dword output {
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
				; }

		; 3rd dword output {
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
				; }

		; 4th dword output {
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
				; }

		; * copy
			mov eax,[esi]           ; read all 8 pixels
			bswap eax
			mov edx,esi
			add edx,8
			mov esi,[src]
		sub edx,esi
			and edx,0x7FF
			add esi,edx
			mov edx,eax

			; 1st dword output {
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
					; }

		; 2nd dword output {
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
				; }

		; 3rd dword output {
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
				; }

		; 4th dword output {
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
				; }
		; *

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
	}
}

/*****************************************************************
; Size: 0, Format: 2
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad4bIAPal (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
{
	_asm {
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

			mov eax,[esi]           ; read all 8 pixels
			bswap eax
			add esi,4
			mov edx,eax

			; 1st dword output {
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
					; }

		; 2nd dword output {
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
				; }

		; 3rd dword output {
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
				; }

		; 4th dword output {
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
				; }

		; * copy
			mov eax,[esi]           ; read all 8 pixels
			bswap eax
			add esi,4
			mov edx,eax

			; 1st dword output {
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
					; }

		; 2nd dword output {
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
				; }

		; 3rd dword output {
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
				; }

		; 4th dword output {
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
				; }
		; *

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

			mov eax,[esi+4]         ; read all 8 pixels
			bswap eax
			mov edx,eax

			; 1st dword output {
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
					; }

		; 2nd dword output {
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
				; }

		; 3rd dword output {
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
				; }

		; 4th dword output {
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
				; }

		; * copy
			mov eax,[esi]           ; read all 8 pixels
			bswap eax
			mov edx,esi
			add edx,8
			mov esi,[src]
		sub edx,esi
			and edx,0x7FF
			add esi,edx
			mov edx,eax

			; 1st dword output {
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
					; }

		; 2nd dword output {
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
				; }

		; 3rd dword output {
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
				; }

		; 4th dword output {
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
				; }
		; *

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

	}
}

/*proc asmLoad4bCI
CPU 586
src     arg
%$dst     arg
%$wid_64  arg
%$height  arg
%$line    arg
%$ext     arg
%$pal     arg
endproc ;asmLoad4bIAPal
*/

/****************************************************************
; Size: 0, Format: 3
;
; ** BY GUGAMAN **
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad4bIA (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
{
	_asm {
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

mov eax,[esi]           ; read all 8 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword {
	xor ecx,ecx

		; pixel #1
		;       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,24 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,28 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #2
		;       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		mov eax,edx
		shr eax,12 ;Alpha
		and eax,0x00001000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,16 ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #3
		;       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shr eax,4 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #4
		;       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,12 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,8 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax


		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx

		; pixel #5
		;       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,8 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,12 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #6
		;       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		;Alpha
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
		mov eax,edx     ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #7
		;       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shl eax,12 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #8
		;       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,28 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,24 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; * copy
mov eax,[esi]           ; read all 8 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword {
	xor ecx,ecx

		; pixel #1
		;       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,24 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,28 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #2
		;       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		mov eax,edx
		shr eax,12 ;Alpha
		and eax,0x00001000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,16 ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #3
		;       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shr eax,4 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #4
		;       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,12 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,8 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax


		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx

		; pixel #5
		;       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,8 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,12 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #6
		;       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		;Alpha
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
		mov eax,edx     ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #7
		;       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shl eax,12 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #8
		;       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,28 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,24 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; *

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

mov eax,[esi+4]         ; read all 8 pixels
bswap eax
mov edx,eax

; 1st dword {
	xor ecx,ecx

		; pixel #1
		;       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,24 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,28 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #2
		;       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		mov eax,edx
		shr eax,12 ;Alpha
		and eax,0x00001000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,16 ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #3
		;       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shr eax,4 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #4
		;       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,12 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,8 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax


		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx

		; pixel #5
		;       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,8 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,12 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #6
		;       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		;Alpha
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
		mov eax,edx     ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #7
		;       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shl eax,12 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #8
		;       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,28 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,24 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; * copy
mov eax,[esi]           ; read all 8 pixels
bswap eax
add esi,8
mov edx,eax

; 1st dword {
	xor ecx,ecx

		; pixel #1
		;       IIIAxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,24 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,28 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #2
		;       xxxxIIIAxxxxxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		mov eax,edx
		shr eax,12 ;Alpha
		and eax,0x00001000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,16 ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #3
		;       xxxxxxxxIIIAxxxxxxxxxxxxxxxxxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shr eax,4 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #4
		;       xxxxxxxxxxxxIIIAxxxxxxxxxxxxxxxx
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,12 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,8 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax


		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx

		; pixel #5
		;       xxxxxxxxxxxxxxxxIIIAxxxxxxxxxxxx
		;       xxxxxxxxxxxxxxxxxxxxxxxxAAAAIIII
		mov eax,edx
		shr eax,8 ;Alpha
		and eax,0x00000010
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shr eax,12 ; Intensity
		and eax,0x0000000E
		or ecx,eax
		shr eax,3
		or ecx,eax

		; pixel #6
		;       xxxxxxxxxxxxxxxxxxxxIIIAxxxxxxxx
		;       xxxxxxxxxxxxxxxxAAAAIIIIxxxxxxxx
		;Alpha
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
		mov eax,edx     ; Intensity
		and eax,0x00000E00
		or ecx,eax
		shr eax,3
		and eax,0x00000100
		or ecx,eax

		; pixel #7
		;       xxxxxxxxxxxxxxxxxxxxxxxxIIIAxxxx
		;       xxxxxxxxAAAAIIIIxxxxxxxxxxxxxxxx
		;Alpha
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
		shl eax,12 ; Intensity
		and eax,0x000E0000
		or ecx,eax
		shr eax,3
		and eax,0x00010000
		or ecx,eax

		; pixel #8
		;       xxxxxxxxxxxxxxxxxxxxxxxxxxxxIIIA
		;       AAAAIIIIxxxxxxxxxxxxxxxxxxxxxxxx
		mov eax,edx
		shl eax,28 ;Alpha
		and eax,0x10000000
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		shl eax,1
		or ecx,eax
		mov eax,edx
		shl eax,24 ; Intensity
		and eax,0x0E000000
		or ecx,eax
		shr eax,3
		and eax,0x01000000
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }
; *

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
	}
}

/*****************************************************************
; Size: 0, Format: 4
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad4bI (wxUIntPtr src, int dst, wxUIntPtr wid_64, int height, int line, int ext)
{
	_asm {
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

mov eax,[esi]           ; read all 8 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword {
	xor ecx,ecx
		shr eax,28              ; 0xF0000000 -> 0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x0F000000 -> 0x00000F00
		shr eax,16
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shr eax,4               ; 0x00F00000 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,8               ; 0x000F0000 -> 0x0F000000
		and eax,0x0F000000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx
		mov eax,edx
		shr eax,12              ; 0x0000F000 -> 0x0000000F
		and eax,0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x00000F00 -> 0x00000F00
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,12              ; 0x000000F0 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		shl edx,24              ; 0x0000000F -> 0x0F000000
		and edx,0x0F000000
		or ecx,edx
		shl edx,4
		or ecx,edx

		mov [edi],ecx
		add edi,4
		; }

; * copy
mov eax,[esi]           ; read all 8 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword {
	xor ecx,ecx
		shr eax,28              ; 0xF0000000 -> 0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x0F000000 -> 0x00000F00
		shr eax,16
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shr eax,4               ; 0x00F00000 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,8               ; 0x000F0000 -> 0x0F000000
		and eax,0x0F000000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx
		mov eax,edx
		shr eax,12              ; 0x0000F000 -> 0x0000000F
		and eax,0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x00000F00 -> 0x00000F00
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,12              ; 0x000000F0 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		shl edx,24              ; 0x0000000F -> 0x0F000000
		and edx,0x0F000000
		or ecx,edx
		shl edx,4
		or ecx,edx

		mov [edi],ecx
		add edi,4
		; }
; *

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

mov eax,[esi+4]         ; read all 8 pixels
bswap eax
mov edx,eax

; 1st dword {
	xor ecx,ecx
		shr eax,28              ; 0xF0000000 -> 0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x0F000000 -> 0x00000F00
		shr eax,16
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shr eax,4               ; 0x00F00000 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,8               ; 0x000F0000 -> 0x0F000000
		and eax,0x0F000000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx
		mov eax,edx
		shr eax,12              ; 0x0000F000 -> 0x0000000F
		and eax,0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x00000F00 -> 0x00000F00
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,12              ; 0x000000F0 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		shl edx,24              ; 0x0000000F -> 0x0F000000
		and edx,0x0F000000
		or ecx,edx
		shl edx,4
		or ecx,edx

		mov [edi],ecx
		add edi,4
		; }

; * copy
mov eax,[esi]           ; read all 8 pixels
bswap eax
add esi,8
mov edx,eax

; 1st dword {
	xor ecx,ecx
		shr eax,28              ; 0xF0000000 -> 0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x0F000000 -> 0x00000F00
		shr eax,16
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shr eax,4               ; 0x00F00000 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,8               ; 0x000F0000 -> 0x0F000000
		and eax,0x0F000000
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov [edi],ecx
		add edi,4
		; }

; 2nd dword {
	xor ecx,ecx
		mov eax,edx
		shr eax,12              ; 0x0000F000 -> 0x0000000F
		and eax,0x0000000F
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx             ; 0x00000F00 -> 0x00000F00
		and eax,0x00000F00
		or ecx,eax
		shl eax,4
		or ecx,eax

		mov eax,edx
		shl eax,12              ; 0x000000F0 -> 0x000F0000
		and eax,0x000F0000
		or ecx,eax
		shl eax,4
		or ecx,eax

		shl edx,24              ; 0x0000000F -> 0x0F000000
		and edx,0x0F000000
		or ecx,edx
		shl edx,4
		or ecx,edx

		mov [edi],ecx
		add edi,4
		; }
; *

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
	}
}


/*****************************************************************
;
;            ******** Textures mirror/clamp/wrap ********
;
;*****************************************************************/

