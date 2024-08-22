#include "RspRecompilerCPU.h"
#include "RspProfiling.h"
#include "RspRecompilerOps.h"
#include "X86.h"
#include <Common/StdString.h>
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPCpu.h>
#include <Project64-rsp-core/cpu/RSPInstruction.h>
#include <Project64-rsp-core/cpu/RSPInterpreterCPU.h>
#include <Project64-rsp-core/cpu/RSPOpcode.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspLog.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <float.h>
#include <zlib/zlib.h>

#pragma warning(disable : 4152) // Non-standard extension, function/data pointer conversion in expression

// #define REORDER_BLOCK_VERBOSE
#define LINK_BRANCHES_VERBOSE // No choice really
#define X86_RECOMP_VERBOSE
#define BUILD_BRANCHLABELS_VERBOSE

uint32_t CompilePC, JumpTableSize, BlockID = 0;
uint32_t dwBuffer = MainBuffer;
bool ChangedPC;

RSP_CODE RspCode;
RSP_COMPILER Compiler;

uint8_t *pLastSecondary = NULL, *pLastPrimary = NULL;

p_Recompfunc RSP_Recomp_Opcode[64];
p_Recompfunc RSP_Recomp_RegImm[32];
p_Recompfunc RSP_Recomp_Special[64];
p_Recompfunc RSP_Recomp_Cop0[32];
p_Recompfunc RSP_Recomp_Cop2[32];
p_Recompfunc RSP_Recomp_Vector[64];
p_Recompfunc RSP_Recomp_Lc2[32];
p_Recompfunc RSP_Recomp_Sc2[32];

CRSPRecompiler::CRSPRecompiler(CRSPSystem & System) :
    m_System(System),
    m_RSPRegisterHandler(System.m_RSPRegisterHandler),
    m_OpCode(System.m_OpCode),
    m_IMEM(System.m_IMEM)
{
}

