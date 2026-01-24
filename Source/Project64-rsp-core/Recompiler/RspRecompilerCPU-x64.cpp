#if defined(__amd64__) || defined(_M_X64)

#include "RspRecompilerCPU-x64.h"
#include "RspCodeBlock.h"
#include <Common/Log.h>
#include <Common/StdString.h>
#include <Project64-rsp-core/Recompiler/RspAssembler.h>
#include <Project64-rsp-core/Recompiler/RspCodeBlock.h>
#include <Project64-rsp-core/Recompiler/RspProfiling.h>
#include <Project64-rsp-core/Settings/RspSettings.h>
#include <Project64-rsp-core/cpu/RspMemory.h>
#include <Project64-rsp-core/cpu/RspSystem.h>
#include <Settings/Settings.h>

extern CLog * CPULog;

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
    m_RecompilerOps(System, *this),
    m_NextInstruction(RSPPIPELINE_NORMAL),
    m_BlockID(0),
    m_CompilePC(0),
    m_OpCode(System.m_OpCode),
    m_Assembler(nullptr),
    m_RegState(m_RecompilerOps)
{
    m_Environment = asmjit::Environment::host();
    BuildRecompilerCPU();
}

CRSPRecompiler::~CRSPRecompiler()
{
    if (m_Assembler != nullptr)
    {
        delete m_Assembler;
        m_Assembler = nullptr;
    }
}

void CRSPRecompiler::ClearBranchJump()
{
    m_BranchTargets.clear();
}

void CRSPRecompiler::AddBranchJump(uint32_t Target)
{
    BranchTargets::iterator it = m_BranchTargets.find(Target);
    if (it == m_BranchTargets.end())
    {
        asmjit::Label Label = m_Assembler->newLabel();
        m_BranchTargets[Target] = Label;
        m_Assembler->AddLabelSymbol(Label, stdstr_f("0x%X", Target).c_str());
    }
}

bool CRSPRecompiler::FindBranchJump(uint32_t Target, asmjit::Label & Jump)
{
    BranchTargets::iterator it = m_BranchTargets.find(Target);
    if (it != m_BranchTargets.end())
    {
        Jump = m_BranchTargets[Target];
        return true;
    }
    return false;
}
void CRSPRecompiler::BuildRecompilerCPU(void)
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
}

void CRSPRecompiler::Reset()
{
    if (m_Assembler != nullptr)
    {
        m_Assembler->Reset();
    }
}

bool CRSPRecompiler::CompileSubFunctions(RspCodeBlocks & Functions, const RspCodeBlock::Addresses & Addresses)
{
    for (RspCodeBlock::Addresses::iterator itr = Addresses.begin(); itr != Addresses.end(); itr++)
    {
        RspCodeBlocks::iterator funcitr = Functions.find(*itr);
        if (funcitr == Functions.end())
        {
            return false;
        }
        RspCodeBlockPtr & FuncCodeBlock = funcitr->second;
        if (FuncCodeBlock->GetCompiledLocation() == nullptr)
        {
            if (!CompileSubFunctions(Functions, FuncCodeBlock->GetFunctionCalls()))
            {
                return nullptr;
            }
            CompileCodeBlock(*FuncCodeBlock);
        }
    }
    return true;
}

