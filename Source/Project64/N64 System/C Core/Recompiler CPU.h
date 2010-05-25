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
#ifndef _RECOMPILER_CPU__H_
#define _RECOMPILER_CPU__H_


#define MaxCodeBlocks			50000
#define MaxOrigMem				65000

#ifdef toremove

#define NotMapped				0
#define GPR_Mapped				1
#define Temp_Mapped				2
#define Stack_Mapped			3

//Exit Block Methods
#define Normal					0
#define Normal_NoSysCheck		1
#define DoCPU_Action			2
#define COP1_Unuseable			3
#define DoSysCall				4
#define TLBReadMiss				5
#define ExitResetRecompCode		6


#define BranchTypeCop1			0
#define BranchTypeRs			1
#define BranchTypeRsRt			2

#define STATE_KNOWN_VALUE		1
//#define STATE_UNKNOW_VALUE

#define STATE_X86_MAPPED		2
//#define STATE_CONST

#define STATE_SIGN				4
//#define STATE_ZERO

#define STATE_32BIT				8
//#define STATE_64BIT

#define STATE_UNKNOWN			0

//STATE_MAPPED_64 = 3;
//STATE_MAPPED_32_ZERO = 11
//STATE_MAPPED_32_SIGN = 15
#define STATE_MAPPED_64			(STATE_KNOWN_VALUE | STATE_X86_MAPPED) 
#define STATE_MAPPED_32_ZERO	(STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT)
#define STATE_MAPPED_32_SIGN	(STATE_KNOWN_VALUE | STATE_X86_MAPPED | STATE_32BIT | STATE_SIGN)

//STATE_CONST_64 = 1
//STATE_CONST_32 = 13
#define STATE_CONST_64			(STATE_KNOWN_VALUE)
#define STATE_CONST_32			(STATE_KNOWN_VALUE | STATE_32BIT | STATE_SIGN)

#define IsKnown(Reg)			((MipsRegState(Reg) & STATE_KNOWN_VALUE) != 0)
#define IsUnknown(Reg)			(!IsKnown(Reg))

#define IsMapped(Reg)			(IsKnown(Reg) && (MipsRegState(Reg) & STATE_X86_MAPPED) != 0)
#define IsConst(Reg)			(IsKnown(Reg) && !IsMapped(Reg))

#define IsSigned(Reg)			(IsKnown(Reg) && (MipsRegState(Reg) & STATE_SIGN) != 0)
#define IsUnsigned(Reg)			(IsKnown(Reg) && !IsSigned(Reg))

#define Is32Bit(Reg)			(IsKnown(Reg) && (MipsRegState(Reg) & STATE_32BIT) != 0)
#define Is64Bit(Reg)			(IsKnown(Reg) && !Is32Bit(Reg))

#define Is32BitMapped(Reg)		(Is32Bit(Reg) && (MipsRegState(Reg) & STATE_X86_MAPPED) != 0)
#define Is64BitMapped(Reg)		(Is64Bit(Reg) && !Is32BitMapped(Reg))

/*#define MipsRegState(Reg)		Section->RegWorking.MIPS_RegState[Reg]
#define MipsReg(Reg)			Section->RegWorking.MIPS_RegVal[Reg].UDW
#define MipsReg_S(Reg)			Section->RegWorking.MIPS_RegVal[Reg].DW
#define MipsRegLo(Reg)			Section->RegWorking.MIPS_RegVal[Reg].UW[0]
#define MipsRegLo_S(Reg)		Section->RegWorking.MIPS_RegVal[Reg].W[0]
#define MipsRegHi(Reg)			Section->RegWorking.MIPS_RegVal[Reg].UW[1]
#define MipsRegHi_S(Reg)		Section->RegWorking.MIPS_RegVal[Reg].W[1]
*/
#define x86MapOrder(Reg)		Section->RegWorking.x86reg_MapOrder[Reg]
#define x86Protected(Reg)		Section->RegWorking.x86reg_Protected[Reg]
#define x86Mapped(Reg)			Section->RegWorking.x86reg_MappedTo[Reg]

#define BlockCycleCount			Section->RegWorking.CycleCount
#define BlockRandomModifier		Section->RegWorking.RandomModifier


