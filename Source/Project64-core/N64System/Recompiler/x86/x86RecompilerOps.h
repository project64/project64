#pragma once
#if defined(__i386__) || defined(_M_IX86)

#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Mips/R4300iOpcode.h>
#include <Project64-core/N64System/Recompiler/ExitInfo.h>
#include <Project64-core/N64System/Recompiler/RegInfo.h>
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>
#include <Project64-core/N64System/Recompiler/JumpInfo.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/Settings/N64SystemSettings.h>
#include <Project64-core/Settings/RecompilerSettings.h>
#include <Project64-core/Settings/GameSettings.h>

class CCodeBlock;
class CCodeSection;

class CX86RecompilerOps :
    protected R4300iOp,
    protected CN64SystemSettings,
    protected CRecompilerSettings,
    private CGameSettings
{
public:
    CX86RecompilerOps(CMipsMemoryVM & MMU, CCodeBlock & CodeBlock);
    ~CX86RecompilerOps();

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
    void COP0_MT();

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

    void ClearCachedInstructionInfo();
    void FoundMemoryBreakpoint();
    void PreReadInstruction();
    void PreWriteInstruction();
    void TestWriteBreakpoint(CX86Ops::x86Reg AddressReg, void * FunctAddress, const char * FunctName);
    void TestReadBreakpoint(CX86Ops::x86Reg AddressReg, void * FunctAddress, const char * FunctName);
    void TestBreakpoint(CX86Ops::x86Reg AddressReg, void * FunctAddress, const char * FunctName);
    void EnterCodeBlock();
    void ExitCodeBlock();
    void CompileExitCode();
    void CompileCop1Test();
    void CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter);
    void SyncRegState(const CRegInfo & SyncTo);
    bool SetupRegisterForLoop(CCodeBlock & BlockInfo, const CRegInfo & RegSet);
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
    void CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason);

    void CompileReadTLBMiss(uint32_t VirtualAddress, CX86Ops::x86Reg LookUpReg);
    void CompileReadTLBMiss(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg LookUpReg);
    void CompileWriteTLBMiss(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg LookUpReg);
    void UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles);
    void UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues = false, bool UpdateTimer = true);
    void CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet);
    void CompileExecuteBP(void);
    void CompileExecuteDelaySlotBP(void);
    static void ChangeDefaultRoundingModel();
    void OverflowDelaySlot(bool TestTimer);

    CX86Ops & Assembler() { return m_Assembler; }

    // Helper functions
    typedef CRegInfo::REG_STATE REG_STATE;

    REG_STATE GetMipsRegState(int32_t Reg) { return m_RegWorkingSet.GetMipsRegState(Reg); }
    uint64_t GetMipsReg(int32_t Reg) { return m_RegWorkingSet.GetMipsReg(Reg); }
    int64_t GetMipsReg_S(int32_t Reg) { return m_RegWorkingSet.GetMipsReg_S(Reg); }
    uint32_t GetMipsRegLo(int32_t Reg) { return m_RegWorkingSet.GetMipsRegLo(Reg); }
    int32_t GetMipsRegLo_S(int32_t Reg) { return m_RegWorkingSet.GetMipsRegLo_S(Reg); }
    uint32_t GetMipsRegHi(int32_t Reg) { return m_RegWorkingSet.GetMipsRegHi(Reg); }
    int32_t GetMipsRegHi_S(int32_t Reg) { return m_RegWorkingSet.GetMipsRegHi_S(Reg); }
    CX86Ops::x86Reg GetMipsRegMapLo(int32_t Reg) { return m_RegWorkingSet.GetMipsRegMapLo(Reg); }
    CX86Ops::x86Reg GetMipsRegMapHi(int32_t Reg) { return m_RegWorkingSet.GetMipsRegMapHi(Reg); }

    bool IsKnown(int32_t Reg) { return m_RegWorkingSet.IsKnown(Reg); }
    bool IsUnknown(int32_t Reg) { return m_RegWorkingSet.IsUnknown(Reg); }
    bool IsMapped(int32_t Reg) { return m_RegWorkingSet.IsMapped(Reg); }
    bool IsConst(int32_t Reg) { return m_RegWorkingSet.IsConst(Reg); }
    bool IsSigned(int32_t Reg) { return m_RegWorkingSet.IsSigned(Reg); }
    bool IsUnsigned(int32_t Reg) { return m_RegWorkingSet.IsUnsigned(Reg); }
    bool Is32Bit(int32_t Reg) { return m_RegWorkingSet.Is32Bit(Reg); }
    bool Is64Bit(int32_t Reg) { return m_RegWorkingSet.Is64Bit(Reg); }
    bool Is32BitMapped(int32_t Reg) { return m_RegWorkingSet.Is32BitMapped(Reg); }
    bool Is64BitMapped(int32_t Reg) { return m_RegWorkingSet.Is64BitMapped(Reg); }

    void FixRoundModel(CRegInfo::FPU_ROUND RoundMethod) { m_RegWorkingSet.FixRoundModel(RoundMethod); }
    void ChangeFPURegFormat(int32_t Reg, CRegInfo::FPU_STATE OldFormat, CRegInfo::FPU_STATE NewFormat, CRegInfo::FPU_ROUND RoundingModel) { m_RegWorkingSet.ChangeFPURegFormat(Reg, OldFormat, NewFormat, RoundingModel); }
    void Load_FPR_ToTop(int32_t Reg, int32_t RegToLoad, CRegInfo::FPU_STATE Format) { m_RegWorkingSet.Load_FPR_ToTop(Reg, RegToLoad, Format); }
    bool RegInStack(int32_t Reg, CRegInfo::FPU_STATE Format) { return m_RegWorkingSet.RegInStack(Reg, Format); }
    CX86Ops::x86FpuValues StackPosition(int32_t Reg) { return m_RegWorkingSet.StackPosition(Reg); }
    void UnMap_AllFPRs() { m_RegWorkingSet.UnMap_AllFPRs(); }
    void UnMap_FPR(uint32_t Reg, bool WriteBackValue) { m_RegWorkingSet.UnMap_FPR(Reg, WriteBackValue); }

    CX86Ops::x86Reg FreeX86Reg() { return m_RegWorkingSet.FreeX86Reg(); }
    CX86Ops::x86Reg Free8BitX86Reg() { return m_RegWorkingSet.Free8BitX86Reg(); }
    void Map_GPR_32bit(int32_t Reg, bool SignValue, int32_t MipsRegToLoad) { m_RegWorkingSet.Map_GPR_32bit(Reg, SignValue, MipsRegToLoad); }
    void Map_GPR_64bit(int32_t Reg, int32_t MipsRegToLoad) { m_RegWorkingSet.Map_GPR_64bit(Reg, MipsRegToLoad); }
    CX86Ops::x86Reg Get_MemoryStack() { return m_RegWorkingSet.Get_MemoryStack(); }
    CX86Ops::x86Reg Map_MemoryStack(CX86Ops::x86Reg Reg, bool bMapRegister, bool LoadValue = true) { return m_RegWorkingSet.Map_MemoryStack(Reg, bMapRegister, LoadValue); }
    CX86Ops::x86Reg Map_TempReg(CX86Ops::x86Reg Reg, int32_t MipsReg, bool LoadHiWord) { return m_RegWorkingSet.Map_TempReg(Reg, MipsReg, LoadHiWord); }
    void ProtectGPR(uint32_t Reg) { m_RegWorkingSet.ProtectGPR(Reg); }
    void UnProtectGPR(uint32_t Reg) { m_RegWorkingSet.UnProtectGPR(Reg); }
    void ResetX86Protection() { m_RegWorkingSet.ResetX86Protection(); }
    CX86Ops::x86Reg UnMap_TempReg() { return m_RegWorkingSet.UnMap_TempReg(); }
    void UnMap_GPR(uint32_t Reg, bool WriteBackValue) { m_RegWorkingSet.UnMap_GPR(Reg, WriteBackValue); }
    bool UnMap_X86reg(CX86Ops::x86Reg Reg) { return m_RegWorkingSet.UnMap_X86reg(Reg); }

