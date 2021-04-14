#pragma once

#include <Common/Log.h>
#include <Project64-core/N64System/Mips/Register.h>

class CDebugTlb;

__interface CTLB_CB
{
    virtual void TLB_Mapped(uint32_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly) = 0;
    virtual void TLB_Unmaped(uint32_t VAddr, uint32_t Len) = 0;
    virtual void TLB_Changed() = 0;
};

#pragma warning(push)
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union

class CTLB :
    protected CSystemRegisters
{
public:
    struct TLB_ENTRY
    {
        bool EntryDefined;
        union
        {
            uint32_t Value;
            uint8_t A[4];

            struct
            {
                unsigned zero : 13;
                unsigned Mask : 12;
                unsigned zero2 : 7;
            };
        } PageMask;

        union
        {
            uint32_t Value;
            uint8_t A[4];

            struct
            {
                unsigned ASID : 8;
                unsigned Zero : 4;
                unsigned G : 1;
                unsigned VPN2 : 19;
            };
        } EntryHi;

        union
        {
            uint32_t Value;
            uint8_t A[4];

            struct
            {
                unsigned GLOBAL : 1;
                unsigned V : 1;
                unsigned D : 1;
                unsigned C : 3;
                unsigned PFN : 20;
                unsigned ZERO : 6;
            };
        } EntryLo0;

        union
        {
            uint32_t Value;
            uint8_t A[4];

            struct
            {
                unsigned GLOBAL : 1;
                unsigned V : 1;
                unsigned D : 1;
                unsigned C : 3;
                unsigned PFN : 20;
                unsigned ZERO : 6;
            };
        } EntryLo1;
    };

public:
    CTLB(CTLB_CB * CallBack);
    ~CTLB();

    void Reset(bool InvalidateTLB);

    //Used by opcodes of the same name to manipulate the tlb (reads the registers)
    void Probe();
    void ReadEntry();
    void WriteEntry(int32_t index, bool Random);

    //See if a VAddr has an entry to translate to a PAddr
    bool AddressDefined(uint32_t VAddr);

    const TLB_ENTRY & TlbEntry(int32_t Entry) const
    {
        return m_tlb[Entry];
    }

    bool PAddrToVAddr(uint32_t PAddr, uint32_t & VAddr, uint32_t & Index);

    void RecordDifference(CLog &LogFile, const CTLB& rTLB);

    bool operator == (const CTLB& rTLB) const;
    bool operator != (const CTLB& rTLB) const;

private:
    struct FASTTLB
    {
        uint32_t VSTART;
        uint32_t VEND;
        uint32_t PHYSSTART;
        uint32_t PHYSEND;
        uint32_t Length;
        bool  VALID;
        bool  DIRTY;
        bool  GLOBAL;
        bool  ValidEntry;
        bool  Random;
        bool  Probed;
    };

    friend class CDebugTlb; // enable debug window to read class

    CTLB_CB * const m_CB;

    TLB_ENTRY m_tlb[32];
    FASTTLB   m_FastTlb[64];

    void SetupTLB_Entry(int32_t index, bool Random);

private:
    CTLB();
    CTLB(const CTLB&);
    CTLB& operator=(const CTLB&);
};

#pragma warning(pop)
