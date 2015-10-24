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

#include <windows.h>
#include <stdio.h>
#include "RSP.h"
#include "CPU.h"
#include "Interpreter CPU.h"
#include "Interpreter Ops.h"
#include "Recompiler CPU.h"
#include "RSP Command.h"
#include "RSP Registers.h"
#include "memory.h"
#include "dma.h"
#include "log.h"
#include "x86.h"
#include "Profiling.h"

#pragma warning(disable : 4152) // nonstandard extension, function/data pointer conversion in expression

extern BOOL AudioHle, GraphicsHle;
UWORD32 Recp, RecpResult, SQroot, SQrootResult;
DWORD ESP_RegSave = 0, EBP_RegSave = 0;
DWORD BranchCompare = 0;

/* align option affects: sw, lh, sh */
/* align option affects: lrv, ssv, lsv */

#define Compile_Immediates	/* ADDI, ADDIU, ANDI, ORI, XORI, LUI */
#define Compile_GPRLoads	/* LB, LH, LW, LBU, LHU */
#define Compile_GPRStores	/* SB, SH, SW */
#define Compile_Special		/* SLL, SRL, SRA, SRLV */
							/* XOR, OR, AND, SUB, SUBU, ADDU, ADD, SLT */
#define Compile_Cop0
#define Compile_Cop2

#define RSP_VectorMuls
#define RSP_VectorLoads
#define RSP_VectorMisc

#ifdef RSP_VectorMuls
#	define CompileVmulf	/* Verified 12/17/2000 - Jabo */
#	define CompileVmacf	/* Rewritten & Verified 12/15/2000 - Jabo */
#	define CompileVmudm	/* Verified 12/17/2000 - Jabo */
#	define CompileVmudh	/* Verified 12/17/2000 - Jabo */
#	define CompileVmudn	/* Verified 12/17/2000 - Jabo */
#	define CompileVmudl	/* Verified 12/17/2000 - Jabo */
#	define CompileVmadl	
#	define CompileVmadm	/* Verified 12/17/2000 - Jabo */
#	define CompileVmadh	/* Verified 12/15/2000 - Jabo */
#	define CompileVmadn	/* Verified 12/17/2000 - Jabo */
#endif
#ifdef RSP_VectorMisc
#	define CompileVne
#	define CompileVeq
#	define CompileVge
#	define CompileVlt
#	define CompileVrcp
#	define CompileVrcpl
#	define CompileVrsqh
#	define CompileVrcph
#	define CompileVsaw		/* Verified 12/17/2000 - Jabo */
#	define CompileVabs		/* Verified 12/15/2000 - Jabo */
#	define CompileVmov		/* Verified 12/17/2000 - Jabo */
#	define CompileVxor		/* Verified 12/17/2000 - Jabo */
#	define CompileVor		/* Verified 12/17/2000 - Jabo */
#	define CompileVand		/* Verified 12/17/2000 - Jabo */
#	define CompileVsub		/* Verified 12/17/2000 - Jabo (watch flags) */
#	define CompileVadd		/* Verified 12/17/2000 - Jabo (watch flags) */
#	define CompileVaddc
#	define CompileVsubc
#	define CompileVmrg
#	define CompileVnxor
#	define CompileVnor
#	define CompileVnand
#endif
#ifdef RSP_VectorLoads
#	define CompileSqv		/* Verified 12/17/2000 - Jabo */
#	define CompileSdv		/* Verified 12/17/2000 - Jabo */
#	define CompileSsv		/* Verified 12/17/2000 - Jabo */
#	define CompileLrv		/* Rewritten & Verified 12/17/2000 - Jabo */
#	define CompileLqv		/* Verified 12/17/2000 - Jabo */
#	define CompileLdv		/* Verified 12/17/2000 - Jabo */
#	define CompileLsv		/* Verified 12/17/2000 - Jabo */
#	define CompileLlv		/* Verified 12/17/2000 - Jabo */
#	define CompileSlv
#endif

void Branch_AddRef(DWORD Target, DWORD * X86Loc) {
	if (CurrentBlock.ResolveCount >= 150) {
		CompilerWarning("Out of branch reference space");
	} else {
		BYTE * KnownCode = *(JumpTable + (Target >> 2));

		if (KnownCode == NULL) {
			DWORD i = CurrentBlock.ResolveCount;
			CurrentBlock.BranchesToResolve[i].TargetPC = Target;
			CurrentBlock.BranchesToResolve[i].X86JumpLoc = X86Loc;
			CurrentBlock.ResolveCount += 1;
		} else {
			CPU_Message("      (static jump to %X)", KnownCode);
			x86_SetBranch32b((DWORD*)X86Loc, (DWORD*)KnownCode);
		}
	}
}

void Cheat_r4300iOpcode(p_func FunctAddress, char * FunctName) {
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	MoveConstToVariable(RSPOpC.Hex, &RSPOpC.Hex, "RSPOpC.Hex" );
	Call_Direct(FunctAddress, FunctName);
}

void Cheat_r4300iOpcodeNoMessage(p_func FunctAddress, char * FunctName) {
	MoveConstToVariable(RSPOpC.Hex, &RSPOpC.Hex, "RSPOpC.Hex" );
	Call_Direct(FunctAddress, FunctName);
}

void x86_SetBranch8b(void * JumpByte, void * Destination) {
	/* calculate 32-bit relative offset */
	size_t n = (BYTE*)Destination - ((BYTE*)JumpByte + 1);
	SSIZE_T signed_n = (SSIZE_T)n;

	/* check limits, no pun intended */
	if (signed_n > +128 || signed_n < -127) {
		CompilerWarning(
			"FATAL: Jump out of 8b range %i (PC = %04X)", n, CompilePC
		);
	} else {
		*(uint8_t *)(JumpByte) = (uint8_t)(n & 0xFF);
	}
}

void x86_SetBranch32b(void * JumpByte, void * Destination) {
	*(DWORD*)(JumpByte) = (DWORD)((BYTE*)Destination - (BYTE*)((DWORD*)JumpByte + 1));
}

void BreakPoint() {
	CPU_Message("      int 3");
	*(RecompPos++) = 0xCC;
}

void CompileBranchExit(DWORD TargetPC, DWORD ContinuePC)
{
	DWORD * X86Loc = NULL;

	NextInstruction = FINISH_SUB_BLOCK;
	CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
	JeLabel32("BranchEqual", 0);
	X86Loc = (DWORD*)(RecompPos - 4);
	MoveConstToVariable(ContinuePC,PrgCount,"RSP PC");
	Ret();

	CPU_Message("BranchEqual:");
	x86_SetBranch32b(X86Loc, RecompPos);
	MoveConstToVariable(TargetPC,PrgCount,"RSP PC");
	Ret();
}

/************************* OpCode functions *************************/
void Compile_SPECIAL ( void ) {
	RSP_Special[ RSPOpC.funct ]();
}

void Compile_REGIMM ( void ) {
	RSP_RegImm[ RSPOpC.rt ]();
}

void Compile_J ( void ) {
	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		JmpLabel32 ( "BranchToJump", 0 );
		Branch_AddRef((RSPOpC.target << 2) & 0xFFC, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		MoveConstToVariable((RSPOpC.target << 2) & 0xFFC,PrgCount,"RSP PC");
		NextInstruction = FINISH_SUB_BLOCK;
		Ret();
	} else {
		CompilerWarning("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_JAL ( void ) {
	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		MoveConstToVariable(CompilePC + 8, &RSP_GPR[31].UW, "RA.W");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		// before we branch quickly update our stats
		if (Profiling && IndvidualBlock)
		{
			char Str[40];
			sprintf(Str,"%03X",(RSPOpC.target << 2) & 0xFFC);
			Push(x86_EAX);
			PushImm32(Str,*PrgCount);
			Call_Direct(StartTimer, "StartTimer");
			AddConstToX86Reg(x86_ESP, 4);
			Pop(x86_EAX);
		}
		JmpLabel32 ( "BranchToJump", 0 );
		Branch_AddRef((RSPOpC.target << 2) & 0xFFC, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		MoveConstToVariable((RSPOpC.target << 2) & 0xFFC,PrgCount,"RSP PC");
		NextInstruction = FINISH_SUB_BLOCK;
		Ret();
	} else {
		CompilerWarning("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_BEQ ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		if (RSPOpC.rt == 0) {
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		} else if (RSPOpC.rs == 0) {			
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
		} else {
			MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
			CompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		}
		SetzVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {
			JmpLabel32 ( "BranchToJump", 0 );
			Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			if (RSPOpC.rt == 0) {
				CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			} else if (RSPOpC.rs == 0) {			
				CompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
			} else {
				MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
				CompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			}
			JeLabel32("BranchEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchEqual", 0);
		}
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BEQ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_BNE ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}

		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		if (RSPOpC.rt == 0) {			
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		} else if (RSPOpC.rs == 0) {			
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
		} else {
			MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
			CompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		}
		SetnzVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0 && RSPOpC.rt == 0) {			
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}

		if (FALSE == bDelayAffect) {
			if (RSPOpC.rt == 0) {			
				CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			} else if (RSPOpC.rs == 0) {			
				CompConstToVariable(0,&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt));
			} else {
				MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W,GPR_Name(RSPOpC.rt),x86_EAX);
				CompX86regToVariable(x86_EAX,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			}
			JneLabel32("BranchNotEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchNotEqual", 0);
		}
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BNE error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_BLEZ ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		if (RSPOpC.rs == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		SetleVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {
			JmpLabel32 ( "BranchToJump", 0 );
			Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			JleLabel32("BranchLessEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchLessEqual", 0);
		}

		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BLEZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_BGTZ ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		if (RSPOpC.rs == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		SetgVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {			
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			JgLabel32("BranchGreater", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchGreater", 0);
		}
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BGTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_ADDI ( void ) {
	int Immediate = (short)RSPOpC.immediate;

	#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_ADDI,"RSP_Opcode_ADDI"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		if (Immediate != 0) {
			AddConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		}
	} else if (RSPOpC.rs == 0) {
		MoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if ((IsRegConst(RSPOpC.rs) & 1) != 0) {
		MoveConstToVariable(MipsRegConst(RSPOpC.rs) + Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			AddConstToX86Reg(x86_EAX, Immediate);
		}
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void Compile_ADDIU ( void ) {
	int Immediate = (short)RSPOpC.immediate;

	#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_ADDIU,"RSP_Opcode_ADDIU"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		if (Immediate != 0) {
			AddConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		}
	} else if (RSPOpC.rs == 0) {
		MoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			AddConstToX86Reg(x86_EAX, Immediate);
		}
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void Compile_SLTI ( void ) {
#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_SLTI,"RSP_Opcode_SLTI");
#endif
	int Immediate;

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));
	
	if (RSPOpC.rt == 0) return;

	Immediate = (short)RSPOpC.immediate;
	XorX86RegToX86Reg(x86_ECX, x86_ECX);
	CompConstToVariable(Immediate, &RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs));
	Setl(x86_ECX);
	MoveX86regToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
}

void Compile_SLTIU ( void ) {
#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_SLTIU,"RSP_Opcode_SLTIU");
#endif
	int Immediate;

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));
	
	if (RSPOpC.rt == 0) return;

	Immediate = (short)RSPOpC.immediate;
	XorX86RegToX86Reg(x86_ECX, x86_ECX);
	CompConstToVariable(Immediate, &RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs));
	Setb(x86_ECX);
	MoveX86regToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
}

void Compile_ANDI ( void ) {
	int Immediate = (unsigned short)RSPOpC.immediate;

	#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_ANDI,"RSP_Opcode_ANDI"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		AndConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if (RSPOpC.rs == 0) {
		MoveConstToVariable(0, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		AndConstToX86Reg(x86_EAX, Immediate);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void Compile_ORI ( void ) {
	int Immediate = (unsigned short)RSPOpC.immediate;

	#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_ORI,"RSP_Opcode_ORI"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		OrConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else if (RSPOpC.rs == 0) {
		MoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			OrConstToX86Reg(Immediate, x86_EAX);
		}
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void Compile_XORI ( void ) {
	int Immediate = (unsigned short)RSPOpC.immediate;

	#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_XORI,"RSP_Opcode_XORI"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rt == 0) return;

	if (RSPOpC.rt == RSPOpC.rs) {
		XorConstToVariable(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), Immediate);
	} else if (RSPOpC.rs == 0) {
		MoveConstToVariable(Immediate, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
		if (Immediate != 0) {
			XorConstToX86Reg(x86_EAX, Immediate);
		}
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
	}
}

void Compile_LUI ( void ) {
	int constant = (short)RSPOpC.offset << 16;

	#ifndef Compile_Immediates
	Cheat_r4300iOpcode(RSP_Opcode_LUI,"RSP_Opcode_LUI"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rt == 0) return;
	MoveConstToVariable(constant, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
}

void Compile_COP0 (void) {
	RSP_Cop0[ RSPOpC.rs ]();
}

void Compile_COP2 (void) {
	RSP_Cop2[ RSPOpC.rs ]();
}

void Compile_LB ( void ) {
	int Offset = (short)RSPOpC.offset;

	if (RSPOpC.rt == 0)
		return;
	#ifndef Compile_GPRLoads
	Cheat_r4300iOpcode(RSP_Opcode_LB,"RSP_Opcode_LB"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	
	if (IsRegConst(RSPOpC.base) == TRUE) {
		char Address[32];
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 3;
		Addr &= 0xfff;
		
		sprintf(Address, "Dmem + %Xh", Addr);
		MoveSxVariableToX86regByte(RSPInfo.DMEM + Addr, Address, x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
	XorConstToX86Reg(x86_EBX, 3);
	AndConstToX86Reg(x86_EBX, 0x0fff);

	MoveSxN64MemToX86regByte(x86_EAX, x86_EBX);
	MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
}

void Compile_LH ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	if (RSPOpC.rt == 0)
		return;
	#ifndef Compile_GPRLoads
	Cheat_r4300iOpcode(RSP_Opcode_LH,"RSP_Opcode_LH"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	
	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
		Addr &= 0xfff;

		if ((Addr & 1) != 0) {
			if ((Addr & 2) == 0) {
				CompilerWarning("Unaligned LH at constant address PC = %04X", CompilePC);
				Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LH,"RSP_Opcode_LH");
			} else {
				char Address[32];
				sprintf(Address, "Dmem + %Xh", Addr);
				MoveSxVariableToX86regHalf(RSPInfo.DMEM + (Addr ^ 2), Address, x86_EAX);
				MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			}
		} else {
			char Address[32];
			sprintf(Address, "Dmem + %Xh", Addr);
			MoveSxVariableToX86regHalf(RSPInfo.DMEM + Addr, Address, x86_EAX);
			MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		}
		return;
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);

	AndConstToX86Reg(x86_EBX, 0x0fff);
	TestConstToX86Reg(1, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	x86_SetBranch32b(Jump[0], RecompPos);

	Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LH,"RSP_Opcode_LH");

	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;

	CompilerToggleBuffer();

	XorConstToX86Reg(x86_EBX, 2);

	MoveSxN64MemToX86regHalf(x86_EAX, x86_EBX);
	MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

	CPU_Message("   Done:");
	x86_SetBranch32b(Jump[1], RecompPos);
}

void Compile_LW ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	if (RSPOpC.rt == 0)
		return;
	#ifndef Compile_GPRLoads
	Cheat_r4300iOpcode(RSP_Opcode_LW,"RSP_Opcode_LW"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	
	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) & 0xfff;

		if ((Addr & 1) != 0) {
			CompilerWarning("Unaligned LW at constant address PC = %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LW,"RSP_Opcode_LW");
		} else if ((Addr & 2) != 0) {
			char Address[32];
			sprintf(Address, "Dmem + %Xh", Addr - 2);
			MoveVariableToX86regHalf(RSPInfo.DMEM + Addr - 2, Address, x86_EAX);
			sprintf(Address, "Dmem + %Xh", Addr);
			MoveVariableToX86regHalf(RSPInfo.DMEM + Addr + 4, Address, x86_ECX);
			
			MoveX86regHalfToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UHW[1], GPR_Name(RSPOpC.rt));
			MoveX86regHalfToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UHW[0], GPR_Name(RSPOpC.rt));
		} else {
			char Address[32];
			sprintf(Address, "Dmem + %Xh", Addr);
			MoveVariableToX86reg(RSPInfo.DMEM + Addr, Address, x86_EAX);
			MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		}
		return;
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
	
	AndConstToX86Reg(x86_EBX, 0x0fff);
	TestConstToX86Reg(3, x86_EBX);
	JneLabel32("UnAligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();

	x86_SetBranch32b(Jump[0], RecompPos);
	CPU_Message("   Unaligned:");

	LeaSourceAndOffset(x86_ECX , x86_EBX, 2);
	LeaSourceAndOffset(x86_EDX , x86_EBX, 3);
	MoveX86RegToX86Reg(x86_EBX, x86_EAX);
	AddConstToX86Reg(x86_EBX, 1);
	
	XorConstToX86Reg(x86_EAX, 3);
	XorConstToX86Reg(x86_EBX, 3);
	XorConstToX86Reg(x86_ECX, 3);
	XorConstToX86Reg(x86_EDX, 3);
	MoveN64MemToX86regByte(x86_EAX, x86_EAX);
	MoveN64MemToX86regByte(x86_EBX, x86_EBX);
	MoveN64MemToX86regByte(x86_ECX, x86_ECX);
	MoveN64MemToX86regByte(x86_EDX, x86_EDX);
	MoveX86regByteToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UB[3], GPR_Name(RSPOpC.rt));
	MoveX86regByteToVariable(x86_EBX, &RSP_GPR[RSPOpC.rt].UB[2], GPR_Name(RSPOpC.rt));
	MoveX86regByteToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UB[1], GPR_Name(RSPOpC.rt));
	MoveX86regByteToVariable(x86_EDX, &RSP_GPR[RSPOpC.rt].UB[0], GPR_Name(RSPOpC.rt));

	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;
	CompilerToggleBuffer();

	MoveN64MemToX86reg(x86_EAX, x86_EBX);
	MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

	CPU_Message("   Done:");
	x86_SetBranch32b(Jump[1], RecompPos);
}

void Compile_LBU ( void ) {
	int Offset = (short)RSPOpC.offset;

	if (RSPOpC.rt == 0)
		return;
	#ifndef Compile_GPRLoads
	Cheat_r4300iOpcode(RSP_Opcode_LBU,"RSP_Opcode_LBU"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		char Address[32];
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 3;
		Addr &= 0xfff;

		sprintf(Address, "Dmem + %Xh", Addr);
		MoveZxVariableToX86regByte(RSPInfo.DMEM + Addr, Address, x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	XorX86RegToX86Reg(x86_EAX, x86_EAX);

	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
	XorConstToX86Reg(x86_EBX, 3);
	AndConstToX86Reg(x86_EBX, 0x0fff);

	MoveN64MemToX86regByte(x86_EAX, x86_EBX);
	MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
}

void Compile_LHU ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	if (RSPOpC.rt == 0)
		return;
	#ifndef Compile_GPRLoads
	Cheat_r4300iOpcode(RSP_Opcode_LHU,"RSP_Opcode_LHU"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
		Addr &= 0xfff;

		if ((Addr & 1) != 0) {
			if ((Addr & 2) == 0) {
				CompilerWarning("Unaligned LHU at constant address PC = %04X", CompilePC);
				Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LHU, "RSP_Opcode_LHU");
			} else {
				char Address[32];
				sprintf(Address, "Dmem + %Xh", Addr);
				MoveZxVariableToX86regHalf(RSPInfo.DMEM + (Addr ^ 2), Address, x86_ECX);
				MoveX86regToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			}
			return;
		} else {
			char Address[32];
			sprintf(Address, "Dmem + %Xh", Addr);
			MoveZxVariableToX86regHalf(RSPInfo.DMEM + Addr, Address, x86_ECX);
			MoveX86regToVariable(x86_ECX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			return;
		}
	}

	/*
	 * should really just do it by bytes but whatever for now
	 */
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) {
		AddConstToX86Reg(x86_EBX, Offset);
	}
	TestConstToX86Reg(1, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	x86_SetBranch32b(Jump[0], RecompPos);
	Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LHU,"RSP_Opcode_LHU");
	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;
	CompilerToggleBuffer();

	XorConstToX86Reg(x86_EBX, 2);
	AndConstToX86Reg(x86_EBX, 0x0fff);
	MoveZxN64MemToX86regHalf(x86_EAX, x86_EBX);
	MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

	CPU_Message("   Done:");
	x86_SetBranch32b(Jump[1], RecompPos);
}

void Compile_SB ( void ) {
	int Offset = (short)RSPOpC.offset;

	#ifndef Compile_GPRStores
	Cheat_r4300iOpcode(RSP_Opcode_SB,"RSP_Opcode_SB"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);

	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
	XorConstToX86Reg(x86_EBX, 3);
	AndConstToX86Reg(x86_EBX, 0x0fff);

	MoveX86regByteToN64Mem(x86_EAX, x86_EBX);
}

void Compile_SH ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef Compile_GPRStores
	Cheat_r4300iOpcode(RSP_Opcode_SH,"RSP_Opcode_SH"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
		Addr &= 0xfff;

		if ((Offset & 1) != 0) {
			CompilerWarning("Unaligned SH at constant address PC = %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SH,"RSP_Opcode_SH");
			return;
		} else {
			char Address[32];
			sprintf(Address, "Dmem + %Xh", Addr);
			MoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
			MoveX86regHalfToVariable(x86_EAX, RSPInfo.DMEM + Addr, Address);
			return;
		}
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);

	TestConstToX86Reg(1, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	x86_SetBranch32b(Jump[0], RecompPos);

	X86BreakPoint(__FILE__,__LINE__);
	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;

	CompilerToggleBuffer();

	XorConstToX86Reg(x86_EBX, 2);
	AndConstToX86Reg(x86_EBX, 0x0fff);

	MoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
	MoveX86regHalfToN64Mem(x86_EAX, x86_EBX);

	CPU_Message("   Done:");
	x86_SetBranch32b(Jump[1], RecompPos);
}

void Compile_SW ( void ) {
	int Offset = (short)RSPOpC.offset;
	BYTE * Jump[2];

	#ifndef Compile_GPRStores
	Cheat_r4300iOpcode(RSP_Opcode_SW,"RSP_Opcode_SW"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + Offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned SW at constant address PC = %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SW,"RSP_Opcode_SW");
			return;
		} else {
			char Address[32];
			sprintf(Address, "Dmem + %Xh", Addr);
			MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
			MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr, Address);
			return;
		}
	} 

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
	
	AndConstToX86Reg(x86_EBX, 0x0fff);
	TestConstToX86Reg(3, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	x86_SetBranch32b(Jump[0], RecompPos);

//	X86BreakPoint(__FILE__,__LINE__);

	Push(x86_EBX);
	LeaSourceAndOffset(x86_ECX , x86_EBX, 2);
	LeaSourceAndOffset(x86_EDX , x86_EBX, 3);
	XorConstToX86Reg(x86_ECX, 3);
	XorConstToX86Reg(x86_EDX, 3);
	MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].UB[1], GPR_Name(RSPOpC.rt),x86_EAX); //cx
	MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].UB[0], GPR_Name(RSPOpC.rt),x86_EBX); //dx
	MoveX86regByteToN64Mem(x86_EAX, x86_ECX);
	MoveX86regByteToN64Mem(x86_EBX, x86_EDX);
	Pop(x86_EBX);
	
	MoveX86RegToX86Reg(x86_EBX, x86_EAX);
	AddConstToX86Reg(x86_EBX, 1);
	XorConstToX86Reg(x86_EAX, 3);
	XorConstToX86Reg(x86_EBX, 3);
	
	MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].UB[3], GPR_Name(RSPOpC.rt),x86_ECX); //ax
	MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].UB[2], GPR_Name(RSPOpC.rt),x86_EDX); //bx

	MoveX86regByteToN64Mem(x86_ECX, x86_EAX);
	MoveX86regByteToN64Mem(x86_EDX, x86_EBX);

	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;

	CompilerToggleBuffer();

	if (RSPOpC.rt == 0) {
		XorX86RegToX86Reg(x86_EAX,x86_EAX);
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
	}
	MoveX86regToN64Mem(x86_EAX, x86_EBX);
	
	CPU_Message("   Done:");
	x86_SetBranch32b(Jump[1], RecompPos);
}

