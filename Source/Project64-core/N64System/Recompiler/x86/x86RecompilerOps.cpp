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
#include <fenv.h>
#include <stdio.h>

uint32_t CX86RecompilerOps::m_RoundingModeValue = 0;
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

CX86RecompilerOps::CX86RecompilerOps(CMipsMemoryVM & MMU, CRegisters & Reg, CCodeBlock & CodeBlock) :
    CRecompilerOpsBase(MMU, Reg, CodeBlock),
    m_Assembler(CodeBlock),
    m_RegWorkingSet(CodeBlock, m_Assembler),
    m_CompilePC(0),
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
    }*/

    /*if (m_CompilePC >= 0x80000000 && m_CompilePC <= 0x80400000 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_CompilePC);
    if (g_SyncSystem) {
    #ifdef _WIN32
    m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.push((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(asmjit::x86::esp, 4);
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
            m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, (uint32_t)g_BaseSystem);
            m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
            m_Assembler.push((uint32_t)g_BaseSystem);
            m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
            m_Assembler.AddConstToX86Reg(asmjit::x86::esp, 4);
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
    m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.push((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(asmjit::x86::esp, 4);
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
    m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.push((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(asmjit::x86::esp, 4);
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
    m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, (uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    m_Assembler.push((uint32_t)g_BaseSystem);
    m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    m_Assembler.AddConstToX86Reg(asmjit::x86::esp, 4);
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
    if (!g_System->bFPURegCaching())
    {
        m_RegWorkingSet.UnMap_AllFPRs();
    }
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
            m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, (uint32_t)g_BaseSystem);
            m_Assembler.CallFunc(AddressOf(&CN64System::SyncSystemPC), "CN64System::SyncSystemPC");
            m_RegWorkingSet.AfterCallDirect();
        }
    }*/
}

void CX86RecompilerOps::CompileReadTLBMiss(uint32_t VirtualAddress, const asmjit::x86::Gp & LookUpReg)
{
    m_Assembler.MoveConstToVariable(g_TLBLoadAddress, "TLBLoadAddress", VirtualAddress);
    m_Assembler.CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_TLBReadMiss, false, &CX86Ops::JeLabel);
}

void CX86RecompilerOps::CompileReadTLBMiss(const asmjit::x86::Gp & AddressReg, const asmjit::x86::Gp & LookUpReg)
{
    m_Assembler.MoveX86regToVariable(g_TLBLoadAddress, "TLBLoadAddress", AddressReg);
    m_Assembler.CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_TLBReadMiss, false, &CX86Ops::JeLabel);
}

void CX86RecompilerOps::CompileWriteTLBMiss(const asmjit::x86::Gp & AddressReg, const asmjit::x86::Gp & LookUpReg)
{
    m_Assembler.MoveX86regToVariable(&g_TLBStoreAddress, "g_TLBStoreAddress", AddressReg);
    m_Assembler.CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_TLBWriteMiss, false, &CX86Ops::JeLabel);
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
        if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rs, true);
        }
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
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
    if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT)
    {
        Compile_BranchCompare(CompareType);
        if (m_Section->m_Jump.FallThrough || (!m_Section->m_Cont.FallThrough && m_Section->m_Jump.LinkLocation.isValid()))
        {
            LinkJump(m_Section->m_Jump);
            m_Assembler.MoveConstToVariable(&g_System->m_JumpDelayLocation, "System::m_JumpDelayLocation", m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 8);
            if (m_Section->m_Cont.LinkLocation.isValid())
            {
                // jump to link
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }

        if (m_Section->m_Cont.FallThrough)
        {
            LinkJump(m_Section->m_Cont);
            m_Assembler.MoveConstToVariable(&g_System->m_JumpDelayLocation, "System::m_JumpDelayLocation", m_CompilePC + 8);
            if (m_Section->m_Jump.LinkLocation.isValid())
            {
                // jump to link
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }

        if (m_Section->m_Jump.LinkLocation.isValid() || m_Section->m_Cont.LinkLocation.isValid())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        if (Link)
        {
            m_RegWorkingSet.Map_GPR_32bit(31, true, -1);
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(31), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
            m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(31), 4);
        }
        OverflowDelaySlot(false);
    }
    else if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
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
        if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT)
        {
            m_Section->m_Jump.TargetPC += 4;
            m_EffectDelaySlot = true;
        }
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Section_%d", m_Section->m_JumpSection->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = stdstr_f("Exit_%X_jump_%X", m_Section->m_EnterPC, m_Section->m_Jump.TargetPC);
        }
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
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
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();
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
            R4300iInstruction Instruction(m_CompilePC, m_Opcode.Value);
            uint32_t ReadReg1, ReadReg2;
            Instruction.ReadsGPR(ReadReg1, ReadReg2);

            if (ReadReg1 != 31 && ReadReg2 != 31)
            {
                m_RegWorkingSet.UnMap_GPR(31, false);
                m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
                m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_Section->m_Cont.LinkAddress = m_CompilePC + 8;
                m_Section->m_Jump.LinkAddress = m_CompilePC + 8;
            }
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
                if (m_Section->m_Jump.LinkLocation.isValid())
                {
                    LinkJump(m_Section->m_Jump);
                    m_Section->m_Jump.FallThrough = true;
                }
                else if (m_Section->m_Cont.LinkLocation.isValid())
                {
                    LinkJump(m_Section->m_Cont);
                    m_Section->m_Cont.FallThrough = true;
                }
            }
            if ((m_CompilePC & 0xFFC) == 0xFFC)
            {
                asmjit::Label DelayLinkLocation;
                if (m_Section->m_Jump.FallThrough)
                {
                    if (m_Section->m_Jump.LinkLocation.isValid() || m_Section->m_Jump.LinkLocation2.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Jump.TargetPC);
                }
                else if (m_Section->m_Cont.FallThrough)
                {
                    if (m_Section->m_Cont.LinkLocation.isValid() || m_Section->m_Cont.LinkLocation2.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Cont.TargetPC);
                }

                if (m_Section->m_Jump.LinkLocation.isValid() || m_Section->m_Jump.LinkLocation2.isValid())
                {
                    if (DelayLinkLocation.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    DelayLinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel("DoDelaySlot", DelayLinkLocation);

                    LinkJump(m_Section->m_Jump);
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Jump.TargetPC);
                }
                if (m_Section->m_Cont.LinkLocation.isValid() || m_Section->m_Cont.LinkLocation2.isValid())
                {
                    if (DelayLinkLocation.isValid())
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    DelayLinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel("DoDelaySlot", DelayLinkLocation);

                    LinkJump(m_Section->m_Cont);
                    m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_Section->m_Cont.TargetPC);
                }
                if (DelayLinkLocation.isValid())
                {
                    m_CodeBlock.Log("");
                    m_Assembler.bind(DelayLinkLocation);
                }
                OverflowDelaySlot(false);
                return;
            }
            m_RegWorkingSet.ResetX86Protection();
            m_RegBeforeDelay = m_RegWorkingSet;
        }
        if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
        }
        else
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
            m_Section->m_Cont.RegSet = m_RegWorkingSet;
            m_Section->GenerateSectionLinkage();
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
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
                m_RegWorkingSet.ResetX86Protection();
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
                        m_RegWorkingSet.ResetX86Protection();
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
                    FallInfo->LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(FallInfo->BranchLabel.c_str(), FallInfo->LinkLocation);

                    if (JumpInfo->LinkLocation.isValid())
                    {
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
                m_RegWorkingSet.ResetX86Protection();
                m_Section->m_Cont.RegSet = m_RegWorkingSet;
                m_Section->m_Jump.RegSet = m_RegWorkingSet;
                if (m_Section->m_Cont.LinkAddress != (uint32_t)-1)
                {
                    m_Section->m_Cont.RegSet.UnMap_GPR(31, false);
                    m_Section->m_Cont.RegSet.SetMipsRegLo(31, m_Section->m_Cont.LinkAddress);
                    m_Section->m_Cont.RegSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
                    m_Section->m_Cont.LinkAddress = (uint32_t)-1;
                }
                if (m_Section->m_Jump.LinkAddress != (uint32_t)-1)
                {
                    m_Section->m_Jump.RegSet.UnMap_GPR(31, false);
                    m_Section->m_Jump.RegSet.SetMipsRegLo(31, m_Section->m_Jump.LinkAddress);
                    m_Section->m_Jump.RegSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
                    m_Section->m_Jump.LinkAddress = (uint32_t)-1;
                }
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
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();
        if (Link)
        {
            R4300iInstruction Instruction(m_CompilePC, m_Opcode.Value);
            uint32_t ReadReg1, ReadReg2;
            Instruction.ReadsGPR(ReadReg1, ReadReg2);

            if (ReadReg1 != 31 && ReadReg2 != 31)
            {
                m_RegWorkingSet.UnMap_GPR(31, false);
                m_RegWorkingSet.SetMipsRegLo(31, m_CompilePC + 8);
                m_RegWorkingSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_Section->m_Cont.LinkAddress = m_CompilePC + 8;
                m_Section->m_Jump.LinkAddress = m_CompilePC + 8;
            }
        }

        Compile_BranchCompare(CompareType);
        m_RegWorkingSet.ResetX86Protection();

        m_Section->m_Cont.RegSet = m_RegWorkingSet;
        m_Section->m_Cont.RegSet.SetBlockCycleCount(m_Section->m_Cont.RegSet.GetBlockCycleCount() + g_System->CountPerOp());
        if (m_Section->m_Cont.LinkAddress != (uint32_t)-1)
        {
            m_Section->m_Cont.RegSet.UnMap_GPR(31, false);
            m_Section->m_Cont.RegSet.SetMipsRegLo(31, m_Section->m_Cont.LinkAddress);
            m_Section->m_Cont.RegSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
            m_Section->m_Cont.LinkAddress = (uint32_t)-1;
        }
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation.isValid())
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }

            if (m_Section->m_Jump.LinkLocation.isValid() || m_Section->m_Jump.FallThrough)
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
            if (m_Section->m_Jump.LinkAddress != (uint32_t)-1)
            {
                m_Section->m_Jump.RegSet.UnMap_GPR(31, false);
                m_Section->m_Jump.RegSet.SetMipsRegLo(31, m_Section->m_Jump.LinkAddress);
                m_Section->m_Jump.RegSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
                m_Section->m_Jump.LinkAddress = (uint32_t)-1;
            }
            m_Section->GenerateSectionLinkage();
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else
        {
            if (m_Section->m_Cont.FallThrough)
            {
                if (m_Section->m_Jump.LinkLocation.isValid())
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
        m_RegWorkingSet.ResetX86Protection();
        m_Section->m_Jump.RegSet = m_RegWorkingSet;
        m_Section->m_Jump.RegSet.SetBlockCycleCount(m_Section->m_Jump.RegSet.GetBlockCycleCount());
        if (m_Section->m_Jump.LinkAddress != (uint32_t)-1)
        {
            m_Section->m_Jump.RegSet.UnMap_GPR(31, false);
            m_Section->m_Jump.RegSet.SetMipsRegLo(31, m_Section->m_Jump.LinkAddress);
            m_Section->m_Jump.RegSet.SetMipsRegState(31, CRegInfo::STATE_CONST_32_SIGN);
            m_Section->m_Jump.LinkAddress = (uint32_t)-1;
        }
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
    asmjit::Label Jump;

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                CX86RecompilerOps::UnknownOpcode();
            }
            else if (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) != m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt))
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
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                m_Assembler.cmp(
                    m_RegWorkingSet.Is32Bit(m_Opcode.rs) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false) : m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs),
                    m_RegWorkingSet.Is32Bit(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false) : m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt));

                if (m_Section->m_Jump.FallThrough)
                {
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JneLabel("Continue", Jump);
                }
                else
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
                }
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(ConstReg) || m_RegWorkingSet.Is64Bit(MappedReg))
            {
                if (m_RegWorkingSet.Is32Bit(ConstReg) || m_RegWorkingSet.Is32Bit(MappedReg))
                {
                    m_RegWorkingSet.ProtectGPR(MappedReg);
                    if (m_RegWorkingSet.Is32Bit(MappedReg))
                    {
                        m_Assembler.CompConstToX86reg(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, MappedReg, true, false), m_RegWorkingSet.GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(MappedReg), m_RegWorkingSet.GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(MappedReg), m_RegWorkingSet.GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Jump.FallThrough)
                {
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JneLabel("Continue", Jump);
                }
                else
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
                }
            }
            else
            {
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rs) || m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!b32BitCore())
        {
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], m_RegWorkingSet.GetMipsRegHi(KnownReg));
                }
                else if (m_RegWorkingSet.IsSigned(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], (m_RegWorkingSet.GetMipsRegLo_S(KnownReg) >> 31));
                }
                else
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            else
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapHi(KnownReg), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (m_RegWorkingSet.IsSigned(KnownReg))
                {
                    m_RegWorkingSet.ProtectGPR(KnownReg);
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, KnownReg, true, false), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            if (m_Section->m_Jump.FallThrough)
            {
                Jump = m_Assembler.newLabel();
                m_Assembler.JneLabel("Continue", Jump);
            }
            else
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }
        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
        }
        else
        {
            m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapLo(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            if (b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);

            if (Jump.isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump);
            }
        }
        else
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            if (b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
            }
        }
    }
    else
    {
        asmjit::x86::Gp Reg;

        if (!b32BitCore())
        {
            Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                Jump = m_Assembler.newLabel();
                m_Assembler.JneLabel("Continue", Jump);
            }
            else
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }

        Reg = m_RegWorkingSet.Map_TempReg(Reg, m_Opcode.rt, false, false);
        m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        if (m_Section->m_Cont.FallThrough)
        {
            if (b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            if (Jump.isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump);
            }
        }
        else
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            if (b32BitCore())
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
            }
        }
    }
}

void CX86RecompilerOps::BEQ_Compare()
{
    asmjit::Label Jump;

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt))
            {
                CX86RecompilerOps::UnknownOpcode();
            }
            else if (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) == m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt))
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
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            if ((m_RegWorkingSet.Is64Bit(m_Opcode.rs) || m_RegWorkingSet.Is64Bit(m_Opcode.rt)) && !b32BitCore())
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);

                m_Assembler.cmp(
                    m_RegWorkingSet.Is32Bit(m_Opcode.rs) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false) : m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs),
                    m_RegWorkingSet.Is32Bit(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false) : m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JneLabel("Continue", Jump);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(ConstReg) || m_RegWorkingSet.Is64Bit(MappedReg))
            {
                if (m_RegWorkingSet.Is32Bit(ConstReg) || m_RegWorkingSet.Is32Bit(MappedReg))
                {
                    if (m_RegWorkingSet.Is32Bit(MappedReg))
                    {
                        m_RegWorkingSet.ProtectGPR(MappedReg);
                        m_Assembler.CompConstToX86reg(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, MappedReg, true, false), m_RegWorkingSet.GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(MappedReg), m_RegWorkingSet.GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(MappedReg), m_RegWorkingSet.GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Cont.FallThrough)
                {
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JneLabel("Continue", Jump);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
            else
            {
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                    m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rs) || m_RegWorkingSet.IsKnown(m_Opcode.rt))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (!b32BitCore())
        {
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], m_RegWorkingSet.GetMipsRegHi(KnownReg));
                }
                else if (m_RegWorkingSet.IsSigned(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], m_RegWorkingSet.GetMipsRegLo_S(KnownReg) >> 31);
                }
                else
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            else
            {
                m_RegWorkingSet.ProtectGPR(KnownReg);
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapHi(KnownReg), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (m_RegWorkingSet.IsSigned(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, KnownReg, true, false), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], 0);
                }
            }
            if (m_Section->m_Cont.FallThrough)
            {
                Jump = m_Assembler.newLabel();
                m_Assembler.JneLabel("Continue", Jump);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
        }
        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
        }
        else
        {
            m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapLo(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            if (Jump.isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            if (b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
            }
        }
        else
        {
            m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
            m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
    else
    {
        asmjit::x86::Gp Reg;
        if (!b32BitCore())
        {
            Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
            m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
            if (m_Section->m_Cont.FallThrough)
            {
                Jump = m_Assembler.newLabel();
                m_Assembler.JneLabel("Continue", Jump);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
        }
        m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(Reg, m_Opcode.rs, false, false), &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            if (Jump.isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            if (b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
            }
        }
        else
        {
            if (b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
            }
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
}

void CX86RecompilerOps::BGTZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            if (m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) > 0)
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
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) > 0)
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
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
    {
        m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), 0);
        if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JleLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JgLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JleLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
    else if (m_RegWorkingSet.IsUnknown(m_Opcode.rs) && b32BitCore())
    {
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JleLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JgLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JleLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
    else
    {
        asmjit::Label Jump;

        if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs), 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            Jump = m_Assembler.newLabel();
            m_Assembler.JgLabel("Continue", Jump);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            Jump = m_Assembler.newLabel();
            m_Assembler.JlLabel("Continue", Jump);
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JgLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JgLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }

        if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
            m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
            m_Assembler.bind(Jump);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            m_Assembler.bind(Jump);
        }
        else
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
        }
    }
}

void CX86RecompilerOps::BLEZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            if (m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) <= 0)
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
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) <= 0)
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
            if (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) == 0)
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
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is32Bit(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JleLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }
        else
        {
            asmjit::Label Jump;

            if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs), 0);
            }
            else
            {
                m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                Jump = m_Assembler.newLabel();
                m_Assembler.JlLabel("Continue", Jump);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                Jump = m_Assembler.newLabel();
                m_Assembler.JgLabel("Continue", Jump);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }

            if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), 0);
            }
            else
            {
                m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                m_Assembler.bind(Jump);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
                m_Assembler.bind(Jump);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JmpLabel("BranchToJump", m_Section->m_Jump.LinkLocation2);
            }
        }
    }
    else
    {
        asmjit::Label Jump;

        if (!b32BitCore())
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                Jump = m_Assembler.newLabel();
                m_Assembler.JlLabel("Continue", Jump);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                Jump = m_Assembler.newLabel();
                m_Assembler.JgLabel("Continue", Jump);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
            if (m_Section->m_Jump.FallThrough)
            {
                if (b32BitCore())
                {
                    m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                }
                else
                {
                    m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                    m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                }
                if (Jump.isValid())
                {
                    m_Assembler.bind(Jump);
                }
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation2);
                if (Jump.isValid())
                {
                    m_Assembler.bind(Jump);
                }
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation2);
                m_Section->m_Jump.LinkLocation2 = m_Assembler.newLabel();
                m_Assembler.JmpLabel("BranchToJump", m_Section->m_Jump.LinkLocation2);
            }
        }
        else
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JleLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }
    }
}

void CX86RecompilerOps::BLTZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            if (m_RegWorkingSet.GetMipsReg_S(m_Opcode.rs) < 0)
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
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < 0)
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
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }
        else
        {
            m_Section->m_Jump.FallThrough = false;
            m_Section->m_Cont.FallThrough = true;
        }
    }
    else if (m_RegWorkingSet.IsUnknown(m_Opcode.rs))
    {
        if (b32BitCore())
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JgeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JlLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
    }
}

void CX86RecompilerOps::BGEZ_Compare()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            CX86RecompilerOps::UnknownOpcode();
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) >= 0)
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
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JgeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            }
            else
            {
                m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
                m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
                m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
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
        if (b32BitCore())
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], 0);
        }
        else
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], 0);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JgeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        }
        else
        {
            m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JlLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
            m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
            m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
        }
    }
}

void CX86RecompilerOps::COP1_BCF_Compare()
{
    m_Assembler.TestVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", FPCSR_C);
    if (m_Section->m_Cont.FallThrough)
    {
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JeLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
    }
    else
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JneLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
}

