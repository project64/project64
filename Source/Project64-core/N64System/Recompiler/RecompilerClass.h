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

#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/FunctionMapClass.h>
#include <Project64-core/N64System/Recompiler/RecompilerMemory.h>
#include <Project64-core/N64System/ProfilingClass.h>
#include <Project64-core/Settings/RecompilerSettings.h>
#include <Project64-core/Settings/DebugSettings.h>

class CRecompiler :
    protected CDebugSettings,
    public CRecompilerSettings,
    public CFunctionMap,
    public CRecompMemory,
    private CSystemRegisters
{
public:

    enum REMOVE_REASON
    {
        Remove_InitialCode,
        Remove_Cache,
        Remove_ProtectedMem,
        Remove_ValidateFunc,
        Remove_TLB,
        Remove_DMA,
        Remove_StoreInstruc,
        Remove_Cheats,
    };

    typedef void(*DelayFunc)();

public:
    CRecompiler(CMipsMemoryVM & MMU, CRegisters & Registers, bool & EndEmulation);
    ~CRecompiler();

    void Run();
    void Reset();
    void ResetRecompCode(bool bAllocate);

    //Self modifying code methods
    void ClearRecompCode_Virt(uint32_t VirtualAddress, int32_t length, REMOVE_REASON Reason);
    void ClearRecompCode_Phys(uint32_t PhysicalAddress, int32_t length, REMOVE_REASON Reason);

    void ResetMemoryStackPos();
    void ResetFunctionTimes();
    void DumpFunctionTimes();

    uint32_t& MemoryStackPos() { return m_MemoryStack; }

private:
    CRecompiler();                              // Disable default constructor
    CRecompiler(const CRecompiler&);            // Disable copy constructor
    CRecompiler& operator=(const CRecompiler&); // Disable assignment

    CCompiledFunc * CompileCode();

    typedef struct
    {
        uint32_t Address;
        uint64_t TimeTaken;
    } FUNCTION_PROFILE_DATA;

    typedef std::map <CCompiledFunc::Func, FUNCTION_PROFILE_DATA> FUNCTION_PROFILE;

    // Main loops for the different look up methods
    void RecompilerMain_VirtualTable();
    void RecompilerMain_VirtualTable_validate();
    void RecompilerMain_ChangeMemory();
    void RecompilerMain_Lookup();
    void RecompilerMain_Lookup_TLB();
    void RecompilerMain_Lookup_validate();
    void RecompilerMain_Lookup_validate_TLB();

    CCompiledFuncList  m_Functions;
    CMipsMemoryVM    & m_MMU;
    CRegisters       & m_Registers;
    bool             & m_EndEmulation;
    uint32_t           m_MemoryStack;
    FUNCTION_PROFILE m_BlockProfile;

    //Quick access to registers
    uint32_t            & PROGRAM_COUNTER;
};
