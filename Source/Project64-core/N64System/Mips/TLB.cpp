#include "stdafx.h"

#include "TLB.h"
#include <Project64-core/Debugger.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/Recompiler/Recompiler.h>
#include <Project64-core/N64System/SystemGlobals.h>

CTLB::CTLB(CMipsMemoryVM & MMU, CRegisters & Reg, CRecompiler *& Recomp) :
    m_MMU(MMU),
    m_Reg(Reg),
    m_Recomp(Recomp),
    m_PrivilegeMode(PrivilegeMode_Kernel),
    m_AddressSize32bit(true)
{
    WriteTrace(TraceTLB, TraceDebug, "Start");
    memset(m_tlb, 0, sizeof(m_tlb));
    memset(m_FastTlb, 0, sizeof(m_FastTlb));
    Reset(true);
    WriteTrace(TraceTLB, TraceDebug, "Done");
}

CTLB::~CTLB()
{
    WriteTrace(TraceTLB, TraceDebug, "Start");
    WriteTrace(TraceTLB, TraceDebug, "Done");
}

void CTLB::Reset(bool InvalidateTLB)
{
    uint32_t count;

    for (count = 0; count < 64; count++)
    {
        m_FastTlb[count].ValidEntry = false;
    }

    if (InvalidateTLB)
    {
        for (count = 0; count < 32; count++)
        {
            m_tlb[count].EntryDefined = false;
        }
    }
    else
    {
        for (count = 0; count < 32; count++)
        {
            SetupTLB_Entry(count, false);
        }
    }
    COP0StatusChanged();
}

bool CTLB::AddressDefined(uint64_t VAddr, bool & Dirty)
{
    Dirty = true;
    MemorySegment Segment = VAddrMemorySegment(VAddr);
    if (Segment == MemorySegment_Mapped)
    {
        for (uint32_t i = 0; i < 64; i++)
        {
            if (!m_FastTlb[i].GLOBAL || !m_FastTlb[i].ValidEntry || VAddr < m_FastTlb[i].VSTART || VAddr > m_FastTlb[i].VEND + 1)
            {
                continue;
            }
            if (!m_FastTlb[i].VALID)
            {
                return true;
            }
            Dirty = m_FastTlb[i].DIRTY;
            return true;
        }
        return false;
    }
    return false;
}

TLB_ENTRY & CTLB::TlbEntry(int32_t Entry)
{
    return m_tlb[Entry];
}

void CTLB::Probe()
{
    WriteTrace(TraceTLB, TraceDebug, "Start");
    m_Reg.INDEX_REGISTER = 0x80000000;
    for (uint32_t i = 0; i < 32; i++)
    {
        if (!m_tlb[i].EntryDefined)
        {
            continue;
        }

        const COP0EntryHi & TlbEntryHiValue = m_tlb[i].EntryHi;
        uint64_t Mask = ~m_tlb[i].PageMask.Mask << 13;
        uint64_t TlbValueMasked = TlbEntryHiValue.Value & Mask;
        uint64_t EntryHiMasked = m_Reg.ENTRYHI_REGISTER.Value & Mask;

        if (TlbValueMasked != EntryHiMasked ||
            TlbEntryHiValue.R != m_Reg.ENTRYHI_REGISTER.R ||
            (m_tlb[i].EntryLo0.GLOBAL == 0 || m_tlb[i].EntryLo1.GLOBAL == 0) && TlbEntryHiValue.ASID != m_Reg.ENTRYHI_REGISTER.ASID)
        {
            continue;
        }
        m_Reg.INDEX_REGISTER = i;
        uint32_t FastIndx = i << 1;
        m_FastTlb[FastIndx].Probed = true;
        m_FastTlb[FastIndx + 1].Probed = true;
        break;
    }
    WriteTrace(TraceTLB, TraceDebug, "Done");
}

void CTLB::ReadEntry()
{
    uint32_t Index = m_Reg.INDEX_REGISTER & 0x1F;

    m_Reg.PAGE_MASK_REGISTER.Value = m_tlb[Index].PageMask.Value;
    m_Reg.ENTRYHI_REGISTER.Value = (m_tlb[Index].EntryHi.Value & ~m_tlb[Index].PageMask.Value);
    m_Reg.ENTRYLO0_REGISTER.Value = m_tlb[Index].EntryLo0.Value;
    m_Reg.ENTRYLO1_REGISTER.Value = m_tlb[Index].EntryLo1.Value;
}

