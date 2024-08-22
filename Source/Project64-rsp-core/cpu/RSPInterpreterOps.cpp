#include "RSPCpu.h"
#include "RSPInterpreterCPU.h"
#include "RSPRegisters.h"
#include "RspLog.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/RSPDebugger.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>
#include <Project64-rsp-core/cpu/RspClamp.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>
#include <algorithm>
#include <float.h>
#include <math.h>

extern bool AudioHle, GraphicsHle;

uint32_t clz32(uint32_t val)
{
#if defined(__GNUC__)
    return val ? __builtin_clz(val) : 32;
#else
    /* Binary search for the leading one bit.  */
    int cnt = 0;

    if ((val & 0xFFFF0000U) == 0)
    {
        cnt += 16;
        val <<= 16;
    }
    if ((val & 0xFF000000U) == 0)
    {
        cnt += 8;
        val <<= 8;
    }
    if ((val & 0xF0000000U) == 0)
    {
        cnt += 4;
        val <<= 4;
    }
    if ((val & 0xC0000000U) == 0)
    {
        cnt += 2;
        val <<= 2;
    }
    if ((val & 0x80000000U) == 0)
    {
        cnt++;
        val <<= 1;
    }
    if ((val & 0x80000000U) == 0)
    {
        cnt++;
    }
    return cnt;
#endif
}

RSPOp::RSPOp(CRSPSystem & System) :
    m_System(System),
    m_RSPRegisterHandler(System.m_RSPRegisterHandler),
    m_OpCode(System.m_OpCode),
    m_Reg(System.m_Reg),
    m_MI_INTR_REG(System.m_MI_INTR_REG),
    m_SP_PC_REG(System.m_SP_PC_REG),
    m_SP_STATUS_REG(System.m_SP_STATUS_REG),
    m_SP_DMA_FULL_REG(System.m_SP_DMA_FULL_REG),
    m_SP_DMA_BUSY_REG(System.m_SP_DMA_BUSY_REG),
    m_SP_SEMAPHORE_REG(System.m_SP_SEMAPHORE_REG),
    m_DPC_START_REG(System.m_DPC_START_REG),
    m_DPC_END_REG(System.m_DPC_END_REG),
    m_DPC_CURRENT_REG(System.m_DPC_CURRENT_REG),
    m_DPC_STATUS_REG(System.m_DPC_STATUS_REG),
    m_DPC_CLOCK_REG(System.m_DPC_CLOCK_REG),
    m_DMEM(System.m_DMEM),
    m_GPR(System.m_Reg.m_GPR),
    m_ACCUM(System.m_Reg.m_ACCUM),
    m_Flags(System.m_Reg.m_Flags),
    m_Vect(System.m_Reg.m_Vect),
    VCOL(System.m_Reg.VCOL),
    VCOH(System.m_Reg.VCOH),
    VCCL(System.m_Reg.VCCL),
    VCCH(System.m_Reg.VCCH),
    VCE(System.m_Reg.VCE),
    CheckInterrupts(System.CheckInterrupts),
    ProcessRdpList(System.ProcessRdpList)
{
    BuildInterpreter();
}

RSPOp::~RSPOp()
{
}