public:
    uint32_t CompilePC() { return m_CompilePC; }

private:
    CX86RecompilerOps(const CX86RecompilerOps&);
    CX86RecompilerOps& operator=(const CX86RecompilerOps&);

    CX86Ops::x86Reg BaseOffsetAddress(bool UseBaseRegister);
    void CompileLoadMemoryValue(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg ValueReg, CX86Ops::x86Reg ValueRegHi, uint8_t ValueSize, bool SignExtend);
    void CompileStoreMemoryValue(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg ValueReg, CX86Ops::x86Reg ValueRegHi, uint64_t Value, uint8_t ValueSize);

    void SB_Const(uint32_t Value, uint32_t Addr);
    void SB_Register(CX86Ops::x86Reg Reg, uint32_t Addr);
    void SH_Const(uint32_t Value, uint32_t Addr);
    void SH_Register(CX86Ops::x86Reg Reg, uint32_t Addr);
    void SW_Const(uint32_t Value, uint32_t Addr);
    void SW_Register(CX86Ops::x86Reg Reg, uint32_t Addr);
    void LB_KnownAddress(CX86Ops::x86Reg Reg, uint32_t VAddr, bool SignExtend);
    void LH_KnownAddress(CX86Ops::x86Reg Reg, uint32_t VAddr, bool SignExtend);
    void LW_KnownAddress(CX86Ops::x86Reg Reg, uint32_t VAddr);
    void LW(bool ResultSigned, bool bRecordLLBit);
    void SW(bool bCheckLLbit);
    void CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason, bool CompileNow, void(CX86Ops::*x86Jmp)(const char * Label, uint32_t Value));
    void ResetMemoryStack();

    EXIT_LIST m_ExitInfo;
    CMipsMemoryVM & m_MMU;
    CCodeBlock & m_CodeBlock;
    CX86Ops m_Assembler;
    PIPELINE_STAGE m_PipelineStage;
    uint32_t m_CompilePC;
    R4300iOpcode m_Opcode;
    CX86RegInfo m_RegWorkingSet;
    CCodeSection * m_Section;
    CRegInfo m_RegBeforeDelay;
    bool m_EffectDelaySlot;
    static uint32_t m_TempValue32;
    static uint32_t m_BranchCompare;
};

#endif
