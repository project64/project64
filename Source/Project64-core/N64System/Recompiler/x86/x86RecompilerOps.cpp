#include "stdafx.h"

#if defined(__i386__) || defined(_M_IX86)

#include <Project64-core/Debugger.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/R4300iInstruction.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/LoopAnalysis.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/SectionInfo.h>
#include <Project64-core/N64System/Recompiler/x86/x86RecompilerOps.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <stdio.h>

uint32_t CX86RecompilerOps::m_TempValue32 = 0;
uint64_t CX86RecompilerOps::m_TempValue64 = 0;
uint32_t CX86RecompilerOps::m_BranchCompare = 0;

/*int TestValue = 0;
void TestFunc()
{
TestValue += 1;
if (TestValue >= 4)
{
g_Notify->BreakPoint(__FILE__, __LINE__);
}
}*/

static void x86_compiler_Break_Point()
{
    g_Settings->SaveBool(Debugger_SteppingOps, true);
    do
    {
        g_Debugger->WaitForStep();
        if (CDebugSettings::SkipOp())
        {
            // Skip command if instructed by the debugger
            g_Settings->SaveBool(Debugger_SkipOp, false);
            g_Reg->m_PROGRAM_COUNTER += 4;

            uint32_t OpcodeValue;
            if (!g_MMU->MemoryValue32(g_Reg->m_PROGRAM_COUNTER, OpcodeValue))
            {
                g_Reg->DoTLBReadMiss(false, g_Reg->m_PROGRAM_COUNTER);
                continue;
            }
            continue;
        }
        CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
        if (g_SyncSystem)
        {
            g_System->UpdateSyncCPU(g_SyncSystem, g_System->CountPerOp());
            g_System->SyncCPU(g_SyncSystem);
        }

    } while (CDebugSettings::isStepping());

    if (g_System->PipelineStage() != PIPELINE_STAGE_NORMAL)
    {
        CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
        if (g_SyncSystem)
        {
            g_System->UpdateSyncCPU(g_SyncSystem, g_System->CountPerOp());
            g_System->SyncCPU(g_SyncSystem);
        }
    }
}

static void x86_Break_Point_DelaySlot()
{
    CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
    if (g_SyncSystem)
    {
        g_System->UpdateSyncCPU(g_SyncSystem, g_System->CountPerOp());
        g_System->SyncCPU(g_SyncSystem);
    }
    if (g_Debugger->ExecutionBP(g_Reg->m_PROGRAM_COUNTER))
    {
        x86_compiler_Break_Point();
    }
    if (g_System->PipelineStage() != PIPELINE_STAGE_NORMAL)
    {
        CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
        if (g_SyncSystem)
        {
            g_System->UpdateSyncCPU(g_SyncSystem, g_System->CountPerOp());
            g_System->SyncCPU(g_SyncSystem);
        }
    }
}

static uint32_t memory_access_address;
static uint32_t memory_write_in_delayslot;
static uint32_t memory_breakpoint_found = 0;

static void x86MemoryBreakpoint()
{
    memory_breakpoint_found = 1;
    if (memory_write_in_delayslot)
    {
        g_Reg->m_PROGRAM_COUNTER -= 4;
        *g_NextTimer += g_System->CountPerOp();
        CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
    }
    x86_compiler_Break_Point();
}

static void x86TestReadBreakpoint8()
{
    if (g_Debugger->ReadBP8(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestReadBreakpoint16()
{
    if (g_Debugger->ReadBP16(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestReadBreakpoint32()
{
    if (g_Debugger->ReadBP32(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestReadBreakpoint64()
{
    if (g_Debugger->ReadBP64(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestWriteBreakpoint8()
{
    if (g_Debugger->WriteBP8(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestWriteBreakpoint16()
{
    if (g_Debugger->WriteBP16(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestWriteBreakpoint32()
{
    if (g_Debugger->WriteBP32(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

static void x86TestWriteBreakpoint64()
{
    if (g_Debugger->WriteBP64(memory_access_address))
    {
        x86MemoryBreakpoint();
    }
}

CX86RecompilerOps::CX86RecompilerOps(CMipsMemoryVM & MMU, CCodeBlock & CodeBlock) :
    m_MMU(MMU),
    m_CodeBlock(CodeBlock),
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler),
    m_CompilePC(0),
    m_Section(nullptr),
    m_RegBeforeDelay(CodeBlock, m_Assembler),
    m_EffectDelaySlot(false)
{
}

CX86RecompilerOps::~CX86RecompilerOps()
{
}

void CX86RecompilerOps::PreCompileOpcode(void)
{
    if (m_PipelineStage != PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        m_CodeBlock.Log("  %X %s", m_CompilePC, R4300iInstruction(m_CompilePC, m_Opcode.Value).NameAndParam().c_str());
    }
    /*if (m_CompilePC == 0x803275F4 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        m_Assembler.X86BreakPoint(__FILE__, __LINE__);
    }

    /*if (m_CompilePC >= 0x80000000 && m_CompilePC <= 0x80400000 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (g_SyncSystem) {
    #ifdef _WIN32
    m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.PushImm32((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_ESP, 4);
    #endif
    }
    }*/

    /*if ((m_CompilePC == 0x8031C0E4 || m_CompilePC == 0x8031C118 ||
    m_CompilePC == 0x8031CD88 ||  m_CompilePC == 0x8031CE24 ||
    m_CompilePC == 0x8031CE30 || m_CompilePC == 0x8031CE40) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet, false, true);
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
        if (g_SyncSystem)
        {
#ifdef _WIN32
            m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)g_BaseSystem);
            m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
            m_Assembler.PushImm32((uint32_t)g_BaseSystem);
            m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
            m_Assembler.AddConstToX86Reg(CX86Ops::x86_ESP, 4);
#endif
        }
    }*/

    /*if (m_CompilePC == 0x801C1B88)
    {
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallFunc(AddressOf(TestFunc), "TestFunc");
    m_RegWorkingSet.AfterCallDirect();
    }*/

    /*if ((m_CompilePC == 0x80263900) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_Assembler.X86BreakPoint(__FILEW__,__LINE__);
    }*/

    /*if ((m_CompilePC >= 0x80325D80 && m_CompilePC <= 0x80325DF0) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER",m_CompilePC);
    if (g_SyncSystem) {
    #ifdef _WIN32
    m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.PushImm32((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_ESP, 4);
    #endif
    }
    }*/
    /*if ((m_CompilePC == 0x80324E14) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_Assembler.X86BreakPoint(__FILEW__,__LINE__);
    }*/

    /*if (m_CompilePC == 0x80324E18 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER",m_CompilePC);
    if (g_SyncSystem) {
    #ifdef _WIN32
    m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.PushImm32((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_ESP, 4);
    #endif
    }
    }*/
    /*if (m_CompilePC >= 0x80324E00 && m_CompilePC <= 0x80324E18 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER",m_CompilePC);
    if (g_SyncSystem) {
    #ifdef _WIN32
    m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.PushImm32((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_ESP, 4);
    #endif
    }
    }*/
    /*        if (m_CompilePC == 0x803245CC && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    //m_RegWorkingSet.UnMap_AllFPRs();
    g_Notify->BreakPoint(__FILE__, __LINE__);
    //X86HardBreakPoint();
    //m_Assembler.X86BreakPoint(__FILEW__,__LINE__);
    //m_RegWorkingSet.UnMap_AllFPRs();
    }*/
    /*if (m_CompilePC >= 0x80179DC4 && m_CompilePC <= 0x80179DF0 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.UnMap_AllFPRs();
    }*/

    m_RegWorkingSet.ResetX86Protection();
}

void CX86RecompilerOps::PostCompileOpcode(void)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    if (!g_System->bRegCaching())
    {
        m_RegWorkingSet.WriteBackRegisters();
    }
    m_RegWorkingSet.UnMap_AllFPRs();

    /*if (m_CompilePC >= 0x800933B4 && m_CompilePC <= 0x80093414 && (m_PipelineStage == PIPELINE_STAGE_NORMAL || m_PipelineStage == PIPELINE_STAGE_DO_DELAY_SLOT))
    {
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC + 4);
        UpdateSyncCPU(m_RegWorkingSet, m_RegWorkingSet.GetBlockCycleCount());
        m_Assembler.SubConstFromVariable(m_RegWorkingSet.GetBlockCycleCount(), g_NextTimer, "g_NextTimer"); // Updates compare flag
        m_RegWorkingSet.SetBlockCycleCount(0);

        UpdateCounters(m_RegWorkingSet, false, true);
        if (g_SyncSystem)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)g_BaseSystem);
            m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystemPC), "CN64System::SyncSystemPC");
            m_RegWorkingSet.AfterCallDirect();
        }
    }*/
}

void CX86RecompilerOps::CompileReadTLBMiss(uint32_t VirtualAddress, CX86Ops::x86Reg LookUpReg)
{
    m_Assembler.MoveConstToVariable(g_TLBLoadAddress, "TLBLoadAddress", VirtualAddress);
    m_Assembler.CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_TLBReadMiss, false, &CX86Ops::JeLabel32);
}

void CX86RecompilerOps::CompileReadTLBMiss(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg LookUpReg)
{
    m_Assembler.MoveX86regToVariable(AddressReg, g_TLBLoadAddress, "TLBLoadAddress");
    m_Assembler.CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_TLBReadMiss, false, &CX86Ops::JeLabel32);
}

void CX86RecompilerOps::CompileWriteTLBMiss(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg LookUpReg)
{
    m_Assembler.MoveX86regToVariable(AddressReg, &g_TLBStoreAddress, "g_TLBStoreAddress");
    m_Assembler.CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_TLBWriteMiss, false, &CX86Ops::JeLabel32);
}

// Trap functions
void CX86RecompilerOps::Compile_TrapCompare(RecompilerTrapCompare CompareType)
{
    uint32_t FunctAddress = 0;
    const char * FunctName = nullptr;
    switch (CompareType)
    {
    case RecompilerTrapCompare_TEQ:
        FunctAddress = (uint32_t)R4300iOp::SPECIAL_TEQ;
        FunctName = "R4300iOp::SPECIAL_TEQ";
        break;
    case RecompilerTrapCompare_TNE:
        FunctAddress = (uint32_t)R4300iOp::SPECIAL_TNE;
        FunctName = "R4300iOp::SPECIAL_TNE";
        break;
    case RecompilerTrapCompare_TGE:
        FunctAddress = (uint32_t)R4300iOp::SPECIAL_TGE;
        FunctName = "R4300iOp::SPECIAL_TGE";
        break;
    case RecompilerTrapCompare_TGEU:
        FunctAddress = (uint32_t)R4300iOp::SPECIAL_TGEU;
        FunctName = "R4300iOp::SPECIAL_TGEU";
        break;
    case RecompilerTrapCompare_TLT:
        FunctAddress = (uint32_t)R4300iOp::SPECIAL_TLT;
        FunctName = "R4300iOp::SPECIAL_TLT";
        break;
    case RecompilerTrapCompare_TLTU:
        FunctAddress = (uint32_t)R4300iOp::SPECIAL_TLTU;
        FunctName = "R4300iOp::SPECIAL_TLTU";
        break;
    case RecompilerTrapCompare_TEQI:
        FunctAddress = (uint32_t)R4300iOp::REGIMM_TEQI;
        FunctName = "R4300iOp::REGIMM_TEQI";
        break;
    case RecompilerTrapCompare_TNEI:
        FunctAddress = (uint32_t)R4300iOp::REGIMM_TNEI;
        FunctName = "R4300iOp::REGIMM_TNEI";
        break;
    case RecompilerTrapCompare_TGEI:
        FunctAddress = (uint32_t)R4300iOp::REGIMM_TGEI;
        FunctName = "R4300iOp::REGIMM_TGEI";
        break;
    case RecompilerTrapCompare_TGEIU:
        FunctAddress = (uint32_t)R4300iOp::REGIMM_TGEIU;
        FunctName = "R4300iOp::REGIMM_TGEIU";
        break;
    case RecompilerTrapCompare_TLTI:
        FunctAddress = (uint32_t)R4300iOp::REGIMM_TLTI;
        FunctName = "R4300iOp::REGIMM_TLTI";
        break;
    case RecompilerTrapCompare_TLTIU:
        FunctAddress = (uint32_t)R4300iOp::REGIMM_TLTIU;
        FunctName = "R4300iOp::REGIMM_TLTIU";
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (FunctName != nullptr && FunctAddress != 0)
    {
        if (IsMapped(m_Opcode.rs))
        {
            UnMap_GPR(m_Opcode.rs, true);
        }
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, true);
        }
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
        m_Assembler.CallFunc(FunctAddress, FunctName);
        m_RegWorkingSet.AfterCallDirect();
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

// Branch functions
void CX86RecompilerOps::Compile_BranchCompare(RecompilerBranchCompare CompareType)
{
    switch (CompareType)
    {
    case RecompilerBranchCompare_BEQ: BEQ_Compare(); break;
    case RecompilerBranchCompare_BNE: BNE_Compare(); break;
    case RecompilerBranchCompare_BLTZ: BLTZ_Compare(); break;
    case RecompilerBranchCompare_BLEZ: BLEZ_Compare(); break;
    case RecompilerBranchCompare_BGTZ: BGTZ_Compare(); break;
    case RecompilerBranchCompare_BGEZ: BGEZ_Compare(); break;
    case RecompilerBranchCompare_COP1BCF: COP1_BCF_Compare(); break;
    case RecompilerBranchCompare_COP1BCT: COP1_BCT_Compare(); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86RecompilerOps::Compile_Branch(RecompilerBranchCompare CompareType, bool Link)
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if (CompareType == RecompilerBranchCompare_COP1BCF || CompareType == RecompilerBranchCompare_COP1BCT)
        {
            CompileCop1Test();
        }
        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8 && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
            return;
        }

        if ((m_CompilePC & 0xFFC) != 0xFFC)
        {
            R4300iOpcode DelaySlot;
            m_EffectDelaySlot = g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value) && R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value);
        }
        else
        {
            m_EffectDelaySlot = true;
        }
        m_Section->m_Jump.JumpPC = m_CompilePC;
        m_Section->m_Jump.TargetPC = m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", m_Section->m_JumpSection->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Exit_%X_jump_%X", m_Section->m_EnterPC, m_Section->m_Jump.TargetPC);
        }
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_Section->m_Jump.DoneDelaySlot = false;
        m_Section->m_Cont.JumpPC = m_CompilePC;
        m_Section->m_Cont.TargetPC = m_CompilePC + 8;
        if (m_Section->m_ContinueSection != nullptr)
        {
            m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", m_Section->m_ContinueSection->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel = stdstr_f("Exit_%X_continue_%X", m_Section->m_EnterPC, m_Section->m_Cont.TargetPC);
        }
        m_Section->m_Cont.LinkLocation = nullptr;
        m_Section->m_Cont.LinkLocation2 = nullptr;
        m_Section->m_Cont.DoneDelaySlot = false;
        if (m_Section->m_Jump.TargetPC < m_Section->m_Cont.TargetPC)
        {
            m_Section->m_Cont.FallThrough = false;
            m_Section->m_Jump.FallThrough = true;
        }
        else
        {
            m_Section->m_Cont.FallThrough = true;
            m_Section->m_Jump.FallThrough = false;
        }

        if (Link)
        {
            UnMap_GPR(31, false);
            m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
            m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
        }
        if (m_EffectDelaySlot)
        {
            if ((m_CompilePC & 0xFFC) != 0xFFC)
            {
                m_Section->m_Cont.BranchLabel = m_Section->m_ContinueSection != nullptr ? "Continue" : stdstr_f("ExitBlock_%X_Continue", m_Section->m_EnterPC);
                m_Section->m_Jump.BranchLabel = m_Section->m_JumpSection != nullptr ? "Jump" : stdstr_f("ExitBlock_%X_Jump", m_Section->m_EnterPC);
            }
            else
            {
                m_Section->m_Cont.BranchLabel = "Continue";
                m_Section->m_Jump.BranchLabel = "Jump";
            }
            if (m_Section->m_Jump.TargetPC != m_Section->m_Cont.TargetPC)
            {
                Compile_BranchCompare(CompareType);
            }
            if (!m_Section->m_Jump.FallThrough && !m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != nullptr)
                {
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    LinkJump(m_Section->m_Jump);
                    m_Section->m_Jump.FallThrough = true;
                }
                else if (m_Section->m_Cont.LinkLocation != nullptr)
                {
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    LinkJump(m_Section->m_Cont);
                    m_Section->m_Cont.FallThrough = true;
                }
            }
            if ((m_CompilePC & 0xFFC) == 0xFFC)
            {
                uint8_t * DelayLinkLocation = nullptr;
                if (m_Section->m_Jump.FallThrough)
                {
                    if (m_Section->m_Jump.LinkLocation != nullptr || m_Section->m_Jump.LinkLocation2 != nullptr)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Jump.TargetPC);
                }
                else if (m_Section->m_Cont.FallThrough)
                {
                    if (m_Section->m_Cont.LinkLocation != nullptr || m_Section->m_Cont.LinkLocation2 != nullptr)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Cont.TargetPC);
                }

                if (m_Section->m_Jump.LinkLocation != nullptr || m_Section->m_Jump.LinkLocation2 != nullptr)
                {
                    m_Assembler.JmpLabel8("DoDelaySlot", 0);
                    if (DelayLinkLocation != nullptr)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    DelayLinkLocation = (uint8_t *)(*g_RecompPos - 1);

                    m_CodeBlock.Log("      ");
                    m_CodeBlock.Log("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    LinkJump(m_Section->m_Jump);
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Jump.TargetPC);
                }
                if (m_Section->m_Cont.LinkLocation != nullptr || m_Section->m_Cont.LinkLocation2 != nullptr)
                {
                    m_Assembler.JmpLabel8("DoDelaySlot", 0);
                    if (DelayLinkLocation != nullptr)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    DelayLinkLocation = (uint8_t *)(*g_RecompPos - 1);

                    m_CodeBlock.Log("      ");
                    m_CodeBlock.Log("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    LinkJump(m_Section->m_Cont);
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Cont.TargetPC);
                }
                if (DelayLinkLocation)
                {
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      DoDelaySlot:");
                    m_Assembler.SetJump8(DelayLinkLocation, *g_RecompPos);
                }
                OverflowDelaySlot(false);
                return;
            }
            ResetX86Protection();
            m_RegBeforeDelay = m_RegWorkingSet;
        }
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8)
        {
            m_PipelineStage = PIPELINE_STAGE_NORMAL;
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            SetCurrentPC(GetCurrentPC() + 4);
            return;
        }
        if (m_EffectDelaySlot)
        {
            CJumpInfo * FallInfo = m_Section->m_Jump.FallThrough ? &m_Section->m_Jump : &m_Section->m_Cont;
            CJumpInfo * JumpInfo = m_Section->m_Jump.FallThrough ? &m_Section->m_Cont : &m_Section->m_Jump;

            if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot)
            {
                ResetX86Protection();
                FallInfo->RegSet = m_RegWorkingSet;
                if (FallInfo == &m_Section->m_Jump)
                {
                    if (m_Section->m_JumpSection != nullptr)
                    {
                        m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", m_Section->m_JumpSection->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Jump.BranchLabel = "ExitBlock";
                    }
                    if (FallInfo->TargetPC <= m_CompilePC)
                    {
                        UpdateCounters(m_Section->m_Jump.RegSet, true, true, true);
                        m_CodeBlock.Log("CompileSystemCheck 12");
                        CompileSystemCheck(FallInfo->TargetPC, m_Section->m_Jump.RegSet);
                        ResetX86Protection();
                        FallInfo->Reason = ExitReason_NormalNoSysCheck;
                        FallInfo->JumpPC = (uint32_t)-1;
                    }
                }
                else
                {
                    if (m_Section->m_ContinueSection != nullptr)
                    {
                        m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", m_Section->m_ContinueSection->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Cont.BranchLabel = stdstr_f("ExitBlock_%X_Continue", m_Section->m_EnterPC);
                    }
                }
                FallInfo->DoneDelaySlot = true;
                if (!JumpInfo->DoneDelaySlot)
                {
                    FallInfo->FallThrough = false;
                    m_Assembler.JmpLabel32(FallInfo->BranchLabel.c_str(), 0);
                    FallInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);

                    if (JumpInfo->LinkLocation != nullptr)
                    {
                        m_CodeBlock.Log("      %s:", JumpInfo->BranchLabel.c_str());
                        LinkJump(*JumpInfo);
                        JumpInfo->FallThrough = true;
                        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
                        m_RegWorkingSet = m_RegBeforeDelay;
                        return;
                    }
                }
            }
        }
        else
        {
            if (m_Section->m_Jump.TargetPC != m_Section->m_Cont.TargetPC)
            {
                Compile_BranchCompare(CompareType);
                ResetX86Protection();
                m_Section->m_Cont.RegSet = m_RegWorkingSet;
                m_Section->m_Jump.RegSet = m_RegWorkingSet;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
                m_Section->m_Cont.RegSet = m_RegWorkingSet;
                if (m_Section->m_ContinueSection == nullptr && m_Section->m_JumpSection != nullptr)
                {
                    m_Section->m_ContinueSection = m_Section->m_JumpSection;
                    m_Section->m_JumpSection = nullptr;
                }
                if (m_Section->m_ContinueSection != nullptr)
                {
                    m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", m_Section->m_ContinueSection->m_SectionID);
                }
                else
                {
                    m_Section->m_Cont.BranchLabel = "ExitBlock";
                }
            }
        }
        m_Section->GenerateSectionLinkage();
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("WTF\n\nBranch\nNextInstruction = %X", m_PipelineStage).c_str());
        }
    }
}

void CX86RecompilerOps::Compile_BranchLikely(RecompilerBranchCompare CompareType, bool Link)
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if (CompareType == RecompilerBranchCompare_COP1BCF || CompareType == RecompilerBranchCompare_COP1BCT)
        {
            CompileCop1Test();
        }
        if (!g_System->bLinkBlocks() || (m_CompilePC & 0xFFC) == 0xFFC)
        {
            m_Section->m_Jump.JumpPC = m_CompilePC;
            m_Section->m_Jump.TargetPC = m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4;
            m_Section->m_Cont.JumpPC = m_CompilePC;
            m_Section->m_Cont.TargetPC = m_CompilePC + 8;
        }
        else
        {
            if (m_Section->m_Jump.JumpPC != m_CompilePC)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_Section->m_Cont.JumpPC != m_CompilePC)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_Section->m_Cont.TargetPC != m_CompilePC + 8)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }

        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }

        if (m_Section->m_ContinueSection != nullptr)
        {
            m_Section->m_Cont.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel = "ExitBlock";
        }

        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = nullptr;
        m_Section->m_Cont.LinkLocation2 = nullptr;
        if (Link)
        {
            UnMap_GPR(31, false);
            m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
            m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
        }

        Compile_BranchCompare(CompareType);
        ResetX86Protection();

        m_Section->m_Cont.RegSet = m_RegWorkingSet;
        m_Section->m_Cont.RegSet.SetBlockCycleCount(m_Section->m_Cont.RegSet.GetBlockCycleCount() + g_System->CountPerOp());
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != nullptr)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }

            if (m_Section->m_Jump.LinkLocation != nullptr || m_Section->m_Jump.FallThrough)
            {
                LinkJump(m_Section->m_Jump);

                m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Jump.TargetPC);
                OverflowDelaySlot(false);
                m_CodeBlock.Log("      ");
                m_CodeBlock.Log("      %s:", m_Section->m_Cont.BranchLabel.c_str());
            }
            else if (!m_Section->m_Cont.FallThrough)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            LinkJump(m_Section->m_Cont);
            CompileExit(m_CompilePC, m_CompilePC + 8, m_Section->m_Cont.RegSet, ExitReason_Normal, true, nullptr);
            return;
        }
        else
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
        }

        if (g_System->bLinkBlocks())
        {
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->m_Jump.RegSet.SetBlockCycleCount(m_Section->m_Jump.RegSet.GetBlockCycleCount() + g_System->CountPerOp());
            m_Section->GenerateSectionLinkage();
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else
        {
            if (m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation != nullptr)
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                m_Section->GenerateSectionLinkage();
                m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
            }
        }
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        ResetX86Protection();
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->m_Jump.RegSet.SetBlockCycleCount(m_Section->m_Jump.RegSet.GetBlockCycleCount());
        m_Section->GenerateSectionLinkage();
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (HaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n%s\nNextInstruction = %X", __FUNCTION__, m_PipelineStage).c_str());
    }
}

void CX86RecompilerOps::BNE_Compare()
{
    uint8_t * Jump = nullptr;

    if (IsKnown(m_Opcode.rs) && IsKnown(m_Opcode.rt))
    {
        if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt))
        {
            if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt))
            {
                CX86RecompilerOps::UnknownOpcode();
            }
            else if (GetMipsRegLo(m_Opcode.rs) != GetMipsRegLo(m_Opcode.rt))
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rs);
            ProtectGPR(m_Opcode.rt);
            if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt))
            {
                m_Assembler.CompX86RegToX86Reg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false) : GetMipsRegMapHi(m_Opcode.rt));

                if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_CodeBlock.Log("      ");
                    m_CodeBlock.Log("      continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                else
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(ConstReg) || Is64Bit(MappedReg))
            {
                if (Is32Bit(ConstReg) || Is32Bit(MappedReg))
                {
                    ProtectGPR(MappedReg);
                    if (Is32Bit(MappedReg))
                    {
                        m_Assembler.CompConstToX86reg(Map_TempReg(CX86Ops::x86_Unknown, MappedReg, true, false), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        m_Assembler.CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    m_Assembler.CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_CodeBlock.Log("      ");
                    m_CodeBlock.Log("      continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                else
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rs) || IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!g_System->b32BitCore())
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegHi(KnownReg));
                }
                else if (IsSigned(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], (GetMipsRegLo_S(KnownReg) >> 31));
                }
                else
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    ProtectGPR(KnownReg);
                    m_Assembler.CompX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, KnownReg, true, false), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        if (IsConst(KnownReg))
        {
            m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegLo(KnownReg));
        }
        else
        {
            m_Assembler.CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);

            if (Jump)
            {
                m_CodeBlock.Log("      ");
                m_CodeBlock.Log("      continue:");

                m_Assembler.SetJump8(Jump, *g_RecompPos);
            }
        }
        else
        {
            m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
    else
    {
        CX86Ops::x86Reg Reg = CX86Ops::x86_Unknown;

        if (!g_System->b32BitCore())
        {
            Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false);
            m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }

        Reg = Map_TempReg(Reg, m_Opcode.rt, false, false);
        m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                m_CodeBlock.Log("      ");
                m_CodeBlock.Log("      continue:");
                m_Assembler.SetJump8(Jump, *g_RecompPos);
            }
        }
        else
        {
            m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
}