void BuildRecompilerCPU(void)
{
    RSP_Recomp_Opcode[0] = &CRSPRecompilerOps::SPECIAL;
    RSP_Recomp_Opcode[1] = &CRSPRecompilerOps::REGIMM;
    RSP_Recomp_Opcode[2] = &CRSPRecompilerOps::J;
    RSP_Recomp_Opcode[3] = &CRSPRecompilerOps::JAL;
    RSP_Recomp_Opcode[4] = &CRSPRecompilerOps::BEQ;
    RSP_Recomp_Opcode[5] = &CRSPRecompilerOps::BNE;
    RSP_Recomp_Opcode[6] = &CRSPRecompilerOps::BLEZ;
    RSP_Recomp_Opcode[7] = &CRSPRecompilerOps::BGTZ;
    RSP_Recomp_Opcode[8] = &CRSPRecompilerOps::ADDI;
    RSP_Recomp_Opcode[9] = &CRSPRecompilerOps::ADDIU;
    RSP_Recomp_Opcode[10] = &CRSPRecompilerOps::SLTI;
    RSP_Recomp_Opcode[11] = &CRSPRecompilerOps::SLTIU;
    RSP_Recomp_Opcode[12] = &CRSPRecompilerOps::ANDI;
    RSP_Recomp_Opcode[13] = &CRSPRecompilerOps::ORI;
    RSP_Recomp_Opcode[14] = &CRSPRecompilerOps::XORI;
    RSP_Recomp_Opcode[15] = &CRSPRecompilerOps::LUI;
    RSP_Recomp_Opcode[16] = &CRSPRecompilerOps::COP0;
    RSP_Recomp_Opcode[17] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[18] = &CRSPRecompilerOps::COP2;
    RSP_Recomp_Opcode[19] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[20] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[21] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[22] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[23] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[24] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[25] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[26] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[27] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[28] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[29] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[30] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[31] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[32] = &CRSPRecompilerOps::LB;
    RSP_Recomp_Opcode[33] = &CRSPRecompilerOps::LH;
    RSP_Recomp_Opcode[34] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[35] = &CRSPRecompilerOps::LW;
    RSP_Recomp_Opcode[36] = &CRSPRecompilerOps::LBU;
    RSP_Recomp_Opcode[37] = &CRSPRecompilerOps::LHU;
    RSP_Recomp_Opcode[38] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[39] = &CRSPRecompilerOps::LWU;
    RSP_Recomp_Opcode[40] = &CRSPRecompilerOps::SB;
    RSP_Recomp_Opcode[41] = &CRSPRecompilerOps::SH;
    RSP_Recomp_Opcode[42] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[43] = &CRSPRecompilerOps::SW;
    RSP_Recomp_Opcode[44] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[45] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[46] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[47] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[48] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[49] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[50] = &CRSPRecompilerOps::LC2;
    RSP_Recomp_Opcode[51] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[52] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[53] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[54] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[55] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[56] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[57] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[58] = &CRSPRecompilerOps::SC2;
    RSP_Recomp_Opcode[59] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[60] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[61] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[62] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Opcode[63] = &CRSPRecompilerOps::UnknownOpcode;

    RSP_Recomp_Special[0] = &CRSPRecompilerOps::Special_SLL;
    RSP_Recomp_Special[1] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[2] = &CRSPRecompilerOps::Special_SRL;
    RSP_Recomp_Special[3] = &CRSPRecompilerOps::Special_SRA;
    RSP_Recomp_Special[4] = &CRSPRecompilerOps::Special_SLLV;
    RSP_Recomp_Special[5] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[6] = &CRSPRecompilerOps::Special_SRLV;
    RSP_Recomp_Special[7] = &CRSPRecompilerOps::Special_SRAV;
    RSP_Recomp_Special[8] = &CRSPRecompilerOps::Special_JR;
    RSP_Recomp_Special[9] = &CRSPRecompilerOps::Special_JALR;
    RSP_Recomp_Special[10] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[11] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[12] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[13] = &CRSPRecompilerOps::Special_BREAK;
    RSP_Recomp_Special[14] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[15] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[16] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[17] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[18] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[19] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[20] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[21] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[22] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[23] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[24] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[25] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[26] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[27] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[28] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[29] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[30] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[31] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[32] = &CRSPRecompilerOps::Special_ADD;
    RSP_Recomp_Special[33] = &CRSPRecompilerOps::Special_ADDU;
    RSP_Recomp_Special[34] = &CRSPRecompilerOps::Special_SUB;
    RSP_Recomp_Special[35] = &CRSPRecompilerOps::Special_SUBU;
    RSP_Recomp_Special[36] = &CRSPRecompilerOps::Special_AND;
    RSP_Recomp_Special[37] = &CRSPRecompilerOps::Special_OR;
    RSP_Recomp_Special[38] = &CRSPRecompilerOps::Special_XOR;
    RSP_Recomp_Special[39] = &CRSPRecompilerOps::Special_NOR;
    RSP_Recomp_Special[40] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[41] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[42] = &CRSPRecompilerOps::Special_SLT;
    RSP_Recomp_Special[43] = &CRSPRecompilerOps::Special_SLTU;
    RSP_Recomp_Special[44] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[45] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[46] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[47] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[48] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[49] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[50] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[51] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[52] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[53] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[54] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[55] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[56] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[57] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[58] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[59] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[60] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[61] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[62] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Special[63] = &CRSPRecompilerOps::UnknownOpcode;

    RSP_Recomp_RegImm[0] = &CRSPRecompilerOps::RegImm_BLTZ;
    RSP_Recomp_RegImm[1] = &CRSPRecompilerOps::RegImm_BGEZ;
    RSP_Recomp_RegImm[2] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[3] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[4] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[5] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[6] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[7] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[8] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[9] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[10] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[11] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[12] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[13] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[14] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[15] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[16] = &CRSPRecompilerOps::RegImm_BLTZAL;
    RSP_Recomp_RegImm[17] = &CRSPRecompilerOps::RegImm_BGEZAL;
    RSP_Recomp_RegImm[18] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[19] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[20] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[21] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[22] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[23] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[24] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[25] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[26] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[27] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[28] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[29] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[30] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_RegImm[31] = &CRSPRecompilerOps::UnknownOpcode;

    RSP_Recomp_Cop0[0] = &CRSPRecompilerOps::Cop0_MF;
    RSP_Recomp_Cop0[1] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[2] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[3] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[4] = &CRSPRecompilerOps::Cop0_MT;
    RSP_Recomp_Cop0[5] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[6] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[7] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[8] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[9] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[10] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[11] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[12] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[13] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[14] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[15] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[16] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[17] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[18] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[19] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[20] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[21] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[22] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[23] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[24] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[25] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[26] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[27] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[28] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[29] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[30] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop0[31] = &CRSPRecompilerOps::UnknownOpcode;

    RSP_Recomp_Cop2[0] = &CRSPRecompilerOps::Cop2_MF;
    RSP_Recomp_Cop2[1] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[2] = &CRSPRecompilerOps::Cop2_CF;
    RSP_Recomp_Cop2[3] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[4] = &CRSPRecompilerOps::Cop2_MT;
    RSP_Recomp_Cop2[5] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[6] = &CRSPRecompilerOps::Cop2_CT;
    RSP_Recomp_Cop2[7] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[8] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[9] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[10] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[11] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[12] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[13] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[14] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[15] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Cop2[16] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[17] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[18] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[19] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[20] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[21] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[22] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[23] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[24] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[25] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[26] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[27] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[28] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[29] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[30] = &CRSPRecompilerOps::COP2_VECTOR;
    RSP_Recomp_Cop2[31] = &CRSPRecompilerOps::COP2_VECTOR;

    RSP_Recomp_Vector[0] = &CRSPRecompilerOps::Vector_VMULF;
    RSP_Recomp_Vector[1] = &CRSPRecompilerOps::Vector_VMULU;
    RSP_Recomp_Vector[2] = &CRSPRecompilerOps::Vector_VRNDP;
    RSP_Recomp_Vector[3] = &CRSPRecompilerOps::Vector_VMULQ;
    RSP_Recomp_Vector[4] = &CRSPRecompilerOps::Vector_VMUDL;
    RSP_Recomp_Vector[5] = &CRSPRecompilerOps::Vector_VMUDM;
    RSP_Recomp_Vector[6] = &CRSPRecompilerOps::Vector_VMUDN;
    RSP_Recomp_Vector[7] = &CRSPRecompilerOps::Vector_VMUDH;
    RSP_Recomp_Vector[8] = &CRSPRecompilerOps::Vector_VMACF;
    RSP_Recomp_Vector[9] = &CRSPRecompilerOps::Vector_VMACU;
    RSP_Recomp_Vector[10] = &CRSPRecompilerOps::Vector_VRNDN;
    RSP_Recomp_Vector[11] = &CRSPRecompilerOps::Vector_VMACQ;
    RSP_Recomp_Vector[12] = &CRSPRecompilerOps::Vector_VMADL;
    RSP_Recomp_Vector[13] = &CRSPRecompilerOps::Vector_VMADM;
    RSP_Recomp_Vector[14] = &CRSPRecompilerOps::Vector_VMADN;
    RSP_Recomp_Vector[15] = &CRSPRecompilerOps::Vector_VMADH;
    RSP_Recomp_Vector[16] = &CRSPRecompilerOps::Vector_VADD;
    RSP_Recomp_Vector[17] = &CRSPRecompilerOps::Vector_VSUB;
    RSP_Recomp_Vector[18] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[19] = &CRSPRecompilerOps::Vector_VABS;
    RSP_Recomp_Vector[20] = &CRSPRecompilerOps::Vector_VADDC;
    RSP_Recomp_Vector[21] = &CRSPRecompilerOps::Vector_VSUBC;
    RSP_Recomp_Vector[22] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[23] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[24] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[25] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[26] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[27] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[28] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[29] = &CRSPRecompilerOps::Vector_VSAW;
    RSP_Recomp_Vector[30] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[31] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[32] = &CRSPRecompilerOps::Vector_VLT;
    RSP_Recomp_Vector[33] = &CRSPRecompilerOps::Vector_VEQ;
    RSP_Recomp_Vector[34] = &CRSPRecompilerOps::Vector_VNE;
    RSP_Recomp_Vector[35] = &CRSPRecompilerOps::Vector_VGE;
    RSP_Recomp_Vector[36] = &CRSPRecompilerOps::Vector_VCL;
    RSP_Recomp_Vector[37] = &CRSPRecompilerOps::Vector_VCH;
    RSP_Recomp_Vector[38] = &CRSPRecompilerOps::Vector_VCR;
    RSP_Recomp_Vector[39] = &CRSPRecompilerOps::Vector_VMRG;
    RSP_Recomp_Vector[40] = &CRSPRecompilerOps::Vector_VAND;
    RSP_Recomp_Vector[41] = &CRSPRecompilerOps::Vector_VNAND;
    RSP_Recomp_Vector[42] = &CRSPRecompilerOps::Vector_VOR;
    RSP_Recomp_Vector[43] = &CRSPRecompilerOps::Vector_VNOR;
    RSP_Recomp_Vector[44] = &CRSPRecompilerOps::Vector_VXOR;
    RSP_Recomp_Vector[45] = &CRSPRecompilerOps::Vector_VNXOR;
    RSP_Recomp_Vector[46] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[47] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[48] = &CRSPRecompilerOps::Vector_VRCP;
    RSP_Recomp_Vector[49] = &CRSPRecompilerOps::Vector_VRCPL;
    RSP_Recomp_Vector[50] = &CRSPRecompilerOps::Vector_VRCPH;
    RSP_Recomp_Vector[51] = &CRSPRecompilerOps::Vector_VMOV;
    RSP_Recomp_Vector[52] = &CRSPRecompilerOps::Vector_VRSQ;
    RSP_Recomp_Vector[53] = &CRSPRecompilerOps::Vector_VRSQL;
    RSP_Recomp_Vector[54] = &CRSPRecompilerOps::Vector_VRSQH;
    RSP_Recomp_Vector[55] = &CRSPRecompilerOps::Vector_VNOOP;
    RSP_Recomp_Vector[56] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[57] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[58] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[59] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[60] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[61] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[62] = &CRSPRecompilerOps::Vector_Reserved;
    RSP_Recomp_Vector[63] = &CRSPRecompilerOps::Vector_VNOOP;

    RSP_Recomp_Lc2[0] = &CRSPRecompilerOps::Opcode_LBV;
    RSP_Recomp_Lc2[1] = &CRSPRecompilerOps::Opcode_LSV;
    RSP_Recomp_Lc2[2] = &CRSPRecompilerOps::Opcode_LLV;
    RSP_Recomp_Lc2[3] = &CRSPRecompilerOps::Opcode_LDV;
    RSP_Recomp_Lc2[4] = &CRSPRecompilerOps::Opcode_LQV;
    RSP_Recomp_Lc2[5] = &CRSPRecompilerOps::Opcode_LRV;
    RSP_Recomp_Lc2[6] = &CRSPRecompilerOps::Opcode_LPV;
    RSP_Recomp_Lc2[7] = &CRSPRecompilerOps::Opcode_LUV;
    RSP_Recomp_Lc2[8] = &CRSPRecompilerOps::Opcode_LHV;
    RSP_Recomp_Lc2[9] = &CRSPRecompilerOps::Opcode_LFV;
    RSP_Recomp_Lc2[10] = &CRSPRecompilerOps::Opcode_LWV;
    RSP_Recomp_Lc2[11] = &CRSPRecompilerOps::Opcode_LTV;
    RSP_Recomp_Lc2[12] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[13] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[14] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[15] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[16] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[17] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[18] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[19] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[20] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[21] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[22] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[23] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[24] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[25] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[26] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[27] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[28] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[29] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[30] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Lc2[31] = &CRSPRecompilerOps::UnknownOpcode;

    RSP_Recomp_Sc2[0] = &CRSPRecompilerOps::Opcode_SBV;
    RSP_Recomp_Sc2[1] = &CRSPRecompilerOps::Opcode_SSV;
    RSP_Recomp_Sc2[2] = &CRSPRecompilerOps::Opcode_SLV;
    RSP_Recomp_Sc2[3] = &CRSPRecompilerOps::Opcode_SDV;
    RSP_Recomp_Sc2[4] = &CRSPRecompilerOps::Opcode_SQV;
    RSP_Recomp_Sc2[5] = &CRSPRecompilerOps::Opcode_SRV;
    RSP_Recomp_Sc2[6] = &CRSPRecompilerOps::Opcode_SPV;
    RSP_Recomp_Sc2[7] = &CRSPRecompilerOps::Opcode_SUV;
    RSP_Recomp_Sc2[8] = &CRSPRecompilerOps::Opcode_SHV;
    RSP_Recomp_Sc2[9] = &CRSPRecompilerOps::Opcode_SFV;
    RSP_Recomp_Sc2[10] = &CRSPRecompilerOps::Opcode_SWV;
    RSP_Recomp_Sc2[11] = &CRSPRecompilerOps::Opcode_STV;
    RSP_Recomp_Sc2[12] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[13] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[14] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[15] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[16] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[17] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[18] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[19] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[20] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[21] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[22] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[23] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[24] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[25] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[26] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[27] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[28] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[29] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[30] = &CRSPRecompilerOps::UnknownOpcode;
    RSP_Recomp_Sc2[31] = &CRSPRecompilerOps::UnknownOpcode;

    BlockID = 0;
    ChangedPC = false;
#ifdef Log_x86Code
    Start_x86_Log();
#endif
}

