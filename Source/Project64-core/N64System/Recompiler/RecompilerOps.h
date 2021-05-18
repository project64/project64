#pragma once
#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/Recompiler/JumpInfo.h>
#include <Project64-core/N64System/Mips/OpCode.h>

class CCodeSection;

class CRecompilerOps
{
public:
    enum BRANCH_TYPE
    {
        BranchTypeCop1,
        BranchTypeRs,
        BranchTypeRsRt
    };
    enum BRANCH_COMPARE
    {
        CompareTypeBEQ,
        CompareTypeBNE,
        CompareTypeBLTZ,
        CompareTypeBLEZ,
        CompareTypeBGTZ,
        CompareTypeBGEZ,
        CompareTypeCOP1BCF,
        CompareTypeCOP1BCT,
    };
    enum TRAP_COMPARE
    {
        CompareTypeTEQ,
        CompareTypeTNE,
        CompareTypeTGE,
        CompareTypeTGEU,
        CompareTypeTLT,
        CompareTypeTLTU,
        CompareTypeTEQI,
        CompareTypeTNEI,
        CompareTypeTGEI,
        CompareTypeTGEIU,
        CompareTypeTLTI,
        CompareTypeTLTIU,
    };

    // Trap functions
    virtual void Compile_TrapCompare(TRAP_COMPARE CompareType) = 0;

    // Branch functions
    virtual void Compile_Branch(BRANCH_COMPARE CompareType, BRANCH_TYPE BranchType, bool Link) = 0;
    virtual void Compile_BranchLikely(BRANCH_COMPARE CompareType, bool Link) = 0;

    //  Opcode functions
    virtual void J() = 0;
    virtual void JAL() = 0;
    virtual void ADDI() = 0;
    virtual void ADDIU() = 0;
    virtual void SLTI() = 0;
    virtual void SLTIU() = 0;
    virtual void ANDI() = 0;
    virtual void ORI() = 0;
    virtual void XORI() = 0;
    virtual void LUI() = 0;
    virtual void DADDIU() = 0;
    virtual void LDL() = 0;
    virtual void LDR() = 0;
    virtual void LB() = 0;
    virtual void LH() = 0;
    virtual void LWL() = 0;
    virtual void LW() = 0;
    virtual void LBU() = 0;
    virtual void LHU() = 0;
    virtual void LWR() = 0;
    virtual void LWU() = 0;
    virtual void SB() = 0;
    virtual void SH() = 0;
    virtual void SWL() = 0;
    virtual void SW() = 0;
    virtual void SWR() = 0;
    virtual void SDL() = 0;
    virtual void SDR() = 0;
    virtual void CACHE() = 0;
    virtual void LL() = 0;
    virtual void LWC1() = 0;
    virtual void LDC1() = 0;
    virtual void LD() = 0;
    virtual void SC() = 0;
    virtual void SWC1() = 0;
    virtual void SDC1() = 0;
    virtual void SD() = 0;

    // R4300i opcodes: Special
    virtual void SPECIAL_SLL() = 0;
    virtual void SPECIAL_SRL() = 0;
    virtual void SPECIAL_SRA() = 0;
    virtual void SPECIAL_SLLV() = 0;
    virtual void SPECIAL_SRLV() = 0;
    virtual void SPECIAL_SRAV() = 0;
    virtual void SPECIAL_JR() = 0;
    virtual void SPECIAL_JALR() = 0;
    virtual void SPECIAL_SYSCALL() = 0;
    virtual void SPECIAL_MFLO() = 0;
    virtual void SPECIAL_MTLO() = 0;
    virtual void SPECIAL_MFHI() = 0;
    virtual void SPECIAL_MTHI() = 0;
    virtual void SPECIAL_DSLLV() = 0;
    virtual void SPECIAL_DSRLV() = 0;
    virtual void SPECIAL_DSRAV() = 0;
    virtual void SPECIAL_MULT() = 0;
    virtual void SPECIAL_MULTU() = 0;
    virtual void SPECIAL_DIV() = 0;
    virtual void SPECIAL_DIVU() = 0;
    virtual void SPECIAL_DMULT() = 0;
    virtual void SPECIAL_DMULTU() = 0;
    virtual void SPECIAL_DDIV() = 0;
    virtual void SPECIAL_DDIVU() = 0;
    virtual void SPECIAL_ADD() = 0;
    virtual void SPECIAL_ADDU() = 0;
    virtual void SPECIAL_SUB() = 0;
    virtual void SPECIAL_SUBU() = 0;
    virtual void SPECIAL_AND() = 0;
    virtual void SPECIAL_OR() = 0;
    virtual void SPECIAL_XOR() = 0;
    virtual void SPECIAL_NOR() = 0;
    virtual void SPECIAL_SLT() = 0;
    virtual void SPECIAL_SLTU() = 0;
    virtual void SPECIAL_DADD() = 0;
    virtual void SPECIAL_DADDU() = 0;
    virtual void SPECIAL_DSUB() = 0;
    virtual void SPECIAL_DSUBU() = 0;
    virtual void SPECIAL_DSLL() = 0;
    virtual void SPECIAL_DSRL() = 0;
    virtual void SPECIAL_DSRA() = 0;
    virtual void SPECIAL_DSLL32() = 0;
    virtual void SPECIAL_DSRL32() = 0;
    virtual void SPECIAL_DSRA32() = 0;