void Compile_LC2 (void) {
	RSP_Lc2 [ RSPOpC.rd ]();
}

void Compile_SC2 (void) {
	RSP_Sc2 [ RSPOpC.rd ]();
}
/********************** R4300i OpCodes: Special **********************/

void Compile_Special_SLL ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SLL,"RSP_Special_SLL"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rt) {
		ShiftLeftSignVariableImmed(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (BYTE)RSPOpC.sa);
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		ShiftLeftSignImmed(x86_EAX, (BYTE)RSPOpC.sa);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_SRL ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SRL,"RSP_Special_SRL"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rt) {
		ShiftRightUnsignVariableImmed(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (BYTE)RSPOpC.sa);
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		ShiftRightUnsignImmed(x86_EAX, (BYTE)RSPOpC.sa);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_SRA ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SRA,"RSP_Special_SRA"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rt) {
		ShiftRightSignVariableImmed(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (BYTE)RSPOpC.sa);
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		ShiftRightSignImmed(x86_EAX, (BYTE)RSPOpC.sa);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_SLLV ( void ) {
	Cheat_r4300iOpcode(RSP_Special_SLLV,"RSP_Special_SLLV");
}

void Compile_Special_SRLV ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SRLV,"RSP_Special_SRLV"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	if (RSPOpC.rd == 0) return;

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_ECX);
	AndConstToX86Reg(x86_ECX, 0x1F);
	ShiftRightUnsign(x86_EAX);
	MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
}

void Compile_Special_SRAV ( void ) {
	Cheat_r4300iOpcode(RSP_Special_SRAV,"RSP_Special_SRAV");
}

void UpdateAudioTimer() {
/*	char Label[100];
	sprintf(Label,"COMMAND: %02X (PC = %08X)",RSP_GPR[1].UW >> 1, *PrgCount);
	StartTimer(Label);*/
}

void Compile_Special_JR (void) {
	BYTE * Jump;

	if ( NextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		/* transfer destination to location pointed to by PrgCount */
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs),x86_EAX);
		AndConstToX86Reg(x86_EAX,0xFFC);
		MoveX86regToVariable(x86_EAX,PrgCount,"RSP PC");
		ChangedPC = TRUE;
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		MoveVariableToX86reg(PrgCount,"RSP PC", x86_EAX);
		if (Profiling && IndvidualBlock)
		{
			Push(x86_EAX);
			Push(x86_EAX);
			Call_Direct(StartTimer, "StartTimer");
			AddConstToX86Reg(x86_ESP, 4);
			Pop(x86_EAX);
		}
		AddVariableToX86reg(x86_EAX, &JumpTable, "JumpTable");
		MoveX86regPointerToX86reg(x86_EAX, x86_EAX);

		TestX86RegToX86Reg(x86_EAX, x86_EAX);
		JeLabel8("Null", 0);
		Jump = RecompPos - 1;

		// before we branch quickly update our stats
		/*if (CompilePC == 0x080) {
			Pushad();
			Call_Direct(UpdateAudioTimer, "UpdateAudioTimer");
			Popad();
		}*/
		JumpX86Reg(x86_EAX);

		x86_SetBranch8b(Jump, RecompPos);
		CPU_Message(" Null:");
		Ret();
		ChangedPC = FALSE;
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		NextInstruction = FINISH_SUB_BLOCK;
		Ret();
	} else {
		CompilerWarning("WTF\n\nJR\nNextInstruction = %X", NextInstruction);
		BreakPoint();
	}
}

void Compile_Special_JALR ( void ) {
	BYTE * Jump;
	DWORD Const = (CompilePC + 8) & 0xFFC;

	if (NextInstruction == NORMAL) {		
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		MoveConstToVariable(Const, &RSP_GPR[RSPOpC.rd].W,GPR_Name(RSPOpC.rd));
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs),x86_EAX);
		AndConstToX86Reg(x86_EAX,0xFFC);
		MoveX86regToVariable(x86_EAX,PrgCount,"RSP PC");
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE) {
		MoveVariableToX86reg(PrgCount,"RSP PC", x86_EAX);
		AddVariableToX86reg(x86_EAX, &JumpTable, "JumpTable");
		MoveX86regPointerToX86reg(x86_EAX, x86_EAX);

		TestX86RegToX86Reg(x86_EAX, x86_EAX);
		JeLabel8("Null", 0);
		Jump = RecompPos - 1;
		JumpX86Reg(x86_EAX);

		x86_SetBranch8b(Jump, RecompPos);
		CPU_Message(" Null:");
		Ret();
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		NextInstruction = FINISH_SUB_BLOCK;
		Ret();
	} else {
		CompilerWarning("WTF\n\nJALR\nNextInstruction = %X", NextInstruction);
		BreakPoint();
	}
}

void Compile_Special_BREAK ( void ) {
	Cheat_r4300iOpcode(RSP_Special_BREAK,"RSP_Special_BREAK");
	if (NextInstruction == NORMAL) {
		MoveConstToVariable(CompilePC + 4,PrgCount,"RSP PC");
		Ret();
		NextInstruction = FINISH_SUB_BLOCK;
	} else {
		CompilerWarning("WTF\n\nBREAK\nNextInstruction = %X", NextInstruction);
		BreakPoint();
	}
}

