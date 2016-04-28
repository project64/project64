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

#define NORMAL				    0
#define DO_DELAY_SLOT 			1
#define DELAY_SLOT 				2
#define DELAY_SLOT_DONE			3
#define DELAY_SLOT_EXIT			4
#define DELAY_SLOT_EXIT_DONE	5
#define JUMP	 				6
#define SINGLE_STEP	 		    7
#define SINGLE_STEP_DONE		8
#define FINISH_BLOCK			9
#define FINISH_SUB_BLOCK		10

extern DWORD RSP_NextInstruction, RSP_JumpTo, RSP_MfStatusCount;

/*
 * standard MIPS PC-relative branch
 * returns the new PC, based on whether the condition passes
 */
unsigned int RSP_branch_if(int condition);

void BuildInterpreterCPU(void);
DWORD RunInterpreterCPU(DWORD Cycles);