void CX86RecompilerOps::COP1_BCT_Compare()
{
    m_Assembler.TestVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", FPCSR_C);
    if (m_Section->m_Cont.FallThrough)
    {
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JneLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
    }
    else
    {
        m_Section->m_Cont.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JeLabel(m_Section->m_Cont.BranchLabel.c_str(), m_Section->m_Cont.LinkLocation);
        m_Section->m_Jump.LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(m_Section->m_Jump.BranchLabel.c_str(), m_Section->m_Jump.LinkLocation);
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
        R4300iOpcode DelaySlot;
        g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value);
        if (R4300iInstruction(m_CompilePC + 4, DelaySlot.Value).HasDelaySlot())
        {
            m_Assembler.MoveConstToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2));
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
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
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
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
        m_RegWorkingSet.Map_GPR_32bit(31, true, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(31), &m_Reg.m_PROGRAM_COUNTER, "_PROGRAM_COUNTER");
        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(31), 0xF0000000);
        m_Assembler.AddConstToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(31), (m_CompilePC + 8) & ~0xF0000000);
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
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
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

            asmjit::x86::Gp PCReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(PCReg, &m_Reg.m_PROGRAM_COUNTER, "_PROGRAM_COUNTER");
            m_Assembler.and_(PCReg, 0xF0000000);
            m_Assembler.AddConstToX86Reg(PCReg, (m_Opcode.target << 2));
            m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "_PROGRAM_COUNTER", PCReg);

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
        m_Assembler.AddConstToX86Reg(m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true), (int16_t)m_Opcode.immediate);
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int32_t rs = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs);
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
            if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, sum);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        }
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        m_Assembler.AddConstToX86Reg(Reg, (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rt != 0)
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Reg);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        m_RegWorkingSet.ResetX86Protection();
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
            m_Assembler.AddConstToX86Reg(m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true), (int16_t)m_Opcode.immediate);
        }
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) + (int16_t)m_Opcode.immediate);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        m_Assembler.AddConstToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        m_RegWorkingSet.ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SLTIU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t Result = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) < ((unsigned)((int64_t)((int16_t)m_Opcode.immediate))) ? 1 : 0 : m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) < ((unsigned)((int16_t)m_Opcode.immediate)) ? 1
                                                                                                                                                                                                                                                                : 0;
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            asmjit::Label Jump[2];

            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            Jump[0] = m_Assembler.newLabel();
            m_Assembler.JeLabel("Low Compare", Jump[0]);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            Jump[1] = m_Assembler.newLabel();
            m_Assembler.JmpLabel("Continue", Jump[1]);
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[0]);
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[1]);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
    }
    else if (b32BitCore())
    {
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
    }
    else
    {
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], ((int16_t)m_Opcode.immediate >> 31));
        asmjit::Label Jump = m_Assembler.newLabel();
        m_Assembler.JneLabel("CompareSet", Jump);
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);
        m_CodeBlock.Log("");
        m_Assembler.bind(Jump);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::SLTI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t Result = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? ((int64_t)m_RegWorkingSet.GetMipsReg(m_Opcode.rs) < (int64_t)((int16_t)m_Opcode.immediate) ? 1 : 0) : (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < (int16_t)m_Opcode.immediate ? 1 : 0);

        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            asmjit::Label Jump[2];

            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            Jump[0] = m_Assembler.newLabel();
            m_Assembler.JeLabel("Low Compare", Jump[0]);
            m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            Jump[1] = m_Assembler.newLabel();
            m_Assembler.JmpLabel("Continue", Jump[1]);
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[0]);
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[1]);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);

            if (m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) != asmjit::x86::eax && m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) != asmjit::x86::ebx)
            {
                m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.setl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), 1);
            }
        }
    }
    else if (b32BitCore())
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);

        if (m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) != asmjit::x86::eax && m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) != asmjit::x86::ebx)
        {
            m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_Assembler.setl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
            m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), 1);
        }
    }
    else
    {
        asmjit::Label Jump[2];
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], ((int16_t)m_Opcode.immediate >> 31));
        Jump[0] = m_Assembler.newLabel();
        m_Assembler.JeLabel("Low Compare", Jump[0]);
        m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
        Jump[1] = m_Assembler.newLabel();
        m_Assembler.JmpLabel("Continue", Jump[1]);
        m_CodeBlock.Log("");
        m_Assembler.bind(Jump[0]);
        m_Assembler.CompConstToVariable(&m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], (int16_t)m_Opcode.immediate);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1].isValid())
        {
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[1]);
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::ANDI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & m_Opcode.immediate);
    }
    else if (m_Opcode.immediate != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.rs);
        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, 0);
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
        m_Assembler.or_(m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true), m_Opcode.immediate);
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
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, m_RegWorkingSet.IsSigned(m_Opcode.rs), m_Opcode.rs);
            }
        }
        m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
    }
    else
    {
        if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        }
        m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
    }

    if (g_System->bFastSP() && m_Opcode.rt == 29 && m_Opcode.rs != 29)
    {
        m_RegWorkingSet.ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::XORI()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_Opcode.rs != m_Opcode.rt)
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, m_RegWorkingSet.GetMipsRegState(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs));
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) ^ m_Opcode.immediate);
    }
    else
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is32Bit(m_Opcode.rs))
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, m_RegWorkingSet.IsSigned(m_Opcode.rs), m_Opcode.rs);
        }
        else if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rs);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rs);
        }
        if (m_Opcode.immediate != 0)
        {
            m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
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
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true, false);
        uint32_t Address;

        m_MMU.VAddrToPAddr(((int16_t)m_Opcode.offset << 16), Address);
        if (!Reg.isValid())
        {
            m_Assembler.MoveConstToVariable(&(g_Recompiler->MemoryStackPos()), "MemoryStack", (uint32_t)(Address + g_MMU->Rdram()));
        }
        else
        {
            m_Assembler.MoveConstToX86reg(Reg, (uint32_t)(Address + g_MMU->Rdram()));
        }
    }

    m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, ((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
}

void CX86RecompilerOps::DADDI()
{
    int64_t imm = (int64_t)((int16_t)m_Opcode.immediate);

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int64_t rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
        int64_t sum = rs + imm;
        if ((~(rs ^ imm) & (rs ^ sum)) & 0x8000000000000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rt != 0)
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
            }
            m_RegWorkingSet.SetMipsReg(m_Opcode.rt, sum);
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == -1)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == 0)
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
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
        asmjit::x86::Gp RegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        asmjit::x86::Gp RegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);

        m_Assembler.add(RegLo, (uint32_t)imm);
        m_Assembler.adc(RegHi, (uint32_t)(imm >> 32));
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rt != 0)
        {
            m_RegWorkingSet.UnProtectGPR(m_Opcode.rs);
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), RegLo);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), RegHi);
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

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int64_t rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, false);
        }
        m_RegWorkingSet.SetMipsReg(m_Opcode.rt, rs + imm);
        if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rt) == 0)
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
        m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
        asmjit::x86::Gp RegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        asmjit::x86::Gp RegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);

        m_Assembler.add(RegLo, (uint32_t)imm);
        m_Assembler.adc(RegHi, (uint32_t)(imm >> 32));
        if (m_Opcode.rt != 0)
        {
            m_RegWorkingSet.UnProtectGPR(m_Opcode.rs);
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), RegLo);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), RegHi);
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
        m_Assembler.push(0x20);
        if (m_RegWorkingSet.IsConst(m_Opcode.base))
        {
            uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            m_Assembler.PushImm32("Address", Address);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.base))
        {
            m_Assembler.AddConstToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            m_Assembler.push(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.base));
        }
        else
        {
            m_Assembler.MoveVariableToX86reg(asmjit::x86::eax, &m_Reg.m_GPR[m_Opcode.base].UW[0], CRegName::GPR_Lo[m_Opcode.base]);
            m_Assembler.AddConstToX86Reg(asmjit::x86::eax, (int16_t)m_Opcode.offset);
            m_Assembler.push(asmjit::x86::eax);
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
        m_RegWorkingSet.UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
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
        m_RegWorkingSet.UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::LDR, "R4300iOp::LDR");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::LB_KnownAddress(const asmjit::x86::Gp & Reg, uint32_t VAddr, bool SignExtend)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileLoadMemoryValue(AddressReg, Reg, x86Reg_Unknown, 8, SignExtend);
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
                m_Assembler.MoveZxVariableToX86regByte(Reg, (PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 3)", PAddr).c_str());
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
                m_Assembler.MoveZxVariableToX86regByte(Reg, ((PAddr ^ 3) - 0x04000000) + g_MMU->Dmem(), stdstr_f("Dmem + (%X ^ 3)", (PAddr - 0x04000000)).c_str());
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
                m_Assembler.MoveZxVariableToX86regByte(Reg, ((PAddr ^ 3) - 0x04001000) + g_MMU->Imem(), stdstr_f("Imem + (%X ^ 3)", (PAddr - 0x04001000)).c_str());
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
            m_Assembler.push(((PAddr + 2) & ~0x3) & 0x1FFFFFFC);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][0], "RomMemoryHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            uint8_t Shift = (((PAddr & 1) ^ 3) << 3);
            if (Shift == 0x10)
            {
                m_Assembler.shl(Reg, 0x8);
            }
            if (SignExtend)
            {
                m_Assembler.sar(Reg, 0x18);
            }
            else
            {
                m_Assembler.shr(Reg, 0x18);
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

void CX86RecompilerOps::LH_KnownAddress(const asmjit::x86::Gp & Reg, uint32_t VAddr, bool SignExtend)
{
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileLoadMemoryValue(AddressReg, Reg, x86Reg_Unknown, 16, SignExtend);
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
                m_Assembler.MoveSxVariableToX86regHalf(Reg, (PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str());
            }
            else
            {
                m_Assembler.MoveZxVariableToX86regHalf(Reg, (PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str());
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
            m_Assembler.push(((PAddr + 2) & ~0x3) & 0x1FFFFFFC);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][0], "RomMemoryHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            if (SignExtend)
            {
                m_Assembler.sar(Reg, 16);
            }
            else
            {
                m_Assembler.shr(Reg, 16);
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

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP8(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        LB_KnownAddress(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address, true);
        return;
    }
    PreReadInstruction();
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    CompileLoadMemoryValue(x86Reg_Unknown, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), x86Reg_Unknown, 8, true);
}

void CX86RecompilerOps::LH()
{
    if (m_Opcode.rt == 0) return;

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP16(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
        LH_KnownAddress(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address, true);
        return;
    }
    PreReadInstruction();
    CompileLoadMemoryValue(x86Reg_Unknown, x86Reg_Unknown, x86Reg_Unknown, 16, true);
}

void CX86RecompilerOps::LWL()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        uint32_t Offset = Address & 3;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        asmjit::x86::Gp Value = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), R4300iOp::LWL_MASK[Offset]);
        m_Assembler.shl(Value, (uint8_t)R4300iOp::LWL_SHIFT[Offset]);
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Value);
    }
    else
    {
        PreReadInstruction();
        asmjit::x86::Gp shift = m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1, false, false);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        asmjit::x86::Gp AddressReg = BaseOffsetAddress(false);
        asmjit::x86::Gp OffsetReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.mov(OffsetReg, AddressReg);
        m_Assembler.and_(OffsetReg, 3);
        m_Assembler.and_(AddressReg, (uint32_t)~3);
        TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
        CompileLoadMemoryValue(AddressReg, AddressReg, x86Reg_Unknown, 32, false);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        m_Assembler.AndVariableDispToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), (void *)R4300iOp::LWL_MASK, "LWL_MASK", OffsetReg, CX86Ops::Multip_x4);
        m_Assembler.MoveVariableDispToX86Reg(shift, (void *)R4300iOp::LWL_SHIFT, "LWL_SHIFT", OffsetReg, CX86Ops::Multip_x4);
        m_Assembler.shl(AddressReg, asmjit::x86::cl);
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), AddressReg);
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
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true);
        m_Assembler.MoveVariableDispToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), (void *)((uint32_t)(int16_t)m_Opcode.offset), stdstr_f("%Xh", (int16_t)m_Opcode.offset).c_str(), TempReg1, CX86Ops::Multip_x1);
        if (bRecordLLBit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        LW_KnownAddress(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address);
        if (bRecordLLBit)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        PreReadInstruction();
        CompileLoadMemoryValue(x86Reg_Unknown, x86Reg_Unknown, x86Reg_Unknown, 32, false);
        if (bRecordLLBit)
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LLBit, "LLBit", 1);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        m_RegWorkingSet.ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::LW_KnownAddress(const asmjit::x86::Gp & Reg, uint32_t VAddr)
{
    m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileLoadMemoryValue(AddressReg, Reg, x86Reg_Unknown, 32, true);
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
                    m_Assembler.push(PAddr | 0xA0000000);
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
            m_Assembler.push(PAddr | 0xA0000000);
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
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][0], "VideoInterfaceHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            break;
        case 0x04500000:
            UpdateCounters(m_RegWorkingSet, false, true, false);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][0], "AudioInterfaceHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            break;
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
            UpdateCounters(m_RegWorkingSet, false, true, false);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_PifRamHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_PifRamHandler)[0][0], "PifRamHandler::Read32", 16);
            m_RegWorkingSet.AfterCallDirect();
            m_Assembler.MoveVariableToX86reg(Reg, &m_TempValue32, "m_TempValue32");
            break;
        default:
            if ((PAddr & 0xF0000000) == 0x10000000 && (PAddr - 0x10000000) < g_Rom->GetRomSize())
            {
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
                m_Assembler.push(PAddr & 0x1FFFFFFF);
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

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP8(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        LB_KnownAddress(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address, false);
        return;
    }
    PreReadInstruction();
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    CompileLoadMemoryValue(x86Reg_Unknown, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), x86Reg_Unknown, 8, false);
}

void CX86RecompilerOps::LHU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveReadBP() && g_Debugger->ReadBP16(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
        LH_KnownAddress(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address, false);
        return;
    }
    PreReadInstruction();
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, m_Opcode.base == m_Opcode.rt ? m_Opcode.rt : -1);
    CompileLoadMemoryValue(x86Reg_Unknown, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), x86Reg_Unknown, 16, false);
}

void CX86RecompilerOps::LWR()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        uint32_t Offset = Address & 3;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        asmjit::x86::Gp Value = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), R4300iOp::LWR_MASK[Offset]);
        m_Assembler.shr(Value, (uint8_t)R4300iOp::LWR_SHIFT[Offset]);
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Value);
    }
    else
    {
        PreReadInstruction();
        asmjit::x86::Gp shift = m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1, false, false);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        asmjit::x86::Gp AddressReg = BaseOffsetAddress(false);
        asmjit::x86::Gp OffsetReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.mov(OffsetReg, AddressReg);
        m_Assembler.and_(OffsetReg, 3);
        m_Assembler.and_(AddressReg, (uint32_t)~3);
        TestReadBreakpoint(AddressReg, (uint32_t)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
        CompileLoadMemoryValue(AddressReg, AddressReg, x86Reg_Unknown, 32, false);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
        m_Assembler.AndVariableDispToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), (void *)R4300iOp::LWR_MASK, "LWR_MASK", OffsetReg, CX86Ops::Multip_x4);
        m_Assembler.MoveVariableDispToX86Reg(shift, (void *)R4300iOp::LWR_SHIFT, "LWR_SHIFT", OffsetReg, CX86Ops::Multip_x4);
        m_Assembler.shr(AddressReg, asmjit::x86::cl);
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), AddressReg);
    }
}

void CX86RecompilerOps::LWU()
{
    LW(false, false);
}

void CX86RecompilerOps::SB()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveWriteBP() && g_Debugger->WriteBP8(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            SB_Const(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), Address);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_Assembler.Is8BitReg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt)))
        {
            SB_Register(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SB_Register(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, true), Address);
        }
        return;
    }
    PreWriteInstruction();

    asmjit::x86::Gp ValueReg;
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        ValueReg = m_RegWorkingSet.IsUnknown(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, true) : m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && !m_Assembler.Is8BitReg(ValueReg))
        {
            m_RegWorkingSet.UnProtectGPR(m_Opcode.rt);
            ValueReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, true);
        }
    }
    CompileStoreMemoryValue(x86Reg_Unknown, ValueReg, x86Reg_Unknown, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), 8);
}

void CX86RecompilerOps::SH()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset);
        if (HaveWriteBP() && g_Debugger->WriteBP16(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            SH_Const(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), Address);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            SH_Register(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SH_Register(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false), Address);
        }
        return;
    }

    PreWriteInstruction();

    asmjit::x86::Gp ValueReg;
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        ValueReg = m_RegWorkingSet.IsUnknown(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false) : m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
    }
    CompileStoreMemoryValue(x86Reg_Unknown, ValueReg, x86Reg_Unknown, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), 16);
}

void CX86RecompilerOps::SWL()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address;

        Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        uint32_t Offset = Address & 3;

        asmjit::x86::Gp Value = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.and_(Value, R4300iOp::SWL_MASK[Offset]);
        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
        m_Assembler.shr(TempReg1, (uint8_t)R4300iOp::SWL_SHIFT[Offset]);
        m_Assembler.add(Value, TempReg1);
        SW_Register(Value, (Address & ~3));
        return;
    }
    PreWriteInstruction();
    asmjit::x86::Gp shift = m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1, false, false), AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");

    asmjit::x86::Gp TempReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    asmjit::x86::Gp OffsetReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.mov(TempReg2, AddressReg);
    m_Assembler.shr(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(TempReg2, g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, CX86Ops::Multip_x4);
    m_Assembler.CompConstToX86reg(TempReg2, (uint32_t)-1);
    asmjit::Label JumpFound = m_Assembler.newLabel();
    m_Assembler.JneLabel(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), JumpFound);
    m_Assembler.mov(TempReg2, AddressReg);
    m_Assembler.shr(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(TempReg2, g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, CX86Ops::Multip_x4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    m_Assembler.AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    m_CodeBlock.Log("");
    m_Assembler.bind(JumpFound);
    m_Assembler.mov(OffsetReg, AddressReg);
    m_Assembler.and_(OffsetReg, 3);
    m_Assembler.and_(AddressReg, (uint32_t)~3);
    m_Assembler.mov(ValueReg, asmjit::x86::dword_ptr(AddressReg, TempReg2));

    m_Assembler.AndVariableDispToX86Reg(ValueReg, (void *)R4300iOp::SWL_MASK, "R4300iOp::SWL_MASK", OffsetReg, CX86Ops::Multip_x4);
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt) || m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) != 0)
    {
        m_Assembler.MoveVariableDispToX86Reg(shift, (void *)R4300iOp::SWL_SHIFT, "R4300iOp::SWL_SHIFT", OffsetReg, CX86Ops::Multip_x4);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.MoveConstToX86reg(OffsetReg, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.mov(OffsetReg, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.MoveVariableToX86reg(OffsetReg, &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_Assembler.shr(OffsetReg, asmjit::x86::cl);
        m_Assembler.add(ValueReg, OffsetReg);
    }

    CompileStoreMemoryValue(AddressReg, ValueReg, x86Reg_Unknown, 0, 32);
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
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true);

        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg1, (uint32_t)((int16_t)m_Opcode.offset)), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg1, (uint32_t)((int16_t)m_Opcode.offset)), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            asmjit::x86::Gp TempReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg1, (uint32_t)((int16_t)m_Opcode.offset)), TempReg2);
        }
    }
    else
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.base))
        {
            uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
            {
                FoundMemoryBreakpoint();
                return;
            }

            if (bCheckLLbit)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            if (m_RegWorkingSet.IsConst(m_Opcode.rt))
            {
                SW_Const(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), Address);
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
                SW_Register(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address);
            }
            else
            {
                SW_Register(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false), Address);
            }
            return;
        }

        PreWriteInstruction();
        asmjit::Label JumpLLBit;
        if (bCheckLLbit)
        {
            m_Assembler.CompConstToVariable(&m_Reg.m_LLBit, "_LLBit", 1);
            JumpLLBit = m_Assembler.newLabel();
            m_Assembler.JneLabel("LLBit_Continue", JumpLLBit);
        }

        asmjit::x86::Gp ValueReg;
        if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            }
            ValueReg = m_RegWorkingSet.IsUnknown(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false) : m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
        }
        CompileStoreMemoryValue(x86Reg_Unknown, ValueReg, x86Reg_Unknown, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), 32);
        if (bCheckLLbit)
        {
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpLLBit);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, false, -1);
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_Reg.m_LLBit, "_LLBit");
        }
    }
}

