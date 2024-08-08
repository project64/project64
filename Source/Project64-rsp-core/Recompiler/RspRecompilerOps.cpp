#include "Project64-rsp-core/Recompiler/RspRecompilerCPU.h"
#include "RspProfiling.h"
#include "RspRecompilerCPU.h"
#include "X86.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/Recompiler/RspRecompilerOps.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPInstruction.h>
#include <Project64-rsp-core/cpu/RSPInterpreterCPU.h>
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Project64-rsp-core/cpu/RspTypes.h>

#pragma warning(disable : 4152) // Non-standard extension, function/data pointer conversion in expression

extern bool AudioHle, GraphicsHle;
UWORD32 Recp, RecpResult, SQroot, SQrootResult;
uint32_t ESP_RegSave = 0, EBP_RegSave = 0;
uint32_t BranchCompare = 0;

// Align option affects: SW, LH, SH
// Align option affects: LRV, SSV, LSV

#define Compile_Immediates // ADDI, ADDIU, ANDI, ORI, XORI, LUI
#define Compile_GPRLoads   // LB, LH, LW, LBU, LHU
#define Compile_GPRStores  // SB, SH, SW
#define Compile_Special    // SLL, SRL, SRA, SRLV \
                           // XOR, OR, AND, SUB, SUBU, ADDU, ADD, SLT
#define Compile_Cop0
#define Compile_Cop2

#define RSP_VectorMuls
#define RSP_VectorLoads
#define RSP_VectorMisc

#ifdef RSP_VectorMuls
//#define CompileVmulf
//#define CompileVmacf
#define CompileVmudm
//#define CompileVmudh
#define CompileVmudn
//#define CompileVmudl
//#define CompileVmadl
//#define CompileVmadm
#define CompileVmadh
#define CompileVmadn
#endif
#ifdef RSP_VectorMisc
//#define CompileVne
//#define CompileVeq
//#define CompileVge
//#define CompileVlt
//#define CompileVrcp
//#define CompileVrcpl
//#define CompileVrsqh
//#define CompileVrcph
//#define CompileVsaw
//#define CompileVabs
//#define CompileVmov
#define CompileVxor
#define CompileVor
#define CompileVand
#define CompileVsub
#define CompileVadd
#define CompileVaddc
#define CompileVsubc
//#define CompileVmrg
#define CompileVnxor
#define CompileVnor
#define CompileVnand
#endif
#ifdef RSP_VectorLoads
#define CompileLbv
//#define CompileLpv
//#define CompileLuv
//#define CompileLhv
#define CompileSqv
#define CompileSdv
#define CompileSsv
#define CompileLrv
#define CompileLqv
#define CompileLdv
#define CompileLsv
#define CompileLlv
#define CompileSlv
#endif

extern p_Recompfunc RSP_Recomp_Opcode[64];
extern p_Recompfunc RSP_Recomp_RegImm[32];
extern p_Recompfunc RSP_Recomp_Special[64];
extern p_Recompfunc RSP_Recomp_Cop0[32];
extern p_Recompfunc RSP_Recomp_Cop2[32];
extern p_Recompfunc RSP_Recomp_Vector[64];
extern p_Recompfunc RSP_Recomp_Lc2[32];
extern p_Recompfunc RSP_Recomp_Sc2[32];

void CRSPRecompilerOps::Cheat_r4300iOpcode(RSPOp::Func FunctAddress, const char * FunctName)
{
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    MoveConstToVariable(RSPOpC.Value, &RSPOpC.Value, "RSPOpC.Value");
    MoveConstToX86reg((uint32_t) & (RSPSystem.m_OpCodes), x86_ECX);
    Call_Direct(AddressOf(FunctAddress), FunctName);
}

void CRSPRecompilerOps::Cheat_r4300iOpcodeNoMessage(RSPOp::Func FunctAddress, const char * FunctName)
{
    MoveConstToVariable(RSPOpC.Value, &RSPOpC.Value, "RSPOpC.Value");
    MoveConstToX86reg((uint32_t) & (RSPSystem.m_OpCodes), x86_ECX);
    Call_Direct(AddressOf(FunctAddress), FunctName);
}

void x86_SetBranch8b(void * JumpByte, void * Destination)
{
    // Calculate 32-bit relative offset
    size_t n = (uint8_t *)Destination - ((uint8_t *)JumpByte + 1);
    intptr_t signed_n = (intptr_t)n;

    // Check limits, no pun intended
    if (signed_n > +128 || signed_n < -127)
    {
        CompilerWarning(stdstr_f("FATAL: Jump out of 8b range %i (PC = %04X)", n, CompilePC).c_str());
    }
    else
    {
        *(uint8_t *)(JumpByte) = (uint8_t)(n & 0xFF);
    }
}

void x86_SetBranch32b(void * JumpByte, void * Destination)
{
    *(uint32_t *)(JumpByte) = (uint32_t)((uint8_t *)Destination - (uint8_t *)((uint32_t *)JumpByte + 1));
}

void BreakPoint()
{
    CPU_Message("      int 3");
    *(RecompPos++) = 0xCC;
}

void CompileBranchExit(uint32_t TargetPC, uint32_t ContinuePC)
{
    uint32_t * X86Loc = NULL;

    NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    CompConstToVariable(true, &BranchCompare, "BranchCompare");
    JeLabel32("BranchEqual", 0);
    X86Loc = (uint32_t *)(RecompPos - 4);
    MoveConstToVariable(ContinuePC, PrgCount, "RSP PC");
    Ret();

    CPU_Message("BranchEqual:");
    x86_SetBranch32b(X86Loc, RecompPos);
    MoveConstToVariable(TargetPC, PrgCount, "RSP PC");
    Ret();
}

CRSPRecompilerOps::CRSPRecompilerOps(CRSPSystem & System, CRSPRecompiler & Recompiler) :
    m_System(System),
    m_Recompiler(Recompiler),
    m_Reg(System.m_Reg),
    m_GPR(System.m_Reg.m_GPR),
    m_ACCUM(System.m_Reg.m_ACCUM),
    m_Flags(System.m_Reg.m_Flags),
    m_Vect(System.m_Reg.m_Vect)
{
}

// Opcode functions

void CRSPRecompilerOps::SPECIAL(void)
{
    (this->*RSP_Recomp_Special[RSPOpC.funct])();
}

void CRSPRecompilerOps::REGIMM(void)
{
    (this->*RSP_Recomp_RegImm[RSPOpC.rt])();
}

void CRSPRecompilerOps::J(void)
{
    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        JmpLabel32("BranchToJump", 0);
        m_Recompiler.Branch_AddRef((RSPOpC.target << 2) & 0xFFC, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        MoveConstToVariable((RSPOpC.target << 2) & 0xFFC, PrgCount, "RSP PC");
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
        Ret();
    }
    else
    {
        CompilerWarning(stdstr_f("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::JAL(void)
{
    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        MoveConstToVariable((CompilePC + 8) & 0xFFC, &m_GPR[31].UW, "RA.W");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        // Before we branch quickly update our stats
        if (Profiling && IndvidualBlock)
        {
            char Str[40];
            sprintf(Str, "%03X", (RSPOpC.target << 2) & 0xFFC);
            Push(x86_EAX);
            PushImm32(Str, *PrgCount);
            Call_Direct((void *)StartTimer, "StartTimer");
            AddConstToX86Reg(x86_ESP, 4);
            Pop(x86_EAX);
        }
        JmpLabel32("BranchToJump", 0);
        m_Recompiler.Branch_AddRef((RSPOpC.target << 2) & 0xFFC, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        MoveConstToVariable((RSPOpC.target << 2) & 0xFFC, PrgCount, "RSP PC");
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
        Ret();
    }
    else
    {
        CompilerWarning(stdstr_f("J error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::BEQ(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0 && RSPOpC.rt == 0)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            MoveConstByteToVariable(1, &BranchCompare, "BranchCompare");
            return;
        }
        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        if (RSPOpC.rt == 0)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        }
        else if (RSPOpC.rs == 0)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        }
        else
        {
            MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
            CompX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        }
        SetzVariable(&BranchCompare, "BranchCompare");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0 && RSPOpC.rt == 0)
        {
            JmpLabel32("BranchToJump", 0);
            m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }
        if (!bDelayAffect)
        {
            if (RSPOpC.rt == 0)
            {
                CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            }
            else if (RSPOpC.rs == 0)
            {
                CompConstToVariable(0, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
            }
            else
            {
                MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
                CompX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            }
            JeLabel32("BranchEqual", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchEqual", 0);
        }
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BEQ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::BNE(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0 && RSPOpC.rt == 0)
        {
            MoveConstByteToVariable(0, &BranchCompare, "BranchCompare");
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }

        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        if (RSPOpC.rt == 0)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        }
        else if (RSPOpC.rs == 0)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        }
        else
        {
            MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
            CompX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        }
        SetnzVariable(&BranchCompare, "BranchCompare");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0 && RSPOpC.rt == 0)
        {
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }

        if (!bDelayAffect)
        {
            if (RSPOpC.rt == 0)
            {
                CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            }
            else if (RSPOpC.rs == 0)
            {
                CompConstToVariable(0, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
            }
            else
            {
                MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
                CompX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            }
            JneLabel32("BranchNotEqual", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchNotEqual", 0);
        }
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BNE error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::BLEZ(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        SetleVariable(&BranchCompare, "BranchCompare");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0)
        {
            JmpLabel32("BranchToJump", 0);
            m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }
        if (!bDelayAffect)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            JleLabel32("BranchLessEqual", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchLessEqual", 0);
        }

        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BLEZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::BGTZ(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        SetgVariable(&BranchCompare, "BranchCompare");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }
        if (!bDelayAffect)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            JgLabel32("BranchGreater", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchGreater", 0);
        }
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BGTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::ADDI(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::ADDI, "RSPOp::ADDI");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Immediate = (short)RSPOpC.immediate;
    if (RSPOpC.rt == RSPOpC.rs)
    {
        if (Immediate != 0)
        {
            AddConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        }
    }
    else if (RSPOpC.rs == 0)
    {
        MoveConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else if ((IsRegConst(RSPOpC.rs) & 1) != 0)
    {
        MoveConstToVariable(MipsRegConst(RSPOpC.rs) + Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
        if (Immediate != 0)
        {
            AddConstToX86Reg(x86_EAX, Immediate);
        }
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
#endif
}

void CRSPRecompilerOps::ADDIU(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::ADDIU, "RSPOp::ADDIU");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Immediate = (short)RSPOpC.immediate;

    if (RSPOpC.rt == RSPOpC.rs)
    {
        if (Immediate != 0)
        {
            AddConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        }
    }
    else if (RSPOpC.rs == 0)
    {
        MoveConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
        if (Immediate != 0)
        {
            AddConstToX86Reg(x86_EAX, Immediate);
        }
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
#endif
}

void CRSPRecompilerOps::SLTI(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::SLTI, "&RSPOp::SLTI");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Immediate = (short)RSPOpC.immediate;
    if (Immediate == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_ECX);
        ShiftRightUnsignImmed(x86_ECX, 31);
    }
    else
    {
        XorX86RegToX86Reg(x86_ECX, x86_ECX);
        CompConstToVariable(Immediate, &m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs));
        Setl(x86_ECX);
    }
    MoveX86regToVariable(x86_ECX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
#endif
}

void CRSPRecompilerOps::SLTIU(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::SLTIU, "RSPOp::SLTIU");
#else
    int Immediate;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    Immediate = (short)RSPOpC.immediate;
    XorX86RegToX86Reg(x86_ECX, x86_ECX);
    CompConstToVariable(Immediate, &m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs));
    Setb(x86_ECX);
    MoveX86regToVariable(x86_ECX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
#endif
}

void CRSPRecompilerOps::ANDI(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::ANDI, "RSPOp::ANDI");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Immediate = (unsigned short)RSPOpC.immediate;
    if (RSPOpC.rt == RSPOpC.rs)
    {
        AndConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else if (RSPOpC.rs == 0)
    {
        MoveConstToVariable(0, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else if (Immediate == 0xFFFF)
    {
        MoveZxVariableToX86regHalf(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
        AndConstToX86Reg(x86_EAX, Immediate);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
#endif
}

void CRSPRecompilerOps::ORI(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::ORI, "RSPOp::ORI");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Immediate = (unsigned short)RSPOpC.immediate;
    if (RSPOpC.rt == RSPOpC.rs)
    {
        OrConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else if (RSPOpC.rs == 0)
    {
        MoveConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
        if (Immediate != 0)
        {
            OrConstToX86Reg(Immediate, x86_EAX);
        }
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
#endif
}

void CRSPRecompilerOps::XORI(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::XORI, "RSPOp::XORI");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Immediate = (unsigned short)RSPOpC.immediate;
    if (RSPOpC.rt == RSPOpC.rs)
    {
        XorConstToVariable(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), Immediate);
    }
    else if (RSPOpC.rs == 0)
    {
        MoveConstToVariable(Immediate, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
        if (Immediate != 0)
        {
            XorConstToX86Reg(x86_EAX, Immediate);
        }
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
    }
#endif
}

void CRSPRecompilerOps::LUI(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Immediates
    Cheat_r4300iOpcode(&RSPOp::LUI, "RSPOp::LUI");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int constant = (short)RSPOpC.offset << 16;
    MoveConstToVariable(constant, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
#endif
}

void CRSPRecompilerOps::COP0(void)
{
    (this->*RSP_Recomp_Cop0[RSPOpC.rs])();
}

void CRSPRecompilerOps::COP2(void)
{
    (this->*RSP_Recomp_Cop2[RSPOpC.rs])();
}

void CRSPRecompilerOps::LB(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_GPRLoads
    Cheat_r4300iOpcode(&RSPOp::LB, "RSPOp::LB");
#else
    int Offset = (short)RSPOpC.offset;
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (IsRegConst(RSPOpC.base))
    {
        char Address[32];
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 3;
        Addr &= 0xfff;

        sprintf(Address, "Dmem + %Xh", Addr);
        MoveSxVariableToX86regByte(RSPInfo.DMEM + Addr, Address, x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
    XorConstToX86Reg(x86_EBX, 3);
    AndConstToX86Reg(x86_EBX, 0x0fff);

    MoveSxN64MemToX86regByte(x86_EAX, x86_EBX);
    MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
#endif
}

void CRSPRecompilerOps::LH(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_GPRLoads
    Cheat_r4300iOpcode(&RSPOp::LH, "RSPOp::LH");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    uint8_t * Jump[2];
    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
        Addr &= 0xfff;

        if ((Addr & 1) != 0)
        {
            if ((Addr & 2) == 0)
            {
                CompilerWarning(stdstr_f("Unaligned LH at constant address PC = %04X", CompilePC).c_str());
                Cheat_r4300iOpcodeNoMessage(&RSPOp::LH, "RSPOp::LH");
            }
            else
            {
                char Address[32];
                sprintf(Address, "DMEM + %Xh", Addr);
                MoveSxVariableToX86regHalf(RSPInfo.DMEM + (Addr ^ 2), Address, x86_EAX);
                MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
            }
        }
        else
        {
            char Address[32];
            sprintf(Address, "DMEM + %Xh", Addr);
            MoveSxVariableToX86regHalf(RSPInfo.DMEM + Addr, Address, x86_EAX);
            MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        }
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);

    AndConstToX86Reg(x86_EBX, 0x0fff);
    TestConstToX86Reg(1, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();

    CPU_Message("   Unaligned:");
    x86_SetBranch32b(Jump[0], RecompPos);

    Cheat_r4300iOpcodeNoMessage(&RSPOp::LH, "RSPOp::LH");

    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;

    CompilerToggleBuffer();

    XorConstToX86Reg(x86_EBX, 2);

    MoveSxN64MemToX86regHalf(x86_EAX, x86_EBX);
    MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

    CPU_Message("   Done:");
    x86_SetBranch32b(Jump[1], RecompPos);
#endif
}

void CRSPRecompilerOps::LW(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_GPRLoads
    Cheat_r4300iOpcode(&RSPOp::LW, "RSPOp::LW");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    uint8_t * Jump[2];
    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) & 0xfff;

        if ((Addr & 1) != 0)
        {
            CompilerWarning(stdstr_f("Unaligned LW at constant address PC = %04X", CompilePC).c_str());
            Cheat_r4300iOpcodeNoMessage(&RSPOp::LW, "RSPOp::LW");
        }
        else if ((Addr & 2) != 0)
        {
            char Address[32];
            sprintf(Address, "DMEM + %Xh", Addr - 2);
            MoveVariableToX86regHalf(RSPInfo.DMEM + ((Addr - 2) & 0xFFF), Address, x86_EAX);
            sprintf(Address, "DMEM + %Xh", Addr);
            MoveVariableToX86regHalf(RSPInfo.DMEM + ((Addr + 4) & 0xFFF), Address, x86_ECX);

            MoveX86regHalfToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UHW[1], GPR_Name(RSPOpC.rt));
            MoveX86regHalfToVariable(x86_ECX, &m_GPR[RSPOpC.rt].UHW[0], GPR_Name(RSPOpC.rt));
        }
        else
        {
            char Address[32];
            sprintf(Address, "DMEM + %Xh", Addr);
            MoveVariableToX86reg(RSPInfo.DMEM + Addr, Address, x86_EAX);
            MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        }
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (Offset != 0)
    {
        AddConstToX86Reg(x86_EBX, Offset);
    }
    AndConstToX86Reg(x86_EBX, 0x0fff);
    TestConstToX86Reg(3, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();

    x86_SetBranch32b(Jump[0], RecompPos);
    CPU_Message("   Unaligned:");

    LeaSourceAndOffset(x86_ECX, x86_EBX, 2);
    LeaSourceAndOffset(x86_EDX, x86_EBX, 3);
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
    MoveX86regByteToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UB[3], GPR_Name(RSPOpC.rt));
    MoveX86regByteToVariable(x86_EBX, &m_GPR[RSPOpC.rt].UB[2], GPR_Name(RSPOpC.rt));
    MoveX86regByteToVariable(x86_ECX, &m_GPR[RSPOpC.rt].UB[1], GPR_Name(RSPOpC.rt));
    MoveX86regByteToVariable(x86_EDX, &m_GPR[RSPOpC.rt].UB[0], GPR_Name(RSPOpC.rt));

    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;
    CompilerToggleBuffer();

    MoveN64MemToX86reg(x86_EAX, x86_EBX);
    MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

    CPU_Message("   Done:");
    x86_SetBranch32b(Jump[1], RecompPos);
#endif
}

void CRSPRecompilerOps::LBU(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_GPRLoads
    Cheat_r4300iOpcode(&RSPOp::LBU, "RSPOp::LBU");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    if (IsRegConst(RSPOpC.base))
    {
        char Address[32];
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 3;
        Addr &= 0xfff;

        sprintf(Address, "DMEM + %Xh", Addr);
        MoveZxVariableToX86regByte(RSPInfo.DMEM + Addr, Address, x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    XorX86RegToX86Reg(x86_EAX, x86_EAX);

    if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
    XorConstToX86Reg(x86_EBX, 3);
    AndConstToX86Reg(x86_EBX, 0x0fff);

    MoveN64MemToX86regByte(x86_EAX, x86_EBX);
    MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
#endif
}

void CRSPRecompilerOps::LHU(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_GPRLoads
    Cheat_r4300iOpcode(&RSPOp::LHU, "RSPOp::LHU");
#else

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    uint8_t * Jump[2];
    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
        Addr &= 0xfff;

        if ((Addr & 1) != 0)
        {
            if ((Addr & 2) == 0)
            {
                CompilerWarning(stdstr_f("Unaligned LHU at constant address PC = %04X", CompilePC).c_str());
                Cheat_r4300iOpcodeNoMessage(&RSPOp::LHU, "RSPOp::LHU");
            }
            else
            {
                char Address[32];
                sprintf(Address, "DMEM + %Xh", Addr);
                MoveZxVariableToX86regHalf(RSPInfo.DMEM + (Addr ^ 2), Address, x86_ECX);
                MoveX86regToVariable(x86_ECX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
            }
            return;
        }
        else
        {
            char Address[32];
            sprintf(Address, "DMEM + %Xh", Addr);
            MoveZxVariableToX86regHalf(RSPInfo.DMEM + Addr, Address, x86_ECX);
            MoveX86regToVariable(x86_ECX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
            return;
        }
    }

    // TODO: Should really just do it by bytes but whatever for now

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (Offset != 0)
    {
        AddConstToX86Reg(x86_EBX, Offset);
    }
    TestConstToX86Reg(1, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();
    CPU_Message("   Unaligned:");
    x86_SetBranch32b(Jump[0], RecompPos);
    Cheat_r4300iOpcodeNoMessage(&RSPOp::LHU, "RSPOp::LHU");
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;
    CompilerToggleBuffer();

    XorConstToX86Reg(x86_EBX, 2);
    AndConstToX86Reg(x86_EBX, 0x0fff);
    MoveZxN64MemToX86regHalf(x86_EAX, x86_EBX);
    MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));

    CPU_Message("   Done:");
    x86_SetBranch32b(Jump[1], RecompPos);
#endif
}

void CRSPRecompilerOps::LWU(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
    Cheat_r4300iOpcode(&RSPOp::LWU, "RSPOp::LWU");
}

void CRSPRecompilerOps::SB(void)
{
#ifndef Compile_GPRStores
    Cheat_r4300iOpcode(&RSPOp::SB, "RSPOp::SB");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    if (IsRegConst(RSPOpC.base))
    {
        char Address[32];
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 3;
        Addr &= 0xfff;
        sprintf(Address, "DMEM + %Xh", Addr);

        if (IsRegConst(RSPOpC.rt))
        {
            MoveConstByteToVariable((uint8_t)MipsRegConst(RSPOpC.rt), RSPInfo.DMEM + Addr, Address);
            return;
        }
        else
        {
            MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
            MoveX86regByteToVariable(x86_EAX, RSPInfo.DMEM + Addr, Address);
            return;
        }
    }

    if (IsRegConst(RSPOpC.rt))
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);

        if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
        XorConstToX86Reg(x86_EBX, 3);
        AndConstToX86Reg(x86_EBX, 0x0fff);

        MoveConstByteToN64Mem((uint8_t)MipsRegConst(RSPOpC.rt), x86_EBX);
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
        MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);

        if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);
        XorConstToX86Reg(x86_EBX, 3);
        AndConstToX86Reg(x86_EBX, 0x0fff);

        MoveX86regByteToN64Mem(x86_EAX, x86_EBX);
    }
#endif
}

void CRSPRecompilerOps::SH(void)
{
#ifndef Compile_GPRStores
    Cheat_r4300iOpcode(&RSPOp::SH, "&RSPOp::SH");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    uint8_t * Jump[2];
    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) ^ 2;
        Addr &= 0xfff;

        if ((Offset & 1) != 0)
        {
            CompilerWarning(stdstr_f("Unaligned SH at constant address PC = %04X", CompilePC).c_str());
            Cheat_r4300iOpcodeNoMessage(&RSPOp::SH, "RSPOp::SH");
            return;
        }
        else
        {
            char Address[32];
            sprintf(Address, "DMEM + %Xh", Addr);
            if (IsRegConst(RSPOpC.rt))
            {
                MoveConstHalfToVariable((uint16_t)MipsRegConst(RSPOpC.rt), RSPInfo.DMEM + Addr, Address);
            }
            else
            {
                MoveVariableToX86regHalf(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
                MoveX86regHalfToVariable(x86_EAX, RSPInfo.DMEM + Addr, Address);
            }
            return;
        }
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (Offset != 0) AddConstToX86Reg(x86_EBX, Offset);

    TestConstToX86Reg(1, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();

    CPU_Message("   Unaligned:");
    x86_SetBranch32b(Jump[0], RecompPos);

    X86BreakPoint(__FILE__, __LINE__);
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;

    CompilerToggleBuffer();

    XorConstToX86Reg(x86_EBX, 2);
    AndConstToX86Reg(x86_EBX, 0x0fff);

    if (IsRegConst(RSPOpC.rt))
    {
        MoveConstHalfToN64Mem((uint16_t)MipsRegConst(RSPOpC.rt), x86_EBX);
    }
    else
    {
        MoveVariableToX86regHalf(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regHalfToN64Mem(x86_EAX, x86_EBX);
    }

    CPU_Message("   Done:");
    x86_SetBranch32b(Jump[1], RecompPos);
#endif
}

void CRSPRecompilerOps::SW(void)
{
#ifndef Compile_GPRStores
    Cheat_r4300iOpcode(&RSPOp::SW, "&RSPOp::SW");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    int Offset = (short)RSPOpC.offset;
    uint8_t * Jump[2];
    if (IsRegConst(RSPOpC.base))
    {
        char Address[32];
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + Offset) & 0xfff;

        if ((Addr & 3) != 0)
        {
            if (IsRegConst(RSPOpC.rt))
            {
                if (Addr > 0xFFC)
                {
                    g_Notify->DisplayError("There is a problem with:\nRSP_SW_DMEM");
                    return;
                }
                uint32_t Value = MipsRegConst(RSPOpC.rt);
                sprintf(Address, "DMEM + %Xh", ((Addr + 0) ^ 3) & 0xFFF);
                MoveConstByteToVariable((Value >> 24) & 0xFF, RSPInfo.DMEM + (((Addr + 0) ^ 3) & 0xFFF), Address);
                sprintf(Address, "DMEM + %Xh", ((Addr + 1) ^ 3) & 0xFFF);
                MoveConstByteToVariable((Value >> 16) & 0xFF, RSPInfo.DMEM + (((Addr + 1) ^ 3) & 0xFFF), Address);
                sprintf(Address, "DMEM + %Xh", ((Addr + 2) ^ 3) & 0xFFF);
                MoveConstByteToVariable((Value >> 8) & 0xFF, RSPInfo.DMEM + (((Addr + 2) ^ 3) & 0xFFF), Address);
                sprintf(Address, "DMEM + %Xh", ((Addr + 3) ^ 3) & 0xFFF);
                MoveConstByteToVariable((Value >> 0) & 0xFF, RSPInfo.DMEM + (((Addr + 3) ^ 3) & 0xFFF), Address);
            }
            else
            {
                CompilerWarning(stdstr_f("Unaligned SW at constant address PC = %04X", CompilePC).c_str());
                Cheat_r4300iOpcodeNoMessage(&RSPOp::SW, "RSPOp::SW");
            }
            return;
        }
        else
        {
            sprintf(Address, "DMEM + %Xh", Addr);

            if (IsRegConst(RSPOpC.rt))
            {
                MoveConstToVariable(MipsRegConst(RSPOpC.rt), RSPInfo.DMEM + Addr, Address);
            }
            else
            {
                MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
                MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr, Address);
            }
            return;
        }
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
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
    LeaSourceAndOffset(x86_ECX, x86_EBX, 2);
    LeaSourceAndOffset(x86_EDX, x86_EBX, 3);
    XorConstToX86Reg(x86_ECX, 3);
    XorConstToX86Reg(x86_EDX, 3);
    MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].UB[1], GPR_Name(RSPOpC.rt), x86_EAX); // CX
    MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].UB[0], GPR_Name(RSPOpC.rt), x86_EBX); // DX
    MoveX86regByteToN64Mem(x86_EAX, x86_ECX);
    MoveX86regByteToN64Mem(x86_EBX, x86_EDX);
    Pop(x86_EBX);

    MoveX86RegToX86Reg(x86_EBX, x86_EAX);
    AddConstToX86Reg(x86_EBX, 1);
    XorConstToX86Reg(x86_EAX, 3);
    XorConstToX86Reg(x86_EBX, 3);

    MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].UB[3], GPR_Name(RSPOpC.rt), x86_ECX); // AX
    MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].UB[2], GPR_Name(RSPOpC.rt), x86_EDX); // BX

    MoveX86regByteToN64Mem(x86_ECX, x86_EAX);
    MoveX86regByteToN64Mem(x86_EDX, x86_EBX);

    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;

    CompilerToggleBuffer();

    if (IsRegConst(RSPOpC.rt))
    {
        MoveConstToN64Mem(MipsRegConst(RSPOpC.rt), x86_EBX);
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToN64Mem(x86_EAX, x86_EBX);
    }

    CPU_Message("   Done:");
    x86_SetBranch32b(Jump[1], RecompPos);
#endif
}

