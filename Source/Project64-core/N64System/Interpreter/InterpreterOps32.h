#pragma once

#include <Project64-core/N64System/Interpreter/InterpreterOps.h>

class R4300iOp32 :
    public R4300iOp
{
public:
    /************************* OpCode functions *************************/
    static void  JAL();
    static void  BEQ();
    static void  BNE();
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
    static void  LB();
    static void  LH();
    static void  LWL();
    static void  LW();
    static void  LBU();
    static void  LHU();
    static void  LWR();
    static void  LWU();
    static void  LL();

    /********************** R4300i OpCodes: Special **********************/
    static void  SPECIAL_SLL();
    static void  SPECIAL_SRL();
    static void  SPECIAL_SRA();
    static void  SPECIAL_SLLV();
    static void  SPECIAL_SRLV();
    static void  SPECIAL_SRAV();
    static void  SPECIAL_JALR();
    static void  SPECIAL_ADD();
    static void  SPECIAL_ADDU();
    static void  SPECIAL_SUB();
    static void  SPECIAL_SUBU();
    static void  SPECIAL_AND();
    static void  SPECIAL_OR();
    static void  SPECIAL_NOR();
    static void  SPECIAL_SLT();
    static void  SPECIAL_SLTU();
    static void  SPECIAL_TEQ();

    /********************** R4300i OpCodes: RegImm **********************/
    static void  REGIMM_BLTZ();
    static void  REGIMM_BGEZ();
    static void  REGIMM_BLTZL();
    static void  REGIMM_BGEZL();
    static void  REGIMM_BLTZAL();
    static void  REGIMM_BGEZAL();

    /************************** COP0 functions **************************/
    static void  COP0_MF();
    static void  COP0_MT();

    /************************** COP1 functions **************************/
    static void  COP1_MF();
    static void  COP1_CF();
    static void  COP1_DMT();

    static Func* BuildInterpreter();
};