void RSPOp::BuildInterpreter(void)
{
    Jump_Opcode[0] = &RSPOp::SPECIAL;
    Jump_Opcode[1] = &RSPOp::REGIMM;
    Jump_Opcode[2] = &RSPOp::J;
    Jump_Opcode[3] = &RSPOp::JAL;
    Jump_Opcode[4] = &RSPOp::BEQ;
    Jump_Opcode[5] = &RSPOp::BNE;
    Jump_Opcode[6] = &RSPOp::BLEZ;
    Jump_Opcode[7] = &RSPOp::BGTZ;
    Jump_Opcode[8] = &RSPOp::ADDI;
    Jump_Opcode[9] = &RSPOp::ADDIU;
    Jump_Opcode[10] = &RSPOp::SLTI;
    Jump_Opcode[11] = &RSPOp::SLTIU;
    Jump_Opcode[12] = &RSPOp::ANDI;
    Jump_Opcode[13] = &RSPOp::ORI;
    Jump_Opcode[14] = &RSPOp::XORI;
    Jump_Opcode[15] = &RSPOp::LUI;
    Jump_Opcode[16] = &RSPOp::COP0;
    Jump_Opcode[17] = &RSPOp::UnknownOpcode;
    Jump_Opcode[18] = &RSPOp::COP2;
    Jump_Opcode[19] = &RSPOp::UnknownOpcode;
    Jump_Opcode[20] = &RSPOp::UnknownOpcode;
    Jump_Opcode[21] = &RSPOp::UnknownOpcode;
    Jump_Opcode[22] = &RSPOp::UnknownOpcode;
    Jump_Opcode[23] = &RSPOp::UnknownOpcode;
    Jump_Opcode[24] = &RSPOp::UnknownOpcode;
    Jump_Opcode[25] = &RSPOp::UnknownOpcode;
    Jump_Opcode[26] = &RSPOp::UnknownOpcode;
    Jump_Opcode[27] = &RSPOp::UnknownOpcode;
    Jump_Opcode[28] = &RSPOp::UnknownOpcode;
    Jump_Opcode[29] = &RSPOp::UnknownOpcode;
    Jump_Opcode[30] = &RSPOp::UnknownOpcode;
    Jump_Opcode[31] = &RSPOp::UnknownOpcode;
    Jump_Opcode[32] = &RSPOp::LB;
    Jump_Opcode[33] = &RSPOp::LH;
    Jump_Opcode[34] = &RSPOp::UnknownOpcode;
    Jump_Opcode[35] = &RSPOp::LW;
    Jump_Opcode[36] = &RSPOp::LBU;
    Jump_Opcode[37] = &RSPOp::LHU;
    Jump_Opcode[38] = &RSPOp::UnknownOpcode;
    Jump_Opcode[39] = &RSPOp::LWU;
    Jump_Opcode[40] = &RSPOp::SB;
    Jump_Opcode[41] = &RSPOp::SH;
    Jump_Opcode[42] = &RSPOp::UnknownOpcode;
    Jump_Opcode[43] = &RSPOp::SW;
    Jump_Opcode[44] = &RSPOp::UnknownOpcode;
    Jump_Opcode[45] = &RSPOp::UnknownOpcode;
    Jump_Opcode[46] = &RSPOp::UnknownOpcode;
    Jump_Opcode[47] = &RSPOp::UnknownOpcode;
    Jump_Opcode[48] = &RSPOp::UnknownOpcode;
    Jump_Opcode[49] = &RSPOp::UnknownOpcode;
    Jump_Opcode[50] = &RSPOp::LC2;
    Jump_Opcode[51] = &RSPOp::UnknownOpcode;
    Jump_Opcode[52] = &RSPOp::UnknownOpcode;
    Jump_Opcode[53] = &RSPOp::UnknownOpcode;
    Jump_Opcode[54] = &RSPOp::UnknownOpcode;
    Jump_Opcode[55] = &RSPOp::UnknownOpcode;
    Jump_Opcode[56] = &RSPOp::UnknownOpcode;
    Jump_Opcode[57] = &RSPOp::UnknownOpcode;
    Jump_Opcode[58] = &RSPOp::SC2;
    Jump_Opcode[59] = &RSPOp::UnknownOpcode;
    Jump_Opcode[60] = &RSPOp::UnknownOpcode;
    Jump_Opcode[61] = &RSPOp::UnknownOpcode;
    Jump_Opcode[62] = &RSPOp::UnknownOpcode;
    Jump_Opcode[63] = &RSPOp::UnknownOpcode;

    Jump_Special[0] = &RSPOp::Special_SLL;
    Jump_Special[1] = &RSPOp::UnknownOpcode;
    Jump_Special[2] = &RSPOp::Special_SRL;
    Jump_Special[3] = &RSPOp::Special_SRA;
    Jump_Special[4] = &RSPOp::Special_SLLV;
    Jump_Special[5] = &RSPOp::UnknownOpcode;
    Jump_Special[6] = &RSPOp::Special_SRLV;
    Jump_Special[7] = &RSPOp::Special_SRAV;
    Jump_Special[8] = &RSPOp::Special_JR;
    Jump_Special[9] = &RSPOp::Special_JALR;
    Jump_Special[10] = &RSPOp::UnknownOpcode;
    Jump_Special[11] = &RSPOp::UnknownOpcode;
    Jump_Special[12] = &RSPOp::UnknownOpcode;
    Jump_Special[13] = &RSPOp::Special_BREAK;
    Jump_Special[14] = &RSPOp::UnknownOpcode;
    Jump_Special[15] = &RSPOp::UnknownOpcode;
    Jump_Special[16] = &RSPOp::UnknownOpcode;
    Jump_Special[17] = &RSPOp::UnknownOpcode;
    Jump_Special[18] = &RSPOp::UnknownOpcode;
    Jump_Special[19] = &RSPOp::UnknownOpcode;
    Jump_Special[20] = &RSPOp::UnknownOpcode;
    Jump_Special[21] = &RSPOp::UnknownOpcode;
    Jump_Special[22] = &RSPOp::UnknownOpcode;
    Jump_Special[23] = &RSPOp::UnknownOpcode;
    Jump_Special[24] = &RSPOp::UnknownOpcode;
    Jump_Special[25] = &RSPOp::UnknownOpcode;
    Jump_Special[26] = &RSPOp::UnknownOpcode;
    Jump_Special[27] = &RSPOp::UnknownOpcode;
    Jump_Special[28] = &RSPOp::UnknownOpcode;
    Jump_Special[29] = &RSPOp::UnknownOpcode;
    Jump_Special[30] = &RSPOp::UnknownOpcode;
    Jump_Special[31] = &RSPOp::UnknownOpcode;
    Jump_Special[32] = &RSPOp::Special_ADD;
    Jump_Special[33] = &RSPOp::Special_ADDU;
    Jump_Special[34] = &RSPOp::Special_SUB;
    Jump_Special[35] = &RSPOp::Special_SUBU;
    Jump_Special[36] = &RSPOp::Special_AND;
    Jump_Special[37] = &RSPOp::Special_OR;
    Jump_Special[38] = &RSPOp::Special_XOR;
    Jump_Special[39] = &RSPOp::Special_NOR;
    Jump_Special[40] = &RSPOp::UnknownOpcode;
    Jump_Special[41] = &RSPOp::UnknownOpcode;
    Jump_Special[42] = &RSPOp::Special_SLT;
    Jump_Special[43] = &RSPOp::Special_SLTU;
    Jump_Special[44] = &RSPOp::UnknownOpcode;
    Jump_Special[45] = &RSPOp::UnknownOpcode;
    Jump_Special[46] = &RSPOp::UnknownOpcode;
    Jump_Special[47] = &RSPOp::UnknownOpcode;
    Jump_Special[48] = &RSPOp::UnknownOpcode;
    Jump_Special[49] = &RSPOp::UnknownOpcode;
    Jump_Special[50] = &RSPOp::UnknownOpcode;
    Jump_Special[51] = &RSPOp::UnknownOpcode;
    Jump_Special[52] = &RSPOp::UnknownOpcode;
    Jump_Special[53] = &RSPOp::UnknownOpcode;
    Jump_Special[54] = &RSPOp::UnknownOpcode;
    Jump_Special[55] = &RSPOp::UnknownOpcode;
    Jump_Special[56] = &RSPOp::UnknownOpcode;
    Jump_Special[57] = &RSPOp::UnknownOpcode;
    Jump_Special[58] = &RSPOp::UnknownOpcode;
    Jump_Special[59] = &RSPOp::UnknownOpcode;
    Jump_Special[60] = &RSPOp::UnknownOpcode;
    Jump_Special[61] = &RSPOp::UnknownOpcode;
    Jump_Special[62] = &RSPOp::UnknownOpcode;
    Jump_Special[63] = &RSPOp::UnknownOpcode;

    Jump_RegImm[0] = &RSPOp::BLTZ;
    Jump_RegImm[1] = &RSPOp::BGEZ;
    Jump_RegImm[2] = &RSPOp::UnknownOpcode;
    Jump_RegImm[3] = &RSPOp::UnknownOpcode;
    Jump_RegImm[4] = &RSPOp::UnknownOpcode;
    Jump_RegImm[5] = &RSPOp::UnknownOpcode;
    Jump_RegImm[6] = &RSPOp::UnknownOpcode;
    Jump_RegImm[7] = &RSPOp::UnknownOpcode;
    Jump_RegImm[8] = &RSPOp::UnknownOpcode;
    Jump_RegImm[9] = &RSPOp::UnknownOpcode;
    Jump_RegImm[10] = &RSPOp::UnknownOpcode;
    Jump_RegImm[11] = &RSPOp::UnknownOpcode;
    Jump_RegImm[12] = &RSPOp::UnknownOpcode;
    Jump_RegImm[13] = &RSPOp::UnknownOpcode;
    Jump_RegImm[14] = &RSPOp::UnknownOpcode;
    Jump_RegImm[15] = &RSPOp::UnknownOpcode;
    Jump_RegImm[16] = &RSPOp::BLTZAL;
    Jump_RegImm[17] = &RSPOp::BGEZAL;
    Jump_RegImm[18] = &RSPOp::UnknownOpcode;
    Jump_RegImm[19] = &RSPOp::UnknownOpcode;
    Jump_RegImm[20] = &RSPOp::UnknownOpcode;
    Jump_RegImm[21] = &RSPOp::UnknownOpcode;
    Jump_RegImm[22] = &RSPOp::UnknownOpcode;
    Jump_RegImm[23] = &RSPOp::UnknownOpcode;
    Jump_RegImm[24] = &RSPOp::UnknownOpcode;
    Jump_RegImm[25] = &RSPOp::UnknownOpcode;
    Jump_RegImm[26] = &RSPOp::UnknownOpcode;
    Jump_RegImm[27] = &RSPOp::UnknownOpcode;
    Jump_RegImm[28] = &RSPOp::UnknownOpcode;
    Jump_RegImm[29] = &RSPOp::UnknownOpcode;
    Jump_RegImm[30] = &RSPOp::UnknownOpcode;
    Jump_RegImm[31] = &RSPOp::UnknownOpcode;

    Jump_Cop0[0] = &RSPOp::Cop0_MF;
    Jump_Cop0[1] = &RSPOp::UnknownOpcode;
    Jump_Cop0[2] = &RSPOp::UnknownOpcode;
    Jump_Cop0[3] = &RSPOp::UnknownOpcode;
    Jump_Cop0[4] = &RSPOp::Cop0_MT;
    Jump_Cop0[5] = &RSPOp::UnknownOpcode;
    Jump_Cop0[6] = &RSPOp::UnknownOpcode;
    Jump_Cop0[7] = &RSPOp::UnknownOpcode;
    Jump_Cop0[8] = &RSPOp::UnknownOpcode;
    Jump_Cop0[9] = &RSPOp::UnknownOpcode;
    Jump_Cop0[10] = &RSPOp::UnknownOpcode;
    Jump_Cop0[11] = &RSPOp::UnknownOpcode;
    Jump_Cop0[12] = &RSPOp::UnknownOpcode;
    Jump_Cop0[13] = &RSPOp::UnknownOpcode;
    Jump_Cop0[14] = &RSPOp::UnknownOpcode;
    Jump_Cop0[15] = &RSPOp::UnknownOpcode;
    Jump_Cop0[16] = &RSPOp::UnknownOpcode;
    Jump_Cop0[17] = &RSPOp::UnknownOpcode;
    Jump_Cop0[18] = &RSPOp::UnknownOpcode;
    Jump_Cop0[19] = &RSPOp::UnknownOpcode;
    Jump_Cop0[20] = &RSPOp::UnknownOpcode;
    Jump_Cop0[21] = &RSPOp::UnknownOpcode;
    Jump_Cop0[22] = &RSPOp::UnknownOpcode;
    Jump_Cop0[23] = &RSPOp::UnknownOpcode;
    Jump_Cop0[24] = &RSPOp::UnknownOpcode;
    Jump_Cop0[25] = &RSPOp::UnknownOpcode;
    Jump_Cop0[26] = &RSPOp::UnknownOpcode;
    Jump_Cop0[27] = &RSPOp::UnknownOpcode;
    Jump_Cop0[28] = &RSPOp::UnknownOpcode;
    Jump_Cop0[29] = &RSPOp::UnknownOpcode;
    Jump_Cop0[30] = &RSPOp::UnknownOpcode;
    Jump_Cop0[31] = &RSPOp::UnknownOpcode;

    Jump_Cop2[0] = &RSPOp::Cop2_MF;
    Jump_Cop2[1] = &RSPOp::UnknownOpcode;
    Jump_Cop2[2] = &RSPOp::Cop2_CF;
    Jump_Cop2[3] = &RSPOp::UnknownOpcode;
    Jump_Cop2[4] = &RSPOp::Cop2_MT;
    Jump_Cop2[5] = &RSPOp::UnknownOpcode;
    Jump_Cop2[6] = &RSPOp::Cop2_CT;
    Jump_Cop2[7] = &RSPOp::UnknownOpcode;
    Jump_Cop2[8] = &RSPOp::UnknownOpcode;
    Jump_Cop2[9] = &RSPOp::UnknownOpcode;
    Jump_Cop2[10] = &RSPOp::UnknownOpcode;
    Jump_Cop2[11] = &RSPOp::UnknownOpcode;
    Jump_Cop2[12] = &RSPOp::UnknownOpcode;
    Jump_Cop2[13] = &RSPOp::UnknownOpcode;
    Jump_Cop2[14] = &RSPOp::UnknownOpcode;
    Jump_Cop2[15] = &RSPOp::UnknownOpcode;
    Jump_Cop2[16] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[17] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[18] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[19] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[20] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[21] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[22] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[23] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[24] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[25] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[26] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[27] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[28] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[29] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[30] = &RSPOp::Cop2_VECTOR;
    Jump_Cop2[31] = &RSPOp::Cop2_VECTOR;

    Jump_Vector[0] = &RSPOp::Vector_VMULF;
    Jump_Vector[1] = &RSPOp::Vector_VMULU;
    Jump_Vector[2] = &RSPOp::Vector_VRNDP;
    Jump_Vector[3] = &RSPOp::Vector_VMULQ;
    Jump_Vector[4] = &RSPOp::Vector_VMUDL;
    Jump_Vector[5] = &RSPOp::Vector_VMUDM;
    Jump_Vector[6] = &RSPOp::Vector_VMUDN;
    Jump_Vector[7] = &RSPOp::Vector_VMUDH;
    Jump_Vector[8] = &RSPOp::Vector_VMACF;
    Jump_Vector[9] = &RSPOp::Vector_VMACU;
    Jump_Vector[10] = &RSPOp::Vector_VRNDN;
    Jump_Vector[11] = &RSPOp::Vector_VMACQ;
    Jump_Vector[12] = &RSPOp::Vector_VMADL;
    Jump_Vector[13] = &RSPOp::Vector_VMADM;
    Jump_Vector[14] = &RSPOp::Vector_VMADN;
    Jump_Vector[15] = &RSPOp::Vector_VMADH;
    Jump_Vector[16] = &RSPOp::Vector_VADD;
    Jump_Vector[17] = &RSPOp::Vector_VSUB;
    Jump_Vector[18] = &RSPOp::Vector_Reserved;
    Jump_Vector[19] = &RSPOp::Vector_VABS;
    Jump_Vector[20] = &RSPOp::Vector_VADDC;
    Jump_Vector[21] = &RSPOp::Vector_VSUBC;
    Jump_Vector[22] = &RSPOp::Vector_Reserved;
    Jump_Vector[23] = &RSPOp::Vector_Reserved;
    Jump_Vector[24] = &RSPOp::Vector_Reserved;
    Jump_Vector[25] = &RSPOp::Vector_Reserved;
    Jump_Vector[26] = &RSPOp::Vector_Reserved;
    Jump_Vector[27] = &RSPOp::Vector_Reserved;
    Jump_Vector[28] = &RSPOp::Vector_Reserved;
    Jump_Vector[29] = &RSPOp::Vector_VSAW;
    Jump_Vector[30] = &RSPOp::Vector_Reserved;
    Jump_Vector[31] = &RSPOp::Vector_Reserved;
    Jump_Vector[32] = &RSPOp::Vector_VLT;
    Jump_Vector[33] = &RSPOp::Vector_VEQ;
    Jump_Vector[34] = &RSPOp::Vector_VNE;
    Jump_Vector[35] = &RSPOp::Vector_VGE;
    Jump_Vector[36] = &RSPOp::Vector_VCL;
    Jump_Vector[37] = &RSPOp::Vector_VCH;
    Jump_Vector[38] = &RSPOp::Vector_VCR;
    Jump_Vector[39] = &RSPOp::Vector_VMRG;
    Jump_Vector[40] = &RSPOp::Vector_VAND;
    Jump_Vector[41] = &RSPOp::Vector_VNAND;
    Jump_Vector[42] = &RSPOp::Vector_VOR;
    Jump_Vector[43] = &RSPOp::Vector_VNOR;
    Jump_Vector[44] = &RSPOp::Vector_VXOR;
    Jump_Vector[45] = &RSPOp::Vector_VNXOR;
    Jump_Vector[46] = &RSPOp::Vector_Reserved;
    Jump_Vector[47] = &RSPOp::Vector_Reserved;
    Jump_Vector[48] = &RSPOp::Vector_VRCP;
    Jump_Vector[49] = &RSPOp::Vector_VRCPL;
    Jump_Vector[50] = &RSPOp::Vector_VRCPH;
    Jump_Vector[51] = &RSPOp::Vector_VMOV;
    Jump_Vector[52] = &RSPOp::Vector_VRSQ;
    Jump_Vector[53] = &RSPOp::Vector_VRSQL;
    Jump_Vector[54] = &RSPOp::Vector_VRSQH;
    Jump_Vector[55] = &RSPOp::Vector_VNOOP;
    Jump_Vector[56] = &RSPOp::Vector_Reserved;
    Jump_Vector[57] = &RSPOp::Vector_Reserved;
    Jump_Vector[58] = &RSPOp::Vector_Reserved;
    Jump_Vector[59] = &RSPOp::Vector_Reserved;
    Jump_Vector[60] = &RSPOp::Vector_Reserved;
    Jump_Vector[61] = &RSPOp::Vector_Reserved;
    Jump_Vector[62] = &RSPOp::Vector_Reserved;
    Jump_Vector[63] = &RSPOp::Vector_VNOOP;

    Jump_Lc2[0] = &RSPOp::LBV;
    Jump_Lc2[1] = &RSPOp::LSV;
    Jump_Lc2[2] = &RSPOp::LLV;
    Jump_Lc2[3] = &RSPOp::LDV;
    Jump_Lc2[4] = &RSPOp::LQV;
    Jump_Lc2[5] = &RSPOp::LRV;
    Jump_Lc2[6] = &RSPOp::LPV;
    Jump_Lc2[7] = &RSPOp::LUV;
    Jump_Lc2[8] = &RSPOp::LHV;
    Jump_Lc2[9] = &RSPOp::LFV;
    Jump_Lc2[10] = &RSPOp::LWV;
    Jump_Lc2[11] = &RSPOp::LTV;
    Jump_Lc2[12] = &RSPOp::UnknownOpcode;
    Jump_Lc2[13] = &RSPOp::UnknownOpcode;
    Jump_Lc2[14] = &RSPOp::UnknownOpcode;
    Jump_Lc2[15] = &RSPOp::UnknownOpcode;
    Jump_Lc2[16] = &RSPOp::UnknownOpcode;
    Jump_Lc2[17] = &RSPOp::UnknownOpcode;
    Jump_Lc2[18] = &RSPOp::UnknownOpcode;
    Jump_Lc2[19] = &RSPOp::UnknownOpcode;
    Jump_Lc2[20] = &RSPOp::UnknownOpcode;
    Jump_Lc2[21] = &RSPOp::UnknownOpcode;
    Jump_Lc2[22] = &RSPOp::UnknownOpcode;
    Jump_Lc2[23] = &RSPOp::UnknownOpcode;
    Jump_Lc2[24] = &RSPOp::UnknownOpcode;
    Jump_Lc2[25] = &RSPOp::UnknownOpcode;
    Jump_Lc2[26] = &RSPOp::UnknownOpcode;
    Jump_Lc2[27] = &RSPOp::UnknownOpcode;
    Jump_Lc2[28] = &RSPOp::UnknownOpcode;
    Jump_Lc2[29] = &RSPOp::UnknownOpcode;
    Jump_Lc2[30] = &RSPOp::UnknownOpcode;
    Jump_Lc2[31] = &RSPOp::UnknownOpcode;

    Jump_Sc2[0] = &RSPOp::SBV;
    Jump_Sc2[1] = &RSPOp::SSV;
    Jump_Sc2[2] = &RSPOp::SLV;
    Jump_Sc2[3] = &RSPOp::SDV;
    Jump_Sc2[4] = &RSPOp::SQV;
    Jump_Sc2[5] = &RSPOp::SRV;
    Jump_Sc2[6] = &RSPOp::SPV;
    Jump_Sc2[7] = &RSPOp::SUV;
    Jump_Sc2[8] = &RSPOp::SHV;
    Jump_Sc2[9] = &RSPOp::SFV;
    Jump_Sc2[10] = &RSPOp::SWV;
    Jump_Sc2[11] = &RSPOp::STV;
    Jump_Sc2[12] = &RSPOp::UnknownOpcode;
    Jump_Sc2[13] = &RSPOp::UnknownOpcode;
    Jump_Sc2[14] = &RSPOp::UnknownOpcode;
    Jump_Sc2[15] = &RSPOp::UnknownOpcode;
    Jump_Sc2[16] = &RSPOp::UnknownOpcode;
    Jump_Sc2[17] = &RSPOp::UnknownOpcode;
    Jump_Sc2[18] = &RSPOp::UnknownOpcode;
    Jump_Sc2[19] = &RSPOp::UnknownOpcode;
    Jump_Sc2[20] = &RSPOp::UnknownOpcode;
    Jump_Sc2[21] = &RSPOp::UnknownOpcode;
    Jump_Sc2[22] = &RSPOp::UnknownOpcode;
    Jump_Sc2[23] = &RSPOp::UnknownOpcode;
    Jump_Sc2[24] = &RSPOp::UnknownOpcode;
    Jump_Sc2[25] = &RSPOp::UnknownOpcode;
    Jump_Sc2[26] = &RSPOp::UnknownOpcode;
    Jump_Sc2[27] = &RSPOp::UnknownOpcode;
    Jump_Sc2[28] = &RSPOp::UnknownOpcode;
    Jump_Sc2[29] = &RSPOp::UnknownOpcode;
    Jump_Sc2[30] = &RSPOp::UnknownOpcode;
    Jump_Sc2[31] = &RSPOp::UnknownOpcode;
}

