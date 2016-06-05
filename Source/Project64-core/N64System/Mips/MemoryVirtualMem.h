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
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include "TranslateVaddr.h"
#include <Project64-core/N64System/Recompiler/RecompilerOps.h>
#include <Project64-core/N64System/Interpreter/InterpreterOps.h>
#include <Project64-core/N64System/Mips/PifRam.h>
#include <Project64-core/N64System/Mips/FlashRam.h>
#include <Project64-core/N64System/Mips/Sram.h>
#include <Project64-core/N64System/Mips/Dma.h>

#ifdef __arm__
#include <sys/ucontext.h>
#endif

#ifndef _WIN32
#include <signal.h>
/* siginfo_t */
#endif

/*
* 64-bit Windows exception recovery facilities will expect to interact with
* the 64-bit registers of the Intel architecture (e.g., rax instead of eax).
*
* Attempting to read the 32-bit subsets seems to be erroneous and forbidden.
* Refer to "MemoryFilter" in `Memory Virtual Mem.cpp`.
*/
#ifdef _WIN64
#define Eax     Rax
#define Ebx     Rbx
#define Ecx     Rcx
#define Edx     Rdx
#define Esp     Rsp
#define Ebp     Rbp
#define Esi     Rsi
#define Edi     Rdi

#define Eip     Rip
#endif

/*
 * To do:  Have address translation functions here?
 * `return` either the translated address or the mask to XOR by?
 *
 * This will help us gradually be able to port Project64 for big-endian CPUs.
 * Currently it is written to assume 32-bit little-endian, like so:
 *
 * 0xAABBCCDD EEFFGGHH --> 0xDDCCBBAA HHGGFFEE
 *   GPR bits[63..0]         b1b2b3b4 b5b6b7b8
 */

class CMipsMemoryVM :
    public CTransVaddr,
    private CRecompilerOps,
    private R4300iOp,
    private CPifRam,
    private CFlashram,
    private CSram,
    private CDMA
{
public:
    CMipsMemoryVM(bool SavesReadOnly);
    ~CMipsMemoryVM();

    static void ReserveMemory();
    static void FreeReservedMemory();

    bool   Initialize();
    void   Reset(bool EraseMemory);

    uint8_t * Rdram();
    uint32_t  RdramSize();
    uint8_t * Dmem();
    uint8_t * Imem();
    uint8_t * PifRam();

    bool  LB_VAddr(uint32_t VAddr, uint8_t & Value);
    bool  LH_VAddr(uint32_t VAddr, uint16_t & Value);
    bool  LW_VAddr(uint32_t VAddr, uint32_t & Value);
    bool  LD_VAddr(uint32_t VAddr, uint64_t & Value);

    bool  LB_PAddr(uint32_t PAddr, uint8_t & Value);
    bool  LH_PAddr(uint32_t PAddr, uint16_t & Value);
    bool  LW_PAddr(uint32_t PAddr, uint32_t & Value);
    bool  LD_PAddr(uint32_t PAddr, uint64_t & Value);

    bool  SB_VAddr(uint32_t VAddr, uint8_t Value);
    bool  SH_VAddr(uint32_t VAddr, uint16_t Value);
    bool  SW_VAddr(uint32_t VAddr, uint32_t Value);
    bool  SD_VAddr(uint32_t VAddr, uint64_t Value);

    bool  SB_PAddr(uint32_t PAddr, uint8_t Value);
    bool  SH_PAddr(uint32_t PAddr, uint16_t Value);
    bool  SW_PAddr(uint32_t PAddr, uint32_t Value);
    bool  SD_PAddr(uint32_t PAddr, uint64_t Value);

    int32_t   MemoryFilter(uint32_t dwExptCode, void * lpExceptionPointer);
    void  UpdateFieldSerration(uint32_t interlaced);
#ifndef _WIN32
    static bool SetupSegvHandler(void);
    static void segv_handler(int signal, siginfo_t *siginfo, void *sigcontext);
#endif

    //Protect the Memory from being written to
    void  ProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr);
    void  UnProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr);

    //Compilation Functions
    void ResetMemoryStack();

    void Compile_LB();
    void Compile_LBU();
    void Compile_LH();
    void Compile_LHU();
    void Compile_LW();
    void Compile_LL();
    void Compile_LWC1();
    void Compile_LWU();
    void Compile_LWL();
    void Compile_LWR();
    void Compile_LD();
    void Compile_LDC1();
    void Compile_LDL();
    void Compile_LDR();
    void Compile_SB();
    void Compile_SH();
    void Compile_SW();
    void Compile_SWL();
    void Compile_SWR();
    void Compile_SD();
    void Compile_SDL();
    void Compile_SDR();
    void Compile_SC();
    void Compile_SWC1();
    void Compile_SDC1();

    void ResetMemoryStack(CRegInfo& RegInfo);
    void Compile_LB(CX86Ops::x86Reg Reg, uint32_t Addr, bool SignExtend);
    void Compile_LH(CX86Ops::x86Reg Reg, uint32_t Addr, bool SignExtend);
    void Compile_LW(CX86Ops::x86Reg Reg, uint32_t Addr);
    void Compile_SB_Const(uint8_t Value, uint32_t Addr);
    void Compile_SB_Register(CX86Ops::x86Reg Reg, uint32_t Addr);
    void Compile_SH_Const(uint16_t Value, uint32_t Addr);
    void Compile_SH_Register(CX86Ops::x86Reg Reg, uint32_t Addr);
    void Compile_SW_Const(uint32_t Value, uint32_t Addr);

    void Compile_SW_Register(CX86Ops::x86Reg Reg, uint32_t Addr);

    //Functions for TLB notification
    void TLB_Mapped(uint32_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly);
    void TLB_Unmaped(uint32_t Vaddr, uint32_t Len);

    // CTransVaddr interface
    bool TranslateVaddr(uint32_t VAddr, uint32_t &PAddr) const;
    bool ValidVaddr(uint32_t VAddr) const;
    bool VAddrToRealAddr(uint32_t VAddr, void * &RealAddress) const;

    // Labels
    const char * LabelName(uint32_t Address) const;