void CX86RecompilerOps::SWR()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        uint32_t Offset = Address & 3;

        asmjit::x86::Gp Value = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        LW_KnownAddress(Value, (Address & ~3));
        m_Assembler.and_(Value, R4300iOp::SWR_MASK[Offset]);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
        m_Assembler.shl(TempReg, (uint8_t)R4300iOp::SWR_SHIFT[Offset]);
        m_Assembler.add(Value, TempReg);
        SW_Register(Value, (Address & ~3));
        return;
    }
    PreWriteInstruction();
    asmjit::x86::Gp shift = m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1, false, false);
    asmjit::x86::Gp AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (uint32_t)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");
    asmjit::x86::Gp TempReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    asmjit::x86::Gp OffsetReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.mov(TempReg2, AddressReg);
    m_Assembler.shr(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(TempReg2, g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, CX86Ops::Multip_x4);
    m_Assembler.CompConstToX86reg(TempReg2, (uint32_t)-1);
    asmjit::Label JumpFound = m_Assembler.newLabel();
    m_Assembler.JneLabel(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), JumpFound);
    m_Assembler.mov(TempReg2, AddressReg);
    m_Assembler.shr(TempReg2, 12);
    m_Assembler.MoveVariableDispToX86Reg(TempReg2, g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, CX86Ops::Multip_x4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    m_Assembler.AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    m_CodeBlock.Log("");
    m_Assembler.bind(JumpFound);

    m_Assembler.mov(OffsetReg, AddressReg);
    m_Assembler.and_(OffsetReg, 3);
    m_Assembler.and_(AddressReg, (uint32_t)~3);

    m_Assembler.mov(ValueReg, asmjit::x86::dword_ptr(AddressReg, TempReg2));

    m_Assembler.AndVariableDispToX86Reg(ValueReg, (void *)R4300iOp::SWR_MASK, "R4300iOp::SWR_MASK", OffsetReg, CX86Ops::Multip_x4);
    if (!m_RegWorkingSet.IsConst(m_Opcode.rt) || m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) != 0)
    {
        m_Assembler.MoveVariableDispToX86Reg(shift, (void *)R4300iOp::SWR_SHIFT, "R4300iOp::SWR_SHIFT", OffsetReg, CX86Ops::Multip_x4);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.MoveConstToX86reg(OffsetReg, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.mov(OffsetReg, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.MoveVariableToX86reg(OffsetReg, &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_Assembler.shl(OffsetReg, asmjit::x86::cl);
        m_Assembler.add(ValueReg, OffsetReg);
    }

    CompileStoreMemoryValue(AddressReg, ValueReg, x86Reg_Unknown, 0, 32);
}

void CX86RecompilerOps::SDL()
{
    if (m_Opcode.base != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
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
        m_RegWorkingSet.UnMap_GPR(m_Opcode.base, true);
    }

    if (m_Opcode.rt != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
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
        if (m_RegWorkingSet.RegInStack(m_Opcode.ft - 1, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.ft - 1, CRegInfo::FPU_Qword))
        {
            m_RegWorkingSet.UnMap_FPR(m_Opcode.ft - 1, true);
        }
    }
    if (m_RegWorkingSet.RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.ft, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, true);
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, false);
    }
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        LW_KnownAddress(TempReg1, Address);

        asmjit::x86::Gp TempReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg2, &m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg2), TempReg1);
        return;
    }
    PreReadInstruction();
    asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    CompileLoadMemoryValue(x86Reg_Unknown, ValueReg, x86Reg_Unknown, 32, false);
    asmjit::x86::Gp FPR_SPtr = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(FPR_SPtr, &m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(asmjit::x86::dword_ptr(FPR_SPtr), ValueReg);
}

void CX86RecompilerOps::LDC1()
{
    CompileCop1Test();

    m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, false);
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP64(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        LW_KnownAddress(TempReg1, Address);

        asmjit::x86::Gp TempReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg2, &m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.AddConstToX86Reg(TempReg2, 4);
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg2), TempReg1);

        LW_KnownAddress(TempReg1, Address + 4);
        m_Assembler.MoveVariableToX86reg(TempReg2, &m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg2), TempReg1);
    }
    else
    {
        PreReadInstruction();
        m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, true);

        asmjit::x86::Gp ValueRegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false), ValueRegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        CompileLoadMemoryValue(x86Reg_Unknown, ValueRegLo, ValueRegHi, 64, false);

        asmjit::x86::Gp FPR_DPtr = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(FPR_DPtr, &m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(asmjit::x86::dword_ptr(FPR_DPtr), ValueRegLo);
        m_Assembler.AddConstToX86Reg(FPR_DPtr, 4);
        m_Assembler.mov(asmjit::x86::dword_ptr(FPR_DPtr), ValueRegHi);
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
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
        asmjit::x86::Gp StackReg = m_RegWorkingSet.Map_MemoryStack(x86Reg_Unknown, true);
        m_Assembler.MoveVariableDispToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), (void *)((uint32_t)(int16_t)m_Opcode.offset), stdstr_f("%Xh", (int16_t)m_Opcode.offset).c_str(), StackReg, CX86Ops::Multip_x1);
        m_Assembler.MoveVariableDispToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), (void *)((uint32_t)(int16_t)m_Opcode.offset + 4), stdstr_f("%Xh", (int16_t)m_Opcode.offset + 4).c_str(), StackReg, CX86Ops::Multip_x1);
    }
    else if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveReadBP() && g_Debugger->ReadBP64(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
        LW_KnownAddress(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), Address);
        LW_KnownAddress(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address + 4);
        if (g_System->bFastSP() && m_Opcode.rt == 29)
        {
            ResetMemoryStack();
        }
    }
    else
    {
        PreReadInstruction();
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }

        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, m_Opcode.rt == m_Opcode.base ? m_Opcode.base : -1);
        CompileLoadMemoryValue(x86Reg_Unknown, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), 64, false);
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        m_RegWorkingSet.ResetX86Protection();
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

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, true);
        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg1, &m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(TempReg1, asmjit::x86::dword_ptr(TempReg1));
        SW_Register(TempReg1, Address);
        return;
    }
    PreWriteInstruction();
    m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, true);
    asmjit::x86::Gp ValueReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(ValueReg, &m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(ValueReg, asmjit::x86::dword_ptr(ValueReg));

    CompileStoreMemoryValue(x86Reg_Unknown, ValueReg, x86Reg_Unknown, 0, 32);
}

void CX86RecompilerOps::SDC1()
{
    CompileCop1Test();

    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg1, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.AddConstToX86Reg(TempReg1, 4);
        m_Assembler.mov(TempReg1, asmjit::x86::dword_ptr(TempReg1));
        SW_Register(TempReg1, Address);

        m_Assembler.MoveVariableToX86reg(TempReg1, &m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_Assembler.mov(TempReg1, asmjit::x86::dword_ptr(TempReg1));
        SW_Register(TempReg1, Address + 4);
        return;
    }
    PreWriteInstruction();
    m_RegWorkingSet.UnMap_FPR(m_Opcode.ft, true);
    asmjit::x86::Gp ValueRegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false), ValueRegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(ValueRegHi, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
    m_Assembler.mov(ValueRegLo, ValueRegHi);
    m_Assembler.AddConstToX86Reg(ValueRegHi, 4);
    m_Assembler.mov(ValueRegHi, asmjit::x86::dword_ptr(ValueRegHi));
    m_Assembler.mov(ValueRegLo, asmjit::x86::dword_ptr(ValueRegLo));

    CompileStoreMemoryValue(x86Reg_Unknown, ValueRegLo, ValueRegHi, 0, 64);
}

void CX86RecompilerOps::SD()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.base))
    {
        uint32_t Address = m_RegWorkingSet.GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            SW_Const(m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt) : (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> 31), Address);
            SW_Const(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt), Address + 4);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            SW_Register(m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false), Address);
            SW_Register(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), Address + 4);
        }
        else
        {
            asmjit::x86::Gp TempReg1 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            SW_Register(TempReg1, Address);
            SW_Register(m_RegWorkingSet.Map_TempReg(TempReg1, m_Opcode.rt, false, false), Address + 4);
        }
    }
    else
    {
        PreWriteInstruction();
        asmjit::x86::Gp ValueReg;
        if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
            {
                m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            }
            ValueReg = m_RegWorkingSet.IsUnknown(m_Opcode.rt) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false) : m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
        }
        uint64_t RtValue = 0;
        asmjit::x86::Gp ValueRegHi, ValueRegLo;
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            RtValue = ((uint64_t)(m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt) : (uint32_t)(m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> 31)) << 32) | m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            ValueRegHi = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            ValueRegLo = m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
        }
        else
        {
            ValueRegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            ValueRegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
        }
        CompileStoreMemoryValue(x86Reg_Unknown, ValueReg, ValueRegHi, RtValue, 64);
    }
}

// R4300i opcodes: Special
void CX86RecompilerOps::SPECIAL_SLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    if (m_Opcode.rd != m_Opcode.rt && m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        switch (m_Opcode.sa)
        {
        case 0:
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            break;
        case 1:
        case 2:
        case 3:
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.lea(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::ptr(0, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), m_Opcode.sa));
            break;
        default:
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    }
}

void CX86RecompilerOps::SPECIAL_SRL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_SRA()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }
    if (b32BitCore())
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    }
    else
    {
        asmjit::x86::Gp reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), reg, (uint8_t)m_Opcode.sa);
    }
}

void CX86RecompilerOps::SPECIAL_SLLV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) << Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        }
        return;
    }
    m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs, false, false);
    m_Assembler.and_(asmjit::x86::ecx, 0x1F);
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);
}

void CX86RecompilerOps::SPECIAL_SRLV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) >> Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }

    m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs, false, false);
    m_Assembler.and_(asmjit::x86::ecx, 0x1F);
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);
}

void CX86RecompilerOps::SPECIAL_SRAV()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & 0x1F);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> Shift);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            return;
        }
        if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        }
        else
        {
            asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Reg, (uint8_t)Shift);
        }
        return;
    }
    m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs, false, false);
    if (b32BitCore())
    {
        m_Assembler.and_(asmjit::x86::ecx, 0x1F);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);
    }
    else
    {
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
        m_Assembler.and_(asmjit::x86::ecx, 0x1F);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Reg, asmjit::x86::cl);
    }
}

void CX86RecompilerOps::SPECIAL_JR()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                m_Assembler.MoveX86regToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false));
            }
            OverflowDelaySlot(true);
            return;
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();

        R4300iOpcode DelaySlot;
        if (g_MMU->MemoryValue32(m_CompilePC + 4, DelaySlot.Value) &&
            R4300iInstruction(m_CompilePC, m_Opcode.Value).DelaySlotEffectsCompare(DelaySlot.Value))
        {
            if (m_RegWorkingSet.IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
            }
            else
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false));
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
            if (m_RegWorkingSet.IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
            }
            else
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false));
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
            if (m_RegWorkingSet.IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
            }
            else
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false));
            }
        }
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_CompilePC + 8);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                m_Assembler.MoveX86regToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false));
            }
            OverflowDelaySlot(true);
            return;
        }

        m_Section->m_Jump.FallThrough = false;
        m_Section->m_Jump.LinkLocation = asmjit::Label();
        m_Section->m_Jump.LinkLocation2 = asmjit::Label();
        m_Section->m_Cont.FallThrough = false;
        m_Section->m_Cont.LinkLocation = asmjit::Label();
        m_Section->m_Cont.LinkLocation2 = asmjit::Label();

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
            if (m_RegWorkingSet.IsConst(m_Opcode.rs))
            {
                m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            }
            else if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
            }
            else
            {
                m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false));
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

    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, -1);
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_LO.UW[0], "_RegLO->UW[0]");
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_LO.UW[1], "_RegLO->UW[1]");
}

void CX86RecompilerOps::SPECIAL_MTLO()
{
    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs));
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs) && ((m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
        }
        else
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0);
        }
        m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs));
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false));
        }
        else
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0);
        }
        m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
    }
    else
    {
        asmjit::x86::Gp reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", reg);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", m_RegWorkingSet.Map_TempReg(reg, m_Opcode.rs, false, false));
    }
}

void CX86RecompilerOps::SPECIAL_MFHI()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, -1);
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_HI.UW[0], "_RegHI->UW[0]");
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_HI.UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_MTHI()
{
    if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", m_RegWorkingSet.GetMipsRegHi(m_Opcode.rs));
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs) && ((m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", 0xFFFFFFFF);
        }
        else
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", 0);
        }
        m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rs) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs));
        }
        else if (m_RegWorkingSet.IsSigned(m_Opcode.rs))
        {
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false));
        }
        else
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", 0);
        }
        m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs));
    }
    else
    {
        asmjit::x86::Gp reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", reg);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", m_RegWorkingSet.Map_TempReg(reg, m_Opcode.rs, false, false));
    }
}

void CX86RecompilerOps::SPECIAL_DSLLV()
{
    asmjit::Label Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }
    m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs, false, false);
    m_Assembler.and_(asmjit::x86::ecx, 0x3F);
    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.CompConstToX86reg(asmjit::x86::ecx, 0x20);
    Jump[0] = m_Assembler.newLabel();
    m_Assembler.JaeLabel("MORE32", Jump[0]);
    m_Assembler.shld(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);
    m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);
    Jump[1] = m_Assembler.newLabel();
    m_Assembler.JmpLabel("Continue", Jump[1]);

    // MORE32:
    m_CodeBlock.Log("");
    m_Assembler.bind(Jump[0]);
    m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
    m_Assembler.and_(asmjit::x86::ecx, 0x1F);
    m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);

    // Continue:
    m_CodeBlock.Log("");
    m_Assembler.bind(Jump[1]);
}

void CX86RecompilerOps::SPECIAL_DSRLV()
{
    asmjit::Label Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t Shift = (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) & 0x3F);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }

            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt));
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, m_RegWorkingSet.GetMipsReg(m_Opcode.rd) >> Shift);
            if ((m_RegWorkingSet.GetMipsRegHi(m_Opcode.rd) == 0) && (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rd) & 0x80000000) == 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if ((m_RegWorkingSet.GetMipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rd) & 0x80000000) != 0)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
            }
            return;
        }
        m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, -1, false, false);
        m_Assembler.MoveConstToX86reg(asmjit::x86::ecx, Shift);
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        if ((Shift & 0x20) == 0x20)
        {
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd));
            m_Assembler.and_(asmjit::x86::ecx, 0x1F);
            m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);
        }
        else
        {
            m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);
            m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);
        }
    }
    else
    {
        m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs, false, false);
        m_Assembler.and_(asmjit::x86::ecx, 0x3F);
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.CompConstToX86reg(asmjit::x86::ecx, 0x20);
        Jump[0] = m_Assembler.newLabel();
        m_Assembler.JaeLabel("MORE32", Jump[0]);
        m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);
        m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);
        Jump[1] = m_Assembler.newLabel();
        m_Assembler.JmpLabel("Continue", Jump[1]);

        // MORE32:
        m_CodeBlock.Log("");
        m_Assembler.bind(Jump[0]);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd));
        m_Assembler.and_(asmjit::x86::ecx, 0x1F);
        m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);

        // Continue:
        m_CodeBlock.Log("");
        m_Assembler.bind(Jump[1]);
    }
}

void CX86RecompilerOps::SPECIAL_DSRAV()
{
    asmjit::Label Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }
    m_RegWorkingSet.Map_TempReg(asmjit::x86::ecx, m_Opcode.rs, false, false);
    m_Assembler.and_(asmjit::x86::ecx, 0x3F);
    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.CompConstToX86reg(asmjit::x86::ecx, 0x20);
    Jump[0] = m_Assembler.newLabel();
    m_Assembler.JaeLabel("MORE32", Jump[0]);
    m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);
    m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), asmjit::x86::cl);
    Jump[1] = m_Assembler.newLabel();
    m_Assembler.JmpLabel("Continue", Jump[1]);

    // MORE32:
    m_CodeBlock.Log("");
    m_Assembler.bind(Jump[0]);
    m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd));
    m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), 0x1F);
    m_Assembler.and_(asmjit::x86::ecx, 0x1F);
    m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), asmjit::x86::cl);

    // Continue:
    m_CodeBlock.Log("");
    m_Assembler.bind(Jump[1]);
}

void CX86RecompilerOps::SPECIAL_MULT()
{
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs, false, false);
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, m_Opcode.rt, false, false);

    m_Assembler.imul(asmjit::x86::edx);

    m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", asmjit::x86::edx);
    m_Assembler.sar(asmjit::x86::eax, 31); // Paired
    m_Assembler.sar(asmjit::x86::edx, 31);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", asmjit::x86::edx);
}

void CX86RecompilerOps::SPECIAL_MULTU()
{
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs, false, false);
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, m_Opcode.rt, false, false);

    m_Assembler.mul(asmjit::x86::edx);

    m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", asmjit::x86::edx);
    m_Assembler.sar(asmjit::x86::eax, 31); // Paired
    m_Assembler.sar(asmjit::x86::edx, 31);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", asmjit::x86::edx);
}

