#include "stdafx.h"

#if defined(__i386__) || defined(_M_IX86)

#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/CodeSection.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/Recompiler/SectionInfo.h>
#include <Project64-core/N64System/Recompiler/LoopAnalysis.h>
#include <Project64-core/N64System/Recompiler/x86/x86RecompilerOps.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/ExceptionHandler.h>
#include <Project64-core/Debugger.h>
#include <stdio.h>

CCodeSection * CX86RecompilerOps::m_Section = nullptr;
CRegInfo CX86RecompilerOps::m_RegWorkingSet;
PIPELINE_STAGE CX86RecompilerOps::m_PipelineStage;
uint32_t CX86RecompilerOps::m_CompilePC;
OPCODE CX86RecompilerOps::m_Opcode;
uint32_t CX86RecompilerOps::m_BranchCompare = 0;
uint32_t CX86RecompilerOps::m_TempValue32 = 0;

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

CX86RecompilerOps::CX86RecompilerOps(CMipsMemoryVM & MMU) :
    m_MMU(MMU)
{
}

CX86RecompilerOps::~CX86RecompilerOps()
{
}

void CX86RecompilerOps::PreCompileOpcode(void)
{
    if (m_PipelineStage != PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        CPU_Message("  %X %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));
    }
    /*if (m_CompilePC == 0x803275F4 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        X86BreakPoint(__FILE__, __LINE__);
    }

    /*if (m_CompilePC >= 0x80000000 && m_CompilePC <= 0x80400000 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem) {
    #ifdef _WIN32
    MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    PushImm32((uint32_t)g_BaseSystem);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    AddConstToX86Reg(x86_ESP, 4);
    #endif
    }
    }*/

    /*if ((m_CompilePC == 0x8031C0E4 || m_CompilePC == 0x8031C118 ||
    m_CompilePC == 0x8031CD88 ||  m_CompilePC == 0x8031CE24 ||
    m_CompilePC == 0x8031CE30 || m_CompilePC == 0x8031CE40) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        m_RegWorkingSet.WriteBackRegisters();
        UpdateCounters(m_RegWorkingSet, false, true);
        MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
        if (g_SyncSystem)
        {
#ifdef _WIN32
            MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
            PushImm32((uint32_t)g_BaseSystem);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
            AddConstToX86Reg(x86_ESP, 4);
#endif
        }
    }*/

    /*if (m_CompilePC == 0x801C1B88)
    {
    m_RegWorkingSet.BeforeCallDirect();
    Call_Direct(AddressOf(TestFunc), "TestFunc");
    m_RegWorkingSet.AfterCallDirect();
    }*/

    /*if (m_CompilePC >= 0x801C1AF8 && m_CompilePC <= 0x801C1C00 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    UpdateCounters(m_RegWorkingSet,false,true);
    MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
    if (g_SyncSystem) {
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToX86reg((uint32_t)g_BaseSystem,x86_ECX);
    Call_Direct(AddressOf(&CN64System::SyncSystemPC), "CN64System::SyncSystemPC");
    m_RegWorkingSet.AfterCallDirect();
    }
    }*/

    /*if ((m_CompilePC == 0x80263900) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    X86BreakPoint(__FILEW__,__LINE__);
    }*/

    /*if ((m_CompilePC >= 0x80325D80 && m_CompilePC <= 0x80325DF0) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
    if (g_SyncSystem) {
    #ifdef _WIN32
    MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    PushImm32((uint32_t)g_BaseSystem);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    AddConstToX86Reg(x86_ESP, 4);
    #endif
    }
    }*/
    /*if ((m_CompilePC == 0x80324E14) && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    X86BreakPoint(__FILEW__,__LINE__);
    }*/

    /*if (m_CompilePC == 0x80324E18 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
    if (g_SyncSystem) {
    #ifdef _WIN32
    MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    PushImm32((uint32_t)g_BaseSystem);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    AddConstToX86Reg(x86_ESP, 4);
    #endif
    }
    }*/
    /*if (m_CompilePC >= 0x80324E00 && m_CompilePC <= 0x80324E18 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet,false,true);
    MoveConstToVariable(m_CompilePC,&g_Reg->m_PROGRAM_COUNTER,"PROGRAM_COUNTER");
    if (g_SyncSystem) {
    #ifdef _WIN32
    MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    #else
    PushImm32((uint32_t)g_BaseSystem);
    Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
    AddConstToX86Reg(x86_ESP, 4);
    #endif
    }
    }*/
    /*        if (m_CompilePC == 0x803245CC && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    //m_RegWorkingSet.UnMap_AllFPRs();
    g_Notify->BreakPoint(__FILE__, __LINE__);
    //X86HardBreakPoint();
    //X86BreakPoint(__FILEW__,__LINE__);
    //m_RegWorkingSet.UnMap_AllFPRs();
    }*/
    /*if (m_CompilePC >= 0x80179DC4 && m_CompilePC <= 0x80179DF0 && m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
    m_RegWorkingSet.UnMap_AllFPRs();
    }*/

    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    m_RegWorkingSet.ResetX86Protection();
}

void CX86RecompilerOps::PostCompileOpcode(void)
{
    if (!g_System->bRegCaching()) { m_RegWorkingSet.WriteBackRegisters(); }
    m_RegWorkingSet.UnMap_AllFPRs();
}

void CX86RecompilerOps::CompileReadTLBMiss(uint32_t VirtualAddress, x86Reg LookUpReg)
{
    MoveConstToVariable(VirtualAddress, g_TLBLoadAddress, "TLBLoadAddress");
    CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBReadMiss, false, JeLabel32);
}

void CX86RecompilerOps::CompileReadTLBMiss(x86Reg AddressReg, x86Reg LookUpReg)
{
    MoveX86regToVariable(AddressReg, g_TLBLoadAddress, "TLBLoadAddress");
    CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBReadMiss, false, JeLabel32);
}

void CX86RecompilerOps::CompileWriteTLBMiss(x86Reg AddressReg, x86Reg LookUpReg)
{
    MoveX86regToVariable(AddressReg, &g_TLBStoreAddress, "g_TLBStoreAddress");
    CompConstToX86reg(LookUpReg, (uint32_t)-1);
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::TLBWriteMiss, false, JeLabel32);
}

bool DelaySlotEffectsCompare(uint32_t PC, uint32_t Reg1, uint32_t Reg2);

// Trap functions
void CX86RecompilerOps::Compile_TrapCompare(TRAP_COMPARE CompareType)
{
    void *FunctAddress = nullptr;
    const char *FunctName = nullptr;
    switch (CompareType)
    {
    case CompareTypeTEQ:
        FunctAddress = (void*)R4300iOp::SPECIAL_TEQ;
        FunctName = "R4300iOp::SPECIAL_TEQ";
        break;
    case CompareTypeTNE:
        FunctAddress = (void*)R4300iOp::SPECIAL_TNE;
        FunctName = "R4300iOp::SPECIAL_TNE";
        break;
    case CompareTypeTGE:
        FunctAddress = (void*)R4300iOp::SPECIAL_TGE;
        FunctName = "R4300iOp::SPECIAL_TGE";
        break;
    case CompareTypeTGEU:
        FunctAddress = (void*)R4300iOp::SPECIAL_TGEU;
        FunctName = "R4300iOp::SPECIAL_TGEU";
        break;
    case CompareTypeTLT:
        FunctAddress = (void*)R4300iOp::SPECIAL_TLT;
        FunctName = "R4300iOp::SPECIAL_TLT";
        break;
    case CompareTypeTLTU:
        FunctAddress = (void*)R4300iOp::SPECIAL_TLTU;
        FunctName = "R4300iOp::SPECIAL_TLTU";
        break;
    case CompareTypeTEQI:
        FunctAddress = (void*)R4300iOp::REGIMM_TEQI;
        FunctName = "R4300iOp::REGIMM_TEQI";
        break;
    case CompareTypeTNEI:
        FunctAddress = (void*)R4300iOp::REGIMM_TNEI;
        FunctName = "R4300iOp::REGIMM_TNEI";
        break;
    case CompareTypeTGEI:
        FunctAddress = (void*)R4300iOp::REGIMM_TGEI;
        FunctName = "R4300iOp::REGIMM_TGEI";
        break;
    case CompareTypeTGEIU:
        FunctAddress = (void*)R4300iOp::REGIMM_TGEIU;
        FunctName = "R4300iOp::REGIMM_TGEIU";
        break;
    case CompareTypeTLTI:
        FunctAddress = (void*)R4300iOp::REGIMM_TLTI;
        FunctName = "R4300iOp::REGIMM_TLTI";
        break;
    case CompareTypeTLTIU:
        FunctAddress = (void*)R4300iOp::REGIMM_TLTIU;
        FunctName = "R4300iOp::REGIMM_TLTIU";
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (FunctName != nullptr && FunctAddress != nullptr)
    {
        if (IsMapped(m_Opcode.rs)) {
            UnMap_GPR(m_Opcode.rs, true);
        }
        if (IsMapped(m_Opcode.rt)) {
            UnMap_GPR(m_Opcode.rt, true);
        }
        m_RegWorkingSet.BeforeCallDirect();
        MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
        Call_Direct(FunctAddress, FunctName);
        m_RegWorkingSet.AfterCallDirect();
    }
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

// Branch functions
void CX86RecompilerOps::Compile_BranchCompare(BRANCH_COMPARE CompareType)
{
    switch (CompareType)
    {
    case CompareTypeBEQ: BEQ_Compare(); break;
    case CompareTypeBNE: BNE_Compare(); break;
    case CompareTypeBLTZ: BLTZ_Compare(); break;
    case CompareTypeBLEZ: BLEZ_Compare(); break;
    case CompareTypeBGTZ: BGTZ_Compare(); break;
    case CompareTypeBGEZ: BGEZ_Compare(); break;
    case CompareTypeCOP1BCF: COP1_BCF_Compare(); break;
    case CompareTypeCOP1BCT: COP1_BCT_Compare(); break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86RecompilerOps::Compile_Branch(BRANCH_COMPARE CompareType, BRANCH_TYPE BranchType, bool Link)
{
    static CRegInfo RegBeforeDelay;
    static bool EffectDelaySlot;

    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if (CompareType == CompareTypeCOP1BCF || CompareType == CompareTypeCOP1BCT)
        {
            CompileCop1Test();
        }
        if (m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4 == m_CompilePC + 8)
        {
            return;
        }

        if ((m_CompilePC & 0xFFC) != 0xFFC)
        {
            switch (BranchType)
            {
            case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0); break;
            case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, m_Opcode.rt); break;
            case BranchTypeCop1:
                {
                    OPCODE Command;

                    if (!g_MMU->MemoryValue32(m_CompilePC + 4, Command.Hex))
                    {
                        g_Notify->FatalError(GS(MSG_FAIL_LOAD_WORD));
                    }

                    EffectDelaySlot = false;
                    if (Command.op == R4300i_CP1)
                    {
                        if ((Command.fmt == R4300i_COP1_S && (Command.funct & 0x30) == 0x30) ||
                            (Command.fmt == R4300i_COP1_D && (Command.funct & 0x30) == 0x30))
                        {
                            EffectDelaySlot = true;
                        }
                    }
                }
                break;
            default:
                if (HaveDebugger()) { g_Notify->DisplayError("Unknown branch type"); }
            }
        }
        else
        {
            EffectDelaySlot = true;
        }
        m_Section->m_Jump.JumpPC = m_CompilePC;
        m_Section->m_Jump.TargetPC = m_CompilePC + ((int16_t)m_Opcode.offset << 2) + 4;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", m_Section->m_JumpSection->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel.Format("Exit_%X_jump_%X", m_Section->m_EnterPC, m_Section->m_Jump.TargetPC);
        }
        m_Section->m_Jump.LinkLocation = nullptr;
        m_Section->m_Jump.LinkLocation2 = nullptr;
        m_Section->m_Jump.DoneDelaySlot = false;
        m_Section->m_Cont.JumpPC = m_CompilePC;
        m_Section->m_Cont.TargetPC = m_CompilePC + 8;
        if (m_Section->m_ContinueSection != nullptr)
        {
            m_Section->m_Cont.BranchLabel.Format("Section_%d", m_Section->m_ContinueSection->m_SectionID);
        }
        else
        {
            m_Section->m_Cont.BranchLabel.Format("Exit_%X_continue_%X", m_Section->m_EnterPC, m_Section->m_Cont.TargetPC);
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
        if (EffectDelaySlot)
        {
            if ((m_CompilePC & 0xFFC) != 0xFFC)
            {
                m_Section->m_Cont.BranchLabel = m_Section->m_ContinueSection != nullptr ? "Continue" : "ExitBlock";
                m_Section->m_Jump.BranchLabel = m_Section->m_JumpSection != nullptr ? "Jump" : "ExitBlock";
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
                    CPU_Message("");
                    CPU_Message("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    LinkJump(m_Section->m_Jump);
                    m_Section->m_Jump.FallThrough = true;
                }
                else if (m_Section->m_Cont.LinkLocation != nullptr)
                {
                    CPU_Message("");
                    CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
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
                    MoveConstToVariable(m_Section->m_Jump.TargetPC, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                }
                else if (m_Section->m_Cont.FallThrough)
                {
                    if (m_Section->m_Cont.LinkLocation != nullptr || m_Section->m_Cont.LinkLocation2 != nullptr)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                    }
                    MoveConstToVariable(m_Section->m_Cont.TargetPC, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                }

                if (m_Section->m_Jump.LinkLocation != nullptr || m_Section->m_Jump.LinkLocation2 != nullptr)
                {
                    JmpLabel8("DoDelaySlot", 0);
                    if (DelayLinkLocation != nullptr) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                    DelayLinkLocation = (uint8_t *)(*g_RecompPos - 1);

                    CPU_Message("      ");
                    CPU_Message("      %s:", m_Section->m_Jump.BranchLabel.c_str());
                    LinkJump(m_Section->m_Jump);
                    MoveConstToVariable(m_Section->m_Jump.TargetPC, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                }
                if (m_Section->m_Cont.LinkLocation != nullptr || m_Section->m_Cont.LinkLocation2 != nullptr)
                {
                    JmpLabel8("DoDelaySlot", 0);
                    if (DelayLinkLocation != nullptr) { g_Notify->BreakPoint(__FILE__, __LINE__); }
                    DelayLinkLocation = (uint8_t *)(*g_RecompPos - 1);

                    CPU_Message("      ");
                    CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
                    LinkJump(m_Section->m_Cont);
                    MoveConstToVariable(m_Section->m_Cont.TargetPC, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                }
                if (DelayLinkLocation)
                {
                    CPU_Message("");
                    CPU_Message("      DoDelaySlot:");
                    SetJump8(DelayLinkLocation, *g_RecompPos);
                }
                OverflowDelaySlot(false);
                return;
            }
            ResetX86Protection();
            RegBeforeDelay = m_RegWorkingSet;
        }
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        if (EffectDelaySlot)
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
                        m_Section->m_Jump.BranchLabel.Format("Section_%d", m_Section->m_JumpSection->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Jump.BranchLabel = "ExitBlock";
                    }
                    if (FallInfo->TargetPC <= m_CompilePC)
                    {
                        UpdateCounters(m_Section->m_Jump.RegSet, true, true);
                        CPU_Message("CompileSystemCheck 12");
                        CompileSystemCheck(FallInfo->TargetPC, m_Section->m_Jump.RegSet);
                        ResetX86Protection();
                        FallInfo->ExitReason = CExitInfo::Normal_NoSysCheck;
                        FallInfo->JumpPC = (uint32_t)-1;
                    }
                }
                else
                {
                    if (m_Section->m_ContinueSection != nullptr)
                    {
                        m_Section->m_Cont.BranchLabel.Format("Section_%d", m_Section->m_ContinueSection->m_SectionID);
                    }
                    else
                    {
                        m_Section->m_Cont.BranchLabel = "ExitBlock";
                    }
                }
                FallInfo->DoneDelaySlot = true;
                if (!JumpInfo->DoneDelaySlot)
                {
                    FallInfo->FallThrough = false;
                    JmpLabel32(FallInfo->BranchLabel.c_str(), 0);
                    FallInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);

                    if (JumpInfo->LinkLocation != nullptr)
                    {
                        CPU_Message("      %s:", JumpInfo->BranchLabel.c_str());
                        LinkJump(*JumpInfo);
                        JumpInfo->FallThrough = true;
                        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
                        m_RegWorkingSet = RegBeforeDelay;
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
                    m_Section->m_Cont.BranchLabel.Format("Section_%d", m_Section->m_ContinueSection->m_SectionID);
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

void CX86RecompilerOps::Compile_BranchLikely(BRANCH_COMPARE CompareType, bool Link)
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if (CompareType == CompareTypeCOP1BCF || CompareType == CompareTypeCOP1BCT)
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
            m_Section->m_Jump.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
        }
        else
        {
            m_Section->m_Jump.BranchLabel = "ExitBlock";
        }

        if (m_Section->m_ContinueSection != nullptr)
        {
            m_Section->m_Cont.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
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

                MoveConstToVariable(m_Section->m_Jump.TargetPC, &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                OverflowDelaySlot(false);
                CPU_Message("      ");
                CPU_Message("      %s:", m_Section->m_Cont.BranchLabel.c_str());
            }
            else if (!m_Section->m_Cont.FallThrough)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }

            LinkJump(m_Section->m_Cont);
            CompileExit(m_CompilePC, m_CompilePC + 8, m_Section->m_Cont.RegSet, CExitInfo::Normal, true, nullptr);
            return;
        }
        else
        {
            m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
        }

        if (g_System->bLinkBlocks())
        {
            m_Section->m_Jump.RegSet = m_RegWorkingSet;
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
    uint8_t *Jump = nullptr;

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
                CompX86RegToX86Reg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(x86_Any, m_Opcode.rs, true) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, true) : GetMipsRegMapHi(m_Opcode.rt)
                    );

                if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
                        CompConstToX86reg(Map_TempReg(x86_Any, MappedReg, true), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    CompConstToVariable((GetMipsRegLo_S(KnownReg) >> 31), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    ProtectGPR(KnownReg);
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        if (IsConst(KnownReg))
        {
            CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        else
        {
            CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);

            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");

                SetJump8(Jump, *g_RecompPos);
            }
        }
        else
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
        x86Reg Reg = x86_Any;

        if (!g_System->b32BitCore())
        {
            Reg = Map_TempReg(x86_Any, m_Opcode.rt, true);
            CompX86regToVariable(Reg, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }

        Reg = Map_TempReg(Reg, m_Opcode.rt, false);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        if (m_Section->m_Cont.FallThrough)
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
        }
        else
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
    uint8_t *Jump = nullptr;

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

                CompX86RegToX86Reg(
                    Is32Bit(m_Opcode.rs) ? Map_TempReg(x86_Any, m_Opcode.rs, true) : GetMipsRegMapHi(m_Opcode.rs),
                    Is32Bit(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, true) : GetMipsRegMapHi(m_Opcode.rt)
                    );
                if (m_Section->m_Cont.FallThrough)
                {
                    JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
                        CompConstToX86reg(Map_TempReg(x86_Any, MappedReg, true), GetMipsRegHi(ConstReg));
                    }
                    else
                    {
                        CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegLo_S(ConstReg) >> 31);
                    }
                }
                else
                {
                    CompConstToX86reg(GetMipsRegMapHi(MappedReg), GetMipsRegHi(ConstReg));
                }
                if (m_Section->m_Cont.FallThrough) {
                    JneLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    CPU_Message("      ");
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
            }
            else
            {
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (m_Section->m_Cont.FallThrough)
                {
                    JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                    m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else if (m_Section->m_Jump.FallThrough)
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                }
                else
                {
                    JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                    m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                    JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    CompConstToVariable(GetMipsRegLo_S(KnownReg) >> 31, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                ProtectGPR(KnownReg);
                if (Is64Bit(KnownReg))
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else if (IsSigned(KnownReg))
                {
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable(0, &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            if (m_Section->m_Cont.FallThrough)
            {
                JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        if (IsConst(KnownReg))
        {
            CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        else
        {
            CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
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
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else
    {
        x86Reg Reg = x86_Any;
        if (!g_System->b32BitCore())
        {
            Reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
            CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
            if (m_Section->m_Cont.FallThrough)
            {
                JneLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (m_Section->m_Cont.FallThrough)
        {
            JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            if (Jump)
            {
                CPU_Message("      ");
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
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
            JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            if (g_System->b32BitCore())
            {
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
        CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
        if (m_Section->m_Jump.FallThrough)
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else if (IsUnknown(m_Opcode.rs) && g_System->b32BitCore())
    {
        CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        if (m_Section->m_Jump.FallThrough)
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            JleLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
    else
    {
        uint8_t *Jump = nullptr;

        if (IsMapped(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JgLabel8("Continue", 0);
            Jump = *g_RecompPos - 1;
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JlLabel8("Continue", 0);
            Jump = *g_RecompPos - 1;
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JgLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }

        if (IsMapped(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            CPU_Message("      continue:");
            SetJump8(Jump, *g_RecompPos);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            CPU_Message("      continue:");
            SetJump8(Jump, *g_RecompPos);
        }
        else
        {
            JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
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
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JleLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            uint8_t *Jump = nullptr;

            if (IsMapped(m_Opcode.rs))
            {
                CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            }
            else
            {
                CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JlLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JgLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }

            if (IsMapped(m_Opcode.rs))
            {
                CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            }
            else
            {
                CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
            }
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                CPU_Message("      continue:");
                SetJump8(Jump, *g_RecompPos);
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32("BranchToJump", 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
    }
    else {
        uint8_t *Jump = nullptr;

        if (!g_System->b32BitCore())
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JlLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JgLabel8("Continue", 0);
                Jump = *g_RecompPos - 1;
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
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
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                if (Jump)
                {
                    CPU_Message("      continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
            }
            else
            {
                JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32("BranchToJump", 0);
                m_Section->m_Jump.LinkLocation2 = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
            if (m_Section->m_Jump.FallThrough)
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JleLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JgLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Jump.FallThrough)
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Cont.FallThrough)
            {
                JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        if (m_Section->m_Jump.FallThrough)
        {
            JgeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Cont.FallThrough)
        {
            JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            JlLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
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
            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
        }
        else if (IsSigned(m_Opcode.rs))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), 0);
            if (m_Section->m_Cont.FallThrough)
            {
                JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
                m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else if (m_Section->m_Jump.FallThrough)
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            }
            else
            {
                JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
                m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
                JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        }
        if (m_Section->m_Cont.FallThrough)
        {
            JgeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else if (m_Section->m_Jump.FallThrough)
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
        else
        {
            JlLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
            m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
            JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
            m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        }
    }
}

void CX86RecompilerOps::COP1_BCF_Compare()
{
    TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        JeLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else
    {
        JneLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
}

void CX86RecompilerOps::COP1_BCT_Compare()
{
    TestVariable(FPCSR_C, &_FPCR[31], "_FPCR[31]");
    if (m_Section->m_Cont.FallThrough)
    {
        JneLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
        m_Section->m_Jump.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else if (m_Section->m_Jump.FallThrough)
    {
        JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
    }
    else
    {
        JeLabel32(m_Section->m_Cont.BranchLabel.c_str(), 0);
        m_Section->m_Cont.LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(), 0);
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
            MoveConstToVariable((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
            OverflowDelaySlot(false);
            return;
        }

        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);;
        m_Section->m_Jump.JumpPC = m_CompilePC;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
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
        MoveVariableToX86reg(_PROGRAM_COUNTER, "_PROGRAM_COUNTER", GetMipsRegMapLo(31));
        AndConstToX86Reg(GetMipsRegMapLo(31), 0xF0000000);
        AddConstToX86Reg(GetMipsRegMapLo(31), (m_CompilePC + 8) & ~0xF0000000);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            MoveConstToVariable((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
            OverflowDelaySlot(false);
            return;
        }
        m_Section->m_Jump.TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
        m_Section->m_Jump.JumpPC = m_CompilePC;
        if (m_Section->m_JumpSection != nullptr)
        {
            m_Section->m_Jump.BranchLabel.Format("Section_%d", ((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
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

            x86Reg pc_reg = Map_TempReg(x86_Any, -1, false);
            MoveVariableToX86reg(_PROGRAM_COUNTER, "_PROGRAM_COUNTER", pc_reg);
            AndConstToX86Reg(pc_reg, 0xF0000000);
            AddConstToX86Reg(pc_reg, (m_Opcode.target << 2));
            MoveX86regToVariable(pc_reg, _PROGRAM_COUNTER, "_PROGRAM_COUNTER");

            uint32_t TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
            bool bCheck = TargetPC <= m_CompilePC;
            UpdateCounters(m_RegWorkingSet, bCheck, true);

            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, bCheck ? CExitInfo::Normal : CExitInfo::Normal_NoSysCheck, true, nullptr);
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
    if (m_Opcode.rt == 0) { return; }

    if (g_System->bFastSP() && m_Opcode.rs == 29 && m_Opcode.rt == 29)
    {
        AddConstToX86Reg(Map_MemoryStack(x86_Any, true), (int16_t)m_Opcode.immediate);
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
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
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
            AddConstToX86Reg(Map_MemoryStack(x86_Any, true), (int16_t)m_Opcode.immediate);
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
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), (int16_t)m_Opcode.immediate);
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
        uint32_t Result = Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) < ((unsigned)((int64_t)((int16_t)m_Opcode.immediate))) ? 1 : 0 :
            GetMipsRegLo(m_Opcode.rs) < ((unsigned)((int16_t)m_Opcode.immediate)) ? 1 : 0;
        UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            uint8_t * Jump[2];

            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;
            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], *g_RecompPos);
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
    }
    else if (g_System->b32BitCore())
    {
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        uint8_t * Jump = nullptr;

        CompConstToVariable(((int16_t)m_Opcode.immediate >> 31), &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        JneLabel8("CompareSet", 0);
        Jump = *g_RecompPos - 1;
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        CPU_Message("");
        CPU_Message("      CompareSet:");
        SetJump8(Jump, *g_RecompPos);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
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
        uint32_t Result = Is64Bit(m_Opcode.rs) ?
            ((int64_t)GetMipsReg(m_Opcode.rs) < (int64_t)((int16_t)m_Opcode.immediate) ? 1 : 0) :
            (GetMipsRegLo_S(m_Opcode.rs) < (int16_t)m_Opcode.immediate ? 1 : 0);

        UnMap_GPR(m_Opcode.rt, false);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, Result);
    }
    else if (IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            uint8_t * Jump[2];

            CompConstToX86reg(GetMipsRegMapHi(m_Opcode.rs), ((int16_t)m_Opcode.immediate >> 31));
            JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;
            SetlVariable(&m_BranchCompare, "m_BranchCompare");
            JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;
            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], *g_RecompPos);
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);
            SetbVariable(&m_BranchCompare, "m_BranchCompare");
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            /*    CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs),(int16_t)m_Opcode.immediate);
            SetlVariable(&m_BranchCompare,"m_BranchCompare");
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",GetMipsRegMapLo(m_Opcode.rt));
            */
            ProtectGPR(m_Opcode.rs);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rs), (int16_t)m_Opcode.immediate);

            if (GetMipsRegMapLo(m_Opcode.rt) > x86_EBX)
            {
                SetlVariable(&m_BranchCompare, "m_BranchCompare");
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
            }
            else
            {
                Setl(GetMipsRegMapLo(m_Opcode.rt));
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 1);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);

        if (GetMipsRegMapLo(m_Opcode.rt) > x86_EBX)
        {
            SetlVariable(&m_BranchCompare, "m_BranchCompare");
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            Setl(GetMipsRegMapLo(m_Opcode.rt));
            AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), 1);
        }
    }
    else
    {
        uint8_t * Jump[2] = { nullptr, nullptr };
        CompConstToVariable(((int16_t)m_Opcode.immediate >> 31), &_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs]);
        JeLabel8("Low Compare", 0);
        Jump[0] = *g_RecompPos - 1;
        SetlVariable(&m_BranchCompare, "m_BranchCompare");
        JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;
        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(Jump[0], *g_RecompPos);
        CompConstToVariable((int16_t)m_Opcode.immediate, &_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], *g_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rt, false, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rt));
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
        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate);
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
        OrConstToX86Reg(m_Opcode.immediate, Map_MemoryStack(x86_Any, true));
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
        OrConstToX86Reg(m_Opcode.immediate, GetMipsRegMapLo(m_Opcode.rt));
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
        OrConstToX86Reg(m_Opcode.immediate, GetMipsRegMapLo(m_Opcode.rt));
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
        if (m_Opcode.immediate != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), m_Opcode.immediate); }
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
        x86Reg Reg = Map_MemoryStack(x86_Any, true, false);
        uint32_t Address;

        m_MMU.VAddrToPAddr(((int16_t)m_Opcode.offset << 16), Address);
        if (Reg < 0)
        {
            MoveConstToVariable((uint32_t)(Address + g_MMU->Rdram()), &(g_Recompiler->MemoryStackPos()), "MemoryStack");
        }
        else
        {
            MoveConstToX86reg((uint32_t)(Address + g_MMU->Rdram()), Reg);
        }
    }

    UnMap_GPR(m_Opcode.rt, false);
    m_RegWorkingSet.SetMipsRegLo(m_Opcode.rt, ((int16_t)m_Opcode.offset << 16));
    m_RegWorkingSet.SetMipsRegState(m_Opcode.rt, CRegInfo::STATE_CONST_32_SIGN);
}

void CX86RecompilerOps::DADDIU()
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
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::DADDIU, "R4300iOp::DADDIU");
    m_RegWorkingSet.AfterCallDirect();
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
        PushImm32("CRecompiler::Remove_Cache", CRecompiler::Remove_Cache);
        PushImm32("0x20", 0x20);
        if (IsConst(m_Opcode.base))
        {
            uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
            PushImm32("Address", Address);
        }
        else if (IsMapped(m_Opcode.base))
        {
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            Push(GetMipsRegMapLo(m_Opcode.base));
        }
        else
        {
            MoveVariableToX86reg(&_GPR[m_Opcode.base].UW[0], CRegName::GPR_Lo[m_Opcode.base], x86_EAX);
            AddConstToX86Reg(x86_EAX, (int16_t)m_Opcode.offset);
            Push(x86_EAX);
        }
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_Recompiler, x86_ECX);
        Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
#else
        PushImm32((uint32_t)g_Recompiler);
        Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
        AddConstToX86Reg(x86_ESP, 16);
#endif
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
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::LDL, "R4300iOp::LDL");
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
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::LDR, "R4300iOp::LDR");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::LB_KnownAddress(x86Reg Reg, uint32_t VAddr, bool SignExtend)
{
    uint32_t PAddr;
    char VarName[100];

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        x86Reg TlbMappReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr >> 12, TlbMappReg);
        x86Reg AddrReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddrReg);
        MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TlbMappReg, TlbMappReg, 4);
        CompConstToX86reg(TlbMappReg, (uint32_t)-1);
        JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
        uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
        MoveConstToX86reg(VAddr >> 12, TlbMappReg);
        MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TlbMappReg, TlbMappReg, 4);
        CompileReadTLBMiss(AddrReg, TlbMappReg);
        AddConstToX86Reg(TlbMappReg, (uint32_t)m_MMU.Rdram());
        CPU_Message("");
        CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
        SetJump8(JumpFound, *g_RecompPos);
        if (SignExtend)
        {
            MoveSxByteX86regPointerToX86reg(AddrReg, TlbMappReg, Reg);
        }
        else
        {
            MoveZxByteX86regPointerToX86reg(AddrReg, TlbMappReg, Reg);
        }
        return;
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        MoveConstToX86reg(0, Reg);
        CPU_Message("%s\nFailed to translate address %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address %08X", __FUNCTION__, VAddr).c_str());
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
    case 0x10000000:
        sprintf(VarName, "RDRAM + %X", PAddr);
        if (SignExtend)
        {
            MoveSxVariableToX86regByte(PAddr + g_MMU->Rdram(), VarName, Reg);
        }
        else
        {
            MoveZxVariableToX86regByte(PAddr + g_MMU->Rdram(), VarName, Reg);
        }
        break;
    default:
        MoveConstToX86reg(0, Reg);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to compile address: %08X", __FUNCTION__, VAddr).c_str());
        }
    }
}