void CX86RecompilerOps::BEQ_Compare()
{
    uint8_t * Jump = nullptr;

    if (IsKnown(m_Opcode.rs) && IsKnown(m_Opcode.rt))
    {
        if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt))
        {
            if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt))
            {
                CX86RecompilerOps::UnknownOpcode();
            }
            else if (GetMipsRegLo(m_Opcode.rs) == GetMipsRegLo(m_Opcode.rt))
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt))
        {
            if ((Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)) && !g_System->b32BitCore())
            {
                ProtectGPR(m_Opcode.rs);
                ProtectGPR(m_Opcode.rt);

                m_Assembler.CompX86RegToX86Reg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false) : GetMipsRegMapHi(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_CodeBlock.Log("      ");
                    m_CodeBlock.Log("      continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(ConstReg) || Is64Bit(MappedReg))
            {
                if (Is32Bit(ConstReg) || Is32Bit(MappedReg))
                {
                    if (Is32Bit(MappedReg))
                    {
                        ProtectGPR(MappedReg);
                        m_Assembler.CompConstToX86reg(Map_TempReg(CX86Ops::x86_Unknown, MappedReg, true, false), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        m_Assembler.CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    m_Assembler.CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_CodeBlock.Log("      ");
                    m_CodeBlock.Log("      continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rs) || IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!g_System->b32BitCore())
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegHi(KnownReg));
                }
                else if (IsSigned(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegLo_S(KnownReg) >> 31);
                }
                else
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            else
            {
                ProtectGPR(KnownReg);
                if (Is64Bit(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, KnownReg, true, false), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        if (IsConst(KnownReg))
        {
            m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegLo(KnownReg));
        }
        else
        {
            m_Assembler.CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                m_CodeBlock.Log("      ");
                m_CodeBlock.Log("      continue:");
                m_Assembler.SetJump8(Jump, *g_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else
    {
        CX86Ops::x86Reg Reg = CX86Ops::x86_Unknown;
        if (!g_System->b32BitCore())
        {
            Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
            m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
            if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        m_Assembler.CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                m_CodeBlock.Log("      ");
                m_CodeBlock.Log("      continue:");
                m_Assembler.SetJump8(Jump, *g_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CX86RecompilerOps::BGTZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            if (GetMipsReg_S(m_Opcode.rs) > 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            if (GetMipsRegLo_S(m_Opcode.rs) > 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
    }
    else if (IsMapped(m_Opcode.rs) && Is32Bit(m_Opcode.rs))
    {
        m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
        if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            m_Assembler.JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else if (IsUnknown(m_Opcode.rs) && g_System->b32BitCore())
    {
        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            m_Assembler.JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else
    {
        uint8_t * Jump = nullptr;

        if (IsMapped(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JgLabel8("Continue", 0);
            Jump = *g_RecompPos - 1;
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JlLabel8("Continue", 0);
            Jump = *g_RecompPos - 1;
            m_Assembler.JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }

        if (IsMapped(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            m_CodeBlock.Log("      continue:");
            m_Assembler.SetJump8(Jump, *g_RecompPos);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_CodeBlock.Log("      continue:");
            m_Assembler.SetJump8(Jump, *g_RecompPos);
        }
        else
        {
            m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CX86RecompilerOps::BLEZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            if (GetMipsReg_S(m_Opcode.rs) <= 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            if (GetMipsRegLo_S(m_Opcode.rs) <= 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            if (GetMipsRegLo(m_Opcode.rs) == 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is32Bit(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JleLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            uint8_t * Jump = nullptr;

            if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            }
            else
            {
                m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JlLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JgLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
                m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }

            if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            }
            else
            {
                m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                m_CodeBlock.Log("      continue:");
                m_Assembler.SetJump8(Jump, *g_RecompPos);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                m_CodeBlock.Log("      continue:");
                m_Assembler.SetJump8(Jump, *g_RecompPos);
            }
            else
            {
                m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32("BranchToJump", 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
    else
    {
        uint8_t * Jump = nullptr;

        if (!g_System->b32BitCore())
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JlLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JgLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
                m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                if (g_System->b32BitCore())
                {
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                if (Jump)
                {
                    m_CodeBlock.Log("      continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                if (Jump)
                {
                    m_CodeBlock.Log("      continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
            }
            else
            {
                m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32("BranchToJump", 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JleLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
}

void CX86RecompilerOps::BLTZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            if (GetMipsReg_S(m_Opcode.rs) < 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            if (GetMipsRegLo_S(m_Opcode.rs) < 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
        }
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
        }
    }
    else if (IsUnknown(m_Opcode.rs))
    {
        if (g_System->b32BitCore())
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            m_Assembler.JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CX86RecompilerOps::BGEZ_Compare()
{
    if (IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            CX86RecompilerOps::UnknownOpcode();
        }
        else if (IsSigned(m_Opcode.rs))
        {
            if (GetMipsRegLo_S(m_Opcode.rs) >= 0)
            {
                m_Section->m_Jump.FallThrough = true;
                m_Section->m_Cont.FallThrough = false;
            }
            else
            {
                m_Section->m_Jump.FallThrough = false;
                m_Section->m_Cont.FallThrough = true;
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = true;
            m_Section->m_Cont.FallThrough = false;
        }
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                m_Assembler.JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = true;
            m_Section->m_Cont.FallThrough = false;
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            m_Assembler.JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            m_Assembler.JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CX86RecompilerOps::COP1_BCF_Compare()
{
    m_Assembler.TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        m_Assembler.JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else
    {
        m_Assembler.JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
}

void CX86RecompilerOps::COP1_BCT_Compare()
{
    m_Assembler.TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        m_Assembler.JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else
    {
        m_Assembler.JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        m_Assembler.JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
}

//  Opcode functions
void CX86RecompilerOps::J()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2));
            OverflowDelaySlot(false);
            return;
        }

        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
        m_Section->m_Jump.JumpPC = m_CompilePC;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }
        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->GenerateSectionLinkage();
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (HaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nJ\nNextInstruction = %X", m_PipelineStage).c_str());
    }
}

void CX86RecompilerOps::JAL()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        Map_GPR_32bit(31, true, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(31), _PROGRAM_COUNTER, "_PROGRAM_COUNTER");
        m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(31), 0xF0000000);
        m_Assembler.AddConstToX86Reg(GetMipsRegMapLo(31), (m_CompilePC + 8) & ~0xF0000000);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2));
            OverflowDelaySlot(false);
            return;
        }
        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
        m_Section->m_Jump.JumpPC = m_CompilePC;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }
        m_Section->m_Jump.FallThrough = true;
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        if (m_Section->m_JumpSection)
        {
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->GenerateSectionLinkage();
        }
        else
        {
            m_RegWorkingSet.WriteBackRegisters();

            CX86Ops::x86Reg PCReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(PCReg, _PROGRAM_COUNTER, "_PROGRAM_COUNTER");
            m_Assembler.AndConstToX86Reg(PCReg, 0xF0000000);
            m_Assembler.AddConstToX86Reg(PCReg, (m_Opcode.target << 2));
            m_Assembler.MoveX86regToVariable(PCReg, _PROGRAM_COUNTER, "_PROGRAM_COUNTER");

            uint32_t TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
            bool bCheck = TargetPC <= m_CompilePC;
            UpdateCounters(m_RegWorkingSet, bCheck, true);

            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, bCheck ? ExitReason_Normal : ExitReason_NormalNoSysCheck, true, nullptr);
        }
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86RecompilerOps::ADDI()
{
    if (g_System->bFastSP() && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        m_Assembler.AddConstToX86Reg(Map_MemoryStack(CX86Ops::x86_Unknown, true), (int16_t)m_Opcode.immediate);
    }

    if (IsConst(m_Opcode.rs))
    {
        int32_t rs = GetMipsRegLo(m_Opcode.rs);
        int32_t imm = (int16_t)m_Opcode.immediate;
        int32_t sum = rs + imm;
        if ((~(rs ^ imm) & (rs ^ sum)) & 0x80000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rt != 0)
        {
            if (IsMapped(m_Opcode.rt))
            {
                UnMap_GPR(m_Opcode.rt, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, sum);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        }
    }
    else
    {
        ProtectGPR(m_Opcode.rt);
        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        m_Assembler.AddConstToX86Reg(Reg, (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rt != 0)
        {
            Map_GPR_32bit(m_Opcode.rt, true, -1);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), Reg);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::ADDIU()
{
    if (m_Opcode.rt == 0 || (m_Opcode.immediate == 0 && m_Opcode.rs == m_Opcode.rt))
    {
        return;
    }

    if (g_System->bFastSP())
    {
        if (m_Opcode.rs == 29 && m_Opcode.rt == 29)
        {
            m_Assembler.AddConstToX86Reg(Map_MemoryStack(CX86Ops::x86_Unknown, true), (int16_t)m_Opcode.immediate);
        }
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) + (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        m_Assembler.AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SLTIU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Result = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) < ((unsigned)((int64_t)((int16_t)m_Opcode.immediate))) ? 1 : 0 : GetMipsRegLo(m_Opcode.rs) < ((unsigned)((int16_t)m_Opcode.immediate)) ? 1
                                                                                                                                                                                                                : 0;
        UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            uint8_t * Jump[2];

            m_Assembler.CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            m_Assembler.JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            m_Assembler.JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Low Compare:");
            m_Assembler.SetJump8(Jump[0], *g_RecompPos);
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Continue:");
            m_Assembler.SetJump8(Jump[1], *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
    }
    else if (g_System->b32BitCore())
    {
        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
    }
    else
    {
        uint8_t * Jump = nullptr;

        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], ((int16_t)m_Opcode.immediate >> 31));
        m_Assembler.JneLabel8("CompareSet", 0);
        Jump = *g_RecompPos - 1;
        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      CompareSet:");
        m_Assembler.SetJump8(Jump, *g_RecompPos);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::SLTI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Result = Is64Bit(m_Opcode.rs) ? ((int64_t)GetMipsReg(m_Opcode.rs) < (int64_t)((int16_t)m_Opcode.immediate) ? 1 : 0) : (GetMipsRegLo_S(m_Opcode.rs) < (int16_t)m_Opcode.immediate ? 1 : 0);

        UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            uint8_t * Jump[2];

            m_Assembler.CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            m_Assembler.JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;
            m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            m_Assembler.JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Low Compare:");
            m_Assembler.SetJump8(Jump[0], *g_RecompPos);
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Continue:");
            m_Assembler.SetJump8(Jump[1], *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            /*    m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs),(int16_t)m_Opcode.immediate);
            m_Assembler.SetlVariable(&m_BranchCompare,"m_BranchCompare");
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt),&m_BranchCompare,"m_BranchCompare");
            */
            ProtectGPR(m_Opcode.rs);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);

            if (GetMipsRegMapLo(m_Opcode.rt) > CX86Ops::x86_EBX)
            {
                m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.Setl(GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 1);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);

        if (GetMipsRegMapLo(m_Opcode.rt) > CX86Ops::x86_EBX)
        {
            m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_Assembler.Setl(GetMipsRegMapLo(m_Opcode.rt));
            m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 1);
        }
    }
    else
    {
        uint8_t * Jump[2] = {nullptr, nullptr};
        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], ((int16_t)m_Opcode.immediate >> 31));
        m_Assembler.JeLabel8("Low Compare", 0);
        Jump[0] = *g_RecompPos - 1;
        m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
        m_Assembler.JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      Low Compare:");
        m_Assembler.SetJump8(Jump[0], *g_RecompPos);
        m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Continue:");
            m_Assembler.SetJump8(Jump[1], *g_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::ANDI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) & m_Opcode.immediate);
    }
    else if (m_Opcode.immediate != 0)
    {
        Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.rs);
        m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rt, false, 0);
    }
}

void CX86RecompilerOps::ORI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_System->bFastSP() && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        m_Assembler.OrConstToX86Reg(m_Opcode.immediate, Map_MemoryStack(CX86Ops::x86_Unknown, true));
    }

    if (IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) | m_Opcode.immediate);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            if (Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rt, IsSigned(m_Opcode.rs), m_Opcode.rs);
            }
        }
        m_Assembler.OrConstToX86Reg(m_Opcode.immediate, GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        }
        m_Assembler.OrConstToX86Reg(m_Opcode.immediate, GetMipsRegMapLo(m_Opcode.rt));
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::XORI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        if (m_Opcode.rs != m_Opcode.rt)
        {
            UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, GetMipsRegLo(m_Opcode.rs) ^ m_Opcode.immediate);
    }
    else
    {
        if (IsMapped(m_Opcode.rs) && Is32Bit(m_Opcode.rs))
        {
            Map_GPR_32bit(m_Opcode.rt, IsSigned(m_Opcode.rs), m_Opcode.rs);
        }
        else if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        }
        if (m_Opcode.immediate != 0)
        {
            m_Assembler.XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
        }
    }
}

void CX86RecompilerOps::LUI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        CX86Ops::x86Reg Reg = Map_MemoryStack(CX86Ops::x86_Unknown, true, false);
        uint32_t Address;

        m_MMU.VAddrToPAddr(((int16_t)m_Opcode.offset << 16), Address);
        if (Reg < 0)
        {
            m_Assembler.MoveConstToVariable(&(g_Recompiler->MemoryStackPos()), "MemoryStack", (uint32_t)(Address + g_MMU->Rdram()));
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, (uint32_t)(Address + g_MMU->Rdram()));
        }
    }

    UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, ((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
}

void CX86RecompilerOps::DADDI()
{
    int64_t imm = (int64_t)((int16_t)m_Opcode.immediate);

    if (IsConst(m_Opcode.rs))
    {
        int64_t rs = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs);
        int64_t sum = rs + imm;
        if ((~(rs ^ imm) & (rs ^ sum)) & 0x8000000000000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rt != 0)
        {
            if (IsMapped(m_Opcode.rt))
            {
                UnMap_GPR(m_Opcode.rt, false);
            }
            m_RegWorkingSet.SetMipsReg(m_Opcode.rt, sum);
            if (GetMipsRegLo_S(m_Opcode.rt) < 0 && GetMipsRegHi_S(m_Opcode.rt) == -1)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if (GetMipsRegLo_S(m_Opcode.rt) >= 0 && GetMipsRegHi_S(m_Opcode.rt) == 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_64);
            }
        }
    }
    else
    {
        ProtectGPR(m_Opcode.rs);
        CX86Ops::x86Reg RegLo = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        CX86Ops::x86Reg RegHi = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);

        m_Assembler.AddConstToX86Reg(RegLo, (uint32_t)imm, true);
        m_Assembler.AdcConstToX86Reg(RegHi, (uint32_t)(imm >> 32));
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rt != 0)
        {
            UnProtectGPR(m_Opcode.rs);
            Map_GPR_64bit(m_Opcode.rt, -1);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), RegLo);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rt), RegHi);
        }
    }
}

void CX86RecompilerOps::DADDIU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }
    int64_t imm = (int64_t)((int16_t)m_Opcode.immediate);

    if (IsConst(m_Opcode.rs))
    {
        int64_t rs = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs);
        if (IsMapped(m_Opcode.rt))
        {
            UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rt, rs + imm);
        if (GetMipsRegLo_S(m_Opcode.rt) < 0 && GetMipsRegHi_S(m_Opcode.rt) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rt) >= 0 && GetMipsRegHi_S(m_Opcode.rt) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        ProtectGPR(m_Opcode.rs);
        CX86Ops::x86Reg RegLo = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        CX86Ops::x86Reg RegHi = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);

        m_Assembler.AddConstToX86Reg(RegLo, (uint32_t)imm, true);
        m_Assembler.AdcConstToX86Reg(RegHi, (uint32_t)(imm >> 32));
        if (m_Opcode.rt != 0)
        {
            UnProtectGPR(m_Opcode.rs);
            Map_GPR_64bit(m_Opcode.rt, -1);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), RegLo);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rt), RegHi);
        }
    }
}

void CX86RecompilerOps::CACHE()
{
    if (g_Settings->LoadDword(Game_SMM_Cache) == 0)
    {
        return;
    }

    switch (m_Opcode.rt)
    {
    case 0:
    case 16:
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32("CRecompiler::Remove_Cache", CRecompiler::Remove_Cache);
        m_Assembler.PushImm32("0x20", 0x20);
        if (IsConst(m_Opcode.base))
        {
            uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            m_Assembler.PushImm32("Address", Address);
        }
        else if (IsMapped(m_Opcode.base))
        {
            m_Assembler.AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            m_Assembler.Push(GetMipsRegMapLo(m_Opcode.base));
        }
        else
        {
            m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EAX, &_GPR[m_Opcode.base].UW[0], CRegName::GPR_Lo[m_Opcode.base]);
            m_Assembler.AddConstToX86Reg(CX86Ops::x86_EAX, (int16_t)m_Opcode.offset);
            m_Assembler.Push(CX86Ops::x86_EAX);
        }
        m_Assembler.CallThis((uint32_t)g_Recompiler, AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 1:
    case 3:
    case 13:
    case 5:
    case 8:
    case 9:
    case 17:
    case 21:
    case 25:
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("cache: %d", m_Opcode.rt).c_str());
        }
    }
}

void CX86RecompilerOps::LDL()
{
    if (m_Opcode.base != 0)
    {
        UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::LDL, "R4300iOp::LDL");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::LDR()
{
    if (m_Opcode.base != 0)
    {
        UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::LDR, "R4300iOp::LDR");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::LB_KnownAddress(CX86Ops::x86Reg Reg, uint32_t VAddr, bool SignExtend)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileLoadMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 8, SignExtend);
        return;
    }

    uint32_t PAddr;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_Assembler.MoveConstToX86reg(Reg, 0);
        m_CodeBlock.Log("%s\nFailed to translate address %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (PAddr < g_MMU->RdramSize())
        {
            if (SignExtend)
            {
                m_Assembler.MoveSxVariableToX86regByte(Reg, (PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 3)", PAddr).c_str());
            }
            else
            {
                m_Assembler.MoveZxVariableToX86regByte((PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 3)", PAddr).c_str(), Reg);
            }
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, 0);
        }
        break;
    case 0x04000000:
        if (PAddr < 0x04001000)
        {
            if (SignExtend)
            {
                m_Assembler.MoveSxVariableToX86regByte(Reg, ((PAddr ^ 3) - 0x04000000) + g_MMU->Dmem(), stdstr_f("Dmem + (%X ^ 3)", (PAddr - 0x04000000)).c_str());
            }
            else
            {
                m_Assembler.MoveZxVariableToX86regByte(((PAddr ^ 3) - 0x04000000) + g_MMU->Dmem(), stdstr_f("Dmem + (%X ^ 3)", (PAddr - 0x04000000)).c_str(), Reg);
            }
        }
        else if (PAddr < 0x04002000)
        {
            if (SignExtend)
            {
                m_Assembler.MoveSxVariableToX86regByte(Reg, ((PAddr ^ 3) - 0x04001000) + g_MMU->Imem(), stdstr_f("Imem + (%X ^ 3)", (PAddr - 0x04001000)).c_str());
            }
            else
            {
                m_Assembler.MoveZxVariableToX86regByte(((PAddr ^ 3) - 0x04001000) + g_MMU->Imem(), stdstr_f("Imem + (%X ^ 3)", (PAddr - 0x04001000)).c_str(), Reg);
            }
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, 0);
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x10000000:
        if ((PAddr - 0x10000000) < g_Rom->GetRomSize())
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.PushImm32(((PAddr + 2) & ~0x3) & 0x1FFFFFFC);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][0], "RomMemoryHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            uint8_t Shift = (((PAddr & 1) ^ 3) << 3);
            if (Shift == 0x10)
            {
                m_Assembler.ShiftLeftSignImmed(Reg, 0x8);
            }
            if (SignExtend)
            {
                m_Assembler.ShiftRightSignImmed(Reg, 0x18);
            }
            else
            {
                m_Assembler.ShiftRightUnsignImmed(Reg, 0x18);
            }
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, 0);
        }
        break;
    default:
        m_Assembler.MoveConstToX86reg(Reg, 0);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::LH_KnownAddress(CX86Ops::x86Reg Reg, uint32_t VAddr, bool SignExtend)
{
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileLoadMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 16, SignExtend);
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_Assembler.MoveConstToX86reg(Reg, 0);
        m_CodeBlock.Log("%s\nFailed to translate address %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (PAddr < g_MMU->RdramSize())
        {
            if (SignExtend)
            {
                m_Assembler.MoveSxVariableToX86regHalf((PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str(), Reg);
            }
            else
            {
                m_Assembler.MoveZxVariableToX86regHalf((PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str(), Reg);
            }
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, 0);
        }
        break;
    case 0x10000000:
        if ((PAddr - 0x10000000) < g_Rom->GetRomSize())
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.PushImm32(((PAddr + 2) & ~0x3) & 0x1FFFFFFC);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][0], "RomMemoryHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            if (SignExtend)
            {
                m_Assembler.ShiftRightSignImmed(Reg, 16);
            }
            else
            {
                m_Assembler.ShiftRightUnsignImmed(Reg, 16);
            }
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, 0);
        }
        break;
    default:
        m_Assembler.MoveConstToX86reg(Reg, 0);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::RESERVED31()
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_IllegalInstruction, true, nullptr);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::LB()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP8(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_32bit(m_Opcode.rt, true, -1);
        LB_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address, true);
        return;
    }
    PreReadInstruction();
    Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    CompileLoadMemoryValue(CX86Ops::x86_Unknown, GetMipsRegMapLo(m_Opcode.rt), CX86Ops::x86_Unknown, 8, true);
}

void CX86RecompilerOps::LH()
{
    if (m_Opcode.rt == 0) return;

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP16(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_32bit(m_Opcode.rt, true, -1);
        LH_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address, true);
        return;
    }
    PreReadInstruction();
    CompileLoadMemoryValue(CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, 16, true);
}

void CX86RecompilerOps::LWL()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        uint32_t Offset = Address & 3;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        CX86Ops::x86Reg Value = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), R4300iOp::LWL_MASK[Offset]);
        m_Assembler.ShiftLeftSignImmed(Value, (uint8_t)R4300iOp::LWL_SHIFT[Offset]);
        m_Assembler.AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), Value);
    }
    else
    {
        PreReadInstruction();
        CX86Ops::x86Reg shift = Map_TempReg(CX86Ops::x86_ECX, -1, false, false);
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        CX86Ops::x86Reg AddressReg = BaseOffsetAddress(false);
        CX86Ops::x86Reg OffsetReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveX86RegToX86Reg(OffsetReg, AddressReg);
        m_Assembler.AndConstToX86Reg(OffsetReg, 3);
        m_Assembler.AndConstToX86Reg(AddressReg, (uint32_t)~3);
        TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
        CompileLoadMemoryValue(AddressReg, AddressReg, CX86Ops::x86_Unknown, 32, false);
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        m_Assembler.AndVariableDispToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (void *)R4300iOp::LWL_MASK, "LWL_MASK", OffsetReg, CX86Ops::Multip_x4);
        m_Assembler.MoveVariableDispToX86Reg((void *)R4300iOp::LWL_SHIFT, "LWL_SHIFT", shift, OffsetReg, 4);
        m_Assembler.ShiftLeftSign(AddressReg);
        m_Assembler.AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), AddressReg);
    }
}

void CX86RecompilerOps::LW()
{
    LW(true, false);
}

void CX86RecompilerOps::LW(bool ResultSigned, bool bRecordLLBit)
{
    if (m_Opcode.rt == 0) return;

    if (!HaveReadBP() && m_Opcode.base == 29 && g_System->bFastSP())
    {
        Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        CX86Ops::x86Reg TempReg1 = Map_MemoryStack(CX86Ops::x86_Unknown, true);
        m_Assembler.MoveVariableDispToX86Reg((void *)((uint32_t)(int16_t)m_Opcode.offset), stdstr_f("%Xh", (int16_t)m_Opcode.offset).c_str(), GetMipsRegMapLo(m_Opcode.rt), TempReg1, 1);
        if (bRecordLLBit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        LW_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address);
        if (bRecordLLBit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        PreReadInstruction();
        CompileLoadMemoryValue(CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, 32, false);
        if (bRecordLLBit)
        {
            m_Assembler.MoveConstToVariable(_LLBit, "LLBit", 1);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::LW_KnownAddress(CX86Ops::x86Reg Reg, uint32_t VAddr)
{
    m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileLoadMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 32, true);
    }
    else
    {
        uint32_t PAddr;
        if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }

        switch (PAddr & 0xFFF00000)
        {
        case 0x00000000:
        case 0x00100000:
        case 0x00200000:
        case 0x00300000:
        case 0x00400000:
        case 0x00500000:
        case 0x00600000:
        case 0x00700000:
            if (PAddr < g_MMU->RdramSize())
            {
                m_Assembler.MoveVariableToX86reg(Reg, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + 0x%X", PAddr).c_str());
            }
            else
            {
                m_Assembler.MoveConstToX86reg(Reg, 0);
            }
            break;
        case 0x04000000:
            if (PAddr < 0x04001000)
            {
                m_Assembler.MoveVariableToX86reg(Reg, (PAddr - 0x04000000) + g_MMU->Dmem(), stdstr_f("Dmem + 0x%X", (PAddr - 0x04000000)).c_str());
            }
            else if (PAddr < 0x04002000)
            {
                m_Assembler.MoveVariableToX86reg(Reg, (PAddr - 0x04001000) + g_MMU->Imem(), stdstr_f("Imem + 0x%X", (PAddr - 0x04001000)).c_str());
            }
            else
            {
                switch (PAddr)
                {
                case 0x04040010: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->SP_STATUS_REG, "SP_STATUS_REG"); break;
                case 0x04040014: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->SP_DMA_FULL_REG, "SP_DMA_FULL_REG"); break;
                case 0x04040018: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG"); break;
                case 0x0404001C:
                    m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
                    m_Assembler.MoveConstToVariable(&g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", 1);
                    break;
                case 0x04080000: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->SP_PC_REG, "SP_PC_REG"); break;
                default:
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
                    m_Assembler.PushImm32(PAddr | 0xA0000000);
                    m_Assembler.CallThis((uint32_t)(g_MMU), AddressOf(&CMipsMemoryVM::LW_NonMemory), "CMipsMemoryVM::LW_NonMemory", 12);
                    m_RegWorkingSet.AfterCallDirect();
                    m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
                    break;
                }
            }
            break;
        case 0x04100000:
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.PushImm32(PAddr | 0xA0000000);
            m_Assembler.CallThis((uint32_t)(g_MMU), AddressOf(&CMipsMemoryVM::LW_NonMemory), "CMipsMemoryVM::LW_NonMemory", 12);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            break;
        case 0x04300000:
            switch (PAddr)
            {
            case 0x04300000: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->MI_MODE_REG, "MI_MODE_REG"); break;
            case 0x04300004: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->MI_VERSION_REG, "MI_VERSION_REG"); break;
            case 0x04300008: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->MI_INTR_REG, "MI_INTR_REG"); break;
            case 0x0430000C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG"); break;
            default:
                m_Assembler.MoveConstToX86reg(Reg, 0);
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            break;
        case 0x04400000:
        {
            UpdateCounters(m_RegWorkingSet, false, true);

            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][0], "VideoInterfaceHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            break;
        }
        case 0x04500000:
        {
            UpdateCounters(m_RegWorkingSet, false, true, false);

            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][0], "AudioInterfaceHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            break;
        }
        case 0x04600000:
            switch (PAddr)
            {
            case 0x04600000: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG"); break;
            case 0x04600004: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG"); break;
            case 0x04600008: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG"); break;
            case 0x0460000C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG"); break;
            case 0x04600010: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_STATUS_REG, "PI_STATUS_REG"); break;
            case 0x04600014: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG"); break;
            case 0x04600018: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG"); break;
            case 0x0460001C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG"); break;
            case 0x04600020: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG"); break;
            case 0x04600024: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG"); break;
            case 0x04600028: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG"); break;
            case 0x0460002C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG"); break;
            case 0x04600030: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG"); break;
            default:
                m_Assembler.MoveConstToX86reg(Reg, 0);
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            break;
        case 0x04700000:
            switch (PAddr)
            {
            case 0x0470000C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->RI_SELECT_REG, "RI_SELECT_REG"); break;
            case 0x04700010: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG"); break;
            default:
                m_Assembler.MoveConstToX86reg(Reg, 0);
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            break;
        case 0x04800000:
            switch (PAddr)
            {
            case 0x04800018: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->SI_STATUS_REG, "SI_STATUS_REG"); break;
            default:
                m_Assembler.MoveConstToX86reg(Reg, 0);
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            break;
        case 0x05000000:
            // 64DD registers
            if (EnableDisk())
            {
                switch (PAddr)
                {
                case 0x05000500: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_DATA, "ASIC_DATA"); break;
                case 0x05000504: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_MISC_REG, "ASIC_MISC_REG"); break;
                case 0x05000508:
                    m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_STATUS, "ASIC_STATUS");
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc(AddressOf(&DiskGapSectorCheck), "DiskGapSectorCheck");
                    m_RegWorkingSet.AfterCallDirect();
                    break;
                case 0x0500050C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_CUR_TK, "ASIC_CUR_TK"); break;
                case 0x05000510: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_BM_STATUS, "ASIC_BM_STATUS"); break;
                case 0x05000514: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_ERR_SECTOR, "ASIC_ERR_SECTOR"); break;
                case 0x05000518: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_SEQ_STATUS, "ASIC_SEQ_STATUS"); break;
                case 0x0500051C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_CUR_SECTOR, "ASIC_CUR_SECTOR"); break;
                case 0x05000520: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_HARD_RESET, "ASIC_HARD_RESET"); break;
                case 0x05000524: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_C1_S0, "ASIC_C1_S0"); break;
                case 0x05000528: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE"); break;
                case 0x0500052C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_C1_S2, "ASIC_C1_S2"); break;
                case 0x05000530: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE"); break;
                case 0x05000534: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_C1_S4, "ASIC_C1_S4"); break;
                case 0x05000538: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_C1_S6, "ASIC_C1_S6"); break;
                case 0x0500053C: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_CUR_ADDR, "ASIC_CUR_ADDR"); break;
                case 0x05000540: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_ID_REG, "ASIC_ID_REG"); break;
                case 0x05000544: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_TEST_REG, "ASIC_TEST_REG"); break;
                case 0x05000548: m_Assembler.MoveVariableToX86reg(Reg, &g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL"); break;
                default:
                    m_Assembler.MoveConstToX86reg(Reg, 0);
                    if (BreakOnUnhandledMemory())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                }
            }
            else
            {
                m_Assembler.MoveConstToX86reg(Reg, (uint32_t)((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF));
            }
            break;
        case 0x1FC00000:
            m_Assembler.MoveVariableToX86reg(Reg, PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X").c_str());
            break;
        default:
            if ((PAddr & 0xF0000000) == 0x10000000 && (PAddr - 0x10000000) < g_Rom->GetRomSize())
            {
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
                m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
                m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][0], "RomMemoryHandler::Read32", 16);
                m_RegWorkingSet.AfterCallDirect();
                m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            }
            else if (g_DDRom != nullptr && ((PAddr & 0xFF000000) == 0x06000000 && (PAddr - 0x06000000) < g_DDRom->GetRomSize()))
            {
                m_Assembler.MoveVariableToX86reg(Reg, g_DDRom->GetRomAddress() + (PAddr - 0x06000000), stdstr_f("DDRom + %X", (PAddr - 0x06000000)).c_str());
            }
            else
            {
                m_Assembler.MoveConstToX86reg(Reg, ((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF));
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
    }
}

