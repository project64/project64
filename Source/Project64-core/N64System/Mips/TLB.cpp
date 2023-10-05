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
    m_Recomp(Recomp)
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
}

bool CTLB::AddressDefined(uint64_t VAddr)
{
    if (VAddr >= 0x80000000 && VAddr <= 0xBFFFFFFF)
    {
        return true;
    }

    for (uint32_t i = 0; i < 64; i++)
    {
        if (m_FastTlb[i].ValidEntry &&
            VAddr >= m_FastTlb[i].VSTART &&
            VAddr <= m_FastTlb[i].VEND)
        {
            return true;
        }
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

        uint64_t & TlbEntryHiValue = m_tlb[i].EntryHi.Value;
        uint32_t Mask = (uint32_t)(~m_tlb[i].PageMask.Mask << 13);
        uint32_t TlbValueMasked = TlbEntryHiValue & Mask;
        uint32_t EntryHiMasked = m_Reg.ENTRYHI_REGISTER.Value & Mask;

        if (TlbValueMasked == EntryHiMasked)
        {
            if ((TlbEntryHiValue & 0x100) != 0 ||                                    // Global
                ((TlbEntryHiValue & 0xFF) == (m_Reg.ENTRYHI_REGISTER.Value & 0xFF))) // SameAsid
            {
                m_Reg.INDEX_REGISTER = i;
                uint32_t FastIndx = i << 1;
                m_FastTlb[FastIndx].Probed = true;
                m_FastTlb[FastIndx + 1].Probed = true;
                return;
            }
        }
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
    m_FastTlb[FastIndx].Length = (m_tlb[Index].PageMask.Mask << 12) + 0xFFF;
    m_FastTlb[FastIndx].VSTART = m_tlb[Index].EntryHi.R << 62 | m_tlb[Index].EntryHi.VPN2 << 13;
    m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].PHYSSTART = m_tlb[Index].EntryLo0.PFN << 12;
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
    m_FastTlb[FastIndx].Length = (m_tlb[Index].PageMask.Mask << 12) + 0xFFF;
    m_FastTlb[FastIndx].VSTART = (m_tlb[Index].EntryHi.R << 62 | (m_tlb[Index].EntryHi.VPN2 << 13)) + (m_FastTlb[FastIndx].Length + 1);
    m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].PHYSSTART = m_tlb[Index].EntryLo1.PFN << 12;
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

void CTLB::TLB_Unmaped(uint32_t VAddr, uint32_t Len)
{
    m_MMU.TLB_Unmaped(VAddr, Len);
    if (m_Recomp && bSMM_TLB())
    {
        m_Recomp->ClearRecompCode_Virt(VAddr, Len, CRecompiler::Remove_TLB);
    }
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
            VAddr = m_FastTlb[i].VSTART + (PAddr - m_FastTlb[i].PHYSSTART);
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