#define StackTopPos				Section->RegWorking.Stack_TopPos
#define FpuMappedTo(Reg)		Section->RegWorking.x86fpu_MappedTo[Reg]
#define FpuState(Reg)			Section->RegWorking.x86fpu_State[Reg]
#define FpuRoundingModel(Reg)	Section->RegWorking.x86fpu_RoundingModel[Reg]
#define FpuBeenUsed				Section->RegWorking.Fpu_Used
#define CurrentRoundingModel	Section->RegWorking.RoundingModel

typedef struct {
	//r4k
	int		    MIPS_RegState[32];
	MIPS_DWORD	MIPS_RegVal[32];

	DWORD		x86reg_MappedTo[10];
	DWORD		x86reg_MapOrder[10];
	BOOL		x86reg_Protected[10];
	
	DWORD		CycleCount;
	DWORD		RandomModifier;

	//FPU
	DWORD		Stack_TopPos;
	DWORD		x86fpu_MappedTo[8];
	DWORD		x86fpu_State[8];
	DWORD		x86fpu_RoundingModel[8];
	
	BOOL        Fpu_Used;
	DWORD       RoundingModel;
} REG_INFO;

typedef struct {
	DWORD		TargetPC;
	char *		BranchLabel;
	BYTE *		LinkLocation;
	BYTE *		LinkLocation2;	
	BOOL		FallThrough;	
	BOOL		PermLoop;
	BOOL		DoneDelaySlot;
	REG_INFO	RegSet;
} JUMP_INFO;

typedef struct {
	/* Block Connection info */
	void **		ParentSection;
	void *		ContinueSection;
	void *		JumpSection;
	BYTE *		CompiledLocation;

	
	DWORD		SectionID;
	DWORD		Test;
	DWORD		Test2;
	BOOL		InLoop;
	
	DWORD		StartPC;
	DWORD		CompilePC;

	/* Register Info */
	REG_INFO	RegStart;
	REG_INFO	RegWorking;

	/* Jump Info */
	JUMP_INFO   Jump;
	JUMP_INFO   Cont;
} BLOCK_SECTION;

typedef struct {
	BLOCK_SECTION * Parent;
	JUMP_INFO     * JumpInfo;
} BLOCK_PARENT;

typedef struct {
	DWORD    TargetPC;
	REG_INFO ExitRegSet;
	int      reason;
	int      NextInstruction;
	BYTE *   JumpLoc; //32bit jump
} EXIT_INFO;

typedef struct {
	DWORD	 	  StartVAddr;
	BYTE *		  CompiledLocation;
	int           NoOfSections;
	BLOCK_SECTION BlockInfo;
	EXIT_INFO  ** ExitInfo;
	int           ExitCount;
} BLOCK_INFO;
#endif

typedef struct {
	void * CodeBlock;
	QWORD  OriginalMemory;
} TARGET_INFO;

typedef struct {
	DWORD PAddr; 
	DWORD VAddr; 
	DWORD OriginalValue; 
	void * CompiledLocation;
} ORIGINAL_MEMMARKER;

typedef struct {
	DWORD NoOfRDRamBlocks[2048]; 
	DWORD NoOfDMEMBlocks; 
	DWORD NoOfIMEMBlocks; 
	DWORD NoOfPifRomBlocks; 
} N64_BLOCKS;

#ifdef __cplusplus

//void CompileSystemCheck     ( DWORD TargetPC, CRegInfo RegSet );
//void FreeSection            ( CCodeSection * Section, CCodeSection * Parent);
void GenerateSectionLinkage ( CCodeSection * Section );

extern "C" {
#endif

BYTE *CompileDelaySlot      ( void );
void FixRandomReg           ( void );
void StartRecompilerCPU     ( void );
void InitilizeInitialCompilerVariable ( void);

extern DWORD TLBLoadAddress, TLBStoreAddress, TargetIndex;
extern ORIGINAL_MEMMARKER * OrigMem;
extern TARGET_INFO * TargetInfo;
extern WORD FPU_RoundingMode;
extern N64_BLOCKS N64_Blocks;

#ifdef __cplusplus
}
#endif

#define SetJump32(Loc,JumpLoc) *(DWORD *)(Loc)= (DWORD)(((DWORD)(JumpLoc)) - (((DWORD)(Loc)) + 4));
#define SetJump8(Loc,JumpLoc)  *(BYTE  *)(Loc)= (BYTE )(((BYTE )(JumpLoc)) - (((BYTE )(Loc)) + 1));

#endif