void CX86RecompilerOps::LBU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP8(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        LB_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address, false);
        return;
    }
    PreReadInstruction();
    Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    CompileLoadMemoryValue(CX86Ops::x86_Unknown, GetMipsRegMapLo(m_Opcode.rt), CX86Ops::x86_Unknown, 8, false);
}

void CX86RecompilerOps::LHU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP16(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        LH_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address, false);
        return;
    }
    PreReadInstruction();
    Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    CompileLoadMemoryValue(CX86Ops::x86_Unknown, GetMipsRegMapLo(m_Opcode.rt), CX86Ops::x86_Unknown, 16, false);
}

void CX86RecompilerOps::LWR()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        uint32_t Offset = Address & 3;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        CX86Ops::x86Reg Value = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), R4300iOp::LWR_MASK[Offset]);
        m_Assembler.ShiftRightUnsignImmed(Value, (uint8_t)R4300iOp::LWR_SHIFT[Offset]);
        m_Assembler.AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), Value);
    }
    else
    {
        PreReadInstruction();
        CX86Ops::x86Reg shift = Map_TempReg(CX86Ops::x86_ECX, -1, false, false);
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        CX86Ops::x86Reg AddressReg = BaseOffsetAddress(false);
        CX86Ops::x86Reg OffsetReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveX86RegToX86Reg(OffsetReg, AddressReg);
        m_Assembler.AndConstToX86Reg(OffsetReg, 3);
        m_Assembler.AndConstToX86Reg(AddressReg, (uint32_t)~3);
        TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
        CompileLoadMemoryValue(AddressReg, AddressReg, CX86Ops::x86_Unknown, 32, false);
        Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        m_Assembler.AndVariableDispToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (void *)R4300iOp::LWR_MASK, "LWR_MASK", OffsetReg, CX86Ops::Multip_x4);
        m_Assembler.MoveVariableDispToX86Reg((void *)R4300iOp::LWR_SHIFT, "LWR_SHIFT", shift, OffsetReg, 4);
        m_Assembler.ShiftRightUnsign(AddressReg);
        m_Assembler.AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), AddressReg);
    }
}

void CX86RecompilerOps::LWU()
{
    LW(false, false);
}

void CX86RecompilerOps::SB()
{
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveWriteBP() && g_Debugger->WriteBP8(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        if (IsConst(m_Opcode.rt))
        {
            SB_Const(GetMipsRegLo(m_Opcode.rt), Address);
        }
        else if (IsMapped(m_Opcode.rt) && m_Assembler.Is8BitReg(GetMipsRegMapLo(m_Opcode.rt)))
        {
            SB_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SB_Register(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, true), Address);
        }
        return;
    }
    PreWriteInstruction();

    CX86Ops::x86Reg ValueReg = CX86Ops::x86_Unknown;
    if (!IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, true) : GetMipsRegMapLo(m_Opcode.rt);
        if (IsMapped(m_Opcode.rt) && !m_Assembler.Is8BitReg(ValueReg))
        {
            UnProtectGPR(m_Opcode.rt);
            ValueReg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, true);
        }
    }
    CompileStoreMemoryValue(CX86Ops::x86_Unknown, ValueReg, CX86Ops::x86_Unknown, GetMipsRegLo(m_Opcode.rt), 8);
}

void CX86RecompilerOps::SH()
{
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveWriteBP() && g_Debugger->WriteBP16(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        if (IsConst(m_Opcode.rt))
        {
            SH_Const(GetMipsRegLo(m_Opcode.rt), Address);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SH_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SH_Register(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false), Address);
        }
        return;
    }

    PreWriteInstruction();

    CX86Ops::x86Reg ValueReg = CX86Ops::x86_Unknown;
    if (!IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false) : GetMipsRegMapLo(m_Opcode.rt);
    }
    CompileStoreMemoryValue(CX86Ops::x86_Unknown, ValueReg, CX86Ops::x86_Unknown, GetMipsRegLo(m_Opcode.rt), 16);
}

void CX86RecompilerOps::SWL()
{
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address;

        Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        uint32_t Offset = Address & 3;

        CX86Ops::x86Reg Value = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.AndConstToX86Reg(Value, R4300iOp::SWL_MASK[Offset]);
        CX86Ops::x86Reg TempReg1 = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
        m_Assembler.ShiftRightUnsignImmed(TempReg1, (uint8_t)R4300iOp::SWL_SHIFT[Offset]);
        m_Assembler.AddX86RegToX86Reg(Value, TempReg1);
        SW_Register(Value, (Address & ~3));
        return;
    }
    PreWriteInstruction();
    CX86Ops::x86Reg shift = Map_TempReg(CX86Ops::x86_ECX, -1, false, false), AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");

    CX86Ops::x86Reg TempReg2 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    CX86Ops::x86Reg OffsetReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    CX86Ops::x86Reg ValueReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveX86RegToX86Reg(TempReg2, AddressReg);
    m_Assembler.ShiftRightUnsignImmed(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    m_Assembler.CompConstToX86reg(TempReg2, (uint32_t)-1);
    m_Assembler.JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    m_Assembler.MoveX86RegToX86Reg(TempReg2, AddressReg);
    m_Assembler.ShiftRightUnsignImmed(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    m_Assembler.AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    m_CodeBlock.Log("");
    m_CodeBlock.Log(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    m_Assembler.SetJump8(JumpFound, *g_RecompPos);
    m_Assembler.MoveX86RegToX86Reg(OffsetReg, AddressReg);
    m_Assembler.AndConstToX86Reg(OffsetReg, 3);
    m_Assembler.AndConstToX86Reg(AddressReg, (uint32_t)~3);
    m_Assembler.MoveX86regPointerToX86reg(AddressReg, TempReg2, ValueReg);

    m_Assembler.AndVariableDispToX86Reg(ValueReg, (void *)R4300iOp::SWL_MASK, "R4300iOp::SWL_MASK", OffsetReg, CX86Ops::Multip_x4);
    if (!IsConst(m_Opcode.rt) || GetMipsRegLo(m_Opcode.rt) != 0)
    {
        m_Assembler.MoveVariableDispToX86Reg((void *)R4300iOp::SWL_SHIFT, "R4300iOp::SWL_SHIFT", shift, OffsetReg, 4);
        if (IsConst(m_Opcode.rt))
        {
            m_Assembler.MoveConstToX86reg(OffsetReg, GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            m_Assembler.MoveX86RegToX86Reg(OffsetReg, GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.MoveVariableToX86reg(OffsetReg, &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_Assembler.ShiftRightUnsign(OffsetReg);
        m_Assembler.AddX86RegToX86Reg(ValueReg, OffsetReg);
    }

    CompileStoreMemoryValue(AddressReg, ValueReg, CX86Ops::x86_Unknown, 0, 32);
}

void CX86RecompilerOps::SW()
{
    SW(false);
}

void CX86RecompilerOps::SW(bool bCheckLLbit)
{
    if (!HaveWriteBP() && m_Opcode.base == 29 && g_System->bFastSP())
    {
        if (bCheckLLbit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        CX86Ops::x86Reg TempReg1 = Map_MemoryStack(CX86Ops::x86_Unknown, true);

        if (IsConst(m_Opcode.rt))
        {
            m_Assembler.MoveConstToMemoryDisp(TempReg1, (uint32_t)((int16_t)m_Opcode.offset), GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            m_Assembler.MoveX86regToMemory(GetMipsRegMapLo(m_Opcode.rt), TempReg1, (uint32_t)((int16_t)m_Opcode.offset));
        }
        else
        {
            CX86Ops::x86Reg TempReg2 = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
            m_Assembler.MoveX86regToMemory(TempReg2, TempReg1, (uint32_t)((int16_t)m_Opcode.offset));
        }
    }
    else
    {
        if (IsConst(m_Opcode.base))
        {
            uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
            {
                FoundMemoryBreakpoint();
                return;
            }

            if (bCheckLLbit)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (IsConst(m_Opcode.rt))
            {
                SW_Const(GetMipsRegLo(m_Opcode.rt), Address);
            }
            else if (IsMapped(m_Opcode.rt))
            {
                ProtectGPR(m_Opcode.rt);
                SW_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
            }
            else
            {
                SW_Register(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false), Address);
            }
            return;
        }

        PreWriteInstruction();
        uint8_t * JumpLLBit = nullptr;
        if (bCheckLLbit)
        {
            m_Assembler.CompConstToVariable(_LLBit, "_LLBit", 1);
            m_Assembler.JneLabel8("LLBit_Continue", 0);
            JumpLLBit = *g_RecompPos - 1;
        }

        CX86Ops::x86Reg ValueReg = CX86Ops::x86_Unknown;
        if (!IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rt))
            {
                ProtectGPR(m_Opcode.rt);
            }
            ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false) : GetMipsRegMapLo(m_Opcode.rt);
        }
        CompileStoreMemoryValue(CX86Ops::x86_Unknown, ValueReg, CX86Ops::x86_Unknown, GetMipsRegLo(m_Opcode.rt), 32);
        if (bCheckLLbit)
        {
            m_CodeBlock.Log("      ");
            m_CodeBlock.Log("      LLBit_Continue:");
            m_Assembler.SetJump8(JumpLLBit, *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), _LLBit, "_LLBit");
        }
    }
}

void CX86RecompilerOps::SWR()
{
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        uint32_t Offset = Address & 3;

        CX86Ops::x86Reg Value = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.AndConstToX86Reg(Value, R4300iOp::SWR_MASK[Offset]);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
        m_Assembler.ShiftLeftSignImmed(TempReg, (uint8_t)R4300iOp::SWR_SHIFT[Offset]);
        m_Assembler.AddX86RegToX86Reg(Value, TempReg);
        SW_Register(Value, (Address & ~3));
        return;
    }
    PreWriteInstruction();
    CX86Ops::x86Reg shift = Map_TempReg(CX86Ops::x86_ECX, -1, false, false);
    CX86Ops::x86Reg AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");
    CX86Ops::x86Reg TempReg2 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    CX86Ops::x86Reg OffsetReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    CX86Ops::x86Reg ValueReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveX86RegToX86Reg(TempReg2, AddressReg);
    m_Assembler.ShiftRightUnsignImmed(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    m_Assembler.CompConstToX86reg(TempReg2, (uint32_t)-1);
    m_Assembler.JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    m_Assembler.MoveX86RegToX86Reg(TempReg2, AddressReg);
    m_Assembler.ShiftRightUnsignImmed(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    m_Assembler.AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    m_CodeBlock.Log("");
    m_CodeBlock.Log(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    m_Assembler.SetJump8(JumpFound, *g_RecompPos);

    m_Assembler.MoveX86RegToX86Reg(OffsetReg, AddressReg);
    m_Assembler.AndConstToX86Reg(OffsetReg, 3);
    m_Assembler.AndConstToX86Reg(AddressReg, (uint32_t)~3);

    m_Assembler.MoveX86regPointerToX86reg(AddressReg, TempReg2, ValueReg);

    m_Assembler.AndVariableDispToX86Reg(ValueReg, (void *)R4300iOp::SWR_MASK, "R4300iOp::SWR_MASK", OffsetReg, CX86Ops::Multip_x4);
    if (!IsConst(m_Opcode.rt) || GetMipsRegLo(m_Opcode.rt) != 0)
    {
        m_Assembler.MoveVariableDispToX86Reg((void *)R4300iOp::SWR_SHIFT, "R4300iOp::SWR_SHIFT", shift, OffsetReg, 4);
        if (IsConst(m_Opcode.rt))
        {
            m_Assembler.MoveConstToX86reg(OffsetReg, GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            m_Assembler.MoveX86RegToX86Reg(OffsetReg, GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.MoveVariableToX86reg(OffsetReg, &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_Assembler.ShiftLeftSign(OffsetReg);
        m_Assembler.AddX86RegToX86Reg(ValueReg, OffsetReg);
    }

    CompileStoreMemoryValue(AddressReg, ValueReg, CX86Ops::x86_Unknown, 0, 32);
}

void CX86RecompilerOps::SDL()
{
    if (m_Opcode.base != 0)
    {
        UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SDL, "R4300iOp::SDL");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SDR()
{
    if (m_Opcode.base != 0)
    {
        UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SDR, "R4300iOp::SDR");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::LL()
{
    LW(true, true);
}

void CX86RecompilerOps::LWC1()
{
    CompileCop1Test();
    if ((m_Opcode.ft & 1) != 0)
    {
        if (RegInStack(m_Opcode.ft - 1, CRegInfo::FPU_Double) || RegInStack(m_Opcode.ft - 1, CRegInfo::FPU_Qword))
        {
            UnMap_FPR(m_Opcode.ft - 1, true);
        }
    }
    if (RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) || RegInStack(m_Opcode.ft, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.ft, true);
    }
    else
    {
        UnMap_FPR(m_Opcode.ft, false);
    }
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        CX86Ops::x86Reg TempReg1 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        LW_KnownAddress(TempReg1, Address);

        CX86Ops::x86Reg TempReg2 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg2, &_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.MoveX86regToX86Pointer(TempReg2, TempReg1);
        return;
    }
    PreReadInstruction();
    CX86Ops::x86Reg ValueReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    CompileLoadMemoryValue(CX86Ops::x86_Unknown, ValueReg, CX86Ops::x86_Unknown, 32, false);
    CX86Ops::x86Reg FPR_SPtr = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(FPR_SPtr, &_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
    m_Assembler.MoveX86regToX86Pointer(FPR_SPtr, ValueReg);
}

void CX86RecompilerOps::LDC1()
{
    CompileCop1Test();

    UnMap_FPR(m_Opcode.ft, false);
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP64(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        CX86Ops::x86Reg TempReg1 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        LW_KnownAddress(TempReg1, Address);

        CX86Ops::x86Reg TempReg2 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg2, &_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.AddConstToX86Reg(TempReg2, 4);
        m_Assembler.MoveX86regToX86Pointer(TempReg2, TempReg1);

        LW_KnownAddress(TempReg1, Address + 4);
        m_Assembler.MoveVariableToX86reg(TempReg2, &_FPR_D[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.MoveX86regToX86Pointer(TempReg2, TempReg1);
    }
    else
    {
        PreReadInstruction();
        UnMap_FPR(m_Opcode.ft, true);

        CX86Ops::x86Reg ValueRegHi = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false), ValueRegLo = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        CompileLoadMemoryValue(CX86Ops::x86_Unknown, ValueRegLo, ValueRegHi, 64, false);

        CX86Ops::x86Reg FPR_DPtr = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(FPR_DPtr, &_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.MoveX86regToX86Pointer(FPR_DPtr, ValueRegLo);
        m_Assembler.AddConstToX86Reg(FPR_DPtr, 4);
        m_Assembler.MoveX86regToX86Pointer(FPR_DPtr, ValueRegHi);
    }
}

void CX86RecompilerOps::LD()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (!HaveReadBP() && m_Opcode.base == 29 && g_System->bFastSP())
    {
        Map_GPR_64bit(m_Opcode.rt, -1);
        CX86Ops::x86Reg StackReg = Map_MemoryStack(CX86Ops::x86_Unknown, true);
        m_Assembler.MoveVariableDispToX86Reg((void *)((uint32_t)(int16_t)m_Opcode.offset), stdstr_f("%Xh", (int16_t)m_Opcode.offset).c_str(), GetMipsRegMapHi(m_Opcode.rt), StackReg, 1);
        m_Assembler.MoveVariableDispToX86Reg((void *)((uint32_t)(int16_t)m_Opcode.offset + 4), stdstr_f("%Xh", (int16_t)m_Opcode.offset + 4).c_str(), GetMipsRegMapLo(m_Opcode.rt), StackReg, 1);
    }
    else if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP64(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        Map_GPR_64bit(m_Opcode.rt, -1);
        LW_KnownAddress(GetMipsRegMapHi(m_Opcode.rt), Address);
        LW_KnownAddress(GetMipsRegMapLo(m_Opcode.rt), Address + 4);
        if (g_System->bFastSP() && m_Opcode.rt == 29)
        {
            ResetMemoryStack();
        }
    }
    else
    {
        PreReadInstruction();
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }

        Map_GPR_64bit(m_Opcode.rt, m_Opcode.rt == m_Opcode.base ? m_Opcode.base : -1);
        CompileLoadMemoryValue(CX86Ops::x86_Unknown, GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapHi(m_Opcode.rt), 64, false);
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SC()
{
    SW(true);
}

void CX86RecompilerOps::SWC1()
{
    CompileCop1Test();

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        UnMap_FPR(m_Opcode.ft, true);
        CX86Ops::x86Reg TempReg1 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg1, &_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.MoveX86PointerToX86reg(TempReg1, TempReg1);
        SW_Register(TempReg1, Address);
        return;
    }
    PreWriteInstruction();
    UnMap_FPR(m_Opcode.ft, true);
    CX86Ops::x86Reg ValueReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(ValueReg, &_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
    m_Assembler.MoveX86PointerToX86reg(ValueReg, ValueReg);

    CompileStoreMemoryValue(CX86Ops::x86_Unknown, ValueReg, CX86Ops::x86_Unknown, 0, 32);
}

void CX86RecompilerOps::SDC1()
{
    CompileCop1Test();

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        CX86Ops::x86Reg TempReg1 = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg1, (uint8_t *)&_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.AddConstToX86Reg(TempReg1, 4);
        m_Assembler.MoveX86PointerToX86reg(TempReg1, TempReg1);
        SW_Register(TempReg1, Address);

        m_Assembler.MoveVariableToX86reg(TempReg1, &_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.MoveX86PointerToX86reg(TempReg1, TempReg1);
        SW_Register(TempReg1, Address + 4);
        return;
    }
    PreWriteInstruction();
    UnMap_FPR(m_Opcode.ft, true);
    CX86Ops::x86Reg ValueRegHi = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false), ValueRegLo = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(ValueRegHi, (uint8_t *)&_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
    m_Assembler.MoveX86RegToX86Reg(ValueRegLo, ValueRegHi);
    m_Assembler.AddConstToX86Reg(ValueRegHi, 4);
    m_Assembler.MoveX86PointerToX86reg(ValueRegHi, ValueRegHi);
    m_Assembler.MoveX86PointerToX86reg(ValueRegLo, ValueRegLo);

    CompileStoreMemoryValue(CX86Ops::x86_Unknown, ValueRegLo, ValueRegHi, 0, 64);
}

void CX86RecompilerOps::SD()
{
    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        if (IsConst(m_Opcode.rt))
        {
            SW_Const(Is64Bit(m_Opcode.rt) ? GetMipsRegHi(m_Opcode.rt) : (GetMipsRegLo_S(m_Opcode.rt) >> 31), Address);
            SW_Const(GetMipsRegLo(m_Opcode.rt), Address + 4);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SW_Register(Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false), Address);
            SW_Register(GetMipsRegMapLo(m_Opcode.rt), Address + 4);
        }
        else
        {
            CX86Ops::x86Reg TempReg1 = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false);
            SW_Register(TempReg1, Address);
            SW_Register(Map_TempReg(TempReg1, m_Opcode.rt, false, false), Address + 4);
        }
    }
    else
    {
        PreWriteInstruction();
        CX86Ops::x86Reg ValueReg = CX86Ops::x86_Unknown;
        if (!IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rt))
            {
                ProtectGPR(m_Opcode.rt);
            }
            ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false) : GetMipsRegMapLo(m_Opcode.rt);
        }
        uint64_t RtValue = 0;
        CX86Ops::x86Reg ValueRegHi = CX86Ops::x86_Unknown, ValueRegLo = CX86Ops::x86_Unknown;
        if (IsConst(m_Opcode.rt))
        {
            RtValue = ((uint64_t)(Is64Bit(m_Opcode.rt) ? GetMipsRegHi(m_Opcode.rt) : (uint32_t)(GetMipsRegLo_S(m_Opcode.rt) >> 31)) << 32) | GetMipsRegLo(m_Opcode.rt);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
            ValueRegHi = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false);
            ValueRegLo = GetMipsRegMapLo(m_Opcode.rt);
        }
        else
        {
            ValueRegHi = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false);
            ValueRegLo = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
        }
        CompileStoreMemoryValue(CX86Ops::x86_Unknown, ValueReg, ValueRegHi, RtValue, 64);
    }
}

// R4300i opcodes: Special
void CX86RecompilerOps::SPECIAL_SLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }
    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    if (m_Opcode.rd != m_Opcode.rt && IsMapped(m_Opcode.rt))
    {
        switch (m_Opcode.sa)
        {
        case 0:
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            break;
        case 1:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, CX86Ops::Multip_x2);
            break;
        case 2:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, CX86Ops::Multip_x4);
            break;
        case 3:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, CX86Ops::Multip_x8);
            break;
        default:
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    }
}

void CX86RecompilerOps::SPECIAL_SRL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_SRA()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_SLLV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) << Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        }
        return;
    }
    Map_TempReg(CX86Ops::x86_ECX, m_Opcode.rs, false, false);
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.ShiftLeftSign(GetMipsRegMapLo(m_Opcode.rd));
}

void CX86RecompilerOps::SPECIAL_SRLV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) >> Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }

    Map_TempReg(CX86Ops::x86_ECX, m_Opcode.rs, false, false);
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));
}

void CX86RecompilerOps::SPECIAL_SRAV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo_S(m_Opcode.rt) >> Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }
    Map_TempReg(CX86Ops::x86_ECX, m_Opcode.rs, false, false);
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.ShiftRightSign(GetMipsRegMapLo(m_Opcode.rd));
}

void CX86RecompilerOps::SPECIAL_JR()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
            }
            OverflowDelaySlot(true);
            return;
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = nullptr;
        m_Section->m_Cont.LinkLocation2 = nullptr;

        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value) &&
            R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            if (IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", GetMipsRegLo(m_Opcode.rs));
            }
            else if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
        }
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value) && R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, ExitReason_Normal, true, nullptr);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            if (IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", GetMipsRegLo(m_Opcode.rs));
            }
            else if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_Normal, true, nullptr);
            if (m_Section->m_JumpSection)
            {
                m_Section->GenerateSectionLinkage();
            }
        }
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (HaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nBranch\nNextInstruction = %X", m_PipelineStage).c_str());
    }
}

void CX86RecompilerOps::SPECIAL_JALR()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value) &&
            R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value) && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            if (IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", GetMipsRegLo(m_Opcode.rs));
            }
            else if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
        }
        UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_CompilePC + 8);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
            }
            OverflowDelaySlot(true);
            return;
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = nullptr;
        m_Section->m_Cont.LinkLocation2 = nullptr;

        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value) &&
            R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, ExitReason_Normal, true, nullptr);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            if (IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", GetMipsRegLo(m_Opcode.rs));
            }
            else if (IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_Normal, true, nullptr);
            if (m_Section->m_JumpSection)
            {
                m_Section->GenerateSectionLinkage();
            }
        }
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
    else if (HaveDebugger())
    {
        g_Notify->DisplayError(stdstr_f("WTF\n\nBranch\nNextInstruction = %X", m_PipelineStage).c_str());
    }
}

void CX86RecompilerOps::SPECIAL_SYSCALL()
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_DoSysCall, true, nullptr);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::SPECIAL_BREAK()
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_Break, true, nullptr);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::SPECIAL_MFLO()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    Map_GPR_64bit(m_Opcode.rd, -1);
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_RegLO->UW[0], "_RegLO->UW[0]");
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_RegLO->UW[1], "_RegLO->UW[1]");
}

void CX86RecompilerOps::SPECIAL_MTLO()
{
    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", GetMipsRegHi(m_Opcode.rs));
        }
        else if (IsSigned(m_Opcode.rs) && ((GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
        }
        else
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0);
        }
        m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", GetMipsRegLo(m_Opcode.rs));
    }
    else if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.rs), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0);
        }
        m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
    else
    {
        CX86Ops::x86Reg reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
        m_Assembler.MoveX86regToVariable(reg, &_RegLO->UW[1], "_RegLO->UW[1]");
        m_Assembler.MoveX86regToVariable(Map_TempReg(reg, m_Opcode.rs, false, false), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
}

void CX86RecompilerOps::SPECIAL_MFHI()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    Map_GPR_64bit(m_Opcode.rd, -1);
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_RegHI->UW[0], "_RegHI->UW[0]");
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_MTHI()
{
    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", GetMipsRegHi(m_Opcode.rs));
        }
        else if (IsSigned(m_Opcode.rs) && ((GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0xFFFFFFFF);
        }
        else
        {
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
        }
        m_Assembler.MoveConstToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", GetMipsRegLo(m_Opcode.rs));
    }
    else if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.rs), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else
        {
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
        }
        m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
    else
    {
        CX86Ops::x86Reg reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
        m_Assembler.MoveX86regToVariable(reg, &_RegHI->UW[1], "_RegHI->UW[1]");
        m_Assembler.MoveX86regToVariable(Map_TempReg(reg, m_Opcode.rs, false, false), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
}

void CX86RecompilerOps::SPECIAL_DSLLV()
{
    uint8_t * Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }
    Map_TempReg(CX86Ops::x86_ECX, m_Opcode.rs, false, false);
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x3F);
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.CompConstToX86reg(CX86Ops::x86_ECX, 0x20);
    m_Assembler.JaeLabel8("MORE32", 0);
    Jump[0] = *g_RecompPos - 1;
    m_Assembler.ShiftLeftDouble(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    m_Assembler.ShiftLeftSign(GetMipsRegMapLo(m_Opcode.rd));
    m_Assembler.JmpLabel8("Continue", 0);
    Jump[1] = *g_RecompPos - 1;

    // MORE32:
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MORE32:");
    m_Assembler.SetJump8(Jump[0], *g_RecompPos);
    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    m_Assembler.XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    m_Assembler.ShiftLeftSign(GetMipsRegMapHi(m_Opcode.rd));

    // Continue:
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      continue:");
    m_Assembler.SetJump8(Jump[1], *g_RecompPos);
}

void CX86RecompilerOps::SPECIAL_DSRLV()
{
    uint8_t * Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x3F);
        if (IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt));
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, GetMipsReg(m_Opcode.rd) >> Shift);
            if ((GetMipsRegHi(m_Opcode.rd) == 0) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) == 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if ((GetMipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) != 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
            }
            return;
        }
        Map_TempReg(CX86Ops::x86_ECX, -1, false, false);
        m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, Shift);
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        if ((Shift & 0x20) == 0x20)
        {
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
            m_Assembler.ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            m_Assembler.ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.ShiftRightUnsign(GetMipsRegMapHi(m_Opcode.rd));
        }
    }
    else
    {
        Map_TempReg(CX86Ops::x86_ECX, m_Opcode.rs, false, false);
        m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x3F);
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.CompConstToX86reg(CX86Ops::x86_ECX, 0x20);
        m_Assembler.JaeLabel8("MORE32", 0);
        Jump[0] = *g_RecompPos - 1;
        m_Assembler.ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.ShiftRightUnsign(GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;

        // MORE32:
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      MORE32:");
        m_Assembler.SetJump8(Jump[0], *g_RecompPos);
        m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
        m_Assembler.ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));

        // Continue:
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      continue:");
        m_Assembler.SetJump8(Jump[1], *g_RecompPos);
    }
}