void CX86RecompilerOps::SPECIAL_DIV()
{
    asmjit::x86::Gp RegRs, RegRsHi, DivReg;
    asmjit::Label JumpNotDiv0;
    asmjit::Label JumpEnd;
    asmjit::Label JumpEnd2;

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == 0)
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0x00000001 : 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0x00000000 : 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0xFFFFFFFF : 0x00000000);
        }
        else
        {
            asmjit::x86::Gp Reg = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
            m_Assembler.CompConstToX86reg(Reg, 0);
            asmjit::Label JumpPositive = m_Assembler.newLabel();
            m_Assembler.JgeLabel(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), JumpPositive);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0x00000001);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0x00000000);
            asmjit::Label JumpLoSet = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), JumpLoSet);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpPositive);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpLoSet);
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", Reg);
            if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
            }
            else
            {
                m_Assembler.sar(Reg, 31);
            }
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", Reg);
        }
        return;
    }
    else if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == -1)
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) == 0x80000000)
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0x80000000);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", 0x00000000);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", 0x00000000);
            return;
        }

        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        m_RegWorkingSet.UnMap_X86reg(asmjit::x86::edx);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
        m_RegWorkingSet.UnMap_X86reg(asmjit::x86::eax);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        RegRs = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(RegRs), true);
        RegRsHi = m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_Opcode.rs : -1, true, false);
        DivReg = m_RegWorkingSet.IsMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);

        if (!m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(RegRs, 0x80000000);
            asmjit::Label JumpValid = m_Assembler.newLabel();
            m_Assembler.JneLabel(stdstr_f("ValidDiv_%08X", m_CompilePC).c_str(), JumpValid);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0x80000000);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", 0x00000000);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", 0x00000000);
            JumpEnd = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), JumpEnd);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpValid);
        }
    }
    else
    {
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        m_RegWorkingSet.UnMap_X86reg(asmjit::x86::edx);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
        m_RegWorkingSet.UnMap_X86reg(asmjit::x86::eax);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        RegRs = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(RegRs), true);
        RegRsHi = m_RegWorkingSet.IsMapped(m_Opcode.rs) && m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_Opcode.rs : -1, true, false);
        DivReg = m_RegWorkingSet.IsMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);

        if (!m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.CompConstToX86reg(DivReg, 0);
            JumpNotDiv0 = m_Assembler.newLabel();
            m_Assembler.JneLabel(stdstr_f("NotDiv0_%08X", m_CompilePC).c_str(), JumpNotDiv0);

            m_Assembler.CompConstToX86reg(RegRs, 0);
            asmjit::Label JumpPositive = m_Assembler.newLabel();
            m_Assembler.JgeLabel(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), JumpPositive);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0x00000001);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0x00000000);
            asmjit::Label JumpLoSet = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), JumpLoSet);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpPositive);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpLoSet);
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", RegRs);
            if (!m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                m_Assembler.sar(RegRsHi, 31);
            }
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", RegRsHi);
            JumpEnd = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), JumpEnd);

            m_CodeBlock.Log("");
            m_Assembler.bind(JumpNotDiv0);
            m_Assembler.CompConstToX86reg(DivReg, (uint32_t)-1);
            asmjit::Label JumpValidDiv0 = m_Assembler.newLabel();
            m_Assembler.JneLabel(stdstr_f("ValidDiv0_%08X", m_CompilePC).c_str(), JumpValidDiv0);

            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0x80000000);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", 0x00000000);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", 0x00000000);
            JumpEnd2 = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("EndDiv_%08X", m_CompilePC).c_str(), JumpEnd2);

            m_CodeBlock.Log("");
            m_Assembler.bind(JumpValidDiv0);
        }
    }

    m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
    m_RegWorkingSet.UnMap_X86reg(asmjit::x86::edx);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, -1, false, false);
    m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs, false, false);

    if (m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        m_Assembler.MoveConstToX86reg(asmjit::x86::edx, m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) >> 31);
    }
    else
    {
        m_Assembler.mov(asmjit::x86::edx, asmjit::x86::eax);
        m_Assembler.sar(asmjit::x86::edx, 31);
    }

    m_Assembler.idiv(DivReg);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", asmjit::x86::edx);
    m_Assembler.sar(asmjit::x86::eax, 31);
    m_Assembler.sar(asmjit::x86::edx, 31);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", asmjit::x86::edx);

    if (JumpEnd.isValid() || JumpEnd2.isValid())
    {
        m_CodeBlock.Log("");
        if (JumpEnd.isValid())
        {
            m_Assembler.bind(JumpEnd);
        }
        if (JumpEnd2.isValid())
        {
            m_Assembler.bind(JumpEnd2);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DIVU()
{
    asmjit::Label JumpEndDivu;
    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) == 0)
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            m_Assembler.MoveConstToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < 0 ? 0xFFFFFFFF : 0x00000000);
        }
        else
        {
            asmjit::x86::Gp RegRs = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
            m_Assembler.CompConstToX86reg(RegRs, 0);
            asmjit::Label JumpPositive = m_Assembler.newLabel();
            m_Assembler.JgeLabel(stdstr_f("RsPositive_%08X", m_CompilePC).c_str(), JumpPositive);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0x00000001);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0x00000000);
            asmjit::Label JumpLoSet = m_Assembler.newLabel();
            m_Assembler.JmpLabel(stdstr_f("LoSet_%08X", m_CompilePC).c_str(), JumpLoSet);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpPositive);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpLoSet);
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", RegRs);
            if (m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                RegRs = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
            }
            else
            {
                m_Assembler.sar(RegRs, 31);
            }
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", RegRs);
        }
    }
    else
    {
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        m_RegWorkingSet.UnMap_X86reg(asmjit::x86::edx);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EDX, true);
        m_RegWorkingSet.UnMap_X86reg(asmjit::x86::eax);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        asmjit::x86::Gp RegRsLo = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        asmjit::x86::Gp RegRsHi = m_RegWorkingSet.IsMapped(m_Opcode.rs) ? m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_RegWorkingSet.IsMapped(m_Opcode.rs), true, false) : x86Reg_Unknown;
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, true);
        m_RegWorkingSet.Map_TempReg(asmjit::x86::edx, 0, false, false);
        m_RegWorkingSet.SetX86Protected(x86RegIndex_EAX, false);

        m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, m_Opcode.rs, false, false);
        asmjit::x86::Gp DivReg = m_RegWorkingSet.IsMapped(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);

        if (!m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.CompConstToX86reg(DivReg, 0);
            asmjit::Label JumpNoExcept = m_Assembler.newLabel();
            m_Assembler.JneLabel("NoExcept", JumpNoExcept);

            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", 0xFFFFFFFF);
            m_Assembler.MoveConstToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", 0xFFFFFFFF);
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", RegRsLo);
            if (!m_RegWorkingSet.IsMapped(m_Opcode.rs))
            {
                RegRsHi = RegRsLo;
                m_Assembler.sar(RegRsHi, 31);
            }
            m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", RegRsHi);

            JumpEndDivu = m_Assembler.newLabel();
            m_Assembler.JmpLabel("EndDivu", JumpEndDivu);
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpNoExcept);
        }
        m_Assembler.div(DivReg);

        m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[0], "_RegLO->UW[0]", asmjit::x86::eax);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[0], "_RegHI->UW[0]", asmjit::x86::edx);
        m_Assembler.sar(asmjit::x86::eax, 31);
        m_Assembler.sar(asmjit::x86::edx, 31);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_LO.UW[1], "_RegLO->UW[1]", asmjit::x86::eax);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_HI.UW[1], "_RegHI->UW[1]", asmjit::x86::edx);

        if (JumpEndDivu.isValid())
        {
            m_CodeBlock.Log("");
            m_Assembler.bind(JumpEndDivu);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DMULT()
{
    if (m_Opcode.rs != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rs, true);
    }

    if (m_Opcode.rs != 0)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
    }

    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DMULTU()
{
    m_RegWorkingSet.UnMap_GPR(m_Opcode.rs, true);
    m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DMULTU, "R4300iOp::SPECIAL_DMULTU");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DDIV()
{
    m_RegWorkingSet.UnMap_GPR(m_Opcode.rs, true);
    m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DDIVU()
{
    m_RegWorkingSet.UnMap_GPR(m_Opcode.rs, true);
    m_RegWorkingSet.UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveConstToVariable(&R4300iOp::m_Opcode.Value, "R4300iOp::m_Opcode.Value", m_Opcode.Value);
    m_Assembler.CallFunc((uint32_t)R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_ADD()
{
    int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
    int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

    if (m_RegWorkingSet.IsConst(source1) && m_RegWorkingSet.IsConst(source2))
    {
        int32_t Val1 = m_RegWorkingSet.GetMipsRegLo(source1);
        int32_t Val2 = m_RegWorkingSet.GetMipsRegLo(source2);
        int32_t Sum = Val1 + Val2;
        if ((~(Val1 ^ Val2) & (Val1 ^ Sum)) & 0x80000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rd != 0)
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, Sum);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        return;
    }

    m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
    asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source1, false, false);
    if (m_RegWorkingSet.IsConst(source2))
    {
        m_Assembler.AddConstToX86Reg(Reg, m_RegWorkingSet.GetMipsRegLo(source2));
    }
    else if (m_RegWorkingSet.IsKnown(source2) && m_RegWorkingSet.IsMapped(source2))
    {
        m_Assembler.add(Reg, m_RegWorkingSet.GetMipsRegMapLo(source2));
    }
    else
    {
        m_Assembler.AddVariableToX86reg(Reg, &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel);
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    if (m_Opcode.rd != 0)
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Reg);
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

    if (m_RegWorkingSet.IsConst(source1) && m_RegWorkingSet.IsConst(source2))
    {
        uint32_t temp = m_RegWorkingSet.GetMipsRegLo(source1) + m_RegWorkingSet.GetMipsRegLo(source2);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        return;
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, source1);
    if (m_RegWorkingSet.IsConst(source2))
    {
        m_Assembler.AddConstToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegLo(source2));
    }
    else if (m_RegWorkingSet.IsKnown(source2) && m_RegWorkingSet.IsMapped(source2))
    {
        m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
    }
    else
    {
        m_Assembler.AddVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_SUB()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int32_t rs = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs);
        int32_t rt = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
        int32_t sub = rs - rt;

        if (((rs ^ rt) & (rs ^ sub)) & 0x80000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else if (m_Opcode.rd != 0)
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, sub);
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
    }
    else
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rd);
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.sub(Reg, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.sub(Reg, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rd != 0)
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Reg);
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

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        uint32_t temp = m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) - m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);

        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, temp);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
    }
    else
    {
        if (m_Opcode.rd == m_Opcode.rt)
        {
            asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Reg);
            return;
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
    }

    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_AND()
{
    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                           (m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt)) &
                                               (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs)));

                if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
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
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) & m_RegWorkingSet.GetMipsReg(m_Opcode.rs));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            m_RegWorkingSet.ProtectGPR(source1);
            m_RegWorkingSet.ProtectGPR(source2);
            if (m_RegWorkingSet.Is32Bit(source1) && m_RegWorkingSet.Is32Bit(source2))
            {
                bool Sign = (m_RegWorkingSet.IsSigned(m_Opcode.rt) && m_RegWorkingSet.IsSigned(m_Opcode.rs));
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, Sign, source1);
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
            }
            else if (m_RegWorkingSet.Is32Bit(source1) || m_RegWorkingSet.Is32Bit(source2))
            {
                if (m_RegWorkingSet.IsUnsigned(m_RegWorkingSet.Is32Bit(source1) ? source1 : source2))
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, source1);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
                    if (m_RegWorkingSet.Is32Bit(source2))
                    {
                        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false));
                    }
                    else
                    {
                        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(source2));
                    }
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
                }
            }
            else
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(source2));
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
            }
        }
        else
        {
            int ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            int MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(ConstReg))
            {
                if (m_RegWorkingSet.Is32Bit(MappedReg) && m_RegWorkingSet.IsUnsigned(MappedReg))
                {
                    if (m_RegWorkingSet.GetMipsRegLo(ConstReg) == 0)
                    {
                        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, 0);
                    }
                    else
                    {
                        uint32_t Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, MappedReg);
                        m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
                    }
                }
                else
                {
                    int64_t Value = m_RegWorkingSet.GetMipsReg(ConstReg);
                    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, MappedReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
                }
            }
            else if (m_RegWorkingSet.Is64Bit(MappedReg))
            {
                uint32_t Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                if (Value != 0)
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(ConstReg), MappedReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(ConstReg), 0);
                }
            }
            else
            {
                uint32_t Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                bool Sign = false;

                if (m_RegWorkingSet.IsSigned(ConstReg) && m_RegWorkingSet.IsSigned(MappedReg))
                {
                    Sign = true;
                }

                if (Value != 0)
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, Sign, MappedReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, 0);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            if (m_RegWorkingSet.Is64Bit(KnownReg))
            {
                uint64_t Value = m_RegWorkingSet.GetMipsReg(KnownReg);
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
            }
            else
            {
                uint32_t Value = m_RegWorkingSet.GetMipsRegLo(KnownReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(KnownReg), UnknownReg);
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
            }
        }
        else
        {
            m_RegWorkingSet.ProtectGPR(KnownReg);
            if (KnownReg == m_Opcode.rd)
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg) || !b32BitCore())
                {
                    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, KnownReg);
                    m_Assembler.AndVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                    m_Assembler.AndVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(KnownReg), KnownReg);
                    m_Assembler.AndVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
                }
            }
            else
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(KnownReg));
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(KnownReg));
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(KnownReg), UnknownReg);
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(KnownReg));
                }
            }
        }
    }
    else
    {
        if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            m_Assembler.AndVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        m_Assembler.AndVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
    }
}

void CX86RecompilerOps::SPECIAL_OR()
{
    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }

            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                           (m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt)) |
                                               (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs)));
                if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
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
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) | m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
                if (m_RegWorkingSet.Is64Bit(source2))
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(source2));
                }
                else
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false));
                }
            }
            else
            {
                m_RegWorkingSet.ProtectGPR(source2);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                uint64_t Value;

                if (m_RegWorkingSet.Is64Bit(ConstReg))
                {
                    Value = m_RegWorkingSet.GetMipsReg(ConstReg);
                }
                else
                {
                    Value = m_RegWorkingSet.IsSigned(ConstReg) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(ConstReg) : m_RegWorkingSet.GetMipsRegLo(ConstReg);
                }
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
                }
            }
            else
            {
                int Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        int KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            uint64_t Value = m_RegWorkingSet.Is64Bit(KnownReg) ? m_RegWorkingSet.GetMipsReg(KnownReg) : m_RegWorkingSet.GetMipsRegLo_S(KnownReg);
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);

            if (b32BitCore() && m_RegWorkingSet.Is32Bit(KnownReg))
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (dwValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
                }
            }
            else
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
                if (dwValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
                }
            }
        }
        else
        {
            if (b32BitCore())
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, KnownReg);
                m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
        }
    }
    else
    {
        if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        m_RegWorkingSet.ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_XOR()
{
    if (m_Opcode.rd == 0)
        return;

    if (m_Opcode.rt == m_Opcode.rs)
    {
        m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
        return;
    }
    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }

            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
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
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) ^ m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs));
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            m_RegWorkingSet.ProtectGPR(source1);
            m_RegWorkingSet.ProtectGPR(source2);
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
                if (m_RegWorkingSet.Is64Bit(source2))
                {
                    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(source2));
                }
                else if (m_RegWorkingSet.IsSigned(source2))
                {
                    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false));
                }
                m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
            }
            else
            {
                if (m_RegWorkingSet.IsSigned(m_Opcode.rt) != m_RegWorkingSet.IsSigned(m_Opcode.rs))
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, source1);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(m_Opcode.rt), source1);
                }
                m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
            }
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo;

                ConstHi = m_RegWorkingSet.Is32Bit(ConstReg) ? (uint32_t)(m_RegWorkingSet.GetMipsRegLo_S(ConstReg) >> 31) : m_RegWorkingSet.GetMipsRegHi(ConstReg);
                ConstLo = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if (ConstHi != 0)
                {
                    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), ConstHi);
                }
                if (ConstLo != 0)
                {
                    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), ConstLo);
                }
            }
            else
            {
                int Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                if (m_RegWorkingSet.IsSigned(m_Opcode.rt) != m_RegWorkingSet.IsSigned(m_Opcode.rs))
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                }
                else
                {
                    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, m_RegWorkingSet.IsSigned(MappedReg), MappedReg);
                }
                if (Value != 0)
                {
                    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        int KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            uint64_t Value;

            if (m_RegWorkingSet.Is64Bit(KnownReg))
            {
                Value = m_RegWorkingSet.GetMipsReg(KnownReg);
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
            }
            else
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (m_RegWorkingSet.IsSigned(KnownReg))
                {
                    Value = (int)m_RegWorkingSet.GetMipsRegLo(KnownReg);
                }
                else
                {
                    Value = m_RegWorkingSet.GetMipsRegLo(KnownReg);
                }
            }
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
            if (dwValue != 0)
            {
                m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
            }
        }
        else
        {
            if (b32BitCore())
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                m_Assembler.XorVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, KnownReg);
                m_Assembler.XorVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                m_Assembler.XorVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
        }
    }
    else if (b32BitCore())
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        m_Assembler.XorVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        m_Assembler.XorVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        m_Assembler.XorVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
    }
}