    // COP0 functions
    virtual void COP0_MF() = 0;
    virtual void COP0_MT() = 0;

    // COP0 CO functions
    virtual void COP0_CO_TLBR() = 0;
    virtual void COP0_CO_TLBWI() = 0;
    virtual void COP0_CO_TLBWR() = 0;
    virtual void COP0_CO_TLBP() = 0;
    virtual void COP0_CO_ERET() = 0;

    // COP1 functions
    virtual void COP1_MF() = 0;
    virtual void COP1_DMF() = 0;
    virtual void COP1_CF() = 0;
    virtual void COP1_MT() = 0;
    virtual void COP1_DMT() = 0;
    virtual void COP1_CT() = 0;

    // COP1: S functions
    virtual void COP1_S_ADD() = 0;
    virtual void COP1_S_SUB() = 0;
    virtual void COP1_S_MUL() = 0;
    virtual void COP1_S_DIV() = 0;
    virtual void COP1_S_ABS() = 0;
    virtual void COP1_S_NEG() = 0;
    virtual void COP1_S_SQRT() = 0;
    virtual void COP1_S_MOV() = 0;
    virtual void COP1_S_ROUND_L() = 0;
    virtual void COP1_S_TRUNC_L() = 0;
    virtual void COP1_S_CEIL_L() = 0;
    virtual void COP1_S_FLOOR_L() = 0;
    virtual void COP1_S_ROUND_W() = 0;
    virtual void COP1_S_TRUNC_W() = 0;
    virtual void COP1_S_CEIL_W() = 0;
    virtual void COP1_S_FLOOR_W() = 0;
    virtual void COP1_S_CVT_D() = 0;
    virtual void COP1_S_CVT_W() = 0;
    virtual void COP1_S_CVT_L() = 0;
    virtual void COP1_S_CMP() = 0;

    // COP1: D functions
    virtual void COP1_D_ADD() = 0;
    virtual void COP1_D_SUB() = 0;
    virtual void COP1_D_MUL() = 0;
    virtual void COP1_D_DIV() = 0;
    virtual void COP1_D_ABS() = 0;
    virtual void COP1_D_NEG() = 0;
    virtual void COP1_D_SQRT() = 0;
    virtual void COP1_D_MOV() = 0;
    virtual void COP1_D_ROUND_L() = 0;
    virtual void COP1_D_TRUNC_L() = 0;
    virtual void COP1_D_CEIL_L() = 0;
    virtual void COP1_D_FLOOR_L() = 0;
    virtual void COP1_D_ROUND_W() = 0;
    virtual void COP1_D_TRUNC_W() = 0;
    virtual void COP1_D_CEIL_W() = 0;
    virtual void COP1_D_FLOOR_W() = 0;
    virtual void COP1_D_CVT_S() = 0;
    virtual void COP1_D_CVT_W() = 0;
    virtual void COP1_D_CVT_L() = 0;
    virtual void COP1_D_CMP() = 0;

    // COP1: W functions
    virtual void COP1_W_CVT_S() = 0;
    virtual void COP1_W_CVT_D() = 0;

    // COP1: L functions
    virtual void COP1_L_CVT_S() = 0;
    virtual void COP1_L_CVT_D() = 0;

    // Other functions
    virtual void UnknownOpcode() = 0;

    virtual void EnterCodeBlock() = 0;
    virtual void ExitCodeBlock() = 0;
    virtual void CompileExitCode() = 0;
    virtual void CompileCop1Test() = 0;
    virtual void CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter) = 0;
    virtual void SyncRegState(const CRegInfo & SyncTo) = 0;
    virtual void CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason) = 0;
    virtual void CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet) = 0;
    virtual CRegInfo & GetRegWorkingSet(void) = 0;
    virtual void SetRegWorkingSet(const CRegInfo & RegInfo) = 0;
    virtual bool InheritParentInfo() = 0;
    virtual void LinkJump(CJumpInfo & JumpInfo, uint32_t SectionID = -1, uint32_t FromSectionID = -1) = 0;
    virtual void JumpToSection(CCodeSection * Section) = 0;
    virtual void JumpToUnknown(CJumpInfo * JumpInfo) = 0;
    virtual void SetCurrentPC(uint32_t ProgramCounter) = 0;
    virtual uint32_t GetCurrentPC(void) = 0;
    virtual void SetCurrentSection(CCodeSection * section) = 0;
    virtual void SetNextStepType(STEP_TYPE StepType) = 0;
    virtual STEP_TYPE GetNextStepType(void) = 0;
    virtual const OPCODE & GetOpcode(void) const = 0;
    virtual void PreCompileOpcode(void) = 0;
    virtual void PostCompileOpcode(void) = 0;
    virtual void UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues = false) = 0;
    virtual void CompileExecuteBP(void) = 0;
    virtual void CompileExecuteDelaySlotBP(void) = 0;
};
