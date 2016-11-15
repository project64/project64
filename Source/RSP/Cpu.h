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

#include "OpCode.h"

extern UDWORD EleSpec[32], Indx[32];

extern p_func RSP_Opcode[64];
extern p_func RSP_RegImm[32];
extern p_func RSP_Special[64];
extern p_func RSP_Cop0[32];
extern p_func RSP_Cop2[32];
extern p_func RSP_Vector[64];
extern p_func RSP_Lc2[32];
extern p_func RSP_Sc2[32];
extern uint32_t * PrgCount, RSP_Running;
extern OPCODE RSPOpC;

void SetCPU(DWORD core);
void Build_RSP (void);

extern DWORD Mfc0Count, SemaphoreExit;
