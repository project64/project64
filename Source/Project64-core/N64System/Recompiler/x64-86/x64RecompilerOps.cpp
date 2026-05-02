#include "stdafx.h"
#if defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/CodeBlock.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/x64-86/x64RecompilerOps.h>
#include <Project64-core/N64System/SystemGlobals.h>

CX64RecompilerOps::CX64RecompilerOps(CN64System & System, CCodeBlock & CodeBlock) :
    CRecompilerOpsBase(System, CodeBlock),
    m_Recompiler(System.m_Recomp),
    m_Rom(*g_Rom),
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler),
    m_MMU(System.m_MMU_VM),
    m_PipelineStage(PIPELINE_STAGE_NORMAL),
    m_CompilePC(m_Instruction.Address32())
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
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_GameSettings.fastSP && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, m_RegWorkingSet.GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) | m_Opcode.immediate);

        if (g_GameSettings.fastSP && m_Opcode.rt == 29 && m_Opcode.rs != 29)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::XORI()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::LUI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_GameSettings.fastSP && m_Opcode.rt == 29)
    {
        uint32_t Address = 0;

        m_MMU.VAddrToPAddr(((int16_t)m_Opcode.offset << 16), Address);
        const uint64_t stackPtrVal = (uint64_t)(Address + m_MMU.Rdram());
        void * const var = &m_Recompiler->MemoryStackPos();
        m_Assembler.mov(asmjit::x86::dword_ptr(reinterpret_cast<uintptr_t>(var)), static_cast<uint32_t>(stackPtrVal));
        m_Assembler.mov(asmjit::x86::dword_ptr(reinterpret_cast<uintptr_t>(var) + 4u), static_cast<uint32_t>(stackPtrVal >> 32));
    }

    m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, static_cast<uint32_t>((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegBase::STATE_CONST_32_SIGN);
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

void CX64RecompilerOps::RESERVED31()
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

void CX64RecompilerOps::SPECIAL_SYNC()
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
}

void CX64RecompilerOps::CompileExitCode()
{
}

void CX64RecompilerOps::CompileInPermLoop(CRegInfo & /*RegSet*/, uint32_t /*ProgramCounter*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SyncRegState(const CRegInfo & /*SyncTo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

CX64RegInfo & CX64RecompilerOps::GetRegWorkingSet(void)
{
    return m_RegWorkingSet;
}

void CX64RecompilerOps::SetRegWorkingSet(const CX64RegInfo & RegInfo)
{
    m_RegWorkingSet = RegInfo;
}

bool CX64RecompilerOps::InheritParentInfo()
{
    m_Section->DisplaySectionInformation();

    if (m_Section->m_ParentSection.empty())
    {
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    if (m_Section->m_ParentSection.size() == 1)
    {
        CCodeSection * Parent = *(m_Section->m_ParentSection.begin());
        if (!Parent->m_EnterLabel.isValid())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        CJumpInfo * JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;

        m_Section->m_RegEnter = JumpInfo->RegSet;
        LinkJump(*JumpInfo);
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    g_Notify->BreakPoint(__FILE__, __LINE__);
    return false;
}

void CX64RecompilerOps::LinkJump(CJumpInfo & JumpInfo, uint32_t /*SectionID*/, uint32_t /*FromSectionID*/)
{
    if (JumpInfo.LinkLocation.isValid())
    {
        m_CodeBlock.Log("");
        m_Assembler.bind(JumpInfo.LinkLocation);
        JumpInfo.LinkLocation = asmjit::Label();
        if (JumpInfo.LinkLocation2.isValid())
        {
            m_Assembler.bind(JumpInfo.LinkLocation2);
            JumpInfo.LinkLocation2 = asmjit::Label();
        }
    }
}

void CX64RecompilerOps::JumpToSection(CCodeSection * /*Section*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::JumpToUnknown(CJumpInfo * /*JumpInfo*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CX64RecompilerOps::SetCurrentPC(uint32_t ProgramCounter)
{
    uint32_t Value;
    if (!m_MMU.MemoryValue32(ProgramCounter, Value))
    {
        g_Notify->FatalError(GS(MSG_FAIL_LOAD_WORD));
    }
    m_Instruction = R4300iInstruction((int32_t)ProgramCounter, Value);
}

uint32_t CX64RecompilerOps::GetCurrentPC(void)
{
    return m_CompilePC;
}

void CX64RecompilerOps::SetCurrentSection(CCodeSection * section)
{
    m_Section = section;
}

void CX64RecompilerOps::SetNextStepType(PIPELINE_STAGE StepType)
{
    m_PipelineStage = StepType;
}

PIPELINE_STAGE CX64RecompilerOps::GetNextStepType(void)
{
    return m_PipelineStage;
}

const R4300iOpcode & CX64RecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
}

void CX64RecompilerOps::PreCompileOpcode(void)
{
    if (m_PipelineStage != PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        m_CodeBlock.Log("  %X %s", (uint32_t)m_CompilePC, m_Instruction.NameAndParam().c_str());
    }
    m_RegWorkingSet.ResetRegisterProtection();
}

void CX64RecompilerOps::PostCompileOpcode(void)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_GameSettings.countPerOp);
    if (!g_GameSettings.regCaching)
    {
        m_RegWorkingSet.WriteBackRegisters();
    }
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