void CX86RecompilerOps::SPECIAL_DSRAV()
{
    uint8_t * Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }
    Map_TempReg(CX86Ops::x86_ECX, m_Opcode.rs, false, false);
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x3F);
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.CompConstToX86reg(CX86Ops::x86_ECX, 0x20);
    m_Assembler.JaeLabel8("MORE32", 0);
    Jump[0] = *g_RecompPos - 1;
    m_Assembler.ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
    m_Assembler.ShiftRightSign(GetMipsRegMapHi(m_Opcode.rd));
    m_Assembler.JmpLabel8("Continue", 0);
    Jump[1] = *g_RecompPos - 1;

    // MORE32:
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      MORE32:");
    m_Assembler.SetJump8(Jump[0], *g_RecompPos);
    m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
    m_Assembler.ShiftRightSignImmed(GetMipsRegMapHi(m_Opcode.rd), 0x1F);
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    m_Assembler.ShiftRightSign(GetMipsRegMapLo(m_Opcode.rd));

    // Continue:
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      continue:");
    m_Assembler.SetJump8(Jump[1], *g_RecompPos);
}

void CX86RecompilerOps::SPECIAL_MULT()
{
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, false, false);
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, false);
    Map_TempReg(CX86Ops::x86_EDX, m_Opcode.rt, false, false);

    m_Assembler.imulX86reg(CX86Ops::x86_EDX);

    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EAX, 31); // Paired
    m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EDX, 31);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_MULTU()
{
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, false, false);
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, false);
    Map_TempReg(CX86Ops::x86_EDX, m_Opcode.rt, false, false);

    m_Assembler.MulX86reg(CX86Ops::x86_EDX);

    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EAX, 31); // Paired
    m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EDX, 31);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_DIV()
{
    CX86Ops::x86Reg RegRs = CX86Ops::x86_Unknown, RegRsHi = CX86Ops::x86_Unknown, DivReg = CX86Ops::x86_Unknown;
    uint8_t * JumpNotDiv0 = nullptr;
    uint8_t * JumpEnd = nullptr;
    uint8_t * JumpEnd2 = nullptr;

    if (IsConst(m_Opcode.rt) && GetMipsRegLo(m_Opcode.rt) == 0)
    {
        if (IsConst(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0x00000001 : 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0x00000000 : 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", GetMipsRegLo(m_Opcode.rs));
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0xFFFFFFFF : 0x00000000);
        }
        else
        {
            CX86Ops::x86Reg Reg = IsMapped(m_Opcode.rs) ? GetMipsRegMapLo(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
            m_Assembler.CompConstToX86reg(Reg, 0);
            m_Assembler.JgeLabel8(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpPositive = *g_RecompPos - 1;
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0x00000001);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0x00000000);
            m_Assembler.JmpLabel8(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpLoSet = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      RsPositive_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpPositive, *g_RecompPos);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      LoSet_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpLoSet, *g_RecompPos);
            m_Assembler.MoveX86regToVariable(Reg, &_RegHI->UW[0], "_RegHI->UW[0]");
            if (IsMapped(m_Opcode.rs))
            {
                Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
            }
            else
            {
                m_Assembler.ShiftRightSignImmed(Reg, 31);
            }
            m_Assembler.MoveX86regToVariable(Reg, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        return;
    }
    else if (IsConst(m_Opcode.rt) && GetMipsRegLo(m_Opcode.rt) == -1)
    {
        if (IsConst(m_Opcode.rs) && GetMipsRegLo(m_Opcode.rs) == 0x80000000)
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0x80000000);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", 0x00000000);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0x00000000);
            return;
        }

        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        UnMap_X86reg(CX86Ops::x86_EDX);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
        UnMap_X86reg(CX86Ops::x86_EAX);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        RegRs = IsMapped(m_Opcode.rs) ? GetMipsRegMapLo(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(RegRs), true);
        RegRsHi = IsMapped(m_Opcode.rs) && Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, IsMapped(m_Opcode.rs) ? m_Opcode.rs : -1, true, false);
        DivReg = IsMapped(m_Opcode.rt) ? GetMipsRegMapLo(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);

        if (!IsConst(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(RegRs, 0x80000000);
            m_Assembler.JneLabel8(stdstr_f("ValidDiv_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpValid = *g_RecompPos - 1;
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0x80000000);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", 0x00000000);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0x00000000);
            m_Assembler.JmpLabel8(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), 0);
            JumpEnd = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      ValidDiv_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpValid, *g_RecompPos);
        }
    }
    else
    {
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        UnMap_X86reg(CX86Ops::x86_EDX);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
        UnMap_X86reg(CX86Ops::x86_EAX);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        RegRs = IsMapped(m_Opcode.rs) ? GetMipsRegMapLo(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(RegRs), true);
        RegRsHi = IsMapped(m_Opcode.rs) && Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, IsMapped(m_Opcode.rs) ? m_Opcode.rs : -1, true, false);
        DivReg = IsMapped(m_Opcode.rt) ? GetMipsRegMapLo(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);

        if (!IsConst(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(DivReg, 0);
            m_Assembler.JneLabel8(stdstr_f("NotDiv0_%08X", m_CompilePC).c_str(), 0);
            JumpNotDiv0 = *g_RecompPos - 1;

            m_Assembler.CompConstToX86reg(RegRs, 0);
            m_Assembler.JgeLabel8(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpPositive = *g_RecompPos - 1;
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0x00000001);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0x00000000);
            m_Assembler.JmpLabel8(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpLoSet = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      RsPositive_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpPositive, *g_RecompPos);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      LoSet_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpLoSet, *g_RecompPos);
            m_Assembler.MoveX86regToVariable(RegRs, &_RegHI->UW[0], "_RegHI->UW[0]");
            if (!IsMapped(m_Opcode.rs))
            {
                m_Assembler.ShiftRightSignImmed(RegRsHi, 31);
            }
            m_Assembler.MoveX86regToVariable(RegRsHi, &_RegHI->UW[1], "_RegHI->UW[1]");
            m_Assembler.JmpLabel8(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), 0);
            JumpEnd = *g_RecompPos - 1;

            m_CodeBlock.Log("");
            m_CodeBlock.Log("      NotDiv0_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpNotDiv0, *g_RecompPos);
            if (IsMapped(m_Opcode.rt))
            {
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rt), (uint32_t)-1);
            }
            else
            {
                m_Assembler.CompConstToVariable(&_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt], (uint32_t)-1);
            }
            m_Assembler.JneLabel8(stdstr_f("ValidDiv0_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpValidDiv0 = *g_RecompPos - 1;

            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0x80000000);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", 0x00000000);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0x00000000);
            m_Assembler.JmpLabel8(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), 0);
            JumpEnd2 = *g_RecompPos - 1;

            m_CodeBlock.Log("");
            m_CodeBlock.Log("      ValidDiv0_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpValidDiv0, *g_RecompPos);
        }
    }

    m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
    UnMap_X86reg(CX86Ops::x86_EDX);
    Map_TempReg(CX86Ops::x86_EDX, -1, false, false);
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, false, false);

    if (IsConst(m_Opcode.rs))
    {
        m_Assembler.MoveConstToX86reg(CX86Ops::x86_EDX, GetMipsRegLo_S(m_Opcode.rs) >> 31);
    }
    else
    {
        m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_EDX, CX86Ops::x86_EAX);
        m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EDX, 31);
    }

    m_Assembler.idivX86reg(DivReg);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EAX, 31);
    m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EDX, 31);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

    if (JumpEnd != nullptr || JumpEnd2 != nullptr)
    {
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      EndDiv_%08X:", m_CompilePC);
        if (JumpEnd != nullptr)
        {
            m_Assembler.SetJump8(JumpEnd, *g_RecompPos);
        }
        if (JumpEnd2 != nullptr)
        {
            m_Assembler.SetJump8(JumpEnd2, *g_RecompPos);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DIVU()
{
    uint8_t * JumpEndDivu = nullptr;
    if (IsConst(m_Opcode.rt) && GetMipsRegLo(m_Opcode.rt) == 0)
    {
        if (IsConst(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", GetMipsRegLo(m_Opcode.rs));
            m_Assembler.MoveConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0xFFFFFFFF : 0x00000000);
        }
        else
        {
            CX86Ops::x86Reg RegRs = IsMapped(m_Opcode.rs) ? GetMipsRegMapLo(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
            m_Assembler.CompConstToX86reg(RegRs, 0);
            m_Assembler.JgeLabel8(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpPositive = *g_RecompPos - 1;
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0x00000001);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0x00000000);
            m_Assembler.JmpLabel8(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), 0);
            uint8_t * JumpLoSet = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      RsPositive_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpPositive, *g_RecompPos);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      LoSet_%08X:", m_CompilePC);
            m_Assembler.SetJump8(JumpLoSet, *g_RecompPos);
            m_Assembler.MoveX86regToVariable(RegRs, &_RegHI->UW[0], "_RegHI->UW[0]");
            if (IsMapped(m_Opcode.rs))
            {
                RegRs = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
            }
            else
            {
                m_Assembler.ShiftRightSignImmed(RegRs, 31);
            }
            m_Assembler.MoveX86regToVariable(RegRs, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
    }
    else
    {
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        UnMap_X86reg(CX86Ops::x86_EDX);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
        UnMap_X86reg(CX86Ops::x86_EAX);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        CX86Ops::x86Reg RegRsLo = IsMapped(m_Opcode.rs) ? GetMipsRegMapLo(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        CX86Ops::x86Reg RegRsHi = IsMapped(m_Opcode.rs) ? Map_TempReg(CX86Ops::x86_Unknown, IsMapped(m_Opcode.rs), true, false) : CX86Ops::x86_Unknown;
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        Map_TempReg(CX86Ops::x86_EDX, 0, false, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);

        Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, false, false);
        CX86Ops::x86Reg DivReg = IsMapped(m_Opcode.rt) ? GetMipsRegMapLo(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);

        if (!IsConst(m_Opcode.rt))
        {
            m_Assembler.CompConstToX86reg(DivReg, 0);
            m_Assembler.JneLabel8("NoExcept", 0);
            uint8_t * JumpNoExcept = *g_RecompPos - 1;

            m_Assembler.MoveConstToVariable(&_RegLO->UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveX86regToVariable(RegRsLo, &_RegHI->UW[0], "_RegHI->UW[0]");
            if (!IsMapped(m_Opcode.rs))
            {
                RegRsHi = RegRsLo;
                m_Assembler.ShiftRightSignImmed(RegRsHi, 31);
            }
            m_Assembler.MoveX86regToVariable(RegRsHi, &_RegHI->UW[1], "_RegHI->UW[1]");

            m_Assembler.JmpLabel8("EndDivu", 0);
            JumpEndDivu = *g_RecompPos - 1;
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      NoExcept:");
            m_Assembler.SetJump8(JumpNoExcept, *g_RecompPos);
        }
        m_Assembler.DivX86reg(DivReg);

        m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
        m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
        m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EAX, 31);
        m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EDX, 31);
        m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
        m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

        if (JumpEndDivu != nullptr)
        {
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      EndDivu:");
            m_Assembler.SetJump8(JumpEndDivu, *g_RecompPos);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DMULT()
{
    if (m_Opcode.rs != 0)
    {
        UnMap_GPR(m_Opcode.rs, true);
    }

    if (m_Opcode.rs != 0)
    {
        UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DMULTU()
{
    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DMULTU, "R4300iOp::SPECIAL_DMULTU");
    m_RegWorkingSet.AfterCallDirect();

#ifdef toremove
    /* _RegLO->UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
    X86Protected(CX86Ops::x86_EDX) = true;
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, false);
    X86Protected(CX86Ops::x86_EDX) = false;
    Map_TempReg(CX86Ops::x86_EDX, m_Opcode.rt, false);

    m_Assembler.MulX86reg(CX86Ops::x86_EDX);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

    /* _RegHI->UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, true);
    Map_TempReg(CX86Ops::x86_EDX, m_Opcode.rt, true);

    m_Assembler.MulX86reg(CX86Ops::x86_EDX);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

    /* Tmp[0].UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, true);
    Map_TempReg(CX86Ops::x86_EDX, m_Opcode.rt, false);

    Map_TempReg(CX86Ops::x86_EBX, -1, false);
    Map_TempReg(CX86Ops::x86_ECX, -1, false);

    m_Assembler.MulX86reg(CX86Ops::x86_EDX);
    m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_EBX, CX86Ops::x86_EAX); // EDX:EAX -> ECX:EBX
    m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_ECX, CX86Ops::x86_EDX);

    /* Tmp[1].UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
    Map_TempReg(CX86Ops::x86_EAX, m_Opcode.rs, false);
    Map_TempReg(CX86Ops::x86_EDX, m_Opcode.rt, true);

    m_Assembler.MulX86reg(CX86Ops::x86_EDX);
    Map_TempReg(CX86Ops::x86_ESI, -1, false);
    Map_TempReg(CX86Ops::x86_EDI, -1, false);
    m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_ESI, CX86Ops::x86_EAX); // EDX:EAX -> EDI:ESI
    m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_EDI, CX86Ops::x86_EDX);

    /* Tmp[2].UDW = (uint64)_RegLO->UW[1] + (uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]; */
    m_Assembler.XorX86RegToX86Reg(CX86Ops::x86_EDX, CX86Ops::x86_EDX);
    m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    m_Assembler.AddX86RegToX86Reg(CX86Ops::x86_EAX, CX86Ops::x86_EBX);
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_EDX, 0);
    m_Assembler.AddX86RegToX86Reg(CX86Ops::x86_EAX, CX86Ops::x86_ESI);
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_EDX, 0); // EDX:EAX

    /* _RegLO->UDW += ((uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]) << 32; */
    /* [low+4] += ebx + esi */

    AddX86regToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", CX86Ops::x86_EBX);
    AddX86regToVariable(&_RegLO->UW[1], "_RegLO->UW[1]", CX86Ops::x86_ESI);

    /* _RegHI->UDW += (uint64)Tmp[0].UW[1] + (uint64)Tmp[1].UW[1] + Tmp[2].UW[1]; */
    /* [hi] += ecx + edi + edx */

    AddX86regToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", CX86Ops::x86_ECX);
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

    AddX86regToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", CX86Ops::x86_EDI);
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

    AddX86regToVariable(&_RegHI->UW[0], "_RegHI->UW[0]", CX86Ops::x86_EDX);
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
#endif
}

void CX86RecompilerOps::SPECIAL_DDIV()
{
    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DDIVU()
{
    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_ADD()
{
    int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    if (IsConst(source1) && IsConst(source2))
    {
        int32_t Val1 = GetMipsRegLo(source1);
        int32_t Val2 = GetMipsRegLo(source2);
        int32_t Sum = Val1 + Val2;
        if ((~(Val1 ^ Val2) & (Val1 ^ Sum)) & 0x80000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rd != 0)
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, Sum);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        return;
    }

    ProtectGPR(m_Opcode.rd);
    CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, source1, false, false);
    if (IsConst(source2))
    {
        m_Assembler.AddConstToX86Reg(Reg, GetMipsRegLo(source2));
    }
    else if (IsKnown(source2) && IsMapped(source2))
    {
        m_Assembler.AddX86RegToX86Reg(Reg, GetMipsRegMapLo(source2));
    }
    else
    {
        m_Assembler.AddVariableToX86reg(Reg, &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel32);
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    if (m_Opcode.rd != 0)
    {
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
    }
}

void CX86RecompilerOps::SPECIAL_ADDU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    if (IsConst(source1) && IsConst(source2))
    {
        uint32_t temp = GetMipsRegLo(source1) + GetMipsRegLo(source2);
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }

    Map_GPR_32bit(m_Opcode.rd, true, source1);
    if (IsConst(source2))
    {
        m_Assembler.AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
    }
    else if (IsKnown(source2) && IsMapped(source2))
    {
        m_Assembler.AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
    }
    else
    {
        m_Assembler.AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_SUB()
{
    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        int32_t rs = GetMipsRegLo(m_Opcode.rs);
        int32_t rt = GetMipsRegLo(m_Opcode.rt);
        int32_t sub = rs - rt;

        if (((rs ^ rt) & (rs ^ sub)) & 0x80000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rd != 0)
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, sub);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
    }
    else
    {
        ProtectGPR(m_Opcode.rd);
        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        if (IsConst(m_Opcode.rt))
        {
            m_Assembler.SubConstFromX86Reg(Reg, GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            m_Assembler.SubX86RegToX86Reg(Reg, GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rd != 0)
        {
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_SUBU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        uint32_t temp = GetMipsRegLo(m_Opcode.rs) - GetMipsRegLo(m_Opcode.rt);

        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            m_Assembler.SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            m_Assembler.SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            m_Assembler.SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
    }

    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_AND()
{
    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                           (Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)) &
                                               (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs)));

                if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
                }
            }
            else
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) & GetMipsReg(m_Opcode.rs));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(source1);
            ProtectGPR(source2);
            if (Is32Bit(source1) && Is32Bit(source2))
            {
                bool Sign = (IsSigned(m_Opcode.rt) && IsSigned(m_Opcode.rs));
                Map_GPR_32bit(m_Opcode.rd, Sign, source1);
                m_Assembler.AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
            else if (Is32Bit(source1) || Is32Bit(source2))
            {
                if (IsUnsigned(Is32Bit(source1) ? source1 : source2))
                {
                    Map_GPR_32bit(m_Opcode.rd, false, source1);
                    m_Assembler.AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
                else
                {
                    Map_GPR_64bit(m_Opcode.rd, source1);
                    if (Is32Bit(source2))
                    {
                        m_Assembler.AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(CX86Ops::x86_Unknown, source2, true, false));
                    }
                    else
                    {
                        m_Assembler.AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                    }
                    m_Assembler.AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                m_Assembler.AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                m_Assembler.AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
        }
        else
        {
            int ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            int MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(ConstReg))
            {
                if (Is32Bit(MappedReg) && IsUnsigned(MappedReg))
                {
                    if (GetMipsRegLo(ConstReg) == 0)
                    {
                        Map_GPR_32bit(m_Opcode.rd, false, 0);
                    }
                    else
                    {
                        uint32_t Value = GetMipsRegLo(ConstReg);
                        Map_GPR_32bit(m_Opcode.rd, false, MappedReg);
                        m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                    }
                }
                else
                {
                    int64_t Value = GetMipsReg(ConstReg);
                    Map_GPR_64bit(m_Opcode.rd, MappedReg);
                    m_Assembler.AndConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                    m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
                }
            }
            else if (Is64Bit(MappedReg))
            {
                uint32_t Value = GetMipsRegLo(ConstReg);
                if (Value != 0)
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(ConstReg), MappedReg);
                    m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(ConstReg), 0);
                }
            }
            else
            {
                uint32_t Value = GetMipsRegLo(ConstReg);
                bool Sign = false;

                if (IsSigned(ConstReg) && IsSigned(MappedReg))
                {
                    Sign = true;
                }

                if (Value != 0)
                {
                    Map_GPR_32bit(m_Opcode.rd, Sign, MappedReg);
                    m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, false, 0);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            if (Is64Bit(KnownReg))
            {
                uint64_t Value = GetMipsReg(KnownReg);
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                m_Assembler.AndConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
            }
            else
            {
                uint32_t Value = GetMipsRegLo(KnownReg);
                Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), UnknownReg);
                m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
            }
        }
        else
        {
            ProtectGPR(KnownReg);
            if (KnownReg == m_Opcode.rd)
            {
                if (Is64Bit(KnownReg) || !g_System->b32BitCore())
                {
                    Map_GPR_64bit(m_Opcode.rd, KnownReg);
                    m_Assembler.AndVariableToX86Reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                    m_Assembler.AndVariableToX86Reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), KnownReg);
                    m_Assembler.AndVariableToX86Reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                    m_Assembler.AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(KnownReg));
                    m_Assembler.AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), UnknownReg);
                    m_Assembler.AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
                }
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            m_Assembler.AndVariableToX86Reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        m_Assembler.AndVariableToX86Reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
    }
}

