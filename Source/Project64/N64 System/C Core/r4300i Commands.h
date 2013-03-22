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

#ifdef __cplusplus
}
#endif
