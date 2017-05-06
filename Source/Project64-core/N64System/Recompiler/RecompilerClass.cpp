/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Recompiler/RecompilerCodeLog.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/N64System/Interpreter/InterpreterCPU.h>
#include <Project64-core/ExceptionHandler.h>

CRecompiler::CRecompiler(CMipsMemoryVM & MMU, CRegisters & Registers, bool & EndEmulation) :
    m_MMU(MMU),
    m_Registers(Registers),
    m_EndEmulation(EndEmulation),
    m_MemoryStack(0),
    PROGRAM_COUNTER(Registers.m_PROGRAM_COUNTER)
{
    CFunctionMap::AllocateMemory();
    ResetMemoryStackPos();
}

CRecompiler::~CRecompiler()
{
    ResetRecompCode(false);
}

void CRecompiler::Run()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start");

    if (bRecordRecompilerAsm())
    {
        Start_Recompiler_Log();
    }

    if (!CRecompMemory::AllocateMemory())
    {
        WriteTrace(TraceRecompiler, TraceError, "AllocateMemory failed");
        return;
    }
    if (!CFunctionMap::AllocateMemory())
    {
        WriteTrace(TraceRecompiler, TraceError, "AllocateMemory failed");
        return;
    }
    m_EndEmulation = false;

#ifdef legacycode
    *g_MemoryStack = (uint32_t)(RDRAM + (_GPR[29].W[0] & 0x1FFFFFFF));
#endif
    __except_try()
    {
        if (g_System->LookUpMode() == FuncFind_VirtualLookup)
        {
            if (g_System->bSMM_ValidFunc())
            {
                RecompilerMain_VirtualTable_validate();
            }
            else
            {
                RecompilerMain_VirtualTable();
            }
        }
        else if (g_System->LookUpMode() == FuncFind_ChangeMemory)
        {
            RecompilerMain_ChangeMemory();
        }
        else
        {
            if (g_System->bUseTlb())
            {
                if (g_System->bSMM_ValidFunc())
                {
                    RecompilerMain_Lookup_validate_TLB();
                }
                else
                {
                    RecompilerMain_Lookup_TLB();
                }
            }
            else
            {
                if (g_System->bSMM_ValidFunc())
                {
                    RecompilerMain_Lookup_validate();
                }
                else
                {
                    RecompilerMain_Lookup();
                }
            }
        }
    }
    __except_catch()
    {
        g_Notify->DisplayError(MSG_UNKNOWN_MEM_ACTION);
    }

    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::RecompilerMain_VirtualTable()
{
    bool & Done = m_EndEmulation;
    uint32_t & PC = PROGRAM_COUNTER;

    while (!Done)
    {
        if (!m_MMU.ValidVaddr(PC))
        {
            m_Registers.DoTLBReadMiss(false, PC);
            if (!m_MMU.ValidVaddr(PC))
            {
                g_Notify->DisplayError(stdstr_f("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PC).c_str());
                return;
            }
            continue;
        }

        PCCompiledFunc_TABLE & table = FunctionTable()[PC >> 0xC];
        uint32_t TableEntry = (PC & 0xFFF) >> 2;
        if (table)
        {
            CCompiledFunc * info = table[TableEntry];
            if (info != NULL)
            {
                (info->Function())();
                continue;
            }
        }
        CCompiledFunc * info = CompileCode();
        if (info == NULL || m_EndEmulation)
        {
            break;
        }

        if (table == NULL)
        {
            table = new PCCompiledFunc[(0x1000 >> 2)];
            if (table == NULL)
            {
                WriteTrace(TraceRecompiler, TraceError, "failed to allocate PCCompiledFunc");
                g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
            }
            memset(table, 0, sizeof(PCCompiledFunc)* (0x1000 >> 2));
            if (g_System->bSMM_Protect())
            {
                WriteTrace(TraceRecompiler, TraceError, "Create Table (%X): Index = %d", table, PC >> 0xC);
                m_MMU.ProtectMemory(PC & ~0xFFF, PC | 0xFFF);
            }
        }

        table[TableEntry] = info;
        (info->Function())();
    }
}

void CRecompiler::RecompilerMain_VirtualTable_validate()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
    PCCompiledFunc_TABLE * m_FunctionTable = m_Functions.GetFunctionTable();

    while (!m_EndEmulation)
    {
        if (NextInstruction == DELAY_SLOT)
        {
            CCompiledFunc * Info = m_FunctionsDelaySlot.FindFunction(PROGRAM_COUNTER);
            //Find Block on hash table
            if (Info == NULL)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
                if (!g_TLB->ValidVaddr(PROGRAM_COUNTER))
                {
                    DoTLBMiss(NextInstruction == DELAY_SLOT, PROGRAM_COUNTER);
                    NextInstruction = NORMAL;
                    if (!g_TLB->ValidVaddr(PROGRAM_COUNTER))
                    {
                        g_Notify->DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PROGRAM_COUNTER);
                        return;
                    }
                    continue;
                }
#endif
                //Find Block on hash table
                Info = CompileDelaySlot(PROGRAM_COUNTER);

                if (Info == NULL || EndEmulation())
                {
                    break;
                }
            }
            const uint8_t * Block = Info->FunctionAddr();
            if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
                (*Info->MemLocation[1] != Info->MemContents[1]))
            {
                ClearRecompCode_Virt((Info->VStartPC() - 0x1000) & ~0xFFF, 0x2000, Remove_ValidateFunc);
                NextInstruction = DELAY_SLOT;
                Info = NULL;
                continue;
            }
            _asm {
                pushad
                call Block
                popad
            }
            continue;
        }
        PCCompiledFunc_TABLE table = m_FunctionTable[PROGRAM_COUNTER >> 0xC];
        if (table)
        {
            CCompiledFunc * info = table[(PROGRAM_COUNTER & 0xFFF) >> 2];
            if (info != NULL)
            {
                if ((*info->MemLocation[0] != info->MemContents[0]) ||
                    (*info->MemLocation[1] != info->MemContents[1]))
                {
                    ClearRecompCode_Virt((info->VStartPC() - 0x1000) & ~0xFFF, 0x3000, Remove_ValidateFunc);
                    info = NULL;
                    continue;
                }
                const uint8_t * Block = info->FunctionAddr();
                _asm {
                    pushad
                    call Block
                    popad
                }
                continue;
            }
        }
        g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
        if (!g_TLB->ValidVaddr(PROGRAM_COUNTER))
        {
            DoTLBMiss(NextInstruction == DELAY_SLOT, PROGRAM_COUNTER);
            NextInstruction = NORMAL;
            if (!g_TLB->ValidVaddr(PROGRAM_COUNTER))
            {
                g_Notify->DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PROGRAM_COUNTER);
                return;
            }
        }
