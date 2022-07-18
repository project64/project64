#pragma once

#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Mips/R4300iOpcode.h>

class R4300iOp :
    public CLogging,
    protected CDebugSettings,
    protected CSystemRegisters
{
public:
    typedef void(*Func)();

    // Opcode functions
    static void  J();
    static void  JAL();
    static void  BNE();
    static void  BEQ();
    static void  BLEZ();
    static void  BGTZ();
    static void  ADDI();
    static void  ADDIU();
    static void  SLTI();
    static void  SLTIU();
    static void  ANDI();
    static void  ORI();
    static void  XORI();
    static void  LUI();
    static void  BEQL();
    static void  BNEL();
    static void  BLEZL();
    static void  BGTZL();
    static void  DADDIU();
    static void  LDL();
    static void  LDR();
    static void  LB();
    static void  LH();
    static void  LWL();
    static void  LW();
    static void  LBU();
    static void  LHU();
    static void  LWR();
    static void  LWU();
    static void  SB();
    static void  SH();
    static void  SWL();
    static void  SW();
    static void  SDL();
    static void  SDR();
    static void  SWR();
    static void  CACHE();
    static void  LL();
    static void  LWC1();
    static void  LDC1();
    static void  LD();
    static void  SC();
    static void  SWC1();
    static void  SDC1();
    static void  SD();

    // R4300i opcodes: Special
    static void  SPECIAL_SLL();
    static void  SPECIAL_SRL();
    static void  SPECIAL_SRA();
    static void  SPECIAL_SLLV();
    static void  SPECIAL_SRLV();
    static void  SPECIAL_SRAV();
    static void  SPECIAL_JR();
    static void  SPECIAL_JALR();
    static void  SPECIAL_SYSCALL();
    static void  SPECIAL_BREAK();
    static void  SPECIAL_SYNC();
    static void  SPECIAL_MFHI();
    static void  SPECIAL_MTHI();
    static void  SPECIAL_MFLO();
    static void  SPECIAL_MTLO();
    static void  SPECIAL_DSLLV();
    static void  SPECIAL_DSRLV();
    static void  SPECIAL_DSRAV();
    static void  SPECIAL_MULT();
    static void  SPECIAL_MULTU();
    static void  SPECIAL_DIV();
    static void  SPECIAL_DIVU();
    static void  SPECIAL_DMULT();
    static void  SPECIAL_DMULTU();
    static void  SPECIAL_DDIV();
    static void  SPECIAL_DDIVU();
    static void  SPECIAL_ADD();
    static void  SPECIAL_ADDU();
    static void  SPECIAL_SUB();
    static void  SPECIAL_SUBU();
    static void  SPECIAL_AND();
    static void  SPECIAL_OR();
    static void  SPECIAL_XOR();
    static void  SPECIAL_NOR();
    static void  SPECIAL_SLT();
    static void  SPECIAL_SLTU();
    static void  SPECIAL_DADD();
    static void  SPECIAL_DADDU();
    static void  SPECIAL_DSUB();
    static void  SPECIAL_DSUBU();
	static void  SPECIAL_TGE();
	static void  SPECIAL_TGEU();
	static void  SPECIAL_TLT();
	static void  SPECIAL_TLTU();
    static void  SPECIAL_TEQ();
	static void  SPECIAL_TNE();
    static void  SPECIAL_DSLL();
    static void  SPECIAL_DSRL();
    static void  SPECIAL_DSRA();
    static void  SPECIAL_DSLL32();
    static void  SPECIAL_DSRL32();
    static void  SPECIAL_DSRA32();

    // R4300i opcodes: RegImm
    static void  REGIMM_BLTZ();
    static void  REGIMM_BGEZ();
    static void  REGIMM_BLTZL();
    static void  REGIMM_BGEZL();
    static void  REGIMM_BLTZAL();
    static void  REGIMM_BGEZAL();
    static void  REGIMM_TEQI();
    static void  REGIMM_TGEI();
    static void  REGIMM_TGEIU();
    static void  REGIMM_TLTI();
    static void  REGIMM_TLTIU();
    static void  REGIMM_TNEI();

    // COP0 functions
    static void  COP0_MF();
    static void  COP0_MT();

    // COP0 CO functions
    static void  COP0_CO_TLBR();
    static void  COP0_CO_TLBWI();
    static void  COP0_CO_TLBWR();
    static void  COP0_CO_TLBP();
    static void  COP0_CO_ERET();

    // COP1 functions
    static void  COP1_MF();
    static void  COP1_DMF();
    static void  COP1_CF();
    static void  COP1_MT();
    static void  COP1_DMT();
    static void  COP1_CT();

    // COP1: BC1 functions
    static void  COP1_BCF();
    static void  COP1_BCT();
    static void  COP1_BCFL();
    static void  COP1_BCTL();

    // COP1: S functions
    static void  COP1_S_ADD();
    static void  COP1_S_SUB();
    static void  COP1_S_MUL();
    static void  COP1_S_DIV();
    static void  COP1_S_SQRT();
    static void  COP1_S_ABS();
    static void  COP1_S_MOV();
    static void  COP1_S_NEG();
    static void  COP1_S_ROUND_L();
    static void  COP1_S_TRUNC_L();
    static void  COP1_S_CEIL_L();  
    static void  COP1_S_FLOOR_L(); 
    static void  COP1_S_ROUND_W();
    static void  COP1_S_TRUNC_W();
    static void  COP1_S_CEIL_W();  
    static void  COP1_S_FLOOR_W();
    static void  COP1_S_CVT_D();
    static void  COP1_S_CVT_W();
    static void  COP1_S_CVT_L();
    static void  COP1_S_CMP();

    // COP1: D functions
    static void  COP1_D_ADD();
    static void  COP1_D_SUB();
    static void  COP1_D_MUL();
    static void  COP1_D_DIV();
    static void  COP1_D_SQRT();
    static void  COP1_D_ABS();
    static void  COP1_D_MOV();
    static void  COP1_D_NEG();
    static void  COP1_D_ROUND_L();
    static void  COP1_D_TRUNC_L(); 
    static void  COP1_D_CEIL_L();  
    static void  COP1_D_FLOOR_L(); 
    static void  COP1_D_ROUND_W();
    static void  COP1_D_TRUNC_W();
    static void  COP1_D_CEIL_W();  
    static void  COP1_D_FLOOR_W(); 
    static void  COP1_D_CVT_S();
    static void  COP1_D_CVT_W();
    static void  COP1_D_CVT_L();
    static void  COP1_D_CMP();

    // COP1: W functions
    static void  COP1_W_CVT_S();
    static void  COP1_W_CVT_D();

    // COP1: L functions
    static void  COP1_L_CVT_S();
    static void  COP1_L_CVT_D();

    // Other functions
    static void  UnknownOpcode();

    static Func* BuildInterpreter();

    static bool m_TestTimer;
    static R4300iOpcode m_Opcode;

    static bool  MemoryBreakpoint();

protected:
    static void SPECIAL();
    static void REGIMM();
    static void COP0();
    static void COP0_CO();
    static void COP1();
    static void COP1_BC();
    static void COP1_S();
    static void COP1_D();
    static void COP1_W();
    static void COP1_L();

    static Func Jump_Opcode[64];
    static Func Jump_Special[64];
    static Func Jump_Regimm[32];
    static Func Jump_CoP0[32];
    static Func Jump_CoP0_Function[64];
    static Func Jump_CoP1[32];
    static Func Jump_CoP1_BC[32];
    static Func Jump_CoP1_S[64];
    static Func Jump_CoP1_D[64];
    static Func Jump_CoP1_W[64];
    static Func Jump_CoP1_L[64];

    static void GenerateAddressErrorException(uint32_t VAddr, bool FromRead);
    static void GenerateTLBReadException(uint32_t VAddr, const char * function);
    static void GenerateTLBWriteException(uint32_t VAddr, const char * function);

    static const uint32_t SWL_MASK[4], SWR_MASK[4], LWL_MASK[4], LWR_MASK[4];
    static const int32_t SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];
};
