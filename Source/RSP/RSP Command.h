/*
 * RSP Compiler plug in for Project 64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
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

char * RSPOpcodeName ( DWORD OpCode, DWORD PC );

void DumpRSPCode (void);
void DumpRSPData (void);
void Disable_RSP_Commands_Window ( void );
void Enable_RSP_Commands_Window ( void );
void Enter_RSP_Commands_Window ( void );
void RefreshRSPCommands ( void );
void SetRSPCommandToRunning ( void );
void SetRSPCommandToStepping ( void );
void SetRSPCommandViewto ( UINT NewLocation );

extern DWORD Stepping_Commands, WaitingForStep;
extern BOOL InRSPCommandsWindow;

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
