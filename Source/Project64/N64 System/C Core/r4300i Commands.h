/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#ifndef __r4300i_commands_h 
#define __r4300i_commands_h 

#ifdef __cplusplus
extern "C" {
#endif

#if (!defined(EXTERNAL_RELEASE))
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

#endif

