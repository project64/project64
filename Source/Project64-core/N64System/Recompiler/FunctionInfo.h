#pragma once
#include <Project64-core/N64System/Recompiler/CodeBlock.h>

class CCompiledFunc
{
public:
    CCompiledFunc(const CCodeBlock & CodeBlock);

    typedef void (*Func)();

    // Get private information
    const uint32_t EnterPC   () const { return m_EnterPC; }
    const uint32_t MinPC     () const { return m_MinPC; }
    const uint32_t MaxPC     () const { return m_MaxPC; }
    const Func     Function  () const { return m_Function; }
    const uint8_t *FunctionEnd() const { return m_FunctionEnd; }
    const MD5Digest&    Hash () const { return m_Hash; }

    CCompiledFunc*    Next () const { return m_Next; }
    void SetNext(CCompiledFunc* Next) { m_Next = Next; }

    uint64_t MemContents(int32_t i) { return m_MemContents[i]; }
    uint64_t* MemLocation(int32_t i) { return m_MemLocation[i]; }

private:
    CCompiledFunc(void);
    CCompiledFunc(const CCompiledFunc&);
    CCompiledFunc& operator=(const CCompiledFunc&);

    // Information
    uint32_t m_EnterPC; // The entry PC
    uint32_t m_MinPC;   // The lowest PC in the function
    uint32_t m_MaxPC;   // The highest PC in the function
    uint8_t * m_FunctionEnd; // Where the code bytes end

    MD5Digest m_Hash;
    // From querying the recompiler, get information about the function
    Func  m_Function;

    CCompiledFunc* m_Next;

    // Validation
    uint64_t m_MemContents[2], * m_MemLocation[2];
};

typedef std::map<uint32_t, CCompiledFunc *> CCompiledFuncList;
