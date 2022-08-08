#pragma once

#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Recompiler/FunctionMap.h>
#include <Project64-core/N64System/Recompiler/RecompilerMemory.h>
#include <Project64-core/N64System/Profiling.h>
#include <Project64-core/Settings/RecompilerSettings.h>
#include <Project64-core/Settings/DebugSettings.h>

class CLog;

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
        Remove_MemViewer,
    };

    typedef void(*DelayFunc)();

public:
    CRecompiler(CMipsMemoryVM & MMU, CRegisters & Registers, bool & EndEmulation);
    ~CRecompiler();

    void Run();
    void Reset();
    void ResetRecompCode(bool bAllocate);

    // Self-modifying code methods
    void ClearRecompCode_Virt(uint32_t VirtualAddress, int32_t length, REMOVE_REASON Reason);
    void ClearRecompCode_Phys(uint32_t PhysicalAddress, int32_t length, REMOVE_REASON Reason);

    void ResetLog();
    void ResetMemoryStackPos();
    void ResetFunctionTimes();
    void DumpFunctionTimes();

    uint32_t& MemoryStackPos() { return m_MemoryStack; }

private:
    CRecompiler();
    CRecompiler(const CRecompiler&);
    CRecompiler& operator=(const CRecompiler&);

    CCompiledFunc * CompileCode();

    typedef struct
    {
        uint32_t Address;
        uint64_t TimeTaken;
    } FUNCTION_PROFILE_DATA;

    typedef std::map <CCompiledFunc::Func, FUNCTION_PROFILE_DATA> FUNCTION_PROFILE;

    void RecompilerMain_VirtualTable();
    void RecompilerMain_VirtualTable_validate();
    void RecompilerMain_ChangeMemory();
    void RecompilerMain_Lookup();
    void RecompilerMain_Lookup_validate();

    void StartLog();
    void StopLog();
    void LogCodeBlock(const CCodeBlock & CodeBlock);

    CCompiledFuncList  m_Functions;
    CMipsMemoryVM & m_MMU;
    CRegisters & m_Registers;
    bool & m_EndEmulation;
    uint32_t m_MemoryStack;
    FUNCTION_PROFILE m_BlockProfile;
    uint32_t & PROGRAM_COUNTER;
    CLog * m_LogFile;
};
