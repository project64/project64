#include "stdafx.h"

#include <Common/MemoryManagement.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/Recompiler/RecompilerMemory.h>
#include <Project64-core/N64System/SystemGlobals.h>

CRecompMemory::CRecompMemory() :
    m_RecompCode(nullptr),
    m_RecompSize(0)
{
    m_RecompPos = nullptr;
}

CRecompMemory::~CRecompMemory()
{
    if (m_RecompCode)
    {
        FreeAddressSpace(m_RecompCode, MaxCompileBufferSize + 4);
        m_RecompCode = nullptr;
    }
    m_RecompPos = nullptr;
}

bool CRecompMemory::AllocateMemory()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start");
    uint8_t * RecompCodeBase = (uint8_t *)AllocateAddressSpace(MaxCompileBufferSize + 4);
    WriteTrace(TraceRecompiler, TraceDebug, "RecompCodeBase = %X", RecompCodeBase);
    if (RecompCodeBase == nullptr)
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to allocate RecompCodeBase");
        g_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
        return false;
    }

    m_RecompCode = (uint8_t *)CommitMemory(RecompCodeBase, InitialCompileBufferSize, MEM_EXECUTE_READWRITE);
    if (m_RecompCode == nullptr)
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to commit initial buffer");
        FreeAddressSpace(RecompCodeBase, MaxCompileBufferSize + 4);
        g_Notify->DisplayError(MSG_MEM_ALLOC_ERROR);
        return false;
    }
    m_RecompSize = InitialCompileBufferSize;
    m_RecompPos = m_RecompCode;
    memset(m_RecompCode, 0, InitialCompileBufferSize);
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
    return true;
}

void CRecompMemory::CheckRecompMem()
{
    uint32_t Size = (uint32_t)((uint8_t *)m_RecompPos - (uint8_t *)m_RecompCode);
    if ((Size + 0x20000) < m_RecompSize)
    {
        return;
    }
    if (m_RecompSize == MaxCompileBufferSize)
    {
        g_Recompiler->ResetRecompCode(true);
        return;
    }
    void * MemAddr = CommitMemory(m_RecompCode + m_RecompSize, IncreaseCompileBufferSize, MEM_EXECUTE_READWRITE);
    if (MemAddr == nullptr)
    {
        WriteTrace(TraceRecompiler, TraceError, "Failed to increase buffer");
        g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
    }
    m_RecompSize += IncreaseCompileBufferSize;
}

void CRecompMemory::Reset()
{
    m_RecompPos = m_RecompCode;
}

void CRecompMemory::ShowMemUsed()
{
    uint32_t Size = (uint32_t)(m_RecompPos - m_RecompCode);
    uint32_t MB = Size / 0x100000;
    Size -= MB * 0x100000;
    uint32_t KB = Size / 1024;
    Size -= KB * 1024;

    uint32_t TotalAvaliable = m_RecompSize / 0x100000;

    g_Notify->DisplayMessage(0, stdstr_f("Memory used: %d mb %-3d kb %-3d bytes     Total Available: %d mb", MB, KB, Size, TotalAvaliable).c_str());
}