void CX86RecompilerOps::SPECIAL_OR()
{
    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                           (Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)) |
                                               (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs)));
                if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
                }
            }
            else
            {
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) | GetMipsRegLo(m_Opcode.rs));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                if (Is64Bit(source2))
                {
                    m_Assembler.OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else
                {
                    m_Assembler.OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(CX86Ops::x86_Unknown, source2, true, false));
                }
            }
            else
            {
                ProtectGPR(source2);
                Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            m_Assembler.OrX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint64_t Value;

                if (Is64Bit(ConstReg))
                {
                    Value = GetMipsReg(ConstReg);
                }
                else
                {
                    Value = IsSigned(ConstReg) ? (int64_t)GetMipsRegLo_S(ConstReg) : GetMipsRegLo(ConstReg);
                }
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0)
                {
                    m_Assembler.OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0)
                {
                    m_Assembler.OrConstToX86Reg(Value, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        int KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            uint64_t Value = Is64Bit(KnownReg) ? GetMipsReg(KnownReg) : GetMipsRegLo_S(KnownReg);
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);

            if (g_System->b32BitCore() && Is32Bit(KnownReg))
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (dwValue != 0)
                {
                    m_Assembler.OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                if (dwValue != 0)
                {
                    m_Assembler.OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                m_Assembler.OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                m_Assembler.OrVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                m_Assembler.OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_XOR()
{
    if (m_Opcode.rd == 0)
        return;

    if (m_Opcode.rt == m_Opcode.rs)
    {
        UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
        return;
    }
    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                if (HaveDebugger())
                {
                    g_Notify->DisplayError("XOR 1");
                }
                CX86RecompilerOps::UnknownOpcode();
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) ^ GetMipsRegLo(m_Opcode.rs));
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(source1);
            ProtectGPR(source2);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                if (Is64Bit(source2))
                {
                    m_Assembler.XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else if (IsSigned(source2))
                {
                    m_Assembler.XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(CX86Ops::x86_Unknown, source2, true, false));
                }
                m_Assembler.XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
            else
            {
                if (IsSigned(m_Opcode.rt) != IsSigned(m_Opcode.rs))
                {
                    Map_GPR_32bit(m_Opcode.rd, true, source1);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(m_Opcode.rt), source1);
                }
                m_Assembler.XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo;

                ConstHi = Is32Bit(ConstReg) ? (uint32_t)(GetMipsRegLo_S(ConstReg) >> 31) : GetMipsRegHi(ConstReg);
                ConstLo = GetMipsRegLo(ConstReg);
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if (ConstHi != 0)
                {
                    m_Assembler.XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), ConstHi);
                }
                if (ConstLo != 0)
                {
                    m_Assembler.XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), ConstLo);
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                if (IsSigned(m_Opcode.rt) != IsSigned(m_Opcode.rs))
                {
                    Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(MappedReg), MappedReg);
                }
                if (Value != 0)
                {
                    m_Assembler.XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        int KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            uint64_t Value;

            if (Is64Bit(KnownReg))
            {
                Value = GetMipsReg(KnownReg);
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (IsSigned(KnownReg))
                {
                    Value = (int)GetMipsRegLo(KnownReg);
                }
                else
                {
                    Value = GetMipsRegLo(KnownReg);
                }
            }
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
            if (dwValue != 0)
            {
                m_Assembler.XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), dwValue);
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                m_Assembler.XorVariableToX86reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                m_Assembler.XorVariableToX86reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                m_Assembler.XorVariableToX86reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.XorVariableToX86reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CX86RecompilerOps::SPECIAL_NOR()
{
    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (IsMapped(m_Opcode.rd))
                UnMap_GPR(m_Opcode.rd, false);

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                           ~((Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)) |
                                             (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs))));
                if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
                }
            }
            else
            {
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, ~(GetMipsRegLo(m_Opcode.rt) | GetMipsRegLo(m_Opcode.rs)));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                if (Is64Bit(source2))
                {
                    m_Assembler.OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else
                {
                    m_Assembler.OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(CX86Ops::x86_Unknown, source2, true, false));
                }
            }
            else
            {
                ProtectGPR(source2);
                Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            m_Assembler.OrX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint64_t Value;

                if (Is64Bit(ConstReg))
                {
                    Value = GetMipsReg(ConstReg);
                }
                else
                {
                    Value = IsSigned(ConstReg) ? (int64_t)GetMipsRegLo_S(ConstReg) : GetMipsRegLo(ConstReg);
                }
                Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0)
                {
                    m_Assembler.OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0)
                {
                    m_Assembler.OrConstToX86Reg(Value, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        int KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (IsConst(KnownReg))
        {
            uint64_t Value = Is64Bit(KnownReg) ? GetMipsReg(KnownReg) : GetMipsRegLo_S(KnownReg);
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);

            if (g_System->b32BitCore() && Is32Bit(KnownReg))
            {
                Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (dwValue != 0)
                {
                    m_Assembler.OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                if (dwValue != 0)
                {
                    m_Assembler.OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                m_Assembler.OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                m_Assembler.OrVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                m_Assembler.OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
    }

    if (IsMapped(m_Opcode.rd))
    {
        if (Is64Bit(m_Opcode.rd))
        {
            m_Assembler.NotX86Reg(GetMipsRegMapHi(m_Opcode.rd));
        }
        m_Assembler.NotX86Reg(GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CX86RecompilerOps::SPECIAL_SLT()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                g_Notify->DisplayError("1");
                CX86RecompilerOps::UnknownOpcode();
            }
            else
            {
                if (IsMapped(m_Opcode.rd))
                {
                    UnMap_GPR(m_Opcode.rd, false);
                }

                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                if (GetMipsRegLo_S(m_Opcode.rs) < GetMipsRegLo_S(m_Opcode.rt))
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 1);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
                }
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if ((Is64Bit(m_Opcode.rt) && Is64Bit(m_Opcode.rs)) ||
                (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))))
            {
                uint8_t * Jump[2];

                m_Assembler.CompX86RegToX86Reg(
                    Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false),
                    Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false));
                m_Assembler.JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                m_Assembler.JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Low Compare:");
                m_Assembler.SetJump8(Jump[0], *g_RecompPos);
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Continue:");
                m_Assembler.SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));

                if (GetMipsRegMapLo(m_Opcode.rd) > CX86Ops::x86_EBX)
                {
                    m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.Setl(GetMipsRegMapLo(m_Opcode.rd));
                    m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
        else
        {
            uint32_t ConstReg = IsConst(m_Opcode.rs) ? m_Opcode.rs : m_Opcode.rt;
            uint32_t MappedReg = IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;

            ProtectGPR(MappedReg);
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint8_t * Jump[2];

                m_Assembler.CompConstToX86reg(
                    Is64Bit(MappedReg) ? GetMipsRegMapHi(MappedReg) : Map_TempReg(CX86Ops::x86_Unknown, MappedReg, true, false),
                    Is64Bit(ConstReg) ? GetMipsRegHi(ConstReg) : (GetMipsRegLo_S(ConstReg) >> 31));
                m_Assembler.JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_Assembler.JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Low Compare:");
                m_Assembler.SetJump8(Jump[0], *g_RecompPos);
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Continue:");
                m_Assembler.SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                uint32_t Constant = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), Constant);

                if (GetMipsRegMapLo(m_Opcode.rd) > CX86Ops::x86_EBX)
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    else
                    {
                        m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        m_Assembler.Setl(GetMipsRegMapLo(m_Opcode.rd));
                    }
                    else
                    {
                        m_Assembler.Setg(GetMipsRegMapLo(m_Opcode.rd));
                    }
                    m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        uint8_t * Jump[2];

        if (!g_System->b32BitCore())
        {
            if (Is64Bit(KnownReg))
            {
                if (IsConst(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegHi(KnownReg));
                }
                else
                {
                    m_Assembler.CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (IsConst(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], (GetMipsRegLo_S(KnownReg) >> 31));
                }
                else
                {
                    ProtectGPR(KnownReg);
                    m_Assembler.CompX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, KnownReg, true, false), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            m_Assembler.JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            }
            m_Assembler.JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;

            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Low Compare:");
            m_Assembler.SetJump8(Jump[0], *g_RecompPos);
            if (IsConst(KnownReg))
            {
                m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegLo(KnownReg));
            }
            else
            {
                m_Assembler.CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Continue:");
            m_Assembler.SetJump8(Jump[1], *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            if (IsMapped(KnownReg))
            {
                ProtectGPR(KnownReg);
            }
            bool bConstant = IsConst(KnownReg);
            uint32_t Value = IsConst(KnownReg) ? GetMipsRegLo(KnownReg) : 0;

            Map_GPR_32bit(m_Opcode.rd, true, -1);
            if (bConstant)
            {
                m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], Value);
            }
            else
            {
                m_Assembler.CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (GetMipsRegMapLo(m_Opcode.rd) > CX86Ops::x86_EBX)
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    m_Assembler.Setg(GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    m_Assembler.Setl(GetMipsRegMapLo(m_Opcode.rd));
                }
                m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (GetMipsRegMapLo(m_Opcode.rd) > CX86Ops::x86_EBX)
        {
            m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_Assembler.Setl(GetMipsRegMapLo(m_Opcode.rd));
            m_Assembler.AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
        }
    }
    else
    {
        uint8_t * Jump[2] = {nullptr, nullptr};

        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
        m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        m_Assembler.JeLabel8("Low Compare", 0);
        Jump[0] = *g_RecompPos - 1;
        m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
        m_Assembler.JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;

        m_CodeBlock.Log("");
        m_CodeBlock.Log("      Low Compare:");
        m_Assembler.SetJump8(Jump[0], *g_RecompPos);
        m_Assembler.CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Continue:");
            m_Assembler.SetJump8(Jump[1], *g_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::SPECIAL_SLTU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsKnown(m_Opcode.rt) && IsKnown(m_Opcode.rs))
    {
        if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                g_Notify->DisplayError("1");
                CX86RecompilerOps::UnknownOpcode();
            }
            else
            {
                if (IsMapped(m_Opcode.rd))
                {
                    UnMap_GPR(m_Opcode.rd, false);
                }

                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                if (GetMipsRegLo(m_Opcode.rs) < GetMipsRegLo(m_Opcode.rt))
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 1);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
                }
            }
        }
        else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs))
        {
            ProtectGPR(m_Opcode.rt);
            ProtectGPR(m_Opcode.rs);
            if ((Is64Bit(m_Opcode.rt) && Is64Bit(m_Opcode.rs)) ||
                (!g_System->b32BitCore() && (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))))
            {
                uint8_t * Jump[2];

                m_Assembler.CompX86RegToX86Reg(
                    Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false),
                    Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false));
                m_Assembler.JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                m_Assembler.JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Low Compare:");
                m_Assembler.SetJump8(Jump[0], *g_RecompPos);
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Continue:");
                m_Assembler.SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
        }
        else
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo, ConstReg, MappedReg;
                CX86Ops::x86Reg MappedRegHi, MappedRegLo;
                uint8_t * Jump[2];

                ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
                MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                ConstLo = GetMipsRegLo_S(ConstReg);
                ConstHi = GetMipsRegLo_S(ConstReg) >> 31;
                if (Is64Bit(ConstReg))
                {
                    ConstHi = GetMipsRegHi(ConstReg);
                }

                ProtectGPR(MappedReg);
                MappedRegLo = GetMipsRegMapLo(MappedReg);
                MappedRegHi = GetMipsRegMapHi(MappedReg);
                if (Is32Bit(MappedReg))
                {
                    MappedRegHi = Map_TempReg(CX86Ops::x86_Unknown, MappedReg, true, false);
                }

                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompConstToX86reg(MappedRegHi, ConstHi);
                m_Assembler.JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_Assembler.JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Low Compare:");
                m_Assembler.SetJump8(Jump[0], *g_RecompPos);
                m_Assembler.CompConstToX86reg(MappedRegLo, ConstLo);
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Continue:");
                m_Assembler.SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                uint32_t Const = IsConst(m_Opcode.rs) ? GetMipsRegLo(m_Opcode.rs) : GetMipsRegLo(m_Opcode.rt);
                uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                m_Assembler.CompConstToX86reg(GetMipsRegMapLo(MappedReg), Const);
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        uint8_t * Jump[2] = {nullptr, nullptr};

        ProtectGPR(KnownReg);
        if (g_System->b32BitCore())
        {
            uint32_t TestReg = IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt;
            if (IsConst(KnownReg))
            {
                uint32_t Value = GetMipsRegLo(KnownReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], Value);
            }
            else
            {
                m_Assembler.CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == TestReg)
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
        }
        else
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegHi(KnownReg));
                }
                else
                {
                    m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], (GetMipsRegLo_S(KnownReg) >> 31));
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    ProtectGPR(KnownReg);
                    m_Assembler.CompX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, KnownReg, true, false), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            m_Assembler.JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;

            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            m_Assembler.JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;

            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Low Compare:");
            m_Assembler.SetJump8(Jump[0], *g_RecompPos);
            if (IsConst(KnownReg))
            {
                m_Assembler.CompConstToVariable(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegLo(KnownReg));
            }
            else
            {
                m_Assembler.CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            if (Jump[1])
            {
                m_CodeBlock.Log("");
                m_CodeBlock.Log("      Continue:");
                m_Assembler.SetJump8(Jump[1], *g_RecompPos);
            }
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
    else if (g_System->b32BitCore())
    {
        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, false, false);
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
    else
    {
        uint8_t * Jump[2] = {nullptr, nullptr};

        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rs, true, false);
        m_Assembler.CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        m_Assembler.JeLabel8("Low Compare", 0);
        Jump[0] = *g_RecompPos - 1;
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        m_Assembler.JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;

        m_CodeBlock.Log("");
        m_CodeBlock.Log("      Low Compare:");
        m_Assembler.SetJump8(Jump[0], *g_RecompPos);
        m_Assembler.CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            m_CodeBlock.Log("");
            m_CodeBlock.Log("      Continue:");
            m_Assembler.SetJump8(Jump[1], *g_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::SPECIAL_DADD()
{
    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        int64_t rs = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs);
        int64_t rt = Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        int64_t sum = rs + rt;
        if ((~(rs ^ rt) & (rs ^ sum)) & 0x8000000000000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, sum);
            if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
            }
        }
    }
    else
    {
        int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        ProtectGPR(source1);
        ProtectGPR(source2);
        CX86Ops::x86Reg RegLo = Map_TempReg(CX86Ops::x86_Unknown, source1, false, false);
        CX86Ops::x86Reg RegHi = Map_TempReg(CX86Ops::x86_Unknown, source1, true, false);

        if (IsConst(source2))
        {
            m_Assembler.AddConstToX86Reg(RegLo, GetMipsRegLo(source2));
            m_Assembler.AdcConstToX86Reg(RegHi, GetMipsRegHi(source2));
        }
        else if (IsMapped(source2))
        {
            CX86Ops::x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(CX86Ops::x86_Unknown, source2, true, false);
            m_Assembler.AddX86RegToX86Reg(RegLo, GetMipsRegMapLo(source2));
            m_Assembler.AdcX86RegToX86Reg(RegHi, HiReg);
        }
        else
        {
            m_Assembler.AddVariableToX86reg(RegLo, &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            m_Assembler.AdcVariableToX86reg(RegHi, &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rd != 0)
        {
            UnProtectGPR(source1);
            UnProtectGPR(source2);
            Map_GPR_64bit(m_Opcode.rd, source1);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), RegLo);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), RegHi);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DADDU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        int64_t ValRs = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs);
        int64_t ValRt = Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        if (IsMapped(m_Opcode.rd))
            UnMap_GPR(m_Opcode.rd, false);

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, ValRs + ValRt);
        if ((GetMipsRegHi(m_Opcode.rd) == 0) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if ((GetMipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (GetMipsRegLo(m_Opcode.rd) & 0x80000000) != 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        if (IsMapped(source2))
        {
            ProtectGPR(source2);
        }
        Map_GPR_64bit(m_Opcode.rd, source1);
        if (IsConst(source2))
        {
            uint32_t LoReg = GetMipsRegLo(source2);
            m_Assembler.AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            if (LoReg != 0)
            {
                m_Assembler.AdcConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
            }
            else
            {
                m_Assembler.AddConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
            }
        }
        else if (IsMapped(source2))
        {
            CX86Ops::x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(CX86Ops::x86_Unknown, source2, true, false);
            m_Assembler.AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            m_Assembler.AdcX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            m_Assembler.AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            m_Assembler.AdcVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSUB()
{
    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        int64_t rs = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs);
        int64_t rt = Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        int64_t sub = rs - rt;

        if (((rs ^ rt) & (rs ^ sub)) & 0x8000000000000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else
        {
            if (IsMapped(m_Opcode.rd))
            {
                UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, sub);
            if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
            }
        }
    }
    else
    {
        int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        ProtectGPR(source1);
        ProtectGPR(source2);
        CX86Ops::x86Reg RegLo = Map_TempReg(CX86Ops::x86_Unknown, source1, false, false);
        CX86Ops::x86Reg RegHi = Map_TempReg(CX86Ops::x86_Unknown, source1, true, false);

        if (IsConst(source2))
        {
            m_Assembler.SubConstFromX86Reg(RegLo, GetMipsRegLo(source2));
            m_Assembler.SbbConstFromX86Reg(RegHi, GetMipsRegHi(source2));
        }
        else if (IsMapped(source2))
        {
            CX86Ops::x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(CX86Ops::x86_Unknown, source2, true, false);
            m_Assembler.SubX86RegToX86Reg(RegLo, GetMipsRegMapLo(source2));
            m_Assembler.SbbX86RegToX86Reg(RegHi, HiReg);
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(RegLo, &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            m_Assembler.SbbVariableFromX86reg(RegHi, &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rd != 0)
        {
            UnProtectGPR(source1);
            UnProtectGPR(source2);
            Map_GPR_64bit(m_Opcode.rd, source1);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), RegLo);
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), RegHi);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSUBU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                   Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) - Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt)
                                                                                                                                                : (int64_t)GetMipsRegLo_S(m_Opcode.rt));
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            CX86Ops::x86Reg HiReg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false);
            CX86Ops::x86Reg LoReg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
            m_Assembler.SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            m_Assembler.SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
            return;
        }
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            m_Assembler.SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
            m_Assembler.SbbConstFromX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            CX86Ops::x86Reg HiReg = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false);
            m_Assembler.SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
            m_Assembler.SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
            m_Assembler.SbbVariableFromX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        int64_t Value = Is64Bit(m_Opcode.rt) ? GetMipsReg_S(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value << m_Opcode.sa);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }

    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.ShiftLeftDoubleImmed(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    m_Assembler.ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_DSRL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }
        int64_t Value = Is64Bit(m_Opcode.rt) ? GetMipsReg_S(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value >> m_Opcode.sa);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.ShiftRightDoubleImmed(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    m_Assembler.ShiftRightUnsignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_DSRA()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rd))
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        int64_t Value = Is64Bit(m_Opcode.rt) ? GetMipsReg_S(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg_S(m_Opcode.rd, Value >> m_Opcode.sa);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }

    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.ShiftRightDoubleImmed(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    m_Assembler.ShiftRightSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_DSLL32()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rd, GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
        if (GetMipsRegLo_S(m_Opcode.rd) < 0 && GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (GetMipsRegLo_S(m_Opcode.rd) >= 0 && GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
        Map_GPR_64bit(m_Opcode.rd, -1);
        if (m_Opcode.rt != m_Opcode.rd)
        {
            m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_CodeBlock.Log("    regcache: switch hi (%s) with lo (%s) for %s", CX86Ops::x86_Name(GetMipsRegMapHi(m_Opcode.rt)), CX86Ops::x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
            CX86Ops::x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
            m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
            m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
        }
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.ShiftLeftSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        m_Assembler.XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rt], CRegName::GPR_Hi[m_Opcode.rt]);
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.ShiftLeftSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        m_Assembler.XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CX86RecompilerOps::SPECIAL_DSRL32()
{
    if (IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, (uint32_t)(GetMipsRegHi(m_Opcode.rt) >> m_Opcode.sa));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
        if (Is64Bit(m_Opcode.rt))
        {
            if (m_Opcode.rt == m_Opcode.rd)
            {
                m_CodeBlock.Log("    regcache: switch hi (%s) with lo (%s) for %s", CX86Ops::x86_Name(GetMipsRegMapHi(m_Opcode.rt)), CX86Ops::x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                CX86Ops::x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                Map_GPR_32bit(m_Opcode.rd, false, -1);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
                m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rt));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                m_Assembler.ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CX86RecompilerOps::UnknownOpcode();
        }
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSRA32()
{
    if (IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)(GetMipsReg_S(m_Opcode.rt) >> (m_Opcode.sa + 32)));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
        if (Is64Bit(m_Opcode.rt))
        {
            if (m_Opcode.rt == m_Opcode.rd)
            {
                m_CodeBlock.Log("    regcache: switch hi (%s) with lo (%s) for %s", CX86Ops::x86_Name(GetMipsRegMapHi(m_Opcode.rt)), CX86Ops::x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                CX86Ops::x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rt));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                m_Assembler.ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CX86RecompilerOps::UnknownOpcode();
        }
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Lo[m_Opcode.rt]);
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

// COP0 functions
void CX86RecompilerOps::COP0_MF()
{
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.PushImm32(m_Opcode.rd);
    m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::Cop0_MF), "CRegisters::Cop0_MF", 8);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
}

void CX86RecompilerOps::COP0_DMF()
{
    Map_GPR_64bit(m_Opcode.rt, -1);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.PushImm32(m_Opcode.rd);
    m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::Cop0_MF), "CRegisters::Cop0_MF", 8);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EAX, &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
    m_Assembler.MoveX86regToVariable(CX86Ops::x86_EDX, &_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapHi(m_Opcode.rt), &_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
}

void CX86RecompilerOps::COP0_MT()
{
    if (m_Opcode.rd == 6 || m_Opcode.rd == 11)
    {
        UpdateCounters(m_RegWorkingSet, false, true);
    }
    m_RegWorkingSet.BeforeCallDirect();
    if (IsConst(m_Opcode.rt))
    {
        m_Assembler.PushImm32(GetMipsRegLo_S(m_Opcode.rt) >> 31);
        m_Assembler.PushImm32(GetMipsRegLo(m_Opcode.rt));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        CX86Ops::x86Reg HiReg = GetMipsRegMapLo(m_Opcode.rt) != CX86Ops::x86_EDX ? CX86Ops::x86_EDX : CX86Ops::x86_EAX;
        m_Assembler.MoveX86RegToX86Reg(HiReg, GetMipsRegMapLo(m_Opcode.rt));
        m_Assembler.ShiftRightSignImmed(HiReg, 0x1F);
        m_Assembler.Push(HiReg);
        m_Assembler.Push(GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EAX, &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_EDX, CX86Ops::x86_EAX);
        m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EDX, 0x1F);
        m_Assembler.Push(CX86Ops::x86_EDX);
        m_Assembler.Push(CX86Ops::x86_EAX);
    }
    m_Assembler.PushImm32(m_Opcode.rd);
    m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::Cop0_MT), "CRegisters::Cop0_MT", 16);
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_DMT()
{
    if (m_Opcode.rd == 6)
    {
        UpdateCounters(m_RegWorkingSet, false, true);
    }
    m_RegWorkingSet.BeforeCallDirect();
    if (IsConst(m_Opcode.rt))
    {
        m_Assembler.PushImm32(Is64Bit(m_Opcode.rt) ? GetMipsRegHi(m_Opcode.rt) : GetMipsRegLo_S(m_Opcode.rt) >> 31);
        m_Assembler.PushImm32(GetMipsRegLo(m_Opcode.rt));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        if (Is64Bit(m_Opcode.rt))
        {
            m_Assembler.Push(GetMipsRegMapHi(m_Opcode.rt));
        }
        else
        {
            CX86Ops::x86Reg HiReg = GetMipsRegMapLo(m_Opcode.rt) != CX86Ops::x86_EDX ? CX86Ops::x86_EDX : CX86Ops::x86_EAX;
            m_Assembler.MoveX86RegToX86Reg(HiReg, GetMipsRegMapLo(m_Opcode.rt));
            m_Assembler.ShiftRightSignImmed(HiReg, 0x1F);
            m_Assembler.Push(HiReg);
        }
        m_Assembler.Push(GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EAX, &_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EDX, &_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
        m_Assembler.Push(CX86Ops::x86_EDX);
        m_Assembler.Push(CX86Ops::x86_EAX);
    }
    m_Assembler.PushImm32(m_Opcode.rd);
    m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::Cop0_MT), "CRegisters::Cop0_MT", 16);
    m_RegWorkingSet.AfterCallDirect();
}

// COP0 CO functions
void CX86RecompilerOps::COP0_CO_TLBR(void)
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallThis((uint32_t)g_TLB, AddressOf(&CTLB::ReadEntry), "CTLB::ReadEntry", 4);
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBWI(void)
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.PushImm32("false", 0);
    m_Assembler.MoveVariableToX86reg(CX86Ops::x86_ECX, &g_Reg->INDEX_REGISTER, "INDEX_REGISTER");
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    m_Assembler.Push(CX86Ops::x86_ECX);
    m_Assembler.CallThis((uint32_t)g_TLB, AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry", 12);
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBWR(void)
{
    UpdateCounters(m_RegWorkingSet, false, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallThis((uint32_t)g_SystemTimer, AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers", 4);
    m_Assembler.PushImm32("true", true);
    m_Assembler.MoveVariableToX86reg(CX86Ops::x86_ECX, &g_Reg->RANDOM_REGISTER, "RANDOM_REGISTER");
    m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX, 0x1F);
    m_Assembler.Push(CX86Ops::x86_ECX);
    m_Assembler.CallThis((uint32_t)g_TLB, AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry", 12);
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBP(void)
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallThis((uint32_t)g_TLB, AddressOf(&CTLB::Probe), "CTLB::TLB_Probe", 4);
    m_RegWorkingSet.AfterCallDirect();
}

void x86_compiler_COP0_CO_ERET()
{
    if ((g_Reg->STATUS_REGISTER & STATUS_ERL) != 0)
    {
        g_Reg->m_PROGRAM_COUNTER = (uint32_t)g_Reg->ERROREPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_ERL;
    }
    else
    {
        g_Reg->m_PROGRAM_COUNTER = (uint32_t)g_Reg->EPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_EXL;
    }
    g_Reg->m_LLBit = 0;
    g_Reg->CheckInterrupts();
}

void CX86RecompilerOps::COP0_CO_ERET(void)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    m_RegWorkingSet.WriteBackRegisters();
    m_Assembler.CallFunc((uint32_t)x86_compiler_COP0_CO_ERET, "x86_compiler_COP0_CO_ERET");

    UpdateCounters(m_RegWorkingSet, true, true);
    CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, ExitReason_Normal, true, nullptr);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

// FPU options
void CX86RecompilerOps::ChangeDefaultRoundingModel()
{
    switch ((_FPCR[31] & 3))
    {
    case 0: *_RoundingModel = FE_TONEAREST; break;
    case 1: *_RoundingModel = FE_TOWARDZERO; break;
    case 2: *_RoundingModel = FE_UPWARD; break;
    case 3: *_RoundingModel = FE_DOWNWARD; break;
    }
}

// COP1 functions
void CX86RecompilerOps::COP1_MF()
{
    CompileCop1Test();

    UnMap_FPR(m_Opcode.fs, true);
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[m_Opcode.fs], stdstr_f("_FPR_S[%d]", m_Opcode.fs).c_str());
    m_Assembler.MoveX86PointerToX86reg(GetMipsRegMapLo(m_Opcode.rt), TempReg);
}

void CX86RecompilerOps::COP1_DMF()
{
    CompileCop1Test();

    UnMap_FPR(m_Opcode.fs, true);
    Map_GPR_64bit(m_Opcode.rt, -1);
    CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[m_Opcode.fs], stdstr_f("_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.AddConstToX86Reg(TempReg, 4);
    m_Assembler.MoveX86PointerToX86reg(GetMipsRegMapHi(m_Opcode.rt), TempReg);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[m_Opcode.fs], stdstr_f("_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.MoveX86PointerToX86reg(GetMipsRegMapLo(m_Opcode.rt), TempReg);
}

void CX86RecompilerOps::COP1_CF()
{
    CompileCop1Test();

    if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
    {
        UnknownOpcode();
        return;
    }

    Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_Assembler.MoveVariableToX86reg(GetMipsRegMapLo(m_Opcode.rt), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
}

void CX86RecompilerOps::COP1_MT()
{
    CompileCop1Test();

    if ((m_Opcode.fs & 1) != 0)
    {
        if (RegInStack(m_Opcode.fs - 1, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs - 1, CRegInfo::FPU_Qword))
        {
            UnMap_FPR(m_Opcode.fs - 1, true);
        }
    }
    UnMap_FPR(m_Opcode.fs, true);
    CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[m_Opcode.fs], stdstr_f("_FPR_S[%d]", m_Opcode.fs).c_str());

    if (IsConst(m_Opcode.rt))
    {
        m_Assembler.MoveConstToX86Pointer(TempReg, GetMipsRegLo(m_Opcode.rt));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        m_Assembler.MoveX86regToX86Pointer(TempReg, GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.MoveX86regToX86Pointer(TempReg, Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false));
    }
}

void CX86RecompilerOps::COP1_DMT()
{
    CompileCop1Test();

    if ((m_Opcode.fs & 1) == 0)
    {
        if (RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Float) || RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Dword))
        {
            UnMap_FPR(m_Opcode.fs + 1, true);
        }
    }
    UnMap_FPR(m_Opcode.fs, true);
    CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[m_Opcode.fs], stdstr_f("_FPR_D[%d]", m_Opcode.fs).c_str());

    if (IsConst(m_Opcode.rt))
    {
        m_Assembler.MoveConstToX86Pointer(TempReg, GetMipsRegLo(m_Opcode.rt));
        m_Assembler.AddConstToX86Reg(TempReg, 4);
        if (Is64Bit(m_Opcode.rt))
        {
            m_Assembler.MoveConstToX86Pointer(TempReg, GetMipsRegHi(m_Opcode.rt));
        }
        else
        {
            m_Assembler.MoveConstToX86Pointer(TempReg, GetMipsRegLo_S(m_Opcode.rt) >> 31);
        }
    }
    else if (IsMapped(m_Opcode.rt))
    {
        m_Assembler.MoveX86regToX86Pointer(TempReg, GetMipsRegMapLo(m_Opcode.rt));
        m_Assembler.AddConstToX86Reg(TempReg, 4);
        if (Is64Bit(m_Opcode.rt))
        {
            m_Assembler.MoveX86regToX86Pointer(TempReg, GetMipsRegMapHi(m_Opcode.rt));
        }
        else
        {
            m_Assembler.MoveX86regToX86Pointer(TempReg, Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, true, false));
        }
    }
    else
    {
        CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false);
        m_Assembler.MoveX86regToX86Pointer(TempReg, Reg);
        m_Assembler.AddConstToX86Reg(TempReg, 4);
        m_Assembler.MoveX86regToX86Pointer(TempReg, Map_TempReg(Reg, m_Opcode.rt, true, false));
    }
}

void CX86RecompilerOps::COP1_CT()
{
    CompileCop1Test();

    if (m_Opcode.fs != 31)
    {
        UnknownOpcode();
        return;
    }

    if (IsConst(m_Opcode.rt))
    {
        m_Assembler.MoveConstToVariable(&_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], GetMipsRegLo(m_Opcode.rt));
    }
    else if (IsMapped(m_Opcode.rt))
    {
        m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    else
    {
        m_Assembler.MoveX86regToVariable(Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.rt, false, false), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallFunc((uint32_t)ChangeDefaultRoundingModel, "ChangeDefaultRoundingModel");
    m_RegWorkingSet.AfterCallDirect();
    m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
}

// COP1: S functions
void CX86RecompilerOps::COP1_S_ADD()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        m_Assembler.fpuAddReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);
        m_Assembler.fpuAddDwordRegPointer(TempReg);
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.fpuSubDwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            m_Assembler.fpuSubReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
            m_Assembler.fpuSubDwordRegPointer(TempReg);
        }
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        m_Assembler.fpuMulReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
        m_Assembler.fpuMulDwordRegPointer(TempReg);
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.fpuDivDwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            m_Assembler.fpuDivReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
            m_Assembler.fpuDivDwordRegPointer(TempReg);
        }
    }

    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_ABS()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    m_Assembler.fpuAbs();
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_NEG()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    m_Assembler.fpuNeg();
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_SQRT()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    m_Assembler.fpuSqrt();
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_MOV()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
}

void CX86RecompilerOps::COP1_S_ROUND_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_S_TRUNC_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_S_CEIL_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_S_FLOOR_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_S_ROUND_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_S_TRUNC_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_S_CEIL_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_S_FLOOR_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_S_CVT_D()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_S_CVT_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_S_CVT_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_S_CMP()
{
    uint32_t Reg1 = m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft;
    uint32_t cmp = 0;

    if ((m_Opcode.funct & 4) == 0)
    {
        Reg1 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float) ? m_Opcode.ft : m_Opcode.fs;
        Reg2 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float) ? m_Opcode.fs : m_Opcode.ft;
    }

    CompileCop1Test();
    if ((m_Opcode.funct & 7) == 0)
    {
        CX86RecompilerOps::UnknownOpcode();
    }
    if ((m_Opcode.funct & 2) != 0)
    {
        cmp |= 0x4000;
    }
    if ((m_Opcode.funct & 4) != 0)
    {
        cmp |= 0x0100;
    }

    Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);
    Map_TempReg(CX86Ops::x86_EAX, 0, false, false);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        m_Assembler.fpuComReg(StackPosition(Reg2), false);
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);

        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
        m_Assembler.fpuComDwordRegPointer(TempReg, false);
    }
    m_Assembler.AndConstToVariable(&_FPCR[31], "_FPCR[31]", (uint32_t)~FPCSR_C);
    m_Assembler.fpuStoreStatus();
    CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, 0, false, true);
    m_Assembler.TestConstToX86Reg(cmp, CX86Ops::x86_EAX);
    m_Assembler.Setnz(Reg);

    if (cmp != 0)
    {
        m_Assembler.TestConstToX86Reg(cmp, CX86Ops::x86_EAX);
        m_Assembler.Setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            CX86Ops::x86Reg _86RegReg2 = Map_TempReg(CX86Ops::x86_Unknown, 0, false, true);
            m_Assembler.AndConstToX86Reg(CX86Ops::x86_EAX, 0x4300);
            m_Assembler.CompConstToX86reg(CX86Ops::x86_EAX, 0x4300);
            m_Assembler.Setz(_86RegReg2);

            m_Assembler.OrX86RegToX86Reg(Reg, _86RegReg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        m_Assembler.AndConstToX86Reg(CX86Ops::x86_EAX, 0x4300);
        m_Assembler.CompConstToX86reg(CX86Ops::x86_EAX, 0x4300);
        m_Assembler.Setz(Reg);
    }
    m_Assembler.ShiftLeftSignImmed(Reg, 23);
    m_Assembler.OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", Reg);
}

// COP1: D functions
void CX86RecompilerOps::COP1_D_ADD()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        m_Assembler.fpuAddReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        m_Assembler.fpuAddQwordRegPointer(TempReg);
    }
}

void CX86RecompilerOps::COP1_D_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        m_Assembler.fpuSubQwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            m_Assembler.fpuSubReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);

            CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            m_Assembler.fpuSubQwordRegPointer(TempReg);
        }
    }
}

void CX86RecompilerOps::COP1_D_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        m_Assembler.fpuMulReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
        m_Assembler.fpuMulQwordRegPointer(TempReg);
    }
}

void CX86RecompilerOps::COP1_D_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        m_Assembler.fpuDivQwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            m_Assembler.fpuDivReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[Reg2], stdstr_f("_FPR_D[%d]").c_str());
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            m_Assembler.fpuDivQwordRegPointer(TempReg);
        }
    }
}

void CX86RecompilerOps::COP1_D_ABS()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    m_Assembler.fpuAbs();
}

void CX86RecompilerOps::COP1_D_NEG()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    m_Assembler.fpuNeg();
}

void CX86RecompilerOps::COP1_D_SQRT()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    m_Assembler.fpuSqrt();
}

void CX86RecompilerOps::COP1_D_MOV()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
}

