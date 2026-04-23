#pragma once
#include <Project64-core/N64System/Recompiler/FunctionInfo.h>

class CFunctionMap
{
protected:
    typedef CCompiledFunc * PCCompiledFunc;
    typedef PCCompiledFunc * PCCompiledFunc_TABLE;

    CFunctionMap();
    ~CFunctionMap();

    bool AllocateMemory();
    void Reset(bool bAllocate);

public:
    PCCompiledFunc_TABLE * FunctionTable() const
    {
        return m_FunctionTable;
    }
    PCCompiledFunc * JumpTable() const
    {
        return m_JumpTable;
    }

private:
    void CleanBuffers();

    PCCompiledFunc * m_JumpTable;
    PCCompiledFunc_TABLE * m_FunctionTable;
};