// Opcode functions

void RSPOp::SPECIAL(void)
{
    (this->*Jump_Special[m_OpCode.funct])();
}

void RSPOp::REGIMM(void)
{
    (this->*Jump_RegImm[m_OpCode.rt])();
}

void RSPOp::J(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = (m_OpCode.target << 2) & 0xFFC;
}

void RSPOp::JAL(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    m_GPR[31].UW = (*m_SP_PC_REG + 8) & 0xFFC;
    RSP_JumpTo = (m_OpCode.target << 2) & 0xFFC;
}

void RSPOp::BEQ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W == m_GPR[m_OpCode.rt].W);
}

void RSPOp::BNE(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W != m_GPR[m_OpCode.rt].W);
}

void RSPOp::BLEZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W <= 0);
}

void RSPOp::BGTZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W > 0);
}

void RSPOp::ADDI(void)
{
    m_GPR[m_OpCode.rt].W = m_GPR[m_OpCode.rs].W + (int16_t)m_OpCode.immediate;
}

void RSPOp::ADDIU(void)
{
    m_GPR[m_OpCode.rt].UW = m_GPR[m_OpCode.rs].UW + (uint32_t)((int16_t)m_OpCode.immediate);
}

void RSPOp::SLTI(void)
{
    m_GPR[m_OpCode.rt].W = (m_GPR[m_OpCode.rs].W < (int16_t)m_OpCode.immediate) ? 1 : 0;
}

void RSPOp::SLTIU(void)
{
    m_GPR[m_OpCode.rt].W = (m_GPR[m_OpCode.rs].UW < (uint32_t)(int16_t)m_OpCode.immediate) ? 1 : 0;
}

void RSPOp::ANDI(void)
{
    m_GPR[m_OpCode.rt].W = m_GPR[m_OpCode.rs].W & m_OpCode.immediate;
}

void RSPOp::ORI(void)
{
    m_GPR[m_OpCode.rt].W = m_GPR[m_OpCode.rs].W | m_OpCode.immediate;
}

void RSPOp::XORI(void)
{
    m_GPR[m_OpCode.rt].W = m_GPR[m_OpCode.rs].W ^ m_OpCode.immediate;
}

void RSPOp::LUI(void)
{
    m_GPR[m_OpCode.rt].W = m_OpCode.immediate << 16;
}

void RSPOp::COP0(void)
{
    (this->*Jump_Cop0[m_OpCode.rs])();
}

void RSPOp::COP2(void)
{
    (this->*Jump_Cop2[m_OpCode.rs])();
}

void RSPOp::LB(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    m_GPR[m_OpCode.rt].W = *(int8_t *)(m_DMEM + ((Address ^ 3) & 0xFFF));
}