void CRSPRecompilerOps::LC2(void)
{
    (this->*RSP_Recomp_Lc2[RSPOpC.rd])();
}

void CRSPRecompilerOps::SC2(void)
{
    (this->*RSP_Recomp_Sc2[RSPOpC.rd])();
}

// R4300i Opcodes: Special

void CRSPRecompilerOps::Special_SLL(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SLL, "RSPOp::Special_SLL");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rt)
    {
        ShiftLeftSignVariableImmed(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (uint8_t)RSPOpC.sa);
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        ShiftLeftSignImmed(x86_EAX, (uint8_t)RSPOpC.sa);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_SRL(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SRL, "RSPOp::Special_SRL");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rt)
    {
        ShiftRightUnsignVariableImmed(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (uint8_t)RSPOpC.sa);
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        ShiftRightUnsignImmed(x86_EAX, (uint8_t)RSPOpC.sa);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_SRA(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SRA, "RSPOp::Special_SRA");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rt)
    {
        ShiftRightSignVariableImmed(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), (uint8_t)RSPOpC.sa);
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        ShiftRightSignImmed(x86_EAX, (uint8_t)RSPOpC.sa);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_SLLV(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
    Cheat_r4300iOpcode(&RSPOp::Special_SLLV, "RSPOp::Special_SLLV");
}

void CRSPRecompilerOps::Special_SRLV(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SRLV, "RSPOp::Special_SRLV");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
    MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_ECX);
    AndConstToX86Reg(x86_ECX, 0x1F);
    ShiftRightUnsign(x86_EAX);
    MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
#endif
}

void CRSPRecompilerOps::Special_SRAV(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
    Cheat_r4300iOpcode(&RSPOp::Special_SRAV, "RSPOp::Special_SRAV");
}

void UpdateAudioTimer()
{
    /*	char Label[100];
	sprintf(Label,"COMMAND: %02X (PC = %08X)",m_GPR[1].UW >> 1, *PrgCount);
	StartTimer(Label);*/
}