void CX86RecompilerOps::SPECIAL_NOR()
{
    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);

            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                           ~((m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt)) |
                                             (m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs))));
                if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
                {
                    m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                }
                else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
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
                m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, ~(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) | m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs)));
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
            int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
                if (m_RegWorkingSet.Is64Bit(source2))
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(source2));
                }
                else
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false));
                }
            }
            else
            {
                m_RegWorkingSet.ProtectGPR(source2);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                uint64_t Value;

                if (m_RegWorkingSet.Is64Bit(ConstReg))
                {
                    Value = m_RegWorkingSet.GetMipsReg(ConstReg);
                }
                else
                {
                    Value = m_RegWorkingSet.IsSigned(ConstReg) ? (int64_t)m_RegWorkingSet.GetMipsRegLo_S(ConstReg) : m_RegWorkingSet.GetMipsRegLo(ConstReg);
                }
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, MappedReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
                }
            }
            else
            {
                int Value = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), Value);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        int KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        int UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

        if (m_RegWorkingSet.IsConst(KnownReg))
        {
            uint64_t Value = m_RegWorkingSet.Is64Bit(KnownReg) ? m_RegWorkingSet.GetMipsReg(KnownReg) : m_RegWorkingSet.GetMipsRegLo_S(KnownReg);
            uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);

            if (b32BitCore() && m_RegWorkingSet.Is32Bit(KnownReg))
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, UnknownReg);
                if (dwValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
                }
            }
            else
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                }
                if (dwValue != 0)
                {
                    m_Assembler.or_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), dwValue);
                }
            }
        }
        else
        {
            if (b32BitCore())
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, KnownReg);
                m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
        }
    }
    else
    {
        if (b32BitCore())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        else
        {
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            m_Assembler.OrVariableToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
    }

    if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rd))
        {
            m_Assembler.not_(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd));
        }
        m_Assembler.not_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CX86RecompilerOps::SPECIAL_SLT()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                g_Notify->DisplayError("1");
                CX86RecompilerOps::UnknownOpcode();
            }
            else
            {
                if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
                {
                    m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
                }

                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) < m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt))
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 1);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
                }
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            if ((m_RegWorkingSet.Is64Bit(m_Opcode.rt) && m_RegWorkingSet.Is64Bit(m_Opcode.rs)) ||
                (!b32BitCore() && (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))))
            {
                asmjit::Label Jump[2];

                m_Assembler.cmp(
                    m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false),
                    m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false));
                Jump[0] = m_Assembler.newLabel();
                m_Assembler.JeLabel("Low Compare", Jump[0]);
                m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                Jump[1] = m_Assembler.newLabel();
                m_Assembler.JmpLabel("Continue", Jump[1]);

                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[0]);
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[1]);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));

                if (m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::eax && m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::ebx)
                {
                    m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.setl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
        else
        {
            uint32_t ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_Opcode.rs : m_Opcode.rt;
            uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_Opcode.rt : m_Opcode.rs;

            m_RegWorkingSet.ProtectGPR(MappedReg);
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                asmjit::Label Jump[2];

                m_Assembler.CompConstToX86reg(
                    m_RegWorkingSet.Is64Bit(MappedReg) ? m_RegWorkingSet.GetMipsRegMapHi(MappedReg) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, MappedReg, true, false),
                    m_RegWorkingSet.Is64Bit(ConstReg) ? m_RegWorkingSet.GetMipsRegHi(ConstReg) : (m_RegWorkingSet.GetMipsRegLo_S(ConstReg) >> 31));
                Jump[0] = m_Assembler.newLabel();
                m_Assembler.JeLabel("Low Compare", Jump[0]);
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                Jump[1] = m_Assembler.newLabel();
                m_Assembler.JmpLabel("Continue", Jump[1]);

                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[0]);
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), m_RegWorkingSet.GetMipsRegLo(ConstReg));
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[1]);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                uint32_t Constant = m_RegWorkingSet.GetMipsRegLo(ConstReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), Constant);

                if (m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::eax && m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::ebx)
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    else
                    {
                        m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        m_Assembler.setl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
                    }
                    else
                    {
                        m_Assembler.setg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
                    }
                    m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        asmjit::Label Jump[2];

        if (!b32BitCore())
        {
            if (m_RegWorkingSet.Is64Bit(KnownReg))
            {
                if (m_RegWorkingSet.IsConst(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], m_RegWorkingSet.GetMipsRegHi(KnownReg));
                }
                else
                {
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapHi(KnownReg), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (m_RegWorkingSet.IsConst(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], (m_RegWorkingSet.GetMipsRegLo_S(KnownReg) >> 31));
                }
                else
                {
                    m_RegWorkingSet.ProtectGPR(KnownReg);
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, KnownReg, true, false), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            Jump[0] = m_Assembler.newLabel();
            m_Assembler.JeLabel("Low Compare", Jump[0]);
            if (KnownReg == (m_RegWorkingSet.IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            }
            Jump[1] = m_Assembler.newLabel();
            m_Assembler.JmpLabel("Continue", Jump[1]);

            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[0]);
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
            }
            else
            {
                m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapLo(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (m_RegWorkingSet.IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[1]);
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            if (m_RegWorkingSet.IsMapped(KnownReg))
            {
                m_RegWorkingSet.ProtectGPR(KnownReg);
            }
            bool bConstant = m_RegWorkingSet.IsConst(KnownReg);
            uint32_t Value = m_RegWorkingSet.IsConst(KnownReg) ? m_RegWorkingSet.GetMipsRegLo(KnownReg) : 0;

            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
            if (bConstant)
            {
                m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], Value);
            }
            else
            {
                m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapLo(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::eax && m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::ebx)
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    m_Assembler.SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    m_Assembler.setg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    m_Assembler.setl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
                }
                m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), 1);
            }
        }
    }
    else if (b32BitCore())
    {
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::eax && m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd) != asmjit::x86::ebx)
        {
            m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
            m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
        }
        else
        {
            m_Assembler.setl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
            m_Assembler.and_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), 1);
        }
    }
    else
    {
        asmjit::Label Jump[2];

        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
        m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        Jump[0] = m_Assembler.newLabel();
        m_Assembler.JeLabel("Low Compare", Jump[0]);
        m_Assembler.SetlVariable(&m_BranchCompare, "m_BranchCompare");
        Jump[1] = m_Assembler.newLabel();
        m_Assembler.JmpLabel("Continue", Jump[1]);

        m_CodeBlock.Log("");
        m_Assembler.bind(Jump[0]);
        m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(Reg, m_Opcode.rs, false, false), &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1].isValid())
        {
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[1]);
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::SPECIAL_SLTU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsKnown(m_Opcode.rt) && m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                g_Notify->DisplayError("1");
                CX86RecompilerOps::UnknownOpcode();
            }
            else
            {
                if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
                {
                    m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
                }

                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
                if (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) < m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt))
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 1);
                }
                else
                {
                    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
                }
            }
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt) && m_RegWorkingSet.IsMapped(m_Opcode.rs))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
            m_RegWorkingSet.ProtectGPR(m_Opcode.rs);
            if ((m_RegWorkingSet.Is64Bit(m_Opcode.rt) && m_RegWorkingSet.Is64Bit(m_Opcode.rs)) ||
                (!b32BitCore() && (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))))
            {
                asmjit::Label Jump[2];

                m_Assembler.cmp(
                    m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rs) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false),
                    m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false));
                Jump[0] = m_Assembler.newLabel();
                m_Assembler.JeLabel("Low Compare", Jump[0]);
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                Jump[1] = m_Assembler.newLabel();
                m_Assembler.JmpLabel("Continue", Jump[1]);

                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[0]);
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[1]);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.cmp(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rs), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
        }
        else
        {
            if (m_RegWorkingSet.Is64Bit(m_Opcode.rt) || m_RegWorkingSet.Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo, ConstReg, MappedReg;
                asmjit::x86::Gp MappedRegHi, MappedRegLo;
                asmjit::Label Jump[2];

                ConstReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
                MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                ConstLo = m_RegWorkingSet.GetMipsRegLo_S(ConstReg);
                ConstHi = m_RegWorkingSet.GetMipsRegLo_S(ConstReg) >> 31;
                if (m_RegWorkingSet.Is64Bit(ConstReg))
                {
                    ConstHi = m_RegWorkingSet.GetMipsRegHi(ConstReg);
                }

                m_RegWorkingSet.ProtectGPR(MappedReg);
                MappedRegLo = m_RegWorkingSet.GetMipsRegMapLo(MappedReg);
                MappedRegHi = m_RegWorkingSet.GetMipsRegMapHi(MappedReg);
                if (m_RegWorkingSet.Is32Bit(MappedReg))
                {
                    MappedRegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, MappedReg, true, false);
                }

                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompConstToX86reg(MappedRegHi, ConstHi);
                Jump[0] = m_Assembler.newLabel();
                m_Assembler.JeLabel("Low Compare", Jump[0]);
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                Jump[1] = m_Assembler.newLabel();
                m_Assembler.JmpLabel("Continue", Jump[1]);

                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[0]);
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
                m_Assembler.bind(Jump[1]);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                uint32_t Const = m_RegWorkingSet.IsConst(m_Opcode.rs) ? m_RegWorkingSet.GetMipsRegLo(m_Opcode.rs) : m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt);
                uint32_t MappedReg = m_RegWorkingSet.IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                m_Assembler.CompConstToX86reg(m_RegWorkingSet.GetMipsRegMapLo(MappedReg), Const);
                if (MappedReg == m_Opcode.rs)
                {
                    m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
            }
        }
    }
    else if (m_RegWorkingSet.IsKnown(m_Opcode.rt) || m_RegWorkingSet.IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = m_RegWorkingSet.IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        asmjit::Label Jump[2];

        m_RegWorkingSet.ProtectGPR(KnownReg);
        if (b32BitCore())
        {
            uint32_t TestReg = m_RegWorkingSet.IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt;
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                uint32_t Value = m_RegWorkingSet.GetMipsRegLo(KnownReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], Value);
            }
            else
            {
                m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapLo(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
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
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], m_RegWorkingSet.GetMipsRegHi(KnownReg));
                }
                else
                {
                    m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], (m_RegWorkingSet.GetMipsRegLo_S(KnownReg) >> 31));
                }
            }
            else
            {
                if (m_RegWorkingSet.Is64Bit(KnownReg))
                {
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapHi(KnownReg), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    m_RegWorkingSet.ProtectGPR(KnownReg);
                    m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, KnownReg, true, false), &m_Reg.m_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            Jump[0] = m_Assembler.newLabel();
            m_Assembler.JeLabel("Low Compare", Jump[0]);

            if (KnownReg == (m_RegWorkingSet.IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            Jump[1] = m_Assembler.newLabel();
            m_Assembler.JmpLabel("Continue", Jump[1]);

            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[0]);
            if (m_RegWorkingSet.IsConst(KnownReg))
            {
                m_Assembler.CompConstToVariable(&m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], m_RegWorkingSet.GetMipsRegLo(KnownReg));
            }
            else
            {
                m_Assembler.CompX86regToVariable(m_RegWorkingSet.GetMipsRegMapLo(KnownReg), &m_Reg.m_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (m_RegWorkingSet.IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                m_Assembler.SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            if (Jump[1].isValid())
            {
                m_CodeBlock.Log("");
                m_Assembler.bind(Jump[1]);
            }
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
    else if (b32BitCore())
    {
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, false, false);
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
    else
    {
        asmjit::Label Jump[2];

        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rs, true, false);
        m_Assembler.CompX86regToVariable(Reg, &m_Reg.m_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        Jump[0] = m_Assembler.newLabel();
        m_Assembler.JeLabel("Low Compare", Jump[0]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Jump[1] = m_Assembler.newLabel();
        m_Assembler.JmpLabel("Continue", Jump[1]);

        m_CodeBlock.Log("");
        m_Assembler.bind(Jump[0]);
        m_Assembler.CompX86regToVariable(m_RegWorkingSet.Map_TempReg(Reg, m_Opcode.rs, false, false), &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1].isValid())
        {
            m_CodeBlock.Log("");
            m_Assembler.bind(Jump[1]);
        }
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_BranchCompare, "m_BranchCompare");
    }
}

void CX86RecompilerOps::SPECIAL_DADD()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int64_t rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
        int64_t rt = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        int64_t sum = rs + rt;
        if ((~(rs ^ rt) & (rs ^ sum)) & 0x8000000000000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, sum);
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
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

        m_RegWorkingSet.ProtectGPR(source1);
        m_RegWorkingSet.ProtectGPR(source2);
        asmjit::x86::Gp RegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source1, false, false);
        asmjit::x86::Gp RegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source1, true, false);

        if (m_RegWorkingSet.IsConst(source2))
        {
            m_Assembler.AddConstToX86Reg(RegLo, m_RegWorkingSet.GetMipsRegLo(source2));
            m_Assembler.adc(RegHi, m_RegWorkingSet.GetMipsRegHi(source2));
        }
        else if (m_RegWorkingSet.IsMapped(source2))
        {
            asmjit::x86::Gp HiReg = m_RegWorkingSet.Is64Bit(source2) ? m_RegWorkingSet.GetMipsRegMapHi(source2) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false);
            m_Assembler.add(RegLo, m_RegWorkingSet.GetMipsRegMapLo(source2));
            m_Assembler.adc(RegHi, HiReg);
        }
        else
        {
            m_Assembler.AddVariableToX86reg(RegLo, &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            m_Assembler.AdcVariableToX86reg(RegHi, &m_Reg.m_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rd != 0)
        {
            m_RegWorkingSet.UnProtectGPR(source1);
            m_RegWorkingSet.UnProtectGPR(source2);
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), RegLo);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), RegHi);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DADDU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int64_t ValRs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
        int64_t ValRt = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, ValRs + ValRt);
        if ((m_RegWorkingSet.GetMipsRegHi(m_Opcode.rd) == 0) && (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rd) & 0x80000000) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if ((m_RegWorkingSet.GetMipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (m_RegWorkingSet.GetMipsRegLo(m_Opcode.rd) & 0x80000000) != 0)
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

        if (m_RegWorkingSet.IsMapped(source2))
        {
            m_RegWorkingSet.ProtectGPR(source2);
        }
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
        if (m_RegWorkingSet.IsConst(source2))
        {
            uint32_t LoReg = m_RegWorkingSet.GetMipsRegLo(source2);
            m_Assembler.AddConstToX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), LoReg);
            if (LoReg != 0)
            {
                m_Assembler.adc(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegHi(source2));
            }
            else
            {
                m_Assembler.AddConstToX86Reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegHi(source2));
            }
        }
        else if (m_RegWorkingSet.IsMapped(source2))
        {
            asmjit::x86::Gp HiReg = m_RegWorkingSet.Is64Bit(source2) ? m_RegWorkingSet.GetMipsRegMapHi(source2) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false);
            m_Assembler.add(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(source2));
            m_Assembler.adc(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            m_Assembler.AddVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            m_Assembler.AdcVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSUB()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        int64_t rs = m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs);
        int64_t rt = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        int64_t sub = rs - rt;

        if (((rs ^ rt) & (rs ^ sub)) & 0x8000000000000000)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, true, nullptr);
            m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
        }
        else
        {
            if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
            {
                m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
            }
            m_RegWorkingSet.SetMipsReg(m_Opcode.rd, sub);
            if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
            {
                m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
            }
            else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
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

        m_RegWorkingSet.ProtectGPR(source1);
        m_RegWorkingSet.ProtectGPR(source2);
        asmjit::x86::Gp RegLo = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source1, false, false);
        asmjit::x86::Gp RegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source1, true, false);

        if (m_RegWorkingSet.IsConst(source2))
        {
            m_Assembler.sub(RegLo, m_RegWorkingSet.GetMipsRegLo(source2));
            m_Assembler.sbb(RegHi, m_RegWorkingSet.GetMipsRegHi(source2));
        }
        else if (m_RegWorkingSet.IsMapped(source2))
        {
            asmjit::x86::Gp HiReg = m_RegWorkingSet.Is64Bit(source2) ? m_RegWorkingSet.GetMipsRegMapHi(source2) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, source2, true, false);
            m_Assembler.sub(RegLo, m_RegWorkingSet.GetMipsRegMapLo(source2));
            m_Assembler.sbb(RegHi, HiReg);
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(RegLo, &m_Reg.m_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            m_Assembler.SbbVariableFromX86reg(RegHi, &m_Reg.m_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_ExceptionOverflow, false, &CX86Ops::JoLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        if (m_Opcode.rd != 0)
        {
            m_RegWorkingSet.UnProtectGPR(source1);
            m_RegWorkingSet.UnProtectGPR(source2);
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, source1);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), RegLo);
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), RegHi);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSUBU()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt) && m_RegWorkingSet.IsConst(m_Opcode.rs))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsReg(m_Opcode.rd,
                                   m_RegWorkingSet.Is64Bit(m_Opcode.rs) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rs) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rs) - m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg(m_Opcode.rt)
                                                                                                                                                                                                                : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt));
        if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
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
            asmjit::x86::Gp HiReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            asmjit::x86::Gp LoReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
            m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), LoReg);
            m_Assembler.sbb(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), HiReg);
            return;
        }
        if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        }
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
        if (m_RegWorkingSet.IsConst(m_Opcode.rt))
        {
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
            m_Assembler.sbb(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
        {
            asmjit::x86::Gp HiReg = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt) : m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false);
            m_Assembler.sub(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
            m_Assembler.sbb(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            m_Assembler.SubVariableFromX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
            m_Assembler.SbbVariableFromX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSLL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value << m_Opcode.sa);
        if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }

    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.shld(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_DSRL()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, Value >> m_Opcode.sa);
        if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }
    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_DSRA()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_RegWorkingSet.IsMapped(m_Opcode.rd))
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        int64_t Value = m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsReg_S(m_Opcode.rt) : (int64_t)m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt);
        m_RegWorkingSet.SetMipsReg_S(m_Opcode.rd, Value >> m_Opcode.sa);
        if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
        return;
    }

    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    m_Assembler.shrd(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
}

void CX86RecompilerOps::SPECIAL_DSLL32()
{
    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }
        m_RegWorkingSet.SetMipsRegHi(m_Opcode.rd, m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) << m_Opcode.sa);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, 0);
        if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) < 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == -1)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else if (m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rd) >= 0 && m_RegWorkingSet.GetMipsRegHi_S(m_Opcode.rd) == 0)
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        }
        else
        {
            m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        }
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, -1);
        if (m_Opcode.rt != m_Opcode.rd)
        {
            m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            m_CodeBlock.Log("    regcache: switch hi (%s) with lo (%s) for %s", CX86Ops::x86_Name(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt)), CX86Ops::x86_Name(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
            asmjit::x86::Gp HiReg = m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt);
            m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
            m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
        }
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rd, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt], CRegName::GPR_Hi[m_Opcode.rt]);
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.shl(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        m_Assembler.xor_(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CX86RecompilerOps::SPECIAL_DSRL32()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_64);
        m_RegWorkingSet.SetMipsReg(m_Opcode.rd, (uint32_t)(m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt) >> m_Opcode.sa));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rt))
        {
            if (m_Opcode.rt == m_Opcode.rd)
            {
                m_CodeBlock.Log("    regcache: switch hi (%s) with lo (%s) for %s", CX86Ops::x86_Name(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt)), CX86Ops::x86_Name(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                asmjit::x86::Gp HiReg = m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
                m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CX86RecompilerOps::UnknownOpcode();
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, false, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.shr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSRA32()
{
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        if (m_Opcode.rt != m_Opcode.rd)
        {
            m_RegWorkingSet.UnMap_GPR(m_Opcode.rd, false);
        }

        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, (uint32_t)(m_RegWorkingSet.GetMipsReg_S(m_Opcode.rt) >> (m_Opcode.sa + 32)));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_RegWorkingSet.ProtectGPR(m_Opcode.rt);
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rt))
        {
            if (m_Opcode.rt == m_Opcode.rd)
            {
                m_CodeBlock.Log("    regcache: switch hi (%s) with lo (%s) for %s", CX86Ops::x86_Name(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt)), CX86Ops::x86_Name(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                asmjit::x86::Gp HiReg = m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
            }
            else
            {
                m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
                m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CX86RecompilerOps::UnknownOpcode();
        }
    }
    else
    {
        m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rd, true, -1);
        m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), &m_Reg.m_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Lo[m_Opcode.rt]);
        if ((uint8_t)m_Opcode.sa != 0)
        {
            m_Assembler.sar(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

// COP0 functions
void CX86RecompilerOps::COP0_MF()
{
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.push(m_Opcode.rd);
    m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::Cop0_MF), "CRegisters::Cop0_MF", 8);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], asmjit::x86::eax);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
}

void CX86RecompilerOps::COP0_DMF()
{
    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.push(m_Opcode.rd);
    m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::Cop0_MF), "CRegisters::Cop0_MF", 8);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], asmjit::x86::eax);
    m_Assembler.MoveX86regToVariable(&m_Reg.m_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt], asmjit::x86::edx);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), &m_Reg.m_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
}

void CX86RecompilerOps::COP0_MT()
{
    if (m_Opcode.rd == 6 || m_Opcode.rd == 11)
    {
        UpdateCounters(m_RegWorkingSet, false, true);
    }
    m_RegWorkingSet.BeforeCallDirect();
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.push(m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> 31);
        m_Assembler.push(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        asmjit::x86::Gp HiReg = m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) != asmjit::x86::edx ? asmjit::x86::edx : asmjit::x86::eax;
        m_Assembler.mov(HiReg, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        m_Assembler.sar(HiReg, 0x1F);
        m_Assembler.push(HiReg);
        m_Assembler.push(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.MoveVariableToX86reg(asmjit::x86::eax, &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.mov(asmjit::x86::edx, asmjit::x86::eax);
        m_Assembler.sar(asmjit::x86::edx, 0x1F);
        m_Assembler.push(asmjit::x86::edx);
        m_Assembler.push(asmjit::x86::eax);
    }
    m_Assembler.push(m_Opcode.rd);
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
    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.push(m_RegWorkingSet.Is64Bit(m_Opcode.rt) ? m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt) : m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> 31);
        m_Assembler.push(m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rt))
        {
            m_Assembler.push(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt));
        }
        else
        {
            asmjit::x86::Gp HiReg = m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt) != asmjit::x86::edx ? asmjit::x86::edx : asmjit::x86::eax;
            m_Assembler.mov(HiReg, m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
            m_Assembler.sar(HiReg, 0x1F);
            m_Assembler.push(HiReg);
        }
        m_Assembler.push(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.MoveVariableToX86reg(asmjit::x86::eax, &m_Reg.m_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt]);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &m_Reg.m_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt]);
        m_Assembler.push(asmjit::x86::edx);
        m_Assembler.push(asmjit::x86::eax);
    }
    m_Assembler.push(m_Opcode.rd);
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
    m_Assembler.MoveVariableToX86reg(asmjit::x86::ecx, &g_Reg->INDEX_REGISTER, "INDEX_REGISTER");
    m_Assembler.and_(asmjit::x86::ecx, 0x1F);
    m_Assembler.push(asmjit::x86::ecx);
    m_Assembler.CallThis((uint32_t)g_TLB, AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry", 12);
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBWR(void)
{
    UpdateCounters(m_RegWorkingSet, false, true);
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallThis((uint32_t)g_SystemTimer, AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers", 4);
    m_Assembler.PushImm32("true", true);
    m_Assembler.MoveVariableToX86reg(asmjit::x86::ecx, &g_Reg->RANDOM_REGISTER, "RANDOM_REGISTER");
    m_Assembler.and_(asmjit::x86::ecx, 0x1F);
    m_Assembler.push(asmjit::x86::ecx);
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
    switch ((g_Reg->m_FPCR[31] & 3))
    {
    case 0: m_RoundingModeValue = 0x0000; break;
    case 1: m_RoundingModeValue = 0x0C00; break;
    case 2: m_RoundingModeValue = 0x0800; break;
    case 3: m_RoundingModeValue = 0x0400; break;
    }
}

// COP1 functions
void CX86RecompilerOps::COP1_MF()
{
    CompileCop1Test();

    m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
    asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.fs], stdstr_f("_FPR_S[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), asmjit::x86::dword_ptr(TempReg));
}

void CX86RecompilerOps::COP1_DMF()
{
    CompileCop1Test();

    m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    m_RegWorkingSet.Map_GPR_64bit(m_Opcode.rt, -1);
    asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.AddConstToX86Reg(TempReg, 4);
    m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt), asmjit::x86::dword_ptr(TempReg));
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("_FPR_D[%d]", m_Opcode.fs).c_str());
    m_Assembler.mov(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), asmjit::x86::dword_ptr(TempReg));
}

void CX86RecompilerOps::COP1_CF()
{
    CompileCop1Test();

    if (m_Opcode.fs != 31 && m_Opcode.fs != 0)
    {
        UnknownOpcode();
        return;
    }

    m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
    m_Assembler.MoveVariableToX86reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt), &m_Reg.m_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
}

void CX86RecompilerOps::COP1_MT()
{
    CompileCop1Test();

    if ((m_Opcode.fs & 1) != 0)
    {
        if (m_RegWorkingSet.RegInStack(m_Opcode.fs - 1, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs - 1, CRegInfo::FPU_Qword))
        {
            m_RegWorkingSet.UnMap_FPR(m_Opcode.fs - 1, true);
        }
    }
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.fs], stdstr_f("_FPR_S[%d]", m_Opcode.fs).c_str());

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false));
    }
}

void CX86RecompilerOps::COP1_DMT()
{
    CompileCop1Test();

    if ((m_Opcode.fs & 1) == 0)
    {
        if (m_RegWorkingSet.RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Float) || m_RegWorkingSet.RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Dword))
        {
            m_RegWorkingSet.UnMap_FPR(m_Opcode.fs + 1, true);
        }
    }
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.fs], stdstr_f("_FPR_D[%d]", m_Opcode.fs).c_str());

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt));
        m_Assembler.AddConstToX86Reg(TempReg, 4);
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rt))
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegHi(m_Opcode.rt));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegLo_S(m_Opcode.rt) >> 31);
        }
    }
    else if (m_RegWorkingSet.IsMapped(m_Opcode.rt))
    {
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt));
        m_Assembler.AddConstToX86Reg(TempReg, 4);
        if (m_RegWorkingSet.Is64Bit(m_Opcode.rt))
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.rt));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, true, false));
        }
    }
    else
    {
        asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), Reg);
        m_Assembler.AddConstToX86Reg(TempReg, 4);
        m_Assembler.mov(asmjit::x86::dword_ptr(TempReg), m_RegWorkingSet.Map_TempReg(Reg, m_Opcode.rt, true, false));
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

    if (m_RegWorkingSet.IsConst(m_Opcode.rt))
    {
        m_Assembler.MoveConstToVariable(&m_Reg.m_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], m_RegWorkingSet.GetMipsRegLo(m_Opcode.rt) & 0x183FFFF);
    }
    else
    {
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.rt, false, false);
        m_Assembler.and_(TempReg, 0x183FFFF);
        m_Assembler.MoveX86regToVariable(&m_Reg.m_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], TempReg);
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
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);

    asmjit::x86::Gp StatusReg = m_RegWorkingSet.Map_FPStatusReg();
    m_Assembler.and_(StatusReg, (uint32_t)(~0x0003F000));
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
    if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        m_Assembler.fadd(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(Reg2, true);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);
        m_Assembler.fadd(asmjit::x86::dword_ptr(TempReg));
    }
    m_RegWorkingSet.UnMap_X86reg(StatusReg);
}