#endif
        CCompiledFunc * info = CompileCode();

        if (info == NULL || EndEmulation())
        {
            break;
        }
    }

    while (!m_EndEmulation)
    {
        if (!m_MMU.ValidVaddr(PROGRAM_COUNTER))
        {
            DoTLBMiss(NextInstruction == DELAY_SLOT, PROGRAM_COUNTER);
            NextInstruction = NORMAL;
            if (!m_MMU.ValidVaddr(PROGRAM_COUNTER))
            {
                g_Notify->DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PROGRAM_COUNTER);
                return;
            }
        }
        if (NextInstruction == DELAY_SLOT)
        {
            CCompiledFunc * Info = m_FunctionsDelaySlot.FindFunction(PROGRAM_COUNTER);

            //Find Block on hash table
            if (Info == NULL)
            {
                Info = CompileDelaySlot(PROGRAM_COUNTER);

                if (Info == NULL || EndEmulation())
                {
                    break;
                }
            }
            if (bSMM_ValidFunc())
            {
                if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
                    (*Info->MemLocation[1] != Info->MemContents[1]))
                {
                    ClearRecompCode_Virt((Info->StartPC() - 0x1000) & ~0xFFF, 0x2000, Remove_ValidateFunc);
                    NextInstruction = DELAY_SLOT;
                    Info = NULL;
                    continue;
                }
            }
            const uint8_t * Block = Info->FunctionAddr();
            _asm {
                pushad
                call Block
                popad
            }
            continue;
        }

        CCompiledFunc * Info = m_Functions.FindFunction(PROGRAM_COUNTER);

        //Find Block on hash table
        if (Info == NULL)
        {
            Info = CompileCode();

            if (Info == NULL || EndEmulation())
            {
                break;
            }
        }
        if (bSMM_ValidFunc())
        {
            if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
                (*Info->MemLocation[1] != Info->MemContents[1]))
            {
                ClearRecompCode_Virt((Info->StartPC() - 0x1000) & ~0xFFF, 0x3000, Remove_ValidateFunc);
                Info = NULL;
                continue;
            }
        }
        const uint8_t * Block = Info->FunctionAddr();
        _asm {
            pushad
            call Block
            popad
        }
    }