void CRSPRecompilerOps::Special_JR(void)
{
    uint8_t * Jump;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        // Transfer destination to location pointed to by PrgCount
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AndConstToX86Reg(x86_EAX, 0xFFC);
        MoveX86regToVariable(x86_EAX, PrgCount, "RSP PC");
        ChangedPC = true;
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        MoveVariableToX86reg(PrgCount, "RSP PC", x86_EAX);
        if (Profiling && IndvidualBlock)
        {
            Push(x86_EAX);
            Push(x86_EAX);
            Call_Direct((void *)StartTimer, "StartTimer");
            AddConstToX86Reg(x86_ESP, 4);
            Pop(x86_EAX);
        }
        AddVariableToX86reg(x86_EAX, &JumpTable, "JumpTable");
        MoveX86regPointerToX86reg(x86_EAX, x86_EAX);

        TestX86RegToX86Reg(x86_EAX, x86_EAX);
        JeLabel8("Null", 0);
        Jump = RecompPos - 1;

        // Before we branch quickly update our stats
        /*if (CompilePC == 0x080) {
			Pushad();
			Call_Direct((void *)UpdateAudioTimer, "UpdateAudioTimer");
			Popad();
		}*/
        JumpX86Reg(x86_EAX);

        x86_SetBranch8b(Jump, RecompPos);
        CPU_Message(" Null:");
        Ret();
        ChangedPC = false;
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
        Ret();
    }
    else
    {
        CompilerWarning(stdstr_f("WTF\n\nJR\nNextInstruction = %X", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::Special_JALR(void)
{
    uint8_t * Jump;
    uint32_t Const = (CompilePC + 8) & 0xFFC;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AndConstToX86Reg(x86_EAX, 0xFFC);
        MoveX86regToVariable(x86_EAX, PrgCount, "RSP PC");
        MoveConstToVariable(Const, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        MoveVariableToX86reg(PrgCount, "RSP PC", x86_EAX);
        AddVariableToX86reg(x86_EAX, &JumpTable, "JumpTable");
        MoveX86regPointerToX86reg(x86_EAX, x86_EAX);

        TestX86RegToX86Reg(x86_EAX, x86_EAX);
        JeLabel8("Null", 0);
        Jump = RecompPos - 1;
        JumpX86Reg(x86_EAX);

        x86_SetBranch8b(Jump, RecompPos);
        CPU_Message(" Null:");
        Ret();
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
        Ret();
    }
    else
    {
        CompilerWarning(stdstr_f("WTF\n\nJALR\nNextInstruction = %X", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::Special_BREAK(void)
{
    Cheat_r4300iOpcode(&RSPOp::Special_BREAK, "RSPOp::Special_BREAK");
    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
        Ret();
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT)
    {
        NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT;
    }
    else
    {
        CompilerWarning(stdstr_f("WTF\n\nBREAK\nNextInstruction = %X", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::Special_ADD(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_ADD, "RSPOp::Special_ADD");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        AddX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rd == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AddX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rs == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AddX86RegToX86Reg(x86_EAX, x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rs == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rt == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AddVariableToX86reg(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_ADDU(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_ADDU, "RSPOp::Special_ADDU");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        AddX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rd == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AddX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rs == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AddX86RegToX86Reg(x86_EAX, x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rs == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rt == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AddVariableToX86reg(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_SUB(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SUB, "RSPOp::Special_SUB");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        SubX86regFromVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, "m_GPR[RSPOpC.rd].W");
    }
    else if (RSPOpC.rs == RSPOpC.rt)
    {
        MoveConstToVariable(0, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        SubVariableFromX86reg(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_SUBU(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SUBU, "RSPOp::Special_SUBU");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        SubX86regFromVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rs == RSPOpC.rt)
    {
        MoveConstToVariable(0, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        SubVariableFromX86reg(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_AND(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_AND, "RSPOp::Special_AND");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        AndX86RegToVariable(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
    }
    else if (RSPOpC.rd == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AndX86RegToVariable(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
    }
    else if (RSPOpC.rs == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        AndVariableToX86Reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_OR(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_Special
    Cheat_r4300iOpcode(RSPOp::Special_OR, "RSPOp::Special_OR");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        OrX86RegToVariable(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
    }
    else if (RSPOpC.rd == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        OrX86RegToVariable(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
    }
    else if (RSPOpC.rs == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else if (RSPOpC.rt == 0)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        OrVariableToX86Reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_XOR(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_XOR, "RSPOp::Special_XOR");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rd == RSPOpC.rs)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        XorX86RegToVariable(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
    }
    else if (RSPOpC.rd == RSPOpC.rt)
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        XorX86RegToVariable(&m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd), x86_EAX);
    }
    else if (RSPOpC.rs == RSPOpC.rt)
    {
        MoveConstToVariable(0, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
    else
    {
        MoveVariableToX86reg(&m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs), x86_EAX);
        XorVariableToX86reg(&m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rd].W, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_NOR(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
    Cheat_r4300iOpcode(&RSPOp::Special_NOR, "RSPOp::Special_NOR");
}

void CRSPRecompilerOps::Special_SLT(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
#ifndef Compile_Special
    Cheat_r4300iOpcode(&RSPOp::Special_SLT, "RSPOp::Special_SLT");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rt == RSPOpC.rs)
    {
        MoveConstToVariable(0, &m_GPR[RSPOpC.rd].UW, GPR_Name(RSPOpC.rd));
    }
    else
    {
        if (RSPOpC.rs == 0)
        {
            MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
            XorX86RegToX86Reg(x86_ECX, x86_ECX);
            CompConstToX86reg(x86_EAX, 0);
            Setg(x86_ECX);
        }
        else if (RSPOpC.rt == 0)
        {
            MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_ECX);
            ShiftRightUnsignImmed(x86_ECX, 31);
        }
        else
        {
            MoveVariableToX86reg(&m_GPR[RSPOpC.rs].UW, GPR_Name(RSPOpC.rs), x86_EAX);
            XorX86RegToX86Reg(x86_ECX, x86_ECX);
            CompX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
            Setl(x86_ECX);
        }
        MoveX86regToVariable(x86_ECX, &m_GPR[RSPOpC.rd].UW, GPR_Name(RSPOpC.rd));
    }
#endif
}

void CRSPRecompilerOps::Special_SLTU(void)
{
    if (RSPOpC.rd == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }
    Cheat_r4300iOpcode(&RSPOp::Special_SLTU, "RSPOp::Special_SLTU");
}

// R4300i Opcodes: RegImm
void CRSPRecompilerOps::RegImm_BLTZ(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        SetlVariable(&BranchCompare, "BranchCompare");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }
        if (!bDelayAffect)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            JlLabel32("BranchLess", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchLess", 0);
        }
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BLTZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nPC = %X\nEmulation will now stop", NextInstruction, CompilePC).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::RegImm_BGEZ(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        SetgeVariable(&BranchCompare, "BranchCompare");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0)
        {
            JmpLabel32("BranchToJump", 0);
            m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }
        if (!bDelayAffect)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            JgeLabel32("BranchGreaterEqual", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchGreaterEqual", 0);
        }
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BGEZ error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::RegImm_BLTZAL(void)
{
    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0)
        {
            MoveConstToVariable((CompilePC + 8) & 0xFFC, &m_GPR[31].UW, "RA.W");
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        SetlVariable(&BranchCompare, "BranchCompare");
        MoveConstToVariable((CompilePC + 8) & 0xFFC, &m_GPR[31].UW, "RA.W");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0)
        {
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }

        // Take a look at the branch compare variable
        CompConstToVariable(true, &BranchCompare, "BranchCompare");
        JeLabel32("BranchLessEqual", 0);
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BLTZAL error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

void CRSPRecompilerOps::RegImm_BGEZAL(void)
{
    static bool bDelayAffect;

    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        if (RSPOpC.rs == 0)
        {
            MoveConstToVariable((CompilePC + 8) & 0xFFC, &m_GPR[31].UW, "RA.W");
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        bDelayAffect = DelaySlotAffectBranch(CompilePC);
        if (!bDelayAffect)
        {
            NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
            return;
        }
        CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
        SetgeVariable(&BranchCompare, "BranchCompare");
        MoveConstToVariable((CompilePC + 8) & 0xFFC, &m_GPR[31].UW, "RA.W");
        NextInstruction = RSPPIPELINE_DO_DELAY_SLOT;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;

        if (RSPOpC.rs == 0)
        {
            JmpLabel32("BranchToJump", 0);
            m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
            NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            return;
        }
        if (!bDelayAffect)
        {
            CompConstToVariable(0, &m_GPR[RSPOpC.rs].W, GPR_Name(RSPOpC.rs));
            MoveConstToVariable((CompilePC + 8) & 0xFFC, &m_GPR[31].UW, "RA.W");
            JgeLabel32("BranchGreaterEqual", 0);
        }
        else
        {
            // Take a look at the branch compare variable
            CompConstToVariable(true, &BranchCompare, "BranchCompare");
            JeLabel32("BranchGreaterEqual", 0);
        }
        m_Recompiler.Branch_AddRef(Target, (uint32_t *)(RecompPos - 4));
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT_EXIT_DONE)
    {
        uint32_t Target = (CompilePC + ((short)RSPOpC.offset << 2) + 4) & 0xFFC;
        CompileBranchExit(Target, CompilePC + 8);
    }
    else
    {
        CompilerWarning(stdstr_f("BGEZAL error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
}

// COP0 functions

void CRSPRecompilerOps::Cop0_MF(void)
{
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    if (LogRDP)
    {
        char str[40];

        sprintf(str, "%d", RSPOpC.rd);
        PushImm32(str, RSPOpC.rd);
        sprintf(str, "%X", CompilePC);
        PushImm32(str, CompilePC);
        Call_Direct((void *)RDP_LogMF0, "RDP_LogMF0");
        AddConstToX86Reg(x86_ESP, 8);
    }

#ifndef Compile_Cop0
    Cheat_r4300iOpcode(RSP_Cop0_MF, "RSP_Cop0_MF");
    if (NextInstruction == RSPPIPELINE_NORMAL)
    {
        MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
        Ret();
        NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
    }
    else if (NextInstruction == RSPPIPELINE_DELAY_SLOT)
    {
        NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT;
    }
    else
    {
        CompilerWarning(stdstr_f("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
        BreakPoint();
    }
    return;
#elif defined(_M_IX86) && defined(_MSC_VER)
    switch (RSPOpC.rd)
    {
    case 0:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        PushImm32("RSPRegister_MEM_ADDR", RSPRegister_MEM_ADDR);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::ReadReg), "RSPRegisterHandlerPlugin::ReadReg");
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 1:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        PushImm32("RSPRegister_DRAM_ADDR", RSPRegister_DRAM_ADDR);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::ReadReg), "RSPRegisterHandlerPlugin::ReadReg");
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 2:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        PushImm32("RSPRegister_RD_LEN", RSPRegister_RD_LEN);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::ReadReg), "RSPRegisterHandlerPlugin::ReadReg");
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 3:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        PushImm32("RSPRegister_WR_LEN", RSPRegister_WR_LEN);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::ReadReg), "RSPRegisterHandlerPlugin::ReadReg");
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 4:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        PushImm32("RSPRegister_STATUS", RSPRegister_STATUS);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::ReadReg), "RSPRegisterHandlerPlugin::ReadReg");
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 5:
        MoveVariableToX86reg(RSPInfo.SP_DMA_FULL_REG, "SP_DMA_FULL_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 6:
        MoveVariableToX86reg(RSPInfo.SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 7:
        if (AudioHle || GraphicsHle || SemaphoreExit == 0)
        {
            MoveConstToVariable(0, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        }
        else
        {
            MoveVariableToX86reg(RSPInfo.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", x86_EAX);
            MoveConstToVariable(0, &RSP_Running, "RSP_Running");
            MoveConstToVariable(1, RSPInfo.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
            MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
            if (NextInstruction == RSPPIPELINE_NORMAL)
            {
                MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
                Ret();
                NextInstruction = RSPPIPELINE_FINISH_SUB_BLOCK;
            }
            else if (NextInstruction == RSPPIPELINE_DELAY_SLOT)
            {
                NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT;
            }
            else
            {
                CompilerWarning(stdstr_f("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
                BreakPoint();
            }
        }
        break;
    case 8:
        MoveVariableToX86reg(RSPInfo.DPC_START_REG, "DPC_START_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 9:
        MoveVariableToX86reg(RSPInfo.DPC_END_REG, "DPC_END_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 10:
        MoveVariableToX86reg(RSPInfo.DPC_CURRENT_REG, "DPC_CURRENT_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 11:
        MoveVariableToX86reg(RSPInfo.DPC_STATUS_REG, "DPC_STATUS_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;
    case 12:
        MoveVariableToX86reg(RSPInfo.DPC_CLOCK_REG, "DPC_CLOCK_REG", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt));
        break;

    default:
        g_Notify->DisplayError(stdstr_f("We have not implemented RSP MF CP0 reg %s (%d)", COP0_Name(RSPOpC.rd), RSPOpC.rd).c_str());
    }
#else
    g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
}

void CRSPRecompilerOps::Cop0_MT(void)
{
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (LogRDP)
    {
        char str[40];

        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        Push(x86_EAX);
        sprintf(str, "%d", RSPOpC.rd);
        PushImm32(str, RSPOpC.rd);
        sprintf(str, "%X", CompilePC);
        PushImm32(str, CompilePC);
        Call_Direct((void *)RDP_LogMT0, "RDP_LogMT0");
        AddConstToX86Reg(x86_ESP, 12);
    }

#ifndef Compile_Cop0
    Cheat_r4300iOpcode(RSP_Cop0_MT, "RSP_Cop0_MT");
    if (RSPOpC.rd == 4)
    {
        if (NextInstruction == RSPPIPELINE_NORMAL)
        {
            MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
            Ret();
            NextInstruction = RSPPIPELINE_FINISH_BLOCK;
        }
        else if (NextInstruction == RSPPIPELINE_DELAY_SLOT)
        {
            NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT;
        }
        else
        {
            CompilerWarning(stdstr_f("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
            BreakPoint();
        }
    }
#elif defined(_M_IX86) && defined(_MSC_VER)
    switch (RSPOpC.rd)
    {
    case 0:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        Push(x86_EAX);
        PushImm32("RSPRegister_MEM_ADDR", RSPRegister_MEM_ADDR);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::WriteReg), "RSPRegisterHandlerPlugin::WriteReg");
        break;
    case 1:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        Push(x86_EAX);
        PushImm32("RSPRegister_DRAM_ADDR", RSPRegister_DRAM_ADDR);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::WriteReg), "RSPRegisterHandlerPlugin::WriteReg");
        break;
    case 2:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        Push(x86_EAX);
        PushImm32("RSPRegister_RD_LEN", RSPRegister_RD_LEN);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::WriteReg), "RSPRegisterHandlerPlugin::WriteReg");
        break;
    case 3:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        Push(x86_EAX);
        PushImm32("RSPRegister_WR_LEN", RSPRegister_WR_LEN);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::WriteReg), "RSPRegisterHandlerPlugin::WriteReg");
        break;
    case 4:
        MoveConstToX86reg((uint32_t)(g_RSPRegisterHandler.get()), x86_ECX);
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        Push(x86_EAX);
        PushImm32("RSPRegister_STATUS", RSPRegister_STATUS);
        Call_Direct(AddressOf(&RSPRegisterHandlerPlugin::WriteReg), "RSPRegisterHandlerPlugin::WriteReg");
        if (NextInstruction == RSPPIPELINE_NORMAL)
        {
            MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
            Ret();
            NextInstruction = RSPPIPELINE_FINISH_BLOCK;
        }
        else if (NextInstruction == RSPPIPELINE_DELAY_SLOT)
        {
            NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT;
        }
        else
        {
            CompilerWarning(stdstr_f("MF error\nWeird Delay Slot.\n\nNextInstruction = %X\nEmulation will now stop", NextInstruction).c_str());
            BreakPoint();
        }
        break;
    case 7:
        MoveConstToVariable(0, RSPInfo.SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
        break;
    case 8:
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, RSPInfo.DPC_START_REG, "DPC_START_REG");
        MoveX86regToVariable(x86_EAX, RSPInfo.DPC_CURRENT_REG, "DPC_CURRENT_REG");
        break;
    case 9:
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, RSPInfo.DPC_END_REG, "DPC_END_REG");

        if (LogRDP)
        {
            Call_Direct((void *)RDP_LogDlist, "RDP_LogDlist");
        }

        if (RSPInfo.ProcessRdpList != NULL)
        {
            if (Profiling)
            {
                PushImm32("Timer_RDP_Running", (uint32_t)Timer_RDP_Running);
                Call_Direct((void *)StartTimer, "StartTimer");
                AddConstToX86Reg(x86_ESP, 4);
                Push(x86_EAX);
            }
            Call_Direct((void *)RSPInfo.ProcessRdpList, "ProcessRdpList");
            if (Profiling)
            {
                Call_Direct((void *)StartTimer, "StartTimer");
                AddConstToX86Reg(x86_ESP, 4);
            }
        }

        break;
    case 10:
        MoveVariableToX86reg(&m_GPR[RSPOpC.rt].UW, GPR_Name(RSPOpC.rt), x86_EAX);
        MoveX86regToVariable(x86_EAX, RSPInfo.DPC_CURRENT_REG, "DPC_CURRENT_REG");
        break;

    default:
        Cheat_r4300iOpcode(&RSPOp::Cop0_MT, "RSPOp::Cop0_MT");
        break;
    }
#else
    g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
    if (RSPOpC.rd == 2 && !ChangedPC)
    {
        uint8_t * Jump;

        TestConstToVariable(0x1000, RSPInfo.SP_MEM_ADDR_REG, "RSPInfo.SP_MEM_ADDR_REG");
        JeLabel8("DontExit", 0);
        Jump = RecompPos - 1;

        MoveConstToVariable(CompilePC + 4, PrgCount, "RSP PC");
        Ret();

        CPU_Message("DontExit:");
        x86_SetBranch8b(Jump, RecompPos);
    }
}

// COP2 functions

void CRSPRecompilerOps::Cop2_MF(void)
{
    if (RSPOpC.rt == 0)
    {
        CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
        return;
    }

#ifndef Compile_Cop2
    Cheat_r4300iOpcode(RSP_Cop2_MF, "RSP_Cop2_MF");
#else
    char Reg[256];
    uint8_t element = (uint8_t)(RSPOpC.sa >> 1);

    uint8_t element1 = 15 - element;
    uint8_t element2 = 15 - ((element + 1) % 16);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (element2 != (element1 - 1))
    {
        XorX86RegToX86Reg(x86_EAX, x86_EAX);
        XorX86RegToX86Reg(x86_EBX, x86_EBX);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rd, element1);
        MoveVariableToX86regByte(&m_Vect[RSPOpC.vs].s8(element1), Reg, x86_EAX);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rd, element2);
        MoveVariableToX86regByte(&m_Vect[RSPOpC.vs].s8(element2), Reg, x86_EBX);

        ShiftLeftSignImmed(x86_EAX, 8);
        OrX86RegToX86Reg(x86_EAX, x86_EBX);
        Cwde();

        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
    }
    else
    {
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rd, element2);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s8(element2), Reg, x86_EAX);

        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
    }
#endif
}

void CRSPRecompilerOps::Cop2_CF(void)
{
#ifndef Compile_Cop2
    Cheat_r4300iOpcode(RSP_Cop2_CF, "RSP_Cop2_CF");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    switch ((RSPOpC.rd & 0x03))
    {
    case 0:
        MoveSxVariableToX86regHalf(&m_Flags[0].HW[0], "m_Flags[0].HW[0]", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        break;
    case 1:
        MoveSxVariableToX86regHalf(&m_Flags[1].HW[0], "m_Flags[1].HW[0]", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        break;
    case 2:
    case 3:
        MoveSxVariableToX86regHalf(&m_Flags[2].HW[0], "m_Flags[2].HW[0]", x86_EAX);
        MoveX86regToVariable(x86_EAX, &m_GPR[RSPOpC.rt].W, GPR_Name(RSPOpC.rt));
        break;
    }
#endif
}

void CRSPRecompilerOps::Cop2_MT(void)
{
#ifndef Compile_Cop2
    Cheat_r4300iOpcode(RSP_Cop2_MT, "RSP_Cop2_MT");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    char Reg[256];
    uint8_t element = (uint8_t)(15 - (RSPOpC.sa >> 1));

    if (element == 0)
    {
        sprintf(Reg, "m_GPR[%i].B[1]", RSPOpC.rt);
        MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].B[1], Reg, x86_EAX);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rd, element);
        MoveX86regByteToVariable(x86_EAX, &m_Vect[RSPOpC.vs].s8(element), Reg);
    }
    else
    {
        sprintf(Reg, "m_GPR[%i].B[0]", RSPOpC.rt);
        MoveVariableToX86regHalf(&m_GPR[RSPOpC.rt].B[0], Reg, x86_EAX);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rd, element - 1);
        MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vs].s8(element - 1), Reg);
    }
#endif
}

void CRSPRecompilerOps::Cop2_CT(void)
{
#ifndef Compile_Cop2
    Cheat_r4300iOpcode(RSP_Cop2_CT, "RSP_Cop2_CT");
#else
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.rt == 0)
    {
        switch ((RSPOpC.rd & 0x03))
        {
        case 0:
            MoveConstHalfToVariable(0, &m_Flags[0].HW[0], "m_Flags[0].HW[0]");
            break;
        case 1:
            MoveConstHalfToVariable(0, &m_Flags[1].HW[0], "m_Flags[1].HW[0]");
            break;
        case 2:
        case 3:
            MoveConstByteToVariable(0, &m_Flags[2].B[0], "m_Flags[2].B[0]");
            break;
        }
    }
    else
    {
        switch ((RSPOpC.rd & 0x03))
        {
        case 0:
            MoveVariableToX86regHalf(&m_GPR[RSPOpC.rt].HW[0], GPR_Name(RSPOpC.rt), x86_EAX);
            MoveX86regHalfToVariable(x86_EAX, &m_Flags[0].HW[0], "m_Flags[0].HW[0]");
            break;
        case 1:
            MoveVariableToX86regHalf(&m_GPR[RSPOpC.rt].HW[0], GPR_Name(RSPOpC.rt), x86_EAX);
            MoveX86regHalfToVariable(x86_EAX, &m_Flags[1].HW[0], "m_Flags[1].HW[0]");
            break;
        case 2:
        case 3:
            MoveVariableToX86regByte(&m_GPR[RSPOpC.rt].B[0], GPR_Name(RSPOpC.rt), x86_EAX);
            MoveX86regByteToVariable(x86_EAX, &m_Flags[2].B[0], "m_Flags[2].B[0]");
            break;
        }
    }
#endif
}

void CRSPRecompilerOps::COP2_VECTOR(void)
{
    (this->*RSP_Recomp_Vector[RSPOpC.funct])();
}

// Vector functions

UDWORD MMX_Scratch;

void CRSPRecompilerOps::RSP_Element2Mmx(int MmxReg)
{
    char Reg[256];

    uint32_t Rs = RSPOpC.rs & 0x0f;
    uint8_t el;

    switch (Rs)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        CompilerWarning("Unimplemented RSP_Element2Mmx");
        break;

    default:
        /*
		 * Noticed the exclusive-or of seven to take into account
		 * the pseudo-swapping we have in the vector registers
		 */

        el = (RSPOpC.rs & 0x07) ^ 7;

        if (!IsMmx2Enabled)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, el);
            MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(el), Reg, x86_ECX);
            MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[0], "MMX_Scratch.HW[0]");
            MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[1], "MMX_Scratch.HW[1]");
            MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[2], "MMX_Scratch.HW[2]");
            MoveX86regHalfToVariable(x86_ECX, &MMX_Scratch.HW[3], "MMX_Scratch.HW[3]");
            MmxMoveQwordVariableToReg(MmxReg, &MMX_Scratch.HW[0], "MMX_Scratch.HW[0]");
        }
        else
        {
            uint8_t Qword = (el >> 2) & 0x1;
            el &= 0x3;

            sprintf(Reg, "m_Vect[%i].DW[%i]", RSPOpC.rt, Qword);
            MmxShuffleMemoryToReg(MmxReg, &m_Vect[RSPOpC.vt].u64(Qword), Reg, _MMX_SHUFFLE(el, el, el, el));
        }
        break;
    }
}

void CRSPRecompilerOps::RSP_MultiElement2Mmx(int MmxReg1, int MmxReg2)
{
    char Reg[256];
    uint32_t Rs = RSPOpC.rs & 0x0f;

    /*
	 * OK, this is tricky, hopefully this clears it up:
	 *
	 * $vd[0] = $vd[0] + $vt[2] 
	 * because of swapped registers becomes:
	 * $vd[7] = $vd[7] + $vt[5]
	 *
	 * We must perform this swap correctly, this involves the 3-bit
	 * exclusive or, 2-bits of which are done within a uint32_t boundary, 
	 * the last bit, is ignored because we are loading the source linearly,
	 * so the exclusive or has transparently happened on that side.
	 */

    switch (Rs)
    {
    case 0:
    case 1:
        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rt);
        MmxMoveQwordVariableToReg(MmxReg1, &m_Vect[RSPOpC.vt].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rt);
        MmxMoveQwordVariableToReg(MmxReg2, &m_Vect[RSPOpC.vt].u16(4), Reg);
        break;
    case 2:
        /* [0q]    | 0 | 0 | 2 | 2 | 4 | 4 | 6 | 6 | */
        sprintf(Reg, "m_Vect[%i].DW[0]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].u64(0), Reg, 0xF5);
        sprintf(Reg, "m_Vect[%i].DW[1]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].u64(1), Reg, 0xF5);
        break;
    case 3:
        /* [1q]    | 1 | 1 | 3 | 3 | 5 | 5 | 7 | 7 | */
        sprintf(Reg, "m_Vect[%i].DW[0]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].u64(0), Reg, 0xA0);
        //MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].s64(0), Reg, 0x0A);
        sprintf(Reg, "m_Vect[%i].DW[1]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].u64(1), Reg, 0xA0);
        //MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].s64(1), Reg, 0x0A);
        break;
    case 4:
        /* [0h]    | 0 | 0 | 0 | 0 | 4 | 4 | 4 | 4 | */
        sprintf(Reg, "m_Vect[%i].DW[0]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].u64(0), Reg, 0xFF);
        sprintf(Reg, "m_Vect[%i].DW[1]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].u64(1), Reg, 0xFF);
        break;
    case 5:
        /* [1h]    | 1 | 1 | 1 | 1 | 5 | 5 | 5 | 5 | */
        sprintf(Reg, "m_Vect[%i].DW[0]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].u64(0), Reg, 0xAA);
        sprintf(Reg, "m_Vect[%i].DW[1]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].u64(1), Reg, 0xAA);
        break;
    case 6:
        /* [2h]    | 2 | 2 | 2 | 2 | 6 | 6 | 6 | 6 | */
        sprintf(Reg, "m_Vect[%i].DW[0]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].u64(0), Reg, 0x55);
        sprintf(Reg, "m_Vect[%i].DW[1]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].u64(1), Reg, 0x55);
        break;
    case 7:
        /* [3h]    | 3 | 3 | 3 | 3 | 7 | 7 | 7 | 7 | */
        sprintf(Reg, "m_Vect[%i].DW[0]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg1, &m_Vect[RSPOpC.vt].u64(0), Reg, 0x00);
        sprintf(Reg, "m_Vect[%i].DW[1]", RSPOpC.rt);
        MmxShuffleMemoryToReg(MmxReg2, &m_Vect[RSPOpC.vt].u64(1), Reg, 0x00);
        break;

    default:
        CompilerWarning("Unimplemented RSP_MultiElement2Mmx [?]");
        break;
    }
}

bool CRSPRecompilerOps::Compile_Vector_VMULF_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    // NOTE: Problem here is the lack of +/- 0x8000 rounding
    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 0xF) < 2)
    {
        if (RSPOpC.rd == RSPOpC.rt)
        {
            MmxPmulhwRegToReg(x86_MM0, x86_MM0);
            MmxPmulhwRegToReg(x86_MM1, x86_MM1);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rt);
            MmxPmulhwRegToVariable(x86_MM0, &m_Vect[RSPOpC.vt].u16(0), Reg);
            sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rt);
            MmxPmulhwRegToVariable(x86_MM1, &m_Vect[RSPOpC.vt].u16(4), Reg);
        }
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPmulhwRegToReg(x86_MM0, x86_MM2);
        MmxPmulhwRegToReg(x86_MM1, x86_MM2);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPmulhwRegToReg(x86_MM0, x86_MM2);
        MmxPmulhwRegToReg(x86_MM1, x86_MM3);
    }
    MmxPsllwImmed(x86_MM0, 1);
    MmxPsllwImmed(x86_MM1, 1);

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VMULF(void)
{
#ifndef CompileVmulf
    Cheat_r4300iOpcode(&RSPOp::Vector_VMULF, "&RSPOp::Vector_VMULF");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VMULF_MMX())
            return;
    }

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    if (bWriteToDest)
    {
        MoveConstToX86reg(0x7fff0000, x86_ESI);
    }
    if (bWriteToAccum)
    {
        XorX86RegToX86Reg(x86_EDI, x86_EDI);
    }

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);

        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (RSPOpC.rt == RSPOpC.rd && !bOptimize)
        {
            imulX86reg(x86_EAX);
        }
        else
        {
            if (!bOptimize)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
            }
            imulX86reg(x86_EBX);
        }

        ShiftLeftSignImmed(x86_EAX, 1);
        AddConstToX86Reg(x86_EAX, 0x8000);

        if (bWriteToAccum)
        {
            MoveX86regToVariable(x86_EAX, &m_ACCUM[el].HW[1], "m_ACCUM[el].HW[1]");
            // Calculate sign extension into EDX
            MoveX86RegToX86Reg(x86_EAX, x86_EDX);
            ShiftRightSignImmed(x86_EDX, 31);
        }

        CompConstToX86reg(x86_EAX, 0x80008000);

        if (bWriteToAccum)
        {
            CondMoveEqual(x86_EDX, x86_EDI);
            MoveX86regHalfToVariable(x86_EDX, &m_ACCUM[el].HW[3], "m_ACCUM[el].HW[3]");
        }
        if (bWriteToDest)
        {
            CondMoveEqual(x86_EAX, x86_ESI);
            ShiftRightUnsignImmed(x86_EAX, 16);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), "m_Vect[RSPOpC.vd].s16(el)");
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VMULU(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VMULU, "RSPOp::Vector_VMULU");
}

void CRSPRecompilerOps::Vector_VRNDN(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VRNDN, "RSPOp::Vector_VRNDN");
}

void CRSPRecompilerOps::Vector_VRNDP(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VRNDP, "RSPOp::Vector_VRNDP");
}

