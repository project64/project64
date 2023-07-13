#include "cpu/RSPOpcode.h"
#include "cpu/RspTypes.h"

extern uint32_t CompilePC, NextInstruction, JumpTableSize;
extern bool ChangedPC;

#define CompilerWarning \
    if (ShowErrors) DisplayError

#define High16BitAccum 1
#define Middle16BitAccum 2
#define Low16BitAccum 4
#define EntireAccum (Low16BitAccum | Middle16BitAccum | High16BitAccum)

bool WriteToAccum(int Location, int PC);
bool WriteToVectorDest(DWORD DestReg, int PC);
bool UseRspFlags(int PC);

bool DelaySlotAffectBranch(DWORD PC);
bool CompareInstructions(DWORD PC, RSPOpcode * Top, RSPOpcode * Bottom);
bool IsOpcodeBranch(DWORD PC, RSPOpcode RspOp);
bool IsOpcodeNop(DWORD PC);

bool IsNextInstructionMmx(DWORD PC);
bool IsRegisterConstant(DWORD Reg, DWORD * Constant);

void RSP_Element2Mmx(int MmxReg);
void RSP_MultiElement2Mmx(int MmxReg1, int MmxReg2);

#define MainBuffer 0
#define SecondaryBuffer 1

DWORD RunRecompilerCPU(DWORD Cycles);
void BuildRecompilerCPU(void);

void CompilerRSPBlock(void);
void CompilerToggleBuffer(void);
bool RSP_DoSections(void);

typedef struct
{
    DWORD StartPC, CurrPC; // Block start

    struct
    {
        DWORD TargetPC;       // Target for this unknown branch
        DWORD * X86JumpLoc;   // Our x86 DWORD to fill
    } BranchesToResolve[200]; // Branches inside or outside block

    DWORD ResolveCount; // Branches with NULL jump table
} RSP_BLOCK;

extern RSP_BLOCK CurrentBlock;

typedef struct
{
    bool bIsRegConst[32]; // bool toggle for constant
    DWORD MipsRegConst[32];  // Value of register 32-bit
    DWORD BranchLabels[250];
    DWORD LabelCount;
    DWORD BranchLocations[250];
    DWORD BranchCount;
} RSP_CODE;

extern RSP_CODE RspCode;

#define IsRegConst(i) (RspCode.bIsRegConst[i])
#define MipsRegConst(i) (RspCode.MipsRegConst[i])

typedef struct
{
    bool mmx, mmx2, sse; // CPU specs and compiling
    bool bFlags;         // RSP flag analysis
    bool bReOrdering;    // Instruction reordering
    bool bSections;      // Microcode sections
    bool bDest;          // Vector destination toggle
    bool bAccum;         // Accumulator toggle
    bool bGPRConstants;  // Analyze GPR constants
    bool bAlignVector;   // Align known vector loads
    bool bAudioUcode;    // Audio microcode analysis
} RSP_COMPILER;

extern RSP_COMPILER Compiler;

#define IsMmxEnabled (Compiler.mmx)
#define IsMmx2Enabled (Compiler.mmx2)
#define IsSseEnabled (Compiler.sse)