/*
ReOrderSubBlock
Description:
This can be done, but will be interesting to put
between branches labels, and actual branches, whichever
occurs first in code
*/

void CRSPRecompiler::ReOrderInstructions(uint32_t StartPC, uint32_t EndPC)
{
    uint32_t InstructionCount = EndPC - StartPC;
    uint32_t Count, ReorderedOps, CurrentPC;
    RSPOpcode PreviousOp, CurrentOp, RspOp;

    PreviousOp.Value = *(uint32_t *)(m_IMEM + (StartPC & 0xFFC));

    if (IsOpcodeBranch(StartPC, PreviousOp))
    {
        // The sub block ends here anyway
        return;
    }

    if (IsOpcodeNop(StartPC) && IsOpcodeNop(StartPC + 4) && IsOpcodeNop(StartPC + 8))
    {
        // Don't even bother
        return;
    }

    CPU_Message("***** Doing reorder (%X to %X) *****", StartPC, EndPC);

    if (InstructionCount < 0x0010)
    {
        return;
    }
    if (InstructionCount > 0x0A00)
    {
        return;
    }

    CPU_Message(" Before:");
    for (Count = StartPC; Count < EndPC; Count += 4)
    {
        RSP_LW_IMEM(Count, &RspOp.Value);
        CPU_Message("  %X %s", Count, RSPInstruction(Count, RspOp.Value).NameAndParam().c_str());
    }

    for (Count = 0; Count < InstructionCount; Count += 4)
    {
        CurrentPC = StartPC;
        PreviousOp.Value = *(uint32_t *)(m_IMEM + (CurrentPC & 0xFFC));
        ReorderedOps = 0;

        for (;;)
        {
            CurrentPC += 4;
            if (CurrentPC >= EndPC)
            {
                break;
            }
            CurrentOp.Value = *(uint32_t *)(m_IMEM + CurrentPC);

            if (CompareInstructions(CurrentPC, &PreviousOp, &CurrentOp))
            {
                // Move current opcode up
                *(uint32_t *)(m_IMEM + CurrentPC - 4) = CurrentOp.Value;
                *(uint32_t *)(m_IMEM + CurrentPC) = PreviousOp.Value;

                ReorderedOps++;

#ifdef REORDER_BLOCK_VERBOSE
                CPU_Message("Swapped %X and %X", CurrentPC - 4, CurrentPC);
#endif
            }
            PreviousOp.Value = *(uint32_t *)(m_IMEM + (CurrentPC & 0xFFC));

            if (IsOpcodeNop(CurrentPC) && IsOpcodeNop(CurrentPC + 4) && IsOpcodeNop(CurrentPC + 8))
            {
                CurrentPC = EndPC;
            }
        }

        if (ReorderedOps == 0)
        {
            Count = InstructionCount;
        }
    }

    CPU_Message(" After:");
    for (Count = StartPC; Count < EndPC; Count += 4)
    {
        RSP_LW_IMEM(Count, &RspOp.Value);
        CPU_Message("  %X %s", Count, RSPInstruction(Count, RspOp.Value).NameAndParam().c_str());
    }
    CPU_Message("");
}