void CRSPRecompilerOps::Vector_VMULQ(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VMULQ, "&RSPOp::Vector_VMULQ");
}

bool CRSPRecompilerOps::Compile_Vector_VMUDL_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if (!IsMmx2Enabled)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 0xF) < 2)
    {
        if (RSPOpC.rd == RSPOpC.rt)
        {
            MmxPmulhuwRegToReg(x86_MM0, x86_MM0);
            MmxPmulhuwRegToReg(x86_MM1, x86_MM1);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM2, &m_Vect[RSPOpC.vt].u16(0), Reg);
            sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM3, &m_Vect[RSPOpC.vt].u16(4), Reg);

            MmxPmulhuwRegToReg(x86_MM0, x86_MM2);
            MmxPmulhuwRegToReg(x86_MM1, x86_MM3);
        }
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPmulhuwRegToReg(x86_MM0, x86_MM2);
        MmxPmulhuwRegToReg(x86_MM1, x86_MM2);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPmulhuwRegToReg(x86_MM0, x86_MM2);
        MmxPmulhuwRegToReg(x86_MM1, x86_MM3);
    }

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VMUDL(void)
{
#ifndef CompileVmudl
    Cheat_r4300iOpcode(&RSPOp::Vector_VMUDL, "&RSPOp::Vector_VMUDL");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VMUDL_MMX())
            return;
    }

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    if (bWriteToAccum)
        XorX86RegToX86Reg(x86_EDI, x86_EDI);

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rd, el);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].u16(el), Reg, x86_EAX);

        if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        imulX86reg(x86_EBX);

        if (bWriteToAccum)
        {
            sprintf(Reg, "m_ACCUM[%i].UW[0]", el);
            MoveX86regToVariable(x86_EAX, &m_ACCUM[el].UW[0], Reg);
            sprintf(Reg, "m_ACCUM[%i].UW[1]", el);
            MoveX86regToVariable(x86_EDI, &m_ACCUM[el].UW[1], Reg);
        }

        if (bWriteToDest)
        {
            ShiftRightUnsignImmed(x86_EAX, 16);
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VMUDM_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if (!IsMmx2Enabled)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 0xF) < 2)
    {
        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rt);
        MmxMoveQwordVariableToReg(x86_MM4, &m_Vect[RSPOpC.vt].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rt);
        MmxMoveQwordVariableToReg(x86_MM5, &m_Vect[RSPOpC.vt].u16(4), Reg);

        // Copy the signed portion
        MmxMoveRegToReg(x86_MM2, x86_MM0);
        MmxMoveRegToReg(x86_MM3, x86_MM1);

        // high((u16)a * b)
        MmxPmulhuwRegToReg(x86_MM0, x86_MM4);
        MmxPmulhuwRegToReg(x86_MM1, x86_MM5);

        // low((a >> 15) * b)
        MmxPsrawImmed(x86_MM2, 15);
        MmxPsrawImmed(x86_MM3, 15);
        MmxPmullwRegToReg(x86_MM2, x86_MM4);
        MmxPmullwRegToReg(x86_MM3, x86_MM5);
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM4);

        // Copy the signed portion
        MmxMoveRegToReg(x86_MM2, x86_MM0);
        MmxMoveRegToReg(x86_MM3, x86_MM1);

        // high((u16)a * b)
        MmxPmulhuwRegToReg(x86_MM0, x86_MM4);
        MmxPmulhuwRegToReg(x86_MM1, x86_MM4);

        // low((a >> 15) * b)
        MmxPsrawImmed(x86_MM2, 15);
        MmxPsrawImmed(x86_MM3, 15);
        MmxPmullwRegToReg(x86_MM2, x86_MM4);
        MmxPmullwRegToReg(x86_MM3, x86_MM4);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM4, x86_MM5);

        // Copy the signed portion
        MmxMoveRegToReg(x86_MM2, x86_MM0);
        MmxMoveRegToReg(x86_MM3, x86_MM1);

        // high((u16)a * b)
        MmxPmulhuwRegToReg(x86_MM0, x86_MM4);
        MmxPmulhuwRegToReg(x86_MM1, x86_MM5);

        // low((a >> 15) * b)
        MmxPsrawImmed(x86_MM2, 15);
        MmxPsrawImmed(x86_MM3, 15);
        MmxPmullwRegToReg(x86_MM2, x86_MM4);
        MmxPmullwRegToReg(x86_MM3, x86_MM5);
    }

    // Add them up
    MmxPaddwRegToReg(x86_MM0, x86_MM2);
    MmxPaddwRegToReg(x86_MM1, x86_MM3);

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VMUDM(void)
{
#ifndef CompileVmudm
    Cheat_r4300iOpcode(&RSPOp::Vector_VMUDM, "&RSPOp::Vector_VMUDM");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VMUDM_MMX())
            return;
    }

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    Push(x86_EBP);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

    if (bWriteToDest)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.sa);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vd].s16(0), Reg, x86_ECX);
    }
    else if (!bOptimize)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vt].s16(0), Reg, x86_ECX);
    }

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);*/
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, (uint8_t)(el * 2), x86_EAX);

        if (!bOptimize)
        {
            if (bWriteToDest)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
            }
            else
            {
                MoveZxX86RegPtrDispToX86RegHalf(x86_ECX, (uint8_t)(del * 2), x86_EBX);
            }
        }

        imulX86reg(x86_EBX);

        if (bWriteToAccum == false && bWriteToDest == true)
        {
            ShiftRightUnsignImmed(x86_EAX, 16);
            /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);*/
            MoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, (uint8_t)(el * 2));
        }
        else
        {
            MoveX86RegToX86Reg(x86_EAX, x86_EDX);
            ShiftRightSignImmed(x86_EDX, 16);
            ShiftLeftSignImmed(x86_EAX, 16);

            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].UW[0]", el);
                MoveX86regToVariable(x86_EAX, &m_ACCUM[el].UW[0], Reg);
                sprintf(Reg, "m_ACCUM[%i].UW[1]", el);
                MoveX86regToVariable(x86_EDX, &m_ACCUM[el].UW[1], Reg);
            }
            if (bWriteToDest)
            {
                /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vd].s16(el), Reg);*/
                MoveX86regHalfToX86regPointerDisp(x86_EDX, x86_ECX, (uint8_t)(el * 2));
            }
        }
    }

    Pop(x86_EBP);
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VMUDN_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 0xF) < 2)
    {
        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rt);
        MmxPmullwVariableToReg(x86_MM0, &m_Vect[RSPOpC.vt].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rt);
        MmxPmullwVariableToReg(x86_MM1, &m_Vect[RSPOpC.vt].u16(4), Reg);
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPmullwRegToReg(x86_MM0, x86_MM2);
        MmxPmullwRegToReg(x86_MM1, x86_MM2);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPmullwRegToReg(x86_MM0, x86_MM2);
        MmxPmullwRegToReg(x86_MM1, x86_MM3);
    }

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VMUDN(void)
{
#ifndef CompileVmudn
    Cheat_r4300iOpcode(&RSPOp::Vector_VMUDN, "RSPOp::Vector_VMUDN");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VMUDN_MMX())
            return;
    }

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    Push(x86_EBP);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        /*sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].u16(el), Reg, x86_EAX);*/
        MoveZxX86RegPtrDispToX86RegHalf(x86_EBP, (uint8_t)(el * 2), x86_EAX);

        if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        imulX86reg(x86_EBX);

        if (bWriteToDest)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }

        if (bWriteToAccum)
        {
            MoveX86RegToX86Reg(x86_EAX, x86_EDX);
            ShiftRightSignImmed(x86_EDX, 16);
            ShiftLeftSignImmed(x86_EAX, 16);
            sprintf(Reg, "m_ACCUM[%i].UW[0]", el);
            MoveX86regToVariable(x86_EAX, &m_ACCUM[el].UW[0], Reg);
            sprintf(Reg, "m_ACCUM[%i].UW[1]", el);
            MoveX86regToVariable(x86_EDX, &m_ACCUM[el].UW[1], Reg);
        }
    }
    Pop(x86_EBP);
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VMUDH_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].s16(0), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].s16(4), Reg);

    // Registers 4 and 5 are high
    MmxMoveRegToReg(x86_MM4, x86_MM0);
    MmxMoveRegToReg(x86_MM5, x86_MM1);

    if ((RSPOpC.rs & 0x0f) < 2)
    {
        if (RSPOpC.rd == RSPOpC.rt)
        {
            MmxPmullwRegToReg(x86_MM0, x86_MM0);
            MmxPmulhwRegToReg(x86_MM4, x86_MM4);
            MmxPmullwRegToReg(x86_MM1, x86_MM1);
            MmxPmulhwRegToReg(x86_MM5, x86_MM5);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM2, &m_Vect[RSPOpC.vt].s16(0), Reg);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM3, &m_Vect[RSPOpC.vt].s16(4), Reg);

            MmxPmullwRegToReg(x86_MM0, x86_MM2);
            MmxPmulhwRegToReg(x86_MM4, x86_MM2);
            MmxPmullwRegToReg(x86_MM1, x86_MM3);
            MmxPmulhwRegToReg(x86_MM5, x86_MM3);
        }
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);

        MmxPmullwRegToReg(x86_MM0, x86_MM2);
        MmxPmulhwRegToReg(x86_MM4, x86_MM2);
        MmxPmullwRegToReg(x86_MM1, x86_MM2);
        MmxPmulhwRegToReg(x86_MM5, x86_MM2);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);

        MmxPmullwRegToReg(x86_MM0, x86_MM2);
        MmxPmulhwRegToReg(x86_MM4, x86_MM2);
        MmxPmullwRegToReg(x86_MM1, x86_MM3);
        MmxPmulhwRegToReg(x86_MM5, x86_MM3);
    }

    // 0 and 1 are low, 4 and 5 are high
    MmxMoveRegToReg(x86_MM6, x86_MM0);
    MmxMoveRegToReg(x86_MM7, x86_MM1);

    MmxUnpackLowWord(x86_MM0, x86_MM4);
    MmxUnpackHighWord(x86_MM6, x86_MM4);
    MmxUnpackLowWord(x86_MM1, x86_MM5);
    MmxUnpackHighWord(x86_MM7, x86_MM5);

    // Integrate copies
    MmxPackSignedDwords(x86_MM0, x86_MM6);
    MmxPackSignedDwords(x86_MM1, x86_MM7);

    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].s16(0), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].s16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VMUDH(void)
{
#ifndef CompileVmudh
    Cheat_r4300iOpcode(&RSPOp::Vector_VMUDH, "RSPOp::Vector_VMUDH");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(EntireAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VMUDH_MMX())
            return;
    }

    if (bWriteToDest == false && bOptimize == true)
    {
        Push(x86_EBP);
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);

        // Load source
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);

        // Pipe lined segment 0

        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 0, x86_EAX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 2, x86_ECX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 4, x86_EDI);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 6, x86_ESI);

        ImulX86RegToX86Reg(x86_EAX, x86_EBX);
        ImulX86RegToX86Reg(x86_ECX, x86_EBX);
        ImulX86RegToX86Reg(x86_EDI, x86_EBX);
        ImulX86RegToX86Reg(x86_ESI, x86_EBX);
        XorX86RegToX86Reg(x86_EDX, x86_EDX);

        MoveOffsetToX86reg((size_t)&m_ACCUM[0].W[0], "m_ACCUM[0].W[0]", x86_EBP);

        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 0);
        MoveX86RegToX86regPointerDisp(x86_EAX, x86_EBP, 4);
        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 8);
        MoveX86RegToX86regPointerDisp(x86_ECX, x86_EBP, 12);
        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 16);
        MoveX86RegToX86regPointerDisp(x86_EDI, x86_EBP, 20);
        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 24);
        MoveX86RegToX86regPointerDisp(x86_ESI, x86_EBP, 28);

        // Pipe lined segment 1

        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 8, x86_EAX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 10, x86_ECX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 12, x86_EDI);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 14, x86_ESI);

        ImulX86RegToX86Reg(x86_EAX, x86_EBX);
        ImulX86RegToX86Reg(x86_ECX, x86_EBX);
        ImulX86RegToX86Reg(x86_EDI, x86_EBX);
        ImulX86RegToX86Reg(x86_ESI, x86_EBX);
        XorX86RegToX86Reg(x86_EDX, x86_EDX);

        MoveOffsetToX86reg((size_t)&m_ACCUM[0].W[0], "m_ACCUM[0].W[0]", x86_EBP);

        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 32);
        MoveX86RegToX86regPointerDisp(x86_EAX, x86_EBP, 36);
        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 40);
        MoveX86RegToX86regPointerDisp(x86_ECX, x86_EBP, 44);
        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 48);
        MoveX86RegToX86regPointerDisp(x86_EDI, x86_EBP, 52);
        MoveX86RegToX86regPointerDisp(x86_EDX, x86_EBP, 56);
        MoveX86RegToX86regPointerDisp(x86_ESI, x86_EBP, 60);

        Pop(x86_EBP);
    }
    else
    {
        if (bOptimize)
        {
            del = (RSPOpC.rs & 0x07) ^ 7;
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }
        if (bWriteToDest)
        {

            // Prepare for conditional moves

            MoveConstToX86reg(0x00007fff, x86_ESI);
            MoveConstToX86reg(0xFFFF8000, x86_EDI);
        }

        for (count = 0; count < 8; count++)
        {
            CPU_Message("     Iteration: %i", count);
            el = Indx[RSPOpC.e].B[count];
            del = EleSpec[RSPOpC.e].B[el];

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

            if (!bOptimize)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
            }
            imulX86reg(x86_EBX);

            if (bWriteToAccum)
            {
                MoveX86regToVariable(x86_EAX, &m_ACCUM[el].W[1], "m_ACCUM[el].W[1]");
                MoveConstToVariable(0, &m_ACCUM[el].W[0], "m_ACCUM[el].W[0]");
            }

            if (bWriteToDest)
            {
                CompX86RegToX86Reg(x86_EAX, x86_ESI);
                CondMoveGreater(x86_EAX, x86_ESI);
                CompX86RegToX86Reg(x86_EAX, x86_EDI);
                CondMoveLess(x86_EAX, x86_EDI);

                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
                MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
            }
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VMACF(void)
{
#ifndef CompileVmacf
    Cheat_r4300iOpcode(&RSPOp::Vector_VMACF, "&RSPOp::Vector_VMACF");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x00007fff, x86_ESI);
        MoveConstToX86reg(0xFFFF8000, x86_EDI);
    }
    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    for (count = 0; count < 8; count++)
    {
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        CPU_Message("     Iteration: %i", count);

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        imulX86reg(x86_EBX);

        MoveX86RegToX86Reg(x86_EAX, x86_EDX);
        ShiftRightSignImmed(x86_EDX, 15);
        ShiftLeftSignImmed(x86_EAX, 17);

        AddX86regToVariable(x86_EAX, &m_ACCUM[el].W[0], "m_ACCUM[el].W[0]");
        AdcX86regToVariable(x86_EDX, &m_ACCUM[el].W[1], "m_ACCUM[el].W[1]");

        if (bWriteToDest)
        {
            MoveVariableToX86reg(&m_ACCUM[el].W[1], "m_ACCUM[el].W[1]", x86_EAX);

            CompX86RegToX86Reg(x86_EAX, x86_ESI);
            CondMoveGreater(x86_EAX, x86_ESI);
            CompX86RegToX86Reg(x86_EAX, x86_EDI);
            CondMoveLess(x86_EAX, x86_EDI);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VMACU(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VMACU, "&RSPOp::Vector_VMACU");
}

void CRSPRecompilerOps::Vector_VMACQ(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VMACQ, "RSPOp::Vector_VMACQ");
}

void CRSPRecompilerOps::Vector_VMADL(void)
{
#ifndef CompileVmadl
    Cheat_r4300iOpcode(&RSPOp::Vector_VMADL, "&RSPOp::Vector_VMADL");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x00007FFF, x86_ESI);
        MoveConstToX86reg(0xFFFF8000, x86_EDI);

        Push(x86_EBP);
        MoveConstToX86reg(0x0000FFFF, x86_EBP);
    }

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        imulX86reg(x86_EBX);
        sprintf(Reg, "m_ACCUM[%i].W[0]", el);
        AddX86regToVariable(x86_EAX, &m_ACCUM[el].W[0], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", el);
        AdcConstToVariable(&m_ACCUM[el].W[1], Reg, 0);

        if (bWriteToDest != false)
        {
            XorX86RegToX86Reg(x86_EDX, x86_EDX);
            MoveVariableToX86reg(&m_ACCUM[el].W[1], "m_ACCUM[el].W[1]", x86_EAX);
            MoveZxVariableToX86regHalf(&m_ACCUM[el].HW[1], "m_ACCUM[el].hW[1]", x86_ECX);

            CompX86RegToX86Reg(x86_EAX, x86_ESI);
            CondMoveGreater(x86_ECX, x86_EBP);
            CompX86RegToX86Reg(x86_EAX, x86_EDI);
            CondMoveLess(x86_ECX, x86_EDX);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }

    if (bWriteToDest)
    {
        Pop(x86_EBP);
    }
#endif
}

void CRSPRecompilerOps::Vector_VMADM(void)
{
#ifndef CompileVmadm
    Cheat_r4300iOpcode(&RSPOp::Vector_VMADM, "&RSPOp::Vector_VMADM");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }
    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x00007fff, x86_ESI);
        MoveConstToX86reg(0xFFFF8000, x86_EDI);
    }

    Push(x86_EBP);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

    if (bWriteToDest)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.sa);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vd].s16(0), Reg, x86_ECX);
    }
    else if (!bOptimize)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vt].s16(0), Reg, x86_ECX);
    }

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);*/
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, (uint8_t)(el * 2), x86_EAX);

        if (!bOptimize)
        {
            if (bWriteToDest)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), "m_Vect[RSPOpC.vt].s16(del)", x86_EBX);
            }
            else
            {
                MoveZxX86RegPtrDispToX86RegHalf(x86_ECX, (uint8_t)(del * 2), x86_EBX);
            }
        }

        imulX86reg(x86_EBX);

        MoveX86RegToX86Reg(x86_EAX, x86_EDX);
        ShiftRightSignImmed(x86_EDX, 16);
        ShiftLeftSignImmed(x86_EAX, 16);
        AddX86regToVariable(x86_EAX, &m_ACCUM[el].W[0], "m_ACCUM[el].W[0]");
        AdcX86regToVariable(x86_EDX, &m_ACCUM[el].W[1], "m_ACCUM[el].W[1]");

        if (bWriteToDest)
        {
            // For compare
            sprintf(Reg, "m_ACCUM[%i].W[1]", el);
            MoveVariableToX86reg(&m_ACCUM[el].W[1], "m_ACCUM[el].W[1]", x86_EAX);

            CompX86RegToX86Reg(x86_EAX, x86_ESI);
            CondMoveGreater(x86_EAX, x86_ESI);
            CompX86RegToX86Reg(x86_EAX, x86_EDI);
            CondMoveLess(x86_EAX, x86_EDI);

            /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
			MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);*/
            MoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, (uint8_t)(el * 2));
        }
    }

    Pop(x86_EBP);
#endif
}

void CRSPRecompilerOps::Vector_VMADN(void)
{
#ifndef CompileVmadn
    Cheat_r4300iOpcode(&RSPOp::Vector_VMADN, "RSPOp::Vector_VMADN");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }
    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x0000ffff, x86_ESI);
        MoveConstToX86reg(0x00000000, x86_EDI);
    }

    Push(x86_EBP);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        /*sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].u16(el), Reg, x86_EAX);*/
        MoveZxX86RegPtrDispToX86RegHalf(x86_EBP, (uint8_t)(el * 2), x86_EAX);

        if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        imulX86reg(x86_EBX);

        MoveX86RegToX86Reg(x86_EAX, x86_EDX);
        ShiftRightSignImmed(x86_EDX, 16);
        ShiftLeftSignImmed(x86_EAX, 16);
        AddX86regToVariable(x86_EAX, &m_ACCUM[el].W[0], "m_ACCUM[el].W[0]");
        AdcX86regToVariable(x86_EDX, &m_ACCUM[el].W[1], "m_ACCUM[el].W[1]");

        if (bWriteToDest)
        {
            // For compare
            sprintf(Reg, "m_ACCUM[%i].W[1]", el);
            MoveVariableToX86reg(&m_ACCUM[el].W[1], Reg, x86_EAX);

            // For vector
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveVariableToX86regHalf(&m_ACCUM[el].HW[1], Reg, x86_ECX);

            // TODO: Weird eh?
            CompConstToX86reg(x86_EAX, 0x7fff);
            CondMoveGreater(x86_ECX, x86_ESI);
            CompConstToX86reg(x86_EAX, (uint32_t)(-0x8000));
            CondMoveLess(x86_ECX, x86_EDI);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }
    Pop(x86_EBP);