void RSPOp::LH(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    if ((Address & 0x1) != 0)
    {
        m_GPR[m_OpCode.rt].UHW[0] = *(uint8_t *)(m_DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 8;
        m_GPR[m_OpCode.rt].UHW[0] += *(uint8_t *)(m_DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        m_GPR[m_OpCode.rt].UHW[0] = *(uint16_t *)(m_DMEM + ((Address ^ 2) & 0xFFF));
    }
    m_GPR[m_OpCode.rt].W = m_GPR[m_OpCode.rt].HW[0];
}

void RSPOp::LW(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    if ((Address & 0x3) != 0)
    {
        m_GPR[m_OpCode.rt].UW = *(uint8_t *)(m_DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 24;
        m_GPR[m_OpCode.rt].UW += *(uint8_t *)(m_DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 16;
        m_GPR[m_OpCode.rt].UW += *(uint8_t *)(m_DMEM + (((Address + 2) & 0xFFF) ^ 3)) << 8;
        m_GPR[m_OpCode.rt].UW += *(uint8_t *)(m_DMEM + (((Address + 3) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        m_GPR[m_OpCode.rt].UW = *(uint32_t *)(m_DMEM + (Address & 0xFFF));
    }
}

void RSPOp::LBU(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    m_GPR[m_OpCode.rt].UW = *(uint8_t *)(m_DMEM + ((Address ^ 3) & 0xFFF));
}

void RSPOp::LHU(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    if ((Address & 0x1) != 0)
    {
        m_GPR[m_OpCode.rt].UHW[0] = *(uint8_t *)(m_DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 8;
        m_GPR[m_OpCode.rt].UHW[0] += *(uint8_t *)(m_DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        m_GPR[m_OpCode.rt].UHW[0] = *(uint16_t *)(m_DMEM + ((Address ^ 2) & 0xFFF));
    }
    m_GPR[m_OpCode.rt].UW = m_GPR[m_OpCode.rt].UHW[0];
}

void RSPOp::LWU(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    if ((Address & 0x3) != 0)
    {
        m_GPR[m_OpCode.rt].UW = *(uint8_t *)(m_DMEM + (((Address + 0) & 0xFFF) ^ 3)) << 24;
        m_GPR[m_OpCode.rt].UW += *(uint8_t *)(m_DMEM + (((Address + 1) & 0xFFF) ^ 3)) << 16;
        m_GPR[m_OpCode.rt].UW += *(uint8_t *)(m_DMEM + (((Address + 2) & 0xFFF) ^ 3)) << 8;
        m_GPR[m_OpCode.rt].UW += *(uint8_t *)(m_DMEM + (((Address + 3) & 0xFFF) ^ 3)) << 0;
    }
    else
    {
        m_GPR[m_OpCode.rt].UW = *(uint32_t *)(m_DMEM + (Address & 0xFFF));
    }
}

void RSPOp::SB(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    *(uint8_t *)(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_GPR[m_OpCode.rt].UB[0];
}

void RSPOp::SH(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    if ((Address & 0x1) != 0)
    {
        *(uint8_t *)(m_DMEM + ((Address ^ 3) & 0xFFF)) = (m_GPR[m_OpCode.rt].UHW[0] >> 8);
        *(uint8_t *)(m_DMEM + (((Address + 1) ^ 3) & 0xFFF)) = (m_GPR[m_OpCode.rt].UHW[0] & 0xFF);
    }
    else
    {
        *(uint16_t *)(m_DMEM + ((Address ^ 2) & 0xFFF)) = m_GPR[m_OpCode.rt].UHW[0];
    }
}

void RSPOp::SW(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (short)m_OpCode.offset) & 0xFFF;
    if ((Address & 0x3) != 0)
    {
        *(uint8_t *)(m_DMEM + (((Address + 0) ^ 3) & 0xFFF)) = (m_GPR[m_OpCode.rt].UW >> 24) & 0xFF;
        *(uint8_t *)(m_DMEM + (((Address + 1) ^ 3) & 0xFFF)) = (m_GPR[m_OpCode.rt].UW >> 16) & 0xFF;
        *(uint8_t *)(m_DMEM + (((Address + 2) ^ 3) & 0xFFF)) = (m_GPR[m_OpCode.rt].UW >> 8) & 0xFF;
        *(uint8_t *)(m_DMEM + (((Address + 3) ^ 3) & 0xFFF)) = (m_GPR[m_OpCode.rt].UW >> 0) & 0xFF;
    }
    else
    {
        *(uint32_t *)(m_DMEM + (Address & 0xFFF)) = m_GPR[m_OpCode.rt].UW;
    }
}

void RSPOp::LC2(void)
{
    (this->*Jump_Lc2[m_OpCode.rd])();
}

void RSPOp::SC2(void)
{
    (this->*Jump_Sc2[m_OpCode.rd])();
}

// R4300i Opcodes: Special

void RSPOp::Special_SLL(void)
{
    m_GPR[m_OpCode.rd].W = m_GPR[m_OpCode.rt].W << m_OpCode.sa;
}

void RSPOp::Special_SRL(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rt].UW >> m_OpCode.sa;
}

void RSPOp::Special_SRA(void)
{
    m_GPR[m_OpCode.rd].W = m_GPR[m_OpCode.rt].W >> m_OpCode.sa;
}

void RSPOp::Special_SLLV(void)
{
    m_GPR[m_OpCode.rd].W = m_GPR[m_OpCode.rt].W << (m_GPR[m_OpCode.rs].W & 0x1F);
}

void RSPOp::Special_SRLV(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rt].UW >> (m_GPR[m_OpCode.rs].W & 0x1F);
}

void RSPOp::Special_SRAV(void)
{
    m_GPR[m_OpCode.rd].W = m_GPR[m_OpCode.rt].W >> (m_GPR[m_OpCode.rs].W & 0x1F);
}

void RSPOp::Special_JR(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = (m_GPR[m_OpCode.rs].W & 0xFFC);
}

void RSPOp::Special_JALR(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = (m_GPR[m_OpCode.rs].W & 0xFFC);
    m_GPR[m_OpCode.rd].W = (*m_SP_PC_REG + 8) & 0xFFC;
}

void RSPOp::Special_BREAK(void)
{
    RSP_Running = false;
    *m_SP_STATUS_REG |= (SP_STATUS_HALT | SP_STATUS_BROKE);
    if ((*m_SP_STATUS_REG & SP_STATUS_INTR_BREAK) != 0)
    {
        *m_MI_INTR_REG |= MI_INTR_SP;
        CheckInterrupts();
    }
}

void RSPOp::Special_ADD(void)
{
    m_GPR[m_OpCode.rd].W = m_GPR[m_OpCode.rs].W + m_GPR[m_OpCode.rt].W;
}

void RSPOp::Special_ADDU(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rs].UW + m_GPR[m_OpCode.rt].UW;
}

void RSPOp::Special_SUB(void)
{
    m_GPR[m_OpCode.rd].W = m_GPR[m_OpCode.rs].W - m_GPR[m_OpCode.rt].W;
}

void RSPOp::Special_SUBU(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rs].UW - m_GPR[m_OpCode.rt].UW;
}

void RSPOp::Special_AND(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rs].UW & m_GPR[m_OpCode.rt].UW;
}

void RSPOp::Special_OR(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rs].UW | m_GPR[m_OpCode.rt].UW;
}

void RSPOp::Special_XOR(void)
{
    m_GPR[m_OpCode.rd].UW = m_GPR[m_OpCode.rs].UW ^ m_GPR[m_OpCode.rt].UW;
}

void RSPOp::Special_NOR(void)
{
    m_GPR[m_OpCode.rd].UW = ~(m_GPR[m_OpCode.rs].UW | m_GPR[m_OpCode.rt].UW);
}

void RSPOp::Special_SLT(void)
{
    m_GPR[m_OpCode.rd].UW = (m_GPR[m_OpCode.rs].W < m_GPR[m_OpCode.rt].W) ? 1 : 0;
}

void RSPOp::Special_SLTU(void)
{
    m_GPR[m_OpCode.rd].UW = (m_GPR[m_OpCode.rs].UW < m_GPR[m_OpCode.rt].UW) ? 1 : 0;
}

// R4300i Opcodes: RegImm

void RSPOp::BLTZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W < 0);
}

void RSPOp::BGEZ(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W >= 0);
}

void RSPOp::BLTZAL(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W < 0);
    m_GPR[31].UW = (*m_SP_PC_REG + 8) & 0xFFC;
}

void RSPOp::BGEZAL(void)
{
    RSP_NextInstruction = RSPPIPELINE_DELAY_SLOT;
    RSP_JumpTo = BranchIf(m_GPR[m_OpCode.rs].W >= 0);
    m_GPR[31].UW = (*m_SP_PC_REG + 8) & 0xFFC;
}

// COP0 functions

void RSPOp::Cop0_MF(void)
{
    if (g_RSPDebugger != nullptr)
    {
        g_RSPDebugger->RDP_LogMF0(*m_SP_PC_REG, m_OpCode.rd);
    }
    switch (m_OpCode.rd)
    {
    case 0: m_GPR[m_OpCode.rt].UW = m_RSPRegisterHandler->ReadReg(RSPRegister_MEM_ADDR); break;
    case 1: m_GPR[m_OpCode.rt].UW = m_RSPRegisterHandler->ReadReg(RSPRegister_DRAM_ADDR); break;
    case 2: m_GPR[m_OpCode.rt].UW = m_RSPRegisterHandler->ReadReg(RSPRegister_RD_LEN); break;
    case 3: m_GPR[m_OpCode.rt].UW = m_RSPRegisterHandler->ReadReg(RSPRegister_WR_LEN); break;
    case 4: m_GPR[m_OpCode.rt].UW = m_RSPRegisterHandler->ReadReg(RSPRegister_STATUS); break;
    case 5: m_GPR[m_OpCode.rt].UW = *m_SP_DMA_FULL_REG; break;
    case 6: m_GPR[m_OpCode.rt].UW = *m_SP_DMA_BUSY_REG; break;
    case 7:
        if (RspMultiThreaded)
        {
            m_GPR[m_OpCode.rt].W = *m_SP_SEMAPHORE_REG;
            *m_SP_SEMAPHORE_REG = 1;
        }
        else
        {
            m_GPR[m_OpCode.rt].W = 0;
        }
        break;
    case 8: m_GPR[m_OpCode.rt].UW = *m_DPC_START_REG; break;
    case 9: m_GPR[m_OpCode.rt].UW = *m_DPC_END_REG; break;
    case 10: m_GPR[m_OpCode.rt].UW = *m_DPC_CURRENT_REG; break;
    case 11: m_GPR[m_OpCode.rt].W = *m_DPC_STATUS_REG; break;
    case 12: m_GPR[m_OpCode.rt].W = *m_DPC_CLOCK_REG; break;
    default:
        g_Notify->DisplayError(stdstr_f("We have not implemented RSP MF CP0 reg %s (%d)", COP0_Name(m_OpCode.rd), m_OpCode.rd).c_str());
    }
}

