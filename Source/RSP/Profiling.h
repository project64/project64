/*
 * RSP Compiler plug in for Project64 (A Nintendo 64 emulator).
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

enum SPECIAL_TIMERS {
	Timer_None          =  0, Timer_Compiling   = -1, Timer_RSP_Running   = -2, 
	Timer_R4300_Running = -3, Timer_RDP_Running = -5, Timer_RefreshScreen = -6,
	Timer_UpdateScreen = -7, Timer_UpdateFPS   = -9, Timer_Idel          = -10,
	Timer_FuncLookup   = -11,Timer_Done        = -13,
};

void  ResetTimerList       ( void );
DWORD StartTimer           ( DWORD Address );
void  StopTimer            ( void );
void  GenerateTimerResults ( void );
