#pragma once
#include <Project64-core\N64System\Interpreter\InterpreterOps.h>
#include <Project64-core\N64System\MemoryHandler\AudioInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\CartridgeDomain1Address1Handler.h>
#include <Project64-core\N64System\MemoryHandler\CartridgeDomain1Address3Handler.h>
#include <Project64-core\N64System\MemoryHandler\CartridgeDomain2Address1Handler.h>
#include <Project64-core\N64System\MemoryHandler\CartridgeDomain2Address2Handler.h>
#include <Project64-core\N64System\MemoryHandler\DisplayControlRegHandler.h>
#include <Project64-core\N64System\MemoryHandler\ISViewerHandler.h>
#include <Project64-core\N64System\MemoryHandler\MIPSInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\PeripheralInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\PifRamHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\RomMemoryHandler.h>
#include <Project64-core\N64System\MemoryHandler\SPRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\SerialInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\VideoInterfaceHandler.h>
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Recompiler\RecompilerOps.h>
#include <Project64-core\N64System\SaveType\FlashRam.h>
#include <Project64-core\Settings\GameSettings.h>

#ifdef __arm__
#include <sys/ucontext.h>
#endif

#ifndef _WIN32
#include <signal.h>
// siginfo_t
#endif

/*
* TODO:  Have address translation functions here?
* `return` either the translated address or the mask to XOR by?
*
* This will help us gradually be able to port Project64 to big-endian CPUs.
* Currently it is written to assume 32-bit little-endian, like so:
*
* 0xAABBCCDD EEFFGGHH --> 0xDDCCBBAA HHGGFFEE
*   GPR bits[63..0]         b1b2b3b4 b5b6b7b8
*/

#if defined(__i386__) || defined(_M_IX86)
class CX86RecompilerOps;
#elif defined(__arm__) || defined(_M_ARM)
class CArmRecompilerOps;
#endif

class CMipsMemoryVM :
    private CGameSettings,
    private CDebugSettings
{
public:
    CMipsMemoryVM(CN64System & System, bool SavesReadOnly);
    ~CMipsMemoryVM();

    static void ReserveMemory();
    static void FreeReservedMemory();

    bool Initialize(bool SyncSystem);
    void Reset(bool EraseMemory);

    uint8_t *& Rdram()
    {
        return m_RDRAM;
    }
    const uint32_t & RdramSize() const
    {
        return m_AllocatedRdramSize;
    }
    uint8_t * Dmem()
    {
        return m_SPRegistersHandler.Dmem();
    }
    uint8_t * Imem()
    {
        return m_SPRegistersHandler.Imem();
    }

    CSram & GetSram()
    {
        return m_CartridgeDomain2Address2Handler.Sram();
    }
    CFlashRam & GetFlashRam()
    {
        return m_CartridgeDomain2Address2Handler.FlashRam();
    }

    uint8_t * MemoryPtr(uint32_t VAddr, uint32_t Size, bool Read);

    bool MemoryValue8(uint32_t VAddr, uint8_t & Value);
    bool MemoryValue16(uint32_t VAddr, uint16_t & Value);
    bool MemoryValue32(uint32_t VAddr, uint32_t & Value);
    bool MemoryValue64(uint32_t VAddr, uint64_t & Value);

    bool UpdateMemoryValue8(uint32_t VAddr, uint8_t Value);
    bool UpdateMemoryValue16(uint32_t VAddr, uint16_t Value);
    bool UpdateMemoryValue32(uint32_t VAddr, uint32_t Value);

    bool LB_Memory(uint64_t VAddr, uint8_t & Value);
    bool LH_Memory(uint64_t VAddr, uint16_t & Value);
    bool LW_Memory(uint64_t VAddr, uint32_t & Value);
    bool LD_Memory(uint64_t VAddr, uint64_t & Value);

    bool SB_Memory(uint64_t VAddr, uint32_t Value);
    bool SH_Memory(uint64_t VAddr, uint32_t Value);
    bool SW_Memory(uint64_t VAddr, uint32_t Value);
    bool SD_Memory(uint64_t VAddr, uint64_t Value);

    int32_t MemoryFilter(uint32_t dwExptCode, void * lpExceptionPointer);

#ifndef _WIN32
    static bool SetupSegvHandler(void);
    static void segv_handler(int signal, siginfo_t * siginfo, void * sigcontext);
#endif

    void ClearMemoryWriteMap(uint32_t VAddr, uint32_t Length);

    // Protect the memory from being written to
    void ProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr);
    void UnProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr);

    // Functions for TLB notification
    void TLB_Mapped(uint64_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly);
    void TLB_Unmaped(uint64_t Vaddr, uint32_t Len);

    bool ValidVaddr(uint32_t VAddr) const;
    bool VAddrToPAddr(uint32_t VAddr, uint32_t & PAddr) const;

    // Labels
    const char * LabelName(uint32_t Address) const;

    AudioInterfaceHandler & AudioInterface(void)
    {
        return m_AudioInterfaceHandler;
    }
    VideoInterfaceHandler & VideoInterface(void)
    {
        return m_VideoInterfaceHandler;
    }
    RomMemoryHandler & RomMemory(void)
    {
        return m_RomMemoryHandler;
    };
    PifRamHandler & PifRam(void)
    {
        return m_PifRamHandler;
    };

