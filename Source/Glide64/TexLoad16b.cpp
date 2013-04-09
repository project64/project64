#include "Gfx #1.3.h"

/*****************************************************************
 16b textures load
*****************************************************************/

/*****************************************************************
; Size: 2, Format: 0
;
; 2008.03.29 cleaned up - H.Morii
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/

void asmLoad16bRGBA (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
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
	mov eax,[esi]   ; read both pixels
	mov ebx,[esi+4] ; read both pixels
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
	mov eax,[esi+4] ; read both pixels
	mov ebx,[esi]   ; read both pixels
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
	}
}

/****************************************************************
; Size: 2, Format: 3
;
; ** by Gugaman/Dave2001 **
;
; 2008.03.29 cleaned up - H.Morii
; 2009 ported to NASM - Sergey (Gonetz) Lipski
*/
void asmLoad16bIA (wxUIntPtr src, wxUIntPtr dst, int wid_64, int height, int line, int ext)
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
mov eax,[esi]   ; read both pixels
mov ebx,[esi+4] ; read both pixels
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
mov eax,[esi+4] ; read both pixels
mov ebx,[esi]   ; read both pixels
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
	}
}

/*****************************************************************
;16b textures mirror/clamp/wrap
;*****************************************************************/
void asmMirror16bS (int tex, int start, int width, int height, int mask, int line, int full, int count)
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
	}
}

void asmWrap16bS (int tex, int start, int height, int mask, int line, int full, int count)
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


void asmClamp16bS (int tex, int constant, int height,int line, int full, int count)
{
	_asm {
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

mov [edi],ax            ; don't unroll or make dword, it may go into next line (doesn't have to be multiple of two)
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
	}
}
