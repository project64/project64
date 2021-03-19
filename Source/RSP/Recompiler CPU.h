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
	DWORD StartPC, CurrPC;		// Block start
	
	struct {
		DWORD TargetPC;			// Target for this unknown branch
		DWORD * X86JumpLoc;		// Our x86 DWORD to fill
	} BranchesToResolve[200];	// Branches inside or outside block
	
	DWORD ResolveCount;			// Branches with NULL jump table
} RSP_BLOCK;

extern RSP_BLOCK CurrentBlock;

typedef struct {
    Boolean bIsRegConst[32];    // Boolean toggle for constant
	DWORD MipsRegConst[32];		// Value of register 32-bit
	DWORD BranchLabels[250];
	DWORD LabelCount;
	DWORD BranchLocations[250];
	DWORD BranchCount;
} RSP_CODE;

extern RSP_CODE RspCode;

#define IsRegConst(i)	(RspCode.bIsRegConst[i])
#define MipsRegConst(i) (RspCode.MipsRegConst[i])

typedef struct {
    Boolean mmx, mmx2, sse;     // CPU specs and compiling
    Boolean bFlags;             // RSP flag analysis
    Boolean bReOrdering;        // Instruction reordering
    Boolean bSections;          // Microcode sections
    Boolean bDest;              // Vector destination toggle
    Boolean bAccum;             // Accumulator toggle
    Boolean bGPRConstants;      // Analyze GPR constants
    Boolean bAlignVector;       // Align known vector loads
    Boolean bAudioUcode;        // Audio microcode analysis
} RSP_COMPILER;

extern RSP_COMPILER Compiler;

#define IsMmxEnabled	(Compiler.mmx)
#define IsMmx2Enabled	(Compiler.mmx2)
#define IsSseEnabled	(Compiler.sse)