private:
    CMipsMemoryVM();
    CMipsMemoryVM(const CMipsMemoryVM &);
    CMipsMemoryVM & operator=(const CMipsMemoryVM &);

#if defined(__i386__) || defined(_M_IX86)
    friend class CX86RecompilerOps;
#elif defined(__arm__) || defined(_M_ARM)
    friend class CArmRegInfo;
    friend class CArmRecompilerOps;
#endif

    static void RdramChanged(CMipsMemoryVM * _this);
    static void ChangeMiIntrMask();

    bool MemoryBreakpoint();

    bool LB_VAddr32(uint32_t VAddr, uint8_t & Value);
    bool LH_VAddr32(uint32_t VAddr, uint16_t & Value);
    bool LW_VAddr32(uint32_t VAddr, uint32_t & Value);
    bool LD_VAddr32(uint32_t VAddr, uint64_t & Value);

    bool LB_PhysicalAddress(uint32_t PAddr, uint8_t & Value);
    bool LH_PhysicalAddress(uint32_t PAddr, uint16_t & Value);
    bool LW_PhysicalAddress(uint32_t PAddr, uint32_t & Value);
    bool LD_PhysicalAddress(uint32_t PAddr, uint64_t & Value);

    bool SB_VAddr32(uint32_t VAddr, uint32_t Value);
    bool SH_VAddr32(uint32_t VAddr, uint32_t Value);
    bool SW_VAddr32(uint32_t VAddr, uint32_t Value);
    bool SD_VAddr32(uint32_t VAddr, uint64_t Value);

    bool SB_PhysicalAddress(uint32_t PAddr, uint32_t Value);
    bool SH_PhysicalAddress(uint32_t PAddr, uint32_t Value);
    bool SW_PhysicalAddress(uint32_t PAddr, uint32_t Value);
    bool SD_PhysicalAddress(uint32_t PAddr, uint64_t Value);

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
    static void DumpArmExceptionInfo(uint32_t MemAddress, mcontext_t & context);
    static bool FilterArmException(uint32_t MemAddress, mcontext_t & context);
#endif
    void FreeMemory();

    static uint8_t *m_Reserve1, *m_Reserve2;
    CN64System & m_System;
    CRegisters & m_Reg;
    AudioInterfaceHandler m_AudioInterfaceHandler;
    CartridgeDomain1Address1Handler m_CartridgeDomain1Address1Handler;
    CartridgeDomain1Address3Handler m_CartridgeDomain1Address3Handler;
    CartridgeDomain2Address1Handler m_CartridgeDomain2Address1Handler;
    CartridgeDomain2Address2Handler m_CartridgeDomain2Address2Handler;
    DisplayControlRegHandler m_DPCommandRegistersHandler;
    ISViewerHandler m_ISViewerHandler;
    MIPSInterfaceHandler m_MIPSInterfaceHandler;
    PeripheralInterfaceHandler m_PeripheralInterfaceHandler;
    PifRamHandler m_PifRamHandler;
    RomMemoryHandler m_RomMemoryHandler;
    RDRAMInterfaceHandler m_RDRAMInterfaceHandler;
    RDRAMRegistersHandler m_RDRAMRegistersHandler;
    SerialInterfaceHandler m_SerialInterfaceHandler;
    SPRegistersHandler m_SPRegistersHandler;
    VideoInterfaceHandler m_VideoInterfaceHandler;
    uint8_t * m_RDRAM;
    uint32_t m_AllocatedRdramSize;
    CN64Rom & m_Rom;

    mutable char m_strLabelName[100];
    uint32_t * m_TLB_ReadMap;
    uint32_t * m_TLB_WriteMap;
    size_t * m_MemoryReadMap;
    size_t * m_MemoryWriteMap;

    static uint32_t RegModValue;
};
