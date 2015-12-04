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
#pragma once

class CCompiledFunc
{
public:
    CCompiledFunc(const CCodeBlock & CodeBlock);

    typedef void (*Func)();

    //Get Private Information
    const uint32_t EnterPC   () const { return m_EnterPC; }
    const uint32_t MinPC     () const { return m_MinPC; }
    const uint32_t MaxPC     () const { return m_MaxPC; }
    const Func     Function  () const { return m_Function; }
    const MD5Digest&    Hash () const { return m_Hash; }

    CCompiledFunc*    Next () const { return m_Next; }
    void SetNext(CCompiledFunc* Next) { m_Next = Next; }

    uint64_t MemContents(int32_t i) { return m_MemContents[i]; }
    uint64_t* MemLocation(int32_t i) { return m_MemLocation[i]; }

private:
    CCompiledFunc(void);                              // Disable default constructor
    CCompiledFunc(const CCompiledFunc&);              // Disable copy constructor
    CCompiledFunc& operator=(const CCompiledFunc&);   // Disable assignment

    //Information
    uint32_t m_EnterPC; // The Entry PC
    uint32_t m_MinPC;   // The Lowest PC in the function
    uint32_t m_MaxPC;   // The Highest PC in the function

    MD5Digest m_Hash;
    //From querying the recompiler get information about the function
    Func  m_Function;

    CCompiledFunc* m_Next;

    //Validation
    uint64_t m_MemContents[2], * m_MemLocation[2];
};

typedef std::map<uint32_t, CCompiledFunc *> CCompiledFuncList;
