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

enum {
    NORMAL,
    DO_DELAY_SLOT,
    DELAY_SLOT,
    DELAY_SLOT_DONE,
    DELAY_SLOT_EXIT,
    DELAY_SLOT_EXIT_DONE,
    JUMP,
    SINGLE_STEP,
    SINGLE_STEP_DONE,
    FINISH_BLOCK,
    FINISH_SUB_BLOCK
};

extern DWORD RSP_NextInstruction, RSP_JumpTo, RSP_MfStatusCount;

/*
 * standard MIPS PC-relative branch
 * returns the new PC, based on whether the condition passes
 */
unsigned int RSP_branch_if(int condition);

void BuildInterpreterCPU(void);
DWORD RunInterpreterCPU(DWORD Cycles);