void CX86RecompilerOps::COP1_S_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.fsub(asmjit::x86::dword_ptr(TempReg));
    }
    else
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            m_Assembler.fsub(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
        }
        else
        {
            m_RegWorkingSet.UnMap_FPR(Reg2, true);
            m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
            m_Assembler.fsub(asmjit::x86::dword_ptr(TempReg));
        }
    }
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);

    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
    if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        m_Assembler.fmul(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(Reg2, true);
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
        m_Assembler.fmul(asmjit::x86::dword_ptr(TempReg));
    }
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str());
        m_Assembler.fdiv(asmjit::x86::dword_ptr(TempReg));
    }
    else
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            m_Assembler.fdiv(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
        }
        else
        {
            m_RegWorkingSet.UnMap_FPR(Reg2, true);
            m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
            m_Assembler.fdiv(asmjit::x86::dword_ptr(TempReg));
        }
    }

    m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_ABS()
{
    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    m_Assembler.fabs();
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_NEG()
{
    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    m_Assembler.fchs();
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_SQRT()
{
    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    m_Assembler.fsqrt();
    m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_MOV()
{
    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
}

void CX86RecompilerOps::COP1_S_ROUND_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_S_TRUNC_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_S_CEIL_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_S_FLOOR_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_S_ROUND_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_S_TRUNC_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_S_CEIL_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_S_FLOOR_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_S_CVT_D()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_S_CVT_W()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Dword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_S_CVT_L()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Float))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Float, CRegInfo::FPU_Qword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_S_CMP()
{
    uint32_t Reg1 = m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft;
    uint32_t cmp = 0;

    if ((m_Opcode.funct & 4) == 0)
    {
        Reg1 = m_RegWorkingSet.RegInStack(m_Opcode.ft, CRegInfo::FPU_Float) ? m_Opcode.ft : m_Opcode.fs;
        Reg2 = m_RegWorkingSet.RegInStack(m_Opcode.ft, CRegInfo::FPU_Float) ? m_Opcode.fs : m_Opcode.ft;
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

    m_RegWorkingSet.Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, 0, false, false);
    if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        m_Assembler.fcom(m_RegWorkingSet.StackPosition(Reg2));
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(Reg2, true);
        m_RegWorkingSet.Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);

        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_S[Reg2], stdstr_f("_FPR_S[%d]", Reg2).c_str());
        m_Assembler.fcom(asmjit::x86::dword_ptr(TempReg));
    }
    m_Assembler.AndConstToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", (uint32_t)~FPCSR_C);
    m_Assembler.fnstsw(asmjit::x86::ax);
    asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, 0, false, true);
    m_Assembler.test(asmjit::x86::eax, cmp);
    m_Assembler.setnz(Reg);

    if (cmp != 0)
    {
        m_Assembler.test(asmjit::x86::eax, cmp);
        m_Assembler.setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            asmjit::x86::Gp _86RegReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, 0, false, true);
            m_Assembler.and_(asmjit::x86::eax, 0x4300);
            m_Assembler.CompConstToX86reg(asmjit::x86::eax, 0x4300);
            m_Assembler.setz(_86RegReg2);

            m_Assembler.or_(Reg, _86RegReg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        m_Assembler.and_(asmjit::x86::eax, 0x4300);
        m_Assembler.CompConstToX86reg(asmjit::x86::eax, 0x4300);
        m_Assembler.setz(Reg);
    }
    m_Assembler.shl(Reg, 23);
    m_Assembler.OrX86RegToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", Reg);
}

// COP1: D functions
void CX86RecompilerOps::COP1_D_ADD()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();

    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        m_Assembler.fadd(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(Reg2, true);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        m_Assembler.fadd(asmjit::x86::qword_ptr(TempReg));
    }
}

void CX86RecompilerOps::COP1_D_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        m_Assembler.fsub(asmjit::x86::qword_ptr(TempReg));
    }
    else
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            m_Assembler.fsub(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
        }
        else
        {
            m_RegWorkingSet.UnMap_FPR(Reg2, true);

            asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
            m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            m_Assembler.fsub(asmjit::x86::qword_ptr(TempReg));
        }
    }
}

void CX86RecompilerOps::COP1_D_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();
    m_RegWorkingSet.FixRoundModel(CRegInfo::RoundDefault);

    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        m_Assembler.fmul(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(Reg2, true);
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
        m_Assembler.fmul(asmjit::x86::qword_ptr(TempReg));
    }
}

void CX86RecompilerOps::COP1_D_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;

    CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str());
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        m_Assembler.fdiv(asmjit::x86::qword_ptr(TempReg));
    }
    else
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            m_Assembler.fdiv(asmjit::x86::st0, m_RegWorkingSet.StackPosition(Reg2));
        }
        else
        {
            m_RegWorkingSet.UnMap_FPR(Reg2, true);
            asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[Reg2], stdstr_f("_FPR_D[%d]").c_str());
            m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            m_Assembler.fdiv(asmjit::x86::qword_ptr(TempReg));
        }
    }
}

void CX86RecompilerOps::COP1_D_ABS()
{
    CompileCop1Test();
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    m_Assembler.fabs();
}

void CX86RecompilerOps::COP1_D_NEG()
{
    CompileCop1Test();
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    m_Assembler.fchs();
}

void CX86RecompilerOps::COP1_D_SQRT()
{
    CompileCop1Test();
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    m_Assembler.fsqrt();
}

void CX86RecompilerOps::COP1_D_MOV()
{
    CompileCop1Test();
    m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
}

void CX86RecompilerOps::COP1_D_ROUND_L()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_D_TRUNC_L()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_D_CEIL_L()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_D_FLOOR_L()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_D_ROUND_W()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundNearest);
}

void CX86RecompilerOps::COP1_D_TRUNC_W()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundTruncate);
}

void CX86RecompilerOps::COP1_D_CEIL_W()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundUp);
}

void CX86RecompilerOps::COP1_D_FLOOR_W()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundDown);
}

void CX86RecompilerOps::COP1_D_CVT_S()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fd, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_D_CVT_W()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Dword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_D_CVT_L()
{
    CompileCop1Test();
    if (m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Double) || m_RegWorkingSet.RegInStack(m_Opcode.fs, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.UnMap_FPR(m_Opcode.fs, true);
    }
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Double))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Double, CRegInfo::FPU_Qword, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_D_CMP()
{
    uint32_t Reg1 = m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft;
    uint32_t cmp = 0;

    if ((m_Opcode.funct & 4) == 0)
    {
        Reg1 = m_RegWorkingSet.RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) ? m_Opcode.ft : m_Opcode.fs;
        Reg2 = m_RegWorkingSet.RegInStack(m_Opcode.ft, CRegInfo::FPU_Double) ? m_Opcode.fs : m_Opcode.ft;
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

    m_RegWorkingSet.Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
    m_RegWorkingSet.Map_TempReg(asmjit::x86::eax, 0, false, false);
    if (m_RegWorkingSet.RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        m_Assembler.fcom(m_RegWorkingSet.StackPosition(Reg2));
    }
    else
    {
        m_RegWorkingSet.UnMap_FPR(Reg2, true);
        asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveVariableToX86reg(TempReg, (uint8_t *)&m_Reg.m_FPR_D[Reg2], stdstr_f("_FPR_D[%d]", Reg2).c_str());
        m_RegWorkingSet.Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
        m_Assembler.fcom(asmjit::x86::qword_ptr(TempReg));
    }
    m_Assembler.AndConstToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", (uint32_t)~FPCSR_C);
    m_Assembler.fnstsw(asmjit::x86::ax);
    asmjit::x86::Gp Reg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, 0, false, true);
    m_Assembler.test(asmjit::x86::eax, cmp);
    m_Assembler.setnz(Reg);
    if (cmp != 0)
    {
        m_Assembler.test(asmjit::x86::eax, cmp);
        m_Assembler.setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            asmjit::x86::Gp _86RegReg2 = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, 0, false, true);
            m_Assembler.and_(asmjit::x86::eax, 0x4300);
            m_Assembler.CompConstToX86reg(asmjit::x86::eax, 0x4300);
            m_Assembler.setz(_86RegReg2);

            m_Assembler.or_(Reg, _86RegReg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        m_Assembler.and_(asmjit::x86::eax, 0x4300);
        m_Assembler.CompConstToX86reg(asmjit::x86::eax, 0x4300);
        m_Assembler.setz(Reg);
    }
    m_Assembler.shl(Reg, 23);
    m_Assembler.OrX86RegToVariable(&m_Reg.m_FPCR[31], "_FPCR[31]", Reg);
}

// COP1: W functions
void CX86RecompilerOps::COP1_W_CVT_S()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Dword))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Dword);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Dword, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_W_CVT_D()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Dword))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Dword);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Dword, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
}

// COP1: L functions
void CX86RecompilerOps::COP1_L_CVT_S()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Qword);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Qword, CRegInfo::FPU_Float, CRegInfo::RoundDefault);
}

void CX86RecompilerOps::COP1_L_CVT_D()
{
    CompileCop1Test();
    if (m_Opcode.fd != m_Opcode.fs || !m_RegWorkingSet.RegInStack(m_Opcode.fd, CRegInfo::FPU_Qword))
    {
        m_RegWorkingSet.Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Qword);
    }
    m_RegWorkingSet.ChangeFPURegFormat(m_Opcode.fd, CRegInfo::FPU_Qword, CRegInfo::FPU_Double, CRegInfo::RoundDefault);
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
    m_Assembler.ret();
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

void CX86RecompilerOps::TestBreakpoint(const asmjit::x86::Gp & AddressReg, uint32_t FunctAddress, const char * FunctName)
{
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.MoveX86regToVariable(&memory_access_address, "memory_access_address", AddressReg);
    m_Assembler.MoveConstToVariable(&memory_write_in_delayslot, "memory_write_in_delayslot", (m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT) ? 1 : 0);
    m_Assembler.CallFunc(FunctAddress, FunctName);
    m_RegWorkingSet.AfterCallDirect();
    m_Assembler.CompConstToVariable(&memory_breakpoint_found, "memory_breakpoint_found", 0);
    asmjit::Label Jump = m_Assembler.newLabel();
    m_Assembler.JeLabel("NoBreakPoint", Jump);
    m_Assembler.MoveConstToVariable(&memory_breakpoint_found, "memory_breakpoint_found", 0);
    ExitCodeBlock();
    m_CodeBlock.Log("");
    m_Assembler.bind(Jump);
}

void CX86RecompilerOps::TestWriteBreakpoint(const asmjit::x86::Gp & AddressReg, uint32_t FunctAddress, const char * FunctName)
{
    if (!HaveWriteBP())
    {
        return;
    }
    TestBreakpoint(AddressReg, FunctAddress, FunctName);
}

void CX86RecompilerOps::TestReadBreakpoint(const asmjit::x86::Gp & AddressReg, uint32_t FunctAddress, const char * FunctName)
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
    m_Assembler.push(asmjit::x86::esi);
#else
    m_Assembler.push(asmjit::x86::edi);
    m_Assembler.push(asmjit::x86::esi);
    m_Assembler.push(asmjit::x86::ebx);
#endif
}

void CX86RecompilerOps::ExitCodeBlock()
{
    if (g_SyncSystem)
    {
        m_Assembler.CallThis((uint32_t)g_BaseSystem, AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem", 4);
    }
#ifdef _DEBUG
    m_Assembler.pop(asmjit::x86::esi);
#else
    m_Assembler.pop(asmjit::x86::ebx);
    m_Assembler.pop(asmjit::x86::esi);
    m_Assembler.pop(asmjit::x86::edi);
#endif
    m_Assembler.ret();
}

void CX86RecompilerOps::CompileExitCode()
{
    for (EXIT_LIST::iterator ExitIter = m_ExitInfo.begin(); ExitIter != m_ExitInfo.end(); ExitIter++)
    {
        m_CodeBlock.Log("");
        m_Assembler.bind(ExitIter->JumpLabel);
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

    m_Assembler.finit();
    m_Assembler.TestVariable(&g_Reg->STATUS_REGISTER, "STATUS_REGISTER", STATUS_CU1);
    CRegInfo ExitRegSet = m_RegWorkingSet;
    ExitRegSet.SetBlockCycleCount(ExitRegSet.GetBlockCycleCount() + g_System->CountPerOp());
    CompileExit(m_CompilePC, m_CompilePC, ExitRegSet, ExitReason_COP1Unuseable, false, &CX86Ops::JeLabel);
    m_RegWorkingSet.SetFpuBeenUsed(true);
}

void CX86RecompilerOps::CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter)
{
    m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", ProgramCounter);
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
            m_RegWorkingSet.UnMap_GPR(i, true);
        }
    }
    return true;
}