void RSPOp::Cop0_MT(void)
{
    if (LogRDP && g_CPUCore == InterpreterCPU)
    {
        RDPLog.LogMT0(*m_SP_PC_REG, m_OpCode.rd, m_GPR[m_OpCode.rt].UW);
    }
    switch (m_OpCode.rd)
    {
    case 0: m_RSPRegisterHandler->WriteReg(RSPRegister_MEM_ADDR, m_GPR[m_OpCode.rt].UW); break;
    case 1: m_RSPRegisterHandler->WriteReg(RSPRegister_DRAM_ADDR, m_GPR[m_OpCode.rt].UW); break;
    case 2: m_RSPRegisterHandler->WriteReg(RSPRegister_RD_LEN, m_GPR[m_OpCode.rt].UW); break;
    case 3: m_RSPRegisterHandler->WriteReg(RSPRegister_WR_LEN, m_GPR[m_OpCode.rt].UW); break;
    case 4: m_RSPRegisterHandler->WriteReg(RSPRegister_STATUS, m_GPR[m_OpCode.rt].UW); break;
    case 7: *m_SP_SEMAPHORE_REG = 0; break;
    case 8:
        *m_DPC_START_REG = m_GPR[m_OpCode.rt].UW;
        *m_DPC_CURRENT_REG = m_GPR[m_OpCode.rt].UW;
        break;
    case 9:
        *m_DPC_END_REG = m_GPR[m_OpCode.rt].UW;
        RDPLog.LogDlist();
        if (ProcessRdpList != nullptr)
        {
            ProcessRdpList();
        }
        break;
    case 10: *m_DPC_CURRENT_REG = m_GPR[m_OpCode.rt].UW; break;
    case 11:
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_XBUS_DMEM_DMA) != 0)
        {
            *m_DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_SET_XBUS_DMEM_DMA) != 0)
        {
            *m_DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_FREEZE) != 0)
        {
            *m_DPC_STATUS_REG &= ~DPC_STATUS_FREEZE;
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_SET_FREEZE) != 0)
        {
            *m_DPC_STATUS_REG |= DPC_STATUS_FREEZE;
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_FLUSH) != 0)
        {
            *m_DPC_STATUS_REG &= ~DPC_STATUS_FLUSH;
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_SET_FLUSH) != 0)
        {
            *m_DPC_STATUS_REG |= DPC_STATUS_FLUSH;
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_TMEM_CTR) != 0)
        { /* DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_TMEM_CTR"); */
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_PIPE_CTR) != 0)
        {
            g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_PIPE_CTR");
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_CMD_CTR) != 0)
        {
            g_Notify->DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CMD_CTR");
        }
        if ((m_GPR[m_OpCode.rt].W & DPC_CLR_CLOCK_CTR) != 0)
        { /* DisplayError("RSP: DPC_STATUS_REG: DPC_CLR_CLOCK_CTR"); */
        }
        break;
    default:
        g_Notify->DisplayError(stdstr_f("We have not implemented RSP MT CP0 reg %s (%d)", COP0_Name(m_OpCode.rd), m_OpCode.rd).c_str());
    }
}

// COP2 functions

void RSPOp::Cop2_MF(void)
{
    uint8_t element = (uint8_t)(m_OpCode.sa >> 1);
    m_GPR[m_OpCode.rt].B[1] = m_Vect[m_OpCode.vs].s8(15 - element);
    m_GPR[m_OpCode.rt].B[0] = m_Vect[m_OpCode.vs].s8(15 - ((element + 1) % 16));
    m_GPR[m_OpCode.rt].W = m_GPR[m_OpCode.rt].HW[0];
}

void RSPOp::Cop2_CF(void)
{
    switch ((m_OpCode.rd & 0x03))
    {
    case 0: m_GPR[m_OpCode.rt].W = m_Flags[0].HW[0]; break;
    case 1: m_GPR[m_OpCode.rt].W = m_Flags[1].HW[0]; break;
    case 2: m_GPR[m_OpCode.rt].W = m_Flags[2].HW[0]; break;
    case 3: m_GPR[m_OpCode.rt].W = m_Flags[2].HW[0]; break;
    }
}

void RSPOp::Cop2_MT(void)
{
    uint8_t element = (uint8_t)(15 - (m_OpCode.sa >> 1));
    m_Vect[m_OpCode.vs].s8(element) = m_GPR[m_OpCode.rt].B[1];
    if (element != 0)
    {
        m_Vect[m_OpCode.vs].s8(element - 1) = m_GPR[m_OpCode.rt].B[0];
    }
}

void RSPOp::Cop2_CT(void)
{
    switch ((m_OpCode.rd & 0x03))
    {
    case 0: m_Flags[0].HW[0] = m_GPR[m_OpCode.rt].HW[0]; break;
    case 1: m_Flags[1].HW[0] = m_GPR[m_OpCode.rt].HW[0]; break;
    case 2: m_Flags[2].B[0] = m_GPR[m_OpCode.rt].B[0]; break;
    case 3: m_Flags[2].B[0] = m_GPR[m_OpCode.rt].B[0]; break;
    }
}

void RSPOp::Cop2_VECTOR(void)
{
    (this->*Jump_Vector[m_OpCode.funct])();
}

// Vector functions

