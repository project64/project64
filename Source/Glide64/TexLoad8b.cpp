#include "Gfx #1.3.h"

/*****************************************************************
;8b textures load
;****************************************************************/

/*****************************************************************
; Size: 1, Format: 2
;
; 2008.03.29 cleaned up - H.Morii
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad8bCI (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
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

mov eax,[esi]           ; read all 4 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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
		; }

; * copy
mov eax,[esi]           ; read all 4 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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

mov eax,[esi+4]         ; read all 4 pixels
bswap eax
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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
		; }

; * copy
mov eax,[esi]           ; read all 4 pixels
bswap eax
mov edx,esi
add edx,8
mov esi,[src]
sub edx,esi
and edx,0x7FF
add esi,edx
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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

void asmLoad8bIA8 (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext, wxUIntPtr pal)
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

mov eax,[esi]           ; read all 4 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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
		; }

; * copy
mov eax,[esi]           ; read all 4 pixels
bswap eax
add esi,4
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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

mov eax,[esi+4]         ; read all 4 pixels
bswap eax
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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
		; }

; * copy
mov eax,[esi]           ; read all 4 pixels
bswap eax
add esi,8
mov edx,eax

; 1st dword output {
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
		; }

; 2nd dword output {
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
; Size: 1, Format: 3
;
; ** by Gugaman **
;
; 2008.03.29 cleaned up - H.Morii
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad8bIA4 (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
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
mov eax,[esi] ; read all 4 pixels
mov edx,eax

shr eax,4     ;all alpha
shl edx,4
and eax,0x0F0F0F0F
and edx,0xF0F0F0F0
add esi,4
or eax,edx

mov [edi],eax ; save dword
add edi,4

mov eax,[esi] ; read all 4 pixels
mov edx,eax

shr eax,4     ;all alpha
shl edx,4
and eax,0x0F0F0F0F
and edx,0xF0F0F0F0
add esi,4
or eax,edx

mov [edi],eax ; save dword
add edi,4
; *

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
mov eax,[esi+4] ; read both pixels
mov edx,eax

shr eax,4       ;all alpha
shl edx,4
and eax,0x0F0F0F0F
and edx,0xF0F0F0F0
or eax,edx

mov [edi],eax ;save dword
add edi,4

mov eax,[esi] ; read both pixels
add esi,8
mov edx,eax

shr eax,4     ;all alpha
shl edx,4
and eax,0x0F0F0F0F
and edx,0xF0F0F0F0
or eax,edx

mov [edi],eax ;save dword
add edi,4
; *

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
; Size: 1, Format: 4
;
; ** by Gugaman **
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad8bI (wxUIntPtr src, int dst, wxUIntPtr wid_64, int height, int line, int ext)
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
mov eax,[esi] ; read all 4 pixels
add esi,4

mov [edi],eax ; save dword
add edi,4

mov eax,[esi] ; read all 4 pixels
add esi,4

mov [edi],eax ; save dword
add edi,4
; *

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
mov eax,[esi+4] ; read both pixels

mov [edi],eax ;save dword
add edi,4

mov eax,[esi] ; read both pixels
add esi,8

mov [edi],eax ;save dword
add edi,4
; *

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

/*****************************************************************
;8b textures mirror/clamp/wrap
;*****************************************************************/

void asmMirror8bS (int tex, int start, int width, int height, int mask, int line, int full, int count)
{
	_asm{
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
	}
}

void asmWrap8bS (int tex, int start, int height, int mask, int line, int full, int count)
{
	_asm {
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
	}
}

void asmClamp8bS (int tex, int constant, int height,int line, int full, int count)
{
	_asm {
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

mov [edi],al            ; don't unroll or make dword, it may go into next line (doesn't have to be multiple of two)
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
	}
}