void CX86RecompilerOps::LH_KnownAddress(x86Reg Reg, uint32_t VAddr, bool SignExtend)
{
    char VarName[100];
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        x86Reg TlbMappReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr >> 12, TlbMappReg);
        x86Reg AddrReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddrReg);
        MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TlbMappReg, TlbMappReg, 4);
        CompConstToX86reg(TlbMappReg, (uint32_t)-1);
        JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
        uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
        MoveConstToX86reg(VAddr >> 12, TlbMappReg);
        MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TlbMappReg, TlbMappReg, 4);
        CompileReadTLBMiss(AddrReg, TlbMappReg);
        AddConstToX86Reg(TlbMappReg, (uint32_t)m_MMU.Rdram());
        CPU_Message("");
        CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
        SetJump8(JumpFound, *g_RecompPos);
        if (SignExtend)
        {
            MoveSxHalfX86regPointerToX86reg(AddrReg, TlbMappReg, Reg);
        }
        else
        {
            MoveZxHalfX86regPointerToX86reg(AddrReg, TlbMappReg, Reg);
        }
        return;
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        MoveConstToX86reg(0, Reg);
        CPU_Message("%s\nFailed to translate address %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address %08X", __FUNCTION__, VAddr).c_str());
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
    case 0x10000000:
        sprintf(VarName, "RDRAM + %X", PAddr);
        if (SignExtend)
        {
            MoveSxVariableToX86regHalf(PAddr + g_MMU->Rdram(), VarName, Reg);
        }
        else
        {
            MoveZxVariableToX86regHalf(PAddr + g_MMU->Rdram(), VarName, Reg);
        }
        break;
    default:
        MoveConstToX86reg(0, Reg);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to compile address: %08X", __FUNCTION__, VAddr).c_str());
        }
    }
}

void CX86RecompilerOps::LB()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset) ^ 3;
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
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }
    x86Reg AddressReg = BaseOffsetAddress(false), TempReg2;
    TestReadBreakpoint(AddressReg, (void *)x86TestReadBreakpoint8, "x86TestReadBreakpoint8");
    TempReg2 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    XorConstToX86Reg(AddressReg, 3);
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    MoveSxByteX86regPointerToX86reg(AddressReg, TempReg2, GetMipsRegMapLo(m_Opcode.rt));
}

void CX86RecompilerOps::LH()
{
    if (m_Opcode.rt == 0) return;

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset) ^ 2;
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
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }

    x86Reg AddressReg = BaseOffsetAddress(false);
    TestReadBreakpoint(AddressReg, (void *)x86TestReadBreakpoint16, "x86TestReadBreakpoint16");

    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    XorConstToX86Reg(AddressReg, 2);
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    MoveSxHalfX86regPointerToX86reg(AddressReg, TempReg2, GetMipsRegMapLo(m_Opcode.rt));
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
        x86Reg Value = Map_TempReg(x86_Any, -1, false);
        LW_KnownAddress(Value, (Address & ~3));
        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), R4300iOp::LWL_MASK[Offset]);
        ShiftLeftSignImmed(Value, (uint8_t)R4300iOp::LWL_SHIFT[Offset]);
        AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), Value);
        return;
    }

    PreReadInstruction();
    x86Reg shift = Map_TempReg(x86_ECX, -1, false);
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }
    x86Reg TempReg1 = BaseOffsetAddress(false);
    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(TempReg1, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);

    CompileReadTLBMiss(TempReg1, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    x86Reg OffsetReg = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(TempReg1, OffsetReg);
    AndConstToX86Reg(OffsetReg, 3);
    AndConstToX86Reg(TempReg1, (uint32_t)~3);
    TestReadBreakpoint(TempReg1, (void *)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");

    Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
    AndVariableDispToX86Reg((void *)R4300iOp::LWL_MASK, "LWL_MASK", GetMipsRegMapLo(m_Opcode.rt), OffsetReg, Multip_x4);
    MoveVariableDispToX86Reg((void *)R4300iOp::LWL_SHIFT, "LWL_SHIFT", shift, OffsetReg, 4);
    MoveX86regPointerToX86reg(TempReg1, TempReg2, TempReg1);
    ShiftLeftSign(TempReg1);
    AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), TempReg1);
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
        x86Reg TempReg1 = Map_MemoryStack(x86_Any, true);
        MoveVariableDispToX86Reg((void *)((uint32_t)(int16_t)m_Opcode.offset), stdstr_f("%Xh", (int16_t)m_Opcode.offset).c_str(), GetMipsRegMapLo(m_Opcode.rt), TempReg1, 1);
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
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        PreReadInstruction();
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        x86Reg TempReg1 = BaseOffsetAddress(true);
        TestReadBreakpoint(TempReg1, (void *)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
        x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
        MoveX86RegToX86Reg(TempReg1, TempReg2);
        ShiftRightUnsignImmed(TempReg2, 12);
        MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
        CompConstToX86reg(TempReg2, (uint32_t)-1);
        JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
        uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
        MoveX86RegToX86Reg(TempReg1, TempReg2);
        ShiftRightUnsignImmed(TempReg2, 12);
        MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
        CompileReadTLBMiss(TempReg1, TempReg2);
        AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
        CPU_Message("");
        CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
        SetJump8(JumpFound, *g_RecompPos);
        Map_GPR_32bit(m_Opcode.rt, ResultSigned, -1);
        MoveX86regPointerToX86reg(TempReg1, TempReg2, GetMipsRegMapLo(m_Opcode.rt));
        if (bRecordLLBit)
        {
            MoveConstToVariable(1, _LLBit, "LLBit");
        }
    }
    if (g_System->bFastSP() && m_Opcode.rt == 29)
    {
        ResetX86Protection();
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::LW_KnownAddress(x86Reg Reg, uint32_t VAddr)
{
    char VarName[100];
    uint32_t PAddr;

    m_RegWorkingSet.SetX86Protected(Reg, true);
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        x86Reg TlbMappReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr >> 12, TlbMappReg);
        MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TlbMappReg, TlbMappReg, 4);
        CompConstToX86reg(TlbMappReg, (uint32_t)-1);
        JneLabel8(stdstr_f("MemoryWriteMap_%X_Found", m_CompilePC).c_str(), 0);
        uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);

        MoveConstToX86reg(VAddr >> 12, TlbMappReg);
        MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TlbMappReg, TlbMappReg, 4);
        CompileReadTLBMiss(VAddr, TlbMappReg);
        AddConstToX86Reg(TlbMappReg, (uint32_t)m_MMU.Rdram());
        CPU_Message("");
        CPU_Message(stdstr_f("      MemoryWriteMap_%X_Found:", m_CompilePC).c_str());
        SetJump8(JumpFound, *g_RecompPos);
        AddConstToX86Reg(TlbMappReg, VAddr);
        MoveX86PointerToX86reg(Reg, TlbMappReg);
    }
    else
    {
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
            sprintf(VarName, "RDRAM + %X", PAddr);
            MoveVariableToX86reg(PAddr + g_MMU->Rdram(), VarName, Reg);
            break;
        case 0x04000000:
            if (PAddr < 0x04002000)
            {
                sprintf(VarName, "RDRAM + %X", PAddr);
                MoveVariableToX86reg(PAddr + g_MMU->Rdram(), VarName, Reg);
                break;
            }
            switch (PAddr)
            {
            case 0x04040010: MoveVariableToX86reg(&g_Reg->SP_STATUS_REG, "SP_STATUS_REG", Reg); break;
            case 0x04040014: MoveVariableToX86reg(&g_Reg->SP_DMA_FULL_REG, "SP_DMA_FULL_REG", Reg); break;
            case 0x04040018: MoveVariableToX86reg(&g_Reg->SP_DMA_BUSY_REG, "SP_DMA_BUSY_REG", Reg); break;
            case 0x0404001C:
                MoveVariableToX86reg(&g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG", Reg);
                MoveConstToVariable(1, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG");
                break;
            case 0x04080000: MoveVariableToX86reg(&g_Reg->SP_PC_REG, "SP_PC_REG", Reg); break;
            default:
                MoveConstToX86reg(0, Reg);
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
            }
            break;
        case 0x04100000:
             {
                 m_RegWorkingSet.BeforeCallDirect();
                 PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
                 PushImm32(PAddr | 0xA0000000);
#ifdef _MSC_VER
                 MoveConstToX86reg((uint32_t)(g_MMU), x86_ECX);
                 Call_Direct(AddressOf(&CMipsMemoryVM::LW_NonMemory), "CMipsMemoryVM::LW_NonMemory");
#else
                 PushImm32((uint32_t)(g_MMU));
                 Call_Direct(AddressOf(&CMipsMemoryVM::LW_NonMemory), "CMipsMemoryVM::LW_NonMemory");
                 AddConstToX86Reg(x86_ESP, 12);
#endif
                 m_RegWorkingSet.AfterCallDirect();
                 MoveVariableToX86reg(&m_TempValue32, "m_TempValue32", Reg);
            }
            break;
        case 0x04300000:
            switch (PAddr)
            {
            case 0x04300000: MoveVariableToX86reg(&g_Reg->MI_MODE_REG, "MI_MODE_REG", Reg); break;
            case 0x04300004: MoveVariableToX86reg(&g_Reg->MI_VERSION_REG, "MI_VERSION_REG", Reg); break;
            case 0x04300008: MoveVariableToX86reg(&g_Reg->MI_INTR_REG, "MI_INTR_REG", Reg); break;
            case 0x0430000C: MoveVariableToX86reg(&g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG", Reg); break;
            default:
                MoveConstToX86reg(0, Reg);
                if (ShowUnhandledMemory()) { g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str()); }
            }
            break;
        case 0x04400000:
            {
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
                UpdateCounters(m_RegWorkingSet, false, true);
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

                m_RegWorkingSet.BeforeCallDirect();
                PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
                PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
                MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, x86_ECX);
                Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][0], "VideoInterfaceHandler::Read32");
#else
                PushImm32((uint32_t)&g_MMU->m_VideoInterfaceHandler);
                Call_Direct(AddressOf(&VideoInterfaceHandler::Read32), "VideoInterfaceHandler::Read32");
                AddConstToX86Reg(x86_ESP, 16);
#endif
                m_RegWorkingSet.AfterCallDirect();
                MoveVariableToX86reg(&m_TempValue32, "m_TempValue32", Reg);
            }
            break;
        case 0x04500000:
            {
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
                UpdateCounters(m_RegWorkingSet, false, true);
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

                m_RegWorkingSet.BeforeCallDirect();
                PushImm32("m_TempValue32", (uint32_t)&m_TempValue32);
                PushImm32(PAddr & 0x1FFFFFFF);
    #ifdef _MSC_VER
                MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, x86_ECX);
                Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][0], "AudioInterfaceHandler::Read32");
    #else
                PushImm32((uint32_t)&g_MMU->m_AudioInterfaceHandler);
                Call_Direct(AddressOf(&AudioInterfaceHandler::Read32), "AudioInterfaceHandler::Read32");
                AddConstToX86Reg(x86_ESP, 16);
    #endif
                m_RegWorkingSet.AfterCallDirect();
                MoveVariableToX86reg(&m_TempValue32, "m_TempValue32", Reg);
            }
            break;
        case 0x04600000:
            switch (PAddr)
            {
            case 0x04600000: MoveVariableToX86reg(&g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG", Reg); break;
            case 0x04600004: MoveVariableToX86reg(&g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG", Reg); break;
            case 0x04600008: MoveVariableToX86reg(&g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG", Reg); break;
            case 0x0460000C: MoveVariableToX86reg(&g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG", Reg); break;
            case 0x04600010: MoveVariableToX86reg(&g_Reg->PI_STATUS_REG, "PI_STATUS_REG", Reg); break;
            case 0x04600014: MoveVariableToX86reg(&g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG", Reg); break;
            case 0x04600018: MoveVariableToX86reg(&g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG", Reg); break;
            case 0x0460001C: MoveVariableToX86reg(&g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG", Reg); break;
            case 0x04600020: MoveVariableToX86reg(&g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG", Reg); break;
            case 0x04600024: MoveVariableToX86reg(&g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG", Reg); break;
            case 0x04600028: MoveVariableToX86reg(&g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG", Reg); break;
            case 0x0460002C: MoveVariableToX86reg(&g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG", Reg); break;
            case 0x04600030: MoveVariableToX86reg(&g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG", Reg); break;
            default:
                MoveConstToX86reg(0, Reg);
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
            }
            break;
        case 0x04700000:
            switch (PAddr)
            {
            case 0x0470000C: MoveVariableToX86reg(&g_Reg->RI_SELECT_REG, "RI_SELECT_REG", Reg); break;
            case 0x04700010: MoveVariableToX86reg(&g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG", Reg); break;
            default:
                MoveConstToX86reg(0, Reg);
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
            }
            break;
        case 0x04800000:
            switch (PAddr)
            {
            case 0x04800018: MoveVariableToX86reg(&g_Reg->SI_STATUS_REG, "SI_STATUS_REG", Reg); break;
            default:
                MoveConstToX86reg(0, Reg);
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                }
            }
            break;
        case 0x05000000:
            // 64DD registers
            if (EnableDisk())
            {
                switch (PAddr)
                {
                case 0x05000500: MoveVariableToX86reg(&g_Reg->ASIC_DATA, "ASIC_DATA", Reg); break;
                case 0x05000504: MoveVariableToX86reg(&g_Reg->ASIC_MISC_REG, "ASIC_MISC_REG", Reg); break;
                case 0x05000508:
                    MoveVariableToX86reg(&g_Reg->ASIC_STATUS, "ASIC_STATUS", Reg);
                    m_RegWorkingSet.BeforeCallDirect();
                    Call_Direct(AddressOf(&DiskGapSectorCheck), "DiskGapSectorCheck");
                    m_RegWorkingSet.AfterCallDirect();
                    break;
                case 0x0500050C: MoveVariableToX86reg(&g_Reg->ASIC_CUR_TK, "ASIC_CUR_TK", Reg); break;
                case 0x05000510: MoveVariableToX86reg(&g_Reg->ASIC_BM_STATUS, "ASIC_BM_STATUS", Reg); break;
                case 0x05000514: MoveVariableToX86reg(&g_Reg->ASIC_ERR_SECTOR, "ASIC_ERR_SECTOR", Reg); break;
                case 0x05000518: MoveVariableToX86reg(&g_Reg->ASIC_SEQ_STATUS, "ASIC_SEQ_STATUS", Reg); break;
                case 0x0500051C: MoveVariableToX86reg(&g_Reg->ASIC_CUR_SECTOR, "ASIC_CUR_SECTOR", Reg); break;
                case 0x05000520: MoveVariableToX86reg(&g_Reg->ASIC_HARD_RESET, "ASIC_HARD_RESET", Reg); break;
                case 0x05000524: MoveVariableToX86reg(&g_Reg->ASIC_C1_S0, "ASIC_C1_S0", Reg); break;
                case 0x05000528: MoveVariableToX86reg(&g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE", Reg); break;
                case 0x0500052C: MoveVariableToX86reg(&g_Reg->ASIC_C1_S2, "ASIC_C1_S2", Reg); break;
                case 0x05000530: MoveVariableToX86reg(&g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE", Reg); break;
                case 0x05000534: MoveVariableToX86reg(&g_Reg->ASIC_C1_S4, "ASIC_C1_S4", Reg); break;
                case 0x05000538: MoveVariableToX86reg(&g_Reg->ASIC_C1_S6, "ASIC_C1_S6", Reg); break;
                case 0x0500053C: MoveVariableToX86reg(&g_Reg->ASIC_CUR_ADDR, "ASIC_CUR_ADDR", Reg); break;
                case 0x05000540: MoveVariableToX86reg(&g_Reg->ASIC_ID_REG, "ASIC_ID_REG", Reg); break;
                case 0x05000544: MoveVariableToX86reg(&g_Reg->ASIC_TEST_REG, "ASIC_TEST_REG", Reg); break;
                case 0x05000548: MoveVariableToX86reg(&g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL", Reg); break;
                default:
                    MoveConstToX86reg(0, Reg);
                    if (ShowUnhandledMemory())
                    {
                        g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
                    }
                }
            }
            else
            {
                MoveConstToX86reg((uint32_t)((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF), Reg);
            }
            break;
        case 0x1FC00000:
            sprintf(VarName, "RDRAM + %X", PAddr);
            MoveVariableToX86reg(PAddr + g_MMU->Rdram(), VarName, Reg);
            break;
        default:
            if ((PAddr & 0xF0000000) == 0x10000000 && (PAddr - 0x10000000) < g_Rom->GetRomSize())
            {
                // Read from ROM
                sprintf(VarName, "Rom + %X", (PAddr - 0x10000000));
                MoveVariableToX86reg((PAddr - 0x10000000) + g_Rom->GetRomAddress(), VarName, Reg);
            }
            else if (g_DDRom != nullptr && ((PAddr & 0xFF000000) == 0x06000000 && (PAddr - 0x06000000) < g_DDRom->GetRomSize()))
            {
                // Read from DDROM (TODO: Is DDROM a disk image or the IPL?)
                sprintf(VarName, "RDRAM + %X", PAddr);
                MoveVariableToX86reg(PAddr + g_MMU->Rdram(), VarName, Reg);
            }
            else
            {
                MoveConstToX86reg(((PAddr & 0xFFFF) << 16) | (PAddr & 0xFFFF), Reg);
                if (ShowUnhandledMemory())
                {
                    CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
                    g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
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
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset) ^ 3;
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
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }
    x86Reg AddressReg = BaseOffsetAddress(false);
    TestReadBreakpoint(AddressReg, (void *)x86TestReadBreakpoint8, "x86TestReadBreakpoint8");
    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    XorConstToX86Reg(AddressReg, 3);
    Map_GPR_32bit(m_Opcode.rt, false, -1);
    MoveZxByteX86regPointerToX86reg(AddressReg, TempReg2, GetMipsRegMapLo(m_Opcode.rt));
}

void CX86RecompilerOps::LHU()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = (GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset) ^ 2;
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
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }

    x86Reg AddressReg = BaseOffsetAddress(false), TempReg2;
    TestReadBreakpoint(AddressReg, (void *)x86TestReadBreakpoint16, "x86TestReadBreakpoint16");
    TempReg2 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    XorConstToX86Reg(AddressReg, 2);
    Map_GPR_32bit(m_Opcode.rt, false, -1);
    MoveZxHalfX86regPointerToX86reg(AddressReg, TempReg2, GetMipsRegMapLo(m_Opcode.rt));
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
        x86Reg Value = Map_TempReg(x86_Any, -1, false);
        LW_KnownAddress(Value, (Address & ~3));
        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rt), R4300iOp::LWR_MASK[Offset]);
        ShiftRightUnsignImmed(Value, (uint8_t)R4300iOp::LWR_SHIFT[Offset]);
        AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), Value);
        return;
    }

    PreReadInstruction();
    x86Reg shift = Map_TempReg(x86_ECX, -1, false);
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }
    x86Reg TempReg1 = BaseOffsetAddress(false);

    TestReadBreakpoint(TempReg1, (void *)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    x86Reg OffsetReg = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(TempReg1, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(TempReg1, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(TempReg1, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);

    MoveX86RegToX86Reg(TempReg1, OffsetReg);
    AndConstToX86Reg(OffsetReg, 3);
    AndConstToX86Reg(TempReg1, (uint32_t)~3);

    Map_GPR_32bit(m_Opcode.rt, true, m_Opcode.rt);
    AndVariableDispToX86Reg((void *)R4300iOp::LWR_MASK, "R4300iOp::LWR_MASK", GetMipsRegMapLo(m_Opcode.rt), OffsetReg, Multip_x4);
    MoveVariableDispToX86Reg((void *)R4300iOp::LWR_SHIFT, "R4300iOp::LWR_SHIFT", shift, OffsetReg, 4);
    MoveX86regPointerToX86reg(TempReg1, TempReg2, TempReg1);
    ShiftRightUnsign(TempReg1);
    AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), TempReg1);
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
            SB_Const((uint8_t)(GetMipsRegLo(m_Opcode.rt) & 0xFF), Address);
        }
        else if (IsMapped(m_Opcode.rt) && Is8BitReg(GetMipsRegMapLo(m_Opcode.rt)))
        {
            SB_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SB_Register(Map_TempReg(x86_Any8Bit, m_Opcode.rt, false), Address);
        }
        return;
    }
    PreWriteInstruction();

    x86Reg ValueReg = x86_Unknown;
    if (!IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(x86_Any8Bit, m_Opcode.rt, false) : GetMipsRegMapLo(m_Opcode.rt);
        if (IsMapped(m_Opcode.rt) && !Is8BitReg(ValueReg))
        {
            UnProtectGPR(m_Opcode.rt);
            ValueReg = Map_TempReg(x86_Any8Bit, m_Opcode.rt, false);
        }
    }
    x86Reg AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint8, "x86TestWriteBreakpoint8");
    CompileStoreMemoryValue(AddressReg, ValueReg, x86_Unknown, GetMipsRegLo(m_Opcode.rt), 8);
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
            SH_Const((uint16_t)(GetMipsRegLo(m_Opcode.rt) & 0xFFFF), Address);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SH_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
        }
        else
        {
            SH_Register(Map_TempReg(x86_Any, m_Opcode.rt, false), Address);
        }
        return;
    }

    PreWriteInstruction();

    x86Reg ValueReg = x86_Unknown;
    if (!IsConst(m_Opcode.rt))
    {
        if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
        }
        ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, false) : GetMipsRegMapLo(m_Opcode.rt);
    }
    x86Reg AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint16, "x86TestWriteBreakpoint16");
    CompileStoreMemoryValue(AddressReg, ValueReg, x86_Unknown, GetMipsRegLo(m_Opcode.rt), 16);
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

        x86Reg Value = Map_TempReg(x86_Any, -1, false);
        LW_KnownAddress(Value, (Address & ~3));
        AndConstToX86Reg(Value, R4300iOp::SWL_MASK[Offset]);
        x86Reg TempReg1 = Map_TempReg(x86_Any, m_Opcode.rt, false);
        ShiftRightUnsignImmed(TempReg1, (uint8_t)R4300iOp::SWL_SHIFT[Offset]);
        AddX86RegToX86Reg(Value, TempReg1);
        SW_Register(Value, (Address & ~3));
        return;
    }
    PreWriteInstruction();
    x86Reg shift = Map_TempReg(x86_ECX, -1, false), AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");

    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    x86Reg OffsetReg = Map_TempReg(x86_Any, -1, false);
    x86Reg ValueReg = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    MoveX86RegToX86Reg(AddressReg, OffsetReg);
    AndConstToX86Reg(OffsetReg, 3);
    AndConstToX86Reg(AddressReg, (uint32_t)~3);
    MoveX86regPointerToX86reg(AddressReg, TempReg2, ValueReg);

    AndVariableDispToX86Reg((void *)R4300iOp::SWL_MASK, "R4300iOp::SWL_MASK", ValueReg, OffsetReg, Multip_x4);
    if (!IsConst(m_Opcode.rt) || GetMipsRegLo(m_Opcode.rt) != 0)
    {
        MoveVariableDispToX86Reg((void *)R4300iOp::SWL_SHIFT, "R4300iOp::SWL_SHIFT", shift, OffsetReg, 4);
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToX86reg(GetMipsRegLo(m_Opcode.rt), OffsetReg);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), OffsetReg);
        }
        else
        {
            MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], OffsetReg);
        }
        ShiftRightUnsign(OffsetReg);
        AddX86RegToX86Reg(ValueReg, OffsetReg);
    }

    CompileStoreMemoryValue(AddressReg, ValueReg, x86_Unknown, 0, 32);
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
        x86Reg TempReg1 = Map_MemoryStack(x86_Any, true);

        if (IsConst(m_Opcode.rt))
        {
            MoveConstToMemoryDisp(GetMipsRegLo(m_Opcode.rt), TempReg1, (uint32_t)((int16_t)m_Opcode.offset));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToMemory(GetMipsRegMapLo(m_Opcode.rt), TempReg1, (uint32_t)((int16_t)m_Opcode.offset));
        }
        else
        {
            x86Reg TempReg2 = Map_TempReg(x86_Any, m_Opcode.rt, false);
            MoveX86regToMemory(TempReg2, TempReg1, (uint32_t)((int16_t)m_Opcode.offset));
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
                SW_Register(GetMipsRegMapLo(m_Opcode.rt), Address);
            }
            else
            {
                SW_Register(Map_TempReg(x86_Any, m_Opcode.rt, false), Address);
            }
            return;
        }

        PreWriteInstruction();
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        uint8_t * JumpLLBit = nullptr;
        if (bCheckLLbit)
        {
            CompConstToVariable(1, _LLBit, "_LLBit");
            JneLabel8("LLBit_Continue", 0);
            JumpLLBit = *g_RecompPos - 1;
        }

        x86Reg ValueReg = x86_Unknown;
        if (!IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rt))
            {
                ProtectGPR(m_Opcode.rt);
            }
            ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, false) : GetMipsRegMapLo(m_Opcode.rt);
        }
        x86Reg AddressReg = BaseOffsetAddress(true);
        Compile_StoreInstructClean(AddressReg, 4);
        TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");
        CompileStoreMemoryValue(AddressReg, ValueReg, x86_Unknown, GetMipsRegLo(m_Opcode.rt), 32);
        if (bCheckLLbit)
        {
            CPU_Message("      ");
            CPU_Message("      LLBit_Continue:");
            SetJump8(JumpLLBit, *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rt, false, -1);
            MoveVariableToX86reg(_LLBit, "_LLBit", GetMipsRegMapLo(m_Opcode.rt));
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

        x86Reg Value = Map_TempReg(x86_Any, -1, false);
        LW_KnownAddress(Value, (Address & ~3));
        AndConstToX86Reg(Value, R4300iOp::SWR_MASK[Offset]);
        x86Reg TempReg = Map_TempReg(x86_Any, m_Opcode.rt, false);
        ShiftLeftSignImmed(TempReg, (uint8_t)R4300iOp::SWR_SHIFT[Offset]);
        AddX86RegToX86Reg(Value, TempReg);
        SW_Register(Value, (Address & ~3));
        return;
    }
    PreWriteInstruction();
    x86Reg shift = Map_TempReg(x86_ECX, -1, false);
    x86Reg AddressReg = BaseOffsetAddress(false);
    TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint32, "x86TestWriteBreakpoint32");
    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    x86Reg OffsetReg = Map_TempReg(x86_Any, -1, false);
    x86Reg ValueReg = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddressReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);

    MoveX86RegToX86Reg(AddressReg, OffsetReg);
    AndConstToX86Reg(OffsetReg, 3);
    AndConstToX86Reg(AddressReg, (uint32_t)~3);

    MoveX86regPointerToX86reg(AddressReg, TempReg2, ValueReg);

    AndVariableDispToX86Reg((void *)R4300iOp::SWR_MASK, "R4300iOp::SWR_MASK", ValueReg, OffsetReg, Multip_x4);
    if (!IsConst(m_Opcode.rt) || GetMipsRegLo(m_Opcode.rt) != 0)
    {
        MoveVariableDispToX86Reg((void *)R4300iOp::SWR_SHIFT, "R4300iOp::SWR_SHIFT", shift, OffsetReg, 4);
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToX86reg(GetMipsRegLo(m_Opcode.rt), OffsetReg);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), OffsetReg);
        }
        else
        {
            MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[0], CRegName::GPR_Lo[m_Opcode.rt], OffsetReg);
        }
        ShiftLeftSign(OffsetReg);
        AddX86RegToX86Reg(ValueReg, OffsetReg);
    }

    CompileStoreMemoryValue(AddressReg, ValueReg, x86_Unknown, 0, 32);
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
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SDL, "R4300iOp::SDL");
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
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SDR, "R4300iOp::SDR");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::LL()
{
    LW(true, true);
}