void CRSPRecompiler::CompileCodeBlock(RspCodeBlock & block)
{
    SetupRspAssembler();
    m_CurrentBlock = &block;

    void * funcPtr = RecompPos;
    m_CompilePC = block.GetStartAddress();
    Log("====== Block %d ======", m_BlockID++);
    if (block.CodeType() == RspCodeType_TASK)
    {
        Log("code type: task");
    }
    else if (block.CodeType() == RspCodeType_SUBROUTINE)
    {
        Log("code type: subroutine");
    }
    Log("asm code at: %016llX", (uint64_t)funcPtr);
    Log("Jump table: %X", Table);
    Log("Start of block: %X", m_CompilePC);
    Log("====== Recompiled code ======");
    m_RecompilerOps.EnterCodeBlock();

    const RspCodeBlock::Addresses & branchTargets = block.GetBranchTargets();
    ClearBranchJump();
    for (uint32_t Target : branchTargets)
    {
        AddBranchJump(Target);
    }

    /*if (Compiler.bReOrdering)
    {
        memcpy(IMEM_SAVE, RSPInfo.IMEM, 0x1000);
        ReOrderSubBlock(&m_CurrentBlock);
    }*/
    const RSPInstructions & instructions = block.GetInstructions();
    for (size_t instructionIndex = 0, instructionCount = instructions.size(); instructionIndex < instructionCount;)
    {
        const RSPInstruction & instruction = instructions[instructionIndex];
        m_CompilePC = instruction.Address();
        m_OpCode.Value = instruction.Value();

        BranchTargets::const_iterator labelItr = m_BranchTargets.find(m_CompilePC);
        bool JumpTarget = false;
        if (labelItr != m_BranchTargets.end())
        {
            m_RegState.WriteBackRegisters();
            if (m_NextInstruction == RSPPIPELINE_NORMAL)
            {
                m_Assembler->bind(labelItr->second);
            }
            else if (m_NextInstruction == RSPPIPELINE_DELAY_SLOT)
            {
                JumpTarget = true;
            }
        }
        (m_RecompilerOps.*RSP_Recomp_Opcode[m_OpCode.op])();
        m_RegState.ResetRegProtection();

        switch (m_NextInstruction)
        {
        case RSPPIPELINE_NORMAL:
            instructionIndex += 1;
            break;
        case RSPPIPELINE_DO_DELAY_SLOT:
            m_NextInstruction = RSPPIPELINE_DELAY_SLOT;
            instructionIndex += 1;
            break;
        case RSPPIPELINE_DO_DELAY_SLOT_TASK_EXIT:
            m_NextInstruction = RSPPIPELINE_DELAY_SLOT_TASK_EXIT;
            instructionIndex += 1;
            break;
        case RSPPIPELINE_DELAY_SLOT:
            m_NextInstruction = JumpTarget ? RSPPIPELINE_DELAY_SLOT_DONE_BRANCH_TARGET : RSPPIPELINE_DELAY_SLOT_DONE;
            instructionIndex -= 1;
            break;
        case RSPPIPELINE_DELAY_SLOT_DONE:
        case RSPPIPELINE_DELAY_SLOT_DONE_BRANCH_TARGET:
            m_NextInstruction = RSPPIPELINE_NORMAL;
            instructionIndex += 2;
            break;
        case RSPPIPELINE_DELAY_SLOT_EXIT:
            m_NextInstruction = RSPPIPELINE_DELAY_SLOT_EXIT_DONE;
            instructionIndex -= 1;
            break;
        case RSPPIPELINE_DELAY_SLOT_TASK_EXIT:
            m_NextInstruction = RSPPIPELINE_DELAY_SLOT_TASK_EXIT_DONE;
            instructionIndex -= 1;
            break;
        case RSPPIPELINE_FINISH_TASK_SUB_BLOCK:
            m_NextInstruction = RSPPIPELINE_FINISH_BLOCK;
            break;
        case RSPPIPELINE_FINISH_BLOCK: break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            instructionIndex -= 1;
            break;
        }
    }

    block.SetCompiledLocation(funcPtr);
    m_Assembler->finalize();
    m_CodeHolder.flatten();
    m_CodeHolder.resolveUnresolvedLinks();
    m_CodeHolder.relocateToBase((uint64_t)funcPtr);
    size_t codeSize = m_CodeHolder.codeSize();
    m_CodeHolder.copyFlattenedData(funcPtr, codeSize);
    RecompPos += codeSize;

    if (LogAsmCode && !m_CodeLog.empty() && CPULog != nullptr)
    {
        CPULog->Log(m_CodeLog.c_str());
        CPULog->Log("\r\n");
        CPULog->Flush();
        m_CodeLog.clear();
    }
    m_CurrentBlock = nullptr;
}

void CRSPRecompiler::CompileOpcode(uint32_t PC)
{
    const RSPInstructions & instructions = m_CurrentBlock->GetInstructions();
    bool found = false;
    for (size_t i = 0, n = instructions.size(); i < n; i++)
    {
        if (instructions[i].Address() != PC)
        {
            continue;
        }
        m_CompilePC = instructions[i].Address();
        m_OpCode.Value = instructions[i].Value();
        found = true;
        break;
    }

    if (!found)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    BranchTargets::const_iterator labelItr = m_BranchTargets.find(m_CompilePC);
    if (labelItr != m_BranchTargets.end())
    {
        m_Assembler->bind(labelItr->second);
    }
    (m_RecompilerOps.*RSP_Recomp_Opcode[m_OpCode.op])();
}