#endif
}

void CRSPRecompilerOps::Vector_VMADH(void)
{
#ifndef CompileVmadh
    Cheat_r4300iOpcode(&RSPOp::Vector_VMADH, "RSPOp::Vector_VMADH");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x00007fff, x86_ESI);
        MoveConstToX86reg(0xFFFF8000, x86_EDI);
    }

    if (bWriteToDest == false && bOptimize == true)
    {
        Push(x86_EBP);
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

        // Pipe lined segment 0

        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 0, x86_EAX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 2, x86_ECX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 4, x86_EDI);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 6, x86_ESI);

        ImulX86RegToX86Reg(x86_EAX, x86_EBX);
        ImulX86RegToX86Reg(x86_ECX, x86_EBX);
        ImulX86RegToX86Reg(x86_EDI, x86_EBX);
        ImulX86RegToX86Reg(x86_ESI, x86_EBX);

        sprintf(Reg, "m_ACCUM[%i].W[1]", 0);
        AddX86regToVariable(x86_EAX, &m_ACCUM[0].W[1], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", 1);
        AddX86regToVariable(x86_ECX, &m_ACCUM[1].W[1], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", 2);
        AddX86regToVariable(x86_EDI, &m_ACCUM[2].W[1], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", 3);
        AddX86regToVariable(x86_ESI, &m_ACCUM[3].W[1], Reg);

        // Pipe lined segment 1

        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 8, x86_EAX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 10, x86_ECX);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 12, x86_EDI);
        MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, 14, x86_ESI);

        ImulX86RegToX86Reg(x86_EAX, x86_EBX);
        ImulX86RegToX86Reg(x86_ECX, x86_EBX);
        ImulX86RegToX86Reg(x86_EDI, x86_EBX);
        ImulX86RegToX86Reg(x86_ESI, x86_EBX);

        sprintf(Reg, "m_ACCUM[%i].W[1]", 4);
        AddX86regToVariable(x86_EAX, &m_ACCUM[4].W[1], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", 5);
        AddX86regToVariable(x86_ECX, &m_ACCUM[5].W[1], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", 6);
        AddX86regToVariable(x86_EDI, &m_ACCUM[6].W[1], Reg);
        sprintf(Reg, "m_ACCUM[%i].W[1]", 7);
        AddX86regToVariable(x86_ESI, &m_ACCUM[7].W[1], Reg);

        Pop(x86_EBP);
    }
    else
    {
        Push(x86_EBP);
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
        MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

        if (bWriteToDest)
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.sa);
            MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vd].s16(0), Reg, x86_ECX);
        }
        else if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vt].s16(0), Reg, x86_ECX);
        }

        for (count = 0; count < 8; count++)
        {
            CPU_Message("     Iteration: %i", count);
            el = Indx[RSPOpC.e].B[count];
            del = EleSpec[RSPOpC.e].B[el];

            /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
			MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);*/
            MoveSxX86RegPtrDispToX86RegHalf(x86_EBP, (uint8_t)(el * 2), x86_EAX);

            if (!bOptimize)
            {
                if (bWriteToDest)
                {
                    sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                    MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
                }
                else
                {
                    MoveSxX86RegPtrDispToX86RegHalf(x86_ECX, (uint8_t)(del * 2), x86_EBX);
                }
            }

            imulX86reg(x86_EBX);
            sprintf(Reg, "m_ACCUM[%i].W[1]", el);
            AddX86regToVariable(x86_EAX, &m_ACCUM[el].W[1], Reg);

            if (bWriteToDest)
            {
                MoveVariableToX86reg(&m_ACCUM[el].W[1], "m_ACCUM[el].W[1]", x86_EAX);

                CompX86RegToX86Reg(x86_EAX, x86_ESI);
                CondMoveGreater(x86_EAX, x86_ESI);
                CompX86RegToX86Reg(x86_EAX, x86_EDI);
                CondMoveLess(x86_EAX, x86_EDI);

                /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
				MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);*/
                MoveX86regHalfToX86regPointerDisp(x86_EAX, x86_ECX, (uint8_t)(el * 2));
            }
        }
        Pop(x86_EBP);
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VADD_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPaddswRegToReg(x86_MM0, x86_MM2);
        MmxPaddswRegToReg(x86_MM1, x86_MM2);
    }
    else if ((RSPOpC.rs & 15) < 2)
    {
        if (RSPOpC.rd == RSPOpC.rt)
        {
            MmxPaddswRegToReg(x86_MM0, x86_MM0);
            MmxPaddswRegToReg(x86_MM1, x86_MM1);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxPaddswVariableToReg(x86_MM0, &m_Vect[RSPOpC.vt].s16(0), Reg);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxPaddswVariableToReg(x86_MM1, &m_Vect[RSPOpC.vt].s16(4), Reg);
        }
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPaddswRegToReg(x86_MM0, x86_MM2);
        MmxPaddswRegToReg(x86_MM1, x86_MM3);
    }

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (IsNextInstructionMmx(CompilePC) != true)
    {
        MmxEmptyMultimediaState();
    }

    return true;
}

void CRSPRecompilerOps::Vector_VADD(void)
{
#ifndef CompileVadd
    Cheat_r4300iOpcode(&RSPOp::Vector_VADD, "RSPOp::Vector_VADD");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bElement = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    bool bFlagUseage = UseRspFlags(CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bWriteToAccum == false && bFlagUseage == false)
    {
        if (true == Compile_Vector_VADD_MMX())
            return;
    }

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }
    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x00007fff, x86_ESI);
        MoveConstToX86reg(0xffff8000, x86_EDI);
    }

    // Used for invoking x86 carry flag
    XorX86RegToX86Reg(x86_ECX, x86_ECX);
    Push(x86_EBP);
    MoveVariableToX86reg(&m_Flags[0].UW, "m_Flags[0].UW", x86_EBP);

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        MoveX86RegToX86Reg(x86_EBP, x86_EDX);
        AndConstToX86Reg(x86_EDX, 1 << (7 - el));
        CompX86RegToX86Reg(x86_ECX, x86_EDX);

        AdcX86RegToX86Reg(x86_EAX, x86_EBX);

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }
        if (bWriteToDest != false)
        {
            CompX86RegToX86Reg(x86_EAX, x86_ESI);
            CondMoveGreater(x86_EAX, x86_ESI);
            CompX86RegToX86Reg(x86_EAX, x86_EDI);
            CondMoveLess(x86_EAX, x86_EDI);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }
    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    Pop(x86_EBP);
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VSUB_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 15) >= 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPsubswRegToReg(x86_MM0, x86_MM2);
        MmxPsubswRegToReg(x86_MM1, x86_MM2);
    }
    else if ((RSPOpC.rs & 15) < 2)
    {
        if (RSPOpC.rd == RSPOpC.rt)
        {
            MmxPsubswRegToReg(x86_MM0, x86_MM0);
            MmxPsubswRegToReg(x86_MM1, x86_MM1);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxPsubswVariableToReg(x86_MM0, &m_Vect[RSPOpC.vt].s16(0), Reg);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxPsubswVariableToReg(x86_MM1, &m_Vect[RSPOpC.vt].s16(4), Reg);
        }
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPsubswRegToReg(x86_MM0, x86_MM2);
        MmxPsubswRegToReg(x86_MM1, x86_MM3);
    }

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);
    if (IsNextInstructionMmx(CompilePC) != true)
    {
        MmxEmptyMultimediaState();
    }

    return true;
}

void CRSPRecompilerOps::Vector_VSUB(void)
{
#ifndef CompileVsub
    Cheat_r4300iOpcode(&RSPOp::Vector_VSUB, "&RSPOp::Vector_VSUB");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bOptimize = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    bool bFlagUseage = UseRspFlags(CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bWriteToAccum == false && bFlagUseage == false)
    {
        if (true == Compile_Vector_VSUB_MMX())
            return;
    }

    Push(x86_EBP);

    // Used for invoking the x86 carry flag
    XorX86RegToX86Reg(x86_ECX, x86_ECX);
    MoveVariableToX86reg(&m_Flags[0].UW, "m_Flags[0].UW", x86_EBP);

    if (bOptimize)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    if (bWriteToDest)
    {

        // Prepare for conditional moves

        MoveConstToX86reg(0x00007fff, x86_ESI);
        MoveConstToX86reg(0xffff8000, x86_EDI);
    }

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), "m_Vect[RSPOpC.vs].s16(el)", x86_EAX);
        if (!bOptimize)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        MoveX86RegToX86Reg(x86_EBP, x86_EDX);
        AndConstToX86Reg(x86_EDX, 1 << (7 - el));
        CompX86RegToX86Reg(x86_ECX, x86_EDX);

        SbbX86RegToX86Reg(x86_EAX, x86_EBX);

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }

        if (bWriteToDest != false)
        {
            CompX86RegToX86Reg(x86_EAX, x86_ESI);
            CondMoveGreater(x86_EAX, x86_ESI);
            CompX86RegToX86Reg(x86_EAX, x86_EDI);
            CondMoveLess(x86_EAX, x86_EDI);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }

    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    Pop(x86_EBP);
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VABS_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 15) >= 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxMoveRegToReg(x86_MM3, x86_MM2);
    }
    else if ((RSPOpC.rs & 15) < 2)
    {
        if (RSPOpC.rd != RSPOpC.rt)
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM2, &m_Vect[RSPOpC.vt].s16(0), Reg);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM3, &m_Vect[RSPOpC.vt].s16(4), Reg);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxMoveRegToReg(x86_MM2, x86_MM0);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxMoveRegToReg(x86_MM3, x86_MM1);
        }
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
    }

    if (RSPOpC.rd == RSPOpC.rt)
    {
        MmxPsrawImmed(x86_MM2, 15);
        MmxPsrawImmed(x86_MM3, 15);

        MmxXorRegToReg(x86_MM0, x86_MM2);
        MmxXorRegToReg(x86_MM1, x86_MM3);

        MmxPsubswRegToReg(x86_MM0, x86_MM2);
        MmxPsubswRegToReg(x86_MM1, x86_MM3);
    }
    else
    {
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

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (IsNextInstructionMmx(CompilePC) != true)
    {
        MmxEmptyMultimediaState();
    }

    return true;
}

void CRSPRecompilerOps::Vector_VABS(void)
{
#ifndef CompileVabs
    Cheat_r4300iOpcode(&RSPOp::Vector_VABS, "RSPOp::Vector_VABS");
#else
    uint8_t count, el, del;
    char Reg[256];

    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VABS_MMX())
            return;
    }

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        if (RSPOpC.rd == RSPOpC.rt && (RSPOpC.rs & 0xF) < 2)
        {

            // Optimize: EDI/ESI unused, and ECX is CONST etc.

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

            // Obtain the negative of the source
            MoveX86RegToX86Reg(x86_EAX, x86_EBX);
            NegateX86reg(x86_EBX);

            // Determine negative value,
            // Note: negate(FFFF8000h) == 00008000h

            MoveConstToX86reg(0x7fff, x86_ECX);
            CompConstToX86reg(x86_EBX, 0x00008000);
            CondMoveEqual(x86_EBX, x86_ECX);

            // sign clamp, dest = (eax >= 0) ? eax : ebx
            CompConstToX86reg(x86_EAX, 0);
            CondMoveLess(x86_EAX, x86_EBX);

            if (bWriteToDest)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
                MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
            }
            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
            }
        }
        else
        {

            // Optimize: ESI unused, and EDX is CONST etc.

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);

            // Obtain the negative of the source
            MoveX86RegToX86Reg(x86_EBX, x86_ECX);
            NegateX86reg(x86_EBX);

            // Determine negative value,
            // Note: negate(FFFF8000h) == 00008000h

            MoveConstToX86reg(0x7fff, x86_EDX);
            CompConstToX86reg(x86_EBX, 0x00008000);
            CondMoveEqual(x86_EBX, x86_EDX);

            // sign clamp, dest = (eax >= 0) ? ecx : ebx
            CompConstToX86reg(x86_EAX, 0);
            CondMoveGreaterEqual(x86_EDI, x86_ECX);
            CondMoveLess(x86_EDI, x86_EBX);

            if (bWriteToDest)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
                MoveX86regHalfToVariable(x86_EDI, &m_Vect[RSPOpC.vd].s16(el), Reg);
            }
            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_EDI, &m_ACCUM[el].HW[1], Reg);
            }
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VADDC(void)
{
#ifndef CompileVaddc
    Cheat_r4300iOpcode(&RSPOp::Vector_VADDC, "&RSPOp::Vector_VADDC");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    bool bElement = (RSPOpC.rs & 8) ? true : false;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    // Initialize flag register
    XorX86RegToX86Reg(x86_ECX, x86_ECX);

    Push(x86_EBP);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vs].s16(0), Reg, x86_EBP);

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        /*sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
		MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);*/
        MoveZxX86RegPtrDispToX86RegHalf(x86_EBP, (uint8_t)(el * 2), x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        AddX86RegToX86Reg(x86_EAX, x86_EBX);

        XorX86RegToX86Reg(x86_EDX, x86_EDX);
        TestConstToX86Reg(0xFFFF0000, x86_EAX);
        Setnz(x86_EDX);
        if ((7 - el) != 0)
        {
            ShiftLeftSignImmed(x86_EDX, (uint8_t)(7 - el));
        }
        OrX86RegToX86Reg(x86_ECX, x86_EDX);

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }

        if (bWriteToDest != false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }
    MoveX86regToVariable(x86_ECX, &m_Flags[0].UW, "m_Flags[0].UW");
    Pop(x86_EBP);
#endif
}

void CRSPRecompilerOps::Vector_VSUBC(void)
{
#ifndef CompileVsubc
    Cheat_r4300iOpcode(&RSPOp::Vector_VSUBC, "&RSPOp::Vector_VSUBC");
#else
    char Reg[256];
    uint8_t count, el, del;

    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    bool bElement = (RSPOpC.rs & 8) ? true : false;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    // Initialize flag register
    XorX86RegToX86Reg(x86_ECX, x86_ECX);

    for (count = 0; count < 8; count++)
    {
        CPU_Message("     Iteration: %i", count);
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
        }

        SubX86RegToX86Reg(x86_EAX, x86_EBX);

        XorX86RegToX86Reg(x86_EDX, x86_EDX);
        TestConstToX86Reg(0x0000FFFF, x86_EAX);
        Setnz(x86_EDX);
        ShiftLeftSignImmed(x86_EDX, (uint8_t)(15 - el));
        OrX86RegToX86Reg(x86_ECX, x86_EDX);

        XorX86RegToX86Reg(x86_EDX, x86_EDX);
        TestConstToX86Reg(0xFFFF0000, x86_EAX);
        Setnz(x86_EDX);
        ShiftLeftSignImmed(x86_EDX, (uint8_t)(7 - el));
        OrX86RegToX86Reg(x86_ECX, x86_EDX);

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }
        if (bWriteToDest != false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }
    }
    MoveX86regToVariable(x86_ECX, &m_Flags[0].UW, "m_Flags[0].UW");
#endif
}