void CX86RecompilerOps::LWC1()
{
    char Name[50];

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
        x86Reg TempReg1 = Map_TempReg(x86_Any, -1, false);
        LW_KnownAddress(TempReg1, Address);

        x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg(&_FPR_S[m_Opcode.ft], Name, TempReg2);
        MoveX86regToX86Pointer(TempReg1, TempReg2);
        return;
    }
    PreReadInstruction();
    x86Reg AddrReg = BaseOffsetAddress(true);
    TestReadBreakpoint(AddrReg, (void *)x86TestReadBreakpoint32, "x86TestReadBreakpoint32");
    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    x86Reg TempReg3 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddrReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddrReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddrReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);

    MoveX86regPointerToX86reg(AddrReg, TempReg2, TempReg3);
    sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
    MoveVariableToX86reg(&_FPR_S[m_Opcode.ft], Name, TempReg2);
    MoveX86regToX86Pointer(TempReg3, TempReg2);
}

void CX86RecompilerOps::LDC1()
{
    char Name[50];

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
        x86Reg TempReg1 = Map_TempReg(x86_Any, -1, false);
        LW_KnownAddress(TempReg1, Address);

        x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg(&_FPR_D[m_Opcode.ft], Name, TempReg2);
        AddConstToX86Reg(TempReg2, 4);
        MoveX86regToX86Pointer(TempReg1, TempReg2);

        LW_KnownAddress(TempReg1, Address + 4);
        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg(&_FPR_D[m_Opcode.ft], Name, TempReg2);
        MoveX86regToX86Pointer(TempReg1, TempReg2);
        return;
    }
    PreReadInstruction();
    x86Reg AddrReg = BaseOffsetAddress(true);
    TestReadBreakpoint(AddrReg, (void *)x86TestReadBreakpoint64, "x86TestReadBreakpoint64");
    x86Reg TempReg2 = Map_TempReg(x86_Any, -1, false);
    x86Reg TempReg3 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddrReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddrReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddrReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    MoveX86regPointerToX86reg(AddrReg, TempReg2, TempReg3);
    Push(TempReg2);
    sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
    MoveVariableToX86reg(&_FPR_D[m_Opcode.ft], Name, TempReg2);
    AddConstToX86Reg(TempReg2, 4);
    MoveX86regToX86Pointer(TempReg3, TempReg2);
    Pop(TempReg2);
    MoveX86regPointerToX86regDisp8(AddrReg, TempReg2, TempReg3, 4);
    sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
    MoveVariableToX86reg(&_FPR_D[m_Opcode.ft], Name, TempReg2);
    MoveX86regToX86Pointer(TempReg3, TempReg2);
}

void CX86RecompilerOps::LD()
{
    if (m_Opcode.rt == 0)
    {
        return;
    }

    x86Reg TempReg2;

    if (IsConst(m_Opcode.base))
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
        return;
    }
    PreReadInstruction();
    if (IsMapped(m_Opcode.rt))
    {
        ProtectGPR(m_Opcode.rt);
    }
    x86Reg AddrReg = BaseOffsetAddress(true);
    TestReadBreakpoint(AddrReg, (void *)x86TestReadBreakpoint64, "x86TestReadBreakpoint64");
    TempReg2 = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddrReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg2, TempReg2, 4);
    CompConstToX86reg(TempReg2, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryReadMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddrReg, TempReg2);
    ShiftRightUnsignImmed(TempReg2, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_ReadMap, "MMU->TLB_ReadMap", TempReg2, TempReg2, 4);
    CompileReadTLBMiss(AddrReg, TempReg2);
    AddConstToX86Reg(TempReg2, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryReadMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);
    Map_GPR_64bit(m_Opcode.rt, -1);
    MoveX86regPointerToX86reg(AddrReg, TempReg2, GetMipsRegMapHi(m_Opcode.rt));
    MoveX86regPointerToX86regDisp8(AddrReg, TempReg2, GetMipsRegMapLo(m_Opcode.rt), 4);
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
    char Name[50];

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
        x86Reg TempReg1 = Map_TempReg(x86_Any, -1, false);

        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg(&_FPR_S[m_Opcode.ft], Name, TempReg1);
        MoveX86PointerToX86reg(TempReg1, TempReg1);
        SW_Register(TempReg1, Address);
        return;
    }
    PreWriteInstruction();
    UnMap_FPR(m_Opcode.ft, true);
    x86Reg ValueReg = Map_TempReg(x86_Any, -1, false);
    MoveVariableToX86reg(&_FPR_S[m_Opcode.ft], stdstr_f("_FPR_S[%d]", m_Opcode.ft).c_str() , ValueReg);
    MoveX86PointerToX86reg(ValueReg, ValueReg);

    x86Reg AddressReg = BaseOffsetAddress(false);
    Compile_StoreInstructClean(AddressReg, 8);
    TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint64, "x86TestWriteBreakpoint32");
    CompileStoreMemoryValue(AddressReg, ValueReg, x86_Unknown, 0, 32);
}

void CX86RecompilerOps::SDC1()
{
    char Name[50];

    CompileCop1Test();

    if (IsConst(m_Opcode.base))
    {
        uint32_t Address = GetMipsRegLo(m_Opcode.base) + (int16_t)m_Opcode.offset;
        if (HaveWriteBP() && g_Debugger->WriteBP32(Address))
        {
            FoundMemoryBreakpoint();
            return;
        }

        x86Reg TempReg1 = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.ft], Name, TempReg1);
        AddConstToX86Reg(TempReg1, 4);
        MoveX86PointerToX86reg(TempReg1, TempReg1);
        SW_Register(TempReg1, Address);

        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg(&_FPR_D[m_Opcode.ft], Name, TempReg1);
        MoveX86PointerToX86reg(TempReg1, TempReg1);
        SW_Register(TempReg1, Address + 4);
        return;
    }
    PreWriteInstruction();
    UnMap_FPR(m_Opcode.ft, true);
    x86Reg ValueRegHi = Map_TempReg(x86_Any, -1, false), ValueRegLo = Map_TempReg(x86_Any, -1, false);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.ft], stdstr_f("_FPR_D[%d]", m_Opcode.ft).c_str(), ValueRegHi);
    MoveX86RegToX86Reg(ValueRegHi, ValueRegLo);
    AddConstToX86Reg(ValueRegHi, 4);
    MoveX86PointerToX86reg(ValueRegHi, ValueRegHi);
    MoveX86PointerToX86reg(ValueRegLo, ValueRegLo);

    x86Reg AddressReg = BaseOffsetAddress(false);
    Compile_StoreInstructClean(AddressReg, 8);
    TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint64, "x86TestWriteBreakpoint64");
    CompileStoreMemoryValue(AddressReg, ValueRegLo, ValueRegHi, 0, 64);
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
            SW_Register(Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true), Address);
            SW_Register(GetMipsRegMapLo(m_Opcode.rt), Address + 4);
        }
        else
        {
            x86Reg TempReg1 = Map_TempReg(x86_Any, m_Opcode.rt, true);
            SW_Register(TempReg1, Address);
            SW_Register(Map_TempReg(TempReg1, m_Opcode.rt, false), Address + 4);
        }
    }
    else
    {
        PreWriteInstruction();
        x86Reg ValueReg = x86_Unknown;
        if (!IsConst(m_Opcode.rt))
        {
            if (IsMapped(m_Opcode.rt))
            {
                ProtectGPR(m_Opcode.rt);
            }
            ValueReg = IsUnknown(m_Opcode.rt) ? Map_TempReg(x86_Any, m_Opcode.rt, false) : GetMipsRegMapLo(m_Opcode.rt);
        }
        uint64_t RtValue = 0;
        x86Reg ValueRegHi = x86_Unknown, ValueRegLo = x86_Unknown;
        if (IsConst(m_Opcode.rt))
        {
            RtValue = ((uint64_t)(Is64Bit(m_Opcode.rt) ? GetMipsRegHi(m_Opcode.rt) : (uint32_t)(GetMipsRegLo_S(m_Opcode.rt) >> 31)) << 32) | GetMipsRegLo(m_Opcode.rt);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            ProtectGPR(m_Opcode.rt);
            ValueRegHi = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true);
            ValueRegLo = GetMipsRegMapLo(m_Opcode.rt);
        }
        else
        {
            ValueRegHi = Map_TempReg(x86_Any, m_Opcode.rt, true);
            ValueRegLo = Map_TempReg(x86_Any, m_Opcode.rt, false);
        }
        
        x86Reg AddressReg = BaseOffsetAddress(false);
        Compile_StoreInstructClean(AddressReg, 8);
        TestWriteBreakpoint(AddressReg, (void *)x86TestWriteBreakpoint64, "x86TestWriteBreakpoint64");
        CompileStoreMemoryValue(AddressReg, ValueReg, ValueRegHi, RtValue, 64);
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
            LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, Multip_x2);
            break;
        case 2:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, Multip_x4);
            break;
        case 3:
            ProtectGPR(m_Opcode.rt);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            LeaRegReg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt), 0, Multip_x8);
            break;
        default:
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
    else
    {
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
    ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
    ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
            ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        }
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftLeftSign(GetMipsRegMapLo(m_Opcode.rd));
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
        ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }

    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));
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
        ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)Shift);
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
    ShiftRightSign(GetMipsRegMapLo(m_Opcode.rd));
}

void CX86RecompilerOps::SPECIAL_JR()
{
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL)
    {
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
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

        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
        }
        m_PipelineStage = PIPELINE_STAGE_DO_DELAY_SLOT;
    }
    else if (m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT_DONE)
    {
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, nullptr);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, nullptr);
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
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0) && (m_CompilePC & 0xFFC) != 0xFFC)
        {
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
        }
        UnMap_GPR(m_Opcode.rd, false);
        m_RegWorkingSet.SetMipsRegLo(m_Opcode.rd, m_CompilePC + 8);
        m_RegWorkingSet.SetMipsRegState(m_Opcode.rd, CRegInfo::STATE_CONST_32_SIGN);
        if ((m_CompilePC & 0xFFC) == 0xFFC)
        {
            if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
                m_RegWorkingSet.WriteBackRegisters();
            }
            else
            {
                m_RegWorkingSet.WriteBackRegisters();
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), &g_System->m_JumpToLocation, "System::m_JumpToLocation");
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
        if (DelaySlotEffectsCompare(m_CompilePC, m_Opcode.rs, 0))
        {
            CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, nullptr);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            if (IsConst(m_Opcode.rs))
            {
                MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else if (IsMapped(m_Opcode.rs))
            {
                MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            else
            {
                MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, false), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
            }
            CompileExit((uint32_t)-1, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, nullptr);
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
    CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::DoSysCall, true, nullptr);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::SPECIAL_MFLO()
{
    if (m_Opcode.rd == 0) { return; }

    Map_GPR_64bit(m_Opcode.rd, -1);
    MoveVariableToX86reg(&_RegLO->UW[0], "_RegLO->UW[0]", GetMipsRegMapLo(m_Opcode.rd));
    MoveVariableToX86reg(&_RegLO->UW[1], "_RegLO->UW[1]", GetMipsRegMapHi(m_Opcode.rd));
}

void CX86RecompilerOps::SPECIAL_MTLO()
{
    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveConstToVariable(GetMipsRegHi(m_Opcode.rs), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs) && ((GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            MoveConstToVariable(0xFFFFFFFF, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
    else if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.rs), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs))
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, true), &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
    else
    {
        x86Reg reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        MoveX86regToVariable(reg, &_RegLO->UW[1], "_RegLO->UW[1]");
        MoveX86regToVariable(Map_TempReg(reg, m_Opcode.rs, false), &_RegLO->UW[0], "_RegLO->UW[0]");
    }
}

void CX86RecompilerOps::SPECIAL_MFHI()
{
    if (m_Opcode.rd == 0) { return; }

    Map_GPR_64bit(m_Opcode.rd, -1);
    MoveVariableToX86reg(&_RegHI->UW[0], "_RegHI->UW[0]", GetMipsRegMapLo(m_Opcode.rd));
    MoveVariableToX86reg(&_RegHI->UW[1], "_RegHI->UW[1]", GetMipsRegMapHi(m_Opcode.rd));
}