void CRSPRecompiler::ReOrderSubBlock(RSP_BLOCK * Block)
{
    uint32_t end = 0x0ffc;
    uint32_t count;

    if (!Compiler.bReOrdering)
    {
        return;
    }
    if (Block->CurrPC > 0xFF0)
    {
        return;
    }

    // Find the label or jump closest to us
    if (RspCode.LabelCount)
    {
        for (count = 0; count < RspCode.LabelCount; count++)
        {
            if (RspCode.BranchLabels[count] < end && RspCode.BranchLabels[count] > Block->CurrPC)
            {
                end = RspCode.BranchLabels[count];
            }
        }
    }
    if (RspCode.BranchCount)
    {
        for (count = 0; count < RspCode.BranchCount; count++)
        {
            if (RspCode.BranchLocations[count] < end && RspCode.BranchLocations[count] > Block->CurrPC)
            {
                end = RspCode.BranchLocations[count];
            }
        }
    }
    // It wont actually re-order the op at the end
    ReOrderInstructions(Block->CurrPC, end);
}

void CRSPRecompiler::ResetJumpTables(void)
{
    extern uint32_t NoOfMaps;
    extern uint8_t * JumpTables;

    memset(JumpTables, 0, 0x1000 * MaxMaps);
    RecompPos = RecompCode;
    pLastPrimary = nullptr;
    pLastSecondary = nullptr;
    NoOfMaps = 0;
}