void CRSPRecompilerOps::Vector_VSAW(void)
{
#ifndef CompileVsaw
    Cheat_r4300iOpcode(&RSPOp::Vector_VSAW, "RSPOp::Vector_VSAW");
#else
    char Reg[256];
    uint32_t Word;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    switch ((RSPOpC.rs & 0xF))
    {
    case 8: Word = 3; break;
    case 9: Word = 2; break;
    case 10: Word = 1; break;
    default:
        MoveConstToVariable(0, &m_Vect[RSPOpC.vd].u64(1), "m_Vect[RSPOpC.vd].s64(1)");
        MoveConstToVariable(0, &m_Vect[RSPOpC.vd].u64(0), "m_Vect[RSPOpC.vd].s64(0)");
        return;
    }

    sprintf(Reg, "m_ACCUM[1].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[1].HW[Word], Reg, x86_EAX);
    sprintf(Reg, "m_ACCUM[3].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[3].HW[Word], Reg, x86_EBX);
    sprintf(Reg, "m_ACCUM[5].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[5].HW[Word], Reg, x86_ECX);
    sprintf(Reg, "m_ACCUM[7].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[7].HW[Word], Reg, x86_EDX);

    ShiftLeftSignImmed(x86_EAX, 16);
    ShiftLeftSignImmed(x86_EBX, 16);
    ShiftLeftSignImmed(x86_ECX, 16);
    ShiftLeftSignImmed(x86_EDX, 16);

    sprintf(Reg, "m_ACCUM[0].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[0].HW[Word], Reg, x86_EAX);
    sprintf(Reg, "m_ACCUM[2].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[2].HW[Word], Reg, x86_EBX);
    sprintf(Reg, "m_ACCUM[4].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[4].HW[Word], Reg, x86_ECX);
    sprintf(Reg, "m_ACCUM[6].HW[%i]", Word);
    MoveVariableToX86regHalf(&m_ACCUM[6].HW[Word], Reg, x86_EDX);

    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.sa);
    MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(0), Reg);
    sprintf(Reg, "m_Vect[%i].HW[2]", RSPOpC.sa);
    MoveX86regToVariable(x86_EBX, &m_Vect[RSPOpC.vd].s16(2), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.sa);
    MoveX86regToVariable(x86_ECX, &m_Vect[RSPOpC.vd].s16(4), Reg);
    sprintf(Reg, "m_Vect[%i].HW[6]", RSPOpC.sa);
    MoveX86regToVariable(x86_EDX, &m_Vect[RSPOpC.vd].s16(6), Reg);
#endif
}

void CRSPRecompilerOps::Vector_VLT(void)
{
#ifndef CompileVlt
    Cheat_r4300iOpcode(&RSPOp::Vector_VLT, "&RSPOp::Vector_VLT");
#else
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    uint8_t * jump[3];
    uint32_t flag;
    char Reg[256];
    uint8_t el, del, last;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    last = (uint8_t)-1;
    XorX86RegToX86Reg(x86_EBX, x86_EBX);
    MoveVariableToX86reg(&m_Flags[0].UW, "&m_Flags[0].UW", x86_ESI);
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        flag = 0x101 << (7 - el);
        if (del != el || RSPOpC.rt != RSPOpC.rd)
        {
            if (del != last)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_ECX);
                last = del;
            }
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EDX);

            CompX86RegToX86Reg(x86_EDX, x86_ECX);
            JgeLabel8("jge", 0);
            jump[0] = (uint8_t *)(RecompPos - 1);

            if (bWriteToAccum || bWriteToDest)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_EDX, &m_ACCUM[el].HW[1], Reg);
            }
            OrConstToX86Reg((flag & 0xFF), x86_EBX);

            JmpLabel8("jmp", 0);
            jump[1] = (uint8_t *)(RecompPos - 1);
            x86_SetBranch8b(jump[0], RecompPos);

            if (bWriteToAccum || bWriteToDest)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
            }
            JneLabel8("jne", 0);
            jump[2] = (uint8_t *)(RecompPos - 1);

            MoveX86RegToX86Reg(x86_ESI, x86_EDI);
            AndConstToX86Reg(x86_EDI, flag);
            ShiftRightUnsignImmed(x86_EDI, 8);
            AndX86RegToX86Reg(x86_EDI, x86_ESI);
            OrX86RegToX86Reg(x86_EBX, x86_EDI);

            x86_SetBranch8b(jump[2], RecompPos);
            x86_SetBranch8b(jump[1], RecompPos);
        }
        else
        {
            MoveX86RegToX86Reg(x86_ESI, x86_EDI);
            if (bWriteToAccum || bWriteToDest)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
            }
            AndConstToX86Reg(x86_EDI, flag);
            ShiftRightUnsignImmed(x86_EDI, 8);
            AndX86RegToX86Reg(x86_EDI, x86_ESI);
            OrX86RegToX86Reg(x86_EBX, x86_EDI);
        }
    }

    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    MoveX86regToVariable(x86_EBX, &m_Flags[1].UW, "m_Flags[1].UW");

    if (bWriteToDest != false)
    {
        for (el = 0; el < 8; el += 2)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveVariableToX86regHalf(&m_ACCUM[el].HW[1], Reg, x86_EAX);

            sprintf(Reg, "m_ACCUM[%i].HW[1]", el + 1);
            MoveVariableToX86regHalf(&m_ACCUM[el + 1].HW[1], Reg, x86_ECX);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el + 1);
            MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].s16(el + 1), Reg);
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VEQ(void)
{
#ifndef CompileVeq
    Cheat_r4300iOpcode(&RSPOp::Vector_VEQ, "&RSPOp::Vector_VEQ");
#else
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    uint32_t flag;
    char Reg[256];
    uint8_t count, el, del, last = (uint8_t)-1;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveZxVariableToX86regHalf(&m_Flags[0].UHW[1], "&m_Flags[0].UHW[1]", x86_EBX);
    XorConstToX86Reg(x86_EBX, 0xFFFF);
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        flag = (0x101 << (7 - el)) ^ 0xFFFF;
        if (del != el || RSPOpC.rt != RSPOpC.rd)
        {
            if (del != last)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_ECX);
                last = del;
            }

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EDX);

            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
            }

            SubX86RegToX86Reg(x86_EDX, x86_ECX);
            CompConstToX86reg(x86_EDX, 1);
            SbbX86RegToX86Reg(x86_EDX, x86_EDX);
            OrConstToX86Reg(flag, x86_EDX);
            AndX86RegToX86Reg(x86_EBX, x86_EDX);
        }
        else
        {
            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
            }
        }
    }

    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    MoveX86regToVariable(x86_EBX, &m_Flags[1].UW, "m_Flags[1].UW");

    if (bWriteToDest != false)
    {
        for (count = 0; count < 8; count++)
        {
            el = EleSpec[RSPOpC.e].B[count];

            if (el != last)
            {
                sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
                MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_EDX);
                last = el;
            }

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, count);
            MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vd].s16(count), Reg);
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VNE(void)
{
#ifndef CompileVne
    Cheat_r4300iOpcode(&RSPOp::Vector_VNE, "&RSPOp::Vector_VNE");
#else
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    uint32_t flag;
    char Reg[256];
    uint8_t el, del, last = (uint8_t)-1;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveZxVariableToX86regHalf(&m_Flags[0].UHW[1], "&m_Flags[0].UHW[1]", x86_EBX);

    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        flag = 0x101 << (7 - el);
        if (del != el || RSPOpC.rt != RSPOpC.rd)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EDX);

            if (del != last)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_ECX);
                last = del;
            }
            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_EDX, &m_ACCUM[el].HW[1], Reg);
            }

            SubX86RegToX86Reg(x86_EDX, x86_ECX);
            NegateX86reg(x86_EDX);
            SbbX86RegToX86Reg(x86_EDX, x86_EDX);
            AndConstToX86Reg(x86_EDX, flag);
            OrX86RegToX86Reg(x86_EBX, x86_EDX);
        }
        else
        {
            if (bWriteToAccum)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_EDX, &m_ACCUM[el].HW[1], Reg);
            }
        }
    }

    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    MoveX86regToVariable(x86_EBX, &m_Flags[1].UW, "m_Flags[1].UW");

    if (bWriteToDest != false)
    {
        for (el = 0; el < 4; el++)
        {
            sprintf(Reg, "m_Vect[%i].W[%i]", RSPOpC.rd, el);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vs].s32(el), Reg, x86_EDX);

            sprintf(Reg, "m_Vect[%i].W[%i]", RSPOpC.sa, el);
            MoveX86regToVariable(x86_EDX, &m_Vect[RSPOpC.vd].s32(el), Reg);
        }
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VGE_MMX(void)
{
    char Reg[256];

    if ((RSPOpC.rs & 0xF) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    MoveConstToVariable(0, &m_Flags[1].UW, "m_Flags[1].UW");

    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].s16(0), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].s16(4), Reg);
    MmxMoveRegToReg(x86_MM2, x86_MM0);
    MmxMoveRegToReg(x86_MM3, x86_MM1);

    if ((RSPOpC.rs & 0x0f) < 2)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MmxMoveQwordVariableToReg(x86_MM4, &m_Vect[RSPOpC.vt].s16(0), Reg);
        sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
        MmxMoveQwordVariableToReg(x86_MM5, &m_Vect[RSPOpC.vt].s16(4), Reg);
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM4);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM4, x86_MM5);
    }

    MmxCompareGreaterWordRegToReg(x86_MM2, x86_MM4);
    MmxCompareGreaterWordRegToReg(x86_MM3, (RSPOpC.rs & 8) ? x86_MM4 : x86_MM5);

    MmxPandRegToReg(x86_MM0, x86_MM2);
    MmxPandRegToReg(x86_MM1, x86_MM3);
    MmxPandnRegToReg(x86_MM2, x86_MM4);
    MmxPandnRegToReg(x86_MM3, (RSPOpC.rs & 8) ? x86_MM4 : x86_MM5);

    MmxPorRegToReg(x86_MM0, x86_MM2);
    MmxPorRegToReg(x86_MM1, x86_MM3);
    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    return true;
}

void CRSPRecompilerOps::Vector_VGE(void)
{
#ifndef CompileVge
    Cheat_r4300iOpcode(&RSPOp::Vector_VGE, "&RSPOp::Vector_VGE");
#else
    /*
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

	TODO: works ok, but needs careful flag analysis */
    /*	#if defined (DLIST)
	if (bWriteToAccum == false && true == Compile_Vector_VGE_MMX()) {
		return;
	}
	#endif
*/
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    uint8_t * jump[3];
    uint32_t flag;
    char Reg[256];
    uint8_t el, del, last = (uint8_t)-1;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    XorX86RegToX86Reg(x86_EBX, x86_EBX);
    MoveVariableToX86reg(&m_Flags[0].UW, "&m_Flags[0].UW", x86_ESI);
    for (el = 0; el < 8; el++)
    {
        del = EleSpec[RSPOpC.e].B[el];
        flag = 0x101 << (7 - el);
        if (del != el || RSPOpC.rt != RSPOpC.rd)
        {
            if (del != last)
            {
                sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
                MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_ECX);
                last = del;
            }
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
            MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EDX);

            CompX86RegToX86Reg(x86_EDX, x86_ECX);
            JleLabel8("jle", 0);
            jump[0] = (uint8_t *)(RecompPos - 1);

            if (bWriteToAccum || bWriteToDest)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_EDX, &m_ACCUM[el].HW[1], Reg);
            }
            OrConstToX86Reg((flag & 0xFF), x86_EBX);

            JmpLabel8("jmp", 0);
            jump[1] = (uint8_t *)(RecompPos - 1);
            x86_SetBranch8b(jump[0], RecompPos);

            if (bWriteToAccum || bWriteToDest)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
            }

            JneLabel8("jne", 0);
            jump[2] = (uint8_t *)(RecompPos - 1);

            MoveX86RegToX86Reg(x86_ESI, x86_EDI);
            AndConstToX86Reg(x86_EDI, flag);
            SubConstFromX86Reg(x86_EDI, flag);
            ShiftRightSignImmed(x86_EDI, 31);
            AndConstToX86Reg(x86_EDI, (flag & 0xFF));
            OrX86RegToX86Reg(x86_EBX, x86_EDI);

            x86_SetBranch8b(jump[1], RecompPos);
            x86_SetBranch8b(jump[2], RecompPos);
        }
        else
        {
            MoveX86RegToX86Reg(x86_ESI, x86_EDI);
            if (bWriteToAccum || bWriteToDest)
            {
                sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
                MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
            }
            AndConstToX86Reg(x86_EDI, flag);
            SubConstFromX86Reg(x86_EDI, flag);
            ShiftRightSignImmed(x86_EDI, 31);
            AndConstToX86Reg(x86_EDI, (flag & 0xFF));
            OrX86RegToX86Reg(x86_EBX, x86_EDI);
        }
    }

    MoveConstToVariable(0, &m_Flags[0].UW, "m_Flags[0].UW");
    MoveX86regToVariable(x86_EBX, &m_Flags[1].UW, "m_Flags[1].UW");

    if (bWriteToDest != false)
    {
        for (el = 0; el < 8; el += 2)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el + 0);
            MoveVariableToX86regHalf(&m_ACCUM[el].HW[1], Reg, x86_EAX);

            sprintf(Reg, "m_ACCUM[%i].HW[1]", el + 1);
            MoveVariableToX86regHalf(&m_ACCUM[el + 1].HW[1], Reg, x86_ECX);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el + 0);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el + 0), Reg);

            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el + 1);
            MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].s16(el + 1), Reg);
        }
    }
#endif
}

void CRSPRecompilerOps::Vector_VCL(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VCL, "RSPOp::Vector_VCL");
}

void CRSPRecompilerOps::Vector_VCH(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VCH, "RSPOp::Vector_VCH");
}

void CRSPRecompilerOps::Vector_VCR(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_VCR, "RSPOp::Vector_VCR");
}

void CRSPRecompilerOps::Vector_VMRG(void)
{
#ifndef CompileVmrg
    Cheat_r4300iOpcode(&RSPOp::Vector_VMRG, "&RSPOp::Vector_VMRG");
#else
    char Reg[256];
    uint8_t count, el, del;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    MoveVariableToX86reg(&m_Flags[1].UW, "m_Flags[1].UW", x86_EDX);

    for (count = 0; count < 8; count++)
    {
        el = Indx[RSPOpC.e].UB[count];
        del = EleSpec[RSPOpC.e].UB[el];
        CPU_Message("     Iteration: %i", count);

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveZxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);

        TestConstToX86Reg(1 << (7 - el), x86_EDX);
        CondMoveNotEqual(x86_ECX, x86_EAX);
        CondMoveEqual(x86_ECX, x86_EBX);

        if (bWriteToAccum)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[el].HW[1], Reg);
        }
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
        MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].s16(el), Reg);
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VAND_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPandRegToReg(x86_MM0, x86_MM2);
        MmxPandRegToReg(x86_MM1, x86_MM2);
    }
    else if ((RSPOpC.rs & 0xF) < 2)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MmxPandVariableToReg(&m_Vect[RSPOpC.vt].s16(0), Reg, x86_MM0);
        sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
        MmxPandVariableToReg(&m_Vect[RSPOpC.vt].s16(4), Reg, x86_MM1);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPandRegToReg(x86_MM0, x86_MM2);
        MmxPandRegToReg(x86_MM1, x86_MM3);
    }

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VAND(void)
{
#ifndef CompileVand
    Cheat_r4300iOpcode(&RSPOp::Vector_VAND, "RSPOp::Vector_VAND");
#else
    char Reg[256];
    uint8_t el, del, count;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bElement = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VAND_MMX())
            return;
    }

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    for (count = 0; count < 8; count++)
    {
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        CPU_Message("     Iteration: %i", count);

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            AndVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EAX);
        }
        else
        {
            AndX86RegHalfToX86RegHalf(x86_EAX, x86_EBX);
        }

        if (bWriteToDest != false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VNAND_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);
    MmxPcmpeqwRegToReg(x86_MM7, x86_MM7);

    if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPandRegToReg(x86_MM0, x86_MM2);
        MmxPandRegToReg(x86_MM1, x86_MM2);
    }
    else if ((RSPOpC.rs & 0xF) < 2)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MmxPandVariableToReg(&m_Vect[RSPOpC.vt].s16(0), Reg, x86_MM0);
        sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
        MmxPandVariableToReg(&m_Vect[RSPOpC.vt].s16(4), Reg, x86_MM1);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPandRegToReg(x86_MM0, x86_MM2);
        MmxPandRegToReg(x86_MM1, x86_MM3);
    }

    MmxXorRegToReg(x86_MM0, x86_MM7);
    MmxXorRegToReg(x86_MM1, x86_MM7);
    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VNAND(void)
{
#ifndef CompileVnand
    Cheat_r4300iOpcode(&RSPOp::Vector_VNAND, "&RSPOp::Vector_VNAND");
#else
    char Reg[256];
    uint8_t el, del, count;
    bool bWriteToDest = WriteToVectorDest(RSPOpC.sa, CompilePC);
    bool bElement = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VNAND_MMX())
            return;
    }

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    for (count = 0; count < 8; count++)
    {
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        CPU_Message("     Iteration: %i", count);

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            AndVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EAX);
        }
        else
        {
            AndX86RegHalfToX86RegHalf(x86_EAX, x86_EBX);
        }

        NotX86reg(x86_EAX);

        if (bWriteToDest != false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
            MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
        }

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VOR_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

    if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt))
    {
    }
    else if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPorRegToReg(x86_MM0, x86_MM2);
        MmxPorRegToReg(x86_MM1, x86_MM2);
    }
    else if ((RSPOpC.rs & 0xF) < 2)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MmxPorVariableToReg(&m_Vect[RSPOpC.vt].s16(0), Reg, x86_MM0);
        sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
        MmxPorVariableToReg(&m_Vect[RSPOpC.vt].s16(4), Reg, x86_MM1);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPorRegToReg(x86_MM0, x86_MM2);
        MmxPorRegToReg(x86_MM1, x86_MM3);
    }

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VOR(void)
{
#ifndef CompileVor
    Cheat_r4300iOpcode(&RSPOp::Vector_VOR, "RSPOp::Vector_VOR");
#else
    char Reg[256];
    uint8_t el, del, count;
    bool bElement = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VOR_MMX())
            return;
    }

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    for (count = 0; count < 8; count++)
    {
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        CPU_Message("     Iteration: %i", count);

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            OrVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EAX);
        }
        else
        {
            OrX86RegToX86Reg(x86_EAX, x86_EBX);
        }

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
        MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VNOR_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
    MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);
    MmxPcmpeqwRegToReg(x86_MM7, x86_MM7);

    if (RSPOpC.rs & 8)
    {
        RSP_Element2Mmx(x86_MM2);
        MmxPorRegToReg(x86_MM0, x86_MM2);
        MmxPorRegToReg(x86_MM1, x86_MM2);
    }
    else if ((RSPOpC.rs & 0xF) < 2)
    {
        sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
        MmxPorVariableToReg(&m_Vect[RSPOpC.vt].s16(0), Reg, x86_MM0);
        sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
        MmxPorVariableToReg(&m_Vect[RSPOpC.vt].s16(4), Reg, x86_MM1);
    }
    else
    {
        RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
        MmxPorRegToReg(x86_MM0, x86_MM2);
        MmxPorRegToReg(x86_MM1, x86_MM3);
    }

    MmxXorRegToReg(x86_MM0, x86_MM7);
    MmxXorRegToReg(x86_MM1, x86_MM7);
    sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
    sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
    MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VNOR(void)
{
#ifndef CompileVnor
    Cheat_r4300iOpcode(&RSPOp::Vector_VNOR, "&RSPOp::Vector_VNOR");
#else
    char Reg[256];
    uint8_t el, del, count;
    bool bElement = (RSPOpC.rs & 8) ? true : false;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum)
    {
        if (true == Compile_Vector_VNOR_MMX())
            return;
    }

    if (bElement == true)
    {
        del = (RSPOpC.rs & 0x07) ^ 7;
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EBX);
    }

    for (count = 0; count < 8; count++)
    {
        el = Indx[RSPOpC.e].B[count];
        del = EleSpec[RSPOpC.e].B[el];

        CPU_Message("     Iteration: %i", count);

        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rd, el);
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vs].s16(el), Reg, x86_EAX);

        if (bElement == false)
        {
            sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.rt, del);
            OrVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(del), Reg, x86_EAX);
        }
        else
        {
            OrX86RegToX86Reg(x86_EAX, x86_EBX);
        }

        NotX86reg(x86_EAX);

        if (bWriteToAccum != false)
        {
            sprintf(Reg, "m_ACCUM[%i].HW[1]", el);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[el].HW[1], Reg);
        }
        sprintf(Reg, "m_Vect[%i].HW[%i]", RSPOpC.sa, el);
        MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
    }
#endif
}

bool CRSPRecompilerOps::Compile_Vector_VXOR_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt))
    {
        static uint32_t VXOR_DynaRegCount = 0;
        MmxXorRegToReg(VXOR_DynaRegCount, VXOR_DynaRegCount);

        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(VXOR_DynaRegCount, &m_Vect[RSPOpC.vd].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(VXOR_DynaRegCount, &m_Vect[RSPOpC.vd].u16(4), Reg);
        VXOR_DynaRegCount = (VXOR_DynaRegCount + 1) & 7;
    }
    else
    {
        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
        MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
        MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);

        if (RSPOpC.rs & 8)
        {
            RSP_Element2Mmx(x86_MM2);
            MmxXorRegToReg(x86_MM0, x86_MM2);
            MmxXorRegToReg(x86_MM1, x86_MM2);
        }
        else if ((RSPOpC.rs & 0xF) < 2)
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM2, &m_Vect[RSPOpC.vt].s16(0), Reg);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM3, &m_Vect[RSPOpC.vt].s16(4), Reg);

            MmxXorRegToReg(x86_MM0, x86_MM2);
            MmxXorRegToReg(x86_MM1, x86_MM3);
        }
        else
        {
            RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
            MmxXorRegToReg(x86_MM0, x86_MM2);
            MmxXorRegToReg(x86_MM1, x86_MM3);
        }

        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);
    }

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VXOR(void)
{
#ifdef CompileVxor
    char Reg[256];
    uint32_t count;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum || ((RSPOpC.rs & 0xF) < 2 && RSPOpC.rd == RSPOpC.rt))
    {
        if (true == Compile_Vector_VXOR_MMX())
        {
            if (bWriteToAccum)
            {
                XorX86RegToX86Reg(x86_EAX, x86_EAX);
                for (count = 0; count < 8; count++)
                {
                    sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
                    MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[count].HW[1], Reg);
                }
            }
            return;
        }
    }
#endif

    Cheat_r4300iOpcodeNoMessage(&RSPOp::Vector_VXOR, "RSPOp::Vector_VXOR");
}

bool CRSPRecompilerOps::Compile_Vector_VNXOR_MMX(void)
{
    char Reg[256];

    // Do our MMX checks here
    if (!IsMmxEnabled)
        return false;
    if ((RSPOpC.rs & 0x0f) >= 2 && !(RSPOpC.rs & 8) && IsMmx2Enabled == false)
        return false;

    if ((RSPOpC.rs & 0xF) < 2 && (RSPOpC.rd == RSPOpC.rt))
    {
        static uint32_t VNXOR_DynaRegCount = 0;
        MmxPcmpeqwRegToReg(VNXOR_DynaRegCount, VNXOR_DynaRegCount);

        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(VNXOR_DynaRegCount, &m_Vect[RSPOpC.vd].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(VNXOR_DynaRegCount, &m_Vect[RSPOpC.vd].u16(4), Reg);
        VNXOR_DynaRegCount = (VNXOR_DynaRegCount + 1) & 7;
    }
    else
    {
        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.rd);
        MmxMoveQwordVariableToReg(x86_MM0, &m_Vect[RSPOpC.vs].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.rd);
        MmxMoveQwordVariableToReg(x86_MM1, &m_Vect[RSPOpC.vs].u16(4), Reg);
        MmxPcmpeqwRegToReg(x86_MM7, x86_MM7);

        if (RSPOpC.rs & 8)
        {
            RSP_Element2Mmx(x86_MM2);
            MmxXorRegToReg(x86_MM0, x86_MM2);
            MmxXorRegToReg(x86_MM1, x86_MM2);
        }
        else if ((RSPOpC.rs & 0xF) < 2)
        {
            sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM2, &m_Vect[RSPOpC.vt].s16(0), Reg);
            sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
            MmxMoveQwordVariableToReg(x86_MM3, &m_Vect[RSPOpC.vt].s16(4), Reg);

            MmxXorRegToReg(x86_MM0, x86_MM2);
            MmxXorRegToReg(x86_MM1, x86_MM3);
        }
        else
        {
            RSP_MultiElement2Mmx(x86_MM2, x86_MM3);
            MmxXorRegToReg(x86_MM0, x86_MM2);
            MmxXorRegToReg(x86_MM1, x86_MM3);
        }

        MmxXorRegToReg(x86_MM0, x86_MM7);
        MmxXorRegToReg(x86_MM1, x86_MM7);
        sprintf(Reg, "m_Vect[%i].UHW[0]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(x86_MM0, &m_Vect[RSPOpC.vd].u16(0), Reg);
        sprintf(Reg, "m_Vect[%i].UHW[4]", RSPOpC.sa);
        MmxMoveQwordRegToVariable(x86_MM1, &m_Vect[RSPOpC.vd].u16(4), Reg);
    }

    if (!IsNextInstructionMmx(CompilePC))
        MmxEmptyMultimediaState();

    return true;
}