void * CRSPRecompiler::CompileHLETask(uint32_t Address, RspCodeBlocks & Functions, const uint32_t DispatchAddress)
{
    void * funcPtr = nullptr;
    bool compile = true;
    if (compile)
    {
        // have code block in CRSPRecompiler and pass to RspCodeBlock, so it is the owner and sub functions are analysised once
        RspCodeBlock CodeInfo(m_System, Address, RspCodeType_TASK, DispatchAddress, Functions);
        if (!CompileSubFunctions(Functions, CodeInfo.GetFunctionCalls()))
        {
            return nullptr;
        }
        CompileCodeBlock(CodeInfo);
        funcPtr = CodeInfo.GetCompiledLocation();
    }
    else
    {
        funcPtr = RecompPos;
        if (LogAsmCode)
        {
            Log("====== Block %d ======", m_BlockID++);
            Log("asm code at: %016llX", (uint64_t)funcPtr);
            Log("Jump table: %X", Table);
            Log("Start of block: %X", Address);
            Log("====== Recompiled code ======");
        }

        SetupRspAssembler();
        m_Assembler->push(asmjit::x86::rbp);
        m_Assembler->mov(asmjit::x86::rbp, asmjit::x86::rsp);
        m_Assembler->sub(asmjit::x86::rsp, 0x30);
        if (Profiling)
        {
            m_Assembler->mov(asmjit::x86::rcx, asmjit::imm((uintptr_t)Address));
            m_Assembler->CallFunc(AddressOf(&StartTimer), "StartTimer");
        }
        m_Assembler->mov(asmjit::x86::rcx, asmjit::imm((uintptr_t)&m_System));
        m_Assembler->mov(asmjit::x86::rdx, asmjit::imm(0x10000));
        m_Assembler->mov(asmjit::x86::r8, asmjit::imm(0x118));
        m_Assembler->CallFunc(AddressOf(&CRSPSystem::ExecuteOps), "CRSPSystem::ExecuteOps");
        if (Profiling)
        {
            m_Assembler->CallFunc(AddressOf(&StopTimer), "StopTimer");
        }
        m_Assembler->add(asmjit::x86::rsp, 0x30);
        m_Assembler->pop(asmjit::x86::rbp);
        m_Assembler->ret();
        m_Assembler->finalize();

        m_CodeHolder.relocateToBase((uint64_t)funcPtr);
        size_t codeSize = m_CodeHolder.codeSize();
        m_CodeHolder.copyFlattenedData(funcPtr, codeSize);
        RecompPos += codeSize;
    }

    if (LogAsmCode && !m_CodeLog.empty() && CPULog != nullptr)
    {
        CPULog->Log(m_CodeLog.c_str());
        CPULog->Log("\r\n");
        CPULog->Flush();
        m_CodeLog.clear();
    }
    return funcPtr;
}

void CRSPRecompiler::Log(_Printf_format_string_ const char * Text, ...)
{
    if (!LogAsmCode)
    {
        return;
    }

    va_list args;
    va_start(args, Text);
#pragma warning(push)
#pragma warning(disable : 4996)
    size_t nlen = _vscprintf(Text, args) + 1;
    char * buffer = (char *)alloca(nlen * sizeof(char));
    buffer[nlen - 1] = 0;
    if (buffer != nullptr)
    {
        vsprintf(buffer, Text, args);
        m_CodeLog += buffer;
        m_CodeLog += "\n";
    }
#pragma warning(pop)
    va_end(args);
}

void CRSPRecompiler::handleError(asmjit::Error /*err*/, const char * /*message*/, asmjit::BaseEmitter * /*origin*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRSPRecompiler::SetupRspAssembler()
{
    if (m_Assembler != nullptr)
    {
        delete m_Assembler;
        m_Assembler = nullptr;
    }

    m_CodeHolder.reset();
    m_CodeHolder.init(m_Environment);
    m_CodeHolder.setErrorHandler(this);

    m_Assembler = new RspAssembler(&m_CodeHolder, m_CodeLog);
    m_CodeHolder.setLogger(LogAsmCode ? m_Assembler : nullptr);
}

void * CRSPRecompiler::GetAddressOf(int value, ...)
{
    void * Address;

    va_list ap;
    va_start(ap, value);
    Address = va_arg(ap, void *);
    va_end(ap);

    return Address;
}
#endif
