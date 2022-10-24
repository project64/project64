#pragma once
#if defined(__aarch64__)

#include <Project64-core/N64System/Mips/R4300iOpcode.h>
#include <Project64-core/N64System/Recompiler/Aarch64/Aarch64ops.h>
#include <Project64-core/N64System/Recompiler/ExitInfo.h>
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>

class CMipsMemoryVM;
class CCodeBlock;
class CCodeSection;
class CAarch64Ops;
struct CJumpInfo;

class CAarch64RecompilerOps
{
public:
    CAarch64RecompilerOps(CMipsMemoryVM & MMU, CCodeBlock & CodeBlock);
    ~CAarch64RecompilerOps();

    // Trap functions
    void Compile_TrapCompare(RecompilerTrapCompare CompareType);

    // Branch functions
    void Compile_BranchCompare(RecompilerBranchCompare CompareType);
    void Compile_Branch(RecompilerBranchCompare CompareType, bool Link);
    void Compile_BranchLikely(RecompilerBranchCompare CompareType, bool Link);
    void BNE_Compare();
    void BEQ_Compare();
    void BGTZ_Compare();
    void BLEZ_Compare();
    void BLTZ_Compare();
    void BGEZ_Compare();
    void COP1_BCF_Compare();
    void COP1_BCT_Compare();

    //  Opcode functions
    void J();
    void JAL();
    void ADDI();
    void ADDIU();
    void SLTI();
    void SLTIU();
    void ANDI();
    void ORI();
    void XORI();
    void LUI();
    void DADDI();
    void DADDIU();
    void LDL();
    void LDR();
    void LB();
    void LH();
    void LWL();
    void LW();
    void LBU();
    void LHU();
    void LWR();
    void LWU();
    void SB();
    void SH();
    void SWL();
    void SW();
    void SWR();
    void SDL();
    void SDR();
    void CACHE();
    void LL();
    void LWC1();
    void LDC1();
    void LD();
    void SC();
    void SWC1();
    void SDC1();
    void SD();

    // R4300i opcodes: Special
    void SPECIAL_SLL();
    void SPECIAL_SRL();
    void SPECIAL_SRA();
    void SPECIAL_SLLV();
    void SPECIAL_SRLV();
    void SPECIAL_SRAV();
    void SPECIAL_JR();
    void SPECIAL_JALR();
    void SPECIAL_SYSCALL();
    void SPECIAL_BREAK();
    void SPECIAL_MFLO();
    void SPECIAL_MTLO();
    void SPECIAL_MFHI();
    void SPECIAL_MTHI();
    void SPECIAL_DSLLV();
    void SPECIAL_DSRLV();
    void SPECIAL_DSRAV();
    void SPECIAL_MULT();
    void SPECIAL_MULTU();
    void SPECIAL_DIV();
    void SPECIAL_DIVU();
    void SPECIAL_DMULT();
    void SPECIAL_DMULTU();
    void SPECIAL_DDIV();
    void SPECIAL_DDIVU();
    void SPECIAL_ADD();
    void SPECIAL_ADDU();
    void SPECIAL_SUB();
    void SPECIAL_SUBU();
    void SPECIAL_AND();
    void SPECIAL_OR();
    void SPECIAL_XOR();
    void SPECIAL_NOR();
    void SPECIAL_SLT();
    void SPECIAL_SLTU();
    void SPECIAL_DADD();
    void SPECIAL_DADDU();
    void SPECIAL_DSUB();
    void SPECIAL_DSUBU();
    void SPECIAL_DSLL();
    void SPECIAL_DSRL();
    void SPECIAL_DSRA();
    void SPECIAL_DSLL32();
    void SPECIAL_DSRL32();
    void SPECIAL_DSRA32();

    // COP0 functions
    void COP0_MF();
    void COP0_DMF();
    void COP0_MT();
    void COP0_DMT();

    // COP0 CO functions
    void COP0_CO_TLBR();
    void COP0_CO_TLBWI();
    void COP0_CO_TLBWR();
    void COP0_CO_TLBP();
    void COP0_CO_ERET();

    // COP1 functions
    void COP1_MF();
    void COP1_DMF();
    void COP1_CF();
    void COP1_MT();
    void COP1_DMT();
    void COP1_CT();

    // COP1: S functions
    void COP1_S_ADD();
    void COP1_S_SUB();
    void COP1_S_MUL();
    void COP1_S_DIV();
    void COP1_S_ABS();
    void COP1_S_NEG();
    void COP1_S_SQRT();
    void COP1_S_MOV();
    void COP1_S_ROUND_L();
    void COP1_S_TRUNC_L();
    void COP1_S_CEIL_L();
    void COP1_S_FLOOR_L();
    void COP1_S_ROUND_W();
    void COP1_S_TRUNC_W();
    void COP1_S_CEIL_W();
    void COP1_S_FLOOR_W();
    void COP1_S_CVT_D();
    void COP1_S_CVT_W();
    void COP1_S_CVT_L();
    void COP1_S_CMP();

    // COP1: D functions
    void COP1_D_ADD();
    void COP1_D_SUB();
    void COP1_D_MUL();
    void COP1_D_DIV();
    void COP1_D_ABS();
    void COP1_D_NEG();
    void COP1_D_SQRT();
    void COP1_D_MOV();
    void COP1_D_ROUND_L();
    void COP1_D_TRUNC_L();
    void COP1_D_CEIL_L();
    void COP1_D_FLOOR_L();
    void COP1_D_ROUND_W();
    void COP1_D_TRUNC_W();
    void COP1_D_CEIL_W();
    void COP1_D_FLOOR_W();
    void COP1_D_CVT_S();
    void COP1_D_CVT_W();
    void COP1_D_CVT_L();
    void COP1_D_CMP();

    // COP1: W functions
    void COP1_W_CVT_S();
    void COP1_W_CVT_D();

    // COP1: L functions
    void COP1_L_CVT_S();
    void COP1_L_CVT_D();

    // Other functions
    void UnknownOpcode();

    void EnterCodeBlock();
    void CompileExitCode();
    void CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter);
    void SyncRegState(const CRegInfo & SyncTo);
    CRegInfo & GetRegWorkingSet(void);
    void SetRegWorkingSet(const CRegInfo & RegInfo);
    bool InheritParentInfo();
    void LinkJump(CJumpInfo & JumpInfo, uint32_t SectionID = -1, uint32_t FromSectionID = -1);
    void JumpToSection(CCodeSection * Section);
    void JumpToUnknown(CJumpInfo * JumpInfo);
    void SetCurrentPC(uint32_t ProgramCounter);
    uint32_t GetCurrentPC(void);
    void SetCurrentSection(CCodeSection * section);
    void SetNextStepType(PIPELINE_STAGE StepType);
    PIPELINE_STAGE GetNextStepType(void);
    const R4300iOpcode & GetOpcode(void) const;
    void PreCompileOpcode(void);
    void PostCompileOpcode(void);
    void CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo & ExitRegSet, ExitReason Reason);

    void UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues = false, bool UpdateTimer = true);
    void CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet);
    void CompileExecuteBP(void);
    void CompileExecuteDelaySlotBP(void);

    CAarch64Ops & Assembler()
    {
        return m_Assembler;
    }

private:
    CAarch64RecompilerOps(const CAarch64RecompilerOps &);
    CAarch64RecompilerOps & operator=(const CAarch64RecompilerOps &);

    CAarch64RegInfo m_RegWorkingSet;
    CAarch64Ops m_Assembler;
    R4300iOpcode m_Opcode;
};

typedef CAarch64RecompilerOps CRecompilerOps;

#endif