/*
DetectGPRConstants
Description:
This needs to be called on a sub-block basis, like
after every time we hit a branch and delay slot
*/

void DetectGPRConstants(RSP_CODE * code)
{
    uint32_t Count, Constant = 0;

    memset(&code->bIsRegConst, 0, sizeof(bool) * 0x20);
    memset(&code->MipsRegConst, 0, sizeof(uint32_t) * 0x20);

    if (!Compiler.bGPRConstants)
    {
        return;
    }
    CPU_Message("***** Detecting constants *****");

    // R0 is constant zero, R31 or RA is not constant
    code->bIsRegConst[0] = true;
    code->MipsRegConst[0] = 0;

    // Do your global search for them
    for (Count = 1; Count < 31; Count++)
    {
        if (IsRegisterConstant(Count, &Constant))
        {
            CPU_Message("Global: %s is a constant of: %08X", GPR_Name(Count), Constant);
            code->bIsRegConst[Count] = true;
            code->MipsRegConst[Count] = Constant;
        }
    }
    CPU_Message("");
}

/*
CompilerToggleBuffer and ClearX86Code
Description:
1: Toggles the compiler buffer, useful for poorly
taken branches like alignment
2: Clears all the x86 code, jump tables etc.
*/

void CompilerToggleBuffer(void)
{
    if (dwBuffer == MainBuffer)
    {
        dwBuffer = SecondaryBuffer;
        pLastPrimary = RecompPos;

        if (pLastSecondary == NULL)
        {
            pLastSecondary = RecompCodeSecondary;
        }

        RecompPos = pLastSecondary;
        CPU_Message("   (Secondary buffer active 0x%08X)", pLastSecondary);
    }
    else
    {
        dwBuffer = MainBuffer;
        pLastSecondary = RecompPos;

        if (pLastPrimary == NULL)
        {
            pLastPrimary = RecompCode;
        }

        RecompPos = pLastPrimary;
        CPU_Message("   (Primary buffer active 0x%08X)", pLastPrimary);
    }
}

void ClearAllx86Code(void)
{
    extern uint32_t NoOfMaps, MapsCRC[32];
    extern uint8_t * JumpTables;

    memset(&MapsCRC, 0, sizeof(uint32_t) * 0x20);
    NoOfMaps = 0;
    memset(JumpTables, 0, 0x1000 * 32);

    RecompPos = RecompCode;

    pLastPrimary = NULL;
    pLastSecondary = NULL;
}

/*
Link branches
Description:
Resolves all the collected branches, x86 style
*/