void Compile_Special_ADD ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_ADD,"RSP_Special_ADD"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		AddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rd == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AddX86RegToX86Reg(x86_EAX, x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == 0) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rt == 0) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AddVariableToX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_ADDU ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_ADDU,"RSP_Special_ADDU"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		AddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rd == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AddX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AddX86RegToX86Reg(x86_EAX, x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == 0) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rt == 0) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AddVariableToX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_SUB ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SUB,"RSP_Special_SUB"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		SubX86regFromVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, "RSP_GPR[RSPOpC.rd].W");
	} else if (RSPOpC.rs == RSPOpC.rt) {
		MoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		SubVariableFromX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_SUBU ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SUBU,"RSP_Special_SUBU"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		SubX86regFromVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rs == RSPOpC.rt) {
		MoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		SubVariableFromX86reg(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_AND ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_AND,"RSP_Special_AND"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		AndX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rd == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AndX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rs == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		AndVariableToX86Reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_OR ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_OR,"RSP_Special_OR"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		OrX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rd == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		OrX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rs == 0) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else if (RSPOpC.rt == 0) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		OrVariableToX86Reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_XOR ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_XOR,"RSP_Special_XOR"); return;
	#endif
	
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.rd == 0) return;

	if (RSPOpC.rd == RSPOpC.rs) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		XorX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rd == RSPOpC.rt) {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		XorX86RegToVariable(&RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
	} else if (RSPOpC.rs == RSPOpC.rt) {
		MoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	} else {
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
		XorVariableToX86reg(&RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_NOR ( void ) {
	Cheat_r4300iOpcode(RSP_Special_NOR,"RSP_Special_NOR");
}

void Compile_Special_SLT ( void ) {
	#ifndef Compile_Special
	Cheat_r4300iOpcode(RSP_Special_SLT,"RSP_Special_SLT"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	if (RSPOpC.rt == 0) { return; }

	if (RSPOpC.rt == RSPOpC.rs) {
		MoveConstToVariable(0, &RSP_GPR[RSPOpC.rd].UW, GPR_Name(RSPOpC.rd));
	} else {
		XorX86RegToX86Reg(x86_EBX, x86_EBX);
		if (RSPOpC.rs == 0) {
			MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
			CompConstToX86reg(x86_EAX, 0);
			Setg(x86_EBX);
		} else if (RSPOpC.rt == 0) {
			MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
			CompConstToX86reg(x86_EAX, 0);
			Setl(x86_EBX);
		} else {
			MoveVariableToX86reg(&RSP_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
			CompX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
			Setl(x86_EBX);
		}
		MoveX86regToVariable(x86_EBX, &RSP_GPR[RSPOpC.rd].UW, GPR_Name(RSPOpC.rd));
	}
}

void Compile_Special_SLTU ( void ) {
	Cheat_r4300iOpcode(RSP_Special_SLTU,"RSP_Special_SLTU");
}

/********************** R4300i OpCodes: RegImm **********************/
void Compile_RegImm_BLTZ ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		if (RSPOpC.rs == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		SetlVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			JlLabel32("BranchLess", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchLess", 0);
		}
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BLTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nPC = %X\nEmulation will now stop", NextInstruction, CompilePC);
		BreakPoint();
	}
}

void Compile_RegImm_BGEZ ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		if (RSPOpC.rs == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		SetgeVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {			
			JmpLabel32 ( "BranchToJump", 0 );
			Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			JgeLabel32("BranchGreaterEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchGreaterEqual", 0);
		}
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BGEZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_RegImm_BLTZAL ( void ) {
	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		MoveConstToVariable(CompilePC + 8, &RSP_GPR[31].UW, "RA.W");
		if (RSPOpC.rs == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		SetlVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}

		/* take a look at the branch compare variable */
		CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
		JeLabel32("BranchLessEqual", 0);
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BLTZAL error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

void Compile_RegImm_BGEZAL ( void ) {
	static BOOL bDelayAffect;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
		MoveConstToVariable(CompilePC + 8, &RSP_GPR[31].UW, "RA.W");
		if (RSPOpC.rs == 0) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		bDelayAffect = DelaySlotAffectBranch(CompilePC);
		if (FALSE == bDelayAffect) {
			NextInstruction = DO_DELAY_SLOT;
			return;
		}
		CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
		SetgeVariable(&BranchCompare, "BranchCompare");
		NextInstruction = DO_DELAY_SLOT;
	} else if ( NextInstruction == DELAY_SLOT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		
		if (RSPOpC.rs == 0) {			
			JmpLabel32 ( "BranchToJump", 0 );
			Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
			NextInstruction = FINISH_SUB_BLOCK;
			return;
		}
		if (FALSE == bDelayAffect) {
			CompConstToVariable(0,&RSP_GPR[RSPOpC.rs].W,GPR_Name(RSPOpC.rs));
			JgeLabel32("BranchGreaterEqual", 0);
		} else {
			/* take a look at the branch compare variable */
			CompConstToVariable(TRUE, &BranchCompare, "BranchCompare");
			JeLabel32("BranchGreaterEqual", 0);
		}
		Branch_AddRef(Target, (DWORD*)(RecompPos - 4));
		NextInstruction = FINISH_SUB_BLOCK;
	} else if ( NextInstruction == DELAY_SLOT_EXIT_DONE ) {
		DWORD Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
		CompileBranchExit(Target, CompilePC + 8);
	} else {
		CompilerWarning("BGEZAL error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
}

/************************** Cop0 functions *************************/

void Compile_Cop0_MF ( void ) {
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	if (LogRDP)
	{		
		char str[40];

		sprintf(str,"%d",RSPOpC.rd);
		PushImm32(str,RSPOpC.rd);
		sprintf(str,"%X",CompilePC);
		PushImm32(str,CompilePC);
		Call_Direct(RDP_LogMF0,"RDP_LogMF0");
		AddConstToX86Reg(x86_ESP, 8);
	}

#ifndef Compile_Cop0
	Cheat_r4300iOpcode(RSP_Cop0_MF,"RSP_Cop0_MF");
	if (NextInstruction == NORMAL)
	{
		MoveConstToVariable(CompilePC + 4,PrgCount,"RSP PC");
		Ret();
		NextInstruction = FINISH_SUB_BLOCK;
	} else if (NextInstruction == DELAY_SLOT) {
		NextInstruction = DELAY_SLOT_EXIT;
	} else {
		CompilerWarning("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
		BreakPoint();
	}
	return;
#else
	switch (RSPOpC.rd) {
	case 0: 
		MoveVariableToX86reg(RSPInfo.SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 1: 
		MoveVariableToX86reg(RSPInfo.SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 5: 
		MoveVariableToX86reg(RSPInfo.SP_DMA_FULL_REG, "SP_DMA_FULL_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 6: 
		MoveVariableToX86reg(RSPInfo.SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 4: 
		MoveVariableToX86reg(&RSP_MfStatusCount, "RSP_MfStatusCount", x86_ECX);
		MoveVariableToX86reg(RSPInfo.SP_STATUS_REG, "SP_STATUS_REG", x86_EAX);
		CompConstToX86reg(x86_ECX, 10);
		JbLabel8("label", 10);
		MoveConstToVariable(0, &RSP_Running, "RSP_Running");
		IncX86reg(x86_ECX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		MoveX86regToVariable(x86_ECX, &RSP_MfStatusCount, "RSP_MfStatusCount");
		if (NextInstruction == NORMAL)
		{
			MoveConstToVariable(CompilePC + 4,PrgCount,"RSP PC");
			Ret();
			NextInstruction = FINISH_SUB_BLOCK;
		} else if (NextInstruction == DELAY_SLOT) {
			NextInstruction = DELAY_SLOT_EXIT;
		} else {
			CompilerWarning("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
			BreakPoint();
		}
		break;
	case 7:
		if (AudioHle || GraphicsHle)
		{
			MoveConstToVariable(0, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		} else {
			MoveVariableToX86reg(RSPInfo.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", x86_EAX);
			MoveConstToVariable(0, &RSP_Running, "RSP_Running");
			MoveConstToVariable(1, RSPInfo.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
			MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
			if (NextInstruction == NORMAL)
			{
				MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
				Ret();
				NextInstruction = FINISH_SUB_BLOCK;
			} else if (NextInstruction == DELAY_SLOT) {
				NextInstruction = DELAY_SLOT_EXIT;
			} else {
				CompilerWarning("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
				BreakPoint();
			}
		}
		break;
	case 8:
		MoveVariableToX86reg(RSPInfo.DPC_START_REG, "DPC_START_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 9:
		MoveVariableToX86reg(RSPInfo.DPC_END_REG, "DPC_END_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 10:
		MoveVariableToX86reg(RSPInfo.DPC_CURRENT_REG, "DPC_CURRENT_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 11: 
		MoveVariableToX86reg(RSPInfo.DPC_STATUS_REG, "DPC_STATUS_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;
	case 12: 
		MoveVariableToX86reg(RSPInfo.DPC_CLOCK_REG, "DPC_CLOCK_REG", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
		break;

	default:
		DisplayError("have not implemented RSP MF CP0 reg %s (%d)",COP0_Name(RSPOpC.rd),RSPOpC.rd);
	}
#endif
}

void Compile_Cop0_MT ( void ) 
{
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (LogRDP)
	{	
		char str[40];

		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		Push(x86_EAX);
		sprintf(str,"%d",RSPOpC.rd);
		PushImm32(str,RSPOpC.rd);
		sprintf(str,"%X",CompilePC);
		PushImm32(str,CompilePC);
		Call_Direct(RDP_LogMT0,"RDP_LogMT0");
		AddConstToX86Reg(x86_ESP, 12);
	}

#ifndef Compile_Cop0
	Cheat_r4300iOpcode(RSP_Cop0_MT,"RSP_Cop0_MT");
	if (RSPOpC.rd == 4)
	{
		if (NextInstruction == NORMAL)
		{
			MoveConstToVariable(CompilePC + 4,PrgCount,"RSP PC");
			Ret();
			NextInstruction = FINISH_BLOCK;
		} else if (NextInstruction == DELAY_SLOT) {
			NextInstruction = DELAY_SLOT_EXIT;
		} else {
			CompilerWarning("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction);
			BreakPoint();
		}
	}
#else
	switch (RSPOpC.rd) {
	case 0:
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.SP_MEM_ADDR_REG,"SP_MEM_ADDR_REG");
		break;
	case 1:
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.SP_DRAM_ADDR_REG,"SP_DRAM_ADDR_REG");
		break;
	case 2: 
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.SP_RD_LEN_REG,"SP_RD_LEN_REG");
		Call_Direct(SP_DMA_READ, "SP_DMA_READ");
		break;
	case 3: 
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.SP_WR_LEN_REG,"SP_WR_LEN_REG");
		Call_Direct(SP_DMA_WRITE, "SP_DMA_WRITE");
		break;
	case 7: 
		MoveConstToVariable(0, RSPInfo.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
		break;
	case 8: 
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.DPC_START_REG,"DPC_START_REG");
		MoveX86regToVariable(x86_EAX, RSPInfo.DPC_CURRENT_REG,"DPC_CURRENT_REG");
		break;
	case 9: 
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.DPC_END_REG,"DPC_END_REG");

		if (LogRDP)
		{
			Call_Direct(RDP_LogDlist, "RDP_LogDlist");
		}
			
		if (RSPInfo.ProcessRdpList != NULL)
		{
			if (Profiling)
			{
				PushImm32("Timer_RDP_Running",(DWORD)Timer_RDP_Running);
				Call_Direct(StartTimer,"StartTimer");
				AddConstToX86Reg(x86_ESP, 4);
				Push(x86_EAX);
			}
			Call_Direct(RSPInfo.ProcessRdpList, "ProcessRdpList");
			if (Profiling)
			{
				Call_Direct(StartTimer,"StartTimer");
				AddConstToX86Reg(x86_ESP, 4);
			}
		}


		break;
	case 10:
		MoveVariableToX86reg(&RSP_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
		MoveX86regToVariable(x86_EAX, RSPInfo.DPC_CURRENT_REG,"DPC_CURRENT_REG");
		break;

	default:
		Cheat_r4300iOpcode(RSP_Cop0_MT,"RSP_Cop0_MT");
		break;
	}
#endif
	if (RSPOpC.rd == 2 && !ChangedPC) {
		BYTE * Jump;

		TestConstToVariable(0x1000, RSPInfo.SP_MEM_ADDR_REG, "RSPInfo.SP_MEM_ADDR_REG");
		JeLabel8("DontExit", 0);
		Jump = RecompPos - 1;

		MoveConstToVariable(CompilePC + 4,PrgCount,"RSP PC");
		Ret();

		CPU_Message("DontExit:");
		x86_SetBranch8b(Jump, RecompPos);
	}
}
/************************** Cop2 functions *************************/

void Compile_Cop2_MF ( void ) {
	char Reg[256];
	int element = (RSPOpC.sa >> 1);

	int element1 = 15 - element;
	int element2 = 15 - ((element + 1) % 16);
	
	#ifndef Compile_Cop2
	Cheat_r4300iOpcode(RSP_Cop2_MF,"RSP_Cop2_MF"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (element2 != (element1 - 1)) {
		XorX86RegToX86Reg(x86_EAX, x86_EAX);
		XorX86RegToX86Reg(x86_EBX, x86_EBX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element1);
		MoveVariableToX86regByte(&RSP_Vect[RSPOpC.rd].B[element1], Reg, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element2);
		MoveVariableToX86regByte(&RSP_Vect[RSPOpC.rd].B[element2], Reg, x86_EBX);

		ShiftLeftSignImmed(x86_EAX, 8);
		OrX86RegToX86Reg(x86_EAX, x86_EBX);
		Cwde();

		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
	} else {
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element2);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].B[element2], Reg, x86_EAX);

		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
	}
}

void Compile_Cop2_CF ( void ) {
	#ifndef Compile_Cop2
	Cheat_r4300iOpcode(RSP_Cop2_CF, "RSP_Cop2_CF"); return;
	#endif
	
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	
	switch ((RSPOpC.rd & 0x03)) {
	case 0:
		MoveSxVariableToX86regHalf(&RSP_Flags[0].HW[0], "RSP_Flags[0].HW[0]", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		break;
	case 1:
		MoveSxVariableToX86regHalf(&RSP_Flags[1].HW[0], "RSP_Flags[1].HW[0]", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		break;
	case 2:
	case 3:
		MoveSxVariableToX86regHalf(&RSP_Flags[2].HW[0], "RSP_Flags[2].HW[0]", x86_EAX);
		MoveX86regToVariable(x86_EAX, &RSP_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
		break;
	}
}

void Compile_Cop2_MT ( void ) {
	char Reg[256];
	int element = 15 - (RSPOpC.sa >> 1);

	#ifndef Compile_Cop2
	Cheat_r4300iOpcode(RSP_Cop2_MT,"RSP_Cop2_MT"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (element == 0) {
		sprintf(Reg, "RSP_GPR[%i].B[1]", RSPOpC.rt);
		MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].B[1], Reg, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element);
		MoveX86regByteToVariable(x86_EAX, &RSP_Vect[RSPOpC.rd].B[element], Reg);
	} else {
		sprintf(Reg, "RSP_GPR[%i].B[0]", RSPOpC.rt);
		MoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].B[0], Reg, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rd, element - 1);
		MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.rd].B[element - 1], Reg);
	}
}

void Compile_Cop2_CT ( void ) {
	#ifndef Compile_Cop2
	Cheat_r4300iOpcode(RSP_Cop2_CT, "RSP_Cop2_CT"); return;
	#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	if (RSPOpC.rt == 0){
		switch ((RSPOpC.rd & 0x03)) {
		case 0:
			MoveConstHalfToVariable(0, &RSP_Flags[0].HW[0], "RSP_Flags[0].HW[0]");
			break;
		case 1:
			MoveConstHalfToVariable(0, &RSP_Flags[1].HW[0], "RSP_Flags[1].HW[0]");
			break;
		case 2:
		case 3:
			MoveConstByteToVariable(0, &RSP_Flags[2].B[0], "RSP_Flags[2].B[0]");
			break;
		}
	} else{
		switch ((RSPOpC.rd & 0x03)) {
		case 0:
			MoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].HW[0], GPR_Name(RSPOpC.rt), x86_EAX);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Flags[0].HW[0], "RSP_Flags[0].HW[0]");
			break;
		case 1:
			MoveVariableToX86regHalf(&RSP_GPR[RSPOpC.rt].HW[0], GPR_Name(RSPOpC.rt), x86_EAX);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Flags[1].HW[0], "RSP_Flags[1].HW[0]");
			break;
		case 2:
		case 3:
			MoveVariableToX86regByte(&RSP_GPR[RSPOpC.rt].B[0], GPR_Name(RSPOpC.rt), x86_EAX);
			MoveX86regByteToVariable(x86_EAX, &RSP_Flags[2].B[0], "RSP_Flags[2].B[0]");
			break;
		}
	}
}

void Compile_COP2_VECTOR (void) {
	RSP_Vector[ RSPOpC.funct ]();
}

/************************** Vect functions **************************/

UDWORD MMX_Scratch;

void RSP_Element2Mmx(int MmxReg) {
	char Reg[256];

	DWORD Rs = RSPOpC.rs & 0x0f;
	DWORD el;

	switch (Rs) {
	case 0: case 1:
	case 2: case 3:
	case 4:	case 5:
	case 6:	case 7:
		CompilerWarning("Unimplemented RSP_Element2Mmx");
		break;

	default:
		/*
		 * Noticed the exclusive-or of seven to take into account
		 * the pseudo-swapping we have in the vector registers
		 */

		el = (RSPOpC.rs & 0x07) ^ 7;

		if (IsMmx2Enabled == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, el);
			MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[el], Reg, x86_ECX);
			MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[0], "MMX_Scratch.HW[0]");
			MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[1], "MMX_Scratch.HW[1]");
			MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[2], "MMX_Scratch.HW[2]");
			MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[3], "MMX_Scratch.HW[3]");
			MmxMoveQwordVariableToReg(MmxReg, &MMX_Scratch.HW[0], "MMX_Scratch.HW[0]");
		} else {
			unsigned long Qword;
			
			Qword = (el >> 2) & 0x1;
			el &= 0x3;

			sprintf(Reg, "RSP_Vect[%i].DW[%i]", RSPOpC.rt, Qword);
			MmxShuffleMemoryToReg(MmxReg, 
				&RSP_Vect[RSPOpC.rt].DW[Qword], Reg, _MMX_SHUFFLE(el, el, el, el));
		}
		break;
	}
}

void RSP_MultiElement2Mmx(int MmxReg1, int MmxReg2) {
	char Reg[256];
	DWORD Rs = RSPOpC.rs & 0x0f;

	/*
	 * Ok, this is tricky, hopefully this clears it up:
	 *
	 * $vd[0] = $vd[0] + $vt[2] 
	 * because of swapped registers becomes:
	 * $vd[7] = $vd[7] + $vt[5]
	 *
	 * we must perform this swap correctly, this involves the 3-bit
	 * xclusive or, 2-bits of which are done within a dword boundary, 
	 * the last bit, is ignored because we are loading the source linearly,
	 * so the xclusive or has transparently happened on that side
	 *
	 */

	switch (Rs) {
	case 0:
	case 1:
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		MmxMoveQwordVariableToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		MmxMoveQwordVariableToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
		break;
	case 2:
		/* [0q]    | 0 | 0 | 2 | 2 | 4 | 4 | 6 | 6 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xF5);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xF5);
		break;
	case 3:
		/* [1q]    | 1 | 1 | 3 | 3 | 5 | 5 | 7 | 7 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xA0);
		//MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0x0A);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xA0);
		//MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0x0A);
		break;
	case 4:
		/* [0h]    | 0 | 0 | 0 | 0 | 4 | 4 | 4 | 4 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xFF);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xFF);
		break;
	case 5:
		/* [1h]    | 1 | 1 | 1 | 1 | 5 | 5 | 5 | 5 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0xAA);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0xAA);
		break;
	case 6:
		/* [2h]    | 2 | 2 | 2 | 2 | 6 | 6 | 6 | 6 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0x55);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0x55);
		break;
	case 7:
		/* [3h]    | 3 | 3 | 3 | 3 | 7 | 7 | 7 | 7 | */
		sprintf(Reg, "RSP_Vect[%i].DW[0]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg1, &RSP_Vect[RSPOpC.rt].DW[0], Reg, 0x00);
		sprintf(Reg, "RSP_Vect[%i].DW[1]", RSPOpC.rt);
		MmxShuffleMemoryToReg(MmxReg2, &RSP_Vect[RSPOpC.rt].DW[1], Reg, 0x00);
		break;

	default:
		CompilerWarning("Unimplemented RSP_MultiElement2Mmx [?]");
		break;
	}
}

BOOL Compile_Vector_VMULF_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	/* NOTE: Problem here is the lack of +/- 0x8000 rounding */
	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		if (RSPOpC.rd == RSPOpC.rt){
			MmxPmulhwRegToReg(x86_MM0, x86_MM0);
			MmxPmulhwRegToReg(x86_MM1, x86_MM1);
		} else {
			sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
			MmxPmulhwRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
			MmxPmulhwRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
		}
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPmulhwRegToReg(x86_MM0, x86_MM2);
		MmxPmulhwRegToReg(x86_MM1, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPmulhwRegToReg(x86_MM0, x86_MM2);
		MmxPmulhwRegToReg(x86_MM1, x86_MM3);
	}
	MmxPsllwImmed(x86_MM0, 1);
	MmxPsllwImmed(x86_MM1, 1);

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VMULF ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

	#ifndef CompileVmulf
	Cheat_r4300iOpcode(RSP_Vector_VMULF,"RSP_Vector_VMULF"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VMULF_MMX())
			return;
	}

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToDest == TRUE) {
		MoveConstToX86reg(0x7fff0000, x86_ESI);
	}
	if (bWriteToAccum == TRUE) {
		XorX86RegToX86Reg(x86_EDI, x86_EDI);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);

		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (RSPOpC.rt == RSPOpC.rd && !bOptimize) {
			imulX86reg(x86_EAX);
		} else {
			if (bOptimize == FALSE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
			}
			imulX86reg(x86_EBX);
		}

		ShiftLeftSignImmed(x86_EAX, 1);
		AddConstToX86Reg(x86_EAX, 0x8000);

		if (bWriteToAccum == TRUE) {
			MoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].HW[1]");
			/* calculate sign extension into edx */
			MoveX86RegToX86Reg(x86_EAX, x86_EDX);
			ShiftRightSignImmed(x86_EDX, 31);
		}

		CompConstToX86reg(x86_EAX, 0x80008000);

		if (bWriteToAccum == TRUE) {
			CondMoveEqual(x86_EDX, x86_EDI);
			MoveX86regHalfToVariable(x86_EDX, &RSP_ACCUM[el].HW[3], "RSP_ACCUM[el].HW[3]");
		}
		if (bWriteToDest == TRUE) {
			CondMoveEqual(x86_EAX, x86_ESI);
			ShiftRightUnsignImmed(x86_EAX, 16);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], "RSP_Vect[RSPOpC.sa].HW[el]");
		}
	}
}

void Compile_Vector_VMULU ( void ) {
	Cheat_r4300iOpcode(RSP_Vector_VMULU,"RSP_Vector_VMULU");
}

BOOL Compile_Vector_VMUDL_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if (IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		if (RSPOpC.rd == RSPOpC.rt){
			MmxPmulhuwRegToReg(x86_MM0, x86_MM0);
			MmxPmulhuwRegToReg(x86_MM1, x86_MM1);
		} else {
			sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);

			MmxPmulhuwRegToReg(x86_MM0, x86_MM2);
			MmxPmulhuwRegToReg(x86_MM1, x86_MM3);
		}
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPmulhuwRegToReg(x86_MM0, x86_MM2);
		MmxPmulhuwRegToReg(x86_MM1, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPmulhuwRegToReg(x86_MM0, x86_MM2);
		MmxPmulhuwRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	
	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VMUDL ( void ) {
	char Reg[256];
	int count, el, del;
	
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

	#ifndef CompileVmudl
	Cheat_r4300iOpcode(RSP_Vector_VMUDL,"RSP_Vector_VMUDL"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VMUDL_MMX())
			return;
	}
	
	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToAccum == TRUE)
		XorX86RegToX86Reg(x86_EDI, x86_EDI);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].UHW[el], Reg, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		imulX86reg(x86_EBX);

		if (bWriteToAccum == TRUE) {
			sprintf(Reg, "RSP_ACCUM[%i].UW[0]", el);
			MoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].UW[0], Reg);
			sprintf(Reg, "RSP_ACCUM[%i].UW[1]", el);
			MoveX86regToVariable(x86_EDI, &RSP_ACCUM[el].UW[1], Reg);
		}

		if (bWriteToDest == TRUE) {
			ShiftRightUnsignImmed(x86_EAX, 16);
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
}