#endif
}

void CRecompiler::RecompilerMain_Lookup()
{
    while (!m_EndEmulation)
    {
        uint32_t PhysicalAddr = PROGRAM_COUNTER & 0x1FFFFFFF;
        if (PhysicalAddr < g_System->RdramSize())
        {
            CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];
            if (info == NULL)
            {
                info = CompileCode();
                if (info == NULL || m_EndEmulation)
                {
                    break;
                }
                if (g_System->bSMM_Protect())
                {
                    m_MMU.ProtectMemory(PROGRAM_COUNTER & ~0xFFF, PROGRAM_COUNTER | 0xFFF);
                }
                JumpTable()[PhysicalAddr >> 2] = info;
            }
            (info->Function())();
        }
        else
        {
            uint32_t opsExecuted = 0;

            while (m_MMU.TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= g_System->RdramSize())
            {
                CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
                opsExecuted += g_System->CountPerOp();
            }

            if (g_SyncSystem)
            {
                g_System->UpdateSyncCPU(g_SyncSystem, opsExecuted);
                g_System->SyncCPU(g_SyncSystem);
            }
        }
    }
    /*
    uint32_t Addr;
    CCompiledFunc * Info;
    //const uint8_t * Block;

    while(!m_EndEmulation)
    {
    if (bUseTlb())
    {
    g_Notify->BreakPoint(__FILE__, __LINE__);
    #ifdef legacycode
    if (!g_TLB->TranslateVaddr(PROGRAM_COUNTER, Addr))
    {
    DoTLBMiss(NextInstruction == DELAY_SLOT,PROGRAM_COUNTER);
    NextInstruction = NORMAL;
    if (!TranslateVaddr(PROGRAM_COUNTER, &Addr)) {
    g_Notify->DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped",PROGRAM_COUNTER);
    return;
    }
    }
    #endif
    } else {
    Addr = PROGRAM_COUNTER & 0x1FFFFFFF;
    }*/

    /*	if (NextInstruction == DELAY_SLOT) {
    CCompiledFunc * Info = m_FunctionsDelaySlot.FindFunction(PROGRAM_COUNTER);

    //Find Block on hash table
    if (Info == NULL)
    {
    Info = CompileDelaySlot(PROGRAM_COUNTER);

    if (Info == NULL || EndEmulation())
    {
    break;
    }
    }
    if (bSMM_ValidFunc())
    {
    if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
    (*Info->MemLocation[1] != Info->MemContents[1]))
    {
    ClearRecompCode_Virt((Info->VStartPC() - 0x1000) & ~0xFFF,0x2000,Remove_ValidateFunc);
    NextInstruction = DELAY_SLOT;
    Info = NULL;
    continue;
    }
    }
    const uint8_t * Block = Info->FunctionAddr();
    _asm {
    pushad
    call Block
    popad
    }
    continue;
    }

    __try {
    if (Addr > 0x10000000)
    {
    if (bRomInMemory())
    {
    if (Addr > 0x20000000)
    {
    WriteTraceF(TraceDebug,"Executing from non mapped space .1 PC: %X Addr: %X",PROGRAM_COUNTER, Addr);
    g_Notify->DisplayError(GS(MSG_NONMAPPED_SPACE));
    break;
    }
    Info = (CCompiledFunc *)*(JumpTable + (Addr >> 2));
    } else {
    if (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
    while (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
    ExecuteInterpreterOpCode();
    }
    continue;
    } else {
    WriteTraceF(TraceDebug,"Executing from non mapped space .1 PC: %X Addr: %X",PROGRAM_COUNTER, Addr);
    g_Notify->DisplayError(GS(MSG_NONMAPPED_SPACE));
    break;
    }
    }
    } else {
    Info = (CCompiledFunc *)*(JumpTable + (Addr >> 2));
    }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    if (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
    while (PROGRAM_COUNTER >= 0xB0000000 && PROGRAM_COUNTER < (RomFileSize | 0xB0000000)) {
    ExecuteInterpreterOpCode();
    }
    continue;
    } else {
    WriteTraceF(TraceDebug,"Executing from non mapped space .2 PC: %X Addr: %X",PROGRAM_COUNTER, Addr);
    g_Notify->DisplayError(GS(MSG_NONMAPPED_SPACE));
    return;
    }
    }

    if (Info == NULL)
    {
    Info = CompileCode();

    if (Info == NULL || EndEmulation())
    {
    break;
    }
    *(JumpTable + (Addr >> 2)) = (void *)Info;

    //			if (SelfModCheck == ModCode_ProtectedMemory) {
    //				VirtualProtect(RDRAM + Addr, 4, PAGE_READONLY, &OldProtect);
    //			}
    }
    if (bSMM_ValidFunc())
    {
    if ((*Info->MemLocation[0] != Info->MemContents[0]) ||
    (*Info->MemLocation[1] != Info->MemContents[1]))
    {
    ClearRecompCode_Virt((Info->VStartPC() - 0x1000) & ~0xFFF,0x3000,Remove_ValidateFunc);
    Info = NULL;
    continue;
    }
    }
    g_Notify->BreakPoint(__FILE__, __LINE__);
    #ifdef legacycode
    if (Profiling && IndvidualBlock) {
    static uint32_t ProfAddress = 0;

    if ((PROGRAM_COUNTER & ~0xFFF) != ProfAddress) {
    char Label[100];

    ProfAddress = PROGRAM_COUNTER & ~0xFFF;
    sprintf(Label,"PC: %X to %X",ProfAddress,ProfAddress+ 0xFFC);
    //						StartTimer(Label);
    }
    if (PROGRAM_COUNTER >= 0x800DD000 && PROGRAM_COUNTER <= 0x800DDFFC) {
    char Label[100];
    sprintf(Label,"PC: %X   Block: %X",PROGRAM_COUNTER,Block);
    StartTimer(Label);
    }
    //				} else 	if ((Profiling || ShowCPUPer) && ProfilingLabel[0] == 0) {
    //					StartTimer("r4300i Running");
    		}
    #endif
    const uint8_t * Block = Info->FunctionAddr();
    _asm {
    pushad
    call Block
    popad
    }
    }*/
}