void CRSPRecompiler::LinkBranches(RSP_BLOCK * Block)
{
    uint32_t OrigPrgCount = *m_System.m_SP_PC_REG;
    uint32_t Count, Target;
    uint32_t * JumpWord;
    uint8_t * X86Code;
    RSP_BLOCK Save;

    if (!m_CurrentBlock.ResolveCount)
    {
        return;
    }
    CPU_Message("***** Linking branches (%i) *****", m_CurrentBlock.ResolveCount);

    for (Count = 0; Count < m_CurrentBlock.ResolveCount; Count++)
    {
        Target = m_CurrentBlock.BranchesToResolve[Count].TargetPC;
        X86Code = (uint8_t *)*(JumpTable + (Target >> 2));

        if (!X86Code)
        {
            *m_System.m_SP_PC_REG = Target;
            CPU_Message("");
            CPU_Message("===== (Generate code: %04X) =====", Target);
            Save = *Block;

            // Compile this block and link
            CompilerRSPBlock();
            LinkBranches(Block);

            *Block = Save;
            CPU_Message("===== (End generate code: %04X) =====", Target);
            CPU_Message("");
            X86Code = (uint8_t *)*(JumpTable + (Target >> 2));
        }

        JumpWord = m_CurrentBlock.BranchesToResolve[Count].X86JumpLoc;
        x86_SetBranch32b(JumpWord, (uint32_t *)X86Code);

        CPU_Message("Linked RSP branch from x86: %08X, to RSP: %X / x86: %08X",
                    JumpWord, Target, X86Code);
    }
    *m_System.m_SP_PC_REG = OrigPrgCount;
    CPU_Message("***** Done linking branches *****");
    CPU_Message("");
}

/*
BuildBranchLabels
Description:
Branch labels are used to start and stop re-ordering
sections as well as set the jump table to points
within a block that are safe.
*/

void BuildBranchLabels(void)
{
    RSPOpcode RspOp;
    uint32_t i, Dest;

#ifdef BUILD_BRANCHLABELS_VERBOSE
    CPU_Message("***** Building branch labels *****");
#endif

    for (i = 0; i < 0x1000; i += 4)
    {
        RspOp.Value = *(uint32_t *)(RSPInfo.IMEM + i);

        if (IsOpcodeBranch(i, RspOp))
        {
            if (RspCode.LabelCount >= (sizeof(RspCode.BranchLabels) / sizeof(RspCode.BranchLabels[0])) - 1)
            {
                CompilerWarning("Out of space for branch labels");
                return;
            }

            if (RspCode.BranchCount >= (sizeof(RspCode.BranchLocations) / sizeof(RspCode.BranchLocations[0])) - 1)
            {
                CompilerWarning("Out of space for branch locations");
                return;
            }
            RspCode.BranchLocations[RspCode.BranchCount++] = i;
            if (RspOp.op == RSP_SPECIAL)
            {
                // Register jump not predictable
            }
            else if (RspOp.op == RSP_J || RspOp.op == RSP_JAL)
            {
                // For JAL its a sub-block for returns
                Dest = (RspOp.target << 2) & 0xFFC;
                RspCode.BranchLabels[RspCode.LabelCount] = Dest;
                RspCode.LabelCount += 1;
#ifdef BUILD_BRANCHLABELS_VERBOSE
                CPU_Message("[%02i] Added branch at %X to %X", RspCode.LabelCount, i, Dest);
#endif
            }
            else
            {
                Dest = (i + ((short)RspOp.offset << 2) + 4) & 0xFFC;
                RspCode.BranchLabels[RspCode.LabelCount] = Dest;
                RspCode.LabelCount += 1;
#ifdef BUILD_BRANCHLABELS_VERBOSE
                CPU_Message("[%02i] Added branch at %X to %X", RspCode.LabelCount, i, Dest);
#endif
            }
        }
    }

#ifdef BUILD_BRANCHLABELS_VERBOSE
    CPU_Message("***** End branch labels *****");
    CPU_Message("");
#endif
}

bool IsJumpLabel(uint32_t PC)
{
    uint32_t Count;

    if (!RspCode.LabelCount)
    {
        return false;
    }

    for (Count = 0; Count < RspCode.LabelCount; Count++)
    {
        if (PC == RspCode.BranchLabels[Count])
        {
            return true;
        }
    }
    return false;
}

void CompilerLinkBlocks(void)
{
    uint8_t * KnownCode = (uint8_t *)*(JumpTable + (CompilePC >> 2));

    CPU_Message("***** Linking block to X86: %08X *****", KnownCode);
    NextInstruction = RSPPIPELINE_FINISH_BLOCK;

    // Block linking scenario
    JmpLabel32("Linked block", 0);
    x86_SetBranch32b(RecompPos - 4, KnownCode);
}

