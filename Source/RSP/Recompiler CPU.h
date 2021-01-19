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
#include "Types.h"

extern uint32_t CompilePC, NextInstruction, JumpTableSize;
extern Boolean ChangedPC;

#define CompilerWarning if (ShowErrors) DisplayError

#define High16BitAccum		1
#define Middle16BitAccum	2
#define Low16BitAccum		4
#define EntireAccum			(Low16BitAccum|Middle16BitAccum|High16BitAccum)

Boolean WriteToAccum(int Location, int PC);
Boolean WriteToVectorDest(DWORD DestReg, int PC);
Boolean UseRspFlags(int PC);

Boolean DelaySlotAffectBranch(DWORD PC);
Boolean CompareInstructions(DWORD PC, OPCODE * Top, OPCODE * Bottom);
Boolean IsOpcodeBranch(DWORD PC, OPCODE RspOp);
Boolean IsOpcodeNop(DWORD PC);

Boolean IsNextInstructionMmx(DWORD PC);
Boolean IsRegisterConstant(DWORD Reg, DWORD * Constant);

void RSP_Element2Mmx(int MmxReg);
void RSP_MultiElement2Mmx(int MmxReg1, int MmxReg2);

#define MainBuffer			0
#define SecondaryBuffer		1

DWORD RunRecompilerCPU ( DWORD Cycles );
void BuildRecompilerCPU ( void );

void CompilerRSPBlock ( void );
void CompilerToggleBuffer (void);
Boolean RSP_DoSections(void);

typedef struct {
	DWORD StartPC, CurrPC;		/* block start */
	
	struct {
		DWORD TargetPC;			/* Target for this unknown branch */
		DWORD * X86JumpLoc;		/* Our x86 dword to fill */
	} BranchesToResolve[200];	/* Branches inside or outside block */
	
	DWORD ResolveCount;			/* Branches with NULL jump table */
} RSP_BLOCK;

extern RSP_BLOCK CurrentBlock;

typedef struct {
    Boolean bIsRegConst[32];    /* Boolean toggle for constant */
	DWORD MipsRegConst[32];		/* Value of register 32-bit */
	DWORD BranchLabels[250];
	DWORD LabelCount;
	DWORD BranchLocations[250];
	DWORD BranchCount;
} RSP_CODE;

extern RSP_CODE RspCode;

#define IsRegConst(i)	(RspCode.bIsRegConst[i])
#define MipsRegConst(i) (RspCode.MipsRegConst[i])

typedef struct {
    Boolean mmx, mmx2, sse;     /* CPU specs and compiling */
    Boolean bFlags;             /* RSP Flag Analysis */
    Boolean bReOrdering;        /* Instruction reordering */
    Boolean bSections;          /* Microcode sections */
    Boolean bDest;              /* Vector destionation toggle */
    Boolean bAccum;             /* Accumulator toggle */
    Boolean bGPRConstants;      /* Analyze GPR constants */
    Boolean bAlignVector;       /* Align known vector loads */
    Boolean bAudioUcode;        /* Audio ucode analysis */
} RSP_COMPILER;

extern RSP_COMPILER Compiler;

#define IsMmxEnabled	(Compiler.mmx)
#define IsMmx2Enabled	(Compiler.mmx2)
#define IsSseEnabled	(Compiler.sse)