BOOL Compile_Vector_VMUDM_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if (IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		MmxMoveQwordVariableToReg(x86_MM4, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		MmxMoveQwordVariableToReg(x86_MM5, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);

		/* Copy the signed portion */
		MmxMoveRegToReg(x86_MM2, x86_MM0);
		MmxMoveRegToReg(x86_MM3, x86_MM1);

		/* high((u16)a * b) */
		MmxPmulhuwRegToReg(x86_MM0, x86_MM4);
		MmxPmulhuwRegToReg(x86_MM1, x86_MM5);

		/* low((a >> 15) * b) */
		MmxPsrawImmed(x86_MM2, 15);
		MmxPsrawImmed(x86_MM3, 15);
		MmxPmullwRegToReg(x86_MM2, x86_MM4);
		MmxPmullwRegToReg(x86_MM3, x86_MM5);
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM4);

		/* Copy the signed portion */
		MmxMoveRegToReg(x86_MM2, x86_MM0);
		MmxMoveRegToReg(x86_MM3, x86_MM1);

		/* high((u16)a * b) */
		MmxPmulhuwRegToReg(x86_MM0, x86_MM4);
		MmxPmulhuwRegToReg(x86_MM1, x86_MM4);

		/* low((a >> 15) * b) */
		MmxPsrawImmed(x86_MM2, 15);
		MmxPsrawImmed(x86_MM3, 15);
		MmxPmullwRegToReg(x86_MM2, x86_MM4);
		MmxPmullwRegToReg(x86_MM3, x86_MM4);
	} else {
		RSP_MultiElement2Mmx(x86_MM4, x86_MM5);

		/* Copy the signed portion */
		MmxMoveRegToReg(x86_MM2, x86_MM0);
		MmxMoveRegToReg(x86_MM3, x86_MM1);

		/* high((u16)a * b) */
		MmxPmulhuwRegToReg(x86_MM0, x86_MM4);
		MmxPmulhuwRegToReg(x86_MM1, x86_MM5);

		/* low((a >> 15) * b) */
		MmxPsrawImmed(x86_MM2, 15);
		MmxPsrawImmed(x86_MM3, 15);
		MmxPmullwRegToReg(x86_MM2, x86_MM4);
		MmxPmullwRegToReg(x86_MM3, x86_MM5);
	}

	/* Add them up */
	MmxPaddwRegToReg(x86_MM0, x86_MM2);
	MmxPaddwRegToReg(x86_MM1, x86_MM3);

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VMUDM ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

	#ifndef CompileVmudm
	Cheat_r4300iOpcode(RSP_Vector_VMUDM,"RSP_Vector_VMUDM"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VMUDM_MMX())
			return;
	}

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	Push(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	if (bWriteToDest) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.sa].HW[0], Reg, x86_ECX);
	} else if (!bOptimize) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_ECX);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
		
		/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, (BYTE)(el * 2), x86_EAX);

		if (bOptimize == FALSE) {
			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
			} else {
				MoveZxX86RegPtrDispToX86RegHalf(x86_ECX, (BYTE)(del * 2), x86_EBX);
			}
		}

		imulX86reg(x86_EBX);

		if (bWriteToAccum == FALSE && bWriteToDest == TRUE) {
			ShiftRightUnsignImmed(x86_EAX, 16);
			/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
			MoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, (BYTE)(el * 2));
		} else {
			MoveX86RegToX86Reg(x86_EAX, x86_EDX);
			ShiftRightSignImmed(x86_EDX, 16);
			ShiftLeftSignImmed(x86_EAX, 16);

			if (bWriteToAccum == TRUE) {
				sprintf(Reg, "RSP_ACCUM[%i].UW[0]", el);
				MoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].UW[0], Reg);
				sprintf(Reg, "RSP_ACCUM[%i].UW[1]", el);
				MoveX86regToVariable(x86_EDX ,&RSP_ACCUM[el].UW[1], Reg);
			}
			if (bWriteToDest == TRUE) {
				/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EDX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
				MoveX86regHalfToX86regPointerDisp(x86_EDX, x86_ECX, (BYTE)(el * 2));
			}
		}
	}

	Pop(x86_EBP);
}

BOOL Compile_Vector_VMUDN_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rt);
		MmxPmullwVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rt].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rt);
		MmxPmullwVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rt].UHW[4], Reg);
	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPmullwRegToReg(x86_MM0, x86_MM2);
		MmxPmullwRegToReg(x86_MM1, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPmullwRegToReg(x86_MM0, x86_MM2);
		MmxPmullwRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VMUDN ( void ) {
	char Reg[256];
	int count, el, del;
	
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

	#ifndef CompileVmudn
	Cheat_r4300iOpcode(RSP_Vector_VMUDN,"RSP_Vector_VMUDN"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VMUDN_MMX())
			return;
	}

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	Push(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		/*sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].UHW[el], Reg, x86_EAX);*/
		MoveZxX86RegPtrDispToX86RegHalf(x86_EBP, (BYTE)(el * 2), x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		imulX86reg(x86_EBX);

		if (bWriteToDest == TRUE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}

		if (bWriteToAccum == TRUE) {
			MoveX86RegToX86Reg(x86_EAX, x86_EDX);
			ShiftRightSignImmed(x86_EDX, 16);
			ShiftLeftSignImmed(x86_EAX, 16);
			sprintf(Reg, "RSP_ACCUM[%i].UW[0]", el);
			MoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].UW[0], Reg);
			sprintf(Reg, "RSP_ACCUM[%i].UW[1]", el);
			MoveX86regToVariable(x86_EDX, &RSP_ACCUM[el].UW[1], Reg);
		}
	}
	Pop(x86_EBP);
}

BOOL Compile_Vector_VMUDH_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].HW[4], Reg);

	/* Registers 4 & 5 are high */
	MmxMoveRegToReg(x86_MM4, x86_MM0);
	MmxMoveRegToReg(x86_MM5, x86_MM1);

	if ((RSPOpC.rs & 0x0f) < 2) {
		if (RSPOpC.rd == RSPOpC.rt){
			MmxPmullwRegToReg(x86_MM0, x86_MM0);
			MmxPmulhwRegToReg(x86_MM4, x86_MM4);
			MmxPmullwRegToReg(x86_MM1, x86_MM1);
			MmxPmulhwRegToReg(x86_MM5, x86_MM5);
		} else {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].HW[4], Reg);

			MmxPmullwRegToReg(x86_MM0, x86_MM2);
			MmxPmulhwRegToReg(x86_MM4, x86_MM2);
			MmxPmullwRegToReg(x86_MM1, x86_MM3);
			MmxPmulhwRegToReg(x86_MM5, x86_MM3);
		}
	} else if ((RSPOpC.rs & 0x0f) >= 8) {
		RSP_Element2Mmx(x86_MM2);

		MmxPmullwRegToReg(x86_MM0, x86_MM2);
		MmxPmulhwRegToReg(x86_MM4, x86_MM2);
		MmxPmullwRegToReg(x86_MM1, x86_MM2);
		MmxPmulhwRegToReg(x86_MM5, x86_MM2);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);

		MmxPmullwRegToReg(x86_MM0, x86_MM2);
		MmxPmulhwRegToReg(x86_MM4, x86_MM2);
		MmxPmullwRegToReg(x86_MM1, x86_MM3);
		MmxPmulhwRegToReg(x86_MM5, x86_MM3);
	}

	/* 0 & 1 are low, 4 & 5 are high */
	MmxMoveRegToReg(x86_MM6, x86_MM0);
	MmxMoveRegToReg(x86_MM7, x86_MM1);

	MmxUnpackLowWord(x86_MM0, x86_MM4);
	MmxUnpackHighWord(x86_MM6, x86_MM4);
	MmxUnpackLowWord(x86_MM1, x86_MM5);
	MmxUnpackHighWord(x86_MM7, x86_MM5);

	/* Integrate copies */
	MmxPackSignedDwords(x86_MM0, x86_MM6);
	MmxPackSignedDwords(x86_MM1, x86_MM7);
	
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].HW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VMUDH ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

	#ifndef CompileVmudh
	Cheat_r4300iOpcode(RSP_Vector_VMUDH,"RSP_Vector_VMUDH"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VMUDH_MMX())
			return;
	}

	if (bWriteToDest == FALSE && bOptimize == TRUE) {
		Push(x86_EBP);
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);

		/* Load source */
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);

		/* 
		 * Pipe lined segment 0
		 */
		
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 0, x86_EAX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 2, x86_ECX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 4, x86_EDI);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 6, x86_ESI);

		ImulX86RegToX86Reg(x86_EAX, x86_EBX);
		ImulX86RegToX86Reg(x86_ECX, x86_EBX);
		ImulX86RegToX86Reg(x86_EDI, x86_EBX);
		ImulX86RegToX86Reg(x86_ESI, x86_EBX);
		XorX86RegToX86Reg(x86_EDX, x86_EDX);

		MoveOffsetToX86reg((DWORD)&RSP_ACCUM[0].W[0], "RSP_ACCUM[0].W[0]", x86_EBP);

		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 0);
		MoveX86RegToX86regPointerDisp(x86_EAX, x86_EBP, 4);
		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 8);
		MoveX86RegToX86regPointerDisp(x86_ECX, x86_EBP, 12);
		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 16);
		MoveX86RegToX86regPointerDisp(x86_EDI, x86_EBP, 20);
		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 24);
		MoveX86RegToX86regPointerDisp(x86_ESI, x86_EBP, 28);

		/* 
		 * Pipe lined segment 1
		 */

		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP,  8, x86_EAX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 10, x86_ECX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 12, x86_EDI);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 14, x86_ESI);

		ImulX86RegToX86Reg(x86_EAX, x86_EBX);
		ImulX86RegToX86Reg(x86_ECX, x86_EBX);
		ImulX86RegToX86Reg(x86_EDI, x86_EBX);
		ImulX86RegToX86Reg(x86_ESI, x86_EBX);
		XorX86RegToX86Reg(x86_EDX, x86_EDX);

		MoveOffsetToX86reg((DWORD)&RSP_ACCUM[0].W[0], "RSP_ACCUM[0].W[0]", x86_EBP);

		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 32);
		MoveX86RegToX86regPointerDisp(x86_EAX, x86_EBP, 36);
		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 40);
		MoveX86RegToX86regPointerDisp(x86_ECX, x86_EBP, 44);
		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 48);
		MoveX86RegToX86regPointerDisp(x86_EDI, x86_EBP, 52);
		MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 56);
		MoveX86RegToX86regPointerDisp(x86_ESI, x86_EBP, 60);

		Pop(x86_EBP);
	} else {
		if (bOptimize == TRUE) {
			del = (RSPOpC.rs & 0x07) ^ 7;
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}
		if (bWriteToDest == TRUE) {
			/*
			 * Prepare for conditional moves
			 */
			MoveConstToX86reg(0x00007fff, x86_ESI);
			MoveConstToX86reg(0xFFFF8000, x86_EDI);
		}

		for (count = 0; count < 8; count++) {
			CPU_Message("     Iteration: %i", count);
			el = Indx[RSPOpC.rs].B[count];
			del = EleSpec[RSPOpC.rs].B[el];
		
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

			if (bOptimize == FALSE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
			}
			imulX86reg(x86_EBX);
			
			if (bWriteToAccum == TRUE) {
				MoveX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");
				MoveConstToVariable(0, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
			}
			
			if (bWriteToDest == TRUE) {
				CompX86RegToX86Reg(x86_EAX, x86_ESI);
				CondMoveGreater(x86_EAX, x86_ESI);
				CompX86RegToX86Reg(x86_EAX, x86_EDI);
				CondMoveLess(x86_EAX, x86_EDI);

				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
			}
		}
	}
}

void Compile_Vector_VMACF ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

	#ifndef CompileVmacf
	Cheat_r4300iOpcode(RSP_Vector_VMACF,"RSP_Vector_VMACF"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x00007fff, x86_ESI);
		MoveConstToX86reg(0xFFFF8000, x86_EDI);
	}
	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		imulX86reg(x86_EBX);

		MoveX86RegToX86Reg(x86_EAX, x86_EDX);
		ShiftRightSignImmed(x86_EDX, 15);
		ShiftLeftSignImmed(x86_EAX, 17);

		AddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
		AdcX86regToVariable(x86_EDX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");

		if (bWriteToDest == TRUE) {
			MoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);

			CompX86RegToX86Reg(x86_EAX, x86_ESI);
			CondMoveGreater(x86_EAX, x86_ESI);
			CompX86RegToX86Reg(x86_EAX, x86_EDI);
			CondMoveLess(x86_EAX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
}

void Compile_Vector_VMACU ( void ) {
	Cheat_r4300iOpcode(RSP_Vector_VMACU,"RSP_Vector_VMACU");
}

void Compile_Vector_VMACQ ( void ) {
	Cheat_r4300iOpcode(RSP_Vector_VMACQ,"RSP_Vector_VMACQ");
}

void Compile_Vector_VMADL ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

	#ifndef CompileVmadl
	Cheat_r4300iOpcode(RSP_Vector_VMADL,"RSP_Vector_VMADL"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x00007FFF, x86_ESI);
		MoveConstToX86reg(0xFFFF8000, x86_EDI);

		Push(x86_EBP);
		MoveConstToX86reg(0x0000FFFF, x86_EBP);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		imulX86reg(x86_EBX);
		sprintf(Reg, "RSP_ACCUM[%i].W[0]", el);
		AddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
		AdcConstToVariable(&RSP_ACCUM[el].W[1], Reg, 0);

		if (bWriteToDest != FALSE) {
			XorX86RegToX86Reg(x86_EDX, x86_EDX);
			MoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);
			MoveZxVariableToX86regHalf(&RSP_ACCUM[el].HW[1], "RSP_ACCUM[el].hW[1]", x86_ECX);

			CompX86RegToX86Reg(x86_EAX, x86_ESI);
			CondMoveGreater(x86_ECX, x86_EBP);
			CompX86RegToX86Reg(x86_EAX, x86_EDI);
			CondMoveLess(x86_ECX, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}

	if (bWriteToDest == TRUE) {
		Pop(x86_EBP);
	}
}

void Compile_Vector_VMADM ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

	#ifndef CompileVmadm
	Cheat_r4300iOpcode(RSP_Vector_VMADM,"RSP_Vector_VMADM"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x00007fff, x86_ESI);
		MoveConstToX86reg(0xFFFF8000, x86_EDI);
	}

	Push(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	if (bWriteToDest) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.sa].HW[0], Reg, x86_ECX);
	} else if (!bOptimize) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_ECX);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
		
		/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, (BYTE)(el * 2), x86_EAX);

		if (bOptimize == FALSE) {
			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], "RSP_Vect[RSPOpC.rt].HW[del]", x86_EBX);
			} else {
				MoveZxX86RegPtrDispToX86RegHalf(x86_ECX, (BYTE)(del * 2), x86_EBX);
			}
		}

		imulX86reg(x86_EBX);

		MoveX86RegToX86Reg(x86_EAX, x86_EDX);
		ShiftRightSignImmed(x86_EDX, 16);
		ShiftLeftSignImmed(x86_EAX, 16);
		AddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
		AdcX86regToVariable(x86_EDX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");

		if (bWriteToDest == TRUE) {
			/* For compare */
			sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
			MoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);

			CompX86RegToX86Reg(x86_EAX, x86_ESI);
			CondMoveGreater(x86_EAX, x86_ESI);
			CompX86RegToX86Reg(x86_EAX, x86_EDI);
			CondMoveLess(x86_EAX, x86_EDI);

			/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
			MoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, (BYTE)(el * 2));
		}
	}

	Pop(x86_EBP);
}

