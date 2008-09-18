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
#ifndef __breakpoints_h 
#define __breakpoints_h 

void __cdecl Enter_BPoint_Window    ( void );
int  Add_R4300iBPoint       ( DWORD Location, int Confirm );
int  CheckForR4300iBPoint   ( DWORD Location );
void __cdecl RefreshBreakPoints     ( void );
void RemoveR4300iBreakPoint ( DWORD Location );
void UpdateBPointGUI        ( void );
void UpdateBP_FunctionList  ( void );

#define MaxBPoints			0x100
#define R4300i_BP				1
#define R4300i_FUNCTION			2
#define RSP_BP					3

typedef struct {
   unsigned int Location;
} BPOINT;

extern BPOINT BPoint[MaxBPoints];
extern int	NoOfBpoints;

#endif