void CTLB::WriteEntry(uint32_t Index, bool Random)
{
    WriteTrace(TraceTLB, TraceDebug, "%02d %d %I64X %I64X %I64X %I64X", Index, Random, m_Reg.PAGE_MASK_REGISTER, m_Reg.ENTRYHI_REGISTER, m_Reg.ENTRYLO0_REGISTER, m_Reg.ENTRYLO1_REGISTER);

    // Check to see if entry is unmapping itself
    if (m_tlb[Index].EntryDefined)
    {
        uint32_t FastIndx = Index << 1;
        if (*_PROGRAM_COUNTER >= m_FastTlb[FastIndx].VSTART &&
            *_PROGRAM_COUNTER < m_FastTlb[FastIndx].VEND &&
            m_FastTlb[FastIndx].ValidEntry && m_FastTlb[FastIndx].VALID)
        {
            WriteTrace(TraceTLB, TraceDebug, "Ignored PC: %X VAddr Start: %I64X VEND: %I64X", *_PROGRAM_COUNTER, m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].VEND);
            return;
        }
        if (*_PROGRAM_COUNTER >= m_FastTlb[FastIndx + 1].VSTART &&
            *_PROGRAM_COUNTER < m_FastTlb[FastIndx + 1].VEND &&
            m_FastTlb[FastIndx + 1].ValidEntry && m_FastTlb[FastIndx + 1].VALID)
        {
            WriteTrace(TraceTLB, TraceDebug, "Ignored PC: %X VAddr Start: %X VEND: %X", *_PROGRAM_COUNTER, m_FastTlb[FastIndx + 1].VSTART, m_FastTlb[FastIndx + 1].VEND);
            return;
        }
    }

    // Reset old addresses
    if (m_tlb[Index].EntryDefined)
    {
        for (uint32_t FastIndx = Index << 1; FastIndx <= (Index << 1) + 1; FastIndx++)
        {
            if (!m_FastTlb[FastIndx].ValidEntry)
            {
                continue;
            }
            if (!m_FastTlb[FastIndx].VALID)
            {
                continue;
            }
            if (m_tlb[Index].PageMask.Value == m_Reg.PAGE_MASK_REGISTER.Value &&
                m_tlb[Index].EntryHi.Value == m_Reg.ENTRYHI_REGISTER.Value)
            {
                if (FastIndx == (Index << 1) && m_tlb[Index].EntryLo0.Value == m_Reg.ENTRYLO0_REGISTER.Value)
                {
                    continue;
                }
                if (FastIndx != (Index << 1) && m_tlb[Index].EntryLo1.Value == m_Reg.ENTRYLO1_REGISTER.Value)
                {
                    continue;
                }
            }
            TLB_Unmaped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length);
        }
    }

    // Fill in m_tlb entry
    bool Gloabl = m_Reg.ENTRYLO0_REGISTER.GLOBAL & m_Reg.ENTRYLO1_REGISTER.GLOBAL;
    m_tlb[Index].PageMask.Value = m_Reg.PAGE_MASK_REGISTER.Value & 0x01554000;
    m_tlb[Index].PageMask.Value |= m_tlb[Index].PageMask.Value >> 1;
    m_tlb[Index].EntryHi = m_Reg.ENTRYHI_REGISTER;
    m_tlb[Index].EntryLo0 = m_Reg.ENTRYLO0_REGISTER;
    m_tlb[Index].EntryLo0.PFN = m_Reg.ENTRYLO0_REGISTER.PFN & 0xFFFFF;
    m_tlb[Index].EntryLo0.GLOBAL = Gloabl;
    m_tlb[Index].EntryLo1 = m_Reg.ENTRYLO1_REGISTER;
    m_tlb[Index].EntryLo1.PFN = m_Reg.ENTRYLO1_REGISTER.PFN & 0xFFFFF;
    m_tlb[Index].EntryLo1.GLOBAL = Gloabl;
    m_tlb[Index].EntryDefined = true;
    SetupTLB_Entry(Index, Random);
    if (g_Debugger != nullptr)
    {
        g_Debugger->TLBChanged();
    }
}

