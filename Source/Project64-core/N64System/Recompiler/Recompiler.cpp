#include "stdafx.h"

#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/SystemGlobals.h>

CRecompiler::CRecompiler(CN64System & System, bool & EndEmulation) :
    m_System(System),
    m_MMU(System.m_MMU_VM),
    m_Reg(System.m_Reg),
    m_TLB(System.m_TLB),
    m_EndEmulation(EndEmulation),
    m_MemoryStack(0),
    PROGRAM_COUNTER(System.m_Reg.m_PROGRAM_COUNTER),
    m_LogFile(nullptr)
{
    CFunctionMap::AllocateMemory();
    ResetMemoryStackPos();
}

CRecompiler::~CRecompiler()
{
    ResetRecompCode(false);
    StopLog();
}

void CRecompiler::Run()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start");
    StartLog();

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

    if (m_System.LookUpMode() == FuncFind_VirtualLookup)
    {
        if (m_System.bSMM_ValidFunc())
        {
            RecompilerMain_VirtualTable_validate();
        }
        else
        {
            RecompilerMain_VirtualTable();
        }
    }
    else if (m_System.LookUpMode() == FuncFind_ChangeMemory)
    {
        RecompilerMain_ChangeMemory();
    }
    else
    {
        if (m_System.bSMM_ValidFunc())
        {
            RecompilerMain_Lookup_validate();
        }
        else
        {
            RecompilerMain_Lookup();
        }
    }

    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::RecompilerMain_VirtualTable()
{
    bool & Done = m_EndEmulation;
    uint64_t & PC = PROGRAM_COUNTER;

    while (!Done)
    {
        if (!m_MMU.ValidVaddr((uint32_t)PC))
        {
            m_Reg.TriggerAddressException(PC, EXC_RMISS);
            PC = m_System.m_JumpToLocation;
            m_System.m_PipelineStage = PIPELINE_STAGE_NORMAL;
            if (!m_MMU.ValidVaddr((uint32_t)PC))
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
            if (info != nullptr)
            {
                (info->Function())();
                continue;
            }
        }
        CCompiledFunc * info = CompileCode();
        if (info == nullptr || m_EndEmulation)
        {
            break;
        }

        if (table == nullptr)
        {
            table = new PCCompiledFunc[(0x1000 >> 2)];
            if (table == nullptr)
            {
                WriteTrace(TraceRecompiler, TraceError, "Failed to allocate PCCompiledFunc");
                g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
            }
            memset(table, 0, sizeof(PCCompiledFunc) * (0x1000 >> 2));
        }

        table[TableEntry] = info;
        (info->Function())();
    }
}