private:
    CMipsMemoryVM();                                // Disable default constructor
    CMipsMemoryVM(const CMipsMemoryVM&);            // Disable copy constructor
    CMipsMemoryVM& operator=(const CMipsMemoryVM&); // Disable assignment

    void Compile_LW(bool ResultSigned, bool bRecordLLbit);
    void Compile_SW(bool bCheckLLbit);

    static void RdramChanged(CMipsMemoryVM * _this);
    static void ChangeSpStatus();
    static void ChangeMiIntrMask();

    bool LB_NonMemory(uint32_t PAddr, uint32_t * Value, bool SignExtend);
    bool LH_NonMemory(uint32_t PAddr, uint32_t * Value, bool SignExtend);
    bool LW_NonMemory(uint32_t PAddr, uint32_t * Value);

    bool SB_NonMemory(uint32_t PAddr, uint8_t Value);
    bool SH_NonMemory(uint32_t PAddr, uint16_t Value);
    bool SW_NonMemory(uint32_t PAddr, uint32_t Value);

    void Compile_StoreInstructClean(x86Reg AddressReg, int32_t Length);

    static void Load32RDRAMRegisters(void);
    static void Load32SPRegisters(void);
    static void Load32DPCommand(void);
    static void Load32MIPSInterface(void);
    static void Load32VideoInterface(void);
    static void Load32AudioInterface(void);
    static void Load32PeripheralInterface(void);
    static void Load32RDRAMInterface(void);
    static void Load32SerialInterface(void);
    static void Load32CartridgeDomain1Address1(void);
    static void Load32CartridgeDomain1Address3(void);
    static void Load32CartridgeDomain2Address1(void);
    static void Load32CartridgeDomain2Address2(void);
    static void Load32PifRam(void);
    static void Load32Rom(void);

    static void Write32RDRAMRegisters(void);
    static void Write32SPRegisters(void);
    static void Write32DPCommandRegisters(void);
    static void Write32MIPSInterface(void);
    static void Write32VideoInterface(void);
    static void Write32AudioInterface(void);
    static void Write32PeripheralInterface(void);
    static void Write32RDRAMInterface(void);
    static void Write32SerialInterface(void);
    static void Write32CartridgeDomain2Address1(void);
    static void Write32CartridgeDomain2Address2(void);
    static void Write32PifRam(void);

#if defined(__i386__) || defined(_M_IX86)

    typedef struct _X86_CONTEXT 
    {
        uint32_t * Edi;
        uint32_t * Esi;
        uint32_t * Ebx;
        uint32_t * Edx;
        uint32_t * Ecx;
        uint32_t * Eax;
        uint32_t * Eip;
        uint32_t * Esp;
        uint32_t * Ebp;
    } X86_CONTEXT;

    static bool FilterX86Exception(uint32_t MemAddress, X86_CONTEXT & context);
#endif
#ifdef __arm__
    static bool FilterArmException(uint32_t MemAddress, mcontext_t & context);
#endif

    //Memory Locations
    static uint8_t   * m_Reserve1, *m_Reserve2;
    uint8_t          * m_RDRAM, *m_DMEM, *m_IMEM;
    uint32_t         m_AllocatedRdramSize;

    //Rom Information
    bool          m_RomMapped;
    uint8_t *     m_Rom;
    uint32_t      m_RomSize;
    bool          m_RomWrittenTo;
    uint32_t      m_RomWroteValue;

    //DDRom Information
    bool          m_DDRomMapped;
    uint8_t *     m_DDRom;
    uint32_t      m_DDRomSize;

    //Current Half line
    void UpdateHalfLine();
    uint32_t         m_HalfLine;
    uint32_t         m_HalfLineCheck;
    uint32_t         m_FieldSerration;
    uint32_t         m_TempValue;

    //Initializing and resetting information about the memory system
    void FreeMemory();

    mutable char m_strLabelName[100];

    //BIG look up table to quickly translate the tlb to real mem address
    size_t * m_TLB_ReadMap;
    size_t * m_TLB_WriteMap;

    static uint32_t m_MemLookupAddress;
    static MIPS_DWORD m_MemLookupValue;
    static bool m_MemLookupValid;
};