void CTLB::COP0StatusChanged(void)
{
    m_PrivilegeMode = m_Reg.STATUS_REGISTER.PrivilegeMode;
    switch (m_PrivilegeMode)
    {
    case PrivilegeMode_Kernel:
        m_AddressSize32bit = m_Reg.STATUS_REGISTER.KernelExtendedAddressing == 0;
        break;
    case PrivilegeMode_Supervisor:
        m_AddressSize32bit = m_Reg.STATUS_REGISTER.SupervisorExtendedAddressing == 0;
        break;
    case PrivilegeMode_User:
        m_AddressSize32bit = m_Reg.STATUS_REGISTER.UserExtendedAddressing == 0;
        break;
    }
}

void CTLB::SetupTLB_Entry(uint32_t Index, bool Random)
{
    // Fix up fast TLB entries
    if (!m_tlb[Index].EntryDefined)
    {
        return;
    }

    uint32_t FastIndx = Index << 1;
    if (m_FastTlb[FastIndx].VALID)
    {
        TLB_Unmaped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length);
    }
    m_FastTlb[FastIndx].Length = (uint32_t)((m_tlb[Index].PageMask.Mask << 12) + 0xFFF);
    m_FastTlb[FastIndx].VSTART = m_tlb[Index].EntryHi.R << 62 | m_tlb[Index].EntryHi.VPN2 << 13;
    m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].PHYSSTART = (uint32_t)(m_tlb[Index].EntryLo0.PFN << 12);
    m_FastTlb[FastIndx].PHYSEND = m_FastTlb[FastIndx].PHYSSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].VALID = m_tlb[Index].EntryLo0.V;
    m_FastTlb[FastIndx].DIRTY = m_tlb[Index].EntryLo0.D;
    m_FastTlb[FastIndx].GLOBAL = m_tlb[Index].EntryLo0.GLOBAL & m_tlb[Index].EntryLo1.GLOBAL;
    m_FastTlb[FastIndx].ValidEntry = false;
    m_FastTlb[FastIndx].Random = Random;
    m_FastTlb[FastIndx].Probed = false;

    FastIndx = (Index << 1) + 1;
    if (m_FastTlb[FastIndx].VALID)
    {
        TLB_Unmaped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length);
    }
    m_FastTlb[FastIndx].Length = (uint32_t)((m_tlb[Index].PageMask.Mask << 12) + 0xFFF);
    m_FastTlb[FastIndx].VSTART = (m_tlb[Index].EntryHi.R << 62 | (m_tlb[Index].EntryHi.VPN2 << 13)) + (m_FastTlb[FastIndx].Length + 1);
    m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].PHYSSTART = (uint32_t)m_tlb[Index].EntryLo1.PFN << 12;
    m_FastTlb[FastIndx].PHYSEND = m_FastTlb[FastIndx].PHYSSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].VALID = m_tlb[Index].EntryLo1.V;
    m_FastTlb[FastIndx].DIRTY = m_tlb[Index].EntryLo1.D;
    m_FastTlb[FastIndx].GLOBAL = m_tlb[Index].EntryLo0.GLOBAL & m_tlb[Index].EntryLo1.GLOBAL;
    m_FastTlb[FastIndx].ValidEntry = false;
    m_FastTlb[FastIndx].Random = Random;
    m_FastTlb[FastIndx].Probed = false;

    // Test both entries to see if they are valid
    for (FastIndx = Index << 1; FastIndx <= (Index << 1) + 1; FastIndx++)
    {
        if (!m_FastTlb[FastIndx].VALID)
        {
            m_FastTlb[FastIndx].ValidEntry = true;
            continue;
        }

        if (m_FastTlb[FastIndx].VEND <= m_FastTlb[FastIndx].VSTART)
        {
            continue;
        }
        if (m_FastTlb[FastIndx].VSTART >= 0x80000000 && m_FastTlb[FastIndx].VEND <= 0xBFFFFFFF)
        {
            continue;
        }
        if (m_FastTlb[FastIndx].PHYSEND > 0x1FFFFFFF)
        {
            continue;
        }

        // Map the new m_tlb entry for reading and writing
        m_FastTlb[FastIndx].ValidEntry = true;
        m_MMU.TLB_Mapped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length, m_FastTlb[FastIndx].PHYSSTART, !m_FastTlb[FastIndx].DIRTY);
    }
}

void CTLB::TLB_Unmaped(uint64_t VAddr, uint32_t Len)
{
    m_MMU.TLB_Unmaped(VAddr, Len);
    if (m_Recomp && bSMM_TLB() && (uint64_t)((int32_t)VAddr) == VAddr)
    {
        m_Recomp->ClearRecompCode_Virt((uint32_t)VAddr, Len, CRecompiler::Remove_TLB);
    }
}