void CX86RecompilerOps::SPECIAL_MTHI()
{
    if (IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveConstToVariable(GetMipsRegHi(m_Opcode.rs), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs) && ((GetMipsRegLo(m_Opcode.rs) & 0x80000000) != 0))
        {
            MoveConstToVariable(0xFFFFFFFF, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        MoveConstToVariable(GetMipsRegLo(m_Opcode.rs), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
    else if (IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs))
    {
        if (Is64Bit(m_Opcode.rs))
        {
            MoveX86regToVariable(GetMipsRegMapHi(m_Opcode.rs), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else if (IsSigned(m_Opcode.rs))
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rs, true), &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        else
        {
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
        }
        MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rs), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
    else
    {
        x86Reg reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        MoveX86regToVariable(reg, &_RegHI->UW[1], "_RegHI->UW[1]");
        MoveX86regToVariable(Map_TempReg(reg, m_Opcode.rs, false), &_RegHI->UW[0], "_RegHI->UW[0]");
    }
}

void CX86RecompilerOps::SPECIAL_DSLLV()
{
    uint8_t * Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        //uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x3F);
        CX86RecompilerOps::UnknownOpcode();
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x3F);
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    CompConstToX86reg(x86_ECX, 0x20);
    JaeLabel8("MORE32", 0);
    Jump[0] = *g_RecompPos - 1;
    ShiftLeftDouble(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    ShiftLeftSign(GetMipsRegMapLo(m_Opcode.rd));
    JmpLabel8("Continue", 0);
    Jump[1] = *g_RecompPos - 1;

    // MORE32:
    CPU_Message("");
    CPU_Message("      MORE32:");
    SetJump8(Jump[0], *g_RecompPos);
    MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
    XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    AndConstToX86Reg(x86_ECX, 0x1F);
    ShiftLeftSign(GetMipsRegMapHi(m_Opcode.rd));

    // Continue:
    CPU_Message("");
    CPU_Message("      continue:");
    SetJump8(Jump[1], *g_RecompPos);
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
        if (m_Opcode.rd == m_Opcode.rt)
        {
            CX86RecompilerOps::UnknownOpcode();
            return;
        }

        Map_TempReg(x86_ECX, -1, false);
        MoveConstToX86reg(Shift, x86_ECX);
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        if ((Shift & 0x20) == 0x20)
        {
            MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
            XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            AndConstToX86Reg(x86_ECX, 0x1F);
            ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
            ShiftRightUnsign(GetMipsRegMapHi(m_Opcode.rd));
        }
    }
    else
    {
        Map_TempReg(x86_ECX, m_Opcode.rs, false);
        AndConstToX86Reg(x86_ECX, 0x3F);
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        CompConstToX86reg(x86_ECX, 0x20);
        JaeLabel8("MORE32", 0);
        Jump[0] = *g_RecompPos - 1;
        ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        ShiftRightUnsign(GetMipsRegMapHi(m_Opcode.rd));
        JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;

        // MORE32:
        CPU_Message("");
        CPU_Message("      MORE32:");
        SetJump8(Jump[0], *g_RecompPos);
        MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
        XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
        AndConstToX86Reg(x86_ECX, 0x1F);
        ShiftRightUnsign(GetMipsRegMapLo(m_Opcode.rd));

        // Continue:
        CPU_Message("");
        CPU_Message("      continue:");
        SetJump8(Jump[1], *g_RecompPos);
    }
}

void CX86RecompilerOps::SPECIAL_DSRAV()
{
    uint8_t * Jump[2];

    if (m_Opcode.rd == 0)
    {
        return;
    }

    if (IsConst(m_Opcode.rs))
    {
        //uint32_t Shift = (GetMipsRegLo(m_Opcode.rs) & 0x3F);
        CX86RecompilerOps::UnknownOpcode();
        return;
    }
    Map_TempReg(x86_ECX, m_Opcode.rs, false);
    AndConstToX86Reg(x86_ECX, 0x3F);
    Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
    CompConstToX86reg(x86_ECX, 0x20);
    JaeLabel8("MORE32", 0);
    Jump[0] = *g_RecompPos - 1;
    ShiftRightDouble(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd));
    ShiftRightSign(GetMipsRegMapHi(m_Opcode.rd));
    JmpLabel8("Continue", 0);
    Jump[1] = *g_RecompPos - 1;

    // MORE32:
    CPU_Message("");
    CPU_Message("      MORE32:");
    SetJump8(Jump[0], *g_RecompPos);
    MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    ShiftRightSignImmed(GetMipsRegMapHi(m_Opcode.rd), 0x1F);
    AndConstToX86Reg(x86_ECX, 0x1F);
    ShiftRightSign(GetMipsRegMapLo(m_Opcode.rd));

    // Continue:
    CPU_Message("");
    CPU_Message("      continue:");
    SetJump8(Jump[1], *g_RecompPos);
}

void CX86RecompilerOps::SPECIAL_MULT()
{
    m_RegWorkingSet.SetX86Protected(x86_EDX, true);
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    m_RegWorkingSet.SetX86Protected(x86_EDX, false);
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    imulX86reg(x86_EDX);

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    ShiftRightSignImmed(x86_EAX, 31);    // Paired
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_MULTU()
{
    m_RegWorkingSet.SetX86Protected(x86_EDX, true);
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    m_RegWorkingSet.SetX86Protected(x86_EDX, false);
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    MulX86reg(x86_EDX);

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    ShiftRightSignImmed(x86_EAX, 31);    // Paired
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_DIV()
{
    if (IsConst(m_Opcode.rt))
    {
        if (GetMipsRegLo(m_Opcode.rt) == 0)
        {
            MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
            MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
            return;
        }
    }
    else
    {
        if (IsMapped(m_Opcode.rt))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rt), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::DivByZero, false, JeLabel32);
    }
    /* lo = (SD)rs / (SD)rt;
    hi = (SD)rs % (SD)rt; */

    m_RegWorkingSet.SetX86Protected(x86_EDX, true);
    Map_TempReg(x86_EAX, m_Opcode.rs, false);

    // EDX is the signed portion to EAX
    m_RegWorkingSet.SetX86Protected(x86_EDX, false);
    Map_TempReg(x86_EDX, -1, false);

    MoveX86RegToX86Reg(x86_EAX, x86_EDX);
    ShiftRightSignImmed(x86_EDX, 31);

    if (IsMapped(m_Opcode.rt))
    {
        idivX86reg(GetMipsRegMapLo(m_Opcode.rt));
    }
    else
    {
        idivX86reg(Map_TempReg(x86_Any, m_Opcode.rt, false));
    }

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    ShiftRightSignImmed(x86_EAX, 31);    // Paired
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");
}

void CX86RecompilerOps::SPECIAL_DIVU()
{
    uint8_t *Jump[2];
    x86Reg Reg;

    if (IsConst(m_Opcode.rt))
    {
        if (GetMipsRegLo(m_Opcode.rt) == 0)
        {
            MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
            MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
            return;
        }
        Jump[1] = nullptr;
    }
    else
    {
        if (IsMapped(m_Opcode.rt))
        {
            CompConstToX86reg(GetMipsRegMapLo(m_Opcode.rt), 0);
        }
        else
        {
            CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        }
        JneLabel8("NoExcept", 0);
        Jump[0] = *g_RecompPos - 1;

        MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
        MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
        MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

        JmpLabel8("EndDivu", 0);
        Jump[1] = *g_RecompPos - 1;

        CPU_Message("");
        CPU_Message("      NoExcept:");
        SetJump8(Jump[0], *g_RecompPos);
    }

    /*    lo = (UD)rs / (UD)rt;
    hi = (UD)rs % (UD)rt; */

    m_RegWorkingSet.SetX86Protected(x86_EAX, true);
    Map_TempReg(x86_EDX, 0, false);
    m_RegWorkingSet.SetX86Protected(x86_EAX, false);

    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);

    DivX86reg(Reg);

    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");

    // Wouldn't these be zero?

    ShiftRightSignImmed(x86_EAX, 31);    // Paired
    ShiftRightSignImmed(x86_EDX, 31);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[1], "_RegLO->UW[1]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

    if (Jump[1] != nullptr)
    {
        CPU_Message("");
        CPU_Message("      EndDivu:");
        SetJump8(Jump[1], *g_RecompPos);
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
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DMULTU()
{
    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DMULTU, "R4300iOp::SPECIAL_DMULTU");
    m_RegWorkingSet.AfterCallDirect();

#ifdef toremove
    /* _RegLO->UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
    X86Protected(x86_EDX) = true;
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    X86Protected(x86_EDX) = false;
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    MulX86reg(x86_EDX);
    MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

    /* _RegHI->UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
    Map_TempReg(x86_EAX, m_Opcode.rs, true);
    Map_TempReg(x86_EDX, m_Opcode.rt, true);

    MulX86reg(x86_EDX);
    MoveX86regToVariable(x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
    MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

    /* Tmp[0].UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
    Map_TempReg(x86_EAX, m_Opcode.rs, true);
    Map_TempReg(x86_EDX, m_Opcode.rt, false);

    Map_TempReg(x86_EBX, -1, false);
    Map_TempReg(x86_ECX, -1, false);

    MulX86reg(x86_EDX);
    MoveX86RegToX86Reg(x86_EAX, x86_EBX); // EDX:EAX -> ECX:EBX
    MoveX86RegToX86Reg(x86_EDX, x86_ECX);

    /* Tmp[1].UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
    Map_TempReg(x86_EAX, m_Opcode.rs, false);
    Map_TempReg(x86_EDX, m_Opcode.rt, true);

    MulX86reg(x86_EDX);
    Map_TempReg(x86_ESI, -1, false);
    Map_TempReg(x86_EDI, -1, false);
    MoveX86RegToX86Reg(x86_EAX, x86_ESI); // EDX:EAX -> EDI:ESI
    MoveX86RegToX86Reg(x86_EDX, x86_EDI);

    /* Tmp[2].UDW = (uint64)_RegLO->UW[1] + (uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]; */
    XorX86RegToX86Reg(x86_EDX, x86_EDX);
    MoveVariableToX86reg(&_RegLO->UW[1], "_RegLO->UW[1]", x86_EAX);
    AddX86RegToX86Reg(x86_EAX, x86_EBX);
    AddConstToX86Reg(x86_EDX, 0);
    AddX86RegToX86Reg(x86_EAX, x86_ESI);
    AddConstToX86Reg(x86_EDX, 0);            // EDX:EAX

    /* _RegLO->UDW += ((uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]) << 32; */
    /* [low+4] += ebx + esi */

    AddX86regToVariable(x86_EBX, &_RegLO->UW[1], "_RegLO->UW[1]");
    AddX86regToVariable(x86_ESI, &_RegLO->UW[1], "_RegLO->UW[1]");

    /* _RegHI->UDW += (uint64)Tmp[0].UW[1] + (uint64)Tmp[1].UW[1] + Tmp[2].UW[1]; */
    /* [hi] += ecx + edi + edx */

    AddX86regToVariable(x86_ECX, &_RegHI->UW[0], "_RegHI->UW[0]");
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

    AddX86regToVariable(x86_EDI, &_RegHI->UW[0], "_RegHI->UW[0]");
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

    AddX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
    AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
#endif
}

void CX86RecompilerOps::SPECIAL_DDIV()
{
    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_DDIVU()
{
    UnMap_GPR(m_Opcode.rs, true);
    UnMap_GPR(m_Opcode.rt, true);
    m_RegWorkingSet.BeforeCallDirect();
    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::SPECIAL_ADD()
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
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
    }
    else if (IsKnown(source2) && IsMapped(source2))
    {
        AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
    }
    else
    {
        AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
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
        AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
    }
    else if (IsKnown(source2) && IsMapped(source2))
    {
        AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
    }
    else
    {
        AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
    }
    if (g_System->bFastSP() && m_Opcode.rd == 29)
    {
        ResetMemoryStack();
    }
}

void CX86RecompilerOps::SPECIAL_SUB()
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
            x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
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
            x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Reg);
            return;
        }
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
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
                    (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs))
                    );

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
                AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            }
            else if (Is32Bit(source1) || Is32Bit(source2))
            {
                if (IsUnsigned(Is32Bit(source1) ? source1 : source2))
                {
                    Map_GPR_32bit(m_Opcode.rd, false, source1);
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
                else
                {
                    Map_GPR_64bit(m_Opcode.rd, source1);
                    if (Is32Bit(source2))
                    {
                        AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                    }
                    else
                    {
                        AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                    }
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, source1);
                AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
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
                        AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
                    }
                }
                else
                {
                    int64_t Value = GetMipsReg(ConstReg);
                    Map_GPR_64bit(m_Opcode.rd, MappedReg);
                    AndConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
                }
            }
            else if (Is64Bit(MappedReg))
            {
                uint32_t Value = GetMipsRegLo(ConstReg);
                if (Value != 0)
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(ConstReg), MappedReg);
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
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
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
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
                AndConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), (uint32_t)(Value & 0xFFFFFFFF));
            }
            else
            {
                uint32_t Value = GetMipsRegLo(KnownReg);
                Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), UnknownReg);
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value);
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
                    AndVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                    AndVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), KnownReg);
                    AndVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                    AndX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(KnownReg));
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
                }
                else
                {
                    Map_GPR_32bit(m_Opcode.rd, IsSigned(KnownReg), UnknownReg);
                    AndX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(KnownReg));
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
            AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
        }
        AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
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
                    (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs))
                    );
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
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else
                {
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                }
            }
            else
            {
                ProtectGPR(source2);
                Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            OrX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
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
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0) { OrConstToX86Reg(Value, GetMipsRegMapLo(m_Opcode.rd)); }
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
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
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
                if (HaveDebugger()) { g_Notify->DisplayError("XOR 1"); }
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
                    XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else if (IsSigned(source2))
                {
                    XorX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                }
                XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
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
                XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
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
                if (ConstHi != 0) { XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), ConstHi); }
                if (ConstLo != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), ConstLo); }
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
                if (Value != 0) { XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), Value); }
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
                    XorConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), (uint32_t)(Value >> 32));
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
                XorConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), dwValue);
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                XorVariableToX86reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                XorVariableToX86reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                XorVariableToX86reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
        XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
        XorVariableToX86reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
        XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
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
                    (Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs)))
                    );
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
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapHi(source2));
                }
                else
                {
                    OrX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), Map_TempReg(x86_Any, source2, true));
                }
            }
            else
            {
                ProtectGPR(source2);
                Map_GPR_32bit(m_Opcode.rd, true, source1);
            }
            OrX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
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
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                uint32_t dwValue = (uint32_t)(Value & 0xFFFFFFFF);
                if (dwValue != 0) {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                int Value = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, MappedReg);
                if (Value != 0) { OrConstToX86Reg(Value, GetMipsRegMapLo(m_Opcode.rd)); }
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
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, UnknownReg);
                if ((Value >> 32) != 0)
                {
                    OrConstToX86Reg((uint32_t)(Value >> 32), GetMipsRegMapHi(m_Opcode.rd));
                }
                if (dwValue != 0)
                {
                    OrConstToX86Reg(dwValue, GetMipsRegMapLo(m_Opcode.rd));
                }
            }
        }
        else
        {
            if (g_System->b32BitCore())
            {
                Map_GPR_32bit(m_Opcode.rd, true, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_64bit(m_Opcode.rd, KnownReg);
                OrVariableToX86Reg(&_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg], GetMipsRegMapHi(m_Opcode.rd));
                OrVariableToX86Reg(&_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg], GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else
    {
        if (g_System->b32BitCore())
        {
            Map_GPR_32bit(m_Opcode.rd, true, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rt);
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1], CRegName::GPR_Hi[m_Opcode.rs], GetMipsRegMapHi(m_Opcode.rd));
            OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0], CRegName::GPR_Lo[m_Opcode.rs], GetMipsRegMapLo(m_Opcode.rd));
        }
    }

    if (IsMapped(m_Opcode.rd))
    {
        if (Is64Bit(m_Opcode.rd))
        {
            NotX86Reg(GetMipsRegMapHi(m_Opcode.rd));
        }
        NotX86Reg(GetMipsRegMapLo(m_Opcode.rd));
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
                uint8_t *Jump[2];

                CompX86RegToX86Reg(
                    Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(x86_Any, m_Opcode.rs, true),
                    Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true)
                    );
                JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                SetlVariable(&m_BranchCompare, "m_BranchCompare");
                JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], *g_RecompPos);
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));

                if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
                {
                    SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    Setl(GetMipsRegMapLo(m_Opcode.rd));
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
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
                uint8_t *Jump[2];

                CompConstToX86reg(
                    Is64Bit(MappedReg) ? GetMipsRegMapHi(MappedReg) : Map_TempReg(x86_Any, MappedReg, true),
                    Is64Bit(ConstReg) ? GetMipsRegHi(ConstReg) : (GetMipsRegLo_S(ConstReg) >> 31)
                    );
                JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                if (MappedReg == m_Opcode.rs)
                {
                    SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], *g_RecompPos);
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), GetMipsRegLo(ConstReg));
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                uint32_t Constant = GetMipsRegLo(ConstReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompConstToX86reg(GetMipsRegMapLo(MappedReg), Constant);

                if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        SetlVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    else
                    {
                        SetgVariable(&m_BranchCompare, "m_BranchCompare");
                    }
                    MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    if (MappedReg == m_Opcode.rs)
                    {
                        Setl(GetMipsRegMapLo(m_Opcode.rd));
                    }
                    else
                    {
                        Setg(GetMipsRegMapLo(m_Opcode.rd));
                    }
                    AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
                }
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        uint8_t *Jump[2];

        if (!g_System->b32BitCore())
        {
            if (Is64Bit(KnownReg))
            {
                if (IsConst(KnownReg))
                {
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (IsConst(KnownReg))
                {
                    CompConstToVariable((GetMipsRegLo_S(KnownReg) >> 31), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    ProtectGPR(KnownReg);
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                SetgVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetlVariable(&m_BranchCompare, "m_BranchCompare");
            }
            JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;

            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], *g_RecompPos);
            if (IsConst(KnownReg))
            {
                CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt)) {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], *g_RecompPos);
            Map_GPR_32bit(m_Opcode.rd, true, -1);
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
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
                CompConstToVariable(Value, &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    SetgVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetlVariable(&m_BranchCompare, "m_BranchCompare");
                }
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                if (KnownReg == (bConstant ? m_Opcode.rs : m_Opcode.rt))
                {
                    Setg(GetMipsRegMapLo(m_Opcode.rd));
                }
                else
                {
                    Setl(GetMipsRegMapLo(m_Opcode.rd));
                }
                AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
            }
        }
    }
    else if (g_System->b32BitCore())
    {
        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, false);
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        if (GetMipsRegMapLo(m_Opcode.rd) > x86_EBX)
        {
            SetlVariable(&m_BranchCompare, "m_BranchCompare");
            MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
        }
        else
        {
            Setl(GetMipsRegMapLo(m_Opcode.rd));
            AndConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), 1);
        }
    }
    else
    {
        uint8_t *Jump[2] = { nullptr, nullptr };

        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        JeLabel8("Low Compare", 0);
        Jump[0] = *g_RecompPos - 1;
        SetlVariable(&m_BranchCompare, "m_BranchCompare");
        JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;

        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(Jump[0], *g_RecompPos);
        CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], *g_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
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
                uint8_t *Jump[2];

                CompX86RegToX86Reg(
                    Is64Bit(m_Opcode.rs) ? GetMipsRegMapHi(m_Opcode.rs) : Map_TempReg(x86_Any, m_Opcode.rs, true),
                    Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true)
                    );
                JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], *g_RecompPos);
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                CompX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rs), GetMipsRegMapLo(m_Opcode.rt));
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
        }
        else
        {
            if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs))
            {
                uint32_t ConstHi, ConstLo, ConstReg, MappedReg;
                x86Reg MappedRegHi, MappedRegLo;
                uint8_t *Jump[2];

                ConstReg = IsConst(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
                MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                ConstLo = GetMipsRegLo_S(ConstReg);
                ConstHi = GetMipsRegLo_S(ConstReg) >> 31;
                if (Is64Bit(ConstReg)) { ConstHi = GetMipsRegHi(ConstReg); }

                ProtectGPR(MappedReg);
                MappedRegLo = GetMipsRegMapLo(MappedReg);
                MappedRegHi = GetMipsRegMapHi(MappedReg);
                if (Is32Bit(MappedReg))
                {
                    MappedRegHi = Map_TempReg(x86_Any, MappedReg, true);
                }

                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompConstToX86reg(MappedRegHi, ConstHi);
                JeLabel8("Low Compare", 0);
                Jump[0] = *g_RecompPos - 1;
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                JmpLabel8("Continue", 0);
                Jump[1] = *g_RecompPos - 1;

                CPU_Message("");
                CPU_Message("      Low Compare:");
                SetJump8(Jump[0], *g_RecompPos);
                CompConstToX86reg(MappedRegLo, ConstLo);
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], *g_RecompPos);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
            else
            {
                uint32_t Const = IsConst(m_Opcode.rs) ? GetMipsRegLo(m_Opcode.rs) : GetMipsRegLo(m_Opcode.rt);
                uint32_t MappedReg = IsConst(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;

                CompConstToX86reg(GetMipsRegMapLo(MappedReg), Const);
                if (MappedReg == m_Opcode.rs)
                {
                    SetbVariable(&m_BranchCompare, "m_BranchCompare");
                }
                else
                {
                    SetaVariable(&m_BranchCompare, "m_BranchCompare");
                }
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
            }
        }
    }
    else if (IsKnown(m_Opcode.rt) || IsKnown(m_Opcode.rs))
    {
        uint32_t KnownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rt : m_Opcode.rs;
        uint32_t UnknownReg = IsKnown(m_Opcode.rt) ? m_Opcode.rs : m_Opcode.rt;
        uint8_t *Jump[2] = { nullptr, nullptr };

        ProtectGPR(KnownReg);
        if (g_System->b32BitCore())
        {
            uint32_t TestReg = IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt;
            if (IsConst(KnownReg))
            {
                uint32_t Value = GetMipsRegLo(KnownReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                CompConstToVariable(Value, &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == TestReg)
            {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
        }
        else
        {
            if (IsConst(KnownReg))
            {
                if (Is64Bit(KnownReg))
                {
                    CompConstToVariable(GetMipsRegHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    CompConstToVariable((GetMipsRegLo_S(KnownReg) >> 31), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            else
            {
                if (Is64Bit(KnownReg))
                {
                    CompX86regToVariable(GetMipsRegMapHi(KnownReg), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
                else
                {
                    ProtectGPR(KnownReg);
                    CompX86regToVariable(Map_TempReg(x86_Any, KnownReg, true), &_GPR[UnknownReg].W[1], CRegName::GPR_Hi[UnknownReg]);
                }
            }
            JeLabel8("Low Compare", 0);
            Jump[0] = *g_RecompPos - 1;

            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            JmpLabel8("Continue", 0);
            Jump[1] = *g_RecompPos - 1;

            CPU_Message("");
            CPU_Message("      Low Compare:");
            SetJump8(Jump[0], *g_RecompPos);
            if (IsConst(KnownReg))
            {
                CompConstToVariable(GetMipsRegLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            else
            {
                CompX86regToVariable(GetMipsRegMapLo(KnownReg), &_GPR[UnknownReg].W[0], CRegName::GPR_Lo[UnknownReg]);
            }
            if (KnownReg == (IsConst(KnownReg) ? m_Opcode.rs : m_Opcode.rt))
            {
                SetaVariable(&m_BranchCompare, "m_BranchCompare");
            }
            else
            {
                SetbVariable(&m_BranchCompare, "m_BranchCompare");
            }
            if (Jump[1])
            {
                CPU_Message("");
                CPU_Message("      Continue:");
                SetJump8(Jump[1], *g_RecompPos);
            }
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
    else if (g_System->b32BitCore())
    {
        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, false);
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        uint8_t *Jump[2] = { nullptr, nullptr };

        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rs, true);
        CompX86regToVariable(Reg, &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
        JeLabel8("Low Compare", 0);
        Jump[0] = *g_RecompPos - 1;
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        JmpLabel8("Continue", 0);
        Jump[1] = *g_RecompPos - 1;

        CPU_Message("");
        CPU_Message("      Low Compare:");
        SetJump8(Jump[0], *g_RecompPos);
        CompX86regToVariable(Map_TempReg(Reg, m_Opcode.rs, false), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
        SetbVariable(&m_BranchCompare, "m_BranchCompare");
        if (Jump[1])
        {
            CPU_Message("");
            CPU_Message("      Continue:");
            SetJump8(Jump[1], *g_RecompPos);
        }
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&m_BranchCompare, "m_BranchCompare", GetMipsRegMapLo(m_Opcode.rd));
    }
}

void CX86RecompilerOps::SPECIAL_DADD()
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
            Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) +
            Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)
            );
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
        int source1 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rt : m_Opcode.rs;
        int source2 = m_Opcode.rd == m_Opcode.rt ? m_Opcode.rs : m_Opcode.rt;

        if (IsMapped(source2)) { ProtectGPR(source2); }
        Map_GPR_64bit(m_Opcode.rd, source1);
        if (IsConst(source2))
        {
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(source2));
            AddConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
        }
        else if (IsMapped(source2))
        {
            x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(x86_Any, source2, true);
            AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            AdcX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            AdcVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
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

        if (IsMapped(source2)) { ProtectGPR(source2); }
        Map_GPR_64bit(m_Opcode.rd, source1);
        if (IsConst(source2))
        {
            uint32_t LoReg = GetMipsRegLo(source2);
            AddConstToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            if (LoReg != 0)
            {
                AdcConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
            }
            else
            {
                AddConstToX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(source2));
            }
        }
        else if (IsMapped(source2))
        {
            x86Reg HiReg = Is64Bit(source2) ? GetMipsRegMapHi(source2) : Map_TempReg(x86_Any, source2, true);
            AddX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(source2));
            AdcX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            AddVariableToX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[source2].W[0], CRegName::GPR_Lo[source2]);
            AdcVariableToX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[source2].W[1], CRegName::GPR_Hi[source2]);
        }
    }
}

void CX86RecompilerOps::SPECIAL_DSUB()
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
            Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) -
            Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)
            );
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
            x86Reg HiReg = Map_TempReg(x86_Any, m_Opcode.rt, true);
            x86Reg LoReg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
            return;
        }

        if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
            SbbConstFromX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            x86Reg HiReg = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
            SbbVariableFromX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
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
            Is64Bit(m_Opcode.rs) ? GetMipsReg(m_Opcode.rs) : (int64_t)GetMipsRegLo_S(m_Opcode.rs) -
            Is64Bit(m_Opcode.rt) ? GetMipsReg(m_Opcode.rt) : (int64_t)GetMipsRegLo_S(m_Opcode.rt)
            );
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
            x86Reg HiReg = Map_TempReg(x86_Any, m_Opcode.rt, true);
            x86Reg LoReg = Map_TempReg(x86_Any, m_Opcode.rt, false);
            Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), LoReg);
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
            return;
        }
        if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
        Map_GPR_64bit(m_Opcode.rd, m_Opcode.rs);
        if (IsConst(m_Opcode.rt))
        {
            SubConstFromX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegLo(m_Opcode.rt));
            SbbConstFromX86Reg(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegHi(m_Opcode.rt));
        }
        else if (IsMapped(m_Opcode.rt))
        {
            x86Reg HiReg = Is64Bit(m_Opcode.rt) ? GetMipsRegMapHi(m_Opcode.rt) : Map_TempReg(x86_Any, m_Opcode.rt, true);
            SubX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rt));
            SbbX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rd), HiReg);
        }
        else
        {
            SubVariableFromX86reg(GetMipsRegMapLo(m_Opcode.rd), &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
            SbbVariableFromX86reg(GetMipsRegMapHi(m_Opcode.rd), &_GPR[m_Opcode.rt].W[1], CRegName::GPR_Hi[m_Opcode.rt]);
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
    ShiftLeftDoubleImmed(GetMipsRegMapHi(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    ShiftLeftSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
    ShiftRightDoubleImmed(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    ShiftRightUnsignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
    ShiftRightDoubleImmed(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
    ShiftRightSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
            MoveX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rt), GetMipsRegMapHi(m_Opcode.rd));
        }
        else
        {
            CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s", x86_Name(GetMipsRegMapHi(m_Opcode.rt)), x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
            x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
            m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
            m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
        }
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftLeftSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
    }
    else
    {
        Map_GPR_64bit(m_Opcode.rd, -1);
        MoveVariableToX86reg(&_GPR[m_Opcode.rt], CRegName::GPR_Hi[m_Opcode.rt], GetMipsRegMapHi(m_Opcode.rd));
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftLeftSignImmed(GetMipsRegMapHi(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
        XorX86RegToX86Reg(GetMipsRegMapLo(m_Opcode.rd), GetMipsRegMapLo(m_Opcode.rd));
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
                CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s", x86_Name(GetMipsRegMapHi(m_Opcode.rt)), x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                Map_GPR_32bit(m_Opcode.rd, false, -1);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, false, -1);
                MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rd));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CX86RecompilerOps::UnknownOpcode();
        }
    }
    else {
        Map_GPR_32bit(m_Opcode.rd, false, -1);
        MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Hi[m_Opcode.rt], GetMipsRegMapLo(m_Opcode.rd));
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftRightUnsignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
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
                CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s", x86_Name(GetMipsRegMapHi(m_Opcode.rt)), x86_Name(GetMipsRegMapLo(m_Opcode.rt)), CRegName::GPR[m_Opcode.rt]);
                x86Reg HiReg = GetMipsRegMapHi(m_Opcode.rt);
                m_RegWorkingSet.SetMipsRegMapHi(m_Opcode.rt, GetMipsRegMapLo(m_Opcode.rt));
                m_RegWorkingSet.SetMipsRegMapLo(m_Opcode.rt, HiReg);
                Map_GPR_32bit(m_Opcode.rd, true, -1);
            }
            else
            {
                Map_GPR_32bit(m_Opcode.rd, true, -1);
                MoveX86RegToX86Reg(GetMipsRegMapHi(m_Opcode.rt), GetMipsRegMapLo(m_Opcode.rd));
            }
            if ((uint8_t)m_Opcode.sa != 0)
            {
                ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
            }
        }
        else
        {
            CX86RecompilerOps::UnknownOpcode();
        }
    }
    else {
        Map_GPR_32bit(m_Opcode.rd, true, -1);
        MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1], CRegName::GPR_Lo[m_Opcode.rt], GetMipsRegMapLo(m_Opcode.rd));
        if ((uint8_t)m_Opcode.sa != 0)
        {
            ShiftRightSignImmed(GetMipsRegMapLo(m_Opcode.rd), (uint8_t)m_Opcode.sa);
        }
    }
}

