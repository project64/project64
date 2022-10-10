#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Recompiler/x64-86/x64RecompilerOps.h>

CX64RecompilerOps::CX64RecompilerOps(CMipsMemoryVM & /*MMU*/, CCodeBlock & CodeBlock) :
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler)
{
}

CX64RecompilerOps::~CX64RecompilerOps()
{
}

void CX64RecompilerOps::Compile_TrapCompare(RecompilerTrapCompare /*CompareType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::Compile_BranchCompare(RecompilerBranchCompare /*CompareType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::Compile_Branch(RecompilerBranchCompare /*CompareType*/, bool /*Link*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::Compile_BranchLikely(RecompilerBranchCompare /*CompareType*/, bool /*Link*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BNE_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BEQ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BGTZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BLEZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BLTZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::BGEZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_BCF_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_BCT_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::J()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::JAL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::ADDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::ADDIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SLTI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SLTIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::ANDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::ORI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::XORI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LUI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::DADDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::DADDIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LDL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LDR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LH()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LW()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LHU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SH()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SWL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SW()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SDL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SDR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CACHE()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LWC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LDC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SC()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SWC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SDC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SRL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SRA()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SRLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SRAV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_JR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_JALR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SYSCALL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_BREAK()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MFLO()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MTLO()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MFHI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MTHI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSLLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRAV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_MULTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DIVU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DMULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DMULTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DDIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DDIVU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_ADDU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SUBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_AND()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_OR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_XOR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_NOR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_SLTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DADDU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSUBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSLL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRA()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSLL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SPECIAL_DSRA32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_MF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_DMF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_MT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBWI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_TLBP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP0_CO_ERET()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_MF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_DMF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_CF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_MT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_CT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_MUL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ABS()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_NEG()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_SQRT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_MOV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ROUND_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_TRUNC_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CEIL_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_FLOOR_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_ROUND_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_TRUNC_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CEIL_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_FLOOR_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CVT_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CVT_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_S_CMP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_MUL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ABS()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_NEG()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_SQRT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_MOV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ROUND_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_TRUNC_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CEIL_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_FLOOR_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_ROUND_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_TRUNC_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CEIL_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_FLOOR_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CVT_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CVT_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_D_CMP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_W_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_W_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_L_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::COP1_L_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::UnknownOpcode()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::EnterCodeBlock()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileExitCode()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileInPermLoop(CRegInfo & /*RegSet*/, uint32_t /*ProgramCounter*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SyncRegState(const CRegInfo & /*SyncTo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CRegInfo & CX64RecompilerOps::GetRegWorkingSet(void)
{
    return m_RegWorkingSet;
}

void CX64RecompilerOps::SetRegWorkingSet(const CRegInfo & /*RegInfo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

bool CX64RecompilerOps::InheritParentInfo()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CX64RecompilerOps::LinkJump(CJumpInfo & /*JumpInfo*/, uint32_t /*SectionID*/, uint32_t /*FromSectionID*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::JumpToSection(CCodeSection * /*Section*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::JumpToUnknown(CJumpInfo * /*JumpInfo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SetCurrentPC(uint32_t /*ProgramCounter*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

uint32_t CX64RecompilerOps::GetCurrentPC(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return 0;
}

void CX64RecompilerOps::SetCurrentSection(CCodeSection * /*section*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SetNextStepType(PIPELINE_STAGE /*StepType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

PIPELINE_STAGE CX64RecompilerOps::GetNextStepType(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return PIPELINE_STAGE_NORMAL;
}

const R4300iOpcode & CX64RecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
}

void CX64RecompilerOps::PreCompileOpcode(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::PostCompileOpcode(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileExit(uint32_t /*JumpPC*/, uint32_t /*TargetPC*/, CRegInfo & /*ExitRegSet*/, ExitReason /*reason*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::UpdateCounters(CRegInfo & /*RegSet*/, bool /*CheckTimer*/, bool /*ClearValues*/, bool /*UpdateTimer*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileSystemCheck(uint32_t /*TargetPC*/, const CRegInfo & /*RegSet*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileExecuteBP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::CompileExecuteDelaySlotBP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

#endif