void CRecompiler::RecompilerMain_Lookup_TLB()
{
    uint32_t PhysicalAddr;

    while (!m_EndEmulation)
    {
        if (!m_MMU.TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr))
        {
            m_Registers.DoTLBReadMiss(false, PROGRAM_COUNTER);
            if (!m_MMU.TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr))
            {
                g_Notify->DisplayError(stdstr_f("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PROGRAM_COUNTER).c_str());
                m_EndEmulation = true;
            }
            continue;
        }
        if (PhysicalAddr < g_System->RdramSize())
        {
            CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];

            if (info == NULL)
            {
                info = CompileCode();
                if (info == NULL || m_EndEmulation)
                {
                    break;
                }
                if (g_System->bSMM_Protect())
                {
                    m_MMU.ProtectMemory(PROGRAM_COUNTER & ~0xFFF, PROGRAM_COUNTER | 0xFFF);
                }
                JumpTable()[PhysicalAddr >> 2] = info;
            }
            (info->Function())();
        }
        else
        {
            uint32_t opsExecuted = 0;

            while (m_MMU.TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= g_System->RdramSize())
            {
                CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
                opsExecuted += g_System->CountPerOp();
            }

            if (g_SyncSystem)
            {
                g_System->UpdateSyncCPU(g_SyncSystem, opsExecuted);
                g_System->SyncCPU(g_SyncSystem);
            }
        }
    }
}

void CRecompiler::RecompilerMain_Lookup_validate()
{
    while (!m_EndEmulation)
    {
        uint32_t PhysicalAddr = PROGRAM_COUNTER & 0x1FFFFFFF;
        if (PhysicalAddr < g_System->RdramSize())
        {
            CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];
            if (info == NULL)
            {
                info = CompileCode();
                if (info == NULL || m_EndEmulation)
                {
                    break;
                }
                if (g_System->bSMM_Protect())
                {
                    m_MMU.ProtectMemory(PROGRAM_COUNTER & ~0xFFF, PROGRAM_COUNTER | 0xFFF);
                }
                JumpTable()[PhysicalAddr >> 2] = info;
            }
            else
            {
                if (*(info->MemLocation(0)) != info->MemContents(0) ||
                    *(info->MemLocation(1)) != info->MemContents(1))
                {
                    ClearRecompCode_Virt((info->EnterPC() - 0x1000) & ~0xFFF, 0x3000, Remove_ValidateFunc);
                    info = NULL;
                    continue;
                }
            }
            (info->Function())();
        }
        else
        {
            uint32_t opsExecuted = 0;

            while (m_MMU.TranslateVaddr(PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= g_System->RdramSize())
            {
                CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
                opsExecuted += g_System->CountPerOp();
            }

            if (g_SyncSystem)
            {
                g_System->UpdateSyncCPU(g_SyncSystem, opsExecuted);
                g_System->SyncCPU(g_SyncSystem);
            }
        }
    }
}