void CRSPRecompilerOps::Vector_VNXOR(void)
{
#ifdef CompileVnxor
    char Reg[256];
    uint32_t count;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (!bWriteToAccum || ((RSPOpC.rs & 0xF) < 2 && RSPOpC.rd == RSPOpC.rt))
    {
        if (true == Compile_Vector_VNXOR_MMX())
        {
            if (bWriteToAccum)
            {
                OrConstToX86Reg(0xFFFFFFFF, x86_EAX);
                for (count = 0; count < 8; count++)
                {
                    sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
                    MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[count].HW[1], Reg);
                }
            }
            return;
        }
    }
#endif

    Cheat_r4300iOpcode(&RSPOp::Vector_VNXOR, "RSPOp::Vector_VNXOR");
}

void CRSPRecompilerOps::Vector_VRCP(void)
{
#ifndef CompileVrcp
    Cheat_r4300iOpcode(&RSPOp::Vector_VRCP, "&RSPOp::Vector_VRCP");
#else
    char Reg[256];
    uint8_t count, el, last;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    uint32_t * end = NULL;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    el = EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)];
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
    MoveSxVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(el), Reg, x86_ESI);
    MoveConstToX86reg(0x7FFFFFFF, x86_EAX);
    TestX86RegToX86Reg(x86_ESI, x86_ESI);
    MoveX86RegToX86Reg(x86_ESI, x86_EDI);
    JeLabel32("Done", 0);
    end = (uint32_t *)(RecompPos - 4);

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

    if (bWriteToAccum != false)
    {
        last = (uint8_t)-1;
        for (count = 0; count < 8; count++)
        {
            el = EleSpec[RSPOpC.e].B[count];

            if (el != last)
            {
                sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
                MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_ECX);
                last = el;
            }

            sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
            MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[count].HW[1], Reg);
        }
    }

    el = 7 - (RSPOpC.rd & 0x7);
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.sa, el);
    MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
    MoveX86regToVariable(x86_EAX, &RecpResult.W, "RecpResult.W");
#endif
}

void CRSPRecompilerOps::Vector_VRCPL(void)
{
#ifndef CompileVrcpl
    Cheat_r4300iOpcode(&RSPOp::Vector_VRCPL, "RSPOp::Vector_VRCPL");
#else
    char Reg[256];
    uint8_t count, el, last;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);
    uint32_t * end = NULL;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    el = EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)];
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
    MoveVariableToX86reg(&Recp.W, "Recp.W", x86_ESI);
    OrVariableToX86regHalf(&m_Vect[RSPOpC.vt].s16(el), Reg, x86_ESI);

    MoveConstToX86reg(0x7FFFFFFF, x86_EAX);
    TestX86RegToX86Reg(x86_ESI, x86_ESI);
    MoveX86RegToX86Reg(x86_ESI, x86_EDI);
    JeLabel32("Done", 0);
    end = (uint32_t *)(RecompPos - 4);

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

    if (bWriteToAccum != false)
    {
        last = (uint8_t)-1;
        for (count = 0; count < 8; count++)
        {
            el = EleSpec[RSPOpC.e].B[count];

            if (el != last)
            {
                sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
                MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_ECX);
                last = el;
            }

            sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
            MoveX86regHalfToVariable(x86_ECX, &m_ACCUM[count].HW[1], Reg);
        }
    }

    el = 7 - (RSPOpC.rd & 0x7);
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.sa, el);
    MoveX86regHalfToVariable(x86_EAX, &m_Vect[RSPOpC.vd].s16(el), Reg);
    MoveX86regToVariable(x86_EAX, &RecpResult.W, "RecpResult.W");
#endif
}

void CRSPRecompilerOps::Vector_VRCPH(void)
{
#ifndef CompileVrcph
    Cheat_r4300iOpcode(&RSPOp::Vector_VRCPH, "&RSPOp::Vector_VRCPH");
#else
    char Reg[256];
    uint8_t count, el, last;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    el = EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)];
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
    MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_EDX);
    MoveX86regHalfToVariable(x86_EDX, &Recp.UHW[1], "Recp.UHW[1]");

    MoveVariableToX86regHalf(&RecpResult.UHW[1], "RecpResult.UHW[1]", x86_ECX);

    if (bWriteToAccum != false)
    {
        last = (uint8_t)-1;
        for (count = 0; count < 8; count++)
        {
            el = EleSpec[RSPOpC.e].B[count];

            if (el != last)
            {
                sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
                MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_EAX);
                last = el;
            }

            sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[count].HW[1], Reg);
        }
    }

    el = 7 - (RSPOpC.rd & 0x7);
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.sa, el);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].u16(el), Reg);
#endif
}

void CRSPRecompilerOps::Vector_VMOV(void)
{
#ifndef CompileVmov
    Cheat_r4300iOpcode(&RSPOp::Vector_VMOV, "&RSPOp::Vector_VMOV");
#else
    char Reg[256];
    uint8_t el, count;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (bWriteToAccum)
    {
        for (count = 0; count < 8; count++)
        {
            sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, EleSpec[RSPOpC.e].B[count]);
            MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(EleSpec[RSPOpC.e].B[count]), Reg, x86_EAX);
            sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[count].HW[1], Reg);
        }
    }

    el = EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)];
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);

    MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_ECX);

    el = 7 - (RSPOpC.rd & 0x7);
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.sa, el);

    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].u16(el), Reg);
#endif
}

void CRSPRecompilerOps::Vector_VRSQ(void)
{
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    Cheat_r4300iOpcodeNoMessage(&RSPOp::Vector_VRSQ, "RSPOp::Vector_VRSQ");
}

void CRSPRecompilerOps::Vector_VRSQL(void)
{
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    Cheat_r4300iOpcodeNoMessage(&RSPOp::Vector_VRSQL, "RSPOp::Vector_VRSQL");
}

void CRSPRecompilerOps::Vector_VRSQH(void)
{
#ifndef CompileVrsqh
    Cheat_r4300iOpcode(&RSPOp::Vector_VRSQH, "RSPOp::Vector_VRSQH");
#else
    char Reg[256];
    uint8_t count, el, last;
    bool bWriteToAccum = WriteToAccum(Low16BitAccum, CompilePC);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    el = EleSpec[RSPOpC.e].B[(RSPOpC.rd & 0x7)];
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
    MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_EDX);
    MoveX86regHalfToVariable(x86_EDX, &SQroot.UHW[1], "SQroot.UHW[1]");

    MoveVariableToX86regHalf(&SQrootResult.UHW[1], "SQrootResult.UHW[1]", x86_ECX);

    if (bWriteToAccum != false)
    {
        last = (uint8_t)-1;
        for (count = 0; count < 8; count++)
        {
            el = EleSpec[RSPOpC.e].B[count];

            if (el != last)
            {
                sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.rt, el);
                MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].u16(el), Reg, x86_EAX);
                last = el;
            }

            sprintf(Reg, "m_ACCUM[%i].HW[1]", count);
            MoveX86regHalfToVariable(x86_EAX, &m_ACCUM[count].HW[1], Reg);
        }
    }

    el = 7 - (RSPOpC.rd & 0x7);
    sprintf(Reg, "m_Vect[%i].UHW[%i]", RSPOpC.sa, el);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vd].u16(el), Reg);
#endif
}

void CRSPRecompilerOps::Vector_VNOOP(void)
{
}

void CRSPRecompilerOps::Vector_Reserved(void)
{
    Cheat_r4300iOpcode(&RSPOp::Vector_Reserved, "&RSPOp::Vector_Reserved");
}

// LC2 functions

void CRSPRecompilerOps::Opcode_LBV(void)
{
#ifndef CompileLbv
    Cheat_r4300iOpcode(&RSPOp::LBV, "RSPOp::LBV");
#else
    char Reg[256];
    int offset = RSPOpC.voffset << 0;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
        AddConstToX86Reg(x86_EBX, offset);

    AndConstToX86Reg(x86_EBX, 0x0FFF);
    XorConstToX86Reg(x86_EBX, 3);
    MoveN64MemToX86regByte(x86_ECX, x86_EBX);
    sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - RSPOpC.del);
    MoveX86regByteToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - RSPOpC.del)), Reg);
#endif
}

void CRSPRecompilerOps::Opcode_LSV(void)
{
#ifndef CompileLsv
    Cheat_r4300iOpcode(&RSPOp::LSV, "RSPOp::LSV");
#else
    if (RSPOpC.del > 14)
    {
        Cheat_r4300iOpcodeNoMessage(&RSPOp::LSV, "RSPOp::LSV");
        return;
    }
    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    char Reg[256];
    int offset = (RSPOpC.voffset << 1);
    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if ((Addr & 1) != 0)
        {
            sprintf(Reg, "DMEM + %Xh", (Addr + 0) ^ 3);
            MoveVariableToX86regByte(RSPInfo.DMEM + ((Addr + 0) ^ 3), Reg, x86_ECX);
            sprintf(Reg, "DMEM + %Xh", (Addr + 1) ^ 3);
            MoveVariableToX86regByte(RSPInfo.DMEM + ((Addr + 1) ^ 3), Reg, x86_EDX);

            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
            MoveX86regByteToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 0))), Reg);
            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
            MoveX86regByteToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg);
        }
        else
        {
            sprintf(Reg, "DMEM + %Xh", Addr ^ 2);
            MoveVariableToX86regHalf(RSPInfo.DMEM + (Addr ^ 2), Reg, x86_EDX);
            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
            MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg);
        }
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    AndConstToX86Reg(x86_EBX, 0x0FFF);

    if (Compiler.bAlignVector == true)
    {
        XorConstToX86Reg(x86_EBX, 2);
        MoveN64MemToX86regHalf(x86_ECX, x86_EBX);
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
        MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg);
    }
    else
    {
        LeaSourceAndOffset(x86_EAX, x86_EBX, 1);
        AndConstToX86Reg(x86_EAX, 0x0FFF);
        XorConstToX86Reg(x86_EBX, 3);
        XorConstToX86Reg(x86_EAX, 3);

        MoveN64MemToX86regByte(x86_ECX, x86_EBX);
        MoveN64MemToX86regByte(x86_EDX, x86_EAX);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
        MoveX86regByteToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 0))), Reg);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
        MoveX86regByteToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg);
    }
#endif
}

void CRSPRecompilerOps::Opcode_LLV(void)
{
#ifndef CompileLlv
    Cheat_r4300iOpcode(&RSPOp::LLV, "RSPOp::LLV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 2);
    uint8_t * Jump[2];

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if ((RSPOpC.del & 0x3) != 0)
    {
        Cheat_r4300iOpcode(&RSPOp::LLV, "RSPOp::LLV");
        return;
    }

    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if ((Addr & 3) != 0)
        {
            CompilerWarning("Unaligned LLV at constant address");
            Cheat_r4300iOpcodeNoMessage(&RSPOp::LLV, "RSPOp::LLV");
            return;
        }

        sprintf(Reg, "DMEM + %Xh", Addr);
        MoveVariableToX86reg(RSPInfo.DMEM + Addr, Reg, x86_EAX);
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
        MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 4)), Reg);
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0) AddConstToX86Reg(x86_EBX, offset);

    TestConstToX86Reg(3, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    // Unaligned

    CompilerToggleBuffer();

    CPU_Message("   Unaligned:");
    *((uint32_t *)(Jump[0])) = (uint32_t)(RecompPos - Jump[0] - 4);
    Cheat_r4300iOpcodeNoMessage(&RSPOp::LLV, "RSPOp::LLV");
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;

    CompilerToggleBuffer();

    // Aligned

    AndConstToX86Reg(x86_EBX, 0x0fff);
    MoveN64MemToX86reg(x86_EAX, x86_EBX);
    // Because of byte swapping this swizzle works nicely
    sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
    MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 4)), Reg);

    CPU_Message("   Done:");
    *((uint32_t *)(Jump[1])) = (uint32_t)(RecompPos - Jump[1] - 4);
#endif
}

void CRSPRecompilerOps::Opcode_LDV(void)
{
#ifndef CompileLdv
    Cheat_r4300iOpcode(&RSPOp::LDV, "RSPOp::LDV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 3), length;
    uint8_t *Jump[2], *LoopEntry;

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    //if ((RSPOpC.del & 0x7) != 0) {
    //	rsp_UnknownOpcode();
    //	return;
    //}
    if ((RSPOpC.del & 0x3) != 0)
    {
        CompilerWarning(stdstr_f("LDV's element = %X, PC = %04X", RSPOpC.del, CompilePC).c_str());
        Cheat_r4300iOpcodeNoMessage(&RSPOp::LDV, "RSPOp::LDV");
        return;
    }

    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if ((Addr & 3) != 0)
        {
            CompilerWarning(stdstr_f("Unaligned LDV at constant address PC = %04X", CompilePC).c_str());
            Cheat_r4300iOpcodeNoMessage(&RSPOp::LDV, "RSPOp::LDV");
            return;
        }

        sprintf(Reg, "DMEM + %Xh", Addr);
        MoveVariableToX86reg(RSPInfo.DMEM + Addr + 0, Reg, x86_EAX);
        sprintf(Reg, "DMEM + %Xh", ((Addr + 4) & 0xFFF));
        MoveVariableToX86reg(RSPInfo.DMEM + ((Addr + 4) & 0xFFF), Reg, x86_ECX);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
        MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 4)), Reg);
        if (RSPOpC.del != 12)
        {
            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
            MoveX86regToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 8)), Reg);
        }
        return;
    }
    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    AndConstToX86Reg(x86_EBX, 0x0fff);
    TestConstToX86Reg(7, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();
    CPU_Message("   Unaligned:");
    x86_SetBranch32b(Jump[0], RecompPos);
    Cheat_r4300iOpcodeNoMessage(&RSPOp::LDV, "RSPOp::LDV");
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;
    CompilerToggleBuffer();

    MoveN64MemToX86reg(x86_EAX, x86_EBX);
    MoveN64MemDispToX86reg(x86_ECX, x86_EBX, 4);

    // Because of byte swapping this swizzle works nicely
    sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
    MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 4)), Reg);
    if (RSPOpC.del != 12)
    {
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 8);
        MoveX86regToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 8)), Reg);
    }
    CPU_Message("   Done:");
    x86_SetBranch32b(Jump[1], RecompPos);
#endif
}

void CRSPRecompilerOps::Opcode_LQV(void)
{
#ifndef CompileLqv
    Cheat_r4300iOpcode(&RSPOp::LQV, "RSPOp::LQV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 4);
    uint8_t * Jump[2];

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.del != 0)
    {
        Cheat_r4300iOpcode(&RSPOp::LQV, "RSPOp::LQV");
        return;
    }

    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if (Addr & 15)
        {
            CompilerWarning(stdstr_f("Unaligned LQV at constant address PC = %04X", CompilePC).c_str());
            Cheat_r4300iOpcodeNoMessage(&RSPOp::LQV, "RSPOp::LQV");
            return;
        }

        // Aligned store

        if (IsSseEnabled == false)
        {
            sprintf(Reg, "DMEM+%Xh+0", Addr);
            MoveVariableToX86reg(RSPInfo.DMEM + Addr + 0, Reg, x86_EAX);
            sprintf(Reg, "DMEM+%Xh+4", Addr);
            MoveVariableToX86reg(RSPInfo.DMEM + Addr + 4, Reg, x86_EBX);
            sprintf(Reg, "DMEM+%Xh+8", Addr);
            MoveVariableToX86reg(RSPInfo.DMEM + Addr + 8, Reg, x86_ECX);
            sprintf(Reg, "DMEM+%Xh+C", Addr);
            MoveVariableToX86reg(RSPInfo.DMEM + Addr + 12, Reg, x86_EDX);

            sprintf(Reg, "m_Vect[%i].B[12]", RSPOpC.rt);
            MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vt].s8(12), Reg);
            sprintf(Reg, "m_Vect[%i].B[8]", RSPOpC.rt);
            MoveX86regToVariable(x86_EBX, &m_Vect[RSPOpC.vt].s8(8), Reg);
            sprintf(Reg, "m_Vect[%i].B[4]", RSPOpC.rt);
            MoveX86regToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8(4), Reg);
            sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
            MoveX86regToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s8(0), Reg);
        }
        else
        {
            sprintf(Reg, "DMEM+%Xh", Addr);
            SseMoveUnalignedVariableToReg(RSPInfo.DMEM + Addr, Reg, x86_XMM0);
            SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
            sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
            SseMoveAlignedRegToVariable(x86_XMM0, &m_Vect[RSPOpC.vt].s8(0), Reg);
        }
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    TestConstToX86Reg(15, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();
    CPU_Message("   Unaligned:");
    x86_SetBranch32b(Jump[0], RecompPos);

    Cheat_r4300iOpcodeNoMessage(&RSPOp::LQV, "RSPOp::LQV");
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;
    CompilerToggleBuffer();

    AndConstToX86Reg(x86_EBX, 0x0fff);
    if (IsSseEnabled == false)
    {
        MoveN64MemDispToX86reg(x86_EAX, x86_EBX, 0);
        MoveN64MemDispToX86reg(x86_ECX, x86_EBX, 4);
        MoveN64MemDispToX86reg(x86_EDX, x86_EBX, 8);
        MoveN64MemDispToX86reg(x86_EDI, x86_EBX, 12);

        sprintf(Reg, "m_Vect[%i].B[12]", RSPOpC.rt);
        MoveX86regToVariable(x86_EAX, &m_Vect[RSPOpC.vt].s8(12), Reg);
        sprintf(Reg, "m_Vect[%i].B[8]", RSPOpC.rt);
        MoveX86regToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s8(8), Reg);
        sprintf(Reg, "m_Vect[%i].B[4]", RSPOpC.rt);
        MoveX86regToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s8(4), Reg);
        sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
        MoveX86regToVariable(x86_EDI, &m_Vect[RSPOpC.vt].s8(0), Reg);
    }
    else
    {
        SseMoveUnalignedN64MemToReg(x86_XMM0, x86_EBX);
        SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
        sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
        SseMoveAlignedRegToVariable(x86_XMM0, &m_Vect[RSPOpC.vt].s8(0), Reg);
    }
    CPU_Message("   Done:");
    x86_SetBranch32b((uint32_t *)Jump[1], (uint32_t *)RecompPos);
#endif
}

void CRSPRecompilerOps::Opcode_LRV(void)
{
#ifndef CompileLrv
    Cheat_r4300iOpcode(&RSPOp::LRV, "RSPOp::LRV");
#else
    int offset = (RSPOpC.voffset << 4);
    uint8_t *Loop, *Jump[2];

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (RSPOpC.del != 0)
    {
        Cheat_r4300iOpcode(&RSPOp::LRV, "RSPOp::LRV");
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0) AddConstToX86Reg(x86_EBX, offset);

    if (Compiler.bAlignVector == false)
    {
        TestConstToX86Reg(1, x86_EBX);
        JneLabel32("Unaligned", 0);
        Jump[0] = RecompPos - 4;

        // Unaligned
        CompilerToggleBuffer();

        CPU_Message(" Unaligned:");
        x86_SetBranch32b(Jump[0], RecompPos);

        Cheat_r4300iOpcodeNoMessage(&RSPOp::LRV, "RSPOp::LRV");
        JmpLabel32("Done", 0);
        Jump[1] = RecompPos - 4;

        CompilerToggleBuffer();
    }

    // Aligned
    MoveX86RegToX86Reg(x86_EBX, x86_EAX);
    AndConstToX86Reg(x86_EAX, 0x0F);
    AndConstToX86Reg(x86_EBX, 0x0ff0);

    MoveX86RegToX86Reg(x86_EAX, x86_ECX);
    ShiftRightUnsignImmed(x86_ECX, 1);

    JeLabel8("Done", 0);
    Jump[0] = RecompPos - 1;
    /*
	DecX86reg(x86_EAX);
	LeaSourceAndOffset(x86_EAX, x86_EAX, (size_t)&m_Vect[RSPOpC.vt].s8(0));
	DecX86reg(x86_EAX);
*/
    AddConstToX86Reg(x86_EAX, ((size_t)&m_Vect[RSPOpC.vt].u8(0)) - 2);

    CPU_Message("   Loop:");
    Loop = RecompPos;

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    XorConstToX86Reg(x86_ESI, 2);
    MoveN64MemToX86regHalf(x86_EDX, x86_ESI);
    MoveX86regHalfToX86regPointer(x86_EDX, x86_EAX);

    AddConstToX86Reg(x86_EBX, 2);   // DMEM pointer
    SubConstFromX86Reg(x86_EAX, 2); // Vector pointer
    DecX86reg(x86_ECX);             // Loop counter
    JneLabel8("Loop", 0);
    x86_SetBranch8b(RecompPos - 1, Loop);

    if (Compiler.bAlignVector == false)
    {
        CPU_Message("   Done:");
        x86_SetBranch32b((uint32_t *)Jump[1], (uint32_t *)RecompPos);
    }

    x86_SetBranch8b(Jump[0], RecompPos);
#endif
}