void Compile_Vector_VMADN ( void ) {
	char Reg[256];
	int count, el, del;
	
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

	#ifndef CompileVmadn
	Cheat_r4300iOpcode(RSP_Vector_VMADN,"RSP_Vector_VMADN"); return;
	#endif
	
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	} 
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x0000ffff, x86_ESI);
		MoveConstToX86reg(0x00000000, x86_EDI);
	}

	Push(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		/*sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].UHW[el], Reg, x86_EAX);*/
		MoveZxX86RegPtrDispToX86RegHalf(x86_EBP, (BYTE)(el * 2), x86_EAX);

		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		imulX86reg(x86_EBX);

		MoveX86RegToX86Reg(x86_EAX, x86_EDX);
		ShiftRightSignImmed(x86_EDX, 16);
		ShiftLeftSignImmed(x86_EAX, 16);
		AddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[0], "RSP_ACCUM[el].W[0]");
		AdcX86regToVariable(x86_EDX, &RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]");

		if (bWriteToDest == TRUE) {
			/* For compare */
			sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
			MoveVariableToX86reg(&RSP_ACCUM[el].W[1], Reg, x86_EAX);

			/* For vector */
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveVariableToX86regHalf(&RSP_ACCUM[el].HW[1], Reg, x86_ECX);

			/* Weird eh */
			CompConstToX86reg(x86_EAX, 0x7fff);
			CondMoveGreater(x86_ECX, x86_ESI);
			CompConstToX86reg(x86_EAX, (DWORD)(-0x8000));
			CondMoveLess(x86_ECX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	Pop(x86_EBP);
}

void Compile_Vector_VMADH ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

	#ifndef CompileVmadh
	Cheat_r4300iOpcode(RSP_Vector_VMADH,"RSP_Vector_VMADH"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	} 
	
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x00007fff, x86_ESI);
		MoveConstToX86reg(0xFFFF8000, x86_EDI);
	}

	if (bWriteToDest == FALSE && bOptimize == TRUE) {
		Push(x86_EBP);
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		/* 
		 * Pipe lined segment 0
		 */
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 0, x86_EAX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 2, x86_ECX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 4, x86_EDI);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 6, x86_ESI);

		ImulX86RegToX86Reg(x86_EAX, x86_EBX);
		ImulX86RegToX86Reg(x86_ECX, x86_EBX);
		ImulX86RegToX86Reg(x86_EDI, x86_EBX);
		ImulX86RegToX86Reg(x86_ESI, x86_EBX);

		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 0);
		AddX86regToVariable(x86_EAX, &RSP_ACCUM[0].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 1);
		AddX86regToVariable(x86_ECX, &RSP_ACCUM[1].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 2);
		AddX86regToVariable(x86_EDI, &RSP_ACCUM[2].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 3);
		AddX86regToVariable(x86_ESI, &RSP_ACCUM[3].W[1], Reg);

		/* 
		 * Pipe lined segment 1
		 */
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP,  8, x86_EAX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 10, x86_ECX);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 12, x86_EDI);
		MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 14, x86_ESI);

		ImulX86RegToX86Reg(x86_EAX, x86_EBX);
		ImulX86RegToX86Reg(x86_ECX, x86_EBX);
		ImulX86RegToX86Reg(x86_EDI, x86_EBX);
		ImulX86RegToX86Reg(x86_ESI, x86_EBX);

		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 4);
		AddX86regToVariable(x86_EAX, &RSP_ACCUM[4].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 5);
		AddX86regToVariable(x86_ECX, &RSP_ACCUM[5].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 6);
		AddX86regToVariable(x86_EDI, &RSP_ACCUM[6].W[1], Reg);
		sprintf(Reg, "RSP_ACCUM[%i].W[1]", 7);
		AddX86regToVariable(x86_ESI, &RSP_ACCUM[7].W[1], Reg);

		Pop(x86_EBP);
	} else {
		Push(x86_EBP);
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
		MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

		if (bWriteToDest) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
			MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.sa].HW[0], Reg, x86_ECX);
		} else if (!bOptimize) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_ECX);
		}

		for (count = 0; count < 8; count++) {
			CPU_Message("     Iteration: %i", count);
			el = Indx[RSPOpC.rs].B[count];
			del = EleSpec[RSPOpC.rs].B[el];

			/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
			MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, (BYTE)(el * 2), x86_EAX);

			if (bOptimize == FALSE) {
				if (bWriteToDest == TRUE) {
					sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
					MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
				} else {
					MoveSxX86RegPtrDispToX86RegHalf(x86_ECX, (BYTE)(del * 2), x86_EBX);
				}
			}

			imulX86reg(x86_EBX);
			sprintf(Reg, "RSP_ACCUM[%i].W[1]", el);
			AddX86regToVariable(x86_EAX, &RSP_ACCUM[el].W[1], Reg);

			if (bWriteToDest == TRUE) {
				MoveVariableToX86reg(&RSP_ACCUM[el].W[1], "RSP_ACCUM[el].W[1]", x86_EAX);

				CompX86RegToX86Reg(x86_EAX, x86_ESI);
				CondMoveGreater(x86_EAX, x86_ESI);
				CompX86RegToX86Reg(x86_EAX, x86_EDI);
				CondMoveLess(x86_EAX, x86_EDI);

				/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);*/
				MoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, (BYTE)(el * 2));
			}
		}
		Pop(x86_EBP);
	}
}

BOOL Compile_Vector_VADD_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 15) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPaddswRegToReg(x86_MM0, x86_MM2);
		MmxPaddswRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 15) < 2) {
		if (RSPOpC.rd == RSPOpC.rt){
			MmxPaddswRegToReg(x86_MM0, x86_MM0);
			MmxPaddswRegToReg(x86_MM1, x86_MM1);
		} else{
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxPaddswVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxPaddswVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rt].HW[4], Reg);
		}
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPaddswRegToReg(x86_MM0, x86_MM2);
		MmxPaddswRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	
	if (IsNextInstructionMmx(CompilePC) != TRUE) {
		MmxEmptyMultimediaState();
	}

	return TRUE;
}

void Compile_Vector_VADD ( void ) {
	char Reg[256];
	int count, el, del;


	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	BOOL bFlagUseage = UseRspFlags(CompilePC);

	#ifndef CompileVadd
	Cheat_r4300iOpcode(RSP_Vector_VADD,"RSP_Vector_VADD"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE && bFlagUseage == FALSE) {
		if (TRUE == Compile_Vector_VADD_MMX())
			return;
	}

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}
	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x00007fff, x86_ESI);
		MoveConstToX86reg(0xffff8000, x86_EDI);
	}
	
	/* Used for involking x86 carry flag */
	XorX86RegToX86Reg(x86_ECX, x86_ECX);
	Push(x86_EBP);
	MoveVariableToX86reg(&RSP_Flags[0].UW, "RSP_Flags[0].UW", x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
	
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}
		
		MoveX86RegToX86Reg(x86_EBP, x86_EDX);
		AndConstToX86Reg(x86_EDX, 1 << (7 - el));
		CompX86RegToX86Reg(x86_ECX, x86_EDX);

		AdcX86RegToX86Reg(x86_EAX, x86_EBX);

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
		if (bWriteToDest != FALSE) {
			CompX86RegToX86Reg(x86_EAX, x86_ESI);
			CondMoveGreater(x86_EAX, x86_ESI);
			CompX86RegToX86Reg(x86_EAX, x86_EDI);
			CondMoveLess(x86_EAX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	Pop(x86_EBP);
}

BOOL Compile_Vector_VSUB_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 15) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPsubswRegToReg(x86_MM0, x86_MM2);
		MmxPsubswRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 15) < 2) {
		if (RSPOpC.rd == RSPOpC.rt){
			MmxPsubswRegToReg(x86_MM0, x86_MM0);
			MmxPsubswRegToReg(x86_MM1, x86_MM1);
		} else {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxPsubswVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxPsubswVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rt].HW[4], Reg);
		}
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPsubswRegToReg(x86_MM0, x86_MM2);
		MmxPsubswRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	if (IsNextInstructionMmx(CompilePC) != TRUE) {
		MmxEmptyMultimediaState();
	}

	return TRUE;
}

void Compile_Vector_VSUB ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bOptimize = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	BOOL bFlagUseage = UseRspFlags(CompilePC);

	#ifndef CompileVsub
	Cheat_r4300iOpcode(RSP_Vector_VSUB,"RSP_Vector_VSUB"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE && bFlagUseage == FALSE) {
		if (TRUE == Compile_Vector_VSUB_MMX())
			return;
	}

	Push(x86_EBP);

	/* Used for involking the x86 carry flag */
	XorX86RegToX86Reg(x86_ECX, x86_ECX);
	MoveVariableToX86reg(&RSP_Flags[0].UW, "RSP_Flags[0].UW", x86_EBP);

	if (bOptimize == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	if (bWriteToDest == TRUE) {
		/*
		 * Prepare for conditional moves
		 */
		MoveConstToX86reg(0x00007fff, x86_ESI);
		MoveConstToX86reg(0xffff8000, x86_EDI);
	}

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], "RSP_Vect[RSPOpC.rd].HW[el]", x86_EAX);
		if (bOptimize == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}
	
		MoveX86RegToX86Reg(x86_EBP, x86_EDX);
		AndConstToX86Reg(x86_EDX, 1 << (7 - el));
		CompX86RegToX86Reg(x86_ECX, x86_EDX);

		SbbX86RegToX86Reg(x86_EAX, x86_EBX);

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}

		if (bWriteToDest != FALSE) {
			CompX86RegToX86Reg(x86_EAX, x86_ESI);
			CondMoveGreater(x86_EAX, x86_ESI);
			CompX86RegToX86Reg(x86_EAX, x86_EDI);
			CondMoveLess(x86_EAX, x86_EDI);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}

	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	Pop(x86_EBP);
}

BOOL Compile_Vector_VABS_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 15) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxMoveRegToReg(x86_MM3, x86_MM2);
	} else if ((RSPOpC.rs & 15) < 2) {
		if (RSPOpC.rd != RSPOpC.rt) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].HW[4], Reg);
		} else {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxMoveRegToReg(x86_MM2, x86_MM0);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxMoveRegToReg(x86_MM3, x86_MM1);
		}
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
	}

	if (RSPOpC.rd == RSPOpC.rt) {
		MmxPsrawImmed(x86_MM2, 15);
		MmxPsrawImmed(x86_MM3, 15);

		MmxXorRegToReg(x86_MM0, x86_MM2);
		MmxXorRegToReg(x86_MM1, x86_MM3);

		MmxPsubswRegToReg(x86_MM0, x86_MM2);
		MmxPsubswRegToReg(x86_MM1, x86_MM3);
	} else {
		MmxXorRegToReg(x86_MM7, x86_MM7);

		MmxMoveRegToReg(x86_MM4, x86_MM0);
		MmxMoveRegToReg(x86_MM5, x86_MM1);

		MmxPsrawImmed(x86_MM4, 15);
		MmxPsrawImmed(x86_MM5, 15);

		MmxPcmpeqwRegToReg(x86_MM0, x86_MM7);
		MmxPcmpeqwRegToReg(x86_MM1, x86_MM7);

		MmxXorRegToReg(x86_MM2, x86_MM4);
		MmxXorRegToReg(x86_MM3, x86_MM5);

		MmxPsubswRegToReg(x86_MM2, x86_MM4);
		MmxPsubswRegToReg(x86_MM3, x86_MM5);

		MmxPandnRegToReg(x86_MM0, x86_MM2);
		MmxPandnRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) != TRUE) {
		MmxEmptyMultimediaState();
	}

	return TRUE;
}

void Compile_Vector_VABS ( void ) {
	int count, el, del;
	char Reg[256];
	
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	#ifndef CompileVabs
	Cheat_r4300iOpcode(RSP_Vector_VABS,"RSP_Vector_VABS"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VABS_MMX())
			return;
	}
	
	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		if (RSPOpC.rd == RSPOpC.rt && (RSPOpC.rs & 0xF) < 2) {
			/**
			** Optimize: EDI/ESI unused, and ECX is const etc
			***/

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

			/*** Obtain the negative of the source ****/
			MoveX86RegToX86Reg(x86_EAX, x86_EBX);
			NegateX86reg(x86_EBX);
		
			/**
			** determine negative value, 
			** note: negate(FFFF8000h) == 00008000h 
			***/

			MoveConstToX86reg(0x7fff, x86_ECX);
			CompConstToX86reg(x86_EBX, 0x00008000);
			CondMoveEqual(x86_EBX, x86_ECX);

			/* sign clamp, dest = (eax >= 0) ? eax : ebx */
			CompConstToX86reg(x86_EAX, 0);
			CondMoveLess(x86_EAX, x86_EBX);

			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
			}
			if (bWriteToAccum == TRUE) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
			}
		} else {
			/**
			** Optimize: ESI unused, and EDX is const etc
			***/

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);

			/*** Obtain the negative of the source ****/
			MoveX86RegToX86Reg(x86_EBX, x86_ECX);
			NegateX86reg(x86_EBX);

			/**
			** determine negative value, 
			** note: negate(FFFF8000h) == 00008000h 
			***/

			MoveConstToX86reg(0x7fff, x86_EDX);
			CompConstToX86reg(x86_EBX, 0x00008000);
			CondMoveEqual(x86_EBX, x86_EDX);

			/* sign clamp, dest = (eax >= 0) ? ecx : ebx */
			CompConstToX86reg(x86_EAX, 0);
			CondMoveGreaterEqual(x86_EDI, x86_ECX);
			CondMoveLess(x86_EDI, x86_EBX);

			if (bWriteToDest == TRUE) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EDI, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
			}
			if (bWriteToAccum == TRUE) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_EDI, &RSP_ACCUM[el].HW[1], Reg);
			}
		}
	}
}

void Compile_Vector_VADDC ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	
	#ifndef CompileVaddc
	Cheat_r4300iOpcode(RSP_Vector_VADDC,"RSP_Vector_VADDC"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	/* Initialize flag register */
	XorX86RegToX86Reg(x86_ECX, x86_ECX);

	Push(x86_EBP);
	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rd].HW[0], Reg, x86_EBP);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
	
		/*sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);*/
		MoveZxX86RegPtrDispToX86RegHalf(x86_EBP, (BYTE)(el * 2), x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		AddX86RegToX86Reg(x86_EAX, x86_EBX);

		XorX86RegToX86Reg(x86_EDX, x86_EDX);
		TestConstToX86Reg(0xFFFF0000, x86_EAX);
		Setnz(x86_EDX);
		if ((7 - el) != 0) {
			ShiftLeftSignImmed(x86_EDX, (BYTE)(7 - el));
		}
		OrX86RegToX86Reg(x86_ECX, x86_EDX);

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}

		if (bWriteToDest != FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	MoveX86regToVariable(x86_ECX, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	Pop(x86_EBP);
}

void Compile_Vector_VSUBC ( void ) {
	char Reg[256];
	int count, el, del;

	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	
	#ifndef CompileVsubc
	Cheat_r4300iOpcode(RSP_Vector_VSUBC,"RSP_Vector_VSUBC"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	/* Initialize flag register */
	XorX86RegToX86Reg(x86_ECX, x86_ECX);

	for (count = 0; count < 8; count++) {
		CPU_Message("     Iteration: %i", count);
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];
	
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
		}

		SubX86RegToX86Reg(x86_EAX, x86_EBX);

		XorX86RegToX86Reg(x86_EDX, x86_EDX);
		TestConstToX86Reg(0x0000FFFF, x86_EAX);
		Setnz(x86_EDX);
		ShiftLeftSignImmed(x86_EDX, (BYTE)(15 - el));
		OrX86RegToX86Reg(x86_ECX, x86_EDX);

		XorX86RegToX86Reg(x86_EDX, x86_EDX);
		TestConstToX86Reg(0xFFFF0000, x86_EAX);
		Setnz(x86_EDX);
		ShiftLeftSignImmed(x86_EDX, (BYTE)(7 - el));
		OrX86RegToX86Reg(x86_ECX, x86_EDX);

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
		if (bWriteToDest != FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}
	}
	MoveX86regToVariable(x86_ECX, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
}

void Compile_Vector_VSAW ( void ) {
	char Reg[256];
	DWORD Word;

	#ifndef CompileVsaw
	Cheat_r4300iOpcode(RSP_Vector_VSAW,"RSP_Vector_VSAW"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	switch ((RSPOpC.rs & 0xF)) {
	case 8: Word = 3; break;
	case 9: Word = 2; break;
	case 10: Word = 1; break;
	default:
		MoveConstToVariable(0, &RSP_Vect[RSPOpC.sa].DW[1], "RSP_Vect[RSPOpC.sa].DW[1]");
		MoveConstToVariable(0, &RSP_Vect[RSPOpC.sa].DW[0], "RSP_Vect[RSPOpC.sa].DW[0]");
		return;
	}

	sprintf(Reg, "RSP_ACCUM[1].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[1].HW[Word], Reg, x86_EAX);
	sprintf(Reg, "RSP_ACCUM[3].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[3].HW[Word], Reg, x86_EBX);
	sprintf(Reg, "RSP_ACCUM[5].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[5].HW[Word], Reg, x86_ECX);
	sprintf(Reg, "RSP_ACCUM[7].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[7].HW[Word], Reg, x86_EDX);

	ShiftLeftSignImmed(x86_EAX, 16);
	ShiftLeftSignImmed(x86_EBX, 16);
	ShiftLeftSignImmed(x86_ECX, 16);
	ShiftLeftSignImmed(x86_EDX, 16);

	sprintf(Reg, "RSP_ACCUM[0].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[0].HW[Word], Reg, x86_EAX);
	sprintf(Reg, "RSP_ACCUM[2].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[2].HW[Word], Reg, x86_EBX);
	sprintf(Reg, "RSP_ACCUM[4].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[4].HW[Word], Reg, x86_ECX);
	sprintf(Reg, "RSP_ACCUM[6].HW[%i]", Word);
	MoveVariableToX86regHalf(&RSP_ACCUM[6].HW[Word], Reg, x86_EDX);

	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.sa);
	MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[2]", RSPOpC.sa);
	MoveX86regToVariable(x86_EBX, &RSP_Vect[RSPOpC.sa].HW[2], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.sa);
	MoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[4], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[6]", RSPOpC.sa);
	MoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.sa].HW[6], Reg);
}

void Compile_Vector_VLT ( void ) {
	BOOL bWriteToDest  = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	BYTE *jump[3];
	DWORD flag;
	char Reg[256];
	int el, del, last;
	
#ifndef CompileVlt
	Cheat_r4300iOpcode(RSP_Vector_VLT,"RSP_Vector_VLT");
	return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));
	last = -1;
	XorX86RegToX86Reg(x86_EBX, x86_EBX);
	MoveVariableToX86reg(&RSP_Flags[0].UW, "&RSP_Flags[0].UW", x86_ESI);
	for (el = 0; el < 8; el++) {
		del = EleSpec[RSPOpC.rs].B[el];
		flag = 0x101 << (7 - el);
		if (del != el || RSPOpC.rt != RSPOpC.rd) {
			if (del != last) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_ECX);
				last = del;
			}
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EDX);

			CompX86RegToX86Reg(x86_EDX, x86_ECX);
			JgeLabel8("jge", 0);
			jump[0] = (BYTE*)(RecompPos - 1);

			if (bWriteToAccum || bWriteToDest) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_EDX, &RSP_ACCUM[el].HW[1], Reg);
			}
			OrConstToX86Reg((flag & 0xFF), x86_EBX);

			JmpLabel8("jmp", 0);
			jump[1] = (BYTE*)(RecompPos - 1);
			x86_SetBranch8b(jump[0], RecompPos);

			if (bWriteToAccum || bWriteToDest) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
			}
			JneLabel8("jne", 0);
			jump[2] = (BYTE*)(RecompPos - 1);

			MoveX86RegToX86Reg(x86_ESI, x86_EDI);
			AndConstToX86Reg(x86_EDI, flag);
			ShiftRightUnsignImmed(x86_EDI, 8);
			AndX86RegToX86Reg(x86_EDI, x86_ESI);
			OrX86RegToX86Reg(x86_EBX, x86_EDI);

			x86_SetBranch8b(jump[2], RecompPos);
			x86_SetBranch8b(jump[1], RecompPos);
		} else {
			MoveX86RegToX86Reg(x86_ESI, x86_EDI);
			if (bWriteToAccum || bWriteToDest) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
			}
			AndConstToX86Reg(x86_EDI, flag);
			ShiftRightUnsignImmed(x86_EDI, 8);
			AndX86RegToX86Reg(x86_EDI, x86_ESI);
			OrX86RegToX86Reg(x86_EBX, x86_EDI);
		}
	}

	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	MoveX86regToVariable(x86_EBX, &RSP_Flags[1].UW, "RSP_Flags[1].UW");

	if (bWriteToDest != FALSE) {
		for (el = 0; el < 8; el += 2) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveVariableToX86regHalf(&RSP_ACCUM[el].HW[1], Reg, x86_EAX);

			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el + 1);
			MoveVariableToX86regHalf(&RSP_ACCUM[el + 1].HW[1], Reg, x86_ECX);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el + 1);
			MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el + 1], Reg);
		}
	}
}

