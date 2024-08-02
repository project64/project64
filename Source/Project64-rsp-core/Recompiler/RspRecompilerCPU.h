#pragma once

#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RSPOpcode.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <Settings/Settings.h>

extern uint32_t CompilePC, NextInstruction, JumpTableSize;
extern bool ChangedPC;

#define CompilerWarning \
    if (ShowErrors) g_Notify->DisplayError

#define High16BitAccum 1
#define Middle16BitAccum 2
#define Low16BitAccum 4
#define EntireAccum (Low16BitAccum | Middle16BitAccum | High16BitAccum)

bool WriteToAccum(int Location, int PC);
bool WriteToVectorDest(uint32_t DestReg, int PC);
bool UseRspFlags(int PC);

bool DelaySlotAffectBranch(uint32_t PC);
bool CompareInstructions(uint32_t PC, RSPOpcode * Top, RSPOpcode * Bottom);
bool IsOpcodeBranch(uint32_t PC, RSPOpcode RspOp);
bool IsOpcodeNop(uint32_t PC);

bool IsNextInstructionMmx(uint32_t PC);
bool IsRegisterConstant(uint32_t Reg, uint32_t * Constant);

void RSP_Element2Mmx(int MmxReg);
void RSP_MultiElement2Mmx(int MmxReg1, int MmxReg2);

#define MainBuffer 0
#define SecondaryBuffer 1

uint32_t RunRecompilerCPU(uint32_t Cycles);
void BuildRecompilerCPU(void);

void CompilerRSPBlock(void);
void CompilerToggleBuffer(void);
bool RSP_DoSections(void);

typedef struct
{
    uint32_t StartPC, CurrPC; // Block start

    struct
    {
        uint32_t TargetPC;     // Target for this unknown branch
        uint32_t * X86JumpLoc; // Our x86 uint32_t to fill
    } BranchesToResolve[200];  // Branches inside or outside block

    uint32_t ResolveCount; // Branches with NULL jump table
} RSP_BLOCK;

extern RSP_BLOCK CurrentBlock;

typedef struct
{
    bool bIsRegConst[32];      // bool toggle for constant
    uint32_t MipsRegConst[32]; // Value of register 32-bit
    uint32_t BranchLabels[250];
    uint32_t LabelCount;
    uint32_t BranchLocations[250];
    uint32_t BranchCount;
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