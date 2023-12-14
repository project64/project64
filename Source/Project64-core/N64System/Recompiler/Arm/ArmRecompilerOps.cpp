#include "stdafx.h"

#if defined(__arm__) || defined(_M_ARM)

#include <Project64-core/N64System/Recompiler/Arm/ArmRecompilerOps.h>
#include <Project64-core/Notification.h>

CArmRecompilerOps::CArmRecompilerOps(CN64System & System, CCodeBlock & CodeBlock) :
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler)
{
}

CArmRecompilerOps::~CArmRecompilerOps()
{
}

void CArmRecompilerOps::Compile_TrapCompare(RecompilerTrapCompare /*CompareType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::Compile_BranchCompare(RecompilerBranchCompare /*CompareType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::Compile_Branch(RecompilerBranchCompare /*CompareType*/, bool /*Link*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::Compile_BranchLikely(RecompilerBranchCompare /*CompareType*/, bool /*Link*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::BNE_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::BEQ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::BGTZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::BLEZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::BLTZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::BGEZ_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_BCF_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_BCT_Compare()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::J()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::JAL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::ADDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::ADDIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SLTI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SLTIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::ANDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::ORI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::XORI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LUI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::DADDI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::DADDIU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LDL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LDR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::RESERVED31()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LH()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LWL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LW()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LHU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LWU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SH()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SWL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SW()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SDL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SDR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CACHE()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LWC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LDC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::LD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SC()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SWC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SDC1()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SLL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SRL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SRA()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SLLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SRLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SRAV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_JR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_JALR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SYSCALL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_BREAK()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SYNC()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_MFLO()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_MTLO()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_MFHI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_MTHI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSLLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSRLV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSRAV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_MULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_MULTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DIVU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DMULT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DMULTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DDIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DDIVU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_ADDU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SUBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_AND()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_OR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_XOR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_NOR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SLT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_SLTU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DADDU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSUBU()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSLL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSRL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSRA()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSLL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSRL32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SPECIAL_DSRA32()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_MF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_DMF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_MT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_CO_TLBR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_CO_TLBWI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_CO_TLBWR()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_CO_TLBP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP0_CO_ERET()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_MF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_DMF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_CF()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_MT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_DMT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_CT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_MUL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_ABS()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_NEG()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_SQRT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_MOV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_ROUND_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_TRUNC_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_CEIL_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_FLOOR_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_ROUND_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_TRUNC_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_CEIL_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_FLOOR_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_CVT_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_CVT_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_S_CMP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_ADD()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_SUB()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_MUL()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_DIV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_ABS()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_NEG()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_SQRT()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_MOV()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_ROUND_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_TRUNC_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_CEIL_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_FLOOR_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_ROUND_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_TRUNC_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_CEIL_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_FLOOR_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_CVT_W()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_CVT_L()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_D_CMP()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_W_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_W_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_L_CVT_S()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::COP1_L_CVT_D()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::UnknownOpcode()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::EnterCodeBlock()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CompileExitCode()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CompileInPermLoop(CRegInfo & /*RegSet*/, uint32_t /*ProgramCounter*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SyncRegState(const CRegInfo & /*SyncTo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CRegInfo & CArmRecompilerOps::GetRegWorkingSet(void)
{
    return m_RegWorkingSet;
}

void CArmRecompilerOps::SetRegWorkingSet(const CRegInfo & /*RegInfo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

bool CArmRecompilerOps::InheritParentInfo()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CArmRecompilerOps::LinkJump(CJumpInfo & /*JumpInfo*/, uint32_t /*SectionID*/, uint32_t /*FromSectionID*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::JumpToSection(CCodeSection * /*Section*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::JumpToUnknown(CJumpInfo * /*JumpInfo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SetCurrentPC(uint32_t /*ProgramCounter*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

uint32_t CArmRecompilerOps::GetCurrentPC(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return 0;
}

void CArmRecompilerOps::SetCurrentSection(CCodeSection * /*section*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::SetNextStepType(PIPELINE_STAGE /*StepType*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

PIPELINE_STAGE CArmRecompilerOps::GetNextStepType(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
    return PIPELINE_STAGE_NORMAL;
}

const R4300iOpcode & CArmRecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
}

void CArmRecompilerOps::PreCompileOpcode(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::PostCompileOpcode(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CompileExit(uint32_t /*JumpPC*/, uint32_t /*TargetPC*/, CRegInfo & /*ExitRegSet*/, ExitReason /*reason*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::UpdateCounters(CRegInfo & /*RegSet*/, bool /*CheckTimer*/, bool /*ClearValues*/, bool /*UpdateTimer*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CompileSystemCheck(uint32_t /*TargetPC*/, const CRegInfo & /*RegSet*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CompileExecuteBP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CArmRecompilerOps::CompileExecuteDelaySlotBP(void)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

#endif