void CX86RecompilerOps::COP1_D_ROUND_L()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_D_TRUNC_L()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_D_CEIL_L()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_D_FLOOR_L()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_D_ROUND_W()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_D_TRUNC_W()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fd, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fd, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_D_CEIL_W()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_D_FLOOR_W()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_D_CVT_S()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fd, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fd, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_D_CVT_W()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_D_CVT_L()
{
    CompileCop1Test();
    if (RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_D_CMP()
{
    uint32_t Reg1 = m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft;
    uint32_t cmp = 0;

    if ((m_Opcode.funct & 4) == 0)
    {
        Reg1 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) ? m_Opcode.ft : m_Opcode.fs;
        Reg2 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) ? m_Opcode.fs : m_Opcode.ft;
    }

    CompileCop1Test();
    if ((m_Opcode.funct & 7) == 0)
    {
        CX86RecompilerOps::UnknownOpcode();
    }
    if ((m_Opcode.funct & 2) != 0)
    {
        cmp |= 0x4000;
    }
    if ((m_Opcode.funct & 4) != 0)
    {
        cmp |= 0x0100;
    }

    Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
    Map_TempReg(CX86Ops::x86_EAX, 0, false, false);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        m_Assembler.fpuComReg(StackPosition(Reg2), false);
    }
    else
    {
        UnMap_FPR(Reg2, true);
        CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
        Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
        m_Assembler.fpuComQwordRegPointer(TempReg, false);
    }
    m_Assembler.AndConstToVariable(&_FPCR[31], "_FPCR[31]", (uint32_t)~FPCSR_C);
    m_Assembler.fpuStoreStatus();
    CX86Ops::x86Reg Reg = Map_TempReg(CX86Ops::x86_Unknown, 0, false, true);
    m_Assembler.TestConstToX86Reg(cmp, CX86Ops::x86_EAX);
    m_Assembler.Setnz(Reg);
    if (cmp != 0)
    {
        m_Assembler.TestConstToX86Reg(cmp, CX86Ops::x86_EAX);
        m_Assembler.Setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            CX86Ops::x86Reg _86RegReg2 = Map_TempReg(CX86Ops::x86_Unknown, 0, false, true);
            m_Assembler.AndConstToX86Reg(CX86Ops::x86_EAX, 0x4300);
            m_Assembler.CompConstToX86reg(CX86Ops::x86_EAX, 0x4300);
            m_Assembler.Setz(_86RegReg2);

            m_Assembler.OrX86RegToX86Reg(Reg, _86RegReg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        m_Assembler.AndConstToX86Reg(CX86Ops::x86_EAX, 0x4300);
        m_Assembler.CompConstToX86reg(CX86Ops::x86_EAX, 0x4300);
        m_Assembler.Setz(Reg);
    }
    m_Assembler.ShiftLeftSignImmed(Reg, 23);
    m_Assembler.OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", Reg);
}

// COP1: W functions
void CX86RecompilerOps::COP1_W_CVT_S()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Dword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Dword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Dword, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_W_CVT_D()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Dword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Dword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Dword, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

// COP1: L functions
void CX86RecompilerOps::COP1_L_CVT_S()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Qword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Qword, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_L_CVT_D()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Qword);
    }
    ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Qword, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

// Other functions
void CX86RecompilerOps::UnknownOpcode()
{
    m_CodeBlock.Log("  %X Unhandled opcode: %s", m_CompilePC, R4300iInstruction(m_CompilePC, m_Opcode.Value).NameAndParam().c_str());

    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());

    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
    m_Assembler.Ret();
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
    }
}

void CX86RecompilerOps::ClearCachedInstructionInfo()
{
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
}

void CX86RecompilerOps::FoundMemoryBreakpoint()
{
    ClearCachedInstructionInfo();
    m_Assembler.MoveConstToVariable(&memory_write_in_delayslot, "memory_write_in_delayslot", (m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT) ? 1 : 0);
    m_Assembler.CallFunc((uint32_t)x86MemoryBreakpoint, "x86MemoryBreakpoint");
    m_Assembler.MoveConstToVariable(&memory_breakpoint_found, "memory_breakpoint_found", 0);
    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::PreReadInstruction()
{
    if (!HaveReadBP())
    {
        return;
    }
    ClearCachedInstructionInfo();
}

void CX86RecompilerOps::PreWriteInstruction()
{
    if (!HaveWriteBP())
    {
        return;
    }
    ClearCachedInstructionInfo();
}

void CX86RecompilerOps::TestBreakpoint(CX86Ops::x86Reg AddressReg, uint32_t FunctAddress, const char * FunctName)
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveX86regToVariable(AddressReg, &memory_access_address, "memory_access_address");
    m_Assembler.MoveConstToVariable(&memory_write_in_delayslot, "memory_write_in_delayslot", (m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT) ? 1 : 0);
    m_Assembler.CallFunc(FunctAddress, FunctName);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.CompConstToVariable(&memory_breakpoint_found, "memory_breakpoint_found", 0);
    m_Assembler.JeLabel8("NoBreakPoint", 0);
    uint8_t * Jump = *g_RecompPos - 1;
    m_Assembler.MoveConstToVariable(&memory_breakpoint_found, "memory_breakpoint_found", 0);
    ExitCodeBlock();
    m_CodeBlock.Log("      ");
    m_CodeBlock.Log("      NoBreakPoint:");
    m_Assembler.SetJump8(Jump, *g_RecompPos);
}

void CX86RecompilerOps::TestWriteBreakpoint(CX86Ops::x86Reg AddressReg, uint32_t FunctAddress, const char * FunctName)
{
    if (!HaveWriteBP())
    {
        return;
    }
    TestBreakpoint(AddressReg, FunctAddress, FunctName);
}

void CX86RecompilerOps::TestReadBreakpoint(CX86Ops::x86Reg AddressReg, uint32_t FunctAddress, const char * FunctName)
{
    if (!HaveReadBP())
    {
        return;
    }
    TestBreakpoint(AddressReg, FunctAddress, FunctName);
}

void CX86RecompilerOps::EnterCodeBlock()
{
#ifdef _DEBUG
    m_Assembler.Push(CX86Ops::x86_ESI);
#else
    m_Assembler.Push(CX86Ops::x86_EDI);
    m_Assembler.Push(CX86Ops::x86_ESI);
    m_Assembler.Push(CX86Ops::x86_EBX);
#endif
}

void CX86RecompilerOps::ExitCodeBlock()
{
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
#ifdef _DEBUG
    m_Assembler.Pop(CX86Ops::x86_ESI);
#else
    m_Assembler.Pop(CX86Ops::x86_EBX);
    m_Assembler.Pop(CX86Ops::x86_ESI);
    m_Assembler.Pop(CX86Ops::x86_EDI);
#endif
    m_Assembler.Ret();
}

void CX86RecompilerOps::CompileExitCode()
{
    for (EXIT_LIST::iterator ExitIter = m_ExitInfo.begin(); ExitIter != m_ExitInfo.end(); ExitIter++)
    {
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      $%s", ExitIter->Name.c_str());
        m_Assembler.SetJump32(ExitIter->JumpLoc, (uint32_t *)*g_RecompPos);
        m_PipelineStage = ExitIter->PipelineStage;
        CompileExit((uint32_t)-1, ExitIter->TargetPC, ExitIter->ExitRegSet, ExitIter->Reason, true, nullptr);
    }
}

void CX86RecompilerOps::CompileCop1Test()
{
    if (m_RegWorkingSet.GetFpuBeenUsed())
    {
        return;
    }

    m_Assembler.TestVariable(STATUS_CU1, &g_Reg->STATUS_REGISTER, "STATUS_REGISTER");
    CRegInfo ExitRegSet = m_RegWorkingSet;
    ExitRegSet.SetBlockCycleCount(ExitRegSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_COP1Unuseable, false, &CX86Ops::JeLabel32);
    m_RegWorkingSet.SetFpuBeenUsed(true);
}

void CX86RecompilerOps::CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter)
{
    m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", ProgramCounter);
    RegSet.WriteBackRegisters();
    UpdateCounters(RegSet, false, true, false);
    m_Assembler.CallFunc(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
    m_Assembler.CallThis((uint32_t)g_SystemTimer, AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone", 4);
    m_CodeBlock.Log("CompileSystemCheck 3");
    CompileSystemCheck((uint32_t)-1, RegSet);
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
}

bool CX86RecompilerOps::SetupRegisterForLoop(CCodeBlock & BlockInfo, const CRegInfo & RegSet)
{
    CRegInfo OriginalReg = m_RegWorkingSet;
    if (!LoopAnalysis(BlockInfo, m_Section).SetupRegisterForLoop())
    {
        return false;
    }
    for (int i = 1; i < 32; i++)
    {
        if (OriginalReg.GetMipsRegState(i) != RegSet.GetMipsRegState(i))
        {
            UnMap_GPR(i, true);
        }
    }
    return true;
}

void CX86RecompilerOps::SyncRegState(const CRegInfo & SyncTo)
{
    ResetX86Protection();

    bool changed = false;
    UnMap_AllFPRs();
    if (m_RegWorkingSet.GetRoundingModel() != SyncTo.GetRoundingModel())
    {
        m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
    }
    CX86Ops::x86Reg MemStackReg = Get_MemoryStack();
    CX86Ops::x86Reg TargetStackReg = SyncTo.Get_MemoryStack();

    //m_CodeBlock.Log("MemoryStack for Original State = %s",MemStackReg > 0?CX86Ops::x86_Name(MemStackReg):"Not Mapped");
    if (MemStackReg != TargetStackReg)
    {
        if (TargetStackReg == CX86Ops::x86_Unknown)
        {
            UnMap_X86reg(MemStackReg);
        }
        else if (MemStackReg == CX86Ops::x86_Unknown)
        {
            UnMap_X86reg(TargetStackReg);
            m_CodeBlock.Log("    regcache: allocate %s as memory stack", CX86Ops::x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(TargetStackReg), CRegInfo::Stack_Mapped);
            m_Assembler.MoveVariableToX86reg(TargetStackReg, &g_Recompiler->MemoryStackPos(), "MemoryStack");
        }
        else
        {
            UnMap_X86reg(TargetStackReg);
            m_CodeBlock.Log("    regcache: change allocation of memory stack from %s to %s", CX86Ops::x86_Name(MemStackReg), CX86Ops::x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(TargetStackReg), CRegInfo::Stack_Mapped);
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(MemStackReg), CRegInfo::NotMapped);
            m_Assembler.MoveX86RegToX86Reg(TargetStackReg, MemStackReg);
        }
    }

    for (int i = 1; i < 32; i++)
    {
        if (GetMipsRegState(i) == SyncTo.GetMipsRegState(i) ||
            (g_System->b32BitCore() && GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN) ||
            (g_System->b32BitCore() && GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO))
        {
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN: continue;
            case CRegInfo::STATE_MAPPED_64:
                if (GetMipsRegMapHi(i) == SyncTo.GetMipsRegMapHi(i) &&
                    GetMipsRegMapLo(i) == SyncTo.GetMipsRegMapLo(i))
                {
                    continue;
                }
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (GetMipsRegMapLo(i) == SyncTo.GetMipsRegMapLo(i))
                {
                    continue;
                }
                break;
            case CRegInfo::STATE_CONST_64:
                if (GetMipsReg(i) != SyncTo.GetMipsReg(i))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (GetMipsRegLo(i) != SyncTo.GetMipsRegLo(i))
                {
                    m_CodeBlock.Log("Value of constant is different register %d (%s) Value: 0x%08X to 0x%08X", i, CRegName::GPR[i], GetMipsRegLo(i), SyncTo.GetMipsRegLo(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            default:
                m_CodeBlock.Log("Unhandled register state %d\nin SyncRegState", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        changed = true;

        switch (SyncTo.GetMipsRegState(i))
        {
        case CRegInfo::STATE_UNKNOWN: UnMap_GPR(i, true); break;
        case CRegInfo::STATE_MAPPED_64:
        {
            CX86Ops::x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
            CX86Ops::x86Reg x86RegHi = SyncTo.GetMipsRegMapHi(i);
            UnMap_X86reg(Reg);
            UnMap_X86reg(x86RegHi);
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN:
                m_Assembler.MoveVariableToX86reg(Reg, &_GPR[i].UW[0], CRegName::GPR_Lo[i]);
                m_Assembler.MoveVariableToX86reg(x86RegHi, &_GPR[i].UW[1], CRegName::GPR_Hi[i]);
                break;
            case CRegInfo::STATE_MAPPED_64:
                m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                m_Assembler.MoveX86RegToX86Reg(x86RegHi, GetMipsRegMapHi(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapHi(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                m_Assembler.MoveX86RegToX86Reg(x86RegHi, GetMipsRegMapLo(i));
                m_Assembler.ShiftRightSignImmed(x86RegHi, 31);
                m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                m_Assembler.XorX86RegToX86Reg(x86RegHi, x86RegHi);
                m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_CONST_64:
                m_Assembler.MoveConstToX86reg(x86RegHi, GetMipsRegHi(i));
                m_Assembler.MoveConstToX86reg(Reg, GetMipsRegLo(i));
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                m_Assembler.MoveConstToX86reg(x86RegHi, GetMipsRegLo_S(i) >> 31);
                m_Assembler.MoveConstToX86reg(Reg, GetMipsRegLo(i));
                break;
            default:
                m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
                continue;
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegMapHi(i, x86RegHi);
            m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_64);
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(Reg), CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(x86RegHi), CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetX86MapOrder(GetIndexFromX86Reg(Reg), 1);
            m_RegWorkingSet.SetX86MapOrder(GetIndexFromX86Reg(x86RegHi), 1);
            break;
        }
        case CRegInfo::STATE_MAPPED_32_SIGN:
        {
            CX86Ops::x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
            UnMap_X86reg(Reg);
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN: m_Assembler.MoveVariableToX86reg(Reg, &_GPR[i].UW[0], CRegName::GPR_Lo[i]); break;
            case CRegInfo::STATE_CONST_32_SIGN: m_Assembler.MoveConstToX86reg(Reg, GetMipsRegLo(i)); break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                if (GetMipsRegMapLo(i) != Reg)
                {
                    m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                    m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                }
                break;
            case CRegInfo::STATE_MAPPED_64:
                m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapHi(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_CONST_64:
                m_CodeBlock.Log("hi %X\nLo %X", GetMipsRegHi(i), GetMipsRegLo(i));
            default:
                m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_32_SIGN);
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(Reg), CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetX86MapOrder(GetIndexFromX86Reg(Reg), 1);
            break;
        }
        case CRegInfo::STATE_MAPPED_32_ZERO:
        {
            CX86Ops::x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
            UnMap_X86reg(Reg);
            switch (GetMipsRegState(i))
            {
            case CRegInfo::STATE_MAPPED_64:
            case CRegInfo::STATE_UNKNOWN:
                m_Assembler.MoveVariableToX86reg(Reg, &_GPR[i].UW[0], CRegName::GPR_Lo[i]);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (g_System->b32BitCore())
                {
                    m_Assembler.MoveX86RegToX86Reg(Reg, GetMipsRegMapLo(i));
                    m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                }
                else
                {
                    m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (!g_System->b32BitCore() && GetMipsRegLo_S(i) < 0)
                {
                    m_CodeBlock.Log("Sign problems in SyncRegState\nSTATE_MAPPED_32_ZERO");
                    m_CodeBlock.Log("%s: %X", CRegName::GPR[i], GetMipsRegLo_S(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                m_Assembler.MoveConstToX86reg(Reg, GetMipsRegLo(i));
                break;
            default:
                m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegState(i, SyncTo.GetMipsRegState(i));
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(Reg), CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetX86MapOrder(GetIndexFromX86Reg(Reg), 1);
            break;
        }
        default:
            m_CodeBlock.Log("%d - %d reg: %s (%d)", SyncTo.GetMipsRegState(i), GetMipsRegState(i), CRegName::GPR[i], i);
            g_Notify->BreakPoint(__FILE__, __LINE__);
            changed = false;
        }
    }
}

CRegInfo & CX86RecompilerOps::GetRegWorkingSet(void)
{
    return m_RegWorkingSet;
}

void CX86RecompilerOps::SetRegWorkingSet(const CRegInfo & RegInfo)
{
    m_RegWorkingSet = RegInfo;
}

bool CX86RecompilerOps::InheritParentInfo()
{
    if (m_Section->m_CompiledLocation == nullptr)
    {
        m_Section->m_CompiledLocation = *g_RecompPos;
        m_Section->DisplaySectionInformation();
        m_Section->m_CompiledLocation = nullptr;
    }
    else
    {
        m_Section->DisplaySectionInformation();
    }

    if (m_Section->m_ParentSection.empty())
    {
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    if (m_Section->m_ParentSection.size() == 1)
    {
        CCodeSection * Parent = *(m_Section->m_ParentSection.begin());
        if (Parent->m_CompiledLocation == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        CJumpInfo * JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;

        m_Section->m_RegEnter = JumpInfo->RegSet;
        LinkJump(*JumpInfo, m_Section->m_SectionID);
        SetRegWorkingSet(m_Section->m_RegEnter);
        return true;
    }

    // Multiple parents
    BLOCK_PARENT_LIST ParentList;
    CCodeSection::SECTION_LIST::iterator iter;
    for (iter = m_Section->m_ParentSection.begin(); iter != m_Section->m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (Parent->m_CompiledLocation == nullptr)
        {
            continue;
        }
        if (Parent->m_JumpSection != Parent->m_ContinueSection)
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
        else
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Cont;
            ParentList.push_back(BlockParent);
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
    }
    size_t NoOfCompiledParents = ParentList.size();
    if (NoOfCompiledParents == 0)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return false;
    }

    // Add all the uncompiled blocks to the end of the list
    for (iter = m_Section->m_ParentSection.begin(); iter != m_Section->m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (Parent->m_CompiledLocation != nullptr)
        {
            continue;
        }
        if (Parent->m_JumpSection != Parent->m_ContinueSection)
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = m_Section == Parent->m_ContinueSection ? &Parent->m_Cont : &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
        else
        {
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Cont;
            ParentList.push_back(BlockParent);
            BlockParent.Parent = Parent;
            BlockParent.JumpInfo = &Parent->m_Jump;
            ParentList.push_back(BlockParent);
        }
    }
    int FirstParent = -1;
    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        if (!ParentList[i].JumpInfo->FallThrough)
        {
            continue;
        }
        if (FirstParent != -1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        FirstParent = i;
    }
    if (FirstParent == -1)
    {
        FirstParent = 0;
    }

    // Link first parent to start
    CCodeSection * Parent = ParentList[FirstParent].Parent;
    CJumpInfo * JumpInfo = ParentList[FirstParent].JumpInfo;

    SetRegWorkingSet(JumpInfo->RegSet);
    m_RegWorkingSet.ResetX86Protection();
    LinkJump(*JumpInfo, m_Section->m_SectionID, Parent->m_SectionID);

    if (JumpInfo->Reason == ExitReason_NormalNoSysCheck)
    {
        if (JumpInfo->RegSet.GetBlockCycleCount() != 0)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (JumpInfo->JumpPC != (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        UpdateCounters(JumpInfo->RegSet, m_Section->m_EnterPC < JumpInfo->JumpPC, true);
        if (JumpInfo->JumpPC == (uint32_t)-1)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (m_Section->m_EnterPC <= JumpInfo->JumpPC)
        {
            m_CodeBlock.Log("CompileSystemCheck 10");
            CompileSystemCheck(m_Section->m_EnterPC, GetRegWorkingSet());
        }
    }
    JumpInfo->FallThrough = false;

    // Fix up initial state
    UnMap_AllFPRs();

    // Determine loop registry usage
    if (m_Section->m_InLoop && ParentList.size() > 1)
    {
        if (!SetupRegisterForLoop(m_Section->m_CodeBlock, m_Section->m_RegEnter))
        {
            return false;
        }
        m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
    }

    for (size_t i = 0; i < ParentList.size(); i++)
    {
        CX86Ops::x86Reg MemoryStackPos;
        int i2;

        if (i == (size_t)FirstParent)
        {
            continue;
        }
        Parent = ParentList[i].Parent;
        if (Parent->m_CompiledLocation == nullptr)
        {
            continue;
        }
        CRegInfo * RegSet = &ParentList[i].JumpInfo->RegSet;

        if (m_RegWorkingSet.GetRoundingModel() != RegSet->GetRoundingModel())
        {
            m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
        }

        // Find parent MapRegState
        MemoryStackPos = CX86Ops::x86_Unknown;
        for (i2 = 0; i2 < x86RegIndex_Size; i2++)
        {
            if (RegSet->GetX86Mapped((x86RegIndex)i2) == CRegInfo::Stack_Mapped)
            {
                MemoryStackPos = GetX86RegFromIndex((x86RegIndex)i2);
                break;
            }
        }
        if (MemoryStackPos == CX86Ops::x86_Unknown)
        {
            // If the memory stack position is not mapped then unmap it
            CX86Ops::x86Reg MemStackReg = Get_MemoryStack();
            if (MemStackReg != CX86Ops::x86_Unknown)
            {
                UnMap_X86reg(MemStackReg);
            }
        }

        for (i2 = 1; i2 < 32; i2++)
        {
            if (Is32BitMapped(i2))
            {
                switch (RegSet->GetMipsRegState(i2))
                {
                case CRegInfo::STATE_MAPPED_64: Map_GPR_64bit(i2, i2); break;
                case CRegInfo::STATE_MAPPED_32_ZERO: break;
                case CRegInfo::STATE_MAPPED_32_SIGN:
                    if (IsUnsigned(i2))
                    {
                        m_RegWorkingSet.SetMipsRegState(i2, CRegInfo::STATE_MAPPED_32_SIGN);
                    }
                    break;
                case CRegInfo::STATE_CONST_64: Map_GPR_64bit(i2, i2); break;
                case CRegInfo::STATE_CONST_32_SIGN:
                    if ((RegSet->GetMipsRegLo_S(i2) < 0) && IsUnsigned(i2))
                    {
                        m_RegWorkingSet.SetMipsRegState(i2, CRegInfo::STATE_MAPPED_32_SIGN);
                    }
                    break;
                case CRegInfo::STATE_UNKNOWN:
                    if (g_System->b32BitCore())
                    {
                        Map_GPR_32bit(i2, true, i2);
                    }
                    else
                    {
                        //Map_GPR_32bit(i2,true,i2);
                        Map_GPR_64bit(i2, i2); //??
                        //UnMap_GPR(Section,i2,true); ??
                    }
                    break;
                default:
                    m_CodeBlock.Log("Unknown CPU state(%d) in InheritParentInfo", GetMipsRegState(i2));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            if (IsConst(i2))
            {
                if (GetMipsRegState(i2) != RegSet->GetMipsRegState(i2))
                {
                    switch (RegSet->GetMipsRegState(i2))
                    {
                    case CRegInfo::STATE_CONST_64:
                    case CRegInfo::STATE_MAPPED_64:
                        Map_GPR_64bit(i2, i2);
                        break;
                    case CRegInfo::STATE_MAPPED_32_ZERO:
                        if (Is32Bit(i2))
                        {
                            Map_GPR_32bit(i2, (GetMipsRegLo(i2) & 0x80000000) != 0, i2);
                        }
                        else
                        {
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        break;
                    case CRegInfo::STATE_MAPPED_32_SIGN:
                        if (Is32Bit(i2))
                        {
                            Map_GPR_32bit(i2, true, i2);
                        }
                        else
                        {
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        break;
                    case CRegInfo::STATE_UNKNOWN:
                        if (g_System->b32BitCore())
                        {
                            Map_GPR_32bit(i2, true, i2);
                        }
                        else
                        {
                            Map_GPR_64bit(i2, i2);
                        }
                        break;
                    default:
                        m_CodeBlock.Log("Unknown CPU state(%d) in InheritParentInfo", RegSet->GetMipsRegState(i2));
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                        break;
                    }
                }
                else if (Is32Bit(i2) && GetMipsRegLo(i2) != RegSet->GetMipsRegLo(i2))
                {
                    Map_GPR_32bit(i2, true, i2);
                }
                else if (Is64Bit(i2) && GetMipsReg(i2) != RegSet->GetMipsReg(i2))
                {
                    Map_GPR_32bit(i2, true, i2);
                }
            }
            ResetX86Protection();
        }

        if (MemoryStackPos > 0)
        {
            Map_MemoryStack(MemoryStackPos, true);
        }
    }
    m_Section->m_RegEnter = m_RegWorkingSet;

    // Sync registers for different blocks
    stdstr_f Label("Section_%d", m_Section->m_SectionID);
    int CurrentParent = FirstParent;
    bool NeedSync = false;
    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        CRegInfo * RegSet;
        int i2;

        if (i == (size_t)FirstParent)
        {
            continue;
        }
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;
        RegSet = &ParentList[i].JumpInfo->RegSet;

        if (JumpInfo->RegSet.GetBlockCycleCount() != 0)
        {
            NeedSync = true;
        }

        for (i2 = 0; !NeedSync && i2 < 8; i2++)
        {
            if (m_RegWorkingSet.FpuMappedTo(i2) == (uint32_t)-1)
            {
                NeedSync = true;
            }
        }

        for (i2 = 0; !NeedSync && i2 < x86RegIndex_Size; i2++)
        {
            if (m_RegWorkingSet.GetX86Mapped((x86RegIndex)i2) == CRegInfo::Stack_Mapped)
            {
                if (m_RegWorkingSet.GetX86Mapped((x86RegIndex)i2) != RegSet->GetX86Mapped((x86RegIndex)i2))
                {
                    NeedSync = true;
                }
                break;
            }
        }
        for (i2 = 0; !NeedSync && i2 < 32; i2++)
        {
            if (NeedSync == true)
            {
                break;
            }
            if (m_RegWorkingSet.GetMipsRegState(i2) != RegSet->GetMipsRegState(i2))
            {
                NeedSync = true;
                continue;
            }
            switch (m_RegWorkingSet.GetMipsRegState(i2))
            {
            case CRegInfo::STATE_UNKNOWN: break;
            case CRegInfo::STATE_MAPPED_64:
                if (GetMipsRegMapHi(i2) != RegSet->GetMipsRegMapHi(i2) ||
                    GetMipsRegMapLo(i2) != RegSet->GetMipsRegMapLo(i2))
                {
                    NeedSync = true;
                }
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (GetMipsRegMapLo(i2) != RegSet->GetMipsRegMapLo(i2))
                {
                    //DisplayError(L"Parent: %d",Parent->SectionID);
                    NeedSync = true;
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (GetMipsRegLo(i2) != RegSet->GetMipsRegLo(i2))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    NeedSync = true;
                }
                break;
            default:
                WriteTrace(TraceRecompiler, TraceError, "Unhandled register state %d\nin InheritParentInfo", GetMipsRegState(i2));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        if (NeedSync == false)
        {
            continue;
        }
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        m_Assembler.JmpLabel32(Label.c_str(), 0);
        JumpInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        JumpInfo->LinkLocation2 = nullptr;

        CurrentParent = i;
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        m_CodeBlock.Log("   Section_%d (from %d):", m_Section->m_SectionID, Parent->m_SectionID);
        if (JumpInfo->LinkLocation != nullptr)
        {
            m_Assembler.SetJump32(JumpInfo->LinkLocation, (uint32_t *)*g_RecompPos);
            JumpInfo->LinkLocation = nullptr;
            if (JumpInfo->LinkLocation2 != nullptr)
            {
                m_Assembler.SetJump32(JumpInfo->LinkLocation2, (uint32_t *)*g_RecompPos);
                JumpInfo->LinkLocation2 = nullptr;
            }
        }

        m_RegWorkingSet = JumpInfo->RegSet;
        if (m_Section->m_EnterPC < JumpInfo->JumpPC)
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            m_CodeBlock.Log("CompileSystemCheck 11");
            CompileSystemCheck(m_Section->m_EnterPC, m_RegWorkingSet);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, false, true);
        }
        SyncRegState(m_Section->m_RegEnter); // Sync
        m_Section->m_RegEnter = m_RegWorkingSet;
    }

    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;
        LinkJump(*JumpInfo);
    }

    m_CodeBlock.Log("   Section_%d:", m_Section->m_SectionID);
    m_Section->m_RegEnter.SetBlockCycleCount(0);
    return true;
}

void CX86RecompilerOps::LinkJump(CJumpInfo & JumpInfo, uint32_t SectionID, uint32_t FromSectionID)
{
    if (JumpInfo.LinkLocation != nullptr)
    {
        if (SectionID != -1)
        {
            if (FromSectionID != -1)
            {
                m_CodeBlock.Log("   Section_%d (from %d):", SectionID, FromSectionID);
            }
            else
            {
                m_CodeBlock.Log("   Section_%d:", SectionID);
            }
        }
        m_Assembler.SetJump32(JumpInfo.LinkLocation, (uint32_t *)*g_RecompPos);
        JumpInfo.LinkLocation = nullptr;
        if (JumpInfo.LinkLocation2 != nullptr)
        {
            m_Assembler.SetJump32(JumpInfo.LinkLocation2, (uint32_t *)*g_RecompPos);
            JumpInfo.LinkLocation2 = nullptr;
        }
    }
}

void CX86RecompilerOps::JumpToSection(CCodeSection * Section)
{
    char Label[100];
    sprintf(Label, "Section_%d", Section->m_SectionID);
    m_Assembler.JmpLabel32(Label, 0);
    m_Assembler.SetJump32(((uint32_t *)*g_RecompPos) - 1, (uint32_t *)(Section->m_CompiledLocation));
}

void CX86RecompilerOps::JumpToUnknown(CJumpInfo * JumpInfo)
{
    m_Assembler.JmpLabel32(JumpInfo->BranchLabel.c_str(), 0);
    JumpInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);
}

void CX86RecompilerOps::SetCurrentPC(uint32_t ProgramCounter)
{
    m_CompilePC = ProgramCounter;
    __except_try()
    {
        if (!g_MMU->MemoryValue32(m_CompilePC, m_Opcode.Value))
        {
            g_Notify->FatalError(GS(MSG_FAIL_LOAD_WORD));
        }
    }
    __except_catch()
    {
        g_Notify->FatalError(GS(MSG_UNKNOWN_MEM_ACTION));
    }
}

uint32_t CX86RecompilerOps::GetCurrentPC(void)
{
    return m_CompilePC;
}

void CX86RecompilerOps::SetCurrentSection(CCodeSection * section)
{
    m_Section = section;
}

void CX86RecompilerOps::SetNextStepType(PIPELINE_STAGE StepType)
{
    m_PipelineStage = StepType;
}

PIPELINE_STAGE CX86RecompilerOps::GetNextStepType(void)
{
    return m_PipelineStage;
}

const R4300iOpcode & CX86RecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
}

void CX86RecompilerOps::UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles)
{
    if (!g_SyncSystem)
    {
        return;
    }

    m_Assembler.WriteX86Comment("Updating sync CPU");
    RegSet.BeforeCallDirect();
    m_Assembler.PushImm32(stdstr_f("%d", Cycles).c_str(), Cycles);
    m_Assembler.PushImm32("g_SyncSystem", (uint32_t)g_SyncSystem);
    m_Assembler.CallThis((uint32_t)g_System, AddressOf(&CN64System::UpdateSyncCPU), "CN64System::UpdateSyncCPU", 12);
    RegSet.AfterCallDirect();
}

void CX86RecompilerOps::UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues, bool UpdateTimer)
{
    if (RegSet.GetBlockCycleCount() != 0)
    {
        UpdateSyncCPU(RegSet, RegSet.GetBlockCycleCount());
        m_Assembler.WriteX86Comment("Update counter");
        m_Assembler.SubConstFromVariable(RegSet.GetBlockCycleCount(), g_NextTimer, "g_NextTimer"); // Updates compare flag
        if (ClearValues)
        {
            RegSet.SetBlockCycleCount(0);
        }
    }
    else if (CheckTimer)
    {
        m_Assembler.CompConstToVariable(g_NextTimer, "g_NextTimer", 0);
    }

    if (CheckTimer)
    {
        m_Assembler.JnsLabel8("Continue_From_Timer_Test", 0);
        uint8_t * Jump = *g_RecompPos - 1;
        RegSet.BeforeCallDirect();
        m_Assembler.CallThis((uint32_t)g_SystemTimer, AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone", 4);
        RegSet.AfterCallDirect();

        m_CodeBlock.Log("");
        m_CodeBlock.Log("      $Continue_From_Timer_Test:");
        m_Assembler.SetJump8(Jump, *g_RecompPos);
    }

    if ((UpdateTimer || CGameSettings::OverClockModifier() != 1) && g_SyncSystem)
    {
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.CallThis((uint32_t)g_SystemTimer, AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers", 4);
        m_RegWorkingSet.AfterCallDirect();
    }
}

void CX86RecompilerOps::CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet)
{
    m_Assembler.CompConstToVariable((void *)&g_SystemEvents->DoSomething(), "g_SystemEvents->DoSomething()", 0);
    m_Assembler.JeLabel32("Continue_From_Interrupt_Test", 0);
    uint32_t * Jump = (uint32_t *)(*g_RecompPos - 4);
    if (TargetPC != (uint32_t)-1)
    {
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", TargetPC);
    }

    CRegInfo RegSetCopy(RegSet);
    RegSetCopy.WriteBackRegisters();
    m_Assembler.CallThis((uint32_t)g_SystemEvents, AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents", 4);
    ExitCodeBlock();
    m_CodeBlock.Log("");
    m_CodeBlock.Log("      $Continue_From_Interrupt_Test:");
    m_Assembler.SetJump32(Jump, (uint32_t *)*g_RecompPos);
}

void CX86RecompilerOps::CompileExecuteBP(void)
{
    bool bDelay = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
    if (bDelay)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_RegWorkingSet.WriteBackRegisters();

    UpdateCounters(m_RegWorkingSet, true, true);
    m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", CompilePC());
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
    m_Assembler.CallFunc((uint32_t)x86_compiler_Break_Point, "x86_compiler_Break_Point");
    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::CompileExecuteDelaySlotBP(void)
{
    bool bDelay = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
    if (bDelay)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    m_RegWorkingSet.WriteBackRegisters();

    UpdateCounters(m_RegWorkingSet, true, true);
    m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", CompilePC());
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
    m_Assembler.CallFunc((uint32_t)x86_Break_Point_DelaySlot, "x86_Break_Point_DelaySlot");
    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::OverflowDelaySlot(bool TestTimer)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    m_Assembler.MoveConstToVariable(_PROGRAM_COUNTER, "PROGRAM_COUNTER", CompilePC() + 4);

    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }

    m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", PIPELINE_STAGE_JUMP);

    if (TestTimer)
    {
        m_Assembler.MoveConstToVariable(&R4300iOp::m_TestTimer, "R4300iOp::m_TestTimer", TestTimer);
    }

    m_Assembler.PushImm32("g_System->CountPerOp()", g_System->CountPerOp());
    m_Assembler.CallFunc((uint32_t)CInterpreterCPU::ExecuteOps, "CInterpreterCPU::ExecuteOps");
    m_Assembler.AddConstToX86Reg(CX86Ops::x86_ESP, 4);

    if (g_System->bFastSP() && g_Recompiler)
    {
        m_Assembler.CallThis((uint32_t)g_Recompiler, AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos", 4);
    }
    if (g_SyncSystem)
    {
        UpdateSyncCPU(m_RegWorkingSet, g_System->CountPerOp());
    }

    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo & ExitRegSet, ExitReason reason)
{
    CompileExit(JumpPC, TargetPC, ExitRegSet, reason, true, nullptr);
}

void CX86RecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo & ExitRegSet, ExitReason reason, bool CompileNow, void (CX86Ops::*x86Jmp)(const char * Label, uint32_t Value))
{
    if (!CompileNow)
    {
        if (x86Jmp == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        stdstr_f ExitName("Exit_%08X_%d", JumpPC, m_ExitInfo.size());
        (m_Assembler.*x86Jmp)(ExitName.c_str(), 0);
        CExitInfo ExitInfo(m_CodeBlock);
        ExitInfo.ID = m_ExitInfo.size();
        ExitInfo.Name = ExitName;
        ExitInfo.TargetPC = TargetPC;
        ExitInfo.ExitRegSet = ExitRegSet;
        ExitInfo.Reason = reason;
        ExitInfo.PipelineStage = m_PipelineStage;
        ExitInfo.JumpLoc = (uint32_t *)(*g_RecompPos - 4);
        m_ExitInfo.push_back(ExitInfo);
        return;
    }

    //m_CodeBlock.Log("CompileExit: %d",reason);
    ExitRegSet.WriteBackRegisters();

    if (TargetPC != (uint32_t)-1)
    {
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", TargetPC);
        UpdateCounters(ExitRegSet, TargetPC <= JumpPC && JumpPC != -1, reason == ExitReason_Normal);
    }
    else
    {
        UpdateCounters(ExitRegSet, false, reason == ExitReason_Normal);
    }

    bool InDelaySlot = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
    switch (reason)
    {
    case ExitReason_Normal:
    case ExitReason_NormalNoSysCheck:
        ExitRegSet.SetBlockCycleCount(0);
        if (TargetPC != (uint32_t)-1)
        {
            if (TargetPC <= JumpPC && reason == ExitReason_Normal)
            {
                m_CodeBlock.Log("CompileSystemCheck 1");
                CompileSystemCheck((uint32_t)-1, ExitRegSet);
            }
        }
        else
        {
            if (reason == ExitReason_Normal)
            {
                m_CodeBlock.Log("CompileSystemCheck 2");
                CompileSystemCheck((uint32_t)-1, ExitRegSet);
            }
        }
#ifdef LinkBlocks
        if (g_SyncSystem)
        {
            m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
        }
        if (bSMM_ValidFunc == false)
        {
            if (LookUpMode() == FuncFind_ChangeMemory)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                //            uint8_t * Jump, * Jump2;
                //            if (TargetPC >= 0x80000000 && TargetPC < 0xC0000000) {
                //                uint32_t pAddr = TargetPC & 0x1FFFFFFF;
                //
                //                m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EAX, (uint8_t *)RDRAM + pAddr,"RDRAM + pAddr");
                //                Jump2 = nullptr;
                //            } else {
                //                m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (TargetPC >> 12));
                //                m_Assembler.MoveConstToX86reg(CX86Ops::x86_EBX, TargetPC);
                //                m_Assembler.MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",CX86Ops::x86_ECX,CX86Ops::x86_ECX,4);
                //                TestX86RegToX86Reg(CX86Ops::x86_ECX,CX86Ops::x86_ECX);
                //                m_Assembler.JeLabel8("NoTlbEntry",0);
                //                Jump2 = *g_RecompPos - 1;
                //                m_Assembler.MoveX86regPointerToX86reg(CX86Ops::x86_ECX, CX86Ops::x86_EBX,CX86Ops::x86_EAX);
                //            }
                //            m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_ECX, CX86Ops::x86_EAX);
                //            m_Assembler.AndConstToX86Reg(CX86Ops::x86_ECX,0xFFFF0000);
                //            m_Assembler.CompConstToX86reg(CX86Ops::x86_ECX,0x7C7C0000);
                //            m_Assembler.JneLabel8("NoCode",0);
                //            Jump = *g_RecompPos - 1;
                //            m_Assembler.AndConstToX86Reg(CX86Ops::x86_EAX,0xFFFF);
                //            m_Assembler.ShiftLeftSignImmed(CX86Ops::x86_EAX,4);
                //            m_Assembler.AddConstToX86Reg(CX86Ops::x86_EAX,0xC);
                //            m_Assembler.MoveVariableDispToX86Reg(OrigMem,"OrigMem",CX86Ops::x86_ECX,CX86Ops::x86_EAX,1);
                //            JmpDirectReg(CX86Ops::x86_ECX);
                //            m_CodeBlock.Log("      NoCode:");
                //            *((uint8_t *)(Jump))=(uint8_t)(*g_RecompPos - Jump - 1);
                //            if (Jump2 != nullptr) {
                //                m_CodeBlock.Log("      NoTlbEntry:");
                //                *((uint8_t *)(Jump2))=(uint8_t)(*g_RecompPos - Jump2 - 1);
                //            }
            }
            else if (LookUpMode() == FuncFind_VirtualLookup)
            {
                m_Assembler.MoveConstToX86reg(CX86Ops::x86_EDX, TargetPC);
                m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (uint32_t)&m_Functions);
                m_Assembler.CallFunc(AddressOf(&CFunctionMap::CompilerFindFunction), "CFunctionMap::CompilerFindFunction");
                m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_ECX, CX86Ops::x86_EAX);
                JecxzLabel8("NullPointer", 0);
                uint8_t * Jump = *g_RecompPos - 1;
                m_Assembler.MoveX86PointerToX86regDisp(CX86Ops::x86_EBX, CX86Ops::x86_ECX, 0xC);
                JmpDirectReg(CX86Ops::x86_EBX);
                m_CodeBlock.Log("      NullPointer:");
                *((uint8_t *)(Jump)) = (uint8_t)(*g_RecompPos - Jump - 1);
            }
            else if (LookUpMode() == FuncFind_PhysicalLookup)
            {
                uint8_t * Jump2 = nullptr;
                if (TargetPC >= 0x80000000 && TargetPC < 0x90000000)
                {
                    uint32_t pAddr = TargetPC & 0x1FFFFFFF;
                    m_Assembler.MoveVariableToX86reg(CX86Ops::x86_ECX, (uint8_t *)JumpTable + pAddr, "JumpTable + pAddr");
                }
                else if (TargetPC >= 0x90000000 && TargetPC < 0xC0000000)
                {
                }
                else
                {
                    m_Assembler.MoveConstToX86reg(CX86Ops::x86_ECX, (TargetPC >> 12));
                    m_Assembler.MoveConstToX86reg(CX86Ops::x86_EBX, TargetPC);
                    m_Assembler.MoveVariableDispToX86Reg(TLB_ReadMap, "TLB_ReadMap", CX86Ops::x86_ECX, CX86Ops::x86_ECX, 4);
                    TestX86RegToX86Reg(CX86Ops::x86_ECX, CX86Ops::x86_ECX);
                    m_Assembler.JeLabel8("NoTlbEntry", 0);
                    Jump2 = *g_RecompPos - 1;
                    m_Assembler.AddConstToX86Reg(CX86Ops::x86_ECX, (uint32_t)JumpTable - (uint32_t)RDRAM);
                    m_Assembler.MoveX86regPointerToX86reg(CX86Ops::x86_ECX, CX86Ops::x86_EBX, CX86Ops::x86_ECX);
                }
                if (TargetPC < 0x90000000 || TargetPC >= 0xC0000000)
                {
                    JecxzLabel8("NullPointer", 0);
                    uint8_t * Jump = *g_RecompPos - 1;
                    m_Assembler.MoveX86PointerToX86regDisp(CX86Ops::x86_EAX, CX86Ops::x86_ECX, 0xC);
                    JmpDirectReg(CX86Ops::x86_EAX);
                    m_CodeBlock.Log("      NullPointer:");
                    *((uint8_t *)(Jump)) = (uint8_t)(*g_RecompPos - Jump - 1);
                    if (Jump2 != nullptr)
                    {
                        m_CodeBlock.Log("      NoTlbEntry:");
                        *((uint8_t *)(Jump2)) = (uint8_t)(*g_RecompPos - Jump2 - 1);
                    }
                }
            }
        }
        ExitCodeBlock();
#else
        ExitCodeBlock();
#endif
        break;
    case ExitReason_DoCPUAction:
        m_Assembler.CallThis((uint32_t)g_SystemEvents, AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents", 4);
        ExitCodeBlock();
        break;
    case ExitReason_DoSysCall:
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoSysCallException), "CRegisters::DoSysCallException", 8);
        ExitCodeBlock();
        break;
    case ExitReason_Break:
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoBreakException), "CRegisters::DoBreakException", 8);
        ExitCodeBlock();
        break;
    case ExitReason_COP1Unuseable:
        m_Assembler.PushImm32("1", 1);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoCopUnusableException), "CRegisters::DoCopUnusableException", 12);
        ExitCodeBlock();
        break;
    case ExitReason_ResetRecompCode:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case ExitReason_TLBReadMiss:
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EDX, g_TLBLoadAddress, "g_TLBLoadAddress");
        m_Assembler.Push(CX86Ops::x86_EDX);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoTLBReadMiss), "CRegisters::DoTLBReadMiss", 12);
        ExitCodeBlock();
        break;
    case ExitReason_TLBWriteMiss:
        m_Assembler.X86BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case ExitReason_ExceptionOverflow:
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoOverflowException), "CRegisters::DoOverflowException", 12);
        ExitCodeBlock();
        break;
    case ExitReason_AddressErrorExceptionRead32:
        m_Assembler.PushImm32("1", 1);
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EDX, &m_TempValue32, "TempValue32");
        m_Assembler.MoveX86RegToX86Reg(CX86Ops::x86_EAX, CX86Ops::x86_EDX);
        m_Assembler.ShiftRightSignImmed(CX86Ops::x86_EAX, 31);
        m_Assembler.Push(CX86Ops::x86_EAX);
        m_Assembler.Push(CX86Ops::x86_EDX);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoAddressError), "CRegisters::DoAddressError", 12);
        ExitCodeBlock();
        break;
    case ExitReason_AddressErrorExceptionRead64:
        m_Assembler.PushImm32("1", 1);
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EDX, &m_TempValue64, "TempValue64");
        m_Assembler.MoveVariableToX86reg(CX86Ops::x86_EAX, &m_TempValue64 + 4, "TempValue64+4");
        m_Assembler.Push(CX86Ops::x86_EAX);
        m_Assembler.Push(CX86Ops::x86_EDX);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoAddressError), "CRegisters::DoAddressError", 12);
        ExitCodeBlock();
        break;
    case ExitReason_IllegalInstruction:
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoIllegalInstructionException), "CRegisters::DoIllegalInstructionException", 8);
        ExitCodeBlock();
        break;
    default:
        WriteTrace(TraceRecompiler, TraceError, "How did you want to exit on reason (%d) ???", reason);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