void Compile_Vector_VEQ ( void ) {
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	DWORD flag;
	char Reg[256];
	int count, el, del, last = -1;

#ifndef CompileVeq
	Cheat_r4300iOpcode(RSP_Vector_VEQ,"RSP_Vector_VEQ");
	return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	MoveZxVariableToX86regHalf(&RSP_Flags[0].UHW[1], "&RSP_Flags[0].UHW[1]", x86_EBX);
	XorConstToX86Reg(x86_EBX, 0xFFFF);
	for (el = 0; el < 8; el++) {
		del = EleSpec[RSPOpC.rs].B[el];
		flag = (0x101 << (7 - el)) ^ 0xFFFF;
		if (del != el || RSPOpC.rt != RSPOpC.rd) {
			if (del != last) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_ECX);
				last = del;
			}

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EDX);

			if (bWriteToAccum) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
			}

			SubX86RegToX86Reg(x86_EDX, x86_ECX);
			CompConstToX86reg(x86_EDX, 1);
			SbbX86RegToX86Reg(x86_EDX, x86_EDX);
			OrConstToX86Reg(flag, x86_EDX);
			AndX86RegToX86Reg(x86_EBX, x86_EDX);
		} else {
			if (bWriteToAccum) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
			}
		}
	}

	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	MoveX86regToVariable(x86_EBX, &RSP_Flags[1].UW, "RSP_Flags[1].UW");

	if (bWriteToDest != FALSE) {
		for (count = 0; count < 8; count++) {
			el = EleSpec[RSPOpC.rs].B[count];

			if (el != last) {
				sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
				MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EDX);
				last = el;
			}

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, count);
			MoveX86regHalfToVariable(x86_EDX, &RSP_Vect[RSPOpC.sa].HW[count], Reg);
		}
	}
}

void Compile_Vector_VNE ( void ) {
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	DWORD flag;
	char Reg[256];
	int el, del, last = -1;

#ifndef CompileVne
	Cheat_r4300iOpcode(RSP_Vector_VNE,"RSP_Vector_VNE");
	return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	MoveZxVariableToX86regHalf(&RSP_Flags[0].UHW[1], "&RSP_Flags[0].UHW[1]", x86_EBX);

	for (el = 0; el < 8; el++) {
		del = EleSpec[RSPOpC.rs].B[el];
		flag = 0x101 << (7 - el);
		if (del != el || RSPOpC.rt != RSPOpC.rd) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EDX);

			if (del != last) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_ECX);
				last = del;
			}
			if (bWriteToAccum) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_EDX, &RSP_ACCUM[el].HW[1], Reg);
			}
			
			SubX86RegToX86Reg(x86_EDX, x86_ECX);
			NegateX86reg(x86_EDX);
			SbbX86RegToX86Reg(x86_EDX, x86_EDX);
			AndConstToX86Reg(x86_EDX, flag);
			OrX86RegToX86Reg(x86_EBX, x86_EDX);
		} else {
			if (bWriteToAccum) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_EDX, &RSP_ACCUM[el].HW[1], Reg);
			}
		}
	}

	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	MoveX86regToVariable(x86_EBX, &RSP_Flags[1].UW, "RSP_Flags[1].UW");

	if (bWriteToDest != FALSE) {
		for (el = 0; el < 4; el++) {
			sprintf(Reg, "RSP_Vect[%i].W[%i]", RSPOpC.rd, el);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rd].W[el], Reg, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].W[%i]", RSPOpC.sa, el);
			MoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.sa].W[el], Reg);
		}
	}
}

BOOL Compile_Vector_VGE_MMX ( void ) {
	char Reg[256];

	if ((RSPOpC.rs & 0xF) >= 2 && (RSPOpC.rs & 0xF) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	MoveConstToVariable(0, &RSP_Flags[1].UW, "RSP_Flags[1].UW");

	sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].HW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].HW[4], Reg);
	MmxMoveRegToReg(x86_MM2, x86_MM0);
	MmxMoveRegToReg(x86_MM3, x86_MM1);

	if ((RSPOpC.rs & 0x0f) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MmxMoveQwordVariableToReg(x86_MM4, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		MmxMoveQwordVariableToReg(x86_MM5, &RSP_Vect[RSPOpC.rt].HW[4], Reg);
	} else if ((RSPOpC.rs & 0x0f) >= 8) {
		RSP_Element2Mmx(x86_MM4);
	} else {
		RSP_MultiElement2Mmx(x86_MM4, x86_MM5);
	}

	MmxCompareGreaterWordRegToReg(x86_MM2, x86_MM4);
	MmxCompareGreaterWordRegToReg(x86_MM3, ((RSPOpC.rs & 0x0f) >= 8) ? x86_MM4 : x86_MM5);

	MmxPandRegToReg(x86_MM0, x86_MM2);
	MmxPandRegToReg(x86_MM1, x86_MM3);
	MmxPandnRegToReg(x86_MM2, x86_MM4);
	MmxPandnRegToReg(x86_MM3, ((RSPOpC.rs & 0x0f) >= 8) ? x86_MM4 : x86_MM5);

	MmxPorRegToReg(x86_MM0, x86_MM2);
	MmxPorRegToReg(x86_MM1, x86_MM3);
	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	return TRUE;
}

void Compile_Vector_VGE ( void ) {
/*	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	/* FIXME: works ok, but needs careful flag analysis */
/*	#if defined (DLIST)
	if (bWriteToAccum == FALSE && TRUE == Compile_Vector_VGE_MMX()) {
		return;
	}
	#endif
*/
	BOOL bWriteToDest  = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	BYTE *jump[3];
	DWORD flag;
	char Reg[256];
	int el, del, last = -1;

#ifndef CompileVge
	Cheat_r4300iOpcode(RSP_Vector_VGE,"RSP_Vector_VGE");
	return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	XorX86RegToX86Reg(x86_EBX, x86_EBX);
	MoveVariableToX86reg(&RSP_Flags[0].UW, "&RSP_Flags[0].UW", x86_ESI);
	for (el = 0; el < 8; el++) {
		del = EleSpec[RSPOpC.rs].B[el];
		flag = 0x101 << (7 - el);
		if (del != el || RSPOpC.rt != RSPOpC.rd) {
			if (del != last) {
				sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
				MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_ECX);
				last = del;
			}
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EDX);

			CompX86RegToX86Reg(x86_EDX, x86_ECX);
			JleLabel8("jle", 0);
			jump[0] = (BYTE*)(RecompPos - 1);

			if (bWriteToAccum || bWriteToDest) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_EDX, &RSP_ACCUM[el].HW[1], Reg);
			}
			OrConstToX86Reg((flag & 0xFF), x86_EBX);

			JmpLabel8("jmp", 0);
			jump[1] = (BYTE*)(RecompPos - 1);\
			x86_SetBranch8b(jump[0], RecompPos);

			if (bWriteToAccum || bWriteToDest) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
			}

			JneLabel8("jne", 0);
			jump[2] = (BYTE*)(RecompPos - 1);

			MoveX86RegToX86Reg(x86_ESI, x86_EDI);
			AndConstToX86Reg(x86_EDI, flag);
			SubConstFromX86Reg(x86_EDI, flag);
			ShiftRightSignImmed(x86_EDI, 31);
			AndConstToX86Reg(x86_EDI, (flag & 0xFF));
			OrX86RegToX86Reg(x86_EBX, x86_EDI);

			x86_SetBranch8b(jump[1], RecompPos);
			x86_SetBranch8b(jump[2], RecompPos);
		} else {
			MoveX86RegToX86Reg(x86_ESI, x86_EDI);
			if (bWriteToAccum || bWriteToDest) {
				sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
				MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
			}
			AndConstToX86Reg(x86_EDI, flag);
			SubConstFromX86Reg(x86_EDI, flag);
			ShiftRightSignImmed(x86_EDI, 31);
			AndConstToX86Reg(x86_EDI, (flag & 0xFF));
			OrX86RegToX86Reg(x86_EBX, x86_EDI);
		}
	}

	MoveConstToVariable(0, &RSP_Flags[0].UW, "RSP_Flags[0].UW");
	MoveX86regToVariable(x86_EBX, &RSP_Flags[1].UW, "RSP_Flags[1].UW");

	if (bWriteToDest != FALSE) {
		for (el = 0; el < 8; el += 2) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el + 0);
			MoveVariableToX86regHalf(&RSP_ACCUM[el].HW[1], Reg, x86_EAX);

			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el + 1);
			MoveVariableToX86regHalf(&RSP_ACCUM[el + 1].HW[1], Reg, x86_ECX);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el + 0);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el + 0], Reg);

			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el + 1);
			MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el + 1], Reg);
		}
	}
}

void Compile_Vector_VCL ( void ) {
	Cheat_r4300iOpcode(RSP_Vector_VCL,"RSP_Vector_VCL");
}

void Compile_Vector_VCH ( void ) {
	Cheat_r4300iOpcode(RSP_Vector_VCH,"RSP_Vector_VCH");
}

void Compile_Vector_VCR ( void ) {
	Cheat_r4300iOpcode(RSP_Vector_VCR,"RSP_Vector_VCR");
}

void Compile_Vector_VMRG ( void ) {
	char Reg[256];
	int count, el, del;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	#ifndef CompileVmrg
	Cheat_r4300iOpcode(RSP_Vector_VMRG,"RSP_Vector_VMRG"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	MoveVariableToX86reg(&RSP_Flags[1].UW, "RSP_Flags[1].UW", x86_EDX);

	for (count = 0;count < 8; count++) {
		el = Indx[RSPOpC.rs].UB[count];
		del = EleSpec[RSPOpC.rs].UB[el];
		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveZxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);

		TestConstToX86Reg(1 << (7 - el), x86_EDX);
		CondMoveNotEqual(x86_ECX, x86_EAX);
		CondMoveEqual(x86_ECX, x86_EBX);

		if (bWriteToAccum == TRUE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[el].HW[1], Reg);
		}
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
		MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	}
}

BOOL Compile_Vector_VAND_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPandRegToReg(x86_MM0, x86_MM2);
		MmxPandRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MmxPandVariableToReg(&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_MM0);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		MmxPandVariableToReg(&RSP_Vect[RSPOpC.rt].HW[4], Reg, x86_MM1);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPandRegToReg(x86_MM0, x86_MM2);
		MmxPandRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VAND ( void ) {
	char Reg[256];
	int el, del, count;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	#ifndef CompileVand
	Cheat_r4300iOpcode(RSP_Vector_VAND,"RSP_Vector_VAND"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VAND_MMX())
			return;
	}
	
	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);
		
		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			AndVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EAX);
		} else {
			AndX86RegHalfToX86RegHalf(x86_EAX, x86_EBX);
		}

		if (bWriteToDest != FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
	}
}

BOOL Compile_Vector_VNAND_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);
	MmxPcmpeqwRegToReg(x86_MM7, x86_MM7);

	if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPandRegToReg(x86_MM0, x86_MM2);
		MmxPandRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MmxPandVariableToReg(&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_MM0);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		MmxPandVariableToReg(&RSP_Vect[RSPOpC.rt].HW[4], Reg, x86_MM1);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPandRegToReg(x86_MM0, x86_MM2);
		MmxPandRegToReg(x86_MM1, x86_MM3);
	}

	MmxXorRegToReg(x86_MM0, x86_MM7);
	MmxXorRegToReg(x86_MM1, x86_MM7);
	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VNAND ( void ) {
	char Reg[256];
	int el, del, count;
	BOOL bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

#ifndef CompileVnand
	Cheat_r4300iOpcode(RSP_Vector_VNAND, "RSP_Vector_VNAND"); return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VNAND_MMX())
			return;
	}

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			AndVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EAX);
		} else {
			AndX86RegHalfToX86RegHalf(x86_EAX, x86_EBX);
		}

		NotX86reg(x86_EAX);

		if (bWriteToDest != FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
		}

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
	}
}

BOOL Compile_Vector_VOR_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

	if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt)) {

	} else if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPorRegToReg(x86_MM0, x86_MM2);
		MmxPorRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MmxPorVariableToReg(&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_MM0);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		MmxPorVariableToReg(&RSP_Vect[RSPOpC.rt].HW[4], Reg, x86_MM1);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPorRegToReg(x86_MM0, x86_MM2);
		MmxPorRegToReg(x86_MM1, x86_MM3);
	}

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VOR ( void ) {
	char Reg[256];
	int el, del, count;
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

#ifndef CompileVor
	Cheat_r4300iOpcode(RSP_Vector_VOR, "RSP_Vector_VOR"); return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VOR_MMX())
			return;
	}

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			OrVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EAX);
		} else {
			OrX86RegToX86Reg(x86_EAX, x86_EBX);
		}

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
		MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	}
}

BOOL Compile_Vector_VNOR_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
	MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);
	MmxPcmpeqwRegToReg(x86_MM7, x86_MM7);

	if ((RSPOpC.rs & 0xF) >= 8) {
		RSP_Element2Mmx(x86_MM2);
		MmxPorRegToReg(x86_MM0, x86_MM2);
		MmxPorRegToReg(x86_MM1, x86_MM2);
	} else if ((RSPOpC.rs & 0xF) < 2) {
		sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
		MmxPorVariableToReg(&RSP_Vect[RSPOpC.rt].HW[0], Reg, x86_MM0);
		sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
		MmxPorVariableToReg(&RSP_Vect[RSPOpC.rt].HW[4], Reg, x86_MM1);
	} else {
		RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
		MmxPorRegToReg(x86_MM0, x86_MM2);
		MmxPorRegToReg(x86_MM1, x86_MM3);
	}

	MmxXorRegToReg(x86_MM0, x86_MM7);
	MmxXorRegToReg(x86_MM1, x86_MM7);
	sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
	sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
	MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VNOR ( void ) {
	char Reg[256];
	int el, del, count;
	BOOL bElement = ((RSPOpC.rs & 0x0f) >= 8) ? TRUE : FALSE;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

#ifndef CompileVnor
	Cheat_r4300iOpcode(RSP_Vector_VNOR, "RSP_Vector_VNOR"); return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	if (bWriteToAccum == FALSE) {
		if (TRUE == Compile_Vector_VNOR_MMX())
			return;
	}

	if (bElement == TRUE) {
		del = (RSPOpC.rs & 0x07) ^ 7;
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EBX);
	}

	for (count = 0; count < 8; count++) {
		el = Indx[RSPOpC.rs].B[count];
		del = EleSpec[RSPOpC.rs].B[el];

		CPU_Message("     Iteration: %i", count);

		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rd].HW[el], Reg, x86_EAX);

		if (bElement == FALSE) {
			sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.rt, del);
			OrVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[del], Reg, x86_EAX);
		} else {
			OrX86RegToX86Reg(x86_EAX, x86_EBX);
		}

		NotX86reg(x86_EAX);

		if (bWriteToAccum != FALSE) {
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", el);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[el].HW[1], Reg);
		}
		sprintf(Reg, "RSP_Vect[%i].HW[%i]", RSPOpC.sa, el);
		MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	}
}

BOOL Compile_Vector_VXOR_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt)) {
		static DWORD VXOR_DynaRegCount = 0;
		MmxXorRegToReg(VXOR_DynaRegCount, VXOR_DynaRegCount);

		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(VXOR_DynaRegCount, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(VXOR_DynaRegCount, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
		VXOR_DynaRegCount = (VXOR_DynaRegCount + 1) & 7;
	} else {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
		MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
		MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);

		if ((RSPOpC.rs & 0xF) >= 8) {
			RSP_Element2Mmx(x86_MM2);
			MmxXorRegToReg(x86_MM0, x86_MM2);
			MmxXorRegToReg(x86_MM1, x86_MM2);
		} else if ((RSPOpC.rs & 0xF) < 2) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].HW[4], Reg);

			MmxXorRegToReg(x86_MM0, x86_MM2);
			MmxXorRegToReg(x86_MM1, x86_MM3);
		} else {
			RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
			MmxXorRegToReg(x86_MM0, x86_MM2);
			MmxXorRegToReg(x86_MM1, x86_MM3);
		}

		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	}

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VXOR ( void ) {
	#ifdef CompileVxor
		char Reg[256];
		DWORD count;
		BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

		CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

		if (!bWriteToAccum || ((RSPOpC.rs & 0xF) < 2 && RSPOpC.rd == RSPOpC.rt)) {
			if (TRUE == Compile_Vector_VXOR_MMX()) {
				if (bWriteToAccum == TRUE) {
					XorX86RegToX86Reg(x86_EAX, x86_EAX);
					for (count = 0; count < 8; count++) {
						sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
						MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
					}
				}
				return;
			}
		}
	#endif

	Cheat_r4300iOpcodeNoMessage(RSP_Vector_VXOR, "RSP_Vector_VXOR");
}