bool CTLB::VAddrToPAddr(uint64_t VAddr, uint32_t & PAddr, bool & MemoryUnused)
{
    MemoryUnused = false;
    if (b32BitCore() && (uint64_t)((int32_t)VAddr) != VAddr)
    {
        MemoryUnused = true;
        return false;
    }
    MemorySegment Segment = VAddrMemorySegment(VAddr);
    if (Segment == MemorySegment_Mapped)
    {
        for (int i = 0; i < 64; i++)
        {
            if (m_FastTlb[i].ValidEntry == false)
            {
                continue;
            }
            if (VAddr >= m_FastTlb[i].VSTART && VAddr < m_FastTlb[i].VEND)
            {
                PAddr = (uint32_t)(m_FastTlb[i].PHYSSTART + (VAddr - m_FastTlb[i].VSTART));
                return true;
            }
        }
        return false;
    }
    if (Segment == MemorySegment_Unused)
    {
        MemoryUnused = true;
        return false;
    }
    if (Segment == MemorySegment_Direct32 || Segment == MemorySegment_Cached32)
    {
        PAddr = VAddr & 0x1FFFFFFF;
        return true;
    }
    return false;
}

MemorySegment CTLB::VAddrMemorySegment(uint64_t VAddr)
{
    if (m_AddressSize32bit)
    {
        if ((uint64_t)((int32_t)VAddr) != VAddr)
        {
            return MemorySegment_Unused;
        }
        uint32_t VAddr32 = (uint32_t)VAddr;
        if (m_PrivilegeMode == PrivilegeMode_Kernel)
        {
            if (VAddr32 <= 0x7fffffff) //kuseg
            {
                return MemorySegment_Mapped;
            }
            if (VAddr32 <= 0x9fffffff) //kseg0
            {
                return MemorySegment_Cached32;
            }
            if (VAddr32 <= 0xbfffffff) //kseg1
            {
                return MemorySegment_Direct32;
            }
            if (VAddr32 <= 0xdfffffff) //ksseg
            {
                return MemorySegment_Mapped;
            }
            if (VAddr32 <= 0xffffffff) //kseg3
            {
                return MemorySegment_Mapped;
            }
        }
        else if (m_PrivilegeMode == PrivilegeMode_Supervisor)
        {
            if (VAddr32 <= 0x7fffffff) //suseg
            {
                return MemorySegment_Mapped;
            }
            if (VAddr32 <= 0xbfffffff)
            {
                return MemorySegment_Unused;
            }
            if (VAddr32 <= 0xdfffffff) //sseg
            {
                return MemorySegment_Mapped;
            }
        }
        else if (m_PrivilegeMode == PrivilegeMode_User)
        {
            if (VAddr32 <= 0x7fffffff) //useg
            {
                return MemorySegment_Mapped;
            }
        }
        return MemorySegment_Unused;
    }

    if (m_PrivilegeMode == PrivilegeMode_Kernel)
    {
        if (VAddr <= 0x000000ffffffffffull) //xkuseg
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0x3fffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0x400000ffffffffffull) //xksseg
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0x7fffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0x80000000ffffffffull) //xkphys*
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0x87ffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0x88000000ffffffffull)
        {
            return MemorySegment_Cached32; //xkphys*
        }
        if (VAddr <= 0x8fffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0x90000000ffffffffull) //xkphys*
        {
            return MemorySegment_Direct32;
        }
        if (VAddr <= 0x97ffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0x98000000ffffffffull) //xkphys*
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0x9fffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xa0000000ffffffffull) //xkphys*
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0xa7ffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xa8000000ffffffffull) //xkphys*
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0xafffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xb0000000ffffffffull) //xkphys*
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0xb7ffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xb8000000ffffffffull) //xkphys*
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0xbfffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xc00000ff7fffffffull) //xkseg
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0xffffffff7fffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xffffffff9fffffffull) //ckseg0
        {
            return MemorySegment_Cached32;
        }
        if (VAddr <= 0xffffffffbfffffffull) //ckseg1
        {
            return MemorySegment_Direct32;
        }
        if (VAddr <= 0xffffffffdfffffffull) //ckseg2
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0xffffffffffffffffull) //ckseg3
        {
            return MemorySegment_Mapped;
        }
    }
    else if (m_PrivilegeMode == PrivilegeMode_Kernel)
    {
        if (VAddr <= 0x000000ffffffffffull) //xsuseg
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0x3fffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0x400000ffffffffffull) //xsseg
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0xffffffffbfffffffull)
        {
            return MemorySegment_Unused;
        }
        if (VAddr <= 0xffffffffdfffffffull) //csseg
        {
            return MemorySegment_Mapped;
        }
        if (VAddr <= 0xffffffffffffffffull)
        {
            return MemorySegment_Unused;
        }
    }
    else if (m_PrivilegeMode == PrivilegeMode_User)
    {
        if (VAddr <= 0x000000ffffffffffull)
        {
            return MemorySegment_Mapped; //xuseg
        }
    }
    return MemorySegment_Unused;
}