CX86Ops::x86Reg CX86RecompilerOps::BaseOffsetAddress(bool UseBaseRegister)
{
    CX86Ops::x86Reg AddressReg;
    if (IsMapped(m_Opcode.base))
    {
        if (m_Opcode.offset != 0)
        {
            bool UnProtect = m_RegWorkingSet.GetX86Protected(GetIndexFromX86Reg(GetMipsRegMapLo(m_Opcode.base)));
            ProtectGPR(m_Opcode.base);
            AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.LeaSourceAndOffset(AddressReg, GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            if (!UnProtect)
            {
                UnProtectGPR(m_Opcode.base);
            }
        }
        else if (UseBaseRegister)
        {
            ProtectGPR(m_Opcode.base);
            AddressReg = GetMipsRegMapLo(m_Opcode.base);
        }
        else
        {
            AddressReg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.base, false, false);
        }
    }
    else
    {
        AddressReg = Map_TempReg(CX86Ops::x86_Unknown, m_Opcode.base, false, false);
        m_Assembler.AddConstToX86Reg(AddressReg, (int16_t)m_Opcode.immediate);
    }

    if (!b32BitCore() && ((IsKnown(m_Opcode.base) && Is64Bit(m_Opcode.base)) || IsUnknown(m_Opcode.base)))
    {
        m_Assembler.MoveX86regToVariable(AddressReg, &m_TempValue64, "TempValue64");
        CX86Ops::x86Reg AddressRegHi = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveX86RegToX86Reg(AddressRegHi, AddressReg);
        m_Assembler.ShiftRightSignImmed(AddressRegHi, 31);

        if (IsConst(m_Opcode.base))
        {
            m_Assembler.MoveConstToVariable(&m_TempValue64 + 4, "TempValue64 + 4", GetMipsRegHi(m_Opcode.base));
            m_Assembler.CompConstToX86reg(AddressRegHi, GetMipsRegHi(m_Opcode.base));
        }
        else if (IsMapped(m_Opcode.base))
        {
            m_Assembler.MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.base), &m_TempValue64 + 4, "TempValue64 + 4");
            m_Assembler.CompX86RegToX86Reg(AddressRegHi, GetMipsRegMapHi(m_Opcode.base));
        }
        else
        {
            CX86Ops::x86Reg AddressMemoryHi = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(AddressMemoryHi, &_GPR[m_Opcode.base].W[1], CRegName::GPR_Hi[m_Opcode.base]);
            m_Assembler.MoveX86regToVariable(AddressMemoryHi, &m_TempValue64 + 4, "TempValue64 + 4");
            m_Assembler.CompX86RegToX86Reg(AddressRegHi, AddressMemoryHi);
            m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(AddressMemoryHi), false);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_AddressErrorExceptionRead64, false, &CX86Ops::JneLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(AddressRegHi), false);
    }
    return AddressReg;
}