void RSPOp::Vector_VMULF(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, ((int64_t)m_Vect[m_OpCode.vs].s16(el) * (int64_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e) * 2) + 0x8000);
        Result.s16(el) = m_Reg.AccumulatorSaturate(el, true);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMULU(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, ((int64_t)m_Vect[m_OpCode.vs].s16(el) * (int64_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e) * 2) + 0x8000);
        if (m_ACCUM[el].HW[3] < 0)
        {
            Result.s16(el) = 0;
        }
        else if ((m_ACCUM[el].HW[3] ^ m_ACCUM[el].HW[2]) < 0)
        {
            Result.s16(el) = -1;
        }
        else
        {
            Result.s16(el) = m_ACCUM[el].HW[2];
        }
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VRNDP(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        if (m_OpCode.vs & 1)
        {
            Value <<= 16;
        }
        int64_t Accum = m_Reg.AccumulatorGet(el);
        if (Accum >= 0)
        {
            Accum = clip48(Accum + Value);
        }
        m_Reg.AccumulatorSet(el, Accum);
        Result.s16(el) = clamp16((int32_t)(Accum >> 16));
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMUDL(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, (uint16_t)((uint32_t)m_Vect[m_OpCode.vs].u16(el) * (uint32_t)m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) >> 16));
        Result.s16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMUDM(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, (int32_t)((int32_t)m_Vect[m_OpCode.vs].s16(el) * (uint32_t)m_Vect[m_OpCode.vt].ue(el, m_OpCode.e)));
        Result.s16(el) = m_ACCUM[el].HW[2];
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMULQ(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Temp = m_Vect[m_OpCode.vs].s16(el) * m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        if (Temp < 0)
        {
            Temp += 31;
        }
        m_ACCUM[el].HW[3] = (int16_t)(Temp >> 16);
        m_ACCUM[el].HW[2] = (int16_t)Temp;
        m_ACCUM[el].HW[1] = 0;

        Result.s16(el) = clamp16(Temp >> 1) & ~15;
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMUDN(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, (int32_t)((uint32_t)m_Vect[m_OpCode.vs].u16(el) * (uint32_t)((int32_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e))));
        Result.s16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMUDH(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_ACCUM[el].W[1] = (int32_t)m_Vect[m_OpCode.vs].s16(el) * (int32_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        m_ACCUM[el].HW[1] = 0;
        Result.u16(el) = m_Reg.AccumulatorSaturate(el, true);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMACF(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, m_Reg.AccumulatorGet(el) + (((int64_t)m_Vect[m_OpCode.vs].s16(el) * (int64_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e)) << 1));
        Result.u16(el) = m_Reg.AccumulatorSaturate(el, true);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMACU(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, m_Reg.AccumulatorGet(el) + (((int64_t)m_Vect[m_OpCode.vs].s16(el) * (int64_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e)) << 1));
        if (m_ACCUM[el].HW[3] < 0)
        {
            Result.s16(el) = 0;
        }
        else if (m_ACCUM[el].UHW[3] != 0 || m_ACCUM[el].HW[2] < 0)
        {
            Result.u16(el) = 0xFFFF;
        }
        else
        {
            Result.s16(el) = m_ACCUM[el].HW[2];
        }
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMACQ(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Accum = (m_ACCUM[el].UHW[3] << 16) | m_ACCUM[el].UHW[2];
        if (Accum < -0x20 && ((Accum & 0x20) == 0))
        {
            Accum += 0x20;
        }
        else if (Accum > 0x20 && (Accum & 0x20) == 0)
        {
            Accum -= 0x20;
        }
        Result.u16(el) = clamp16(Accum >> 1) & 0xFFF0;
        m_ACCUM[el].UHW[3] = (uint16_t)(Accum >> 16);
        m_ACCUM[el].UHW[2] = (uint16_t)Accum;
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VRNDN(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        if (m_OpCode.vs & 1)
        {
            Value <<= 16;
        }
        int64_t Accum = m_Reg.AccumulatorGet(el);
        if (Accum < 0)
        {
            Accum = clip48(Accum + Value);
        }
        m_Reg.AccumulatorSet(el, Accum);
        Result.s16(el) = clamp16((int32_t)(Accum >> 16));
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMADL(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, m_Reg.AccumulatorGet(el) + (((uint32_t)(m_Vect[m_OpCode.vs].u16(el)) * (uint32_t)m_Vect[m_OpCode.vt].ue(el, m_OpCode.e)) >> 16));
        Result.u16(el) = m_Reg.AccumulatorSaturate(el, false);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMADM(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, m_Reg.AccumulatorGet(el) + (m_Vect[m_OpCode.vs].s16(el) * m_Vect[m_OpCode.vt].ue(el, m_OpCode.e)));
        Result.u16(el) = m_Reg.AccumulatorSaturate(el, true);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMADN(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_Reg.AccumulatorSet(el, m_Reg.AccumulatorGet(el) + (int64_t)(m_Vect[m_OpCode.vs].u16(el) * m_Vect[m_OpCode.vt].se(el, m_OpCode.e)));
        Result.u16(el) = m_Reg.AccumulatorSaturate(el, false);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VMADH(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = (int32_t)((m_Reg.AccumulatorGet(el) >> 16) + (int32_t)m_Vect[m_OpCode.vs].s16(el) * (int32_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e));
        m_ACCUM[el].HW[3] = (int16_t)(Value >> 16);
        m_ACCUM[el].HW[2] = (int16_t)(Value >> 0);
        Result.u16(el) = m_Reg.AccumulatorSaturate(el, true);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VADD(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = (int32_t)m_Vect[m_OpCode.vs].s16(el) + (int32_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e) + VCOL.Get(el);
        m_ACCUM[el].HW[1] = (int16_t)Value;
        Result.u16(el) = clamp16(Value);
    }
    m_Vect[m_OpCode.vd] = Result;
    VCOL.Clear();
    VCOH.Clear();
}

void RSPOp::Vector_VSUB(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Value = (int32_t)m_Vect[m_OpCode.vs].s16(el) - (int32_t)m_Vect[m_OpCode.vt].se(el, m_OpCode.e) - VCOL.Get(el);
        m_ACCUM[el].HW[1] = (int16_t)Value;
        Result.u16(el) = clamp16(Value);
    }
    m_Vect[m_OpCode.vd] = Result;
    VCOL.Clear();
    VCOH.Clear();
}

void RSPOp::Vector_VABS(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (m_Vect[m_OpCode.vs].s16(el) > 0)
        {
            Result.s16(el) = m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
            m_ACCUM[el].UHW[1] = Result.u16(el);
        }
        else if (m_Vect[m_OpCode.vs].s16(el) < 0)
        {
            if (m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) == 0x8000)
            {
                Result.u16(el) = 0x7FFF;
                m_ACCUM[el].UHW[1] = 0x8000;
            }
            else
            {
                Result.u16(el) = m_Vect[m_OpCode.vt].se(el, m_OpCode.e) * -1;
                m_ACCUM[el].UHW[1] = Result.u16(el);
            }
        }
        else
        {
            Result.u16(el) = 0;
            m_ACCUM[el].UHW[1] = 0;
        }
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VADDC(void)
{
    RSPVector Result;
    VCOH.Clear();
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Temp = (int32_t)m_Vect[m_OpCode.vs].u16(el) + (int32_t)m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
        m_ACCUM[el].HW[1] = (int16_t)Temp;
        Result.u16(el) = m_ACCUM[el].HW[1];
        VCOL.Set(el, (Temp >> 16) != 0);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VSUBC(void)
{
    RSPVector Result;
    VCOH.Clear();
    for (uint8_t el = 0; el < 8; el++)
    {
        int32_t Temp = (int32_t)m_Vect[m_OpCode.vs].u16(el) - (int32_t)m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
        m_ACCUM[el].HW[1] = (int16_t)Temp;
        Result.u16(el) = m_ACCUM[el].HW[1];
        VCOL.Set(el, (Temp >> 16) != 0);
        VCOH.Set(el, Temp != 0);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_Reserved(void)
{
    for (uint8_t el = 0; el < 8; el++)
    {
        m_ACCUM[el].HW[1] = m_Vect[m_OpCode.vs].s16(el) + m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
    }
    m_Vect[m_OpCode.vd] = RSPVector();
}

void RSPOp::Vector_VSAW(void)
{
    RSPVector Result;

    switch ((m_OpCode.rs & 0xF))
    {
    case 8:
        Result.s16(0) = m_ACCUM[0].HW[3];
        Result.s16(1) = m_ACCUM[1].HW[3];
        Result.s16(2) = m_ACCUM[2].HW[3];
        Result.s16(3) = m_ACCUM[3].HW[3];
        Result.s16(4) = m_ACCUM[4].HW[3];
        Result.s16(5) = m_ACCUM[5].HW[3];
        Result.s16(6) = m_ACCUM[6].HW[3];
        Result.s16(7) = m_ACCUM[7].HW[3];
        break;
    case 9:
        Result.s16(0) = m_ACCUM[0].HW[2];
        Result.s16(1) = m_ACCUM[1].HW[2];
        Result.s16(2) = m_ACCUM[2].HW[2];
        Result.s16(3) = m_ACCUM[3].HW[2];
        Result.s16(4) = m_ACCUM[4].HW[2];
        Result.s16(5) = m_ACCUM[5].HW[2];
        Result.s16(6) = m_ACCUM[6].HW[2];
        Result.s16(7) = m_ACCUM[7].HW[2];
        break;
    case 10:
        Result.s16(0) = m_ACCUM[0].HW[1];
        Result.s16(1) = m_ACCUM[1].HW[1];
        Result.s16(2) = m_ACCUM[2].HW[1];
        Result.s16(3) = m_ACCUM[3].HW[1];
        Result.s16(4) = m_ACCUM[4].HW[1];
        Result.s16(5) = m_ACCUM[5].HW[1];
        Result.s16(6) = m_ACCUM[6].HW[1];
        Result.s16(7) = m_ACCUM[7].HW[1];
        break;
    default:
        Result.u64(1) = 0;
        Result.u64(0) = 0;
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VLT(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (m_Vect[m_OpCode.vs].s16(el) < m_Vect[m_OpCode.vt].se(el, m_OpCode.e) || (m_Vect[m_OpCode.vs].s16(el) == m_Vect[m_OpCode.vt].se(el, m_OpCode.e) && VCOL.Get(el) && VCOH.Get(el)))
        {
            Result.u16(el) = m_Vect[m_OpCode.vs].u16(el);
            VCCL.Set(el, true);
        }
        else
        {
            Result.u16(el) = m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
            VCCL.Set(el, false);
        }
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
    VCCH.Clear();
    VCOL.Clear();
    VCOH.Clear();
}

void RSPOp::Vector_VEQ(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_ACCUM[el].HW[1] = VCCL.Set(el, m_Vect[m_OpCode.vs].u16(el) == m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) && !VCOH.Get(el)) ? m_Vect[m_OpCode.vs].u16(el) : m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
        Result.u16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
    VCOL.Clear();
    VCOH.Clear();
    VCCH.Clear();
}

void RSPOp::Vector_VNE(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_ACCUM[el].HW[1] = VCCL.Set(el, m_Vect[m_OpCode.vs].u16(el) != m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) || VCOH.Get(el)) ? m_Vect[m_OpCode.vs].u16(el) : m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
        Result.u16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
    VCCH.Clear();
    VCOL.Clear();
    VCOH.Clear();
}

void RSPOp::Vector_VGE(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (m_Vect[m_OpCode.vs].s16(el) > m_Vect[m_OpCode.vt].se(el, m_OpCode.e) || (m_Vect[m_OpCode.vs].s16(el) == m_Vect[m_OpCode.vt].se(el, m_OpCode.e) && (!VCOL.Get(el) || !VCOH.Get(el))))
        {
            m_ACCUM[el].UHW[1] = m_Vect[m_OpCode.vs].u16(el);
            VCCL.Set(el, true);
        }
        else
        {
            m_ACCUM[el].UHW[1] = m_Vect[m_OpCode.vt].ue(el, m_OpCode.e);
            VCCL.Set(el, false);
        }
        Result.u16(el) = m_ACCUM[el].UHW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
    VCCH.Clear();
    VCOL.Clear();
    VCOH.Clear();
}

void RSPOp::Vector_VCL(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (VCOL.Get(el))
        {
            if (VCOH.Get(el))
            {
                m_ACCUM[el].HW[1] = VCCL.Get(el) ? -m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].s16(el);
            }
            else
            {
                bool Set = VCE.Get(el) ? (m_Vect[m_OpCode.vs].u16(el) + m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) <= 0x10000) : (m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) + m_Vect[m_OpCode.vs].u16(el) == 0);
                m_ACCUM[el].HW[1] = Set ? -m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].s16(el);
                VCCL.Set(el, Set);
            }
        }
        else
        {
            if (VCOH.Get(el))
            {
                m_ACCUM[el].UHW[1] = VCCH.Get(el) ? m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].s16(el);
            }
            else
            {
                m_ACCUM[el].HW[1] = VCCH.Set(el, m_Vect[m_OpCode.vs].u16(el) - m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) >= 0) ? m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].s16(el);
            }
        }
        Result.s16(el) = m_ACCUM[el].HW[1];
    }
    VCOL.Clear();
    VCOH.Clear();
    VCE.Clear();
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VCH(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if (VCOL.Set(el, (m_Vect[m_OpCode.vs].s16(el) ^ m_Vect[m_OpCode.vt].se(el, m_OpCode.e)) < 0))
        {
            int16_t Value = m_Vect[m_OpCode.vs].s16(el) + m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
            m_ACCUM[el].HW[1] = Value <= 0 ? -m_Vect[m_OpCode.vt].se(el, m_OpCode.e) : m_Vect[m_OpCode.vs].s16(el);
            VCOH.Set(el, Value != 0 && m_Vect[m_OpCode.vs].s16(el) != ~m_Vect[m_OpCode.vt].se(el, m_OpCode.e));
            VCCL.Set(el, Value <= 0);
            VCCH.Set(el, m_Vect[m_OpCode.vt].se(el, m_OpCode.e) < 0);
            VCE.Set(el, Value == -1);
        }
        else
        {
            int16_t Value = m_Vect[m_OpCode.vs].s16(el) - m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
            m_ACCUM[el].HW[1] = Value >= 0 ? m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].s16(el);
            VCOH.Set(el, Value != 0 && m_Vect[m_OpCode.vs].s16(el) != ~m_Vect[m_OpCode.vt].se(el, m_OpCode.e));
            VCCL.Set(el, m_Vect[m_OpCode.vt].se(el, m_OpCode.e) < 0);
            VCCH.Set(el, Value >= 0);
            VCE.Set(el, false);
        }
        Result.s16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VCR(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        if ((m_Vect[m_OpCode.vs].s16(el) ^ m_Vect[m_OpCode.vt].se(el, m_OpCode.e)) < 0)
        {
            VCCH.Set(el, m_Vect[m_OpCode.vt].se(el, m_OpCode.e) < 0);
            m_ACCUM[el].HW[1] = VCCL.Set(el, m_Vect[m_OpCode.vs].s16(el) + m_Vect[m_OpCode.vt].se(el, m_OpCode.e) + 1 <= 0) ? ~m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].u16(el);
        }
        else
        {
            VCCL.Set(el, m_Vect[m_OpCode.vt].se(el, m_OpCode.e) < 0);
            m_ACCUM[el].HW[1] = VCCH.Set(el, m_Vect[m_OpCode.vs].s16(el) - m_Vect[m_OpCode.vt].se(el, m_OpCode.e) >= 0) ? m_Vect[m_OpCode.vt].ue(el, m_OpCode.e) : m_Vect[m_OpCode.vs].u16(el);
        }
        Result.s16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
    VCOL.Clear();
    VCOH.Clear();
    VCE.Clear();
}