void CRSPRecompilerOps::Opcode_LPV(void)
{
#ifndef CompileLpv
    Cheat_r4300iOpcode(&RSPOp::LPV, "RSPOp::LPV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 3);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 0) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 1) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 8);
    ShiftLeftSignImmed(x86_EDX, 8);

    sprintf(Reg, "m_Vect[%i].HW[7]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(7), Reg);
    sprintf(Reg, "m_Vect[%i].HW[6]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(6), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 2) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 3) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 8);
    ShiftLeftSignImmed(x86_EDX, 8);

    sprintf(Reg, "m_Vect[%i].HW[5]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(5), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(4), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 4) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 5) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 8);
    ShiftLeftSignImmed(x86_EDX, 8);

    sprintf(Reg, "m_Vect[%i].HW[3]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(3), Reg);
    sprintf(Reg, "m_Vect[%i].HW[2]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(2), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 6) & 0xF);
    AddConstToX86Reg(x86_EBX, (0x10 - RSPOpC.del + 7) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EBX, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EBX, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EBX);

    ShiftLeftSignImmed(x86_ECX, 8);
    ShiftLeftSignImmed(x86_EDX, 8);

    sprintf(Reg, "m_Vect[%i].HW[1]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(1), Reg);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(0), Reg);
#endif
}

void CRSPRecompilerOps::Opcode_LUV(void)
{
#ifndef CompileLuv
    Cheat_r4300iOpcode(&RSPOp::LUV, "RSPOp::LUV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 3);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 0) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 1) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[7]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(7), Reg);
    sprintf(Reg, "m_Vect[%i].HW[6]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(6), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 2) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 3) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[5]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(5), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(4), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 4) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 5) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[3]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(3), Reg);
    sprintf(Reg, "m_Vect[%i].HW[2]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(2), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 6) & 0xF);
    AddConstToX86Reg(x86_EBX, (0x10 - RSPOpC.del + 7) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EBX, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EBX, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EBX);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[1]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(1), Reg);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(0), Reg);
#endif
}

void CRSPRecompilerOps::Opcode_LHV(void)
{
#ifndef CompileLhv
    Cheat_r4300iOpcode(&RSPOp::LHV, "RSPOp::LHV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 4);

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 0) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 2) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[7]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(7), Reg);
    sprintf(Reg, "m_Vect[%i].HW[6]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(6), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 4) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 6) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[5]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(5), Reg);
    sprintf(Reg, "m_Vect[%i].HW[4]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(4), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);
    MoveX86RegToX86Reg(x86_EBX, x86_EDI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 8) & 0xF);
    AddConstToX86Reg(x86_EDI, (0x10 - RSPOpC.del + 10) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EDI, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EDI, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EDI);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[3]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(3), Reg);
    sprintf(Reg, "m_Vect[%i].HW[2]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(2), Reg);

    MoveX86RegToX86Reg(x86_EBX, x86_ESI);

    AddConstToX86Reg(x86_ESI, (0x10 - RSPOpC.del + 12) & 0xF);
    AddConstToX86Reg(x86_EBX, (0x10 - RSPOpC.del + 14) & 0xF);

    XorConstToX86Reg(x86_ESI, 3);
    XorConstToX86Reg(x86_EBX, 3);

    AndConstToX86Reg(x86_ESI, 0x0fff);
    AndConstToX86Reg(x86_EBX, 0x0fff);

    MoveZxN64MemToX86regByte(x86_ECX, x86_ESI);
    MoveZxN64MemToX86regByte(x86_EDX, x86_EBX);

    ShiftLeftSignImmed(x86_ECX, 7);
    ShiftLeftSignImmed(x86_EDX, 7);

    sprintf(Reg, "m_Vect[%i].HW[1]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_ECX, &m_Vect[RSPOpC.vt].s16(1), Reg);
    sprintf(Reg, "m_Vect[%i].HW[0]", RSPOpC.rt);
    MoveX86regHalfToVariable(x86_EDX, &m_Vect[RSPOpC.vt].s16(0), Reg);
#endif
}

void CRSPRecompilerOps::Opcode_LFV(void)
{
    Cheat_r4300iOpcode(&RSPOp::LFV, "RSPOp::LFV");
}

void CRSPRecompilerOps::Opcode_LWV(void)
{
    Cheat_r4300iOpcode(&RSPOp::LWV, "RSPOp::LWV");
}

void CRSPRecompilerOps::Opcode_LTV(void)
{
    Cheat_r4300iOpcode(&RSPOp::LTV, "RSPOp::LTV");
}

// SC2 functions

void CRSPRecompilerOps::Opcode_SBV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SBV, "RSPOp::SBV");
}

void CRSPRecompilerOps::Opcode_SSV(void)
{
#ifndef CompileSsv
    Cheat_r4300iOpcode(&RSPOp::SSV, "RSPOp::SSV");
#else
    char Reg[256];
    int offset = (RSPOpC.voffset << 1);

    if (RSPOpC.del > 14)
    {
        Cheat_r4300iOpcode(&RSPOp::SSV, "RSPOp::SSV");
        return;
    }

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if ((Addr & 1) != 0)
        {
            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
            MoveVariableToX86regByte(&m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 0))), Reg, x86_ECX);
            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
            MoveVariableToX86regByte(&m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg, x86_EDX);

            sprintf(Reg, "DMEM + %Xh", (Addr + 0) ^ 3);
            MoveX86regByteToVariable(x86_ECX, RSPInfo.DMEM + ((Addr + 0) ^ 3), Reg);
            sprintf(Reg, "DMEM + %Xh", (Addr + 1) ^ 3);
            MoveX86regByteToVariable(x86_EDX, RSPInfo.DMEM + ((Addr + 1) ^ 3), Reg);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
            MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg, x86_ECX);
            sprintf(Reg, "DMEM + %Xh", Addr ^ 2);
            MoveX86regHalfToVariable(x86_ECX, RSPInfo.DMEM + (Addr ^ 2), Reg);
        }
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0) AddConstToX86Reg(x86_EBX, offset);
    AndConstToX86Reg(x86_EBX, 0x0FFF);

    if (Compiler.bAlignVector == true)
    {
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
        MoveVariableToX86regHalf(&m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg, x86_ECX);
        XorConstToX86Reg(x86_EBX, 2);
        MoveX86regHalfToN64Mem(x86_ECX, x86_EBX);
    }
    else
    {
        LeaSourceAndOffset(x86_EAX, x86_EBX, 1);
        XorConstToX86Reg(x86_EBX, 3);
        XorConstToX86Reg(x86_EAX, 3);

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 0));
        MoveVariableToX86regByte(&m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 0))), Reg, x86_ECX);
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 15 - (RSPOpC.del + 1));
        MoveVariableToX86regByte(&m_Vect[RSPOpC.vt].s8((uint8_t)(15 - (RSPOpC.del + 1))), Reg, x86_EDX);

        MoveX86regByteToN64Mem(x86_ECX, x86_EBX);
        MoveX86regByteToN64Mem(x86_EDX, x86_EAX);
    }
#endif
}

void CRSPRecompilerOps::Opcode_SLV(void)
{
#ifndef CompileSlv
    Cheat_r4300iOpcode(&RSPOp::SLV, "RSPOp::SLV");
#else
    if (RSPOpC.del > 12)
    {
        Cheat_r4300iOpcodeNoMessage(&RSPOp::SLV, "RSPOp::SLV");
        return;
    }

    char Reg[256];
    int offset = (RSPOpC.voffset << 2);
    uint8_t * Jump[2];

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;
        if ((Addr & 3) != 0 || RSPOpC.del > 12)
        {
            Cheat_r4300iOpcodeNoMessage(&RSPOp::SLV, "RSPOp::SLV");
            return;
        }

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
        MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 4)), Reg, x86_EAX);
        sprintf(Reg, "DMEM + %Xh", Addr);
        MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr, Reg);
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0) AddConstToX86Reg(x86_EBX, offset);

    TestConstToX86Reg(3, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    // Unaligned

    CompilerToggleBuffer();

    CPU_Message("   Unaligned:");
    *((uint32_t *)(Jump[0])) = (uint32_t)(RecompPos - Jump[0] - 4);
    Cheat_r4300iOpcodeNoMessage(&RSPOp::SLV, "RSPOp::SLV");
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;

    CompilerToggleBuffer();

    // Aligned

    // Because of byte swapping this swizzle works nicely
    sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, 16 - RSPOpC.del - 4);
    MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8((uint8_t)(16 - RSPOpC.del - 4)), Reg, x86_EAX);

    AndConstToX86Reg(x86_EBX, 0x0fff);
    MoveX86regToN64Mem(x86_EAX, x86_EBX);

    CPU_Message("   Done:");
    *((uint32_t *)(Jump[1])) = (uint32_t)(RecompPos - Jump[1] - 4);
#endif
}

void CRSPRecompilerOps::Opcode_SDV(void)
{
#ifndef CompileSdv
    Cheat_r4300iOpcode(&RSPOp::SDV, "RSPOp::SDV");
#else
    if (RSPOpC.del > 8)
    {
        Cheat_r4300iOpcodeNoMessage(&RSPOp::SDV, "RSPOp::SDV");
        return;
    }
    char Reg[256];
    int offset = (RSPOpC.voffset << 3);
    uint8_t *Jump[2], *LoopEntry;

    //if ((RSPOpC.del & 0x7) != 0) {
    //	rsp_UnknownOpcode();
    //	return;
    //}

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if ((Addr & 3) != 0)
        {
            CompilerWarning(stdstr_f("Unaligned SDV at constant address PC = %04X", CompilePC).c_str());
            Cheat_r4300iOpcodeNoMessage(&RSPOp::SDV, "RSPOp::SDV");
            return;
        }

        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 4) & 0xF);
        MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8((16 - RSPOpC.del - 4) & 0xF), Reg, x86_EAX);
        sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 8) & 0xF);
        MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8((16 - RSPOpC.del - 8) & 0xF), Reg, x86_EBX);

        sprintf(Reg, "DMEM + %Xh", Addr);
        MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr, Reg);
        sprintf(Reg, "DMEM + %Xh", Addr + 4);
        MoveX86regToVariable(x86_EBX, RSPInfo.DMEM + Addr + 4, Reg);
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    AndConstToX86Reg(x86_EBX, 0x0fff);
    TestConstToX86Reg(3, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();
    CPU_Message("   Unaligned:");
    x86_SetBranch32b((uint32_t *)Jump[0], (uint32_t *)RecompPos);

    sprintf(Reg, "m_Vect[%i].UB[%i]", RSPOpC.rt, 15 - RSPOpC.del);
    MoveOffsetToX86reg((size_t)&m_Vect[RSPOpC.vt].u8((uint8_t)(15 - RSPOpC.del)), Reg, x86_EDI);
    MoveConstToX86reg(8, x86_ECX);

    CPU_Message("   Loop:");
    LoopEntry = RecompPos;
    MoveX86RegToX86Reg(x86_EBX, x86_EAX);
    XorConstToX86Reg(x86_EAX, 3);
    MoveX86regPointerToX86regByte(x86_EDX, x86_EDI);
    MoveX86regByteToN64Mem(x86_EDX, x86_EAX);
    IncX86reg(x86_EBX); // Address constant
    DecX86reg(x86_EDI); // Vector pointer
    DecX86reg(x86_ECX); // Counter
    JneLabel8("Loop", 0);
    x86_SetBranch8b(RecompPos - 1, LoopEntry);

    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;
    CompilerToggleBuffer();

    sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 4) & 0xF);
    MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8((16 - RSPOpC.del - 4) & 0xF), Reg, x86_EAX);
    sprintf(Reg, "m_Vect[%i].B[%i]", RSPOpC.rt, (16 - RSPOpC.del - 8) & 0xF);
    MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8((16 - RSPOpC.del - 8) & 0xF), Reg, x86_ECX);
    MoveX86regToN64Mem(x86_EAX, x86_EBX);
    MoveX86regToN64MemDisp(x86_ECX, x86_EBX, 4);

    CPU_Message("   Done:");
    x86_SetBranch32b((uint32_t *)Jump[1], (uint32_t *)RecompPos);
#endif
}

void CRSPRecompilerOps::Opcode_SQV(void)
{
#ifndef CompileSqv
    Cheat_r4300iOpcode(&RSPOp::SQV, "RSPOp::SQV");
#else
    if (RSPOpC.del != 0 && RSPOpC.del != 12)
    {
        Cheat_r4300iOpcode(&RSPOp::SQV, "RSPOp::SQV");
        return;
    }

    char Reg[256];
    int offset = (RSPOpC.voffset << 4);
    uint8_t * Jump[2];

    CPU_Message("  %X %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());

    if (IsRegConst(RSPOpC.base))
    {
        uint32_t Addr = (MipsRegConst(RSPOpC.base) + offset) & 0xfff;

        if (Addr & 15)
        {
            CompilerWarning(stdstr_f("Unaligned SQV at constant address %04X", CompilePC).c_str());
            Cheat_r4300iOpcodeNoMessage(&RSPOp::SQV, "RSPOp::SQV");
            return;
        }

        // Aligned store

        if (IsSseEnabled == false)
        {
            if (RSPOpC.del == 12)
            {
                sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(0), Reg, x86_EAX);
                sprintf(Reg, "m_Vect[%i].B[12]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(12), Reg, x86_EBX);
                sprintf(Reg, "m_Vect[%i].B[8]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(8), Reg, x86_ECX);
                sprintf(Reg, "m_Vect[%i].B[4]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(4), Reg, x86_EDX);
            }
            else
            {
                sprintf(Reg, "m_Vect[%i].B[12]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(12), Reg, x86_EAX);
                sprintf(Reg, "m_Vect[%i].B[8]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(8), Reg, x86_EBX);
                sprintf(Reg, "m_Vect[%i].B[4]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(4), Reg, x86_ECX);
                sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
                MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(0), Reg, x86_EDX);
            }

            sprintf(Reg, "DMEM+%Xh+0", Addr);
            MoveX86regToVariable(x86_EAX, RSPInfo.DMEM + Addr + 0, Reg);
            sprintf(Reg, "DMEM+%Xh+4", Addr);
            MoveX86regToVariable(x86_EBX, RSPInfo.DMEM + Addr + 4, Reg);
            sprintf(Reg, "DMEM+%Xh+8", Addr);
            MoveX86regToVariable(x86_ECX, RSPInfo.DMEM + Addr + 8, Reg);
            sprintf(Reg, "DMEM+%Xh+C", Addr);
            MoveX86regToVariable(x86_EDX, RSPInfo.DMEM + Addr + 12, Reg);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
            SseMoveAlignedVariableToReg(&m_Vect[RSPOpC.vt].s8(0), Reg, x86_XMM0);
            if (RSPOpC.del == 12)
            {
                SseShuffleReg(x86_XMM0, x86_MM0, 0x6c);
            }
            else
            {
                SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
            }
            sprintf(Reg, "DMEM+%Xh", Addr);
            SseMoveUnalignedRegToVariable(x86_XMM0, RSPInfo.DMEM + Addr, Reg);
        }
        return;
    }

    MoveVariableToX86reg(&m_GPR[RSPOpC.base].UW, GPR_Name(RSPOpC.base), x86_EBX);
    if (offset != 0)
    {
        AddConstToX86Reg(x86_EBX, offset);
    }
    TestConstToX86Reg(15, x86_EBX);
    JneLabel32("Unaligned", 0);
    Jump[0] = RecompPos - 4;

    CompilerToggleBuffer();
    CPU_Message("   Unaligned:");
    x86_SetBranch32b((uint32_t *)Jump[0], (uint32_t *)RecompPos);
    Cheat_r4300iOpcodeNoMessage(&RSPOp::SQV, "RSPOp::SQV");
    JmpLabel32("Done", 0);
    Jump[1] = RecompPos - 4;
    CompilerToggleBuffer();

    AndConstToX86Reg(x86_EBX, 0x0fff);
    if (IsSseEnabled == false)
    {
        if (RSPOpC.del == 12)
        {
            sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(0), Reg, x86_EAX);
            sprintf(Reg, "m_Vect[%i].B[12]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(12), Reg, x86_ECX);
            sprintf(Reg, "m_Vect[%i].B[8]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(8), Reg, x86_EDX);
            sprintf(Reg, "m_Vect[%i].B[4]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(4), Reg, x86_EDI);
        }
        else
        {
            sprintf(Reg, "m_Vect[%i].B[12]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(12), Reg, x86_EAX);
            sprintf(Reg, "m_Vect[%i].B[8]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(8), Reg, x86_ECX);
            sprintf(Reg, "m_Vect[%i].B[4]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(4), Reg, x86_EDX);
            sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
            MoveVariableToX86reg(&m_Vect[RSPOpC.vt].s8(0), Reg, x86_EDI);
        }

        MoveX86regToN64MemDisp(x86_EAX, x86_EBX, 0);
        MoveX86regToN64MemDisp(x86_ECX, x86_EBX, 4);
        MoveX86regToN64MemDisp(x86_EDX, x86_EBX, 8);
        MoveX86regToN64MemDisp(x86_EDI, x86_EBX, 12);
    }
    else
    {
        sprintf(Reg, "m_Vect[%i].B[0]", RSPOpC.rt);
        SseMoveAlignedVariableToReg(&m_Vect[RSPOpC.vt].s8(0), Reg, x86_XMM0);
        if (RSPOpC.del == 12)
        {
            SseShuffleReg(x86_XMM0, x86_MM0, 0x6c);
        }
        else
        {
            SseShuffleReg(x86_XMM0, x86_MM0, 0x1b);
        }
        SseMoveUnalignedRegToN64Mem(x86_XMM0, x86_EBX);
    }
    CPU_Message("   Done:");
    x86_SetBranch32b((uint32_t *)Jump[1], (uint32_t *)RecompPos);
#endif
}

void CRSPRecompilerOps::Opcode_SRV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SRV, "RSPOp::SRV");
}

void CRSPRecompilerOps::Opcode_SPV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SPV, "RSPOp::SPV");
}

void CRSPRecompilerOps::Opcode_SUV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SUV, "RSPOp::SUV");
}

void CRSPRecompilerOps::Opcode_SHV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SHV, "RSPOp::SHV");
}

void CRSPRecompilerOps::Opcode_SFV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SFV, "RSPOp::SFV");
}

void CRSPRecompilerOps::Opcode_STV(void)
{
    Cheat_r4300iOpcode(&RSPOp::STV, "RSPOp::STV");
}

void CRSPRecompilerOps::Opcode_SWV(void)
{
    Cheat_r4300iOpcode(&RSPOp::SWV, "&RSPOp::SWV");
}

// Other functions

void CRSPRecompilerOps::UnknownOpcode(void)
{
    CPU_Message("  %X Unhandled Opcode: %s", CompilePC, RSPInstruction(CompilePC, RSPOpC.Value).NameAndParam().c_str());
    NextInstruction = RSPPIPELINE_FINISH_BLOCK;
    MoveConstToVariable(CompilePC, PrgCount, "RSP PC");
    MoveConstToVariable(RSPOpC.Value, &RSPOpC.Value, "RSPOpC.Value");
    MoveConstToX86reg((uint32_t) & (RSPSystem.m_OpCodes), x86_ECX);
    Call_Direct(AddressOf(&RSPOp::UnknownOpcode), "&RSPOp::UnknownOpcode");
    Ret();
}