// COP0 functions
void CX86RecompilerOps::COP0_MF()
{
    switch (m_Opcode.rd)
    {
    case 9: // Count
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
    }
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    MoveVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd], GetMipsRegMapLo(m_Opcode.rt));
}

void CX86RecompilerOps::COP0_MT()
{
    uint8_t *Jump;

    switch (m_Opcode.rd)
    {
    case 0: // Index
    case 2: // EntryLo0
    case 3: // EntryLo1
    case 4: // Context
    case 5: // PageMask
    case 10: // Entry Hi
    case 14: // EPC
    case 16: // Config
    case 18: // WatchLo
    case 19: // WatchHi
    case 28: // Tag Lo
    case 29: // Tag Hi
    case 30: // ErrEPC
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        if (m_Opcode.rd == 4) // Context
        {
            AndConstToVariable(0xFF800000, &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        break;
    case 11: // Compare
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        AndConstToVariable((uint32_t)~CAUSE_IP7, &g_Reg->FAKE_CAUSE_REGISTER, "FAKE_CAUSE_REGISTER");
        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 9: // Count
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        m_RegWorkingSet.BeforeCallDirect();
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 12: // Status
    {
                 x86Reg OldStatusReg = Map_TempReg(x86_Any, -1, false);
                 MoveVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd], OldStatusReg);
                 if (IsConst(m_Opcode.rt))
                 {
                     MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
                 }
                 else if (IsMapped(m_Opcode.rt))
                 {
                     MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
                 }
                 else {
                     MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
                 }
                 XorVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd], OldStatusReg);
                 TestConstToX86Reg(STATUS_FR, OldStatusReg);
                 JeLabel8("FpuFlagFine", 0);
                 Jump = *g_RecompPos - 1;
                 m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
                 MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
                 Call_Direct(AddressOf(&CRegisters::FixFpuLocations), "CRegisters::FixFpuLocations");
#else
                 PushImm32((uint32_t)g_Reg);
                 Call_Direct(AddressOf(&CRegisters::FixFpuLocations), "CRegisters::FixFpuLocations");
                 AddConstToX86Reg(x86_ESP, 4);
#endif
                 m_RegWorkingSet.AfterCallDirect();
                 SetJump8(Jump, *g_RecompPos);

                 m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
                 MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
                 Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
                 PushImm32((uint32_t)g_Reg);
                 Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
                 AddConstToX86Reg(x86_ESP, 4);
#endif
                 m_RegWorkingSet.AfterCallDirect();
    }
        break;
    case 6: // Wired
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
        if (IsConst(m_Opcode.rt))
        {
            MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else if (IsMapped(m_Opcode.rt))
        {
            MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        else
        {
            MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        }
        break;
    case 13: // Cause
        AndConstToVariable(0xFFFFCFF, &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
        if (IsConst(m_Opcode.rt))
        {
            if ((GetMipsRegLo(m_Opcode.rt) & 0x300) != 0 && HaveDebugger())
            {
                g_Notify->DisplayError("Set IP0 or IP1");
            }
        }
        /*else if (HaveDebugger())
        {
            UnknownOpcode();
            return;
        }*/
        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
        Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
        PushImm32((uint32_t)g_Reg);
        Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
        break;
    default:
        UnknownOpcode();
    }
}

// COP0 CO functions
void CX86RecompilerOps::COP0_CO_TLBR(void)
{
    m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::ReadEntry), "CTLB::ReadEntry");
#else
    PushImm32((uint32_t)g_TLB);
    Call_Direct(AddressOf(&CTLB::ReadEntry), "CTLB::ReadEntry");
    AddConstToX86Reg(x86_ESP, 4);
#endif
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBWI(void)
{
    m_RegWorkingSet.BeforeCallDirect();
    PushImm32("false", 0);
    MoveVariableToX86reg(&g_Reg->INDEX_REGISTER, "INDEX_REGISTER", x86_ECX);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Push(x86_ECX);
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
#else
    PushImm32((uint32_t)g_TLB);
    Call_Direct(AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
    AddConstToX86Reg(x86_ESP, 12);
#endif
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBWR(void)
{
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    UpdateCounters(m_RegWorkingSet, false, true);
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
    Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
#else
    PushImm32((uint32_t)g_SystemTimer);
    Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
    AddConstToX86Reg(x86_ESP, 4);
#endif

    PushImm32("true", true);
    MoveVariableToX86reg(&g_Reg->RANDOM_REGISTER, "RANDOM_REGISTER", x86_ECX);
    AndConstToX86Reg(x86_ECX, 0x1F);
    Push(x86_ECX);
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
#else
    PushImm32((uint32_t)g_TLB);
    Call_Direct(AddressOf(&CTLB::WriteEntry), "CTLB::WriteEntry");
    AddConstToX86Reg(x86_ESP, 12);
#endif
    m_RegWorkingSet.AfterCallDirect();
}

void CX86RecompilerOps::COP0_CO_TLBP(void)
{
    m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_TLB, x86_ECX);
    Call_Direct(AddressOf(&CTLB::Probe), "CTLB::TLB_Probe");
#else
    PushImm32((uint32_t)g_TLB);
    Call_Direct(AddressOf(&CTLB::Probe), "CTLB::TLB_Probe");
    AddConstToX86Reg(x86_ESP, 4);
#endif
    m_RegWorkingSet.AfterCallDirect();
}

void x86_compiler_COP0_CO_ERET()
{
    if ((g_Reg->STATUS_REGISTER & STATUS_ERL) != 0)
    {
        g_Reg->m_PROGRAM_COUNTER = g_Reg->ERROREPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_ERL;
    }
    else
    {
        g_Reg->m_PROGRAM_COUNTER = g_Reg->EPC_REGISTER;
        g_Reg->STATUS_REGISTER &= ~STATUS_EXL;
    }
    g_Reg->m_LLBit = 0;
    g_Reg->CheckInterrupts();
}

void CX86RecompilerOps::COP0_CO_ERET(void)
{
    m_RegWorkingSet.WriteBackRegisters();
    Call_Direct((void *)x86_compiler_COP0_CO_ERET, "x86_compiler_COP0_CO_ERET");

    UpdateCounters(m_RegWorkingSet, true, true);
    CompileExit(m_CompilePC, (uint32_t)-1, m_RegWorkingSet, CExitInfo::Normal, true, nullptr);
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

// FPU options
void CX86RecompilerOps::ChangeDefaultRoundingModel()
{
    switch ((_FPCR[31] & 3))
    {
    case 0: *_RoundingModel = FE_TONEAREST; break;
    case 1: *_RoundingModel = FE_TOWARDZERO; break;
    case 2: *_RoundingModel = FE_UPWARD;   break;
    case 3: *_RoundingModel = FE_DOWNWARD; break;
    }
}

// COP1 functions
void CX86RecompilerOps::COP1_MF()
{
    CompileCop1Test();

    UnMap_FPR(m_Opcode.fs, true);
    Map_GPR_32bit(m_Opcode.rt, true, -1);
    x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
    char Name[100];
    sprintf(Name, "_FPR_S[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.fs], Name, TempReg);
    MoveX86PointerToX86reg(GetMipsRegMapLo(m_Opcode.rt), TempReg);
}

void CX86RecompilerOps::COP1_DMF()
{
    x86Reg TempReg;
    char Name[50];

    CompileCop1Test();

    UnMap_FPR(m_Opcode.fs, true);
    Map_GPR_64bit(m_Opcode.rt, -1);
    TempReg = Map_TempReg(x86_Any, -1, false);
    sprintf(Name, "_FPR_D[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.fs], Name, TempReg);
    AddConstToX86Reg(TempReg, 4);
    MoveX86PointerToX86reg(GetMipsRegMapHi(m_Opcode.rt), TempReg);
    sprintf(Name, "_FPR_D[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.fs], Name, TempReg);
    MoveX86PointerToX86reg(GetMipsRegMapLo(m_Opcode.rt), TempReg);
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
    MoveVariableToX86reg(&_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs], GetMipsRegMapLo(m_Opcode.rt));
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
    x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
    char Name[50];
    sprintf(Name, "_FPR_S[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.fs], Name, TempReg);

    if (IsConst(m_Opcode.rt))
    {
        MoveConstToX86Pointer(GetMipsRegLo(m_Opcode.rt), TempReg);
    }
    else if (IsMapped(m_Opcode.rt))
    {
        MoveX86regToX86Pointer(GetMipsRegMapLo(m_Opcode.rt), TempReg);
    }
    else
    {
        MoveX86regToX86Pointer(Map_TempReg(x86_Any, m_Opcode.rt, false), TempReg);
    }
}

void CX86RecompilerOps::COP1_DMT()
{
    x86Reg TempReg;

    CompileCop1Test();

    if ((m_Opcode.fs & 1) == 0)
    {
        if (RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Float) || RegInStack(m_Opcode.fs + 1, CRegInfo::FPU_Dword)) {
            UnMap_FPR(m_Opcode.fs + 1, true);
        }
    }
    UnMap_FPR(m_Opcode.fs, true);
    TempReg = Map_TempReg(x86_Any, -1, false);
    char Name[50];
    sprintf(Name, "_FPR_D[%d]", m_Opcode.fs);
    MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.fs], Name, TempReg);

    if (IsConst(m_Opcode.rt))
    {
        MoveConstToX86Pointer(GetMipsRegLo(m_Opcode.rt), TempReg);
        AddConstToX86Reg(TempReg, 4);
        if (Is64Bit(m_Opcode.rt))
        {
            MoveConstToX86Pointer(GetMipsRegHi(m_Opcode.rt), TempReg);
        }
        else
        {
            MoveConstToX86Pointer(GetMipsRegLo_S(m_Opcode.rt) >> 31, TempReg);
        }
    }
    else if (IsMapped(m_Opcode.rt))
    {
        MoveX86regToX86Pointer(GetMipsRegMapLo(m_Opcode.rt), TempReg);
        AddConstToX86Reg(TempReg, 4);
        if (Is64Bit(m_Opcode.rt))
        {
            MoveX86regToX86Pointer(GetMipsRegMapHi(m_Opcode.rt), TempReg);
        }
        else
        {
            MoveX86regToX86Pointer(Map_TempReg(x86_Any, m_Opcode.rt, true), TempReg);
        }
    }
    else
    {
        x86Reg Reg = Map_TempReg(x86_Any, m_Opcode.rt, false);
        MoveX86regToX86Pointer(Reg, TempReg);
        AddConstToX86Reg(TempReg, 4);
        MoveX86regToX86Pointer(Map_TempReg(Reg, m_Opcode.rt, true), TempReg);
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
        MoveConstToVariable(GetMipsRegLo(m_Opcode.rt), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    else if (IsMapped(m_Opcode.rt))
    {
        MoveX86regToVariable(GetMipsRegMapLo(m_Opcode.rt), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    else
    {
        MoveX86regToVariable(Map_TempReg(x86_Any, m_Opcode.rt, false), &_FPCR[m_Opcode.fs], CRegName::FPR_Ctrl[m_Opcode.fs]);
    }
    m_RegWorkingSet.BeforeCallDirect();
    Call_Direct((void *)ChangeDefaultRoundingModel, "ChangeDefaultRoundingModel");
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
        fpuAddReg(StackPosition(Reg2));
    }
    else
    {
        x86Reg TempReg;

        UnMap_FPR(Reg2, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        char Name[50];
        sprintf(Name, "_FPR_S[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);
        fpuAddDwordRegPointer(TempReg);
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.ft], Name, TempReg);
        fpuSubDwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            fpuSubReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_S[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
            fpuSubDwordRegPointer(TempReg);
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
        fpuMulReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

        x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
        char Name[50];
        sprintf(Name, "_FPR_S[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
        fpuMulDwordRegPointer(TempReg);
    }
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    char Name[50];

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);

        x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_S[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[m_Opcode.ft], Name, TempReg);
        fpuDivDwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Float);
        if (RegInStack(Reg2, CRegInfo::FPU_Float))
        {
            fpuDivReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Float);

            x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_S[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
            fpuDivDwordRegPointer(TempReg);
        }
    }

    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_ABS()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    fpuAbs();
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_NEG()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    fpuNeg();
    UnMap_FPR(m_Opcode.fd, true);
}

void CX86RecompilerOps::COP1_S_SQRT()
{
    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Float);
    fpuSqrt();
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
    if ((m_Opcode.funct & 7) == 0) { CX86RecompilerOps::UnknownOpcode(); }
    if ((m_Opcode.funct & 2) != 0) { cmp |= 0x4000; }
    if ((m_Opcode.funct & 4) != 0) { cmp |= 0x0100; }

    Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);
    Map_TempReg(x86_EAX, 0, false);
    if (RegInStack(Reg2, CRegInfo::FPU_Float))
    {
        fpuComReg(StackPosition(Reg2), false);
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Float);

        x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
        char Name[50];
        sprintf(Name, "_FPR_S[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_S[Reg2], Name, TempReg);
        fpuComDwordRegPointer(TempReg, false);
    }
    AndConstToVariable((uint32_t)~FPCSR_C, &_FPCR[31], "_FPCR[31]");
    fpuStoreStatus();
    x86Reg Reg = Map_TempReg(x86_Any8Bit, 0, false);
    TestConstToX86Reg(cmp, x86_EAX);
    Setnz(Reg);

    if (cmp != 0)
    {
        TestConstToX86Reg(cmp, x86_EAX);
        Setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            x86Reg _86RegReg2 = Map_TempReg(x86_Any8Bit, 0, false);
            AndConstToX86Reg(x86_EAX, 0x4300);
            CompConstToX86reg(x86_EAX, 0x4300);
            Setz(_86RegReg2);

            OrX86RegToX86Reg(Reg, _86RegReg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        AndConstToX86Reg(x86_EAX, 0x4300);
        CompConstToX86reg(x86_EAX, 0x4300);
        Setz(Reg);
    }
    ShiftLeftSignImmed(Reg, 23);
    OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", Reg);
}

// COP1: D functions
void CX86RecompilerOps::COP1_D_ADD()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    char Name[50];

    CompileCop1Test();

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        fpuAddReg(StackPosition(Reg2));
    }
    else
    {
        x86Reg TempReg;

        UnMap_FPR(Reg2, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        fpuAddQwordRegPointer(TempReg);
    }
}

void CX86RecompilerOps::COP1_D_SUB()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.ft], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        fpuSubQwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            fpuSubReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);

            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_D[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            fpuSubQwordRegPointer(TempReg);
        }
    }
}

void CX86RecompilerOps::COP1_D_MUL()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CompileCop1Test();
    FixRoundModel(CRegInfo::RoundDefault);

    Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        fpuMulReg(StackPosition(Reg2));
    }
    else
    {
        UnMap_FPR(Reg2, true);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
        fpuMulQwordRegPointer(TempReg);
    }
}

void CX86RecompilerOps::COP1_D_DIV()
{
    uint32_t Reg1 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.ft : m_Opcode.fs;
    uint32_t Reg2 = m_Opcode.ft == m_Opcode.fd ? m_Opcode.fs : m_Opcode.ft;
    x86Reg TempReg;
    char Name[50];

    CompileCop1Test();

    if (m_Opcode.fd == m_Opcode.ft)
    {
        UnMap_FPR(m_Opcode.fd, true);
        TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", m_Opcode.ft);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[m_Opcode.ft], Name, TempReg);
        Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
        fpuDivQwordRegPointer(TempReg);
    }
    else
    {
        Load_FPR_ToTop(m_Opcode.fd, Reg1, CRegInfo::FPU_Double);
        if (RegInStack(Reg2, CRegInfo::FPU_Double))
        {
            fpuDivReg(StackPosition(Reg2));
        }
        else
        {
            UnMap_FPR(Reg2, true);
            TempReg = Map_TempReg(x86_Any, -1, false);
            sprintf(Name, "_FPR_D[%d]", Reg2);
            MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
            Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fd, CRegInfo::FPU_Double);
            fpuDivQwordRegPointer(TempReg);
        }
    }
}

void CX86RecompilerOps::COP1_D_ABS()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    fpuAbs();
}

void CX86RecompilerOps::COP1_D_NEG()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    fpuNeg();
}

void CX86RecompilerOps::COP1_D_SQRT()
{
    CompileCop1Test();
    Load_FPR_ToTop(m_Opcode.fd, m_Opcode.fs, CRegInfo::FPU_Double);
    fpuSqrt();
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
    if ((m_Opcode.funct & 7) == 0) { CX86RecompilerOps::UnknownOpcode(); }
    if ((m_Opcode.funct & 2) != 0) { cmp |= 0x4000; }
    if ((m_Opcode.funct & 4) != 0) { cmp |= 0x0100; }

    Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
    Map_TempReg(x86_EAX, 0, false);
    if (RegInStack(Reg2, CRegInfo::FPU_Double))
    {
        fpuComReg(StackPosition(Reg2), false);
    }
    else
    {
        char Name[50];

        UnMap_FPR(Reg2, true);
        x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
        sprintf(Name, "_FPR_D[%d]", Reg2);
        MoveVariableToX86reg((uint8_t *)&_FPR_D[Reg2], Name, TempReg);
        Load_FPR_ToTop(Reg1, Reg1, CRegInfo::FPU_Double);
        fpuComQwordRegPointer(TempReg, false);
    }
    AndConstToVariable((uint32_t)~FPCSR_C, &_FPCR[31], "_FPCR[31]");
    fpuStoreStatus();
    x86Reg Reg = Map_TempReg(x86_Any8Bit, 0, false);
    TestConstToX86Reg(cmp, x86_EAX);
    Setnz(Reg);
    if (cmp != 0)
    {
        TestConstToX86Reg(cmp, x86_EAX);
        Setnz(Reg);

        if ((m_Opcode.funct & 1) != 0)
        {
            x86Reg _86RegReg2 = Map_TempReg(x86_Any8Bit, 0, false);
            AndConstToX86Reg(x86_EAX, 0x4300);
            CompConstToX86reg(x86_EAX, 0x4300);
            Setz(_86RegReg2);

            OrX86RegToX86Reg(Reg, _86RegReg2);
        }
    }
    else if ((m_Opcode.funct & 1) != 0)
    {
        AndConstToX86Reg(x86_EAX, 0x4300);
        CompConstToX86reg(x86_EAX, 0x4300);
        Setz(Reg);
    }
    ShiftLeftSignImmed(Reg, 23);
    OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", Reg);
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
    CPU_Message("  %X Unhandled opcode: %s", m_CompilePC, R4300iOpcodeName(m_Opcode.Hex, m_CompilePC));

    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem)
    {
#ifdef _WIN32
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());

    MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
    Call_Direct((void *)R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
    Ret();
    if (m_PipelineStage == PIPELINE_STAGE_NORMAL) { m_PipelineStage = PIPELINE_STAGE_END_BLOCK; }
}