void CX86RecompilerOps::CompileLoadMemoryValue(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg ValueReg, CX86Ops::x86Reg ValueRegHi, uint8_t ValueSize, bool SignExtend)
{
    bool UnprotectAddressReg = AddressReg == CX86Ops::x86_Unknown;
    if (AddressReg == CX86Ops::x86_Unknown)
    {
        if (ValueSize == 8)
        {
            AddressReg = BaseOffsetAddress(false);
            TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint8, "x86TestReadBreakpoint8");
        }
        else if (ValueSize == 16)
        {
            AddressReg = BaseOffsetAddress(false);
            TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint16, "x86TestReadBreakpoint16");
        }
        else if (ValueSize == 32)
        {
            AddressReg = BaseOffsetAddress(true);
            TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
        }
        else if (ValueSize == 64)
        {
            AddressReg = BaseOffsetAddress(true);
            TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint64, "x86TestReadBreakpoint64");
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    if (ValueSize == 16)
    {
        m_Assembler.MoveX86regToVariable(AddressReg, &m_TempValue32, "TempValue32");
        m_Assembler.TestConstToX86Reg(1, AddressReg);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_AddressErrorExceptionRead32, false, &CX86Ops::JneLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    }
    else if (ValueSize == 32)
    {
        m_Assembler.MoveX86regToVariable(AddressReg, &m_TempValue32, "TempValue32");
        m_Assembler.TestConstToX86Reg(3, AddressReg);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_AddressErrorExceptionRead32, false, &CX86Ops::JneLabel32);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    }

    m_Assembler.MoveX86RegToX86Reg(TempReg, AddressReg);
    m_Assembler.ShiftRightUnsignImmed(TempReg, 12);
    m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg, TempReg, 4);
    m_Assembler.CompConstToX86reg(TempReg, (uint32_t)-1);
    m_Assembler.JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    uint32_t OpsExecuted = m_RegWorkingSet.GetBlockCycleCount();
    if (OpsExecuted != 0)
    {
        m_Assembler.SubConstFromVariable(OpsExecuted, g_NextTimer, "g_NextTimer");
    }
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
    if (ValueSize == 32)
    {
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)(&m_MMU), AddressOf(&CMipsMemoryVM::LW_NonMemory), "CMipsMemoryVM::LW_NonMemory", 12);
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.MoveConstToX86reg(TempReg, (uint32_t)&m_TempValue32);
        m_Assembler.SubX86RegToX86Reg(TempReg, AddressReg);
    }
    else if (ValueSize == 16)
    {
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)(&m_MMU), AddressOf(&CMipsMemoryVM::LH_NonMemory), "CMipsMemoryVM::LH_NonMemory", 12);
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.MoveConstToX86reg(TempReg, (uint32_t)&m_TempValue32);
        m_Assembler.SubX86RegToX86Reg(TempReg, AddressReg);
        m_Assembler.XorConstToX86Reg(AddressReg, 2);
    }
    else if (ValueSize == 8)
    {
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::LB_NonMemory), "CMipsMemoryVM::LB_NonMemory", 12);
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.MoveConstToX86reg(TempReg, (uint32_t)&m_TempValue32);
        m_Assembler.SubX86RegToX86Reg(TempReg, AddressReg);
        m_Assembler.XorConstToX86Reg(AddressReg, 3);
    }
    else
    {
        m_Assembler.X86BreakPoint(__FILE__, __LINE__);
    }
    if (OpsExecuted != 0)
    {
        m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    m_Assembler.SetJump8(JumpFound, *g_RecompPos);

    if (ValueSize == 8)
    {
        m_Assembler.XorConstToX86Reg(AddressReg, 3);
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (SignExtend)
        {
            m_Assembler.MoveSxByteX86regPointerToX86reg(ValueReg, AddressReg, TempReg);
        }
        else
        {
            m_Assembler.MoveZxByteX86regPointerToX86reg(AddressReg, TempReg, ValueReg);
        }
    }
    else if (ValueSize == 16)
    {
        m_Assembler.XorConstToX86Reg(AddressReg, 2);
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            Map_GPR_32bit(m_Opcode.rt, SignExtend, -1);
            ValueReg = GetMipsRegMapLo(m_Opcode.rt);
        }

        if (SignExtend)
        {
            m_Assembler.MoveSxHalfX86regPointerToX86reg(ValueReg, AddressReg, TempReg);
        }
        else
        {
            m_Assembler.MoveZxHalfX86regPointerToX86reg(AddressReg, TempReg, ValueReg);
        }
    }
    else if (ValueSize == 32)
    {
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            Map_GPR_32bit(m_Opcode.rt, true, -1);
            ValueReg = GetMipsRegMapLo(m_Opcode.rt);
        }

        if (ValueReg != CX86Ops::x86_Unknown)
        {
            m_Assembler.MoveX86regPointerToX86reg(AddressReg, TempReg, ValueReg);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (ValueSize == 64)
    {
        if (ValueReg != CX86Ops::x86_Unknown)
        {
            m_Assembler.MoveX86regPointerToX86reg(AddressReg, TempReg, ValueRegHi);
            m_Assembler.MoveX86regPointerToX86regDisp8(AddressReg, TempReg, ValueReg, 4);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (UnprotectAddressReg)
    {
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(AddressReg), false);
    }
}

void CX86RecompilerOps::CompileStoreMemoryValue(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg ValueReg, CX86Ops::x86Reg ValueRegHi, uint64_t Value, uint8_t ValueSize)
{
    uint8_t * MemoryWriteDone = nullptr;

    if (AddressReg == CX86Ops::x86_Unknown)
    {
        AddressReg = BaseOffsetAddress(ValueSize == 32);
        if (ValueSize == 8)
        {
            TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint8, "x86TestWriteBreakpoint8");
        }
        else if (ValueSize == 16)
        {
            TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint16, "x86TestWriteBreakpoint16");
        }
        else if (ValueSize == 32)
        {
            TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");
        }
        else if (ValueSize == 64)
        {
            TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint64, "x86TestWriteBreakpoint64");
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    CX86Ops::x86Reg TempReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
    m_Assembler.MoveX86RegToX86Reg(TempReg, AddressReg);
    m_Assembler.ShiftRightUnsignImmed(TempReg, 12);
    m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_MemoryWriteMap, "MMU->m_MemoryWriteMap", TempReg, TempReg, 4);
    m_Assembler.CompConstToX86reg(TempReg, (uint32_t)-1);
    m_Assembler.JneLabel8(stdstr_f("MemoryWriteMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);

    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", m_PipelineStage);
    uint32_t OpsExecuted = m_RegWorkingSet.GetBlockCycleCount();
    if (OpsExecuted != 0)
    {
        m_Assembler.SubConstFromVariable(OpsExecuted, g_NextTimer, "g_NextTimer");
    }
    if (ValueSize == 8)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.PushImm32((uint32_t)Value);
        }
        else
        {
            m_Assembler.Push(ValueReg);
        }
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SB_NonMemory), "CMipsMemoryVM::SB_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.JmpLabel8(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), 0);
        MemoryWriteDone = (uint8_t *)(*g_RecompPos - 1);
    }
    else if (ValueSize == 16)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.PushImm32((uint32_t)(Value & 0xFFFF));
        }
        else
        {
            m_Assembler.Push(ValueReg);
        }
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SH_NonMemory), "CMipsMemoryVM::SH_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.JmpLabel8(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), 0);
        MemoryWriteDone = (uint8_t *)(*g_RecompPos - 1);
    }
    else if (ValueSize == 32)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.PushImm32((uint32_t)(Value & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.Push(ValueReg);
        }
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.JmpLabel8(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), 0);
        MemoryWriteDone = (uint8_t *)(*g_RecompPos - 1);
    }
    else if (ValueSize == 64)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.PushImm32((uint32_t)(Value & 0xFFFFFFFF));
            m_Assembler.PushImm32((uint32_t)((Value >> 32) & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.Push(ValueReg);
            m_Assembler.Push(ValueRegHi);
        }
        m_Assembler.Push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SD_NonMemory), "CMipsMemoryVM::SD_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.TestX86ByteRegToX86Reg(CX86Ops::x86_AL, CX86Ops::x86_AL);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel32);
        m_Assembler.JmpLabel8(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), 0);
        MemoryWriteDone = (uint8_t *)(*g_RecompPos - 1);
    }
    else
    {
        m_Assembler.X86BreakPoint(__FILE__, __LINE__);
        m_Assembler.MoveX86RegToX86Reg(TempReg, AddressReg);
        m_Assembler.ShiftRightUnsignImmed(TempReg, 12);
        m_Assembler.MoveVariableDispToX86Reg(g_MMU->m_TLB_WriteMap, "MMU->TLB_WriteMap", TempReg, TempReg, 4);
        CompileWriteTLBMiss(AddressReg, TempReg);
        m_Assembler.AddConstToX86Reg(TempReg, (uint32_t)m_MMU.Rdram());
    }
    m_CodeBlock.Log("");
    m_CodeBlock.Log(stdstr_f("      MemoryWriteMap_%X_Found:", m_CompilePC).c_str());
    m_Assembler.SetJump8(JumpFound, *g_RecompPos);

    if (ValueSize == 8)
    {
        m_Assembler.XorConstToX86Reg(AddressReg, 3);
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.MoveConstByteToX86regPointer(AddressReg, TempReg, (uint8_t)(Value & 0xFF));
        }
        else if (m_Assembler.Is8BitReg(ValueReg))
        {
            m_Assembler.MoveX86regByteToX86regPointer(ValueReg, AddressReg, TempReg);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (ValueSize == 16)
    {
        m_Assembler.XorConstToX86Reg(AddressReg, 2);
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.MoveConstHalfToX86regPointer(AddressReg, TempReg, (uint16_t)(Value & 0xFFFF));
        }
        else
        {
            m_Assembler.MoveX86regHalfToX86regPointer(ValueReg, AddressReg, TempReg);
        }
    }
    else if (ValueSize == 32)
    {
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.MoveConstToX86regPointer(AddressReg, TempReg, (uint32_t)(Value & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.MoveX86regToX86regPointer(AddressReg, TempReg, ValueReg );
        }
    }
    else if (ValueSize == 64)
    {
        if (ValueReg == CX86Ops::x86_Unknown)
        {
            m_Assembler.MoveConstToX86regPointer(AddressReg, TempReg, (uint32_t)(Value >> 32));
            m_Assembler.AddConstToX86Reg(AddressReg, 4);
            m_Assembler.MoveConstToX86regPointer(AddressReg, TempReg, (uint32_t)(Value & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.MoveX86regToX86regPointer(AddressReg, TempReg, ValueRegHi);
            m_Assembler.AddConstToX86Reg(AddressReg, 4);
            m_Assembler.MoveX86regToX86regPointer(AddressReg, TempReg, ValueReg);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (MemoryWriteDone != nullptr)
    {
        m_CodeBlock.Log("");
        m_CodeBlock.Log(stdstr_f("      MemoryWrite_%X_Done:", m_CompilePC).c_str());
        m_Assembler.SetJump8(MemoryWriteDone, *g_RecompPos);
    }
}

void CX86RecompilerOps::SB_Const(uint32_t Value, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, Value, 8);
        return;
    }

    uint32_t PAddr;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (CGameSettings::bSMM_StoreInstruc())
        {
            CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, Value, 8);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            m_Assembler.MoveConstByteToVariable((PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 3)", PAddr).c_str(), (uint8_t)Value);
        }
        break;
    case 0x04000000:
        if (PAddr < 0x04001000)
        {
            m_Assembler.MoveConstByteToVariable(((PAddr - 0x04000000) ^ 3) + g_MMU->Dmem(), stdstr_f("DMem + (%X ^ 3)", (PAddr - 0x04000000)).c_str(), (uint8_t)Value);
        }
        else if (PAddr < 0x04002000)
        {
            m_Assembler.MoveConstByteToVariable(((PAddr - 0x04001000) ^ 3) + g_MMU->Imem(), stdstr_f("Imem + (%X ^ 3)", (PAddr - 0x04001000)).c_str(), (uint8_t)Value);
        }
        else
        {
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    default:
        if (PAddr >= 0x10000000 && PAddr < 0x13F00000)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.PushImm32(Value << ((3 - (PAddr & 3)) * 8));
            m_Assembler.PushImm32(PAddr);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][1], "RomMemoryHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::SB_Register(CX86Ops::x86Reg Reg, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 8);
        return;
    }

    uint32_t PAddr;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (CGameSettings::bSMM_StoreInstruc())
        {
            CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 8);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            m_Assembler.MoveX86regByteToVariable(Reg, (PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 3)", PAddr).c_str());
        }
        break;
    default:
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::SH_Const(uint32_t Value, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, (uint16_t)Value, 16);
        return;
    }

    uint32_t PAddr;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (CGameSettings::bSMM_StoreInstruc())
        {
            CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, (uint16_t)Value, 16);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            m_Assembler.MoveConstHalfToVariable((PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str(), (uint16_t)Value);
        }
        break;
    default:
        if (PAddr >= 0x10000000 && PAddr < 0x13F00000)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.PushImm32(Value << ((2 - (PAddr & 2)) * 8));
            m_Assembler.PushImm32(PAddr);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][1], "RomMemoryHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::SH_Register(CX86Ops::x86Reg Reg, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);

        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 16);
    }
    else
    {
        uint32_t PAddr;
        if (m_MMU.VAddrToPAddr(VAddr, PAddr))
        {
            switch (PAddr & 0xFFF00000)
            {
            case 0x00000000:
            case 0x00100000:
            case 0x00200000:
            case 0x00300000:
            case 0x00400000:
            case 0x00500000:
            case 0x00600000:
            case 0x00700000:
                if (CGameSettings::bSMM_StoreInstruc())
                {
                    CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
                    m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
                    CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 16);
                }
                else if (PAddr < g_MMU->RdramSize())
                {
                    m_Assembler.MoveX86regHalfToVariable(Reg, (PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str());
                }
                break;
            default:
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        else
        {
            m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
}

void CX86RecompilerOps::SW_Const(uint32_t Value, uint32_t VAddr)
{
    uint8_t * Jump;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, Value, 32);
        return;
    }

    uint32_t PAddr;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (CGameSettings::bSMM_StoreInstruc())
        {
            CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, CX86Ops::x86_Unknown, CX86Ops::x86_Unknown, Value, 32);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            m_Assembler.MoveConstToVariable(PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str(), Value);
        }
        break;
    case 0x03F00000:
        switch (PAddr)
        {
        case 0x03F00000: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_CONFIG_REG, "RDRAM_CONFIG_REG", Value); break;
        case 0x03F00004: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_DEVICE_ID_REG, "RDRAM_DEVICE_ID_REG", Value); break;
        case 0x03F00008: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_DELAY_REG, "RDRAM_DELAY_REG", Value); break;
        case 0x03F0000C: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_MODE_REG, "RDRAM_MODE_REG", Value); break;
        case 0x03F00010: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_REF_INTERVAL_REG, "RDRAM_REF_INTERVAL_REG", Value); break;
        case 0x03F00014: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_REF_ROW_REG, "RDRAM_REF_ROW_REG", Value); break;
        case 0x03F00018: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_RAS_INTERVAL_REG, "RDRAM_RAS_INTERVAL_REG", Value); break;
        case 0x03F0001C: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_MIN_INTERVAL_REG, "RDRAM_MIN_INTERVAL_REG", Value); break;
        case 0x03F00020: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_ADDR_SELECT_REG, "RDRAM_ADDR_SELECT_REG", Value); break;
        case 0x03F00024: m_Assembler.MoveConstToVariable(&g_Reg->RDRAM_DEVICE_MANUF_REG, "RDRAM_DEVICE_MANUF_REG", Value); break;
        case 0x03F04004: break;
        case 0x03F08004: break;
        case 0x03F80004: break;
        case 0x03F80008: break;
        case 0x03F8000C: break;
        case 0x03F80014: break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04000000:
        if (PAddr < 0x04001000)
        {
            m_Assembler.MoveConstToVariable((PAddr - 0x04000000) + g_MMU->Dmem(), stdstr_f("DMem + %X", (PAddr - 0x04000000)).c_str(), Value);
        }
        else if (PAddr < 0x04002000)
        {
            m_Assembler.MoveConstToVariable((PAddr - 0x04001000) + g_MMU->Imem(), stdstr_f("Imem + %X", (PAddr - 0x04001000)).c_str(), Value);
        }
        else
        {
            switch (PAddr)
            {
            case 0x04040000: m_Assembler.MoveConstToVariable(&g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG", Value); break;
            case 0x04040004: m_Assembler.MoveConstToVariable(&g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG", Value); break;
            case 0x04040008:
            case 0x0404000C:
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.PushImm32(0xFFFFFFFF);
                m_Assembler.PushImm32(Value);
                m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
                m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_SPRegistersHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_SPRegistersHandler)[0][1], "SPRegistersHandler::Write32", 16);
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04040010:
                UpdateCounters(m_RegWorkingSet, false, true, false);

                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.PushImm32(Value);
                m_Assembler.PushImm32(PAddr | 0xA0000000);
                m_Assembler.CallThis((uint32_t)g_MMU, AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x0404001C: m_Assembler.MoveConstToVariable(&g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", 0); break;
            case 0x04080000: m_Assembler.MoveConstToVariable(&g_Reg->SP_PC_REG, "SP_PC_REG", Value & 0xFFC); break;
            default:
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        break;
    case 0x04100000:
        switch (PAddr)
        {
        case 0x0410000C:
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(Value);
            m_Assembler.PushImm32(PAddr | 0xA0000000);
            m_Assembler.CallThis((uint32_t)g_MMU, AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04300000:
        switch (PAddr)
        {
        case 0x04300000:
        {
            uint32_t ModValue;
            ModValue = 0x7F;
            if ((Value & MI_CLR_INIT) != 0)
            {
                ModValue |= MI_MODE_INIT;
            }
            if ((Value & MI_CLR_EBUS) != 0)
            {
                ModValue |= MI_MODE_EBUS;
            }
            if ((Value & MI_CLR_RDRAM) != 0)
            {
                ModValue |= MI_MODE_RDRAM;
            }
            if (ModValue != 0)
            {
                m_Assembler.AndConstToVariable(&g_Reg->MI_MODE_REG, "MI_MODE_REG", ~ModValue);
            }

            ModValue = (Value & 0x7F);
            if ((Value & MI_SET_INIT) != 0)
            {
                ModValue |= MI_MODE_INIT;
            }
            if ((Value & MI_SET_EBUS) != 0)
            {
                ModValue |= MI_MODE_EBUS;
            }
            if ((Value & MI_SET_RDRAM) != 0)
            {
                ModValue |= MI_MODE_RDRAM;
            }
            if (ModValue != 0)
            {
                m_Assembler.OrConstToVariable(ModValue, &g_Reg->MI_MODE_REG, "MI_MODE_REG");
            }
            if ((Value & MI_CLR_DP_INTR) != 0)
            {
                m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_DP);
                m_Assembler.AndConstToVariable(&g_Reg->m_GfxIntrReg, "m_GfxIntrReg", (uint32_t)~MI_INTR_DP);
            }
            break;
        }
        case 0x0430000C:
        {
            uint32_t ModValue;
            ModValue = 0;
            if ((Value & MI_INTR_MASK_CLR_SP) != 0)
            {
                ModValue |= MI_INTR_MASK_SP;
            }
            if ((Value & MI_INTR_MASK_CLR_SI) != 0)
            {
                ModValue |= MI_INTR_MASK_SI;
            }
            if ((Value & MI_INTR_MASK_CLR_AI) != 0)
            {
                ModValue |= MI_INTR_MASK_AI;
            }
            if ((Value & MI_INTR_MASK_CLR_VI) != 0)
            {
                ModValue |= MI_INTR_MASK_VI;
            }
            if ((Value & MI_INTR_MASK_CLR_PI) != 0)
            {
                ModValue |= MI_INTR_MASK_PI;
            }
            if ((Value & MI_INTR_MASK_CLR_DP) != 0)
            {
                ModValue |= MI_INTR_MASK_DP;
            }
            if (ModValue != 0)
            {
                m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG", ~ModValue);
            }

            ModValue = 0;
            if ((Value & MI_INTR_MASK_SET_SP) != 0)
            {
                ModValue |= MI_INTR_MASK_SP;
            }
            if ((Value & MI_INTR_MASK_SET_SI) != 0)
            {
                ModValue |= MI_INTR_MASK_SI;
            }
            if ((Value & MI_INTR_MASK_SET_AI) != 0)
            {
                ModValue |= MI_INTR_MASK_AI;
            }
            if ((Value & MI_INTR_MASK_SET_VI) != 0)
            {
                ModValue |= MI_INTR_MASK_VI;
            }
            if ((Value & MI_INTR_MASK_SET_PI) != 0)
            {
                ModValue |= MI_INTR_MASK_PI;
            }
            if ((Value & MI_INTR_MASK_SET_DP) != 0)
            {
                ModValue |= MI_INTR_MASK_DP;
            }
            if (ModValue != 0)
            {
                m_Assembler.OrConstToVariable(ModValue, &g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG");
            }
            break;
        }
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04400000:
        if (GenerateLog() && LogVideoInterface())
        {
            UpdateCounters(m_RegWorkingSet, false, true, false);

            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.PushImm32(Value);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][1], "VideoInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else
        {
            switch (PAddr)
            {
            case 0x04400000:
                if (g_Plugins->Gfx()->ViStatusChanged != nullptr)
                {
                    m_Assembler.CompConstToVariable(&g_Reg->VI_STATUS_REG, "VI_STATUS_REG", Value);
                    m_Assembler.JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    m_Assembler.MoveConstToVariable(&g_Reg->VI_STATUS_REG, "VI_STATUS_REG", Value);
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      Continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x04400004: m_Assembler.MoveConstToVariable(&g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG", (Value & 0xFFFFFF)); break;
            case 0x04400008:
                if (g_Plugins->Gfx()->ViWidthChanged != nullptr)
                {
                    m_Assembler.CompConstToVariable(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", Value);
                    m_Assembler.JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    m_Assembler.MoveConstToVariable(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", Value);
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      Continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x0440000C: m_Assembler.MoveConstToVariable(&g_Reg->VI_INTR_REG, "VI_INTR_REG", Value); break;
            case 0x04400010:
                m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_VI);
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts", 4);
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04400014: m_Assembler.MoveConstToVariable(&g_Reg->VI_BURST_REG, "VI_BURST_REG", Value); break;
            case 0x04400018: m_Assembler.MoveConstToVariable(&g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG", Value); break;
            case 0x0440001C: m_Assembler.MoveConstToVariable(&g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG", Value); break;
            case 0x04400020: m_Assembler.MoveConstToVariable(&g_Reg->VI_LEAP_REG, "VI_LEAP_REG", Value); break;
            case 0x04400024: m_Assembler.MoveConstToVariable(&g_Reg->VI_H_START_REG, "VI_H_START_REG", Value); break;
            case 0x04400028: m_Assembler.MoveConstToVariable(&g_Reg->VI_V_START_REG, "VI_V_START_REG", Value); break;
            case 0x0440002C: m_Assembler.MoveConstToVariable(&g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG", Value); break;
            case 0x04400030: m_Assembler.MoveConstToVariable(&g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG", Value); break;
            case 0x04400034: m_Assembler.MoveConstToVariable(&g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG", Value); break;
            default:
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        break;
    case 0x04500000:
        UpdateCounters(m_RegWorkingSet, false, true, false);

        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32(0xFFFFFFFF);
        m_Assembler.PushImm32(Value);
        m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
        m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][1], "AudioInterfaceHandler::Write32", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04600000:
        switch (PAddr)
        {
        case 0x04600000:
        case 0x04600004:
        case 0x04600008:
        case 0x0460000C:
        case 0x04600010:
            if (PAddr == 0x04600008 || PAddr == 0x0460000C)
            {
                UpdateCounters(m_RegWorkingSet, false, true, false);
            }
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.PushImm32(Value);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_PeripheralInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_PeripheralInterfaceHandler)[0][1], "PeripheralInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600014: m_Assembler.MoveConstToVariable(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", (Value & 0xFF)); break;
        case 0x04600018: m_Assembler.MoveConstToVariable(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", (Value & 0xFF)); break;
        case 0x0460001C: m_Assembler.MoveConstToVariable(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", (Value & 0xFF)); break;
        case 0x04600020: m_Assembler.MoveConstToVariable(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", (Value & 0xFF)); break;
        case 0x04600024: m_Assembler.MoveConstToVariable(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", (Value & 0xFF)); break;
        case 0x04600028: m_Assembler.MoveConstToVariable(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", (Value & 0xFF)); break;
        case 0x0460002C: m_Assembler.MoveConstToVariable(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", (Value & 0xFF)); break;
        case 0x04600030: m_Assembler.MoveConstToVariable(&g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", (Value & 0xFF)); break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04700000:
        switch (PAddr)
        {
        case 0x04700000: m_Assembler.MoveConstToVariable(&g_Reg->RI_MODE_REG, "RI_MODE_REG", Value); break;
        case 0x04700004: m_Assembler.MoveConstToVariable(&g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG", Value); break;
        case 0x04700008: m_Assembler.MoveConstToVariable(&g_Reg->RI_CURRENT_LOAD_REG, "RI_CURRENT_LOAD_REG", Value); break;
        case 0x0470000C: m_Assembler.MoveConstToVariable(&g_Reg->RI_SELECT_REG, "RI_SELECT_REG", Value); break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04800000:
        switch (PAddr)
        {
        case 0x04800000: m_Assembler.MoveConstToVariable(&g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG", Value); break;
        case 0x04800004:
            UpdateCounters(m_RegWorkingSet, false, true, false);
            m_Assembler.MoveConstToVariable(&g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG", Value);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)(&g_MMU->m_PifRamHandler), AddressOf(&PifRamHandler::DMA_READ), "PifRamHandler::DMA_READ", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            UpdateCounters(m_RegWorkingSet, false, true, false);
            m_Assembler.MoveConstToVariable(&g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG", Value);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)(&g_MMU->m_PifRamHandler), AddressOf(&PifRamHandler::DMA_WRITE), "PifRamHandler::DMA_WRITE", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800018:
            m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_SI);
            m_Assembler.AndConstToVariable(&g_Reg->SI_STATUS_REG, "SI_STATUS_REG", (uint32_t)~SI_STATUS_INTERRUPT);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x05000000:
        // 64DD registers
        if (EnableDisk())
        {
            switch (PAddr)
            {
            case 0x05000520:
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallFunc(AddressOf(&DiskReset), "DiskReset");
                m_RegWorkingSet.AfterCallDirect();
                break;
            default:
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            break;
        }
    case 0x1fc00000:
    {
        UpdateCounters(m_RegWorkingSet, false, true, false);

        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32(Value);
        m_Assembler.PushImm32(PAddr | 0xA0000000);
        m_Assembler.CallThis((uint32_t)g_MMU, AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 4);
        m_RegWorkingSet.AfterCallDirect();
        break;
    }
    default:
        if (PAddr >= 0x10000000 && PAddr < 0x13F00000)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.PushImm32(Value);
            m_Assembler.PushImm32(PAddr);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][1], "RomMemoryHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else
        {
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            UpdateCounters(m_RegWorkingSet, false, true, true);

            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(Value);
            m_Assembler.PushImm32(PAddr | 0xA0000000);
            m_Assembler.CallThis((uint32_t)(g_MMU), AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
            m_RegWorkingSet.AfterCallDirect();
        }
    }
}

void CX86RecompilerOps::SW_Register(CX86Ops::x86Reg Reg, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);
        CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 32);
        return;
    }

    char VarName[100];
    uint32_t PAddr;

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        m_CodeBlock.Log("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        return;
    }

    switch (PAddr & 0xFFF00000)
    {
    case 0x00000000:
    case 0x00100000:
    case 0x00200000:
    case 0x00300000:
    case 0x00400000:
    case 0x00500000:
    case 0x00600000:
    case 0x00700000:
        if (CGameSettings::bSMM_StoreInstruc())
        {
            CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 32);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            sprintf(VarName, "RDRAM + %X", PAddr);
            m_Assembler.MoveX86regToVariable(Reg, PAddr + g_MMU->Rdram(), VarName);
        }
        break;
    case 0x04000000:
        switch (PAddr)
        {
        case 0x04040000: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG"); break;
        case 0x04040004: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG"); break;
        case 0x04040008:
        case 0x0404000C:
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.Push(Reg);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_SPRegistersHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_SPRegistersHandler)[0][1], "SPRegistersHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04040010:
            UpdateCounters(m_RegWorkingSet, false, true, false);
            m_Assembler.MoveX86regToVariable(Reg, &CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue");
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallFunc((uint32_t)CMipsMemoryVM::ChangeSpStatus, "CMipsMemoryVM::ChangeSpStatus");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0404001C: m_Assembler.MoveConstToVariable(&g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", 0); break;
        case 0x04080000:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->SP_PC_REG, "SP_PC_REG");
            m_Assembler.AndConstToVariable(&g_Reg->SP_PC_REG, "SP_PC_REG", 0xFFC);
            break;
        default:
            if (CGameSettings::bSMM_StoreInstruc())
            {
                CX86Ops::x86Reg AddressReg = Map_TempReg(CX86Ops::x86_Unknown, -1, false, false);
                m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
                CompileStoreMemoryValue(AddressReg, Reg, CX86Ops::x86_Unknown, 0, 32);
            }
            else if (PAddr < 0x04001000)
            {
                m_Assembler.MoveX86regToVariable(Reg, g_MMU->Dmem() + (PAddr - 0x04000000), stdstr_f("DMEM + 0x%X", (PAddr - 0x04000000)).c_str());
            }
            else if (PAddr < 0x04002000)
            {
                m_Assembler.MoveX86regToVariable(Reg, g_MMU->Imem() + (PAddr - 0x04001000), stdstr_f("IMEM + 0x%X", (PAddr - 0x04001000)).c_str());
            }
            else
            {
                m_CodeBlock.Log("    should be moving %s in to %08X ?", CX86Ops::x86_Name(Reg), VAddr);
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        break;
    case 0x04100000:
        if (PAddr == 0x0410000C)
        {
            UpdateCounters(m_RegWorkingSet, false, true, false);
        }
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.Push(Reg);
        m_Assembler.PushImm32(PAddr | 0xA0000000);
        m_Assembler.CallThis((uint32_t)(g_MMU), AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04300000:
        switch (PAddr)
        {
        case 0x04300000:
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.Push(Reg);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_MIPSInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_MIPSInterfaceHandler)[0][1], "MIPSInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0430000C:
            m_Assembler.MoveX86regToVariable(Reg, &CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue");
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallFunc((uint32_t)CMipsMemoryVM::ChangeMiIntrMask, "CMipsMemoryVM::ChangeMiIntrMask");
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            m_CodeBlock.Log("    should be moving %s in to %08X ?", CX86Ops::x86_Name(Reg), VAddr);
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04400000:
        if (GenerateLog() && LogVideoInterface())
        {
            UpdateCounters(m_RegWorkingSet, false, true, false);

            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.Push(Reg);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][1], "VideoInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else
        {
            switch (PAddr)
            {
            case 0x04400000:
                if (g_Plugins->Gfx()->ViStatusChanged != nullptr)
                {
                    uint8_t * Jump;
                    m_Assembler.CompX86regToVariable(Reg, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    m_Assembler.JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      Continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x04400004:
                m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG");
                m_Assembler.AndConstToVariable(&g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG", 0xFFFFFF);
                break;
            case 0x04400008:
                if (g_Plugins->Gfx()->ViWidthChanged != nullptr)
                {
                    uint8_t * Jump;
                    m_Assembler.CompX86regToVariable(Reg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    m_Assembler.JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_CodeBlock.Log("      Continue:");
                    m_Assembler.SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x0440000C: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_INTR_REG, "VI_INTR_REG"); break;
            case 0x04400010:
                m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_VI);
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts", 4);
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04400014: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_BURST_REG, "VI_BURST_REG"); break;
            case 0x04400018: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG"); break;
            case 0x0440001C: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG"); break;
            case 0x04400020: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_LEAP_REG, "VI_LEAP_REG"); break;
            case 0x04400024: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_H_START_REG, "VI_H_START_REG"); break;
            case 0x04400028: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_V_START_REG, "VI_V_START_REG"); break;
            case 0x0440002C: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG"); break;
            case 0x04400030: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG"); break;
            case 0x04400034: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG"); break;
            default:
                m_CodeBlock.Log("    should be moving %s in to %08X ?", CX86Ops::x86_Name(Reg), VAddr);
                if (BreakOnUnhandledMemory())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
        }
        break;
    case 0x04500000: // AI registers
        UpdateCounters(m_RegWorkingSet, false, true);

        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32(0xFFFFFFFF);
        m_Assembler.Push(Reg);
        m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
        m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][1], "AudioInterfaceHandler::Write32", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04600000:
        switch (PAddr)
        {
        case 0x04600000:
        case 0x04600004:
        case 0x04600008:
        case 0x0460000C:
        case 0x04600010:
            if (PAddr == 0x04600008 || PAddr == 0x0460000C)
            {
                UpdateCounters(m_RegWorkingSet, false, true);
            }
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.Push(Reg);
            m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_PeripheralInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_PeripheralInterfaceHandler)[0][1], "PeripheralInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600014:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", 0xFF);
            break;
        case 0x04600018:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", 0xFF);
            break;
        case 0x0460001C:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", 0xFF);
            break;
        case 0x04600020:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", 0xFF);
            break;
        case 0x04600024:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", 0xFF);
            break;
        case 0x04600028:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", 0xFF);
            break;
        case 0x0460002C:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", 0xFF);
            break;
        case 0x04600030:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG");
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", 0xFF);
            break;
        default:
            m_CodeBlock.Log("    should be moving %s in to %08X ?", CX86Ops::x86_Name(Reg), VAddr);
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04700000:
        switch (PAddr)
        {
        case 0x04700000: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->RI_MODE_REG, "RI_MODE_REG"); break;
        case 0x04700004: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG"); break;
        case 0x0470000C: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->RI_SELECT_REG, "RI_SELECT_REG"); break;
        case 0x04700010: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG"); break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x04800000:
        switch (PAddr)
        {
        case 0x04800000: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG"); break;
        case 0x04800004:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)(&g_MMU->m_PifRamHandler), AddressOf(&PifRamHandler::DMA_READ), "PifRamHandler::DMA_READ", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            m_Assembler.MoveX86regToVariable(Reg, &g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)(&g_MMU->m_PifRamHandler), AddressOf(&PifRamHandler::DMA_WRITE), "PifRamHandler::DMA_WRITE", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800018:
            m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_SI);
            m_Assembler.AndConstToVariable(&g_Reg->SI_STATUS_REG, "SI_STATUS_REG", (uint32_t)~SI_STATUS_INTERRUPT);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case 0x05000000:
        // 64DD registers
        if (EnableDisk())
        {
            switch (PAddr)
            {
            case 0x05000500: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->ASIC_DATA, "ASIC_DATA"); break;
            case 0x05000508:
            {
                // ASIC_CMD
                m_Assembler.MoveX86regToVariable(Reg, &g_Reg->ASIC_CMD, "ASIC_CMD");
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallFunc(AddressOf(&DiskCommand), "DiskCommand");
                m_RegWorkingSet.AfterCallDirect();
                break;
            }
            case 0x05000510:
            {
                // ASIC_BM_CTL
                m_Assembler.MoveX86regToVariable(Reg, &g_Reg->ASIC_BM_CTL, "ASIC_BM_CTL");
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallFunc(AddressOf(&DiskBMControl), "DiskBMControl");
                m_RegWorkingSet.AfterCallDirect();
                break;
            }
            case 0x05000518:
                break;
            case 0x05000520:
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallFunc(AddressOf(&DiskReset), "DiskReset");
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x05000528: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE"); break;
            case 0x05000530: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE"); break;
            case 0x05000548: m_Assembler.MoveX86regToVariable(Reg, &g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL"); break;
            }
            break;
        }
    case 0x13F00000:
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32(0xFFFFFFFF);
        m_Assembler.Push(Reg);
        m_Assembler.PushImm32(PAddr & 0x1FFFFFFF);
        m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_ISViewerHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_ISViewerHandler)[0][1], "ISViewerHandler::Write32", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x1FC00000:
        sprintf(VarName, "RDRAM + %X", PAddr);
        m_Assembler.MoveX86regToVariable(Reg, PAddr + g_MMU->Rdram(), VarName);
        break;
    default:
        if (PAddr >= 0x10000000 && PAddr < 0x13F00000)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32(0xFFFFFFFF);
            m_Assembler.Push(Reg);
            m_Assembler.PushImm32(PAddr);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][1], "RomMemoryHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else
        {
            m_CodeBlock.Log("    should be moving %s in to %08X ?", CX86Ops::x86_Name(Reg), VAddr);
            if (BreakOnUnhandledMemory())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
}

void CX86RecompilerOps::ResetMemoryStack()
{
    int32_t MipsReg = 29;
    if (IsConst(MipsReg))
    {
        m_Assembler.MoveConstToVariable(&_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], GetMipsRegLo(MipsReg));
    }
    else if (IsMapped(MipsReg))
    {
        m_Assembler.MoveX86regToVariable(GetMipsRegMapLo(MipsReg), &_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg]);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallThis((uint32_t)g_Recompiler, AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos", 4);
    m_RegWorkingSet.AfterCallDirect();
}

#endif