#pragma once
#include <Project64-core/N64System/Recompiler/CodeBlock.h>

class CCompiledFunc
{
public:
    CCompiledFunc(const CCodeBlock & CodeBlock);

    typedef void (*Func)();

    const uint32_t EnterPC() const
    {
        return m_EnterPC;
    }
    const uint32_t MinPC() const
    {
        return m_MinPC;
    }
    const uint32_t MaxPC() const
    {
        return m_MaxPC;
    }
    const Func Function() const
    {
        return m_Function;
    }
    const uint8_t * FunctionEnd() const
    {
        return m_FunctionEnd;
    }
    const MD5Digest & Hash() const
    {
        return m_Hash;
    }

    CCompiledFunc * Next() const
    {
        return m_Next;
    }
    void SetNext(CCompiledFunc * Next)
    {
        m_Next = Next;
    }

    uint64_t MemContents(int32_t i)
    {
        return m_MemContents[i];
    }
    uint64_t * MemLocation(int32_t i)
    {
        return m_MemLocation[i];
    }

private:
    CCompiledFunc(void);
    CCompiledFunc(const CCompiledFunc &);
    CCompiledFunc & operator=(const CCompiledFunc &);

    uint32_t m_EnterPC;
    uint32_t m_MinPC;
    uint32_t m_MaxPC;
    uint8_t * m_FunctionEnd;

    MD5Digest m_Hash;
    Func m_Function;

    CCompiledFunc * m_Next;
    uint64_t m_MemContents[2], *m_MemLocation[2];
};

typedef std::map<uint32_t, CCompiledFunc *> CCompiledFuncList;