bool CTLB::PAddrToVAddr(uint32_t PAddr, uint32_t & VAddr, uint32_t & Index)
{
    for (int i = Index; i < 64; i++)
    {
        if (m_FastTlb[i].ValidEntry == false)
        {
            continue;
        }
        if (PAddr >= m_FastTlb[i].PHYSSTART && PAddr < m_FastTlb[i].PHYSEND)
        {
            VAddr = (uint32_t)(m_FastTlb[i].VSTART + (PAddr - m_FastTlb[i].PHYSSTART));
            Index = i + 1;
            return true;
        }
    }
    return false;
}

void CTLB::RecordDifference(CLog & LogFile, const CTLB & rTLB)
{
    for (int i = 0, n = sizeof(m_tlb) / sizeof(m_tlb[0]); i < n; i++)
    {
        if (m_tlb[i].EntryDefined != rTLB.m_tlb[i].EntryDefined)
        {
            LogFile.LogF("TLB[%d] Defined: %s %s\r\n", i, m_tlb[i].EntryDefined ? "Yes" : "No", rTLB.m_tlb[i].EntryDefined ? "Yes" : "No");
            continue;
        }
        if (!m_tlb[i].EntryDefined)
        {
            continue;
        }
        if (m_tlb[i].PageMask.Value != rTLB.m_tlb[i].PageMask.Value)
        {
            LogFile.LogF("TLB[%d] PageMask: %X %X\r\n", i, m_tlb[i].PageMask.Value, rTLB.m_tlb[i].PageMask.Value);
        }
        if (m_tlb[i].EntryHi.Value != rTLB.m_tlb[i].EntryHi.Value)
        {
            LogFile.LogF("TLB[%d] EntryHi: %X %X\r\n", i, m_tlb[i].EntryHi.Value, rTLB.m_tlb[i].EntryHi.Value);
        }
        if (m_tlb[i].EntryLo0.Value != rTLB.m_tlb[i].EntryLo0.Value)
        {
            LogFile.LogF("TLB[%d] EntryLo0: %X %X\r\n", i, m_tlb[i].EntryLo0.Value, rTLB.m_tlb[i].EntryLo0.Value);
        }
        if (m_tlb[i].EntryLo1.Value != rTLB.m_tlb[i].EntryLo1.Value)
        {
            LogFile.LogF("TLB[%d] EntryLo1: %X %X\r\n", i, m_tlb[i].EntryLo1.Value, rTLB.m_tlb[i].EntryLo1.Value);
        }
    }
}

bool CTLB::operator==(const CTLB & rTLB) const
{
    const size_t n = sizeof(m_tlb) / sizeof(m_tlb[0]);
    for (size_t i = 0; i < n; i++)
    {
        if (m_tlb[i].EntryDefined != rTLB.m_tlb[i].EntryDefined)
        {
            return false;
        }
        if (!m_tlb[i].EntryDefined)
        {
            continue;
        }
        if (m_tlb[i].PageMask.Value != rTLB.m_tlb[i].PageMask.Value ||
            m_tlb[i].EntryHi.Value != rTLB.m_tlb[i].EntryHi.Value ||
            m_tlb[i].EntryLo0.Value != rTLB.m_tlb[i].EntryLo0.Value ||
            m_tlb[i].EntryLo1.Value != rTLB.m_tlb[i].EntryLo1.Value)
        {
            return false;
        }
    }
    return true;
}

bool CTLB::operator!=(const CTLB & rTLB) const
{
    return !(*this == rTLB);
}