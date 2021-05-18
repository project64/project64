/*
Project64 - A Nintendo 64 emulator
https://www.pj64-emu.com/
Copyright (C) 2012-2021 Project64. All rights reserved.
License:
GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html
*/

#define GLOBAL_FUNCTION(name)  \
    .align 2;                  \
    .global name;              \
    .hidden name;              \
    .type name, %function;     \
    name

#define TEXT_SECTION .text
#define END_SECTION

    .cpu arm9tdmi
    .file    "asm_functions.S"

TEXT_SECTION

GLOBAL_FUNCTION(__clear_cache_android):
    /*  The following bug-fix implements __clear_cache (missing in Android) */
    push   {r7, lr}
    mov    r2, #0
    mov    r7, #0x2
    add    r7, r7, #0xf0000
    svc    0x00000000
    pop    {r7, pc}

END_SECTION
