#pragma once

#include <Project64-core/N64System/Mips/R4300iOpcode.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/Settings/DebugSettings.h>

class CX86RecompilerOps;

class R4300iOp :
    public CLogging,
    private CDebugSettings,
    private CSystemRegisters
{
    friend CX86RecompilerOps;

public:
    R4300iOp(CN64System & System);
    ~R4300iOp(void);

    void ExecuteCPU();
    void ExecuteOps(int32_t Cycles);
    void InPermLoop();

    R4300iOpcode Opcode(void) const
    {
        return m_Opcode;
    }

private:
    R4300iOp();
    R4300iOp(const R4300iOp &);
    R4300iOp & operator=(const R4300iOp &);

    void BuildInterpreter(void);

    typedef void (R4300iOp::*Func)();

    void SPECIAL();
    void REGIMM();
    void COP0();
    void COP0_CO();
    void COP1();
    void COP2();
    void COP3();
    void COP1_BC();
    void COP1_S();
    void COP1_D();
    void COP1_W();
    void COP1_L();

    // Opcode functions
    void J();
    void JAL();
    void BNE();
    void BEQ();
    void BLEZ();
    void BGTZ();
    void ADDI();
    void ADDIU();
    void SLTI();
    void SLTIU();
    void ANDI();
    void ORI();
    void XORI();
    void LUI();
    void BEQL();
    void BNEL();
    void BLEZL();
    void BGTZL();
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
    void SDL();
    void SDR();
    void SWR();
    void CACHE();
    void LL();
    void LWC1();
    void LLD();
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
    void SPECIAL_SYNC();
    void SPECIAL_MFHI();
    void SPECIAL_MTHI();
    void SPECIAL_MFLO();
    void SPECIAL_MTLO();
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
    void SPECIAL_TGE();
    void SPECIAL_TGEU();
    void SPECIAL_TLT();
    void SPECIAL_TLTU();
    void SPECIAL_TEQ();
    void SPECIAL_TNE();
    void SPECIAL_DSLL();
    void SPECIAL_DSRL();
    void SPECIAL_DSRA();
    void SPECIAL_DSLL32();
    void SPECIAL_DSRL32();
    void SPECIAL_DSRA32();

    // R4300i opcodes: RegImm
    void REGIMM_BLTZ();
    void REGIMM_BGEZ();
    void REGIMM_BLTZL();
    void REGIMM_BGEZL();
    void REGIMM_BLTZAL();
    void REGIMM_BGEZAL();
    void REGIMM_BGEZALL();
    void REGIMM_TEQI();
    void REGIMM_TGEI();
    void REGIMM_TGEIU();
    void REGIMM_TLTI();
    void REGIMM_TLTIU();
    void REGIMM_TNEI();

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
    void CPO1_UNIMPLEMENTED_OP(void);
    void COP1_MF();
    void COP1_DMF();
    void COP1_CF();
    void COP1_MT();
    void COP1_DMT();
    void COP1_CT();

    // COP1: BC1 functions
    void COP1_BCF();
    void COP1_BCT();
    void COP1_BCFL();
    void COP1_BCTL();

    // COP1: S functions
    void COP1_S_ADD();
    void COP1_S_SUB();
    void COP1_S_MUL();
    void COP1_S_DIV();
    void COP1_S_SQRT();
    void COP1_S_ABS();
    void COP1_S_MOV();
    void COP1_S_NEG();
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
    void COP1_D_SQRT();
    void COP1_D_ABS();
    void COP1_D_MOV();
    void COP1_D_NEG();
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

    // COP2 functions
    void CPO2_INVALID_OP(void);
    void COP2_MF();
    void COP2_DMF();
    void COP2_CF();
    void COP2_MT();
    void COP2_DMT();
    void COP2_CT();

    // Other functions
    void ReservedInstruction();
    void UnknownOpcode();

    CN64System & m_System;
    CRegisters & m_Reg;
    CMipsMemoryVM & m_MMU;
    R4300iOpcode m_Opcode;

    Func Jump_Opcode[64];
    Func Jump_Special[64];
    Func Jump_Regimm[32];
    Func Jump_CoP0[32];
    Func Jump_CoP0_Function[64];
    Func Jump_CoP1[32];
    Func Jump_CoP1_BC[32];
    Func Jump_CoP1_S[64];
    Func Jump_CoP1_D[64];
    Func Jump_CoP1_W[64];
    Func Jump_CoP1_L[64];
    Func Jump_CoP2[32];

    bool TestCop1UsableException(void);
    bool CheckFPUInput32(const float & Value);
    bool CheckFPUInput32Conv(const float & Value);
    bool CheckFPUInput64(const double & Value);
    bool CheckFPUInput64Conv(const double & Value);
    bool CheckFPUResult32(float & Result);
    bool CheckFPUResult64(double & Result);
    bool CheckFPUInvalidException(void);
    bool InitFpuOperation(FPRoundingMode RoundingModel);
    bool SetFPUException(void);

    static const uint32_t SWL_MASK[4], SWR_MASK[4], LWL_MASK[4], LWR_MASK[4];
    static const int32_t SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];
};
