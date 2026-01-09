#pragma once
#if defined(__amd64__) || defined(_M_X64)

#include "RspRegState.h"
#include <Project64-rsp-core/Recompiler/asmjit.h>
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>

class CRSPSystem;
class CRSPRecompiler;
class RspAssembler;
class RspCodeBlock;

class CRSPRecompilerOps
{
    friend CRspRegState;

    enum
    {
        FunctionStackSize = 40,
    };

    enum class AccumLocation
    {
        High,
        Middle,
        Low,
        Entire,
    };

public:
    CRSPRecompilerOps(CRSPSystem & System, CRSPRecompiler & Recompiler);

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

    void RegImm_BLTZ(void);
    void RegImm_BGEZ(void);
    void RegImm_BLTZAL(void);
    void RegImm_BGEZAL(void);

    // COP0 functions

    void Cop0_MF(void);
    void Cop0_MT(void);

    // COP2 functions

    void Cop2_MF(void);
    void Cop2_CF(void);
    void Cop2_MT(void);
    void Cop2_CT(void);
    void COP2_VECTOR(void);

    // Vector functions

    void Vector_VMULF(void);
    void Vector_VMULU(void);
    void Vector_VRNDN(void);
    void Vector_VRNDP(void);
    void Vector_VMULQ(void);
    void Vector_VMUDL(void);
    void Vector_VMUDM(void);
    void Vector_VMUDN(void);
    void Vector_VMUDH(void);
    void Vector_VMACF(void);
    void Vector_VMACU(void);
    void Vector_VMACQ(void);
    void Vector_VMADL(void);
    void Vector_VMADM(void);
    void Vector_VMADN(void);
    void Vector_VMADH(void);
    void Vector_VADD(void);
    void Vector_VSUB(void);
    void Vector_VABS(void);
    void Vector_VADDC(void);
    void Vector_VSUBC(void);
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
    void Vector_Reserved(void);

    // LC2 functions
    void Opcode_LBV(void);
    void Opcode_LSV(void);
    void Opcode_LLV(void);
    void Opcode_LDV(void);
    void Opcode_LQV(void);
    void Opcode_LRV(void);
    void Opcode_LPV(void);
    void Opcode_LUV(void);
    void Opcode_LHV(void);
    void Opcode_LFV(void);
    void Opcode_LWV(void);
    void Opcode_LTV(void);

    // SC2 functions
    void Opcode_SBV(void);
    void Opcode_SSV(void);
    void Opcode_SLV(void);
    void Opcode_SDV(void);
    void Opcode_SQV(void);
    void Opcode_SRV(void);
    void Opcode_SPV(void);
    void Opcode_SUV(void);
    void Opcode_SHV(void);
    void Opcode_SFV(void);
    void Opcode_SWV(void);
    void Opcode_STV(void);

    // Other functions

    void UnknownOpcode(void);
    bool RSP_DoSections(void);

    void EnterCodeBlock(void);
    void ExitCodeBlock(void);

private:
    void LoadVectorRegister(asmjit::x86::Xmm xmmReg, uint8_t vectorReg, uint8_t e);
    void Cheat_r4300iOpcode(RSPOp::Func FunctAddress, const char * FunctName, bool CommentOp = true);
    bool WriteToVectorDest(uint32_t DestReg, uint32_t PC);
    bool WriteToAccum(AccumLocation Location, uint32_t PC);
    uint32_t GprOffset(uint8_t gpReg) const;
    uint32_t VectorOffset(uint8_t vectorReg) const;
    uint32_t AccumOffset(AccumLocation location) const;
    uint32_t FlagOffset(RspFlags flag) const;

    CRSPSystem & m_System;
    CRSPRecompiler & m_Recompiler;
    RSPOpcode & m_OpCode;
    const uint32_t & m_CompilePC;
    const RspCodeBlock *& m_CurrentBlock;
    RSPPIPELINE_STAGE & m_NextInstruction;
    uint8_t *& m_DMEM;
    CRSPRegisters & m_Reg;
    UWORD32 * m_GPR;
    RSPVector * m_Vect;
    RSPAccumulator & m_ACCUM;
    RSPFlag &m_VCOL, &m_VCOH;
    RSPFlag &m_VCCL, &m_VCCH;
    RSPFlag & m_VCE;
    RspAssembler *& m_Assembler;
    CRspRegState & m_RegState;
    bool m_DelayAffectBranch;
};

typedef void (CRSPRecompilerOps::*p_Recompfunc)(void);

#endif