void CX86RecompilerOps::ClearCachedInstructionInfo()
{
    m_RegWorkingSet.WriteBackRegisters();
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
    UpdateCounters(m_RegWorkingSet, false, true);
    m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
    MoveConstToVariable(m_CompilePC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem) {
#ifdef _WIN32
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
}

void CX86RecompilerOps::FoundMemoryBreakpoint()
{
    ClearCachedInstructionInfo();
    MoveConstToVariable((m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT) ? 1 : 0, &memory_write_in_delayslot, "memory_write_in_delayslot");
    Call_Direct((void *)x86MemoryBreakpoint, "x86MemoryBreakpoint");
    MoveConstToVariable(0, &memory_breakpoint_found, "memory_breakpoint_found");
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

void CX86RecompilerOps::TestBreakpoint(x86Reg AddressReg, void * FunctAddress, const char * FunctName)
{
    m_RegWorkingSet.BeforeCallDirect();
    MoveX86regToVariable(AddressReg, &memory_access_address, "memory_access_address");
    MoveConstToVariable((m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT) ? 1 : 0, &memory_write_in_delayslot, "memory_write_in_delayslot");
    Call_Direct(FunctAddress, FunctName);
    m_RegWorkingSet.AfterCallDirect();
    CompConstToVariable(0, &memory_breakpoint_found, "memory_breakpoint_found");
    JeLabel8("NoBreakPoint", 0);
    uint8_t *  Jump = *g_RecompPos - 1;
    MoveConstToVariable(0, &memory_breakpoint_found, "memory_breakpoint_found");
    ExitCodeBlock();
    CPU_Message("      ");
    CPU_Message("      NoBreakPoint:");
    SetJump8(Jump, *g_RecompPos);
}

void CX86RecompilerOps::TestWriteBreakpoint(x86Reg AddressReg, void * FunctAddress, const char * FunctName)
{
    if (!HaveWriteBP())
    {
        return;
    }
    TestBreakpoint(AddressReg, FunctAddress, FunctName);
}

void CX86RecompilerOps::TestReadBreakpoint(x86Reg AddressReg, void * FunctAddress, const char * FunctName)
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
    Push(x86_ESI);
#else
    Push(x86_EDI);
    Push(x86_ESI);
    Push(x86_EBX);
#endif
}

void CX86RecompilerOps::ExitCodeBlock()
{
    if (g_SyncSystem)
    {
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
#ifdef _DEBUG
    Pop(x86_ESI);
#else
    Pop(x86_EBX);
    Pop(x86_ESI);
    Pop(x86_EDI);
#endif
    Ret();
}

void CX86RecompilerOps::CompileExitCode()
{
    for (EXIT_LIST::iterator ExitIter = m_ExitInfo.begin(); ExitIter != m_ExitInfo.end(); ExitIter++)
    {
        CPU_Message("");
        CPU_Message("      $Exit_%d", ExitIter->ID);
        SetJump32(ExitIter->JumpLoc, (uint32_t *)*g_RecompPos);
        m_PipelineStage = ExitIter->PipelineStage;
        CompileExit((uint32_t)-1, ExitIter->TargetPC, ExitIter->ExitRegSet, ExitIter->reason, true, nullptr);
    }
}

void CX86RecompilerOps::CompileCop1Test()
{
    if (m_RegWorkingSet.GetFpuBeenUsed())
    {
        return;
    }

    TestVariable(STATUS_CU1, &g_Reg->STATUS_REGISTER, "STATUS_REGISTER");
    CompileExit(m_CompilePC, m_CompilePC, m_RegWorkingSet, CExitInfo::COP1_Unuseable, false, JeLabel32);
    m_RegWorkingSet.SetFpuBeenUsed(true);
}

void CX86RecompilerOps::CompileInPermLoop(CRegInfo & RegSet, uint32_t ProgramCounter)
{
    MoveConstToVariable(ProgramCounter, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
    RegSet.WriteBackRegisters();
    UpdateCounters(RegSet, false, true, false);
    Call_Direct(AddressOf(CInterpreterCPU::InPermLoop), "CInterpreterCPU::InPermLoop");
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
    Call_Direct(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
#else
    PushImm32((uint32_t)g_SystemTimer);
    Call_Direct(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
    AddConstToX86Reg(x86_ESP, 4);
#endif
    CPU_Message("CompileSystemCheck 3");
    CompileSystemCheck((uint32_t)-1, RegSet);
    if (g_SyncSystem)
    {
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
}

bool CX86RecompilerOps::SetupRegisterForLoop(CCodeBlock * BlockInfo, const CRegInfo & RegSet)
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
    if (m_RegWorkingSet.GetRoundingModel() != SyncTo.GetRoundingModel()) { m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown); }
    x86Reg MemStackReg = Get_MemoryStack();
    x86Reg TargetStackReg = SyncTo.Get_MemoryStack();

    //CPU_Message("MemoryStack for Original State = %s",MemStackReg > 0?x86_Name(MemStackReg):"Not Mapped");
    if (MemStackReg != TargetStackReg)
    {
        if (TargetStackReg == x86_Unknown)
        {
            UnMap_X86reg(MemStackReg);
        }
        else if (MemStackReg == x86_Unknown)
        {
            UnMap_X86reg(TargetStackReg);
            CPU_Message("    regcache: allocate %s as memory stack", x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(TargetStackReg, CRegInfo::Stack_Mapped);
            MoveVariableToX86reg(&g_Recompiler->MemoryStackPos(), "MemoryStack", TargetStackReg);
        }
        else
        {
            UnMap_X86reg(TargetStackReg);
            CPU_Message("    regcache: change allocation of memory stack from %s to %s", x86_Name(MemStackReg), x86_Name(TargetStackReg));
            m_RegWorkingSet.SetX86Mapped(TargetStackReg, CRegInfo::Stack_Mapped);
            m_RegWorkingSet.SetX86Mapped(MemStackReg, CRegInfo::NotMapped);
            MoveX86RegToX86Reg(MemStackReg, TargetStackReg);
        }
    }

    for (int i = 1; i < 32; i++)
    {
        if (GetMipsRegState(i) == SyncTo.GetMipsRegState(i) ||
            (g_System->b32BitCore() && GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN) ||
            (g_System->b32BitCore() && GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_SIGN && SyncTo.GetMipsRegState(i) == CRegInfo::STATE_MAPPED_32_ZERO))
        {
            switch (GetMipsRegState(i)) {
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
                    CPU_Message("Value of constant is different register %d (%s) Value: 0x%08X to 0x%08X", i, CRegName::GPR[i], GetMipsRegLo(i), SyncTo.GetMipsRegLo(i));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
                continue;
            default:
                CPU_Message("Unhandled register state %d\nin SyncRegState", GetMipsRegState(i));
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        changed = true;

        switch (SyncTo.GetMipsRegState(i))
        {
        case CRegInfo::STATE_UNKNOWN: UnMap_GPR(i, true);  break;
        case CRegInfo::STATE_MAPPED_64:
        {
                                          x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
                                          x86Reg x86RegHi = SyncTo.GetMipsRegMapHi(i);
                                          UnMap_X86reg(Reg);
                                          UnMap_X86reg(x86RegHi);
                                          switch (GetMipsRegState(i))
                                          {
                                          case CRegInfo::STATE_UNKNOWN:
                                              MoveVariableToX86reg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg);
                                              MoveVariableToX86reg(&_GPR[i].UW[1], CRegName::GPR_Hi[i], x86RegHi);
                                              break;
                                          case CRegInfo::STATE_MAPPED_64:
                                              MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                              m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                              MoveX86RegToX86Reg(GetMipsRegMapHi(i), x86RegHi);
                                              m_RegWorkingSet.SetX86Mapped(GetMipsRegMapHi(i), CRegInfo::NotMapped);
                                              break;
                                          case CRegInfo::STATE_MAPPED_32_SIGN:
                                              MoveX86RegToX86Reg(GetMipsRegMapLo(i), x86RegHi);
                                              ShiftRightSignImmed(x86RegHi, 31);
                                              MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                              m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                              break;
                                          case CRegInfo::STATE_MAPPED_32_ZERO:
                                              XorX86RegToX86Reg(x86RegHi, x86RegHi);
                                              MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                              m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                              break;
                                          case CRegInfo::STATE_CONST_64:
                                              MoveConstToX86reg(GetMipsRegHi(i), x86RegHi);
                                              MoveConstToX86reg(GetMipsRegLo(i), Reg);
                                              break;
                                          case CRegInfo::STATE_CONST_32_SIGN:
                                              MoveConstToX86reg(GetMipsRegLo_S(i) >> 31, x86RegHi);
                                              MoveConstToX86reg(GetMipsRegLo(i), Reg);
                                              break;
                                          default:
                                              CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_64\n%d", GetMipsRegState(i));
                                              g_Notify->BreakPoint(__FILE__, __LINE__);
                                              continue;
                                          }
                                          m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
                                          m_RegWorkingSet.SetMipsRegMapHi(i, x86RegHi);
                                          m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_64);
                                          m_RegWorkingSet.SetX86Mapped(Reg, CRegInfo::GPR_Mapped);
                                          m_RegWorkingSet.SetX86Mapped(x86RegHi, CRegInfo::GPR_Mapped);
                                          m_RegWorkingSet.SetX86MapOrder(Reg, 1);
                                          m_RegWorkingSet.SetX86MapOrder(x86RegHi, 1);
        }
            break;
        case CRegInfo::STATE_MAPPED_32_SIGN:
        {
                                               x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
                                               UnMap_X86reg(Reg);
                                               switch (GetMipsRegState(i))
                                               {
                                               case CRegInfo::STATE_UNKNOWN: MoveVariableToX86reg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg); break;
                                               case CRegInfo::STATE_CONST_32_SIGN: MoveConstToX86reg(GetMipsRegLo(i), Reg); break;
                                               case CRegInfo::STATE_MAPPED_32_SIGN:
                                                   MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                                   m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                                   break;
                                               case CRegInfo::STATE_MAPPED_32_ZERO:
                                                   if (GetMipsRegMapLo(i) != Reg)
                                                   {
                                                       MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                                       m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                                   }
                                                   break;
                                               case CRegInfo::STATE_MAPPED_64:
                                                   MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                                   m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                                   m_RegWorkingSet.SetX86Mapped(GetMipsRegMapHi(i), CRegInfo::NotMapped);
                                                   break;
                                               case CRegInfo::STATE_CONST_64:
                                                   CPU_Message("hi %X\nLo %X", GetMipsRegHi(i), GetMipsRegLo(i));
                                               default:
                                                   CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_SIGN\n%d", GetMipsRegState(i));
                                                   g_Notify->BreakPoint(__FILE__, __LINE__);
                                               }
                                               m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
                                               m_RegWorkingSet.SetMipsRegState(i, CRegInfo::STATE_MAPPED_32_SIGN);
                                               m_RegWorkingSet.SetX86Mapped(Reg, CRegInfo::GPR_Mapped);
                                               m_RegWorkingSet.SetX86MapOrder(Reg, 1);
        }
            break;
        case CRegInfo::STATE_MAPPED_32_ZERO:
        {
                                               x86Reg Reg = SyncTo.GetMipsRegMapLo(i);
                                               UnMap_X86reg(Reg);
                                               switch (GetMipsRegState(i))
                                               {
                                               case CRegInfo::STATE_MAPPED_64:
                                               case CRegInfo::STATE_UNKNOWN:
                                                   MoveVariableToX86reg(&_GPR[i].UW[0], CRegName::GPR_Lo[i], Reg);
                                                   break;
                                               case CRegInfo::STATE_MAPPED_32_ZERO:
                                                   MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                                   m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                                   break;
                                               case CRegInfo::STATE_MAPPED_32_SIGN:
                                                   if (g_System->b32BitCore())
                                                   {
                                                       MoveX86RegToX86Reg(GetMipsRegMapLo(i), Reg);
                                                       m_RegWorkingSet.SetX86Mapped(GetMipsRegMapLo(i), CRegInfo::NotMapped);
                                                   }
                                                   else
                                                   {
                                                       CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                                                       g_Notify->BreakPoint(__FILE__, __LINE__);
                                                   }
                                                   break;
                                               case CRegInfo::STATE_CONST_32_SIGN:
                                                   if (!g_System->b32BitCore() && GetMipsRegLo_S(i) < 0)
                                                   {
                                                       CPU_Message("Sign problems in SyncRegState\nSTATE_MAPPED_32_ZERO");
                                                       CPU_Message("%s: %X", CRegName::GPR[i], GetMipsRegLo_S(i));
                                                       g_Notify->BreakPoint(__FILE__, __LINE__);
                                                   }
                                                   MoveConstToX86reg(GetMipsRegLo(i), Reg);
                                                   break;
                                               default:
                                                   CPU_Message("Do something with states in SyncRegState\nSTATE_MAPPED_32_ZERO\n%d", GetMipsRegState(i));
                                                   g_Notify->BreakPoint(__FILE__, __LINE__);
                                               }
                                               m_RegWorkingSet.SetMipsRegMapLo(i, Reg);
                                               m_RegWorkingSet.SetMipsRegState(i, SyncTo.GetMipsRegState(i));
                                               m_RegWorkingSet.SetX86Mapped(Reg, CRegInfo::GPR_Mapped);
                                               m_RegWorkingSet.SetX86MapOrder(Reg, 1);
        }
            break;
        default:
            CPU_Message("%d - %d reg: %s (%d)", SyncTo.GetMipsRegState(i), GetMipsRegState(i), CRegName::GPR[i], i);
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

        if (Parent->m_CompiledLocation == nullptr) { continue; }
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

        if (Parent->m_CompiledLocation != nullptr) { continue; }
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

    if (JumpInfo->ExitReason == CExitInfo::Normal_NoSysCheck)
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
            CPU_Message("CompileSystemCheck 10");
            CompileSystemCheck(m_Section->m_EnterPC, GetRegWorkingSet());
        }
    }
    JumpInfo->FallThrough = false;

    // Fix up initial state
    UnMap_AllFPRs();

    // Determine loop registry usage
    if (m_Section->m_InLoop && ParentList.size() > 1)
    {
        if (!SetupRegisterForLoop(m_Section->m_BlockInfo, m_Section->m_RegEnter)) { return false; }
        m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown);
    }

    for (size_t i = 0; i < ParentList.size(); i++)
    {
        x86Reg MemoryStackPos;
        int i2;

        if (i == (size_t)FirstParent) { continue; }
        Parent = ParentList[i].Parent;
        if (Parent->m_CompiledLocation == nullptr)
        {
            continue;
        }
        CRegInfo * RegSet = &ParentList[i].JumpInfo->RegSet;

        if (m_RegWorkingSet.GetRoundingModel() != RegSet->GetRoundingModel()) { m_RegWorkingSet.SetRoundingModel(CRegInfo::RoundUnknown); }

        // Find parent MapRegState
        MemoryStackPos = x86_Unknown;
        for (i2 = 0; i2 < sizeof(x86_Registers) / sizeof(x86_Registers[0]); i2++)
        {
            if (RegSet->GetX86Mapped(x86_Registers[i2]) == CRegInfo::Stack_Mapped)
            {
                MemoryStackPos = x86_Registers[i2];
                break;
            }
        }
        if (MemoryStackPos == x86_Unknown)
        {
            // If the memory stack position is not mapped then unmap it
            x86Reg MemStackReg = Get_MemoryStack();
            if (MemStackReg != x86_Unknown)
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
                    CPU_Message("Unknown CPU state(%d) in InheritParentInfo", GetMipsRegState(i2));
                    g_Notify->BreakPoint(__FILE__, __LINE__);
                }
            }
            if (IsConst(i2)) {
                if (GetMipsRegState(i2) != RegSet->GetMipsRegState(i2))
                {
                    switch (RegSet->GetMipsRegState(i2))
                    {
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
                        CPU_Message("Unknown CPU state(%d) in InheritParentInfo", RegSet->GetMipsRegState(i2));
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

        if (i == (size_t)FirstParent) { continue; }
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;
        RegSet = &ParentList[i].JumpInfo->RegSet;

        if (JumpInfo->RegSet.GetBlockCycleCount() != 0) { NeedSync = true; }

        for (i2 = 0; !NeedSync && i2 < 8; i2++)
        {
            if (m_RegWorkingSet.FpuMappedTo(i2) == (uint32_t)-1)
            {
                NeedSync = true;
            }
        }

        for (i2 = 0; !NeedSync && i2 < sizeof(x86_Registers) / sizeof(x86_Registers[0]); i2++)
        {
            if (m_RegWorkingSet.GetX86Mapped(x86_Registers[i2]) == CRegInfo::Stack_Mapped)
            {
                if (m_RegWorkingSet.GetX86Mapped(x86_Registers[i2]) != RegSet->GetX86Mapped(x86_Registers[i2]))
                {
                    NeedSync = true;
                }
                break;
            }
        }
        for (i2 = 0; !NeedSync && i2 < 32; i2++)
        {
            if (NeedSync == true)  { break; }
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
        if (NeedSync == false) { continue; }
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        JmpLabel32(Label.c_str(), 0);
        JumpInfo->LinkLocation = (uint32_t *)(*g_RecompPos - 4);
        JumpInfo->LinkLocation2 = nullptr;

        CurrentParent = i;
        Parent = ParentList[CurrentParent].Parent;
        JumpInfo = ParentList[CurrentParent].JumpInfo;
        CPU_Message("   Section_%d (from %d):", m_Section->m_SectionID, Parent->m_SectionID);
        if (JumpInfo->LinkLocation != nullptr)
        {
            SetJump32(JumpInfo->LinkLocation, (uint32_t *)*g_RecompPos);
            JumpInfo->LinkLocation = nullptr;
            if (JumpInfo->LinkLocation2 != nullptr)
            {
                SetJump32(JumpInfo->LinkLocation2, (uint32_t *)*g_RecompPos);
                JumpInfo->LinkLocation2 = nullptr;
            }
        }

        m_RegWorkingSet = JumpInfo->RegSet;
        if (m_Section->m_EnterPC < JumpInfo->JumpPC)
        {
            UpdateCounters(m_RegWorkingSet, true, true);
            CPU_Message("CompileSystemCheck 11");
            CompileSystemCheck(m_Section->m_EnterPC, m_RegWorkingSet);
        }
        else
        {
            UpdateCounters(m_RegWorkingSet, false, true);
        }
        SyncRegState(m_Section->m_RegEnter);         // Sync
        m_Section->m_RegEnter = m_RegWorkingSet;
    }

    for (size_t i = 0; i < NoOfCompiledParents; i++)
    {
        Parent = ParentList[i].Parent;
        JumpInfo = ParentList[i].JumpInfo;
        LinkJump(*JumpInfo);
    }

    CPU_Message("   Section_%d:", m_Section->m_SectionID);
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
                CPU_Message("   Section_%d (from %d):", SectionID, FromSectionID);
            }
            else
            {
                CPU_Message("   Section_%d:", SectionID);
            }
        }
        SetJump32(JumpInfo.LinkLocation, (uint32_t *)*g_RecompPos);
        JumpInfo.LinkLocation = nullptr;
        if (JumpInfo.LinkLocation2 != nullptr)
        {
            SetJump32(JumpInfo.LinkLocation2, (uint32_t *)*g_RecompPos);
            JumpInfo.LinkLocation2 = nullptr;
        }
    }
}

void CX86RecompilerOps::JumpToSection(CCodeSection * Section)
{
    char Label[100];
    sprintf(Label, "Section_%d", Section->m_SectionID);
    JmpLabel32(Label, 0);
    SetJump32(((uint32_t *)*g_RecompPos) - 1, (uint32_t *)(Section->m_CompiledLocation));
}

void CX86RecompilerOps::JumpToUnknown(CJumpInfo * JumpInfo)
{
    JmpLabel32(JumpInfo->BranchLabel.c_str(), 0);
    JumpInfo->LinkLocation = (uint32_t*)(*g_RecompPos - 4);
}

void CX86RecompilerOps::SetCurrentPC(uint32_t ProgramCounter)
{
    m_CompilePC = ProgramCounter;
    __except_try()
    {
        if (!g_MMU->MemoryValue32(m_CompilePC, m_Opcode.Hex))
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

const OPCODE & CX86RecompilerOps::GetOpcode(void) const
{
    return m_Opcode;
}

void CX86RecompilerOps::UpdateSyncCPU(CRegInfo & RegSet, uint32_t Cycles)
{
    if (!g_SyncSystem)
    {
        return;
    }

    WriteX86Comment("Updating sync CPU");
    RegSet.BeforeCallDirect();
    PushImm32(stdstr_f("%d", Cycles).c_str(), Cycles);
    PushImm32("g_SyncSystem", (uint32_t)g_SyncSystem);
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_System, x86_ECX);
    Call_Direct(AddressOf(&CN64System::UpdateSyncCPU), "CN64System::UpdateSyncCPU");
#else
    PushImm32((uint32_t)g_System);
    Call_Direct(AddressOf(&CN64System::UpdateSyncCPU), "CN64System::UpdateSyncCPU");
    AddConstToX86Reg(x86_ESP, 12);
#endif
    RegSet.AfterCallDirect();
}

void CX86RecompilerOps::UpdateCounters(CRegInfo & RegSet, bool CheckTimer, bool ClearValues, bool UpdateTimer)
{
    if (RegSet.GetBlockCycleCount() != 0)
    {
        UpdateSyncCPU(RegSet, RegSet.GetBlockCycleCount());
        WriteX86Comment("Update counter");
        SubConstFromVariable(RegSet.GetBlockCycleCount(), g_NextTimer, "g_NextTimer"); // Updates compare flag
        if (ClearValues)
        {
            RegSet.SetBlockCycleCount(0);
        }
    }
    else if (CheckTimer)
    {
        CompConstToVariable(0, g_NextTimer, "g_NextTimer");
    }

    if (CheckTimer)
    {
        JnsLabel8("Continue_From_Timer_Test", 0);
        uint8_t * Jump = *g_RecompPos - 1;
        RegSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::TimerDone), "CSystemTimer::TimerDone");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        RegSet.AfterCallDirect();

        CPU_Message("");
        CPU_Message("      $Continue_From_Timer_Test:");
        SetJump8(Jump, *g_RecompPos);
    }

    if (UpdateTimer && g_SyncSystem)
    {
        m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemTimer, x86_ECX);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
#else
        PushImm32((uint32_t)g_SystemTimer);
        Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        m_RegWorkingSet.AfterCallDirect();
    }
}

void CX86RecompilerOps::CompileSystemCheck(uint32_t TargetPC, const CRegInfo & RegSet)
{
    CompConstToVariable(0, (void *)&g_SystemEvents->DoSomething(), "g_SystemEvents->DoSomething()");
    JeLabel32("Continue_From_Interrupt_Test", 0);
    uint32_t * Jump = (uint32_t *)(*g_RecompPos - 4);
    if (TargetPC != (uint32_t)-1)
    {
        MoveConstToVariable(TargetPC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
    }

    CRegInfo RegSetCopy(RegSet);
    RegSetCopy.WriteBackRegisters();

#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_SystemEvents, x86_ECX);
    Call_Direct(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
#else
    PushImm32((uint32_t)g_SystemEvents);
    Call_Direct(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
    AddConstToX86Reg(x86_ESP, 4);
#endif
    ExitCodeBlock();
    CPU_Message("");
    CPU_Message("      $Continue_From_Interrupt_Test:");
    SetJump32(Jump, (uint32_t *)*g_RecompPos);
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
    MoveConstToVariable(CompilePC(), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem)
    {
#ifdef _WIN32
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
    Call_Direct((void *)x86_compiler_Break_Point, "x86_compiler_Break_Point");
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
    MoveConstToVariable(CompilePC(), _PROGRAM_COUNTER, "PROGRAM_COUNTER");
    if (g_SyncSystem)
    {
#ifdef _WIN32
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
    Call_Direct((void *)x86_Break_Point_DelaySlot, "x86_Break_Point_DelaySlot");
    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::OverflowDelaySlot(bool TestTimer)
{
    m_RegWorkingSet.WriteBackRegisters();
    UpdateCounters(m_RegWorkingSet, false, true);
    MoveConstToVariable(CompilePC() + 4, _PROGRAM_COUNTER, "PROGRAM_COUNTER");

    if (g_SyncSystem)
    {
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
        PushImm32((uint32_t)g_BaseSystem);
        Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }

    MoveConstToVariable(PIPELINE_STAGE_JUMP, &g_System->m_PipelineStage, "System->m_PipelineStage");

    if (TestTimer)
    {
        MoveConstToVariable(TestTimer, &R4300iOp::m_TestTimer, "R4300iOp::m_TestTimer");
    }

    PushImm32("g_System->CountPerOp()", g_System->CountPerOp());
    Call_Direct((void *)CInterpreterCPU::ExecuteOps, "CInterpreterCPU::ExecuteOps");
    AddConstToX86Reg(x86_ESP, 4);

    if (g_System->bFastSP() && g_Recompiler)
    {
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_Recompiler, x86_ECX);
        Call_Direct(AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos");
#else
        PushImm32((uint32_t)g_Recompiler);
        Call_Direct(AddressOf(&CRecompiler::ResetMemoryStackPos), "CRecompiler::ResetMemoryStackPos");
        AddConstToX86Reg(x86_ESP, 4);
#endif
    }
    if (g_SyncSystem)
    {
        UpdateSyncCPU(m_RegWorkingSet, g_System->CountPerOp());
    }

    ExitCodeBlock();
    m_PipelineStage = PIPELINE_STAGE_END_BLOCK;
}

void CX86RecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason)
{
    CompileExit(JumpPC, TargetPC, ExitRegSet, reason, true, nullptr);
}

void CX86RecompilerOps::CompileExit(uint32_t JumpPC, uint32_t TargetPC, CRegInfo &ExitRegSet, CExitInfo::EXIT_REASON reason, bool CompileNow, void(*x86Jmp)(const char * Label, uint32_t Value))
{
    if (!CompileNow)
    {
        char String[100];
        sprintf(String, "Exit_%d", m_ExitInfo.size());
        if (x86Jmp == nullptr)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
        x86Jmp(String, 0);

        CExitInfo ExitInfo;
        ExitInfo.ID = m_ExitInfo.size();
        ExitInfo.TargetPC = TargetPC;
        ExitInfo.ExitRegSet = ExitRegSet;
        ExitInfo.reason = reason;
        ExitInfo.PipelineStage = m_PipelineStage;
        ExitInfo.JumpLoc = (uint32_t *)(*g_RecompPos - 4);
        m_ExitInfo.push_back(ExitInfo);
        return;
    }

    //CPU_Message("CompileExit: %d",reason);
    ExitRegSet.WriteBackRegisters();

    if (TargetPC != (uint32_t)-1)
    {
        MoveConstToVariable(TargetPC, &g_Reg->m_PROGRAM_COUNTER, "PROGRAM_COUNTER");
        UpdateCounters(ExitRegSet, TargetPC <= JumpPC && JumpPC != -1, reason == CExitInfo::Normal);
    }
    else
    {
        UpdateCounters(ExitRegSet, false, reason == CExitInfo::Normal);
    }

    switch (reason)
    {
    case CExitInfo::Normal:
    case CExitInfo::Normal_NoSysCheck:
        ExitRegSet.SetBlockCycleCount(0);
        if (TargetPC != (uint32_t)-1)
        {
            if (TargetPC <= JumpPC && reason == CExitInfo::Normal)
            {
                CPU_Message("CompileSystemCheck 1");
                CompileSystemCheck((uint32_t)-1, ExitRegSet);
            }
        }
        else
        {
            if (reason == CExitInfo::Normal)
            {
                CPU_Message("CompileSystemCheck 2");
                CompileSystemCheck((uint32_t)-1, ExitRegSet);
            }
        }
#ifdef LinkBlocks
        if (g_SyncSystem)
        {
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)g_BaseSystem, x86_ECX);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
#else
            PushImm32((uint32_t)g_BaseSystem);
            Call_Direct(AddressOf(&CN64System::SyncSystem), "CN64System::SyncSystem");
            AddConstToX86Reg(x86_ESP, 4);
#endif
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
                //                MoveVariableToX86reg((uint8_t *)RDRAM + pAddr,"RDRAM + pAddr",x86_EAX);
                //                Jump2 = nullptr;
                //            } else {
                //                MoveConstToX86reg((TargetPC >> 12),x86_ECX);
                //                MoveConstToX86reg(TargetPC,x86_EBX);
                //                MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",x86_ECX,x86_ECX,4);
                //                TestX86RegToX86Reg(x86_ECX,x86_ECX);
                //                JeLabel8("NoTlbEntry",0);
                //                Jump2 = *g_RecompPos - 1;
                //                MoveX86regPointerToX86reg(x86_ECX, x86_EBX,x86_EAX);
                //            }
                //            MoveX86RegToX86Reg(x86_EAX,x86_ECX);
                //            AndConstToX86Reg(x86_ECX,0xFFFF0000);
                //            CompConstToX86reg(x86_ECX,0x7C7C0000);
                //            JneLabel8("NoCode",0);
                //            Jump = *g_RecompPos - 1;
                //            AndConstToX86Reg(x86_EAX,0xFFFF);
                //            ShiftLeftSignImmed(x86_EAX,4);
                //            AddConstToX86Reg(x86_EAX,0xC);
                //            MoveVariableDispToX86Reg(OrigMem,"OrigMem",x86_ECX,x86_EAX,1);
                //            JmpDirectReg(x86_ECX);
                //            CPU_Message("      NoCode:");
                //            *((uint8_t *)(Jump))=(uint8_t)(*g_RecompPos - Jump - 1);
                //            if (Jump2 != nullptr) {
                //                CPU_Message("      NoTlbEntry:");
                //                *((uint8_t *)(Jump2))=(uint8_t)(*g_RecompPos - Jump2 - 1);
                //            }
            }
            else if (LookUpMode() == FuncFind_VirtualLookup)
            {
                MoveConstToX86reg(TargetPC, x86_EDX);
                MoveConstToX86reg((uint32_t)&m_Functions, x86_ECX);
                Call_Direct(AddressOf(&CFunctionMap::CompilerFindFunction), "CFunctionMap::CompilerFindFunction");
                MoveX86RegToX86Reg(x86_EAX, x86_ECX);
                JecxzLabel8("NullPointer", 0);
                uint8_t * Jump = *g_RecompPos - 1;
                MoveX86PointerToX86regDisp(x86_EBX, x86_ECX, 0xC);
                JmpDirectReg(x86_EBX);
                CPU_Message("      NullPointer:");
                *((uint8_t *)(Jump)) = (uint8_t)(*g_RecompPos - Jump - 1);
            }
            else if (LookUpMode() == FuncFind_PhysicalLookup)
            {
                uint8_t * Jump2 = nullptr;
                if (TargetPC >= 0x80000000 && TargetPC < 0x90000000)
                {
                    uint32_t pAddr = TargetPC & 0x1FFFFFFF;
                    MoveVariableToX86reg((uint8_t *)JumpTable + pAddr, "JumpTable + pAddr", x86_ECX);
                }
                else if (TargetPC >= 0x90000000 && TargetPC < 0xC0000000)
                {
                }
                else
                {
                    MoveConstToX86reg((TargetPC >> 12), x86_ECX);
                    MoveConstToX86reg(TargetPC, x86_EBX);
                    MoveVariableDispToX86Reg(TLB_ReadMap, "TLB_ReadMap", x86_ECX, x86_ECX, 4);
                    TestX86RegToX86Reg(x86_ECX, x86_ECX);
                    JeLabel8("NoTlbEntry", 0);
                    Jump2 = *g_RecompPos - 1;
                    AddConstToX86Reg(x86_ECX, (uint32_t)JumpTable - (uint32_t)RDRAM);
                    MoveX86regPointerToX86reg(x86_ECX, x86_EBX, x86_ECX);
                }
                if (TargetPC < 0x90000000 || TargetPC >= 0xC0000000)
                {
                    JecxzLabel8("NullPointer", 0);
                    uint8_t * Jump = *g_RecompPos - 1;
                    MoveX86PointerToX86regDisp(x86_EAX, x86_ECX, 0xC);
                    JmpDirectReg(x86_EAX);
                    CPU_Message("      NullPointer:");
                    *((uint8_t *)(Jump)) = (uint8_t)(*g_RecompPos - Jump - 1);
                    if (Jump2 != nullptr)
                    {
                        CPU_Message("      NoTlbEntry:");
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
    case CExitInfo::DoCPU_Action:
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_SystemEvents, x86_ECX);
        Call_Direct(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
#else
        PushImm32((uint32_t)g_SystemEvents);
        Call_Direct(AddressOf(&CSystemEvents::ExecuteEvents), "CSystemEvents::ExecuteEvents");
        AddConstToX86Reg(x86_ESP, 4);
#endif
        ExitCodeBlock();
        break;
    case CExitInfo::DoSysCall:
        {
             bool bDelay = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
             PushImm32(bDelay ? "true" : "false", bDelay);
#ifdef _MSC_VER
             MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
             Call_Direct(AddressOf(&CRegisters::DoSysCallException), "CRegisters::DoSysCallException");
#else
            PushImm32((uint32_t)g_Reg);
             Call_Direct(AddressOf(&CRegisters::DoSysCallException), "CRegisters::DoSysCallException");
            AddConstToX86Reg(x86_ESP, 4);
#endif
             ExitCodeBlock();
        }
        break;
    case CExitInfo::COP1_Unuseable:
        {
            bool bDelay = m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT;
            PushImm32("1", 1);
            PushImm32(bDelay ? "true" : "false", bDelay);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
            Call_Direct(AddressOf(&CRegisters::DoCopUnusableException), "CRegisters::DoCopUnusableException");
#else
            PushImm32((uint32_t)g_Reg);
            Call_Direct(AddressOf(&CRegisters::DoCopUnusableException), "CRegisters::DoCopUnusableException");
            AddConstToX86Reg(x86_ESP, 12);
#endif
            ExitCodeBlock();
        }
        break;
    case CExitInfo::ExitResetRecompCode:
        g_Notify->BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case CExitInfo::TLBReadMiss:
        MoveVariableToX86reg(g_TLBLoadAddress, "g_TLBLoadAddress", x86_EDX);
        Push(x86_EDX);
        PushImm32(m_PipelineStage == PIPELINE_STAGE_JUMP || m_PipelineStage == PIPELINE_STAGE_DELAY_SLOT);
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
        Call_Direct(AddressOf(&CRegisters::DoTLBReadMiss), "CRegisters::DoTLBReadMiss");
#else
        PushImm32((uint32_t)g_Reg);
        Call_Direct(AddressOf(&CRegisters::DoTLBReadMiss), "CRegisters::DoTLBReadMiss");
        AddConstToX86Reg(x86_ESP, 12);
#endif
        ExitCodeBlock();
        break;
    case CExitInfo::TLBWriteMiss:
        X86BreakPoint(__FILE__, __LINE__);
        ExitCodeBlock();
        break;
    case CExitInfo::DivByZero:
        AddConstToVariable(4, _PROGRAM_COUNTER, "PROGRAM_COUNTER");
        if (!g_System->b32BitCore())
        {
            MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
            MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
        }
        MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
        MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
        ExitCodeBlock();
        break;
    default:
        WriteTrace(TraceRecompiler, TraceError, "How did you want to exit on reason (%d) ???", reason);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86RecompilerOps::Compile_StoreInstructClean(x86Reg AddressReg, int32_t Length)
{
    if (!g_System->bSMM_StoreInstruc())
    {
        return;
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);

    /*
    stdstr_f strLen("%d",Length);
    UnMap_AllFPRs();

    /*x86Reg StoreTemp1 = Map_TempReg(x86_Any,-1,false);
    MoveX86RegToX86Reg(AddressReg, StoreTemp1);
    AndConstToX86Reg(StoreTemp1,0xFFC);*/
    m_RegWorkingSet.BeforeCallDirect();
    PushImm32("CRecompiler::Remove_StoreInstruc", CRecompiler::Remove_StoreInstruc);
    PushImm32(Length);
    Push(AddressReg);
#ifdef _MSC_VER
    MoveConstToX86reg((uint32_t)g_Recompiler, x86_ECX);
    Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
#else
    PushImm32((uint32_t)g_Recompiler);
    Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
    AddConstToX86Reg(x86_ESP, 16);
#endif
    m_RegWorkingSet.AfterCallDirect();
    /*JmpLabel8("MemCheckDone",0);
    uint8_t * MemCheckDone = *g_RecompPos - 1;

    CPU_Message("      ");
    CPU_Message("      NotDelaySlot:");
    SetJump8(NotDelaySlotJump,*g_RecompPos);

    MoveX86RegToX86Reg(AddressReg, StoreTemp1);
    ShiftRightUnsignImmed(StoreTemp1,12);
    LeaRegReg(StoreTemp1,StoreTemp1,(uint32_t)&(g_Recompiler->FunctionTable()[0]),Multip_x4);
    CompConstToX86regPointer(StoreTemp1,0);
    JeLabel8("MemCheckDone",0);
    uint8_t * MemCheckDone2 = *g_RecompPos - 1;

    m_RegWorkingSet.BeforeCallDirect();
    PushImm32("CRecompiler::Remove_StoreInstruc",CRecompiler::Remove_StoreInstruc);
    PushImm32(strLen.c_str(),Length);
    Push(AddressReg);
    MoveConstToX86reg((uint32_t)g_Recompiler,x86_ECX);
    Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
    m_RegWorkingSet.AfterCallDirect();

    CPU_Message("      ");
    CPU_Message("      MemCheckDone:");
    SetJump8(MemCheckDone,*g_RecompPos);
    SetJump8(MemCheckDone2,*g_RecompPos);

    X86Protected(StoreTemp1) = false;*/
}

CX86Ops::x86Reg CX86RecompilerOps::BaseOffsetAddress(bool UseBaseRegister)
{
    x86Reg AddressReg;
    if (IsMapped(m_Opcode.base))
    {
        if (m_Opcode.offset != 0)
        {
            ProtectGPR(m_Opcode.base);
            AddressReg = Map_TempReg(x86_Any, -1, false);
            LeaSourceAndOffset(AddressReg, GetMipsRegMapLo(m_Opcode.base), (int16_t)m_Opcode.offset);
            UnProtectGPR(m_Opcode.base);
        }
        else if (UseBaseRegister)
        {
            ProtectGPR(m_Opcode.base);
            AddressReg = GetMipsRegMapLo(m_Opcode.base);
        }
        else
        {
            AddressReg = Map_TempReg(x86_Any, m_Opcode.base, false);
        }
    }
    else
    {
        AddressReg = Map_TempReg(x86_Any, m_Opcode.base, false);
        AddConstToX86Reg(AddressReg, (int16_t)m_Opcode.immediate);
    }
    return AddressReg;
}

void CX86RecompilerOps::CompileStoreMemoryValue(CX86Ops::x86Reg AddressReg, CX86Ops::x86Reg ValueReg, CX86Ops::x86Reg ValueRegHi, uint64_t Value, uint8_t ValueSize)
{
    x86Reg TempReg = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(AddressReg, TempReg);
    ShiftRightUnsignImmed(TempReg, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryWriteMap, "MMU->m_MemoryWriteMap", TempReg, TempReg, 4);
    CompConstToX86reg(TempReg, (uint32_t)-1);
    JneLabel8(stdstr_f("MemoryWriteMap_%X_Found", m_CompilePC).c_str(), 0);
    uint8_t * JumpFound = (uint8_t *)(*g_RecompPos - 1);
    MoveX86RegToX86Reg(AddressReg, TempReg);
    ShiftRightUnsignImmed(TempReg, 12);
    MoveVariableDispToX86Reg(g_MMU->m_TLB_WriteMap, "MMU->TLB_WriteMap", TempReg, TempReg, 4);
    CompileWriteTLBMiss(AddressReg, TempReg);
    AddConstToX86Reg(TempReg, (uint32_t)m_MMU.Rdram());
    CPU_Message("");
    CPU_Message(stdstr_f("      MemoryWriteMap_%X_Found:", m_CompilePC).c_str());
    SetJump8(JumpFound, *g_RecompPos);

    if (ValueSize == 8)
    {
        XorConstToX86Reg(AddressReg, 3);
        if (ValueReg == x86_Unknown)
        {
            MoveConstByteToX86regPointer((uint8_t)(Value & 0xFF), AddressReg, TempReg);
        }
        else if (Is8BitReg(ValueReg))
        {            
            MoveX86regByteToX86regPointer(ValueReg, AddressReg, TempReg);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else if (ValueSize == 16)
    {
        XorConstToX86Reg(AddressReg, 2);
        if (ValueReg == x86_Unknown)
        {
            MoveConstHalfToX86regPointer((uint16_t)(Value & 0xFFFF), AddressReg, TempReg);
        }
        else
        {
            MoveX86regHalfToX86regPointer(ValueReg, AddressReg, TempReg);
        }
    }
    else if (ValueSize == 32)
    {
        if (ValueReg == x86_Unknown)
        {
            MoveConstToX86regPointer((uint32_t)(Value & 0xFFFFFFFF), AddressReg, TempReg);
        }
        else
        {
            MoveX86regToX86regPointer(ValueReg, AddressReg, TempReg);
        }
    }
    else if (ValueSize == 64)
    {
        if (ValueReg == x86_Unknown)
        {
            MoveConstToX86regPointer((uint32_t)(Value >> 32), AddressReg, TempReg);
            AddConstToX86Reg(AddressReg, 4);
            MoveConstToX86regPointer((uint32_t)(Value & 0xFFFFFFFF), AddressReg, TempReg);
        }
        else
        {
            MoveX86regToX86regPointer(ValueRegHi, AddressReg, TempReg);
            AddConstToX86Reg(AddressReg, 4);
            MoveX86regToX86regPointer(ValueReg, AddressReg, TempReg);
        }
    }   
    else
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CX86RecompilerOps::SB_Const(uint8_t Value, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        x86Reg AddressReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddressReg);
        CompileStoreMemoryValue(AddressReg, x86_Unknown, x86_Unknown, Value, 8);
        return;
    }

    uint32_t PAddr;
    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory()) { g_Notify->DisplayError(stdstr_f("%s, \nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str()); }
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
        MoveConstByteToVariable(Value, (PAddr ^ 3) + g_MMU->Rdram(), stdstr_f("RDRAM + %X", (PAddr ^ 3)).c_str());
        break;
    default:
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nTrying to store %02X in %08X?", __FUNCTION__, Value, VAddr).c_str());
        }
    }
}

void CX86RecompilerOps::SB_Register(x86Reg Reg, uint32_t VAddr)
{
    char VarName[100];
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetX86Protected(Reg, true);
        x86Reg AddressReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddressReg);
        CompileStoreMemoryValue(AddressReg, Reg, x86_Unknown, 0, 8);
        return;
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
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
        sprintf(VarName, "RDRAM + %X", (PAddr ^ 3));
        MoveX86regByteToVariable(Reg, (PAddr ^ 3) + g_MMU->Rdram(), VarName);
        break;
    default:
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
        }
    }
}

void CX86RecompilerOps::SH_Const(uint16_t Value, uint32_t VAddr)
{
    char VarName[100];
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        x86Reg AddressReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddressReg);
        CompileStoreMemoryValue(AddressReg, x86_Unknown, x86_Unknown, Value, 16);
        return;
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
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
        sprintf(VarName, "RDRAM + %X", (PAddr ^ 2));
        MoveConstHalfToVariable(Value, (PAddr ^ 2) + g_MMU->Rdram(), VarName);
        break;
    default:
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nTrying to store %04X in %08X?", __FUNCTION__, Value, VAddr).c_str());
        }
    }
}

void CX86RecompilerOps::SH_Register(x86Reg Reg, uint32_t VAddr)
{
    char VarName[100];
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetX86Protected(Reg, true);

        x86Reg AddressReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddressReg);
        CompileStoreMemoryValue(AddressReg, Reg, x86_Unknown, 0, 16);
        return;
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
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
        sprintf(VarName, "RDRAM + %X", (PAddr ^ 2));
        MoveX86regHalfToVariable(Reg, (PAddr ^ 2) + g_MMU->Rdram(), VarName);
        break;
    default:
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, PAddr).c_str());
        }
    }
}