void CRSPRecompiler::CompilerRSPBlock(void)
{
    CRSPRecompilerOps RecompilerOps(RSPSystem, *this);

    uint8_t * IMEM_SAVE = (uint8_t *)malloc(0x1000);
    const size_t X86BaseAddress = (size_t)RecompPos;
    NextInstruction = RSPPIPELINE_NORMAL;
    CompilePC = *m_System.m_SP_PC_REG;

    memset(&m_CurrentBlock, 0, sizeof(m_CurrentBlock));
    m_CurrentBlock.StartPC = CompilePC;
    m_CurrentBlock.CurrPC = CompilePC;

    // Align the block to a boundary
    if (X86BaseAddress & 7)
    {
        size_t Count;
        const size_t Padding = (8 - (X86BaseAddress & 7)) & 7;

        for (Count = 0; Count < Padding; Count++)
        {
            CPU_Message("%08X: nop", RecompPos);
            *(RecompPos++) = 0x90;
        }
    }

    CPU_Message("====== Block %d ======", BlockID++);
    CPU_Message("x86 code at: %X", RecompPos);
    CPU_Message("Jump table: %X", Table);
    CPU_Message("Start of block: %X", m_CurrentBlock.StartPC);
    CPU_Message("====== Recompiled code ======");

    if (Compiler.bReOrdering)
    {
        memcpy(IMEM_SAVE, RSPInfo.IMEM, 0x1000);
        ReOrderSubBlock(&m_CurrentBlock);
    }

    // This is for the block about to be compiled
    *(JumpTable + (CompilePC >> 2)) = RecompPos;

    uint32_t EndPC = 0x1000;
    do
    {
        // Reordering is setup to allow us to have loop labels
        // so here we see if this is one and put it in the jump table

        if (NextInstruction == RSPPIPELINE_NORMAL && IsJumpLabel(CompilePC))
        {
            // Jumps come around twice
            if (NULL == *(JumpTable + (CompilePC >> 2)))
            {
                CPU_Message("***** Adding jump table entry for PC: %04X at X86: %08X *****", CompilePC, RecompPos);
                CPU_Message("");
                *(JumpTable + (CompilePC >> 2)) = RecompPos;

                // Reorder from here to next label or branch
                m_CurrentBlock.CurrPC = CompilePC;
                ReOrderSubBlock(&m_CurrentBlock);
            }
            else if (NextInstruction != RSPPIPELINE_DELAY_SLOT_DONE)
            {

                // We could link the blocks here, but performance-wise it might be better to just let it run
            }
        }

        if (Compiler.bSections)
        {
            if (RecompilerOps.RSP_DoSections())
            {
                continue;
            }
        }

#ifdef X86_RECOMP_VERBOSE
        if (!IsOpcodeNop(CompilePC))
        {
            CPU_Message("X86 Address: %08X", RecompPos);
        }
#endif
        RSP_LW_IMEM(CompilePC, &m_OpCode.Value);

        if (m_OpCode.Value == 0xFFFFFFFF)
        {
            // I think this pops up an unknown OP dialog
            // NextInstruction = RSPPIPELINE_FINISH_BLOCK;
        }
        else
        {
            (RecompilerOps.*RSP_Recomp_Opcode[m_OpCode.op])();
        }

        switch (NextInstruction)
        {
        case RSPPIPELINE_NORMAL:
            CompilePC += 4;
            break;
        case RSPPIPELINE_DO_DELAY_SLOT:
            NextInstruction = RSPPIPELINE_DELAY_SLOT;
            CompilePC += 4;
            break;
        case RSPPIPELINE_DELAY_SLOT:
            NextInstruction = RSPPIPELINE_DELAY_SLOT_DONE;
            CompilePC = (CompilePC - 4 & 0xFFC);
            break;
        case RSPPIPELINE_DELAY_SLOT_EXIT:
            NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT_DONE;
            CompilePC = (CompilePC - 4 & 0xFFC);
            break;
        case RSPPIPELINE_FINISH_SUB_BLOCK:
            NextInstruction = RSPPIPELINE_NORMAL;
            CompilePC += 8;
            if (CompilePC >= 0x1000)
            {
                NextInstruction = RSPPIPELINE_FINISH_BLOCK;
            }
            else if (NULL == *(JumpTable + (CompilePC >> 2)))
            {
                // This is for the new block being compiled now
                CPU_Message("***** Continuing static SubBlock (jump table entry added for PC: %04X at X86: %08X) *****", CompilePC, RecompPos);
                *(JumpTable + (CompilePC >> 2)) = RecompPos;

                m_CurrentBlock.CurrPC = CompilePC;
                // Reorder from after delay to next label or branch
                ReOrderSubBlock(&m_CurrentBlock);
            }
            else
            {
                CompilerLinkBlocks();
            }
            break;

        case RSPPIPELINE_FINISH_BLOCK: break;
        default:
            g_Notify->DisplayError(stdstr_f("RSP main loop\n\nWTF NextInstruction = %d", NextInstruction).c_str());
            CompilePC += 4;
            break;
        }

        if (CompilePC >= EndPC && *m_System.m_SP_PC_REG != 0 && EndPC != *m_System.m_SP_PC_REG)
        {
            CompilePC = 0;
            EndPC = *m_System.m_SP_PC_REG;
        }
    } while (NextInstruction != RSPPIPELINE_FINISH_BLOCK && (CompilePC < EndPC || NextInstruction == RSPPIPELINE_DELAY_SLOT || NextInstruction == RSPPIPELINE_DELAY_SLOT_DONE));
    if (CompilePC >= EndPC)
    {
        MoveConstToVariable((CompilePC & 0xFFC), m_System.m_SP_PC_REG, "RSP PC");
        Ret();
    }
    CPU_Message("===== End of recompiled code =====");

    if (Compiler.bReOrdering)
    {
        memcpy(RSPInfo.IMEM, IMEM_SAVE, 0x1000);
    }
    free(IMEM_SAVE);
}