void RSPOp::Vector_VMRG(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        m_ACCUM[el].HW[1] = VCCL.Get(el) ? m_Vect[m_OpCode.vs].s16(el) : m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        Result.s16(el) = m_ACCUM[el].HW[1];
    }
    m_Vect[m_OpCode.vd] = Result;
    VCOL.Clear();
    VCOH.Clear();
}

void RSPOp::Vector_VAND(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        Result.s16(el) = m_Vect[m_OpCode.vs].s16(el) & m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VNAND(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        Result.s16(el) = ~(m_Vect[m_OpCode.vs].s16(el) & m_Vect[m_OpCode.vt].se(el, m_OpCode.e));
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VOR(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        Result.s16(el) = m_Vect[m_OpCode.vs].s16(el) | m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VNOR(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        Result.s16(el) = ~(m_Vect[m_OpCode.vs].s16(el) | m_Vect[m_OpCode.vt].se(el, m_OpCode.e));
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VXOR(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        Result.s16(el) = m_Vect[m_OpCode.vs].s16(el) ^ m_Vect[m_OpCode.vt].se(el, m_OpCode.e);
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VNXOR(void)
{
    RSPVector Result;
    for (uint8_t el = 0; el < 8; el++)
    {
        Result.s16(el) = ~(m_Vect[m_OpCode.vs].s16(el) ^ m_Vect[m_OpCode.vt].se(el, m_OpCode.e));
        m_ACCUM[el].HW[1] = Result.s16(el);
    }
    m_Vect[m_OpCode.vd] = Result;
}

void RSPOp::Vector_VRCP(void)
{
    int32_t Input = m_Vect[m_OpCode.vt].s16(7 - (m_OpCode.e & 0x7));
    int32_t Mask = Input >> 31;
    int32_t Data = Input ^ Mask;
    if (Input > -32768)
    {
        Data -= Mask;
    }
    int32_t Result = 0;
    if (Data == 0)
    {
        Result = 0x7fffffff;
    }
    else if (Input == 0xFFFF8000)
    {
        Result = 0xffff0000;
    }
    else
    {
        uint32_t Shift = clz32(Data);
        uint32_t Index = (uint64_t(Data) << Shift & 0x7fc00000) >> 22;
        Result = (((0x10000 | m_Reg.m_Reciprocals[Index]) << 14) >> (31 - Shift)) ^ Mask;
    }
    m_Reg.m_High = false;
    m_Reg.m_Result = Result >> 16;
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[i]);
    }
    m_Vect[m_OpCode.vd].s16(7 - (m_OpCode.rd & 0x7)) = (int16_t)Result;
}

void RSPOp::Vector_VRCPL(void)
{
    int32_t Result = 0;
    int32_t Input = m_Reg.m_High ? (m_Reg.m_In << 16 | m_Vect[m_OpCode.vt].u16(7 - (m_OpCode.e & 0x7))) : m_Vect[m_OpCode.vt].s16(7 - (m_OpCode.e & 0x7));
    int32_t Mask = Input >> 31;
    int32_t Data = Input ^ Mask;
    if (Input > -32768)
    {
        Data -= Mask;
    }
    if (Data == 0)
    {
        Result = 0x7fffffff;
    }
    else if (Input == 0xFFFF8000)
    {
        Result = 0xffff0000;
    }
    else
    {
        uint32_t Shift = clz32(Data);
        uint32_t Index = (uint64_t(Data) << Shift & 0x7fc00000) >> 22;
        Result = (((0x10000 | m_Reg.m_Reciprocals[Index]) << 14) >> (31 - Shift)) ^ Mask;
    }
    m_Reg.m_High = false;
    m_Reg.m_Result = Result >> 16;
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[i]);
    }
    m_Vect[m_OpCode.vd].s16(7 - (m_OpCode.rd & 0x7)) = (int16_t)Result;
}

void RSPOp::Vector_VRCPH(void)
{
    m_Reg.m_High = true;
    m_Reg.m_In = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[(m_OpCode.de & 0x7)]);
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[i]);
    }
    m_Vect[m_OpCode.vd].u16(7 - (m_OpCode.de & 0x7)) = m_Reg.m_Result;
}

void RSPOp::Vector_VMOV(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].ue(i, m_OpCode.e);
    }
    uint8_t Index = 7 - (m_OpCode.de & 0x7);
    m_Vect[m_OpCode.vd].u16(Index) = m_Vect[m_OpCode.vt].se(Index, m_OpCode.e);
}

void RSPOp::Vector_VRSQ(void)
{
    int64_t Result = 0;
    int32_t Input = m_Vect[m_OpCode.vt].s16(7 - (m_OpCode.e & 0x7));
    int32_t Mask = Input >> 31;
    int32_t Data = Input ^ Mask;
    if (Input > -32768)
    {
        Data -= Mask;
    }
    if (Data == 0)
    {
        Result = 0x7fffffff;
    }
    else if (Input == 0xFFFF8000)
    {
        Result = 0xffff0000;
    }
    else
    {
        uint32_t Shift = clz32(Data);
        uint32_t Index = (uint64_t(Data) << Shift & 0x7fc00000) >> 22;
        Result = (((0x10000 | m_Reg.m_InverseSquareRoots[(Index & 0x1fe) | (Shift & 1)]) << 14) >> ((31 - Shift) >> 1)) ^ Mask;
    }
    m_Reg.m_High = false;
    m_Reg.m_Result = (int16_t)(Result >> 16);
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].ue(i, m_OpCode.e);
    }
    m_Vect[m_OpCode.vd].s16(7 - (m_OpCode.rd & 0x7)) = (int16_t)Result;
}

void RSPOp::Vector_VRSQL(void)
{
    int32_t Result = 0;
    int32_t Input = m_Reg.m_High ? m_Reg.m_In << 16 | m_Vect[m_OpCode.vt].u16(7 - (m_OpCode.e & 0x7)) : m_Vect[m_OpCode.vt].s16(7 - (m_OpCode.e & 0x7));
    int32_t Mask = Input >> 31;
    int32_t Data = Input ^ Mask;
    if (Input > -32768)
    {
        Data -= Mask;
    }
    if (Data == 0)
    {
        Result = 0x7fffffff;
    }
    else if (Input == 0xFFFF8000)
    {
        Result = 0xffff0000;
    }
    else
    {
        uint32_t Shift = clz32(Data);
        uint32_t Index = (uint64_t(Data) << Shift & 0x7fc00000) >> 22;
        Result = (((0x10000 | m_Reg.m_InverseSquareRoots[(Index & 0x1fe) | (Shift & 1)]) << 14) >> ((31 - Shift) >> 1)) ^ Mask;
    }
    m_Reg.m_High = 0;
    m_Reg.m_Result = Result >> 16;
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[i]);
    }
    m_Vect[m_OpCode.vd].s16(7 - (m_OpCode.rd & 0x7)) = (int16_t)Result;
}

void RSPOp::Vector_VRSQH(void)
{
    m_Reg.m_High = 1;
    m_Reg.m_In = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[(m_OpCode.rd & 0x7)]);
    for (uint8_t i = 0; i < 8; i++)
    {
        m_ACCUM[i].HW[1] = m_Vect[m_OpCode.vt].u16(EleSpec[m_OpCode.e].B[i]);
    }
    m_Vect[m_OpCode.vd].u16(7 - (m_OpCode.rd & 0x7)) = m_Reg.m_Result;
}

void RSPOp::Vector_VNOOP(void)
{
}

// LC2 functions

void RSPOp::LBV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 0)) & 0xFFF;
    m_Vect[m_OpCode.vt].u8((uint8_t)(15 - m_OpCode.del)) = *(m_DMEM + (Address ^ 3));
}

void RSPOp::LSV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 1)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)2, (uint8_t)(16 - m_OpCode.del));
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(Length + m_OpCode.del); i < n; i++, Address++)
    {
        m_Vect[m_OpCode.vt].u8(15 - i) = *(m_DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSPOp::LLV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 2)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)4, (uint8_t)(16 - m_OpCode.del));
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(Length + m_OpCode.del); i < n; i++, Address++)
    {
        m_Vect[m_OpCode.vt].u8(15 - i) = *(m_DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSPOp::LDV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 3)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)8, (uint8_t)(16 - m_OpCode.del));
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(Length + m_OpCode.del); i < n; i++, Address++)
    {
        m_Vect[m_OpCode.vt].u8(15 - i) = *(m_DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSPOp::LQV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4)) & 0xFFF;
    uint8_t Length = std::min((uint8_t)(((Address + 0x10) & ~0xF) - Address), (uint8_t)(16 - m_OpCode.del));
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(Length + m_OpCode.del); i < n; i++, Address++)
    {
        m_Vect[m_OpCode.vt].u8(15 - i) = *(m_DMEM + (Address ^ 3));
    }
}

