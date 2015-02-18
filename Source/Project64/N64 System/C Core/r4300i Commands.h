/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
void Disable_R4300i_Commands_Window ( void );
void Enable_R4300i_Commands_Window ( void );
void __cdecl Enter_R4300i_Commands_Window ( void );
char * R4300iOpcodeName ( DWORD OpCode, DWORD PC );
void RefreshR4300iCommands ( void );
void SetR4300iCommandToRunning ( void );
void SetR4300iCommandToStepping ( void );
void SetR4300iCommandViewto ( UINT NewLocation );
void Update_r4300iCommandList (void);

extern BOOL InR4300iCommandsWindow;
#else
char * R4300iOpcodeName ( DWORD OpCode, DWORD PC );
#endif

/*
 * for CPU memory loads and stores
 *
 * The `offset` immediate is sign-extended, but C standard `sprintf` does not
 * natively let us convert to a hexadecimal with a sign prefix.
 */
#define SPRINTF_FIX_SIGNED_HEX(offset) {               \
    abs_offset     = (offset < 0) ? -offset : +offset; \
    sign_offset[0] = (offset < 0) ?     '-' :    '\0'; \
    sign_offset[1] = '\0';                             \
}

#ifdef __cplusplus
}
#endif
