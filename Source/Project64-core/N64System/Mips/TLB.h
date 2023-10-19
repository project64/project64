#pragma once

#include <Common/Log.h>
#include <Project64-core/N64System/Mips/Register.h>

class CDebugTlb;
class CRecompiler;

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

enum MemorySegment
{
    MemorySegment_Mapped,
    MemorySegment_Cached,
    MemorySegment_Cached32,
    MemorySegment_Direct,
    MemorySegment_Direct32,
    MemorySegment_Unused,
};

struct TLB_ENTRY
{
    bool EntryDefined;
    COP0PageMask PageMask;
    COP0EntryHi EntryHi;
    COP0EntryLo EntryLo0;
    COP0EntryLo EntryLo1;
};

class CTLB :
    private CGameSettings
{
    friend class CDebugTlb;

    struct FASTTLB
    {
        uint64_t VSTART;
        uint64_t VEND;
        uint32_t PHYSSTART;
        uint32_t PHYSEND;
        uint32_t Length;
        bool VALID;
        bool DIRTY;
        bool GLOBAL;
        bool ValidEntry;
        bool Random;
        bool Probed;
    };

public:
    CTLB(CMipsMemoryVM & MMU, CRegisters & Reg, CRecompiler *& Recomp);
    ~CTLB();

    void Reset(bool InvalidateTLB);
    void Probe();
    void ReadEntry();
    void WriteEntry(uint32_t Index, bool Random);
    void COP0StatusChanged(void);
    bool AddressDefined(uint64_t VAddr, bool & Dirty);
    TLB_ENTRY & TlbEntry(int32_t Entry);

    bool VAddrToPAddr(uint64_t VAddr, uint32_t & PAddr, bool & MemoryUnused);
    bool PAddrToVAddr(uint32_t PAddr, uint32_t & VAddr, uint32_t & Index);
    void RecordDifference(CLog & LogFile, const CTLB & rTLB);

    bool operator==(const CTLB & rTLB) const;
    bool operator!=(const CTLB & rTLB) const;

private:
    CTLB();
    CTLB(const CTLB &);
    CTLB & operator=(const CTLB &);

    void SetupTLB_Entry(uint32_t Index, bool Random);
    void TLB_Unmaped(uint64_t VAddr, uint32_t Len);
    MemorySegment VAddrMemorySegment(uint64_t VAddr);

    PRIVILEGE_MODE m_PrivilegeMode;
    bool m_AddressSize32bit;
    CMipsMemoryVM & m_MMU;
    CRegisters & m_Reg;
    CRecompiler *& m_Recomp;
    TLB_ENTRY m_tlb[32];
    FASTTLB m_FastTlb[64];
};

#pragma warning(pop)