void CRecompiler::RecompilerMain_Lookup_validate_TLB()
{
    WriteTrace(TraceRecompiler, TraceInfo, "Start");
    bool & Done = m_EndEmulation;
    uint32_t & PC = PROGRAM_COUNTER;

    uint32_t PhysicalAddr;

    while (!Done)
    {
        if (!m_MMU.TranslateVaddr(PC, PhysicalAddr))
        {
            m_Registers.DoTLBReadMiss(false, PC);
            if (!m_MMU.TranslateVaddr(PC, PhysicalAddr))
            {
                g_Notify->DisplayError(stdstr_f("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PC).c_str());
                Done = true;
            }
            continue;
        }
        if (PhysicalAddr < g_System->RdramSize())
        {
            CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];

            if (info == NULL)
            {
                info = CompileCode();
                if (info == NULL || m_EndEmulation)
                {
                    break;
                }
                if (g_System->bSMM_Protect())
                {
                    m_MMU.ProtectMemory(PC & ~0xFFF, PC | 0xFFF);
                }
                JumpTable()[PhysicalAddr >> 2] = info;
            }
            else
            {
                if (*(info->MemLocation(0)) != info->MemContents(0) ||
                    *(info->MemLocation(1)) != info->MemContents(1))
                {
                    if (PhysicalAddr > 0x1000)
                    {
                        ClearRecompCode_Phys((PhysicalAddr - 0x1000) & ~0xFFF, 0x3000, Remove_ValidateFunc);
                    }
                    else
                    {
                        ClearRecompCode_Phys(0, 0x2000, Remove_ValidateFunc);
                    }
                    info = JumpTable()[PhysicalAddr >> 2];
                    if (info != NULL)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                        info = NULL;
                    }
                    continue;
                }
            }

            if (bRecordExecutionTimes())
            {
                uint64_t PreNonCPUTime = g_System->m_CPU_Usage.NonCPUTime();
                HighResTimeStamp StartTime, EndTime;
                StartTime.SetToNow();
                (info->Function())();
                EndTime.SetToNow();
                uint64_t PostNonCPUTime = g_System->m_CPU_Usage.NonCPUTime();
                uint64_t TimeTaken = EndTime.GetMicroSeconds() - StartTime.GetMicroSeconds();
                if (PostNonCPUTime >= PreNonCPUTime)
                {
                    TimeTaken -= PostNonCPUTime - PreNonCPUTime;
                }
                else
                {
                    TimeTaken -= PostNonCPUTime;
                }
                FUNCTION_PROFILE::iterator itr = m_BlockProfile.find(info->Function());
                if (itr == m_BlockProfile.end())
                {
                    FUNCTION_PROFILE_DATA data = { 0 };
                    data.Address = info->EnterPC();
                    std::pair<FUNCTION_PROFILE::iterator, bool> res = m_BlockProfile.insert(FUNCTION_PROFILE::value_type(info->Function(), data));
                    itr = res.first;
                }
                WriteTrace(TraceN64System, TraceNotice, "EndTime: %X StartTime: %X TimeTaken: %X", (uint32_t)(EndTime.GetMicroSeconds() & 0xFFFFFFFF), (uint32_t)(StartTime.GetMicroSeconds() & 0xFFFFFFFF), (uint32_t)TimeTaken);
                itr->second.TimeTaken += TimeTaken;
            }
            else
            {
                (info->Function())();
            }
        }
        else
        {
            uint32_t opsExecuted = 0;

            while (m_MMU.TranslateVaddr(PC, PhysicalAddr) && PhysicalAddr >= g_System->RdramSize())
            {
                CInterpreterCPU::ExecuteOps(g_System->CountPerOp());
                opsExecuted += g_System->CountPerOp();
            }

            if (g_SyncSystem)
            {
                g_System->UpdateSyncCPU(g_SyncSystem, opsExecuted);
                g_System->SyncCPU(g_SyncSystem);
            }
        }
    }
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::Reset()
{
    WriteTrace(TraceRecompiler, TraceDebug, "start");
    ResetRecompCode(true);
    ResetMemoryStackPos();
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::ResetRecompCode(bool bAllocate)
{
    WriteTrace(TraceRecompiler, TraceDebug, "start");
    CRecompMemory::Reset();
    CFunctionMap::Reset(bAllocate);

    for (CCompiledFuncList::iterator iter = m_Functions.begin(); iter != m_Functions.end(); iter++)
    {
        CCompiledFunc * Func = iter->second;
        while (Func != NULL)
        {
            CCompiledFunc * CurrentFunc = Func;
            Func = Func->Next();

            delete CurrentFunc;
        }
    }
    m_Functions.clear();
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::RecompilerMain_ChangeMemory()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
#ifdef legacycode
    uint32_t Value, Addr;
    uint8_t * Block;

    while (!EndEmulation())
    {
        if (UseTlb)
        {
            if (!TranslateVaddr(PROGRAM_COUNTER, &Addr))
            {
                DoTLBMiss(NextInstruction == DELAY_SLOT, PROGRAM_COUNTER);
                NextInstruction = NORMAL;
                if (!TranslateVaddr(PROGRAM_COUNTER, &Addr))
                {
                    g_Notify->DisplayError("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PROGRAM_COUNTER);
                    ExitThread(0);
                }
            }
        }
        else
        {
            Addr = PROGRAM_COUNTER & 0x1FFFFFFF;
        }

        if (NextInstruction == DELAY_SLOT)
        {
            __try
            {
                Value = (uint32_t)(*(DelaySlotTable + (Addr >> 12)));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                g_Notify->DisplayError("Executing Delay Slot from non maped space\nPROGRAM_COUNTER = 0x%X", PROGRAM_COUNTER);
                ExitThread(0);
            }
            if ((Value >> 16) == 0x7C7C)
            {
                uint32_t Index = (Value & 0xFFFF);
                Block = (uint8_t *)OrigMem[Index].CompiledLocation;
                if (OrigMem[Index].PAddr != Addr) { Block = NULL; }
                if (OrigMem[Index].VAddr != PROGRAM_COUNTER) { Block = NULL; }
                if (Index >= TargetIndex) { Block = NULL; }
            }
            else
            {
                Block = NULL;
            }
            if (Block == NULL)
            {
                uint32_t MemValue;

                Block = CompileDelaySlot();
                Value = 0x7C7C0000;
                Value += (uint16_t)(TargetIndex);
                MemValue = *(uint32_t *)(RDRAM + Addr);
                if ((MemValue >> 16) == 0x7C7C)
                {
                    MemValue = OrigMem[(MemValue & 0xFFFF)].OriginalValue;
                }
                OrigMem[(uint16_t)(TargetIndex)].OriginalValue = MemValue;
                OrigMem[(uint16_t)(TargetIndex)].CompiledLocation = Block;
                OrigMem[(uint16_t)(TargetIndex)].PAddr = Addr;
                OrigMem[(uint16_t)(TargetIndex)].VAddr = PROGRAM_COUNTER;
                TargetIndex += 1;
                *(DelaySlotTable + (Addr >> 12)) = (void *)Value;
                NextInstruction = NORMAL;
            }
            _asm
            {
                pushad
                    call Block
                    popad
            }
            continue;
        }

        __try
        {
            Value = *(uint32_t *)(RDRAM + Addr);
            if ((Value >> 16) == 0x7C7C)
            {
                uint32_t Index = (Value & 0xFFFF);
                Block = (uint8_t *)OrigMem[Index].CompiledLocation;
                if (OrigMem[Index].PAddr != Addr) { Block = NULL; }
                if (OrigMem[Index].VAddr != PROGRAM_COUNTER) { Block = NULL; }
                if (Index >= TargetIndex) { Block = NULL; }
            }
            else
            {
                Block = NULL;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_Notify->DisplayError(GS(MSG_NONMAPPED_SPACE));
            ExitThread(0);
        }

        if (Block == NULL)
        {
            uint32_t MemValue;

            __try
            {
                Block = Compiler4300iBlock();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ResetRecompCode();
                Block = Compiler4300iBlock();
            }
            if (EndEmulation())
            {
                continue;
            }
            if (TargetIndex == MaxOrigMem)
            {
                ResetRecompCode();
                continue;
            }
            Value = 0x7C7C0000;
            Value += (uint16_t)(TargetIndex);
            MemValue = *(uint32_t *)(RDRAM + Addr);
            if ((MemValue >> 16) == 0x7C7C)
            {
                MemValue = OrigMem[(MemValue & 0xFFFF)].OriginalValue;
            }
            OrigMem[(uint16_t)(TargetIndex)].OriginalValue = MemValue;
            OrigMem[(uint16_t)(TargetIndex)].CompiledLocation = Block;
            OrigMem[(uint16_t)(TargetIndex)].PAddr = Addr;
            OrigMem[(uint16_t)(TargetIndex)].VAddr = PROGRAM_COUNTER;
            TargetIndex += 1;
            *(uint32_t *)(RDRAM + Addr) = Value;
            NextInstruction = NORMAL;
        }
        if (Profiling && IndvidualBlock)
        {
            static uint32_t ProfAddress = 0;

            /*if ((PROGRAM_COUNTER & ~0xFFF) != ProfAddress)
            {
            char Label[100];

            ProfAddress = PROGRAM_COUNTER & ~0xFFF;
            sprintf(Label,"PC: %X to %X",ProfAddress,ProfAddress+ 0xFFC);
            StartTimer(Label);
            }*/
            /*if (PROGRAM_COUNTER >= 0x800DD000 && PROGRAM_COUNTER <= 0x800DDFFC)
            {
            char Label[100];
            sprintf(Label,"PC: %X   Block: %X",PROGRAM_COUNTER,Block);
            StartTimer(Label);
            }*/
            //				} else 	if ((Profiling || ShowCPUPer) && ProfilingLabel[0] == 0) {
            //					StartTimer("r4300i Running");
        }
        _asm
        {
            pushad
                call Block
                popad
        }
    } // end for(;;)
#endif
}

CCompiledFunc * CRecompiler::CompileCode()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start (PC: %X)", PROGRAM_COUNTER);

    uint32_t pAddr = 0;
    if (!m_MMU.TranslateVaddr(PROGRAM_COUNTER, pAddr))
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to translate %X", PROGRAM_COUNTER);
        return NULL;
    }

    CCompiledFuncList::iterator iter = m_Functions.find(PROGRAM_COUNTER);
    if (iter != m_Functions.end())
    {
        WriteTrace(TraceRecompiler, TraceInfo, "exisiting functions for address (Program Counter: %X pAddr: %X)", PROGRAM_COUNTER, pAddr);
        for (CCompiledFunc * Func = iter->second; Func != NULL; Func = Func->Next())
        {
            uint32_t PAddr;
            if (m_MMU.TranslateVaddr(Func->MinPC(), PAddr))
            {
                MD5Digest Hash;
                MD5(m_MMU.Rdram() + PAddr, (Func->MaxPC() - Func->MinPC()) + 4).get_digest(Hash);
                if (memcmp(Hash.digest, Func->Hash().digest, sizeof(Hash.digest)) == 0)
                {
                    WriteTrace(TraceRecompiler, TraceInfo, "Using extisting compiled code (Program Counter: %X pAddr: %X)", PROGRAM_COUNTER, pAddr);
                    return Func;
                }
            }
        }
    }

    CheckRecompMem();

    //uint32_t StartTime = timeGetTime();
    WriteTrace(TraceRecompiler, TraceDebug, "Compile Block-Start: Program Counter: %X pAddr: %X", PROGRAM_COUNTER, pAddr);

    CCodeBlock CodeBlock(PROGRAM_COUNTER, *g_RecompPos);
    if (!CodeBlock.Compile())
    {
        return NULL;
    }

    if (bShowRecompMemSize())
    {
        ShowMemUsed();
    }

    CCompiledFunc * Func = new CCompiledFunc(CodeBlock);
    std::pair<CCompiledFuncList::iterator, bool> ret = m_Functions.insert(CCompiledFuncList::value_type(Func->EnterPC(), Func));
    if (ret.second == false)
    {
        Func->SetNext(ret.first->second->Next());
        ret.first->second->SetNext(Func);
    }

    if (g_ModuleLogLevel[TraceRecompiler] >= TraceDebug)
    {
        WriteTrace(TraceRecompiler, TraceDebug, "info->Function() = %X", Func->Function());
        std::string dumpline;
        uint32_t start_address = (uint32_t)(Func->Function()) & ~1;
        for (uint8_t * ptr = (uint8_t *)start_address; ptr < CodeBlock.CompiledLocationEnd(); ptr++)
        {
            if (dumpline.empty())
            {
                dumpline += stdstr_f("%X: ", ptr);
            }
            dumpline += stdstr_f(" %02X", *ptr);
            if ((((uint32_t)ptr - start_address) + 1) % 30 == 0)
            {
                WriteTrace(TraceRecompiler, TraceDebug, "%s", dumpline.c_str());
                dumpline.clear();
            }
        }

        if (!dumpline.empty())
        {
            WriteTrace(TraceRecompiler, TraceDebug, "%s", dumpline.c_str());
        }
    }
    WriteTrace(TraceRecompiler, TraceVerbose, "Done");
    return Func;
}

void CRecompiler::ClearRecompCode_Phys(uint32_t Address, int length, REMOVE_REASON Reason)
{
    if (g_System->LookUpMode() == FuncFind_VirtualLookup)
    {
        ClearRecompCode_Virt(Address + 0x80000000, length, Reason);
        ClearRecompCode_Virt(Address + 0xA0000000, length, Reason);

        if (g_System->bUseTlb())
        {
            uint32_t VAddr, Index = 0;
            while (g_TLB->PAddrToVAddr(Address, VAddr, Index))
            {
                WriteTrace(TraceRecompiler, TraceInfo, "ClearRecompCode Vaddr %X  len: %d", VAddr, length);
                ClearRecompCode_Virt(VAddr, length, Reason);
            }
        }
    }
    else if (g_System->LookUpMode() == FuncFind_PhysicalLookup)
    {
        if (Address < g_System->RdramSize())
        {
            int ClearLen = ((length + 3) & ~3);
            if (Address + ClearLen > g_System->RdramSize())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                ClearLen = g_System->RdramSize() - Address;
            }
            WriteTrace(TraceRecompiler, TraceInfo, "Reseting Jump Table, Addr: %X  len: %d", Address, ClearLen);
            memset((uint8_t *)JumpTable() + Address, 0, ClearLen);
            if (g_System->bSMM_Protect())
            {
                m_MMU.UnProtectMemory(Address + 0x80000000, Address + 0x80000004);
            }
        }
        else
        {
            WriteTrace(TraceRecompiler, TraceInfo, "Ignoring reset of Jump Table, Addr: %X  len: %d", Address, ((length + 3) & ~3));
        }
    }
}

void CRecompiler::ClearRecompCode_Virt(uint32_t Address, int length, REMOVE_REASON Reason)
{
    uint32_t AddressIndex, WriteStart;
    int DataInBlock, DataToWrite, DataLeft;

    switch (g_System->LookUpMode())
    {
    case FuncFind_VirtualLookup:
        AddressIndex = Address >> 0xC;
        WriteStart = (Address & 0xFFC);
        length = ((length + 3) & ~0x3);

        DataInBlock = 0x1000 - WriteStart;
        DataToWrite = length < DataInBlock ? length : DataInBlock;
        DataLeft = length - DataToWrite;

        {
            PCCompiledFunc_TABLE & table = FunctionTable()[AddressIndex];
            if (table)
            {
                WriteTrace(TraceRecompiler, TraceError, "Delete Table (%X): Index = %d", table, AddressIndex);
                delete table;
                table = NULL;
                m_MMU.UnProtectMemory(Address, Address + length);
            }

            if (DataLeft > 0)
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
        break;
    case FuncFind_PhysicalLookup:
        {
            uint32_t pAddr = 0;
            if (m_MMU.TranslateVaddr(Address, pAddr))
            {
                ClearRecompCode_Phys(pAddr, length, Reason);
            }
        }
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CRecompiler::ResetMemoryStackPos()
{
    if (m_Registers.m_GPR[29].UW[0] == 0)
    {
        m_MemoryStack = 0;
        return;
    }

    uint32_t pAddr = 0;
    if (m_MMU.TranslateVaddr(m_Registers.m_GPR[29].UW[0], pAddr))
    {
        m_MemoryStack = (uint32_t)(m_MMU.Rdram() + pAddr);
    }
    else
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to translate SP address (%s)", m_Registers.m_GPR[29].UW[0]);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CRecompiler::DumpFunctionTimes()
{
    CPath LogFileName(g_Settings->LoadStringVal(Directory_Log).c_str(), "FunctionTimes.csv");

    CLog Log;
    Log.Open(LogFileName);

    for (FUNCTION_PROFILE::iterator itr = m_BlockProfile.begin(); itr != m_BlockProfile.end(); itr++)
    {
        Log.LogF("%X,0x%X,%d\r\n", (uint32_t)itr->first, itr->second.Address, (uint32_t)itr->second.TimeTaken);
    }
}

void CRecompiler::ResetFunctionTimes()
{
    m_BlockProfile.clear();
}