void CRecompiler::RecompilerMain_VirtualTable_validate()
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CRecompiler::RecompilerMain_Lookup()
{
    uint32_t PhysicalAddr;

    while (!m_EndEmulation)
    {
        if (!m_MMU.VAddrToPAddr((uint32_t)PROGRAM_COUNTER, PhysicalAddr))
        {
            m_Reg.TriggerAddressException(PROGRAM_COUNTER, EXC_RMISS);
            if (!m_MMU.VAddrToPAddr((uint32_t)PROGRAM_COUNTER, PhysicalAddr))
            {
                g_Notify->DisplayError(stdstr_f("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PROGRAM_COUNTER).c_str());
                m_EndEmulation = true;
            }
            PROGRAM_COUNTER = m_System.m_JumpToLocation;
            m_System.m_PipelineStage = PIPELINE_STAGE_NORMAL;
            continue;
        }
        if (PhysicalAddr < m_System.RdramSize())
        {
            CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];

            if (info == nullptr)
            {
                info = CompileCode();
                if (info == nullptr || m_EndEmulation)
                {
                    break;
                }
                JumpTable()[PhysicalAddr >> 2] = info;
            }
            (info->Function())();
        }
        else
        {
            uint32_t opsExecuted = 0;

            while (m_MMU.VAddrToPAddr((uint32_t)PROGRAM_COUNTER, PhysicalAddr) && PhysicalAddr >= m_System.RdramSize())
            {
                m_System.m_OpCodes.ExecuteOps(m_System.CountPerOp());
                opsExecuted += m_System.CountPerOp();
            }

            if (g_SyncSystem)
            {
                m_System.UpdateSyncCPU(opsExecuted);
                m_System.SyncSystem();
            }
        }
    }
}

void CRecompiler::RecompilerMain_Lookup_validate()
{
    WriteTrace(TraceRecompiler, TraceInfo, "Start");
    bool & Done = m_EndEmulation;
    uint64_t & PC = PROGRAM_COUNTER;
    uint32_t PhysicalAddr;

    while (!Done)
    {
        if (!m_MMU.VAddrToPAddr((uint32_t)PC, PhysicalAddr))
        {
            m_Reg.TriggerAddressException(PC, EXC_RMISS);
            if (!m_MMU.VAddrToPAddr((uint32_t)PC, PhysicalAddr))
            {
                g_Notify->DisplayError(stdstr_f("Failed to translate PC to a PAddr: %X\n\nEmulation stopped", PC).c_str());
                Done = true;
            }
            PROGRAM_COUNTER = m_System.m_JumpToLocation;
            m_System.m_PipelineStage = PIPELINE_STAGE_NORMAL;
            continue;
        }
        if (PhysicalAddr < m_System.RdramSize())
        {
            CCompiledFunc * info = JumpTable()[PhysicalAddr >> 2];

            if (info == nullptr)
            {
                info = CompileCode();
                if (info == nullptr || m_EndEmulation)
                {
                    break;
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
                    if (info != nullptr)
                    {
                        g_Notify->BreakPoint(__FILE__, __LINE__);
                        info = nullptr;
                    }
                    continue;
                }
            }

            if (bRecordExecutionTimes())
            {
                uint64_t PreNonCPUTime = m_System.m_CPU_Usage.NonCPUTime();
                HighResTimeStamp StartTime, EndTime;
                StartTime.SetToNow();
                (info->Function())();
                EndTime.SetToNow();
                uint64_t PostNonCPUTime = m_System.m_CPU_Usage.NonCPUTime();
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
                    FUNCTION_PROFILE_DATA data = {0};
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

            while (m_MMU.VAddrToPAddr((uint32_t)PC, PhysicalAddr) && PhysicalAddr >= m_System.RdramSize())
            {
                m_System.m_OpCodes.ExecuteOps(m_System.CountPerOp());
                opsExecuted += m_System.CountPerOp();
            }

            if (g_SyncSystem)
            {
                m_System.UpdateSyncCPU(opsExecuted);
                m_System.SyncSystem();
            }
        }
    }
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::Reset()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start");
    ResetRecompCode(true);
    ResetMemoryStackPos();
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}

void CRecompiler::ResetRecompCode(bool bAllocate)
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start");
    CRecompMemory::Reset();
    CFunctionMap::Reset(bAllocate);

    for (CCompiledFuncList::iterator iter = m_Functions.begin(); iter != m_Functions.end(); iter++)
    {
        CCompiledFunc * Func = iter->second;
        while (Func != nullptr)
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
}

CCompiledFunc * CRecompiler::CompileCode()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start (PC: %016llX)", PROGRAM_COUNTER);

    uint32_t pAddr = 0;
    if (!m_MMU.VAddrToPAddr((uint32_t)PROGRAM_COUNTER, pAddr))
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to translate %016llX", PROGRAM_COUNTER);
        return nullptr;
    }

    CCompiledFuncList::iterator iter = m_Functions.find((uint32_t)PROGRAM_COUNTER);
    if (iter != m_Functions.end())
    {
        WriteTrace(TraceRecompiler, TraceInfo, "Existing functions for address (Program Counter: %016llX pAddr: %X)", PROGRAM_COUNTER, pAddr);
        for (CCompiledFunc * Func = iter->second; Func != nullptr; Func = Func->Next())
        {
            uint32_t PAddr;
            if (m_MMU.VAddrToPAddr(Func->MinPC(), PAddr))
            {
                MD5Digest Hash;
                MD5(m_MMU.Rdram() + PAddr, (Func->MaxPC() - Func->MinPC()) + 4).get_digest(Hash);
                if (memcmp(Hash.digest, Func->Hash().digest, sizeof(Hash.digest)) == 0)
                {
                    WriteTrace(TraceRecompiler, TraceInfo, "Using existing compiled code (Program Counter: %016llX pAddr: %X)", PROGRAM_COUNTER, pAddr);
                    return Func;
                }
            }
        }
    }

    CheckRecompMem();
    WriteTrace(TraceRecompiler, TraceDebug, "Compile Block-Start: Program Counter: %016llX pAddr: %X", PROGRAM_COUNTER, pAddr);

    CCodeBlock CodeBlock(m_System, (uint32_t)PROGRAM_COUNTER);
    if (!CodeBlock.Compile())
    {
        return nullptr;
    }

    if (bShowRecompMemSize())
    {
        ShowMemUsed();
    }

    uint32_t CodeLen = CodeBlock.Finilize(*RecompPos());
    *RecompPos() += CodeLen;
    LogCodeBlock(CodeBlock);

    if (bSMM_StoreInstruc())
    {
        m_MMU.ClearMemoryWriteMap(CodeBlock.VAddrEnter() & ~0xFFF, 0xFFF);
    }

    CCompiledFunc * Func = new CCompiledFunc(CodeBlock);
    std::pair<CCompiledFuncList::iterator, bool> ret = m_Functions.insert(CCompiledFuncList::value_type(Func->EnterPC(), Func));
    if (ret.second == false)
    {
        Func->SetNext(ret.first->second->Next());
        ret.first->second->SetNext(Func);
    }

#if defined(__aarch64__) || defined(__amd64__) || defined(_M_X64)
    g_Notify->BreakPoint(__FILE__, __LINE__);
#else
    if (g_ModuleLogLevel[TraceRecompiler] >= TraceDebug)
    {
        WriteTrace(TraceRecompiler, TraceDebug, "Info->Function() = %X", Func->Function());
        std::string dumpline;
        uint32_t start_address = (uint32_t)(Func->Function()) & ~1;
        for (uint8_t *ptr = (uint8_t *)start_address, *ptr_end = ((uint8_t *)start_address) + CodeLen; ptr < ptr_end; ptr++)
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
#endif
    WriteTrace(TraceRecompiler, TraceVerbose, "Done");
    return Func;
}

void CRecompiler::ClearRecompCode_Phys(uint32_t Address, int length, REMOVE_REASON Reason)
{
    if (m_System.LookUpMode() == FuncFind_VirtualLookup)
    {
        ClearRecompCode_Virt(Address + 0x80000000, length, Reason);
        ClearRecompCode_Virt(Address + 0xA0000000, length, Reason);

        uint32_t VAddr, Index = 0;
        while (m_TLB.PAddrToVAddr(Address, VAddr, Index))
        {
            WriteTrace(TraceRecompiler, TraceInfo, "ClearRecompCode Vaddr %X  len: %d", VAddr, length);
            ClearRecompCode_Virt(VAddr, length, Reason);
        }
    }
    else if (m_System.LookUpMode() == FuncFind_PhysicalLookup)
    {
        if (Address < m_System.RdramSize())
        {
            int ClearLen = ((length + 3) & ~3);
            if (Address + ClearLen > m_System.RdramSize())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
                ClearLen = m_System.RdramSize() - Address;
            }
            WriteTrace(TraceRecompiler, TraceInfo, "Resetting jump table, Addr: %X  len: %d", Address, ClearLen);
            memset((uint8_t *)JumpTable() + Address, 0, ClearLen);
        }
        else
        {
            WriteTrace(TraceRecompiler, TraceInfo, "Ignoring reset of jump table, Addr: %X  len: %d", Address, ((length + 3) & ~3));
        }
    }
}