BOOL Compile_Vector_VNXOR_MMX ( void ) {
	char Reg[256];

	/* Do our MMX checks here */
	if (IsMmxEnabled == FALSE)
		return FALSE;
	if ((RSPOpC.rs & 0x0f) >= 2 && (RSPOpC.rs & 0x0f) <= 7 && IsMmx2Enabled == FALSE)
		return FALSE;

	if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt)) {
		static DWORD VNXOR_DynaRegCount = 0;
		MmxPcmpeqwRegToReg(VNXOR_DynaRegCount, VNXOR_DynaRegCount);

		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(VNXOR_DynaRegCount, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(VNXOR_DynaRegCount, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
		VNXOR_DynaRegCount = (VNXOR_DynaRegCount + 1) & 7;
	} else {
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.rd);
		MmxMoveQwordVariableToReg(x86_MM0, &RSP_Vect[RSPOpC.rd].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.rd);
		MmxMoveQwordVariableToReg(x86_MM1, &RSP_Vect[RSPOpC.rd].UHW[4], Reg);
		MmxPcmpeqwRegToReg(x86_MM7, x86_MM7);

		if ((RSPOpC.rs & 0xF) >= 8) {
			RSP_Element2Mmx(x86_MM2);
			MmxXorRegToReg(x86_MM0, x86_MM2);
			MmxXorRegToReg(x86_MM1, x86_MM2);
		} else if ((RSPOpC.rs & 0xF) < 2) {
			sprintf(Reg, "RSP_Vect[%i].HW[0]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM2, &RSP_Vect[RSPOpC.rt].HW[0], Reg);
			sprintf(Reg, "RSP_Vect[%i].HW[4]", RSPOpC.rt);
			MmxMoveQwordVariableToReg(x86_MM3, &RSP_Vect[RSPOpC.rt].HW[4], Reg);

			MmxXorRegToReg(x86_MM0, x86_MM2);
			MmxXorRegToReg(x86_MM1, x86_MM3);
		} else {
			RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
			MmxXorRegToReg(x86_MM0, x86_MM2);
			MmxXorRegToReg(x86_MM1, x86_MM3);
		}

		MmxXorRegToReg(x86_MM0, x86_MM7);
		MmxXorRegToReg(x86_MM1, x86_MM7);
		sprintf(Reg, "RSP_Vect[%i].UHW[0]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(x86_MM0, &RSP_Vect[RSPOpC.sa].UHW[0], Reg);
		sprintf(Reg, "RSP_Vect[%i].UHW[4]", RSPOpC.sa);
		MmxMoveQwordRegToVariable(x86_MM1, &RSP_Vect[RSPOpC.sa].UHW[4], Reg);
	}

	if (IsNextInstructionMmx(CompilePC) == FALSE)
		MmxEmptyMultimediaState();

	return TRUE;
}

void Compile_Vector_VNXOR ( void ) {
	#ifdef CompileVnxor
		char Reg[256];
		DWORD count;
		BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

		CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

		if (!bWriteToAccum || ((RSPOpC.rs & 0xF) < 2 && RSPOpC.rd == RSPOpC.rt)) {
			if (TRUE == Compile_Vector_VNXOR_MMX()) {
				if (bWriteToAccum == TRUE) {
					OrConstToX86Reg(0xFFFFFFFF, x86_EAX);
					for (count = 0; count < 8; count++) {
						sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
						MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
					}
				}
				return;
			}
		}
	#endif

	Cheat_r4300iOpcode(RSP_Vector_VNXOR, "RSP_Vector_VNXOR");
}

void Compile_Vector_VRCP ( void ) {
	char Reg[256];
	int count, el, last;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	DWORD *end = NULL;
	
#ifndef CompileVrcp
	Cheat_r4300iOpcode(RSP_Vector_VRCP,"RSP_Vector_VRCP");
	return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
	MoveSxVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[el], Reg, x86_ESI);
	MoveConstToX86reg(0x7FFFFFFF, x86_EAX);
	TestX86RegToX86Reg(x86_ESI, x86_ESI);
	MoveX86RegToX86Reg(x86_ESI, x86_EDI);
	JeLabel32("Done", 0);
	end = (DWORD*)(RecompPos - 4);

	MoveConstToX86reg(0xFFC0, x86_EBX);
	ShiftRightSignImmed(x86_ESI, 31);
	XorX86RegToX86Reg(x86_EDX, x86_EDX);
	XorX86RegToX86Reg(x86_EDI, x86_ESI);
	SubX86RegToX86Reg(x86_EDI, x86_ESI);

	BsrX86RegToX86Reg(x86_ECX, x86_EDI);
	XorConstToX86Reg(x86_ECX, 15);
	ShiftRightUnsign(x86_EBX);
	AndX86RegToX86Reg(x86_EDI, x86_EBX);

	idivX86reg(x86_EDI);

	MoveConstToX86reg(0xFFFF8000, x86_EBX);
	BsrX86RegToX86Reg(x86_ECX, x86_EAX);
	XorConstToX86Reg(x86_ECX, 31);
	ShiftRightUnsign(x86_EBX);
	AndX86RegToX86Reg(x86_EAX, x86_EBX);
	XorX86RegToX86Reg(x86_EAX, x86_ESI);

	x86_SetBranch32b(end, RecompPos);

	if (bWriteToAccum != FALSE){
		last = -1;
		for (count = 0; count < 8; count++) {
			el = EleSpec[RSPOpC.rs].B[count];

			if (el != last) {
				sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
				MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_ECX);
				last = el;
			}

			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
			MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[count].HW[1], Reg);
		}
	}

	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);
	MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	MoveX86regToVariable(x86_EAX, &RecpResult.W, "RecpResult.W");
}

void Compile_Vector_VRCPL ( void ) {
	char Reg[256];
	int count, el, last;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
	DWORD *end = NULL;

#ifndef CompileVrcpl
	Cheat_r4300iOpcode(RSP_Vector_VRCPL,"RSP_Vector_VRCPL");
	return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
	MoveVariableToX86reg(&Recp.W, "Recp.W", x86_ESI);
	OrVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].HW[el], Reg, x86_ESI);
	
	MoveConstToX86reg(0x7FFFFFFF, x86_EAX);
	TestX86RegToX86Reg(x86_ESI, x86_ESI);
	MoveX86RegToX86Reg(x86_ESI, x86_EDI);
	JeLabel32("Done", 0);
	end = (DWORD*)(RecompPos - 4);

	MoveConstToX86reg(0xFFC00000, x86_EBX);
	ShiftRightSignImmed(x86_ESI, 31);
	MoveX86RegToX86Reg(x86_EDI, x86_ECX);
	MoveZxX86RegHalfToX86Reg(x86_EDI, x86_EDX);
	OrConstToX86Reg(0xFFFF, x86_ECX);
	ShiftRightUnsignImmed(x86_EDX, 15);

	XorX86RegToX86Reg(x86_EDI, x86_ESI);
	AddX86RegToX86Reg(x86_ECX, x86_EDX);
	AdcConstToX86reg(0, x86_EDI);
	XorX86RegToX86Reg(x86_EDX, x86_EDX);

	BsrX86RegToX86Reg(x86_ECX, x86_EDI);
	XorConstToX86Reg(x86_ECX, 31);
	ShiftRightUnsign(x86_EBX);
	AndX86RegToX86Reg(x86_EDI, x86_EBX);

	idivX86reg(x86_EDI);
	MoveConstToX86reg(0xFFFF8000, x86_EBX);
	BsrX86RegToX86Reg(x86_ECX, x86_EAX);
	XorConstToX86Reg(x86_ECX, 31);
	ShiftRightUnsign(x86_EBX);
	AndX86RegToX86Reg(x86_EAX, x86_EBX);
	XorX86RegToX86Reg(x86_EAX, x86_ESI);

	x86_SetBranch32b(end, RecompPos);

	if (bWriteToAccum != FALSE){
		last = -1;
		for (count = 0; count < 8; count++) {
			el = EleSpec[RSPOpC.rs].B[count];

			if (el != last) {
				sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
				MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_ECX);
				last = el;
			}

			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
			MoveX86regHalfToVariable(x86_ECX, &RSP_ACCUM[count].HW[1], Reg);
		}
	}

	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);
	MoveX86regHalfToVariable(x86_EAX, &RSP_Vect[RSPOpC.sa].HW[el], Reg);
	MoveX86regToVariable(x86_EAX, &RecpResult.W, "RecpResult.W");
}

void Compile_Vector_VRCPH ( void ) {
	char Reg[256];
	int count, el, last;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	#ifndef CompileVrcph
	Cheat_r4300iOpcode(RSP_Vector_VRCPH,"RSP_Vector_VRCPH"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
	MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EDX);
	MoveX86regHalfToVariable(x86_EDX, &Recp.UHW[1], "Recp.UHW[1]");
	
	MoveVariableToX86regHalf(&RecpResult.UHW[1], "RecpResult.UHW[1]", x86_ECX);
	
	if (bWriteToAccum != FALSE) {
		last = -1;
		for (count = 0; count < 8; count++) {
			el = EleSpec[RSPOpC.rs].B[count];

			if (el != last) {
				sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
				MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EAX);
				last = el;
			}

			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
		}
	}
	
	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);
	MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].UHW[el], Reg);
}

void Compile_Vector_VMOV ( void ) {
	char Reg[256];
	int el, count;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
#ifndef CompileVmov
	Cheat_r4300iOpcode(RSP_Vector_VMOV, "RSP_Vector_VMOV"); return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	if (bWriteToAccum){
		for (count = 0; count < 8; count++) {
			sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, EleSpec[RSPOpC.rs].B[count]);
			MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[EleSpec[RSPOpC.rs].B[count]], Reg, x86_EAX);
			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
		}
	}

	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);

	MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_ECX);

	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);

	MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].UHW[el], Reg);
}

void Compile_Vector_VRSQ ( void ) {
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	Cheat_r4300iOpcodeNoMessage(RSP_Vector_VRSQ,"RSP_Vector_VRSQ");
}

void Compile_Vector_VRSQL ( void ) {
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	Cheat_r4300iOpcodeNoMessage(RSP_Vector_VRSQL,"RSP_Vector_VRSQL");
}

void Compile_Vector_VRSQH ( void ) {
	char Reg[256];
	int count, el, last;
	BOOL bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	#ifndef CompileVrsqh
	Cheat_r4300iOpcode(RSP_Vector_VRSQH,"RSP_Vector_VRSQH"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	
	el = EleSpec[RSPOpC.rs].B[(RSPOpC.rd & 0x7)];
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
	MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EDX);
	MoveX86regHalfToVariable(x86_EDX, &SQroot.UHW[1], "SQroot.UHW[1]");

	MoveVariableToX86regHalf(&SQrootResult.UHW[1], "SQrootResult.UHW[1]", x86_ECX);
	
	if (bWriteToAccum != FALSE) {
		last = -1;
		for (count = 0; count < 8; count++) {
			el = EleSpec[RSPOpC.rs].B[count];
			
			if(el != last){
				sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.rt, el);
				MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].UHW[el], Reg, x86_EAX);
				last = el;
			}

			sprintf(Reg, "RSP_ACCUM[%i].HW[1]", count);
			MoveX86regHalfToVariable(x86_EAX, &RSP_ACCUM[count].HW[1], Reg);
		}
	}
	
	el = 7 - (RSPOpC.rd & 0x7);
	sprintf(Reg, "RSP_Vect[%i].UHW[%i]", RSPOpC.sa, el);
	MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.sa].UHW[el], Reg);
}

void Compile_Vector_VNOOP ( void ) {

}

/************************** lc2 functions **************************/

void Compile_Opcode_LBV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_LBV,"RSP_Opcode_LBV");
}

void Compile_Opcode_LSV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 1);

	if (RSPOpC.del > 14) {
		rsp_UnknownOpcode();
		return;
	}

	#ifndef CompileLsv
	Cheat_r4300iOpcode(RSP_Opcode_LSV,"RSP_Opcode_LSV"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));
	
	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 1) != 0) {
			sprintf(Reg, "Dmem + %Xh", (Addr + 0) ^ 3);
			MoveVariableToX86regByte(RSPInfo.DMEM + ((Addr + 0) ^ 3), Reg, x86_ECX);
			sprintf(Reg, "Dmem + %Xh", (Addr + 1) ^ 3);
			MoveVariableToX86regByte(RSPInfo.DMEM + ((Addr + 1) ^ 3), Reg, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
			MoveX86regByteToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			MoveX86regByteToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
		} else {
			sprintf(Reg, "Dmem + %Xh", Addr ^ 2);
			MoveVariableToX86regHalf(RSPInfo.DMEM + (Addr ^ 2), Reg, x86_EDX);
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			MoveX86regHalfToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
		}
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) AddConstToX86Reg(x86_EBX, offset);
	AndConstToX86Reg(x86_EBX, 0x0FFF);

	if (Compiler.bAlignVector == TRUE) {
		XorConstToX86Reg(x86_EBX, 2);
		MoveN64MemToX86regHalf(x86_ECX, x86_EBX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		MoveX86regHalfToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
	} else {
		LeaSourceAndOffset(x86_EAX, x86_EBX, 1);
		XorConstToX86Reg(x86_EBX, 3);
		XorConstToX86Reg(x86_EAX, 3);

		MoveN64MemToX86regByte(x86_ECX, x86_EBX);
		MoveN64MemToX86regByte(x86_EDX, x86_EAX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
		MoveX86regByteToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		MoveX86regByteToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg);
	}
}

void Compile_Opcode_LLV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 2);
	BYTE * Jump[2];

	#ifndef CompileLlv
	Cheat_r4300iOpcode(RSP_Opcode_LLV,"RSP_Opcode_LLV"); return;
	#endif
	
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if ((RSPOpC.del & 0x3) != 0) {
		Cheat_r4300iOpcode(RSP_Opcode_LLV,"RSP_Opcode_LLV"); return;
		return;
	}

 	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned LLV at constant address");
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LLV,"RSP_Opcode_LLV");
			return;
		}

		sprintf(Reg, "Dmem + %Xh", Addr);
		MoveVariableToX86reg(RSPInfo.DMEM + Addr, Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) AddConstToX86Reg(x86_EBX, offset);
	
	TestConstToX86Reg(3, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	/*
	 * Unaligned
	 */
	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	*((DWORD *)(Jump[0]))=(DWORD)(RecompPos - Jump[0] - 4);
	Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LLV,"RSP_Opcode_LLV");
	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;

	CompilerToggleBuffer();

	/*
	 * Aligned
	 */
	AndConstToX86Reg(x86_EBX, 0x0fff);
	MoveN64MemToX86reg(x86_EAX, x86_EBX);
	/* Because of byte swapping this swizzle works nicely */
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);

	CPU_Message("   Done:");
	*((DWORD *)(Jump[1]))=(DWORD)(RecompPos - Jump[1] - 4);
}

void Compile_Opcode_LDV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 3), length;
	BYTE * Jump[2], * LoopEntry;

	#ifndef CompileLdv
	Cheat_r4300iOpcode(RSP_Opcode_LDV,"RSP_Opcode_LDV"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	/* FIXME: Conker's hits this */
	//if ((RSPOpC.del & 0x7) != 0) {
	//	rsp_UnknownOpcode();
	//	return;
	//}
	if ((RSPOpC.del & 0x3) != 0) {
		CompilerWarning("LDV's element = %X, PC = %04X", RSPOpC.del, CompilePC);
		Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LDV,"RSP_Opcode_LDV");
		return;
	}

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned LDV at constant address PC = %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LDV,"RSP_Opcode_LDV");
			return;
		}

		sprintf(Reg, "Dmem + %Xh", Addr);
		MoveVariableToX86reg(RSPInfo.DMEM + Addr + 0, Reg, x86_EAX);
		sprintf(Reg, "Dmem + %Xh", Addr + 4);
		MoveVariableToX86reg(RSPInfo.DMEM + Addr + 4, Reg, x86_ECX);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);
		if (RSPOpC.del != 12){
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
			MoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 8], Reg);
		}
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		AddConstToX86Reg(x86_EBX, offset);
	}
	AndConstToX86Reg(x86_EBX, 0x0fff);
	TestConstToX86Reg(3, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	x86_SetBranch32b(Jump[0], RecompPos);
	sprintf(Reg, "RSP_Vect[%i].UB[%i]", RSPOpC.rt, 15 - RSPOpC.del);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].UB[15 - RSPOpC.del], Reg, x86_EDI);
	length = 8;
	if (RSPOpC.del == 12){
		length = 4;
	}
	MoveConstToX86reg(length, x86_ECX);

/*    mov eax, ebx
      dec edi
      xor eax, 3h
      inc ebx
      mov dl, byte ptr [eax+Dmem]
      dec ecx
      mov byte ptr [edi+1], dl      
      jne $Loop */

	LoopEntry = RecompPos;
	CPU_Message("   Loop:");
	MoveX86RegToX86Reg(x86_EBX, x86_EAX);
	XorConstToX86Reg(x86_EAX, 3);
	MoveN64MemToX86regByte(x86_EDX, x86_EAX);
	MoveX86regByteToX86regPointer(x86_EDX, x86_EDI);
	IncX86reg(x86_EBX); /* address constant */
	DecX86reg(x86_EDI); /* vector pointer */
	DecX86reg(x86_ECX); /* counter */
	JneLabel8("Loop", 0);
	x86_SetBranch8b(RecompPos - 1, LoopEntry);

	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;
	CompilerToggleBuffer();

	MoveN64MemToX86reg(x86_EAX, x86_EBX);
	MoveN64MemDispToX86reg(x86_ECX, x86_EBX, 4);
	
	/* Because of byte swapping this swizzle works nicely */
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg);
	if (RSPOpC.del != 12){
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
		MoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 8], Reg);
	}
	CPU_Message("   Done:");
	x86_SetBranch32b(Jump[1], RecompPos);
}

