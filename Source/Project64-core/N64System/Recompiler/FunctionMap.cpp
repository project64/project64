#include "stdafx.h"

#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/Recompiler/FunctionMap.h>
#include <Project64-core/N64System/SystemGlobals.h>

CFunctionMap::CFunctionMap() :
    m_JumpTable(nullptr),
    m_FunctionTable(nullptr)
{
}

CFunctionMap::~CFunctionMap()
{
    CleanBuffers();
}

bool CFunctionMap::AllocateMemory()
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start");
    if (LookUpMode() == FuncFind_VirtualLookup && m_FunctionTable == nullptr)
    {
        m_FunctionTable = new PCCompiledFunc_TABLE[0x100000];
        if (m_FunctionTable == nullptr)
        {
            WriteTrace(TraceRecompiler, TraceError, "Failed to allocate function table");
            g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
            return false;
        }
        memset(m_FunctionTable, 0, 0x100000 * sizeof(PCCompiledFunc_TABLE));
    }
    if (LookUpMode() == FuncFind_PhysicalLookup && m_JumpTable == nullptr)
    {
        m_JumpTable = new PCCompiledFunc[RdramSize() >> 2];
        if (m_JumpTable == nullptr)
        {
            WriteTrace(TraceRecompiler, TraceError, "Failed to allocate jump table");
            g_Notify->FatalError(MSG_MEM_ALLOC_ERROR);
            return false;
        }
        memset(m_JumpTable, 0, (RdramSize() >> 2) * sizeof(PCCompiledFunc));
    }
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
    return true;
}

void CFunctionMap::CleanBuffers()
{
    if (m_FunctionTable)
    {
        for (int i = 0, n = 0x100000; i < n; i++)
        {
            if (m_FunctionTable[i] != nullptr)
            {
                delete m_FunctionTable[i];
            }
        }
        delete[] m_FunctionTable;
        m_FunctionTable = nullptr;
    }
    if (m_JumpTable)
    {
        delete[] m_JumpTable;
        m_JumpTable = nullptr;
    }
}

void CFunctionMap::Reset(bool bAllocate)
{
    WriteTrace(TraceRecompiler, TraceDebug, "Start (bAllocate: %s)", bAllocate ? "true" : "false");
    CleanBuffers();
    if (bAllocate && (g_System->LookUpMode() == FuncFind_VirtualLookup || g_System->LookUpMode() == FuncFind_PhysicalLookup))
    {
        AllocateMemory();
    }
    WriteTrace(TraceRecompiler, TraceDebug, "Done");
}