void CRecompiler::ClearRecompCode_Virt(uint32_t Address, int length, REMOVE_REASON Reason)
{
    uint32_t AddressIndex, WriteStart;
    int DataInBlock, DataToWrite, DataLeft;

    switch (m_System.LookUpMode())
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
                WriteTrace(TraceRecompiler, TraceError, "Delete table (%X): Index = %d", table, AddressIndex);
                delete table;
                table = nullptr;
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
        if (m_MMU.VAddrToPAddr(Address, pAddr))
        {
            ClearRecompCode_Phys(pAddr, length, Reason);
        }
    }
    break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CRecompiler::ResetLog()
{
    StopLog();
    StartLog();
}

void CRecompiler::ResetMemoryStackPos()
{
    if (m_Reg.m_GPR[29].UW[0] == 0)
    {
        m_MemoryStack = 0;
        return;
    }

    uint32_t pAddr = 0;
    if (m_MMU.VAddrToPAddr(m_Reg.m_GPR[29].UW[0], pAddr))
    {
        if (pAddr <= m_MMU.RdramSize())
        {
            m_MemoryStack = m_MMU.Rdram() + pAddr;
        }
        else if (pAddr > 0x04000000 && pAddr < 0x04001000)
        {
            m_MemoryStack = m_MMU.Dmem() + (pAddr - 0x04000000);
        }
        else if (pAddr > 0x04001000 && pAddr < 0x04002000)
        {
            m_MemoryStack = m_MMU.Imem() + (pAddr - 0x04001000);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    else
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to translate SP address (%s)", m_Reg.m_GPR[29].UW[0]);
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
}

void CRecompiler::DumpFunctionTimes()
{
#if defined(__aarch64__) || defined(__amd64__) || defined(_M_X64)
    g_Notify->BreakPoint(__FILE__, __LINE__);
#else
    CPath LogFileName(g_Settings->LoadStringVal(Directory_Log).c_str(), "FunctionTimes.csv");

    CLog Log;
    Log.Open(LogFileName);

    for (FUNCTION_PROFILE::iterator itr = m_BlockProfile.begin(); itr != m_BlockProfile.end(); itr++)
    {
        Log.LogF("%X,0x%X,%d\r\n", (uint32_t)itr->first, itr->second.Address, (uint32_t)itr->second.TimeTaken);
    }
#endif
}

void CRecompiler::ResetFunctionTimes()
{
    m_BlockProfile.clear();
}

void CRecompiler::StartLog()
{
    if (!bRecordRecompilerAsm())
    {
        return;
    }
    CPath LogFileName(g_Settings->LoadStringVal(Directory_Log).c_str(), "CPUoutput.log");
    if (m_LogFile != nullptr)
    {
        StopLog();
    }
    m_LogFile = new CLog();
    if (m_LogFile)
    {
        if (m_LogFile->Open(LogFileName))
        {
            m_LogFile->SetMaxFileSize(300 * CLog::MB);
        }
        else
        {
            StopLog();
        }
    }
}

void CRecompiler::StopLog(void)
{
    if (m_LogFile != nullptr)
    {
        delete m_LogFile;
        m_LogFile = nullptr;
    }
}

void CRecompiler::LogCodeBlock(const CCodeBlock & CodeBlock)
{
    if (!bRecordRecompilerAsm() || m_LogFile == nullptr || CodeBlock.CodeLog().empty())
    {
        return;
    }
    m_LogFile->Log(CodeBlock.CodeLog().c_str());
    m_LogFile->Log("\r\n");
    m_LogFile->Flush();
}