void Compile_Opcode_LQV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 4);
	BYTE * Jump[2];

	#ifndef CompileLqv
	Cheat_r4300iOpcode(RSP_Opcode_LQV,"RSP_Opcode_LQV"); return;
	#endif
	
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.del != 0) {
		Cheat_r4300iOpcode(RSP_Opcode_LQV,"RSP_Opcode_LQV"); return;
		return;
	}

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if (Addr & 15) {
			CompilerWarning("Unaligned LQV at constant address PC = %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LQV,"RSP_Opcode_LQV");
			return;
		}
	
		/*
		 * Aligned store
		 */

		if (IsSseEnabled == FALSE) {
			sprintf(Reg, "Dmem+%Xh+0", Addr);
			MoveVariableToX86reg(RSPInfo.DMEM + Addr + 0, Reg, x86_EAX);
			sprintf(Reg, "Dmem+%Xh+4", Addr);
			MoveVariableToX86reg(RSPInfo.DMEM + Addr + 4, Reg, x86_EBX);
			sprintf(Reg, "Dmem+%Xh+8", Addr);
			MoveVariableToX86reg(RSPInfo.DMEM + Addr + 8, Reg, x86_ECX);
			sprintf(Reg, "Dmem+%Xh+C", Addr);
			MoveVariableToX86reg(RSPInfo.DMEM + Addr + 12, Reg, x86_EDX);

			sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
			MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[12], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
			MoveX86regToVariable(x86_EBX, &RSP_Vect[RSPOpC.rt].B[8], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
			MoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[4], Reg);
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			MoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[0], Reg);
		} else {
			sprintf(Reg, "Dmem+%Xh", Addr);
			SseMoveUnalignedVariableToReg(RSPInfo.DMEM + Addr, Reg, x86_XMM0);
			SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			SseMoveAlignedRegToVariable(x86_XMM0, &RSP_Vect[RSPOpC.rt].B[0], Reg);
		}
		return;
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		AddConstToX86Reg(x86_EBX, offset);
	}
	TestConstToX86Reg(15, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	x86_SetBranch32b(Jump[0], RecompPos);

	Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LQV,"RSP_Opcode_LQV");
	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;
	CompilerToggleBuffer();

	AndConstToX86Reg(x86_EBX, 0x0fff);
	if (IsSseEnabled == FALSE) {
		MoveN64MemDispToX86reg(x86_EAX, x86_EBX, 0);
		MoveN64MemDispToX86reg(x86_ECX, x86_EBX, 4);
		MoveN64MemDispToX86reg(x86_EDX, x86_EBX, 8);
		MoveN64MemDispToX86reg(x86_EDI, x86_EBX, 12);

		sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
		MoveX86regToVariable(x86_EAX, &RSP_Vect[RSPOpC.rt].B[12], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
		MoveX86regToVariable(x86_ECX, &RSP_Vect[RSPOpC.rt].B[8], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
		MoveX86regToVariable(x86_EDX, &RSP_Vect[RSPOpC.rt].B[4], Reg);
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		MoveX86regToVariable(x86_EDI, &RSP_Vect[RSPOpC.rt].B[0], Reg);
	} else {
		SseMoveUnalignedN64MemToReg(x86_XMM0, x86_EBX);
		SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		SseMoveAlignedRegToVariable(x86_XMM0, &RSP_Vect[RSPOpC.rt].B[0], Reg);
	}
	CPU_Message("   Done:");
	x86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RecompPos);
}

void Compile_Opcode_LRV ( void ) {
	int offset = (RSPOpC.voffset << 4);
	BYTE * Loop, * Jump[2];

	#ifndef CompileLrv
	Cheat_r4300iOpcode(RSP_Opcode_LRV,"RSP_Opcode_LRV"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.del != 0) {
		rsp_UnknownOpcode();
		return;
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) AddConstToX86Reg(x86_EBX, offset);

	if (Compiler.bAlignVector == FALSE) {
		TestConstToX86Reg(1, x86_EBX);
		JneLabel32("Unaligned", 0);
		Jump[0] = RecompPos - 4;

		/* Unaligned */
		CompilerToggleBuffer();

		CPU_Message(" Unaligned:");
		x86_SetBranch32b(Jump[0], RecompPos);

		Cheat_r4300iOpcodeNoMessage(RSP_Opcode_LRV,"RSP_Opcode_LRV");
		JmpLabel32("Done", 0);
		Jump[1] = RecompPos - 4;

		CompilerToggleBuffer();
	}

	/* Aligned */
	MoveX86RegToX86Reg(x86_EBX, x86_EAX);
	AndConstToX86Reg(x86_EAX, 0x0F);
	AndConstToX86Reg(x86_EBX, 0x0ff0);

	MoveX86RegToX86Reg(x86_EAX, x86_ECX);
	ShiftRightUnsignImmed(x86_ECX, 1);

	JeLabel8("Done", 0);
	Jump[0] = RecompPos - 1;
/*
	DecX86reg(x86_EAX);
	LeaSourceAndOffset(x86_EAX, x86_EAX, (DWORD) &RSP_Vect[RSPOpC.rt].B[0]);
	DecX86reg(x86_EAX);
*/
	AddConstToX86Reg(x86_EAX, ((DWORD)&RSP_Vect[RSPOpC.rt].UB[0]) - 2);

	CPU_Message("   Loop:");
	Loop = RecompPos;

	MoveX86RegToX86Reg(x86_EBX, x86_ESI);
	XorConstToX86Reg(x86_ESI, 2);
	MoveN64MemToX86regHalf(x86_EDX, x86_ESI);
	MoveX86regHalfToX86regPointer(x86_EDX, x86_EAX);

	AddConstToX86Reg(x86_EBX, 2);   /* Dmem pointer */
	SubConstFromX86Reg(x86_EAX, 2); /* Vector pointer */
	DecX86reg(x86_ECX);             /* Loop counter */
	JneLabel8("Loop", 0);
	x86_SetBranch8b(RecompPos - 1, Loop);

	if (Compiler.bAlignVector == FALSE) {
		CPU_Message("   Done:");
		x86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RecompPos);
	}

	x86_SetBranch8b(Jump[0], RecompPos);
}

void Compile_Opcode_LPV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_LPV,"RSP_Opcode_LPV");
}

void Compile_Opcode_LUV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_LUV,"RSP_Opcode_LUV");
}


void Compile_Opcode_LHV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_LHV,"RSP_Opcode_LHV");
}


void Compile_Opcode_LFV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_LFV,"RSP_Opcode_LFV");
}

void Compile_Opcode_LTV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_LTV,"RSP_Opcode_LTV");
}

/************************** sc2 functions **************************/

void Compile_Opcode_SBV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SBV,"RSP_Opcode_SBV");
}

void Compile_Opcode_SSV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 1);

	if (RSPOpC.del > 14) {
		rsp_UnknownOpcode();
		return;
	}

	#ifndef CompileSsv
	Cheat_r4300iOpcode(RSP_Opcode_SSV,"RSP_Opcode_SSV"); return;
	#endif
	
	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 1) != 0) {
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
			MoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg, x86_ECX);
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			MoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_EDX);

			sprintf(Reg, "Dmem + %Xh", (Addr + 0) ^ 3);
			MoveX86regByteToVariable(x86_ECX, RSPInfo.DMEM + ((Addr + 0) ^ 3), Reg);
			sprintf(Reg, "Dmem + %Xh", (Addr + 1) ^ 3);
			MoveX86regByteToVariable(x86_EDX, RSPInfo.DMEM + ((Addr + 1) ^ 3), Reg);
		} else {
			sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
			MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_ECX);
			sprintf(Reg, "Dmem + %Xh", Addr ^ 2);
			MoveX86regHalfToVariable(x86_ECX, RSPInfo.DMEM + (Addr ^ 2), Reg);
		}
		return;
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) AddConstToX86Reg(x86_EBX, offset);
	AndConstToX86Reg(x86_EBX, 0x0FFF);

	if (Compiler.bAlignVector == TRUE) {
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		MoveVariableToX86regHalf(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_ECX);
		XorConstToX86Reg(x86_EBX, 2);
		MoveX86regHalfToN64Mem(x86_ECX, x86_EBX);
	} else {
		LeaSourceAndOffset(x86_EAX, x86_EBX, 1);
		XorConstToX86Reg(x86_EBX, 3);
		XorConstToX86Reg(x86_EAX, 3);

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
		MoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 0)], Reg, x86_ECX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
		MoveVariableToX86regByte(&RSP_Vect[RSPOpC.rt].B[15 - (RSPOpC.del + 1)], Reg, x86_EDX);

		MoveX86regByteToN64Mem(x86_ECX, x86_EBX);
		MoveX86regByteToN64Mem(x86_EDX, x86_EAX);
	}
}

void Compile_Opcode_SLV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 2);
	BYTE * Jump[2];

	#ifndef CompileSlv
	Cheat_r4300iOpcode(RSP_Opcode_SLV,"RSP_Opcode_SLV"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

//	if ((RSPOpC.del & 0x3) != 0) {
//		rsp_UnknownOpcode();
//		return;
//	}

 	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned SLV at constant address");
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SLV,"RSP_Opcode_SLV");
			return;
		}

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
		MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg, x86_EAX);
		sprintf(Reg, "Dmem + %Xh", Addr);
		MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr, Reg);
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) AddConstToX86Reg(x86_EBX, offset);
	
	TestConstToX86Reg(3, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	/*
	 * Unaligned
	 */
	CompilerToggleBuffer();

	CPU_Message("   Unaligned:");
	*((DWORD *)(Jump[0]))=(DWORD)(RecompPos - Jump[0] - 4);
	Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SLV,"RSP_Opcode_SLV");
	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;

	CompilerToggleBuffer();

	/*
	 * Aligned
	 */

	/* Because of byte swapping this swizzle works nicely */
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
	MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[16 - RSPOpC.del - 4], Reg, x86_EAX);
	
	AndConstToX86Reg(x86_EBX, 0x0fff);
	MoveX86regToN64Mem(x86_EAX, x86_EBX);

	CPU_Message("   Done:");
	*((DWORD *)(Jump[1]))=(DWORD)(RecompPos - Jump[1] - 4);
}

void Compile_Opcode_SDV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 3);
	BYTE * Jump[2], * LoopEntry;

	//if ((RSPOpC.del & 0x7) != 0) {
	//	rsp_UnknownOpcode();
	//	return;
	//}

#ifndef CompileSdv
	Cheat_r4300iOpcode(RSP_Opcode_SDV, "RSP_Opcode_SDV"); return;
#endif

	CPU_Message("  %X %s", CompilePC, RSPOpcodeName(RSPOpC.Hex, CompilePC));

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if ((Addr & 3) != 0) {
			CompilerWarning("Unaligned SDV at constant address PC = %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SDV, "RSP_Opcode_SDV");
			return;
		}

		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 4) & 0xF);
		MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[(16 - RSPOpC.del - 4) & 0xF], Reg, x86_EAX);
		sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 8) & 0xF);
		MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[(16 - RSPOpC.del - 8) & 0xF], Reg, x86_EBX);

		sprintf(Reg, "Dmem + %Xh", Addr);
		MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr, Reg);
		sprintf(Reg, "Dmem + %Xh", Addr + 4);
		MoveX86regToVariable(x86_EBX, RSPInfo.DMEM + Addr + 4, Reg);
		return;
	}

	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		AddConstToX86Reg(x86_EBX, offset);
	}
	AndConstToX86Reg(x86_EBX, 0x0fff);
	TestConstToX86Reg(3, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	x86_SetBranch32b((DWORD*)Jump[0], (DWORD*)RecompPos);

	sprintf(Reg, "RSP_Vect[%i].UB[%i]", RSPOpC.rt, 15 - RSPOpC.del);
	MoveOffsetToX86reg((DWORD)&RSP_Vect[RSPOpC.rt].UB[15 - RSPOpC.del], Reg, x86_EDI);
	MoveConstToX86reg(8, x86_ECX);

	CPU_Message("   Loop:");
	LoopEntry = RecompPos;
	MoveX86RegToX86Reg(x86_EBX, x86_EAX);
	XorConstToX86Reg(x86_EAX, 3);
	MoveX86regPointerToX86regByte(x86_EDX, x86_EDI);
	MoveX86regByteToN64Mem(x86_EDX, x86_EAX);
	IncX86reg(x86_EBX); /* address constant */
	DecX86reg(x86_EDI); /* vector pointer */
	DecX86reg(x86_ECX); /* counter */
	JneLabel8("Loop", 0);
	x86_SetBranch8b(RecompPos - 1, LoopEntry);

	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;
	CompilerToggleBuffer();

	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 4) & 0xF);
	MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[(16 - RSPOpC.del - 4) & 0xF], Reg, x86_EAX);
	sprintf(Reg, "RSP_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 8) & 0xF);
	MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[(16 - RSPOpC.del - 8) & 0xF], Reg, x86_ECX);
	MoveX86regToN64Mem(x86_EAX, x86_EBX);
	MoveX86regToN64MemDisp(x86_ECX, x86_EBX, 4);

	CPU_Message("   Done:");
	x86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RecompPos);
}

void Compile_Opcode_SQV ( void ) {
	char Reg[256];
	int offset = (RSPOpC.voffset << 4);
	BYTE * Jump[2];

	#ifndef CompileSqv
 	Cheat_r4300iOpcode(RSP_Opcode_SQV,"RSP_Opcode_SQV"); return;
	#endif

	CPU_Message("  %X %s",CompilePC,RSPOpcodeName(RSPOpC.Hex,CompilePC));

	if (RSPOpC.del != 0 && RSPOpC.del != 12) {
		rsp_UnknownOpcode();
		return;
	}

	if (IsRegConst(RSPOpC.base) == TRUE) {
		DWORD Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

		if (Addr & 15) {
			CompilerWarning("Unaligned SQV at constant address %04X", CompilePC);
			Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SQV,"RSP_Opcode_SQV");
			return;
		}
	
		/*
		 * Aligned store
		 */

		if (IsSseEnabled == FALSE) {
			if (RSPOpC.del == 12) {
				sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_EAX);
				sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[12], Reg, x86_EBX);
				sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[8], Reg, x86_ECX);
				sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[4], Reg, x86_EDX);
			} else {
				sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[12], Reg, x86_EAX);
				sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[8], Reg, x86_EBX);
				sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[4], Reg, x86_ECX);
				sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
				MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_EDX);
			}

			sprintf(Reg, "Dmem+%Xh+0", Addr);
			MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr + 0, Reg);
			sprintf(Reg, "Dmem+%Xh+4", Addr);
			MoveX86regToVariable(x86_EBX, RSPInfo.DMEM + Addr + 4, Reg);
			sprintf(Reg, "Dmem+%Xh+8", Addr);
			MoveX86regToVariable(x86_ECX, RSPInfo.DMEM + Addr + 8, Reg);
			sprintf(Reg, "Dmem+%Xh+C", Addr);
			MoveX86regToVariable(x86_EDX, RSPInfo.DMEM + Addr + 12, Reg);
		} else {
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			SseMoveAlignedVariableToReg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_XMM0);
			if (RSPOpC.del == 12) {
				SseShuffleReg(x86_XMM0, x86_MM0, 0x6c);
			} else {
				SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
			}
			sprintf(Reg, "Dmem+%Xh", Addr);
			SseMoveUnalignedRegToVariable(x86_XMM0, RSPInfo.DMEM + Addr, Reg);
		}
		return;
	}
	
	MoveVariableToX86reg(&RSP_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
	if (offset != 0) {
		AddConstToX86Reg(x86_EBX, offset);
	}
	TestConstToX86Reg(15, x86_EBX);
	JneLabel32("Unaligned", 0);
	Jump[0] = RecompPos - 4;

	CompilerToggleBuffer();
	CPU_Message("   Unaligned:");
	x86_SetBranch32b((DWORD*)Jump[0], (DWORD*)RecompPos);
	Cheat_r4300iOpcodeNoMessage(RSP_Opcode_SQV,"RSP_Opcode_SQV");
	JmpLabel32("Done", 0);
	Jump[1] = RecompPos - 4;
	CompilerToggleBuffer();

	AndConstToX86Reg(x86_EBX, 0x0fff);
	if (IsSseEnabled == FALSE) {
		if (RSPOpC.del == 12) {
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_EAX);
			sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[12], Reg, x86_ECX);
			sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[8], Reg, x86_EDX);
			sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[4], Reg, x86_EDI);
		} else {
			sprintf(Reg, "RSP_Vect[%i].B[12]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[12], Reg, x86_EAX);
			sprintf(Reg, "RSP_Vect[%i].B[8]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[8], Reg, x86_ECX);
			sprintf(Reg, "RSP_Vect[%i].B[4]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[4], Reg, x86_EDX);
			sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
			MoveVariableToX86reg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_EDI);
		}

		MoveX86regToN64MemDisp(x86_EAX, x86_EBX, 0);
		MoveX86regToN64MemDisp(x86_ECX, x86_EBX, 4);
		MoveX86regToN64MemDisp(x86_EDX, x86_EBX, 8);
		MoveX86regToN64MemDisp(x86_EDI, x86_EBX, 12);
	} else {
		sprintf(Reg, "RSP_Vect[%i].B[0]", RSPOpC.rt);
		SseMoveAlignedVariableToReg(&RSP_Vect[RSPOpC.rt].B[0], Reg, x86_XMM0);
		if (RSPOpC.del == 12) {
			SseShuffleReg(x86_XMM0, x86_MM0, 0x6c);
		} else {
			SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
		}
		SseMoveUnalignedRegToN64Mem(x86_XMM0, x86_EBX);
	}
	CPU_Message("   Done:");
	x86_SetBranch32b((DWORD*)Jump[1], (DWORD*)RecompPos);
}

void Compile_Opcode_SRV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SRV,"RSP_Opcode_SRV");
}

void Compile_Opcode_SPV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SPV,"RSP_Opcode_SPV");
}

void Compile_Opcode_SUV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SUV,"RSP_Opcode_SUV");
}

void Compile_Opcode_SHV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SHV,"RSP_Opcode_SHV");
}

void Compile_Opcode_SFV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SFV,"RSP_Opcode_SFV");
}

void Compile_Opcode_STV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_STV,"RSP_Opcode_STV");
}

void Compile_Opcode_SWV ( void ) {
	Cheat_r4300iOpcode(RSP_Opcode_SWV,"RSP_Opcode_SWV");
}

/************************** Other functions **************************/

void Compile_UnknownOpcode (void) {
	CPU_Message("  %X Unhandled Opcode: %s",CompilePC, RSPOpcodeName(RSPOpC.Hex,CompilePC) );
	NextInstruction = FINISH_BLOCK;
	MoveConstToVariable(CompilePC,PrgCount,"RSP PC");
	MoveConstToVariable(RSPOpC.Hex,&RSPOpC.Hex, "RSPOpC.Hex");
	Call_Direct(rsp_UnknownOpcode, "rsp_UnknownOpcode" );
	Ret();
}