void CRSPRecompiler::RunCPU(void)
{
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif

    uint8_t * Block;

    RSP_Running = true;
    SetJumpTable(JumpTableSize);

    while (RSP_Running)
    {
        Block = (uint8_t *)*(JumpTable + (*m_System.m_SP_PC_REG >> 2));

        if (Block == NULL)
        {
            if (Profiling && !IndvidualBlock)
            {
                StartTimer((uint32_t)Timer_Compiling);
            }

            memset(&RspCode, 0, sizeof(RspCode));
#if defined(_MSC_VER)
            __try
            {
                BuildBranchLabels();
                DetectGPRConstants(&RspCode);
                CompilerRSPBlock();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                char ErrorMessage[400];
                sprintf(ErrorMessage, "Error CompilePC = %08X", CompilePC);
                g_Notify->DisplayError(ErrorMessage);
                ClearAllx86Code();
                continue;
            }
#else
            BuildBranchLabels();
            DetectGPRConstants(&RspCode);
            CompilerRSPBlock();
#endif

            Block = (uint8_t *)*(JumpTable + (*m_System.m_SP_PC_REG >> 2));

            // We are done compiling, but we may have references
            // to fill in still either from this block, or jumps
            // that go out of it, let's rock

            LinkBranches(&m_CurrentBlock);
            if (Profiling && !IndvidualBlock)
            {
                StopTimer();
            }
        }

        if (Profiling && IndvidualBlock)
        {
            StartTimer(*m_System.m_SP_PC_REG);
        }

#if defined(_M_IX86) && defined(_MSC_VER)
        _asm {
			pushad
			call Block
			popad
        }
#else
        g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
        if (Profiling && IndvidualBlock)
        {
            StopTimer();
        }
        if (RSP_NextInstruction == RSPPIPELINE_SINGLE_STEP)
        {
            RSP_Running = false;
        }
    }

    if (IsMmxEnabled)
    {
#if defined(_M_IX86) && defined(_MSC_VER)
        _asm emms
#else
        g_Notify->BreakPoint(__FILE__, __LINE__);
#endif
    }
}

void CRSPRecompiler::Branch_AddRef(uint32_t Target, uint32_t * X86Loc)
{
    if (m_CurrentBlock.ResolveCount >= 150)
    {
        CompilerWarning("Out of branch reference space");
    }
    else
    {
        uint8_t * KnownCode = (uint8_t *)(*(JumpTable + (Target >> 2)));

        if (KnownCode == NULL)
        {
            uint32_t i = m_CurrentBlock.ResolveCount;
            m_CurrentBlock.BranchesToResolve[i].TargetPC = Target;
            m_CurrentBlock.BranchesToResolve[i].X86JumpLoc = X86Loc;
            m_CurrentBlock.ResolveCount += 1;
        }
        else
        {
            CPU_Message("      (static jump to %X)", KnownCode);
            x86_SetBranch32b((uint32_t *)X86Loc, (uint32_t *)KnownCode);
        }
    }
}

void CRSPRecompiler::SetJumpTable(uint32_t End)
{
    extern uint32_t NoOfMaps, MapsCRC[32];
    extern uint8_t * JumpTables;

    uint32_t CRC = crc32(0L, Z_NULL, 0);
    if (End < 0x800)
    {
        End = 0x800;
    }

    if (End == 0x1000 && ((m_RSPRegisterHandler->PendingSPMemAddr() & 0x0FFF) & ~7) == 0x80)
    {
        End = 0x800;
    }

    CRC = crc32(CRC, RSPInfo.IMEM, End);
    for (uint32_t i = 0; i < NoOfMaps; i++)
    {
        if (CRC == MapsCRC[i])
        {
            JumpTable = (void **)(JumpTables + i * 0x1000);
            Table = i;
            return;
        }
    }
    if (NoOfMaps == MaxMaps)
    {
        ResetJumpTables();
    }
    MapsCRC[NoOfMaps] = CRC;
    JumpTable = (void **)(JumpTables + NoOfMaps * 0x1000);
    Table = NoOfMaps;
    NoOfMaps += 1;
}