void RSPOp::LRV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4)) & 0xFFF;
    uint8_t Offset = (uint8_t)((0x10 - (Address & 0xF)) + m_OpCode.del);
    Address &= 0xFF0;
    for (uint8_t i = Offset; i < 16; i++, Address++)
    {
        m_Vect[m_OpCode.vt].u8(15 - i) = *(m_DMEM + ((Address ^ 3) & 0xFFF));
    }
}

void RSPOp::LPV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 3));
    uint32_t Offset = ((Address & 7) - m_OpCode.del);
    Address &= ~7;

    for (uint8_t i = 0; i < 8; i++)
    {
        m_Vect[m_OpCode.vt].u16(7 - i) = *(m_DMEM + ((Address + ((Offset + i) & 0xF) ^ 3) & 0xFFF)) << 8;
    }
}

void RSPOp::LUV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 3));
    uint32_t Offset = ((Address & 7) - m_OpCode.del);
    Address &= ~7;

    for (uint8_t i = 0; i < 8; i++)
    {
        m_Vect[m_OpCode.vt].s16(7 - i) = *(m_DMEM + ((Address + ((Offset + i) & 0xF) ^ 3) & 0xFFF)) << 7;
    }
}

void RSPOp::LHV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4));
    uint32_t Offset = ((Address & 7) - m_OpCode.del);
    Address &= ~7;

    for (uint8_t i = 0; i < 8; i++)
    {
        m_Vect[m_OpCode.vt].s16(7 - i) = *(m_DMEM + ((Address + ((Offset + (i << 1)) & 0xF) ^ 3) & 0xFFF)) << 7;
    }
}

void RSPOp::LFV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4));
    uint8_t Length = std::min((uint8_t)(8 + m_OpCode.del), (uint8_t)16);
    uint32_t Offset = ((Address & 7) - m_OpCode.del);
    Address &= ~7;

    RSPVector Temp;
    for (uint8_t i = 0; i < 4; i++)
    {
        Temp.s16(i) = *(m_DMEM + ((Address + ((Offset + (i << 2)) & 0xF) ^ 3) & 0xFFF)) << 7;
        Temp.s16(i + 4) = *(m_DMEM + ((Address + ((Offset + (i << 2) + 8) & 0xF) ^ 3) & 0xFFF)) << 7;
    }

    for (uint8_t i = m_OpCode.del; i < Length; i++)
    {
        m_Vect[m_OpCode.vt].s8(15 - i) = Temp.s8(i ^ 1);
    }
}

void RSPOp::LWV(void)
{
}

void RSPOp::LTV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4));
    uint32_t Start = Address & ~7;
    uint32_t End = Start + 0x10;
    Address = Start + ((m_OpCode.del + (Address & 8)) & 0xF);
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t del = (((m_OpCode.del >> 1) + i) & 7) + (m_OpCode.rt & ~7);
        m_Vect[del].s8(15 - (i * 2 + 0)) = *(m_DMEM + ((Address++ ^ 3) & 0xFFF));
        if (Address == End)
        {
            Address = Start;
        }
        m_Vect[del].s8(15 - (i * 2 + 1)) = *(m_DMEM + ((Address++ ^ 3) & 0xFFF));
        if (Address == End)
        {
            Address = Start;
        }
    }
}

// SC2 functions

void RSPOp::SBV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 0)) & 0xFFF;
    *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8((uint8_t)(15 - m_OpCode.del));
}

void RSPOp::SSV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 1)) & 0xFFF;
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(2 + m_OpCode.del); i < n; i++, Address++)
    {
        *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - (i & 0xF));
    }
}

void RSPOp::SLV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 2)) & 0xFFF;
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(4 + m_OpCode.del); i < n; i++, Address++)
    {
        *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - (i & 0xF));
    }
}

void RSPOp::SDV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 3)) & 0xFFF;
    for (uint8_t i = m_OpCode.del; i < (8 + m_OpCode.del); i++, Address++)
    {
        *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - (i & 0xF));
    }
}

void RSPOp::SQV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4)) & 0xFFF;
    uint8_t Length = (uint8_t)(((Address + 0x10) & ~0xF) - Address);
    for (uint8_t i = m_OpCode.del; i < (Length + m_OpCode.del); i++, Address++)
    {
        *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - (i & 0xF));
    }
}

void RSPOp::SRV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4)) & 0xFFF;
    uint8_t Length = (Address & 0xF);
    uint8_t Offset = (0x10 - Length) & 0xF;
    Address &= 0xFF0;
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(Length + m_OpCode.del); i < n; i++, Address++)
    {
        *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - ((i + Offset) & 0xF));
    }
}

void RSPOp::SPV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 3)) & 0xFFF;
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(8 + m_OpCode.del); i < n; i++, Address++)
    {
        if (((i)&0xF) < 8)
        {
            *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - ((i & 0xF) << 1));
        }
        else
        {
            *(m_DMEM + ((Address ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u8(15 - ((i & 0x7) << 1)) << 1) + (m_Vect[m_OpCode.vt].u8(14 - ((i & 0x7) << 1)) >> 7);
        }
    }
}

void RSPOp::SUV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 3)) & 0xFFF;
    for (uint8_t Count = m_OpCode.del; Count < (8 + m_OpCode.del); Count++, Address++)
    {
        if (((Count)&0xF) < 8)
        {
            *(m_DMEM + ((Address ^ 3) & 0xFFF)) = ((m_Vect[m_OpCode.vt].u8(15 - ((Count & 0x7) << 1)) << 1) + (m_Vect[m_OpCode.vt].u8(14 - ((Count & 0x7) << 1)) >> 7)) & 0xFF;
        }
        else
        {
            *(m_DMEM + ((Address ^ 3) & 0xFFF)) = m_Vect[m_OpCode.vt].u8(15 - ((Count & 0x7) << 1));
        }
    }
}

void RSPOp::SHV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4));
    uint8_t Offset = Address & 7;
    Address &= ~7;
    for (uint32_t i = 0; i < 16; i += 2)
    {
        uint8_t Value = (m_Vect[m_OpCode.vt].u8(15 - ((m_OpCode.del + i) & 15)) << 1) | (m_Vect[m_OpCode.vt].u8(15 - ((m_OpCode.del + i + 1) & 15)) >> 7);
        *(m_DMEM + ((Address + (Offset + i & 15) ^ 3) & 0xFFF)) = Value;
    }
}

void RSPOp::SFV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4)) & 0xFFF;
    uint8_t Offset = Address & 7;
    Address &= 0xFF8;

    switch (m_OpCode.del)
    {
    case 0:
    case 15:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(7) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(6) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(5) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(4) >> 7) & 0xFF;
        break;
    case 1:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(1) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(0) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(3) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(2) >> 7) & 0xFF;
        break;
    case 4:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(6) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(5) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(4) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(7) >> 7) & 0xFF;
        break;
    case 5:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(0) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(3) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(2) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(1) >> 7) & 0xFF;
        break;
    case 8:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(3) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(2) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(1) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(0) >> 7) & 0xFF;
        break;
    case 11:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(4) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(7) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(6) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(5) >> 7) & 0xFF;
        break;
    case 12:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(2) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(1) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(0) >> 7) & 0xFF;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF)) ^ 3) & 0xFFF)) = (m_Vect[m_OpCode.vt].u16(3) >> 7) & 0xFF;
        break;
    default:
        *(m_DMEM + (((Address + Offset) ^ 3) & 0xFFF)) = 0;
        *(m_DMEM + (((Address + ((Offset + 4) & 0xF)) ^ 3) & 0xFFF)) = 0;
        *(m_DMEM + (((Address + ((Offset + 8) & 0xF) & 0xFFF)) ^ 3)) = 0;
        *(m_DMEM + (((Address + ((Offset + 12) & 0xF) & 0xFFF)) ^ 3)) = 0;
        break;
    }
}

void RSPOp::STV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4));
    uint8_t Element = 16 - (m_OpCode.del & ~1);
    uint8_t Offset = (Address & 7) - (m_OpCode.del & ~1);
    Address &= ~7;
    for (uint32_t i = 0; i < 16; i += 2)
    {
        uint8_t Del = (uint8_t)((m_OpCode.vt & ~7) + (i >> 1));
        *(m_DMEM + (((Address + (Offset + i & 15) ^ 3)) & 0xFFF)) = m_Vect[Del].s8(15 - ((Element + i) & 15));
        *(m_DMEM + (((Address + (Offset + i + 1 & 15) ^ 3)) & 0xFFF)) = m_Vect[Del].s8(15 - ((Element + i + 1) & 15));
    }
}

void RSPOp::SWV(void)
{
    uint32_t Address = (uint32_t)(m_GPR[m_OpCode.base].W + (m_OpCode.voffset << 4)) & 0xFFF;
    uint8_t Offset = Address & 7;
    Address &= 0xFF8;
    for (uint8_t i = m_OpCode.del, n = (uint8_t)(16 + m_OpCode.del); i < n; i++, Offset++)
    {
        *(m_DMEM + ((Address + (Offset & 0xF)) ^ 3)) = m_Vect[m_OpCode.vt].s8(15 - (i & 0xF));
    }
}

// Other functions

void RSPOp::UnknownOpcode(void)
{
    if (g_RSPDebugger != nullptr)
    {
        g_RSPDebugger->UnknownOpcode();
    }
}

uint32_t RSPOp::BranchIf(bool Condition)
{
    return (*m_SP_PC_REG + 4 + (Condition ? ((short)m_OpCode.offset << 2) : 4)) & 0xFFC;
}