void CX86RecompilerOps::SyncRegState(const CRegInfo & SyncTo)
{
    m_RegWorkingSet.ResetX86Protection();

    bool changed = false;
    m_RegWorkingSet.UnMap_AllFPRs();
    if (m_RegWorkingSet.GetRoundingModel() != SyncTo.GetRoundingModel())
    {
        m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
    }
    asmjit::x86::Gp MemStackReg = m_RegWorkingSet.Get_MemoryStack();
    asmjit::x86::Gp TargetStackReg = SyncTo.Get_MemoryStack();

    //m_CodeBlock.Log("MemoryStack for Original State = %s",MemStackReg > 0?CX86Ops::x86_Name(MemStackReg):"Not Mapped");
    if (MemStackReg != TargetStackReg)
    {
        if (!TargetStackReg.isValid())
        {
            m_RegWorkingSet.UnMap_X86reg(MemStackReg);
        }
        else if (!MemStackReg.isValid())
        {
            m_RegWorkingSet.UnMap_X86reg(TargetStackReg);
            m_CodeBlock.Log("    regcache: allocate %s as memory stack", CX86Ops::x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(TargetStackReg), CRegInfo::Stack_Mapped);
            m_Assembler.MoveVariableToX86reg(TargetStackReg, &g_Recompiler->MemoryStackPos(), "MemoryStack");
        }
        else
        {
            m_RegWorkingSet.UnMap_X86reg(TargetStackReg);
            m_CodeBlock.Log("    regcache: change allocation of memory stack from %s to %s", CX86Ops::x86_Name(MemStackReg), CX86Ops::x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(TargetStackReg), CRegInfo::Stack_Mapped);
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(MemStackReg), CRegInfo::NotMapped);
            m_Assembler.mov(TargetStackReg, MemStackReg);
        }
    }

    for (int i = 1; i < 32; i++)
    {
        if (m_RegWorkingSet.GetMipsRegState(i) == SyncTo.GetMipsRegState(i) ||
            (b32BitCore() && m_RegWorkingSet.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN) ||
            (b32BitCore() && m_RegWorkingSet.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO))
        {
            switch (m_RegWorkingSet.GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN: continue;
            case CRegInfo::STATE_MAPPED_64:
                if (m_RegWorkingSet.GetMipsRegMapHi(i) == SyncTo.GetMipsRegMapHi(i) &&
                    m_RegWorkingSet.GetMipsRegMapLo(i) == SyncTo.GetMipsRegMapLo(i))
                {
                    continue;
                }
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (m_RegWorkingSet.GetMipsRegMapLo(i) == SyncTo.GetMipsRegMapLo(i))
                {
                    continue;
                }
                break;
            case CRegInfo::STATE_CONST_64:
                if (m_RegWorkingSet.GetMipsReg(i) != SyncTo.GetMipsReg(i))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (m_RegWorkingSet.GetMipsRegLo(i) != SyncTo.GetMipsRegLo(i))
                {
                    m_CodeBlock.Log("Value of constant is different register %d (%s) Value: 0x%08X to 0x%08X", i, CRegName::GPR[i], m_RegWorkingSet.GetMipsRegLo(i), SyncTo.GetMipsRegLo(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            default:
                m_CodeBlock.Log("Unhandled register state %d\nin SyncRegState", m_RegWorkingSet.GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        changed = true;

        switch (SyncTo.GetMipsRegState(i))
        {
        case CRegInfo::STATE_UNKNOWN: m_RegWorkingSet.UnMap_GPR(i, true); break;
        case CRegInfo::STATE_MAPPED_64:
        {
            asmjit::x86::Gp Reg = SyncTo.GetMipsRegMapLo(i);
            asmjit::x86::Gp x86RegHi = SyncTo.GetMipsRegMapHi(i);
            m_RegWorkingSet.UnMap_X86reg(Reg);
            m_RegWorkingSet.UnMap_X86reg(x86RegHi);
            switch (m_RegWorkingSet.GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN:
                m_Assembler.MoveVariableToX86reg(Reg, &m_Reg.m_GPR[i].UW[0], CRegName::GPR_Lo[i]);
                m_Assembler.MoveVariableToX86reg(x86RegHi, &m_Reg.m_GPR[i].UW[1], CRegName::GPR_Hi[i]);
                break;
            case CRegInfo::STATE_MAPPED_64:
                m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                m_Assembler.mov(x86RegHi, m_RegWorkingSet.GetMipsRegMapHi(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapHi(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                m_Assembler.mov(x86RegHi, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_Assembler.sar(x86RegHi, 31);
                m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                m_Assembler.xor_(x86RegHi, x86RegHi);
                m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_CONST_64:
                m_Assembler.MoveConstToX86reg(x86RegHi, m_RegWorkingSet.GetMipsRegHi(i));
                m_Assembler.MoveConstToX86reg(Reg, m_RegWorkingSet.GetMipsRegLo(i));
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                m_Assembler.MoveConstToX86reg(x86RegHi, m_RegWorkingSet.GetMipsRegLo_S(i) >> 31);
                m_Assembler.MoveConstToX86reg(Reg, m_RegWorkingSet.GetMipsRegLo(i));
                break;
            default:
                m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d", m_RegWorkingSet.GetMipsRegState(i));
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
            asmjit::x86::Gp Reg = SyncTo.GetMipsRegMapLo(i);
            m_RegWorkingSet.UnMap_X86reg(Reg);
            switch (m_RegWorkingSet.GetMipsRegState(i))
            {
            case CRegInfo::STATE_UNKNOWN: m_Assembler.MoveVariableToX86reg(Reg, &m_Reg.m_GPR[i].UW[0], CRegName::GPR_Lo[i]); break;
            case CRegInfo::STATE_CONST_32_SIGN: m_Assembler.MoveConstToX86reg(Reg, m_RegWorkingSet.GetMipsRegLo(i)); break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                if (m_RegWorkingSet.GetMipsRegMapLo(i) != Reg)
                {
                    m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                    m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                }
                break;
            case CRegInfo::STATE_MAPPED_64:
                m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapHi(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_CONST_64:
                m_CodeBlock.Log("hi %X\nLo %X", m_RegWorkingSet.GetMipsRegHi(i), m_RegWorkingSet.GetMipsRegLo(i));
            default:
                m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d", m_RegWorkingSet.GetMipsRegState(i));
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
            asmjit::x86::Gp Reg = SyncTo.GetMipsRegMapLo(i);
            m_RegWorkingSet.UnMap_X86reg(Reg);
            switch (m_RegWorkingSet.GetMipsRegState(i))
            {
            case CRegInfo::STATE_MAPPED_64:
            case CRegInfo::STATE_UNKNOWN:
                m_Assembler.MoveVariableToX86reg(Reg, &m_Reg.m_GPR[i].UW[0], CRegName::GPR_Lo[i]);
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
                m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                break;
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (b32BitCore())
                {
                    m_Assembler.mov(Reg, m_RegWorkingSet.GetMipsRegMapLo(i));
                    m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(i)), CRegInfo::NotMapped);
                }
                else
                {
                    m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", m_RegWorkingSet.GetMipsRegState(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (!b32BitCore() && m_RegWorkingSet.GetMipsRegLo_S(i) < 0)
                {
                    m_CodeBlock.Log("Sign problems in SyncRegState\nSTATE_MAPPED_32_ZERO");
                    m_CodeBlock.Log("%s: %X", CRegName::GPR[i], m_RegWorkingSet.GetMipsRegLo_S(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                m_Assembler.MoveConstToX86reg(Reg, m_RegWorkingSet.GetMipsRegLo(i));
                break;
            default:
                m_CodeBlock.Log("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", m_RegWorkingSet.GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
            m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
            m_RegWorkingSet.SetMipsRegState(i, SyncTo.GetMipsRegState(i));
            m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(Reg), CRegInfo::GPR_Mapped);
            m_RegWorkingSet.SetX86MapOrder(GetIndexFromX86Reg(Reg), 1);
            break;
        }
        default:
            m_CodeBlock.Log("%d - %d reg: %s (%d)", SyncTo.GetMipsRegState(i), m_RegWorkingSet.GetMipsRegState(i), CRegName::GPR[i], i);
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

    // Multiple parents
    BLOCK_PARENT_LIST ParentList;
    CCodeSection::SECTION_LIST::iterator iter;
    for (iter = m_Section->m_ParentSection.begin(); iter != m_Section->m_ParentSection.end(); iter++)
    {
        CCodeSection * Parent = *iter;
        BLOCK_PARENT BlockParent;

        if (!Parent->m_EnterLabel.isValid())
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

        if (Parent->m_EnterLabel.isValid())
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
    LinkJump(*JumpInfo);

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
    m_RegWorkingSet.UnMap_AllFPRs();

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
        asmjit::x86::Gp MemoryStackPos;
        int i2;

        if (i == (size_t)FirstParent)
        {
            continue;
        }
        Parent = ParentList[i].Parent;
        if (!Parent->m_EnterLabel.isValid())
        {
            continue;
        }
        CRegInfo * RegSet = &ParentList[i].JumpInfo->RegSet;

        if (m_RegWorkingSet.GetRoundingModel() != RegSet->GetRoundingModel())
        {
            m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
        }

        // Find parent MapRegState
        MemoryStackPos = x86Reg_Unknown;
        for (i2 = 0; i2 < x86RegIndex_Size; i2++)
        {
            if (RegSet->GetX86Mapped((x86RegIndex)i2) == CRegInfo::Stack_Mapped)
            {
                MemoryStackPos = GetX86RegFromIndex((x86RegIndex)i2);
                break;
            }
        }
        if (!MemoryStackPos.isValid())
        {
            // If the memory stack position is not mapped then unmap it
            asmjit::x86::Gp MemStackReg = m_RegWorkingSet.Get_MemoryStack();
            if (MemStackReg.isValid())
            {
                m_RegWorkingSet.UnMap_X86reg(MemStackReg);
            }
        }

        for (i2 = 1; i2 < 32; i2++)
        {
            if (m_RegWorkingSet.Is32BitMapped(i2))
            {
                switch (RegSet->GetMipsRegState(i2))
                {
                case CRegInfo::STATE_MAPPED_64: m_RegWorkingSet.Map_GPR_64bit(i2, i2); break;
                case CRegInfo::STATE_MAPPED_32_ZERO: break;
                case CRegInfo::STATE_MAPPED_32_SIGN:
                    if (m_RegWorkingSet.IsUnsigned(i2))
                    {
                        m_RegWorkingSet.SetMipsRegState(i2, CRegInfo::STATE_MAPPED_32_SIGN);
                    }
                    break;
                case CRegInfo::STATE_CONST_64: m_RegWorkingSet.Map_GPR_64bit(i2, i2); break;
                case CRegInfo::STATE_CONST_32_SIGN:
                    if ((RegSet->GetMipsRegLo_S(i2) < 0) && m_RegWorkingSet.IsUnsigned(i2))
                    {
                        m_RegWorkingSet.SetMipsRegState(i2, CRegInfo::STATE_MAPPED_32_SIGN);
                    }
                    break;
                case CRegInfo::STATE_UNKNOWN:
                    if (b32BitCore())
                    {
                        m_RegWorkingSet.Map_GPR_32bit(i2, true, i2);
                    }
                    else
                    {
                        //m_RegWorkingSet.Map_GPR_32bit(i2,true,i2);
                        m_RegWorkingSet.Map_GPR_64bit(i2, i2); //??
                        //m_RegWorkingSet.UnMap_GPR(Section,i2,true); ??
                    }
                    break;
                default:
                    m_CodeBlock.Log("Unknown CPU state(%d) in InheritParentInfo", m_RegWorkingSet.GetMipsRegState(i2));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            if (m_RegWorkingSet.IsConst(i2))
            {
                if (m_RegWorkingSet.GetMipsRegState(i2) != RegSet->GetMipsRegState(i2))
                {
                    switch (RegSet->GetMipsRegState(i2))
                    {
                    case CRegInfo::STATE_CONST_64:
                    case CRegInfo::STATE_MAPPED_64:
                        m_RegWorkingSet.Map_GPR_64bit(i2, i2);
                        break;
                    case CRegInfo::STATE_MAPPED_32_ZERO:
                        if (m_RegWorkingSet.Is32Bit(i2))
                        {
                            m_RegWorkingSet.Map_GPR_32bit(i2, (m_RegWorkingSet.GetMipsRegLo(i2) & 0x80000000) != 0, i2);
                        }
                        else
                        {
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        break;
                    case CRegInfo::STATE_MAPPED_32_SIGN:
                        if (m_RegWorkingSet.Is32Bit(i2))
                        {
                            m_RegWorkingSet.Map_GPR_32bit(i2, true, i2);
                        }
                        else
                        {
                            g_Notify->BreakPoint(__FILE__, __LINE__);
                        }
                        break;
                    case CRegInfo::STATE_UNKNOWN:
                        if (b32BitCore())
                        {
                            m_RegWorkingSet.Map_GPR_32bit(i2, true, i2);
                        }
                        else
                        {
                            m_RegWorkingSet.Map_GPR_64bit(i2, i2);
                        }
                        break;
                    default:
                        m_CodeBlock.Log("Unknown CPU state(%d) in InheritParentInfo", RegSet->GetMipsRegState(i2));
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                        break;
                    }
                }
                else if (m_RegWorkingSet.Is32Bit(i2) && m_RegWorkingSet.GetMipsRegLo(i2) != RegSet->GetMipsRegLo(i2))
                {
                    m_RegWorkingSet.Map_GPR_32bit(i2, true, i2);
                }
                else if (m_RegWorkingSet.Is64Bit(i2) && m_RegWorkingSet.GetMipsReg(i2) != RegSet->GetMipsReg(i2))
                {
                    m_RegWorkingSet.Map_GPR_32bit(i2, true, i2);
                }
            }
            m_RegWorkingSet.ResetX86Protection();
        }

        if (MemoryStackPos.isValid())
        {
            m_RegWorkingSet.Map_MemoryStack(MemoryStackPos, true);
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
                if (m_RegWorkingSet.GetMipsRegMapHi(i2) != RegSet->GetMipsRegMapHi(i2) ||
                    m_RegWorkingSet.GetMipsRegMapLo(i2) != RegSet->GetMipsRegMapLo(i2))
                {
                    NeedSync = true;
                }
                break;
            case CRegInfo::STATE_MAPPED_32_ZERO:
            case CRegInfo::STATE_MAPPED_32_SIGN:
                if (m_RegWorkingSet.GetMipsRegMapLo(i2) != RegSet->GetMipsRegMapLo(i2))
                {
                    //DisplayError(L"Parent: %d",Parent->SectionID);
                    NeedSync = true;
                }
                break;
            case CRegInfo::STATE_CONST_32_SIGN:
                if (m_RegWorkingSet.GetMipsRegLo(i2) != RegSet->GetMipsRegLo(i2))
                {
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                    NeedSync = true;
                }
                break;
            default:
                WriteTrace(TraceRecompiler, TraceError, "Unhandled register state %d\nin InheritParentInfo", m_RegWorkingSet.GetMipsRegState(i2));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        if (NeedSync == false)
        {
            continue;
        }
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        JumpInfo->LinkLocation = m_Assembler.newLabel();
        m_Assembler.JmpLabel(Label.c_str(), JumpInfo->LinkLocation);
        JumpInfo->LinkLocation2 = asmjit::Label();

        CurrentParent = i;
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        m_CodeBlock.Log("   Section_%d (from %d):", m_Section->m_SectionID, Parent->m_SectionID);
        if (JumpInfo->LinkLocation.isValid())
        {
            m_Assembler.bind(JumpInfo->LinkLocation);
            JumpInfo->LinkLocation = asmjit::Label();
            if (JumpInfo->LinkLocation2.isValid())
            {
                m_Assembler.bind(JumpInfo->LinkLocation2);
                JumpInfo->LinkLocation2 = asmjit::Label();
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

void CX86RecompilerOps::LinkJump(CJumpInfo & JumpInfo)
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

void CX86RecompilerOps::JumpToSection(CCodeSection * Section)
{
    m_Assembler.JmpLabel(stdstr_f("Section_%d", Section->m_SectionID).c_str(), Section->m_EnterLabel);
}

void CX86RecompilerOps::JumpToUnknown(CJumpInfo * JumpInfo)
{
    JumpInfo->LinkLocation = m_Assembler.newLabel();
    m_Assembler.JmpLabel(JumpInfo->BranchLabel.c_str(), JumpInfo->LinkLocation);
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

    m_CodeBlock.Log("");
    m_CodeBlock.Log("      // Updating sync CPU");
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
        m_CodeBlock.Log("");
        m_CodeBlock.Log("      // Update counter");
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
        asmjit::Label Jump = m_Assembler.newLabel();
        m_Assembler.JnsLabel("Continue_From_Timer_Test", Jump);
        RegSet.BeforeCallDirect();
        m_Assembler.CallThis((uint32_t)g_SystemTimer, AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone", 4);
        RegSet.AfterCallDirect();

        m_CodeBlock.Log("");
        m_Assembler.bind(Jump);
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
    asmjit::Label Jump = m_Assembler.newLabel();
    m_Assembler.JeLabel("Continue_From_Interrupt_Test", Jump);
    if (TargetPC != (uint32_t)-1)
    {
        m_Assembler.MoveConstToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", TargetPC);
    }

    CRegInfo RegSetCopy(RegSet);
    RegSetCopy.WriteBackRegisters();
    m_Assembler.CallThis((uint32_t)g_SystemEvents, AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents", 4);
    ExitCodeBlock();
    m_CodeBlock.Log("");
    m_Assembler.bind(Jump);
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
    m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", CompilePC());
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
    m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", CompilePC());
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
    if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT)
    {
        m_Assembler.MoveVariableToX86reg(asmjit::x86::ecx, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::ecx);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::ecx, &g_System->m_JumpDelayLocation, "System::JumpDelayLocation");
        m_Assembler.MoveX86regToVariable(&g_System->m_JumpToLocation, "System::m_JumpToLocation", asmjit::x86::ecx);
    }
    else
    {
        m_Assembler.MoveConstToVariable(&m_Reg.m_PROGRAM_COUNTER, "PROGRAM_COUNTER", CompilePC() + 4);
    }
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
    m_Assembler.AddConstToX86Reg(asmjit::x86::esp, 4);

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

void CX86RecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo & ExitRegSet, ExitReason reason, bool CompileNow, void (CX86Ops::*x86Jmp)(const char * LabelName, asmjit::Label & JumpLabel))
{
    if (!CompileNow)
    {
        if (x86Jmp == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        CExitInfo ExitInfo(m_CodeBlock);
        stdstr_f ExitName("Exit_%08X_%d", JumpPC, m_ExitInfo.size());
        (m_Assembler.*x86Jmp)(ExitName.c_str(), ExitInfo.JumpLabel);
        ExitInfo.ID = m_ExitInfo.size();
        ExitInfo.Name = ExitName;
        ExitInfo.TargetPC = TargetPC;
        ExitInfo.ExitRegSet = ExitRegSet;
        ExitInfo.Reason = reason;
        ExitInfo.PipelineStage = m_PipelineStage;
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
        ExitCodeBlock();
        break;
    case ExitReason_DoCPUAction:
        m_Assembler.CallThis((uint32_t)g_SystemEvents, AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents", 4);
        ExitCodeBlock();
        break;
    case ExitReason_DoSysCall:
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.push(0);
        m_Assembler.PushImm32("EXC_SYSCALL", EXC_SYSCALL);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::TriggerException), "CRegisters::TriggerException", 12);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    case ExitReason_Break:
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.push(0);
        m_Assembler.PushImm32("EXC_BREAK", EXC_BREAK);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::TriggerException), "CRegisters::TriggerException", 12);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    case ExitReason_COP1Unuseable:
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.push(1);
        m_Assembler.PushImm32("EXC_CPU", EXC_CPU);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::TriggerException), "CRegisters::TriggerException", 12);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    case ExitReason_ResetRecompCode:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case ExitReason_TLBReadMiss:
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, g_TLBLoadAddress, "g_TLBLoadAddress");
        m_Assembler.push(asmjit::x86::edx);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoTLBReadMiss), "CRegisters::DoTLBReadMiss", 12);
        ExitCodeBlock();
        break;
    case ExitReason_TLBWriteMiss:
        m_Assembler.X86BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case ExitReason_ExceptionOverflow:
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.push(0);
        m_Assembler.PushImm32("EXC_OV", EXC_OV);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::TriggerException), "CRegisters::TriggerException", 12);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    case ExitReason_AddressErrorExceptionRead32:
        m_Assembler.push(1);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &m_TempValue32, "TempValue32");
        m_Assembler.mov(asmjit::x86::eax, asmjit::x86::edx);
        m_Assembler.sar(asmjit::x86::eax, 31);
        m_Assembler.push(asmjit::x86::eax);
        m_Assembler.push(asmjit::x86::edx);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoAddressError), "CRegisters::DoAddressError", 12);
        ExitCodeBlock();
        break;
    case ExitReason_AddressErrorExceptionRead64:
        m_Assembler.push(1);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &m_TempValue64, "TempValue64");
        m_Assembler.MoveVariableToX86reg(asmjit::x86::eax, &m_TempValue64 + 4, "TempValue64+4");
        m_Assembler.push(asmjit::x86::eax);
        m_Assembler.push(asmjit::x86::edx);
        m_Assembler.PushImm32(InDelaySlot ? "true" : "false", InDelaySlot);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::DoAddressError), "CRegisters::DoAddressError", 12);
        ExitCodeBlock();
        break;
    case ExitReason_IllegalInstruction:
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "System->m_PipelineStage", m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT ? PIPELINE_STAGE_JUMP : PIPELINE_STAGE_NORMAL);
        m_Assembler.push(0);
        m_Assembler.PushImm32("EXC_II", EXC_II);
        m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::TriggerException), "CRegisters::TriggerException", 12);
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        ExitCodeBlock();
        break;
    case ExitReason_Exception:
        m_Assembler.MoveVariableToX86reg(asmjit::x86::edx, &g_System->m_JumpToLocation, "System->m_JumpToLocation");
        m_Assembler.MoveX86regToVariable(&g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER", asmjit::x86::edx);
        m_Assembler.MoveConstToVariable(&g_System->m_PipelineStage, "g_System->m_PipelineStage", PIPELINE_STAGE_NORMAL);
        if (TargetPC == (uint32_t)-1)
        {
            ExitRegSet.SetBlockCycleCount(0);
            UpdateCounters(ExitRegSet, true, false, false);
        }
        ExitCodeBlock();
        break;
    default:
        WriteTrace(TraceRecompiler, TraceError, "How did you want to exit on reason (%d) ???", reason);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

asmjit::x86::Gp CX86RecompilerOps::BaseOffsetAddress(bool UseBaseRegister)
{
    asmjit::x86::Gp AddressReg;
    if (m_RegWorkingSet.IsMapped(m_Opcode.base))
    {
        if (m_Opcode.offset != 0)
        {
            bool UnProtect = m_RegWorkingSet.GetX86Protected(GetIndexFromX86Reg(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.base)));
            m_RegWorkingSet.ProtectGPR(m_Opcode.base);
            AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.lea(AddressReg, asmjit::x86::ptr(m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset));
            if (!UnProtect)
            {
                m_RegWorkingSet.UnProtectGPR(m_Opcode.base);
            }
        }
        else if (UseBaseRegister)
        {
            m_RegWorkingSet.ProtectGPR(m_Opcode.base);
            AddressReg = m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.base);
        }
        else
        {
            AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.base, false, false);
        }
    }
    else
    {
        AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, m_Opcode.base, false, false);
        m_Assembler.AddConstToX86Reg(AddressReg, (int16_t)m_Opcode.immediate);
    }

    if (!b32BitCore() && ((m_RegWorkingSet.IsKnown(m_Opcode.base) && m_RegWorkingSet.Is64Bit(m_Opcode.base)) || m_RegWorkingSet.IsUnknown(m_Opcode.base)))
    {
        m_Assembler.MoveX86regToVariable(&m_TempValue64, "TempValue64", AddressReg);
        asmjit::x86::Gp AddressRegHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.mov(AddressRegHi, AddressReg);
        m_Assembler.sar(AddressRegHi, 31);

        if (m_RegWorkingSet.IsConst(m_Opcode.base))
        {
            m_Assembler.MoveConstToVariable(&m_TempValue64 + 4, "TempValue64 + 4", m_RegWorkingSet.GetMipsRegHi(m_Opcode.base));
            m_Assembler.CompConstToX86reg(AddressRegHi, m_RegWorkingSet.GetMipsRegHi(m_Opcode.base));
        }
        else if (m_RegWorkingSet.IsMapped(m_Opcode.base))
        {
            m_Assembler.MoveX86regToVariable(&m_TempValue64 + 4, "TempValue64 + 4", m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.base));
            m_Assembler.cmp(AddressRegHi, m_RegWorkingSet.GetMipsRegMapHi(m_Opcode.base));
        }
        else
        {
            asmjit::x86::Gp AddressMemoryHi = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveVariableToX86reg(AddressMemoryHi, &m_Reg.m_GPR[m_Opcode.base].W[1], CRegName::GPR_Hi[m_Opcode.base]);
            m_Assembler.MoveX86regToVariable(&m_TempValue64 + 4, "TempValue64 + 4", AddressMemoryHi);
            m_Assembler.cmp(AddressRegHi, AddressMemoryHi);
            m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(AddressMemoryHi), false);
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_AddressErrorExceptionRead64, false, &CX86Ops::JneLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(AddressRegHi), false);
    }
    return AddressReg;
}

void CX86RecompilerOps::CompileLoadMemoryValue(asmjit::x86::Gp AddressReg, asmjit::x86::Gp ValueReg, const asmjit::x86::Gp & ValueRegHi, uint8_t ValueSize, bool SignExtend)
{
    bool UnprotectAddressReg = !AddressReg.isValid();
    if (UnprotectAddressReg)
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

    asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    if (ValueSize == 16)
    {
        m_Assembler.MoveX86regToVariable(&m_TempValue32, "TempValue32", AddressReg);
        m_Assembler.test(AddressReg, 1);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_AddressErrorExceptionRead32, false, &CX86Ops::JneLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    }
    else if (ValueSize == 32)
    {
        m_Assembler.MoveX86regToVariable(&m_TempValue32, "TempValue32", AddressReg);
        m_Assembler.test(AddressReg, 3);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_AddressErrorExceptionRead32, false, &CX86Ops::JneLabel);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    }

    m_Assembler.mov(TempReg, AddressReg);
    m_Assembler.shr(TempReg, 12);
    m_Assembler.MoveVariableDispToX86Reg(TempReg, g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg, CX86Ops::Multip_x4);
    m_Assembler.CompConstToX86reg(TempReg, (uint32_t)-1);
    asmjit::Label JumpFound = m_Assembler.newLabel();
    m_Assembler.JneLabel(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), JumpFound);
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
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)(&m_MMU), AddressOf(&CMipsMemoryVM::LW_NonMemory), "CMipsMemoryVM::LW_NonMemory", 12);
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        m_Assembler.MoveConstToX86reg(TempReg, (uint32_t)&m_TempValue32);
        m_Assembler.sub(TempReg, AddressReg);
    }
    else if (ValueSize == 16)
    {
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)(&m_MMU), AddressOf(&CMipsMemoryVM::LH_NonMemory), "CMipsMemoryVM::LH_NonMemory", 12);
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        m_Assembler.MoveConstToX86reg(TempReg, (uint32_t)&m_TempValue32);
        m_Assembler.sub(TempReg, AddressReg);
        m_Assembler.xor_(AddressReg, 2);
    }
    else if (ValueSize == 8)
    {
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::LB_NonMemory), "CMipsMemoryVM::LB_NonMemory", 12);
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        m_Assembler.MoveConstToX86reg(TempReg, (uint32_t)&m_TempValue32);
        m_Assembler.sub(TempReg, AddressReg);
        m_Assembler.xor_(AddressReg, 3);
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
    m_Assembler.bind(JumpFound);

    if (ValueSize == 8)
    {
        m_Assembler.xor_(AddressReg, 3);
        if (!ValueReg.isValid())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        else if (SignExtend)
        {
            m_Assembler.movsx(ValueReg, asmjit::x86::byte_ptr(AddressReg, TempReg));
        }
        else
        {
            m_Assembler.movzx(ValueReg, asmjit::x86::byte_ptr(AddressReg, TempReg));
        }
    }
    else if (ValueSize == 16)
    {
        m_Assembler.xor_(AddressReg, 2);
        if (!ValueReg.isValid())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, SignExtend, -1);
            ValueReg = m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
        }

        if (SignExtend)
        {
            m_Assembler.movsx(ValueReg, asmjit::x86::word_ptr(AddressReg, TempReg));
        }
        else
        {
            m_Assembler.movzx(ValueReg, asmjit::x86::word_ptr(AddressReg, TempReg));
        }
    }
    else if (ValueSize == 32)
    {
        if (!ValueReg.isValid())
        {
            m_RegWorkingSet.Map_GPR_32bit(m_Opcode.rt, true, -1);
            ValueReg = m_RegWorkingSet.GetMipsRegMapLo(m_Opcode.rt);
        }

        if (ValueReg.isValid())
        {
            m_Assembler.mov(ValueReg, asmjit::x86::dword_ptr(AddressReg, TempReg));
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (ValueSize == 64)
    {
        if (ValueReg.isValid())
        {
            m_Assembler.mov(ValueRegHi, asmjit::x86::dword_ptr(AddressReg, TempReg));
            m_Assembler.mov(ValueReg, asmjit::x86::dword_ptr(AddressReg, TempReg, 0, 4));
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

void CX86RecompilerOps::CompileStoreMemoryValue(asmjit::x86::Gp AddressReg, asmjit::x86::Gp ValueReg, const asmjit::x86::Gp & ValueRegHi, uint64_t Value, uint8_t ValueSize)
{
    asmjit::Label MemoryWriteDone;

    if (!AddressReg.isValid())
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
    asmjit::x86::Gp TempReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
    m_Assembler.mov(TempReg, AddressReg);
    m_Assembler.shr(TempReg, 12);
    m_Assembler.MoveVariableDispToX86Reg(TempReg, g_MMU->m_MemoryWriteMap, "MMU->m_MemoryWriteMap", TempReg, CX86Ops::Multip_x4);
    m_Assembler.CompConstToX86reg(TempReg, (uint32_t)-1);
    asmjit::Label JumpFound = m_Assembler.newLabel();
    m_Assembler.JneLabel(stdstr_f("MemoryWriteMap_%X_Found", m_CompilePC).c_str(), JumpFound);

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
        if (!ValueReg.isValid())
        {
            m_Assembler.push((uint32_t)Value);
        }
        else
        {
            m_Assembler.push(ValueReg);
        }
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SB_NonMemory), "CMipsMemoryVM::SB_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        MemoryWriteDone = m_Assembler.newLabel();
        m_Assembler.JmpLabel(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), MemoryWriteDone);
    }
    else if (ValueSize == 16)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (!ValueReg.isValid())
        {
            m_Assembler.push((uint32_t)(Value & 0xFFFF));
        }
        else
        {
            m_Assembler.push(ValueReg);
        }
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SH_NonMemory), "CMipsMemoryVM::SH_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        MemoryWriteDone = m_Assembler.newLabel();
        m_Assembler.JmpLabel(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), MemoryWriteDone);
    }
    else if (ValueSize == 32)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (!ValueReg.isValid())
        {
            m_Assembler.push((uint32_t)(Value & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.push(ValueReg);
        }
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        MemoryWriteDone = m_Assembler.newLabel();
        m_Assembler.JmpLabel(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), MemoryWriteDone);
    }
    else if (ValueSize == 64)
    {
        m_RegWorkingSet.BeforeCallDirect();
        if (!ValueReg.isValid())
        {
            m_Assembler.push((uint32_t)(Value & 0xFFFFFFFF));
            m_Assembler.push((uint32_t)((Value >> 32) & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.push(ValueReg);
            m_Assembler.push(ValueRegHi);
        }
        m_Assembler.push(AddressReg);
        m_Assembler.CallThis((uint32_t)&m_MMU, AddressOf(&CMipsMemoryVM::SD_NonMemory), "CMipsMemoryVM::SD_NonMemory", 12);
        if (OpsExecuted != 0)
        {
            m_Assembler.AddConstToVariable(g_NextTimer, "g_NextTimer", OpsExecuted);
        }
        m_Assembler.test(asmjit::x86::al, asmjit::x86::al);
        m_RegWorkingSet.AfterCallDirect();
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, ExitReason_NormalNoSysCheck, false, &CX86Ops::JeLabel);
        MemoryWriteDone = m_Assembler.newLabel();
        m_Assembler.JmpLabel(stdstr_f("MemoryWrite_%X_Done:", m_CompilePC).c_str(), MemoryWriteDone);
    }
    else
    {
        m_Assembler.X86BreakPoint(__FILE__, __LINE__);
        m_Assembler.mov(TempReg, AddressReg);
        m_Assembler.shr(TempReg, 12);
        m_Assembler.MoveVariableDispToX86Reg(TempReg, g_MMU->m_TLB_WriteMap, "MMU->TLB_WriteMap", TempReg, CX86Ops::Multip_x4);
        CompileWriteTLBMiss(AddressReg, TempReg);
        m_Assembler.AddConstToX86Reg(TempReg, (uint32_t)m_MMU.Rdram());
    }
    m_CodeBlock.Log("");
    m_Assembler.bind(JumpFound);

    if (ValueSize == 8)
    {
        m_Assembler.xor_(AddressReg, 3);
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::byte_ptr(AddressReg, TempReg), (uint8_t)(Value & 0xFF));
        }
        else if (m_Assembler.Is8BitReg(ValueReg))
        {
            m_Assembler.mov(asmjit::x86::byte_ptr(AddressReg, TempReg), ValueReg.r8Lo());
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (ValueSize == 16)
    {
        m_Assembler.xor_(AddressReg, 2);
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::word_ptr(AddressReg, TempReg), (uint16_t)(Value & 0xFFFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::word_ptr(AddressReg, TempReg), ValueReg.r16());
        }
    }
    else if (ValueSize == 32)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, TempReg), (uint32_t)(Value & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, TempReg), ValueReg);
        }
    }
    else if (ValueSize == 64)
    {
        if (!ValueReg.isValid())
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, TempReg), (uint32_t)(Value >> 32));
            m_Assembler.AddConstToX86Reg(AddressReg, 4);
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, TempReg), (uint32_t)(Value & 0xFFFFFFFF));
        }
        else
        {
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, TempReg), ValueRegHi);
            m_Assembler.AddConstToX86Reg(AddressReg, 4);
            m_Assembler.mov(asmjit::x86::dword_ptr(AddressReg, TempReg), ValueReg);
        }
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (MemoryWriteDone.isValid())
    {
        m_CodeBlock.Log("");
        m_Assembler.bind(MemoryWriteDone);
    }
}