void CX86RecompilerOps::SW_Const(uint32_t Value, uint32_t VAddr)
{
    char VarName[100];
    uint8_t * Jump;
    uint32_t PAddr;

    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        x86Reg AddressReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddressReg);
        CompileStoreMemoryValue(AddressReg, x86_Unknown, x86_Unknown, Value, 32);
        return;
    }

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
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
        sprintf(VarName, "RDRAM + %X", PAddr);
        MoveConstToVariable(Value, PAddr + g_MMU->Rdram(), VarName);
        break;
    case 0x03F00000:
        switch (PAddr)
        {
        case 0x03F00000: MoveConstToVariable(Value, &g_Reg->RDRAM_CONFIG_REG, "RDRAM_CONFIG_REG"); break;
        case 0x03F00004: MoveConstToVariable(Value, &g_Reg->RDRAM_DEVICE_ID_REG, "RDRAM_DEVICE_ID_REG"); break;
        case 0x03F00008: MoveConstToVariable(Value, &g_Reg->RDRAM_DELAY_REG, "RDRAM_DELAY_REG"); break;
        case 0x03F0000C: MoveConstToVariable(Value, &g_Reg->RDRAM_MODE_REG, "RDRAM_MODE_REG"); break;
        case 0x03F00010: MoveConstToVariable(Value, &g_Reg->RDRAM_REF_INTERVAL_REG, "RDRAM_REF_INTERVAL_REG"); break;
        case 0x03F00014: MoveConstToVariable(Value, &g_Reg->RDRAM_REF_ROW_REG, "RDRAM_REF_ROW_REG"); break;
        case 0x03F00018: MoveConstToVariable(Value, &g_Reg->RDRAM_RAS_INTERVAL_REG, "RDRAM_RAS_INTERVAL_REG"); break;
        case 0x03F0001C: MoveConstToVariable(Value, &g_Reg->RDRAM_MIN_INTERVAL_REG, "RDRAM_MIN_INTERVAL_REG"); break;
        case 0x03F00020: MoveConstToVariable(Value, &g_Reg->RDRAM_ADDR_SELECT_REG, "RDRAM_ADDR_SELECT_REG"); break;
        case 0x03F00024: MoveConstToVariable(Value, &g_Reg->RDRAM_DEVICE_MANUF_REG, "RDRAM_DEVICE_MANUF_REG"); break;
        case 0x03F04004: break;
        case 0x03F08004: break;
        case 0x03F80004: break;
        case 0x03F80008: break;
        case 0x03F8000C: break;
        case 0x03F80014: break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
        }
        break;
    case 0x04000000:
        if (PAddr < 0x04002000)
        {
            sprintf(VarName, "RDRAM + %X", PAddr);
            MoveConstToVariable(Value, PAddr + g_MMU->Rdram(), VarName);
            break;
        }
        switch (PAddr)
        {
        case 0x04040000: MoveConstToVariable(Value, &g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG"); break;
        case 0x04040004: MoveConstToVariable(Value, &g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG"); break;
        case 0x04040008:
            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(0xFFFFFFFF);
            PushImm32(Value);
            PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_SPRegistersHandler, x86_ECX);
            Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_SPRegistersHandler)[0][1], "SPRegistersHandler::Write32");
#else
            PushImm32((uint32_t)&g_MMU->m_SPRegistersHandler);
            Call_Direct(AddressOf(&SPRegistersHandler::Write32), "SPRegistersHandler::Write32");
            AddConstToX86Reg(x86_ESP, 16);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04040010:
            {
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
                UpdateCounters(m_RegWorkingSet, false, true);
                m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

                m_RegWorkingSet.BeforeCallDirect();
                PushImm32(Value);
                PushImm32(PAddr | 0xA0000000);
#ifdef _MSC_VER
                MoveConstToX86reg((uint32_t)(g_MMU), x86_ECX);
                Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
#else
                PushImm32((uint32_t)(g_MMU));
                Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
                AddConstToX86Reg(x86_ESP, 12);
#endif
                m_RegWorkingSet.AfterCallDirect();
            }
            break;
        case 0x0404001C: MoveConstToVariable(0, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG"); break;
        case 0x04080000: MoveConstToVariable(Value & 0xFFC, &g_Reg->SP_PC_REG, "SP_PC_REG"); break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
        }
        break;
    case 0x04100000:
        switch (PAddr)
        {
        case 0x0410000C:
            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(Value);
            PushImm32(PAddr | 0xA0000000);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            m_RegWorkingSet.AfterCallDirect();
#else
            PushImm32((uint32_t)(g_MMU));
            Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            AddConstToX86Reg(x86_ESP, 12);
#endif
            break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
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
                   AndConstToVariable(~ModValue, &g_Reg->MI_MODE_REG, "MI_MODE_REG");
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
               if (ModValue != 0) {
                   OrConstToVariable(ModValue, &g_Reg->MI_MODE_REG, "MI_MODE_REG");
               }
               if ((Value & MI_CLR_DP_INTR) != 0)
               {
                   AndConstToVariable((uint32_t)~MI_INTR_DP, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
                   AndConstToVariable((uint32_t)~MI_INTR_DP, &g_Reg->m_GfxIntrReg, "m_GfxIntrReg");
               }
            }
            break;
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
                   AndConstToVariable(~ModValue, &g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG");
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
                   OrConstToVariable(ModValue, &g_Reg->MI_INTR_MASK_REG, "MI_INTR_MASK_REG");
               }
            }
            break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
        }
        break;
    case 0x04400000:
        if (GenerateLog() && LogVideoInterface())
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(0xFFFFFFFF);
            PushImm32(Value);
            PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, x86_ECX);
            Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][1], "VideoInterfaceHandler::Write32");
#else
            PushImm32((uint32_t)&g_MMU->m_VideoInterfaceHandler);
            Call_Direct(AddressOf(&m_VideoInterfaceHandler::Write32), "m_VideoInterfaceHandler::Write32");
            AddConstToX86Reg(x86_ESP, 16);
