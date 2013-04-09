#include "Gfx #1.3.h"

/*****************************************************************
;32b textures mirror/clamp/wrap
;*****************************************************************/

void asmMirror32bS (int tex, int start, int width, int height, int mask, int line, int full, int count)
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
	}
}

void asmWrap32bS (int tex, int start, int height, int mask, int line, int full, int count)
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

void asmClamp32bS (int tex, int constant, int height,int line, int full, int count)
{
	_asm {
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

mov [edi],eax           ; don't unroll or make dword, it may go into next line (doesn't have to be multiple of two)
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
	}
}