void CX86RecompilerOps::SB_Const(uint32_t Value, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, x86Reg_Unknown, x86Reg_Unknown, Value, 8);
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
            asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, x86Reg_Unknown, x86Reg_Unknown, Value, 8);
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
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Value << ((3 - (PAddr & 3)) * 8));
            m_Assembler.push(PAddr);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][1], "RomMemoryHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::SB_Register(const asmjit::x86::Gp & Reg, uint32_t VAddr)
{
    m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 8);
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
            asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 8);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            m_Assembler.MoveX86regByteToVariable((PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 3)", PAddr).c_str(), Reg);
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
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, x86Reg_Unknown, x86Reg_Unknown, (uint16_t)Value, 16);
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
            asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, x86Reg_Unknown, x86Reg_Unknown, (uint16_t)Value, 16);
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
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Value << ((2 - (PAddr & 2)) * 8));
            m_Assembler.push(PAddr);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_RomMemoryHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_RomMemoryHandler)[0][1], "RomMemoryHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
        }
        else if (BreakOnUnhandledMemory())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CX86RecompilerOps::SH_Register(const asmjit::x86::Gp & Reg, uint32_t VAddr)
{
    m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 16);
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
                    asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
                    m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
                    CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 16);
                }
                else if (PAddr < g_MMU->RdramSize())
                {
                    m_Assembler.MoveX86regHalfToVariable((PAddr ^ 2) + g_MMU->Rdram(), stdstr_f("RDRAM + (%X ^ 2)", PAddr).c_str(), Reg);
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
    asmjit::Label Jump;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, x86Reg_Unknown, x86Reg_Unknown, Value, 32);
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
            asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, x86Reg_Unknown, x86Reg_Unknown, Value, 32);
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
                m_Assembler.push(0xFFFFFFFF);
                m_Assembler.push(Value);
                m_Assembler.push(PAddr & 0x1FFFFFFF);
                m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_SPRegistersHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_SPRegistersHandler)[0][1], "SPRegistersHandler::Write32", 16);
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04040010:
                UpdateCounters(m_RegWorkingSet, false, true, false);

                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.push(Value);
                m_Assembler.push(PAddr | 0xA0000000);
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
            m_Assembler.push(Value);
            m_Assembler.push(PAddr | 0xA0000000);
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
                m_Assembler.OrConstToVariable(&g_Reg->MI_MODE_REG, "MI_MODE_REG", ModValue);
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
                m_Assembler.OrConstToVariable(&g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG", ModValue);
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
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Value);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
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
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JeLabel("Continue", Jump);
                    m_Assembler.MoveConstToVariable(&g_Reg->VI_STATUS_REG, "VI_STATUS_REG", Value);
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                break;
            case 0x04400004: m_Assembler.MoveConstToVariable(&g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG", (Value & 0xFFFFFF)); break;
            case 0x04400008:
                if (g_Plugins->Gfx()->ViWidthChanged != nullptr)
                {
                    m_Assembler.CompConstToVariable(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", Value);
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JeLabel("Continue", Jump);
                    m_Assembler.MoveConstToVariable(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", Value);
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
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
        m_Assembler.push(0xFFFFFFFF);
        m_Assembler.push(Value);
        m_Assembler.push(PAddr & 0x1FFFFFFF);
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
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Value);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
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
        UpdateCounters(m_RegWorkingSet, false, true, false);
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.push(0xFFFFFFFF);
        m_Assembler.push(Value);
        m_Assembler.push(PAddr & 0x1FFFFFFF);
        m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_PifRamHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_PifRamHandler)[0][1], "PifRamHandler::Write32", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    default:
        if (PAddr >= 0x10000000 && PAddr < 0x13F00000)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Value);
            m_Assembler.push(PAddr);
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
            m_Assembler.push(Value);
            m_Assembler.push(PAddr | 0xA0000000);
            m_Assembler.CallThis((uint32_t)(g_MMU), AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
            m_RegWorkingSet.AfterCallDirect();
        }
    }
}

void CX86RecompilerOps::SW_Register(const asmjit::x86::Gp & Reg, uint32_t VAddr)
{
    m_RegWorkingSet.SetX86Protected(GetIndexFromX86Reg(Reg), true);

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
        m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
        CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 32);
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
            asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
            m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
            CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 32);
        }
        else if (PAddr < g_MMU->RdramSize())
        {
            m_Assembler.MoveX86regToVariable(PAddr + g_MMU->Rdram(), stdstr_f("RDRAM + %X", PAddr).c_str(), Reg);
        }
        break;
    case 0x04000000:
        switch (PAddr)
        {
        case 0x04040000: m_Assembler.MoveX86regToVariable(&g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG", Reg); break;
        case 0x04040004: m_Assembler.MoveX86regToVariable(&g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG", Reg); break;
        case 0x04040008:
        case 0x0404000C:
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Reg);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_SPRegistersHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_SPRegistersHandler)[0][1], "SPRegistersHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04040010:
            UpdateCounters(m_RegWorkingSet, false, true, false);
            m_Assembler.MoveX86regToVariable(&CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue", Reg);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallFunc((uint32_t)CMipsMemoryVM::ChangeSpStatus, "CMipsMemoryVM::ChangeSpStatus");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0404001C: m_Assembler.MoveConstToVariable(&g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", 0); break;
        case 0x04080000:
            m_Assembler.MoveX86regToVariable(&g_Reg->SP_PC_REG, "SP_PC_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->SP_PC_REG, "SP_PC_REG", 0xFFC);
            break;
        default:
            if (CGameSettings::bSMM_StoreInstruc())
            {
                asmjit::x86::Gp AddressReg = m_RegWorkingSet.Map_TempReg(x86Reg_Unknown, -1, false, false);
                m_Assembler.MoveConstToX86reg(AddressReg, VAddr);
                CompileStoreMemoryValue(AddressReg, Reg, x86Reg_Unknown, 0, 32);
            }
            else if (PAddr < 0x04001000)
            {
                m_Assembler.MoveX86regToVariable(g_MMU->Dmem() + (PAddr - 0x04000000), stdstr_f("DMEM + 0x%X", (PAddr - 0x04000000)).c_str(), Reg);
            }
            else if (PAddr < 0x04002000)
            {
                m_Assembler.MoveX86regToVariable(g_MMU->Imem() + (PAddr - 0x04001000), stdstr_f("IMEM + 0x%X", (PAddr - 0x04001000)).c_str(), Reg);
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
        m_Assembler.push(Reg);
        m_Assembler.push(PAddr | 0xA0000000);
        m_Assembler.CallThis((uint32_t)(g_MMU), AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory", 12);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04300000:
        switch (PAddr)
        {
        case 0x04300000:
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Reg);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_MIPSInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_MIPSInterfaceHandler)[0][1], "MIPSInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0430000C:
            m_Assembler.MoveX86regToVariable(&CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue", Reg);
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
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Reg);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
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
                    asmjit::Label Jump;
                    m_Assembler.CompX86regToVariable(Reg, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JeLabel("Continue", Jump);
                    m_Assembler.MoveX86regToVariable(&g_Reg->VI_STATUS_REG, "VI_STATUS_REG", Reg);
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                break;
            case 0x04400004:
                m_Assembler.MoveX86regToVariable(&g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG", Reg);
                m_Assembler.AndConstToVariable(&g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG", 0xFFFFFF);
                break;
            case 0x04400008:
                if (g_Plugins->Gfx()->ViWidthChanged != nullptr)
                {
                    asmjit::Label Jump;
                    m_Assembler.CompX86regToVariable(Reg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    Jump = m_Assembler.newLabel();
                    m_Assembler.JeLabel("Continue", Jump);
                    m_Assembler.MoveX86regToVariable(&g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG", Reg);
                    m_RegWorkingSet.BeforeCallDirect();
                    m_Assembler.CallFunc((uint32_t)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    m_CodeBlock.Log("");
                    m_Assembler.bind(Jump);
                }
                break;
            case 0x0440000C: m_Assembler.MoveX86regToVariable(&g_Reg->VI_INTR_REG, "VI_INTR_REG", Reg); break;
            case 0x04400010:
                m_Assembler.AndConstToVariable(&g_Reg->MI_INTR_REG, "MI_INTR_REG", (uint32_t)~MI_INTR_VI);
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallThis((uint32_t)g_Reg, AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts", 4);
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04400014: m_Assembler.MoveX86regToVariable(&g_Reg->VI_BURST_REG, "VI_BURST_REG", Reg); break;
            case 0x04400018: m_Assembler.MoveX86regToVariable(&g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG", Reg); break;
            case 0x0440001C: m_Assembler.MoveX86regToVariable(&g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG", Reg); break;
            case 0x04400020: m_Assembler.MoveX86regToVariable(&g_Reg->VI_LEAP_REG, "VI_LEAP_REG", Reg); break;
            case 0x04400024: m_Assembler.MoveX86regToVariable(&g_Reg->VI_H_START_REG, "VI_H_START_REG", Reg); break;
            case 0x04400028: m_Assembler.MoveX86regToVariable(&g_Reg->VI_V_START_REG, "VI_V_START_REG", Reg); break;
            case 0x0440002C: m_Assembler.MoveX86regToVariable(&g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG", Reg); break;
            case 0x04400030: m_Assembler.MoveX86regToVariable(&g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG", Reg); break;
            case 0x04400034: m_Assembler.MoveX86regToVariable(&g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG", Reg); break;
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
        m_Assembler.push(0xFFFFFFFF);
        m_Assembler.push(Reg);
        m_Assembler.push(PAddr & 0x1FFFFFFF);
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
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Reg);
            m_Assembler.push(PAddr & 0x1FFFFFFF);
            m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_PeripheralInterfaceHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_PeripheralInterfaceHandler)[0][1], "PeripheralInterfaceHandler::Write32", 16);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600014:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", 0xFF);
            break;
        case 0x04600018:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", 0xFF);
            break;
        case 0x0460001C:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", 0xFF);
            break;
        case 0x04600020:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", 0xFF);
            break;
        case 0x04600024:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", 0xFF);
            break;
        case 0x04600028:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", 0xFF);
            break;
        case 0x0460002C:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", Reg);
            m_Assembler.AndConstToVariable(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", 0xFF);
            break;
        case 0x04600030:
            m_Assembler.MoveX86regToVariable(&g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", Reg);
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
        case 0x04700000: m_Assembler.MoveX86regToVariable(&g_Reg->RI_MODE_REG, "RI_MODE_REG", Reg); break;
        case 0x04700004: m_Assembler.MoveX86regToVariable(&g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG", Reg); break;
        case 0x0470000C: m_Assembler.MoveX86regToVariable(&g_Reg->RI_SELECT_REG, "RI_SELECT_REG", Reg); break;
        case 0x04700010: m_Assembler.MoveX86regToVariable(&g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG", Reg); break;
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
        case 0x04800000: m_Assembler.MoveX86regToVariable(&g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG", Reg); break;
        case 0x04800004:
            m_Assembler.MoveX86regToVariable(&g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG", Reg);
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.CallThis((uint32_t)(&g_MMU->m_PifRamHandler), AddressOf(&PifRamHandler::DMA_READ), "PifRamHandler::DMA_READ", 4);
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            m_Assembler.MoveX86regToVariable(&g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG", Reg);
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
            case 0x05000500: m_Assembler.MoveX86regToVariable(&g_Reg->ASIC_DATA, "ASIC_DATA", Reg); break;
            case 0x05000508:
            {
                // ASIC_CMD
                m_Assembler.MoveX86regToVariable(&g_Reg->ASIC_CMD, "ASIC_CMD", Reg);
                m_RegWorkingSet.BeforeCallDirect();
                m_Assembler.CallFunc(AddressOf(&DiskCommand), "DiskCommand");
                m_RegWorkingSet.AfterCallDirect();
                break;
            }
            case 0x05000510:
            {
                // ASIC_BM_CTL
                m_Assembler.MoveX86regToVariable(&g_Reg->ASIC_BM_CTL, "ASIC_BM_CTL", Reg);
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
            case 0x05000528: m_Assembler.MoveX86regToVariable(&g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE", Reg); break;
            case 0x05000530: m_Assembler.MoveX86regToVariable(&g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE", Reg); break;
            case 0x05000548: m_Assembler.MoveX86regToVariable(&g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL", Reg); break;
            }
            break;
        }
    case 0x13F00000:
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.push(0xFFFFFFFF);
        m_Assembler.push(Reg);
        m_Assembler.push(PAddr & 0x1FFFFFFF);
        m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_ISViewerHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_ISViewerHandler)[0][1], "ISViewerHandler::Write32", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x1FC00000:
        UpdateCounters(m_RegWorkingSet, false, true, false);
        m_RegWorkingSet.BeforeCallDirect();
        m_Assembler.push(0xFFFFFFFF);
        m_Assembler.push(Reg);
        m_Assembler.push(PAddr & 0x1FFFFFFF);
        m_Assembler.CallThis((uint32_t)(MemoryHandler *)&g_MMU->m_PifRamHandler, (uint32_t)((long **)(MemoryHandler *)&g_MMU->m_PifRamHandler)[0][1], "PifRamHandler::Write32", 16);
        m_RegWorkingSet.AfterCallDirect();
        break;
    default:
        if (PAddr >= 0x10000000 && PAddr < 0x13F00000)
        {
            m_RegWorkingSet.BeforeCallDirect();
            m_Assembler.push(0xFFFFFFFF);
            m_Assembler.push(Reg);
            m_Assembler.push(PAddr);
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
    if (m_RegWorkingSet.IsConst(MipsReg))
    {
        m_Assembler.MoveConstToVariable(&m_Reg.m_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], m_RegWorkingSet.GetMipsRegLo(MipsReg));
    }
    else if (m_RegWorkingSet.IsMapped(MipsReg))
    {
        m_Assembler.MoveX86regToVariable(&m_Reg.m_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], m_RegWorkingSet.GetMipsRegMapLo(MipsReg));
    }

    asmjit::x86::Gp MemoryStackReg = m_RegWorkingSet.Get_MemoryStack();
    if (MemoryStackReg.isValid())
    {
        m_CodeBlock.Log("    regcache: unallocate %s from memory stack", CX86Ops::x86_Name(MemoryStackReg));
        m_RegWorkingSet.SetX86Mapped(GetIndexFromX86Reg(MemoryStackReg), CRegInfo::NotMapped);
    }
    m_RegWorkingSet.BeforeCallDirect();
    m_Assembler.CallThis((uint32_t)g_Recompiler, AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos", 4);
    m_RegWorkingSet.AfterCallDirect();
}

#endif