#endif
            m_RegWorkingSet.AfterCallDirect();
        }
        else
        {
            switch (PAddr)
            {
            case 0x04400000:
                if (g_Plugins->Gfx()->ViStatusChanged != nullptr)
                {
                    CompConstToVariable(Value, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    MoveConstToVariable(Value, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    m_RegWorkingSet.BeforeCallDirect();
                    Call_Direct((void *)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    CPU_Message("");
                    CPU_Message("      Continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x04400004: MoveConstToVariable((Value & 0xFFFFFF), &g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG"); break;
            case 0x04400008:
                if (g_Plugins->Gfx()->ViWidthChanged != nullptr)
                {
                    CompConstToVariable(Value, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    MoveConstToVariable(Value, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    m_RegWorkingSet.BeforeCallDirect();
                    Call_Direct((void *)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    CPU_Message("");
                    CPU_Message("      Continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x0440000C: MoveConstToVariable(Value, &g_Reg->VI_INTR_REG, "VI_INTR_REG"); break;
            case 0x04400010:
                AndConstToVariable((uint32_t)~MI_INTR_VI, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
                m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
                MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
                Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
                PushImm32((uint32_t)g_Reg);
                Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
                AddConstToX86Reg(x86_ESP, 4);
#endif
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04400014: MoveConstToVariable(Value, &g_Reg->VI_BURST_REG, "VI_BURST_REG"); break;
            case 0x04400018: MoveConstToVariable(Value, &g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG"); break;
            case 0x0440001C: MoveConstToVariable(Value, &g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG"); break;
            case 0x04400020: MoveConstToVariable(Value, &g_Reg->VI_LEAP_REG, "VI_LEAP_REG"); break;
            case 0x04400024: MoveConstToVariable(Value, &g_Reg->VI_H_START_REG, "VI_H_START_REG"); break;
            case 0x04400028: MoveConstToVariable(Value, &g_Reg->VI_V_START_REG, "VI_V_START_REG"); break;
            case 0x0440002C: MoveConstToVariable(Value, &g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG"); break;
            case 0x04400030: MoveConstToVariable(Value, &g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG"); break;
            case 0x04400034: MoveConstToVariable(Value, &g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG"); break;
            default:
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
                }
            }
        }
        break;
    case 0x04500000:
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        m_RegWorkingSet.BeforeCallDirect();
        PushImm32(0xFFFFFFFF);
        PushImm32(Value);
        PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, x86_ECX);
        Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][1], "AudioInterfaceHandler::Write32");
#else
        PushImm32((uint32_t)&g_MMU->m_AudioInterfaceHandler);
        Call_Direct(AddressOf(&AudioInterfaceHandler::Write32), "AudioInterfaceHandler::Write32");
        AddConstToX86Reg(x86_ESP, 16);
#endif
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04600000:
        switch (PAddr)
        {
        case 0x04600000: MoveConstToVariable(Value, &g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG"); break;
        case 0x04600004: MoveConstToVariable(Value, &g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG"); break;
        case 0x04600008:
            MoveConstToVariable(Value, &g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CDMA *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CDMA::PI_DMA_READ), "CDMA::PI_DMA_READ");
#else
            PushImm32((uint32_t)((CDMA *)g_MMU));
            Call_Direct(AddressOf(&CDMA::PI_DMA_READ), "CDMA::PI_DMA_READ");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0460000C:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveConstToVariable(Value, &g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CDMA *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CDMA::PI_DMA_WRITE), "CDMA::PI_DMA_WRITE");
#else
            PushImm32((uint32_t)((CDMA *)g_MMU));
            Call_Direct(AddressOf(&CDMA::PI_DMA_WRITE), "CDMA::PI_DMA_WRITE");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600010:
            if ((Value & PI_CLR_INTR) != 0)
            {
                AndConstToVariable((uint32_t)~MI_INTR_PI, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
                m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
                MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
                Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
                PushImm32((uint32_t)g_Reg);
                Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
                AddConstToX86Reg(x86_ESP, 4);
#endif
                m_RegWorkingSet.AfterCallDirect();
            }
            break;
        case 0x04600014: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG"); break;
        case 0x04600018: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG"); break;
        case 0x0460001C: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG"); break;
        case 0x04600020: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG"); break;
        case 0x04600024: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG"); break;
        case 0x04600028: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG"); break;
        case 0x0460002C: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG"); break;
        case 0x04600030: MoveConstToVariable((Value & 0xFF), &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG"); break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
        }
        break;
    case 0x04700000:
        switch (PAddr)
        {
        case 0x04700000: MoveConstToVariable(Value, &g_Reg->RI_MODE_REG, "RI_MODE_REG"); break;
        case 0x04700004: MoveConstToVariable(Value, &g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG"); break;
        case 0x04700008: MoveConstToVariable(Value, &g_Reg->RI_CURRENT_LOAD_REG, "RI_CURRENT_LOAD_REG"); break;
        case 0x0470000C: MoveConstToVariable(Value, &g_Reg->RI_SELECT_REG, "RI_SELECT_REG"); break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
            }
        }
        break;
    case 0x04800000:
        switch (PAddr)
        {
        case 0x04800000: MoveConstToVariable(Value, &g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG"); break;
        case 0x04800004:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveConstToVariable(Value, &g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CPifRam *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CPifRam::SI_DMA_READ), "CPifRam::SI_DMA_READ");
#else
            PushImm32((uint32_t)((CPifRam *)g_MMU));
            Call_Direct(AddressOf(&CPifRam::SI_DMA_READ), "CPifRam::SI_DMA_READ");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveConstToVariable(Value, &g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CPifRam *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CPifRam::SI_DMA_WRITE), "CPifRam::SI_DMA_WRITE");
#else
            PushImm32((uint32_t)((CPifRam *)g_MMU));
            Call_Direct(AddressOf(&CPifRam::SI_DMA_WRITE), "CPifRam::SI_DMA_WRITE");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800018:
            AndConstToVariable((uint32_t)~MI_INTR_SI, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
            AndConstToVariable((uint32_t)~SI_STATUS_INTERRUPT, &g_Reg->SI_STATUS_REG, "SI_STATUS_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
            Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
            PushImm32((uint32_t)g_Reg);
            Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
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
                Call_Direct(AddressOf(&DiskReset), "DiskReset");
                m_RegWorkingSet.AfterCallDirect();
                break;
            default:
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
                }
            }
            break;
        }
    case 0x1fc00000:
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(Value);
            PushImm32(PAddr | 0xA0000000);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
#else
            PushImm32((uint32_t)g_MMU);
            Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
        }
        break;
    default:
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nTrying to store %08X in %08X?", __FUNCTION__, Value, VAddr).c_str());
        }
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        m_RegWorkingSet.BeforeCallDirect();
        PushImm32(Value);
        PushImm32(PAddr | 0xA0000000);
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)(g_MMU), x86_ECX);
        Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
#else
        PushImm32((uint32_t)(g_MMU));
        Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
        AddConstToX86Reg(x86_ESP, 12);
#endif
        m_RegWorkingSet.AfterCallDirect();
    }
}

void CX86RecompilerOps::SW_Register(x86Reg Reg, uint32_t VAddr)
{
    if (VAddr < 0x80000000 || VAddr >= 0xC0000000)
    {
        m_RegWorkingSet.SetX86Protected(Reg, true);
        x86Reg AddressReg = Map_TempReg(x86_Any, -1, false);
        MoveConstToX86reg(VAddr, AddressReg);
        CompileStoreMemoryValue(AddressReg, Reg, x86_Unknown, 0, 32);
        return;
    }

    char VarName[100];
    uint32_t PAddr;

    if (!m_MMU.VAddrToPAddr(VAddr, PAddr))
    {
        CPU_Message("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nFailed to translate address: %08X", __FUNCTION__, VAddr).c_str());
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
        sprintf(VarName, "RDRAM + %X", PAddr);
        MoveX86regToVariable(Reg, PAddr + g_MMU->Rdram(), VarName);
        break;
    case 0x04000000:
        switch (PAddr)
        {
        case 0x04040000: MoveX86regToVariable(Reg, &g_Reg->SP_MEM_ADDR_REG, "SP_MEM_ADDR_REG"); break;
        case 0x04040004: MoveX86regToVariable(Reg, &g_Reg->SP_DRAM_ADDR_REG, "SP_DRAM_ADDR_REG"); break;
        case 0x04040008:
        case 0x0404000C:
            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(0xFFFFFFFF);
            Push(Reg);
            PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_SPRegistersHandler, x86_ECX);
            Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_SPRegistersHandler)[0][1], "SPRegistersHandler::Write32");
#else
            PushImm32((uint32_t)&g_MMU->m_SPRegistersHandler);
            Call_Direct(AddressOf(&SPRegistersHandler::Write32), "SPRegistersHandler::Write32");
            AddConstToX86Reg(x86_ESP, 16);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04040010:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveX86regToVariable(Reg, &CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue");
            m_RegWorkingSet.BeforeCallDirect();
            Call_Direct((void *)CMipsMemoryVM::ChangeSpStatus, "CMipsMemoryVM::ChangeSpStatus");
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0404001C: MoveConstToVariable(0, &g_Reg->SP_SEMAPHORE_REG, "SP_SEMAPHORE_REG"); break;
        case 0x04080000:
            MoveX86regToVariable(Reg, &g_Reg->SP_PC_REG, "SP_PC_REG");
            AndConstToVariable(0xFFC, &g_Reg->SP_PC_REG, "SP_PC_REG");
            break;
        default:
            if (PAddr < 0x04002000)
            {
                sprintf(VarName, "RDRAM + %X", PAddr);
                MoveX86regToVariable(Reg, PAddr + g_MMU->Rdram(), VarName);
            }
            else
            {
                CPU_Message("    should be moving %s in to %08X ?", x86_Name(Reg), VAddr);
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
                }
            }
        }
        break;
    case 0x04100000:
        if (PAddr == 0x0410000C)
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
        }
        m_RegWorkingSet.BeforeCallDirect();
        Push(Reg);
        PushImm32(PAddr | 0xA0000000);
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)(g_MMU), x86_ECX);
        Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
#else
        PushImm32((uint32_t)(g_MMU));
        Call_Direct(AddressOf(&CMipsMemoryVM::SW_NonMemory), "CMipsMemoryVM::SW_NonMemory");
        AddConstToX86Reg(x86_ESP, 12);
#endif
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04300000:
        switch (PAddr)
        {
        case 0x04300000:
            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(0xFFFFFFFF);
            Push(Reg);
            PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(MemoryHandler*)&g_MMU->m_MIPSInterfaceHandler, x86_ECX);
            Call_Direct((void*)((long**)(MemoryHandler*)&g_MMU->m_MIPSInterfaceHandler)[0][1], "MIPSInterfaceHandler::Write32");
#else
            PushImm32((uint32_t)&g_MMU->m_MIPSInterfaceHandler);
            Call_Direct(AddressOf(&SPRegistersHandler::Write32), "MIPSInterfaceHandler::Write32");
            AddConstToX86Reg(x86_ESP, 16);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0430000C:
            MoveX86regToVariable(Reg, &CMipsMemoryVM::RegModValue, "CMipsMemoryVM::RegModValue");
            m_RegWorkingSet.BeforeCallDirect();
            Call_Direct((void *)CMipsMemoryVM::ChangeMiIntrMask, "CMipsMemoryVM::ChangeMiIntrMask");
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            CPU_Message("    should be moving %s in to %08X ?", x86_Name(Reg), VAddr);
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
        }
        break;
    case 0x04400000:
        if (GenerateLog() && LogVideoInterface())
        {
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

            m_RegWorkingSet.BeforeCallDirect();
            PushImm32(0xFFFFFFFF);
            Push(Reg);
            PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler, x86_ECX);
            Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_VideoInterfaceHandler)[0][1], "VideoInterfaceHandler::Write32");
#else
            PushImm32((uint32_t)&g_MMU->m_VideoInterfaceHandler);
            Call_Direct(AddressOf(&VideoInterfaceHandler::Write32), "SPRegistersHandler::Write32");
            AddConstToX86Reg(x86_ESP, 16);
#endif
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
                    CompX86regToVariable(Reg, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    MoveX86regToVariable(Reg, &g_Reg->VI_STATUS_REG, "VI_STATUS_REG");
                    m_RegWorkingSet.BeforeCallDirect();
                    Call_Direct((void *)g_Plugins->Gfx()->ViStatusChanged, "ViStatusChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    CPU_Message("");
                    CPU_Message("      Continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x04400004:
                MoveX86regToVariable(Reg, &g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG");
                AndConstToVariable(0xFFFFFF, &g_Reg->VI_ORIGIN_REG, "VI_ORIGIN_REG");
                break;
            case 0x04400008:
                if (g_Plugins->Gfx()->ViWidthChanged != nullptr)
                {
                    uint8_t * Jump;
                    CompX86regToVariable(Reg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    JeLabel8("Continue", 0);
                    Jump = *g_RecompPos - 1;
                    MoveX86regToVariable(Reg, &g_Reg->VI_WIDTH_REG, "VI_WIDTH_REG");
                    m_RegWorkingSet.BeforeCallDirect();
                    Call_Direct((void *)g_Plugins->Gfx()->ViWidthChanged, "ViWidthChanged");
                    m_RegWorkingSet.AfterCallDirect();
                    CPU_Message("");
                    CPU_Message("      Continue:");
                    SetJump8(Jump, *g_RecompPos);
                }
                break;
            case 0x0440000C: MoveX86regToVariable(Reg, &g_Reg->VI_INTR_REG, "VI_INTR_REG"); break;
            case 0x04400010:
                AndConstToVariable((uint32_t)~MI_INTR_VI, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
                m_RegWorkingSet.BeforeCallDirect();
    #ifdef _MSC_VER
                MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
                Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
    #else
                PushImm32((uint32_t)g_Reg);
                Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
                AddConstToX86Reg(x86_ESP, 4);
    #endif
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x04400014: MoveX86regToVariable(Reg, &g_Reg->VI_BURST_REG, "VI_BURST_REG"); break;
            case 0x04400018: MoveX86regToVariable(Reg, &g_Reg->VI_V_SYNC_REG, "VI_V_SYNC_REG"); break;
            case 0x0440001C: MoveX86regToVariable(Reg, &g_Reg->VI_H_SYNC_REG, "VI_H_SYNC_REG"); break;
            case 0x04400020: MoveX86regToVariable(Reg, &g_Reg->VI_LEAP_REG, "VI_LEAP_REG"); break;
            case 0x04400024: MoveX86regToVariable(Reg, &g_Reg->VI_H_START_REG, "VI_H_START_REG"); break;
            case 0x04400028: MoveX86regToVariable(Reg, &g_Reg->VI_V_START_REG, "VI_V_START_REG"); break;
            case 0x0440002C: MoveX86regToVariable(Reg, &g_Reg->VI_V_BURST_REG, "VI_V_BURST_REG"); break;
            case 0x04400030: MoveX86regToVariable(Reg, &g_Reg->VI_X_SCALE_REG, "VI_X_SCALE_REG"); break;
            case 0x04400034: MoveX86regToVariable(Reg, &g_Reg->VI_Y_SCALE_REG, "VI_Y_SCALE_REG"); break;
            default:
                CPU_Message("    should be moving %s in to %08X ?", x86_Name(Reg), VAddr);
                if (ShowUnhandledMemory())
                {
                    g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
                }
            }
        }
        break;
    case 0x04500000: // AI registers
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
        UpdateCounters(m_RegWorkingSet, false, true);
        m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());

        m_RegWorkingSet.BeforeCallDirect();
        PushImm32(0xFFFFFFFF);
        Push(Reg);
        PushImm32(PAddr & 0x1FFFFFFF);
#ifdef _MSC_VER
        MoveConstToX86reg((uint32_t)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler, x86_ECX);
        Call_Direct((void *)((long**)(MemoryHandler *)&g_MMU->m_AudioInterfaceHandler)[0][1], "AudioInterfaceHandler::Write32");
#else
        PushImm32((uint32_t)&g_MMU->m_AudioInterfaceHandler);
        Call_Direct(AddressOf(&AudioInterfaceHandler::Write32), "AudioInterfaceHandler::Write32");
        AddConstToX86Reg(x86_ESP, 16);
#endif
        m_RegWorkingSet.AfterCallDirect();
        break;
    case 0x04600000:
        switch (PAddr)
        {
        case 0x04600000: MoveX86regToVariable(Reg, &g_Reg->PI_DRAM_ADDR_REG, "PI_DRAM_ADDR_REG"); break;
        case 0x04600004:
            MoveX86regToVariable(Reg, &g_Reg->PI_CART_ADDR_REG, "PI_CART_ADDR_REG");
            if (EnableDisk())
            {
                m_RegWorkingSet.BeforeCallDirect();
                Call_Direct(AddressOf(&DiskDMACheck), "DiskDMACheck");
                m_RegWorkingSet.AfterCallDirect();
            }
            break;
        case 0x04600008:
            MoveX86regToVariable(Reg, &g_Reg->PI_RD_LEN_REG, "PI_RD_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CDMA *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CDMA::PI_DMA_READ), "CDMA::PI_DMA_READ");
#else
            PushImm32((uint32_t)((CDMA *)g_MMU));
            Call_Direct(AddressOf(&CDMA::PI_DMA_READ), "CDMA::PI_DMA_READ");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x0460000C:
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() - g_System->CountPerOp());
            UpdateCounters(m_RegWorkingSet, false, true);
            m_RegWorkingSet.SetBlockCycleCount(m_RegWorkingSet.GetBlockCycleCount() + g_System->CountPerOp());
            MoveX86regToVariable(Reg, &g_Reg->PI_WR_LEN_REG, "PI_WR_LEN_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CDMA *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CDMA::PI_DMA_WRITE), "CDMA::PI_DMA_WRITE");
#else
            PushImm32((uint32_t)((CDMA *)g_MMU));
            Call_Direct(AddressOf(&CDMA::PI_DMA_WRITE), "CDMA::PI_DMA_WRITE");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600010:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
            AndConstToVariable((uint32_t)~MI_INTR_PI, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
            Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
            PushImm32((uint32_t)g_Reg);
            Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04600014:
            MoveX86regToVariable(Reg, &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_DOMAIN1_REG, "PI_DOMAIN1_REG");
            break;
        case 0x04600018:
            MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_BSD_DOM1_PWD_REG, "PI_BSD_DOM1_PWD_REG");
            break;
        case 0x0460001C:
            MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_BSD_DOM1_PGS_REG, "PI_BSD_DOM1_PGS_REG");
            break;
        case 0x04600020:
            MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_BSD_DOM1_RLS_REG, "PI_BSD_DOM1_RLS_REG");
            break;
        case 0x04600024:
            MoveX86regToVariable(Reg, &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_DOMAIN2_REG, "PI_DOMAIN2_REG");
            break;
        case 0x04600028:
            MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_BSD_DOM2_PWD_REG, "PI_BSD_DOM2_PWD_REG");
            break;
        case 0x0460002C:
            MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_BSD_DOM2_PGS_REG, "PI_BSD_DOM2_PGS_REG");
            break;
        case 0x04600030:
            MoveX86regToVariable(Reg, &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG");
            AndConstToVariable(0xFF, &g_Reg->PI_BSD_DOM2_RLS_REG, "PI_BSD_DOM2_RLS_REG");
            break;
        default:
            CPU_Message("    should be moving %s in to %08X ?", x86_Name(Reg), VAddr);
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
        }
        break;
    case 0x04700000:
        switch (PAddr)
        {
        case 0x04700000: MoveX86regToVariable(Reg, &g_Reg->RI_MODE_REG, "RI_MODE_REG"); break;
        case 0x04700004: MoveX86regToVariable(Reg, &g_Reg->RI_CONFIG_REG, "RI_CONFIG_REG"); break;
        case 0x0470000C: MoveX86regToVariable(Reg, &g_Reg->RI_SELECT_REG, "RI_SELECT_REG"); break;
        case 0x04700010: MoveX86regToVariable(Reg, &g_Reg->RI_REFRESH_REG, "RI_REFRESH_REG"); break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
        }
        break;
    case 0x04800000:
        switch (PAddr)
        {
        case 0x04800000: MoveX86regToVariable(Reg, &g_Reg->SI_DRAM_ADDR_REG, "SI_DRAM_ADDR_REG"); break;
        case 0x04800004:
            MoveX86regToVariable(Reg, &g_Reg->SI_PIF_ADDR_RD64B_REG, "SI_PIF_ADDR_RD64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CPifRam *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CPifRam::SI_DMA_READ), "CPifRam::SI_DMA_READ");
#else
            PushImm32((uint32_t)((CPifRam *)g_MMU));
            Call_Direct(AddressOf(&CPifRam::SI_DMA_READ), "CPifRam::SI_DMA_READ");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800010:
            MoveX86regToVariable(Reg, &g_Reg->SI_PIF_ADDR_WR64B_REG, "SI_PIF_ADDR_WR64B_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)((CPifRam *)g_MMU), x86_ECX);
            Call_Direct(AddressOf(&CPifRam::SI_DMA_WRITE), "CPifRam::SI_DMA_WRITE");
#else
            PushImm32((uint32_t)((CPifRam *)g_MMU));
            Call_Direct(AddressOf(&CPifRam::SI_DMA_WRITE), "CPifRam::SI_DMA_WRITE");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        case 0x04800018:
            AndConstToVariable((uint32_t)~MI_INTR_SI, &g_Reg->MI_INTR_REG, "MI_INTR_REG");
            AndConstToVariable((uint32_t)~SI_STATUS_INTERRUPT, &g_Reg->SI_STATUS_REG, "SI_STATUS_REG");
            m_RegWorkingSet.BeforeCallDirect();
#ifdef _MSC_VER
            MoveConstToX86reg((uint32_t)g_Reg, x86_ECX);
            Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
#else
            PushImm32((uint32_t)g_Reg);
            Call_Direct(AddressOf(&CRegisters::CheckInterrupts), "CRegisters::CheckInterrupts");
            AddConstToX86Reg(x86_ESP, 4);
#endif
            m_RegWorkingSet.AfterCallDirect();
            break;
        default:
            if (ShowUnhandledMemory())
            {
                g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
            }
        }
        break;
    case 0x05000000:
        // 64DD registers
        if (EnableDisk())
        {
            switch (PAddr)
            {
            case 0x05000500: MoveX86regToVariable(Reg, &g_Reg->ASIC_DATA, "ASIC_DATA"); break;
            case 0x05000508:
            {
                // ASIC_CMD
                MoveX86regToVariable(Reg, &g_Reg->ASIC_CMD, "ASIC_CMD");
                m_RegWorkingSet.BeforeCallDirect();
                Call_Direct(AddressOf(&DiskCommand), "DiskCommand");
                m_RegWorkingSet.AfterCallDirect();
                break;
            }
            case 0x05000510:
            {
                // ASIC_BM_CTL
                MoveX86regToVariable(Reg, &g_Reg->ASIC_BM_CTL, "ASIC_BM_CTL");
                m_RegWorkingSet.BeforeCallDirect();
                Call_Direct(AddressOf(&DiskBMControl), "DiskBMControl");
                m_RegWorkingSet.AfterCallDirect();
                break;
            }
            case 0x05000518:
                break;
            case 0x05000520:
                m_RegWorkingSet.BeforeCallDirect();
                Call_Direct(AddressOf(&DiskReset), "DiskReset");
                m_RegWorkingSet.AfterCallDirect();
                break;
            case 0x05000528: MoveX86regToVariable(Reg, &g_Reg->ASIC_HOST_SECBYTE, "ASIC_HOST_SECBYTE"); break;
            case 0x05000530: MoveX86regToVariable(Reg, &g_Reg->ASIC_SEC_BYTE, "ASIC_SEC_BYTE"); break;
            case 0x05000548: MoveX86regToVariable(Reg, &g_Reg->ASIC_TEST_PIN_SEL, "ASIC_TEST_PIN_SEL"); break;
            }
            break;
        }
    case 0x1FC00000:
        sprintf(VarName, "RDRAM + %X", PAddr);
        MoveX86regToVariable(Reg, PAddr + g_MMU->Rdram(), VarName);
        break;
    default:
        CPU_Message("    should be moving %s in to %08X ?", x86_Name(Reg), VAddr);
        if (ShowUnhandledMemory())
        {
            g_Notify->DisplayError(stdstr_f("%s\nTrying to store in %08X?", __FUNCTION__, VAddr).c_str());
        }
    }
}

void CX86RecompilerOps::ResetMemoryStack()
{
    x86Reg Reg, TempReg;

    int32_t MipsReg = 29;
    CPU_Message("    ResetMemoryStack");
    Reg = Get_MemoryStack();
    if (Reg == x86_Unknown)
    {
        Reg = Map_TempReg(x86_Any, MipsReg, false);
    }
    else
    {
        if (IsUnknown(MipsReg))
        {
            MoveVariableToX86reg(&_GPR[MipsReg].UW[0], CRegName::GPR_Lo[MipsReg], Reg);
        }
        else if (IsMapped(MipsReg))
        {
            MoveX86RegToX86Reg(GetMipsRegMapLo(MipsReg), Reg);
        }
        else
        {
            MoveConstToX86reg(GetMipsRegLo(MipsReg), Reg);
        }
    }

    TempReg = Map_TempReg(x86_Any, -1, false);
    MoveX86RegToX86Reg(Reg, TempReg);
    ShiftRightUnsignImmed(TempReg, 12);
    MoveVariableDispToX86Reg(g_MMU->m_MemoryReadMap, "MMU->m_MemoryReadMap", TempReg, TempReg, 4);
    AddX86RegToX86Reg(Reg, TempReg);
    MoveX86regToVariable(Reg, &(g_Recompiler->MemoryStackPos()), "MemoryStack");
}

#endif