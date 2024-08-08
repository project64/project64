#pragma once
#include <Project64-rsp-core/cpu/RSPOpcode.h>
#include <Project64-rsp-core/cpu/RspTypes.h>

class CRSPSystem;
class CRSPRegisters;

class RSPOp
{
    friend class CRSPSystem;
    friend class CRSPRecompilerOps;

public:
    RSPOp(CRSPSystem & System);
    ~RSPOp();

private:
    RSPOp();
    RSPOp(const RSPOp &);
    RSPOp & operator=(const RSPOp &);

    void BuildInterpreter(void);

    typedef void (RSPOp::*Func)();

    void SPECIAL(void);
    void REGIMM(void);
    void J(void);
    void JAL(void);
    void BEQ(void);
    void BNE(void);
    void BLEZ(void);
    void BGTZ(void);
    void ADDI(void);
    void ADDIU(void);
    void SLTI(void);
    void SLTIU(void);
    void ANDI(void);
    void ORI(void);
    void XORI(void);
    void LUI(void);
    void COP0(void);
    void COP2(void);
    void LB(void);
    void LH(void);
    void LW(void);
    void LBU(void);
    void LHU(void);
    void LWU(void);
    void SB(void);
    void SH(void);
    void SW(void);
    void LC2(void);
    void SC2(void);

    // R4300i Opcodes: Special
    void Special_SLL(void);
    void Special_SRL(void);
    void Special_SRA(void);
    void Special_SLLV(void);
    void Special_SRLV(void);
    void Special_SRAV(void);
    void Special_JR(void);
    void Special_JALR(void);
    void Special_BREAK(void);
    void Special_ADD(void);
    void Special_ADDU(void);
    void Special_SUB(void);
    void Special_SUBU(void);
    void Special_AND(void);
    void Special_OR(void);
    void Special_XOR(void);
    void Special_NOR(void);
    void Special_SLT(void);
    void Special_SLTU(void);

    // R4300i Opcodes: RegImm
    void BLTZ(void);
    void BGEZ(void);
    void BLTZAL(void);
    void BGEZAL(void);

    // COP0 functions
    void Cop0_MF(void);
    void Cop0_MT(void);

    // COP2 functions
    void Cop2_MF(void);
    void Cop2_CF(void);
    void Cop2_MT(void);
    void Cop2_CT(void);
    void Cop2_VECTOR(void);

    // Vector functions
    void Vector_VMULF(void);
    void Vector_VMULU(void);
    void Vector_VRNDP(void);
    void Vector_VMULQ(void);
    void Vector_VMUDL(void);
    void Vector_VMUDM(void);
    void Vector_VMUDN(void);
    void Vector_VMUDH(void);
    void Vector_VMACF(void);
    void Vector_VMACU(void);
    void Vector_VMACQ(void);
    void Vector_VRNDN(void);
    void Vector_VMADL(void);
    void Vector_VMADM(void);
    void Vector_VMADN(void);
    void Vector_VMADH(void);
    void Vector_VADD(void);
    void Vector_VSUB(void);
    void Vector_VABS(void);
    void Vector_VADDC(void);
    void Vector_VSUBC(void);
    void Vector_Reserved(void);
    void Vector_VSAW(void);
    void Vector_VLT(void);
    void Vector_VEQ(void);
    void Vector_VNE(void);
    void Vector_VGE(void);
    void Vector_VCL(void);
    void Vector_VCH(void);
    void Vector_VCR(void);
    void Vector_VMRG(void);
    void Vector_VAND(void);
    void Vector_VNAND(void);
    void Vector_VOR(void);
    void Vector_VNOR(void);
    void Vector_VXOR(void);
    void Vector_VNXOR(void);
    void Vector_VRCP(void);
    void Vector_VRCPL(void);
    void Vector_VRCPH(void);
    void Vector_VMOV(void);
    void Vector_VRSQ(void);
    void Vector_VRSQL(void);
    void Vector_VRSQH(void);
    void Vector_VNOOP(void);

    // LC2 functions
    void LBV(void);
    void LSV(void);
    void LLV(void);
    void LDV(void);
    void LQV(void);
    void LRV(void);
    void LPV(void);
    void LUV(void);
    void LHV(void);
    void LFV(void);
    void LWV(void);
    void LTV(void);

    // LC2 functions
    void SBV(void);
    void SSV(void);
    void SLV(void);
    void SDV(void);
    void SQV(void);
    void SRV(void);
    void SPV(void);
    void SUV(void);
    void SHV(void);
    void SFV(void);
    void STV(void);
    void SWV(void);

    // Other functions
    void UnknownOpcode(void);
    uint32_t BranchIf(bool Condition);

    typedef void (RSPOp::*Func)();

    Func Jump_Opcode[64];
    Func Jump_RegImm[32];
    Func Jump_Special[64];
    Func Jump_Cop0[32];
    Func Jump_Cop2[32];
    Func Jump_Vector[64];
    Func Jump_Lc2[32];
    Func Jump_Sc2[32];

    CRSPSystem & m_System;
    RSPOpcode & m_OpCode;
    CRSPRegisters & m_Reg;
    UWORD32 * m_GPR;
    UDWORD * m_ACCUM;
    UWORD32 * m_Flags;
    RSPVector * m_Vect;
    RSPFlag &VCOL, &VCOH;
    RSPFlag &VCCL, &VCCH;
    RSPFlag & VCE;
};