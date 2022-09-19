#include "stdafx.h"
#include "TLB.h"
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/Register.h>

CTLB::CTLB(CTLB_CB * CallBack) :
m_CB(CallBack)
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

bool CTLB::AddressDefined(uint32_t VAddr)
{
    uint32_t i;

    if (VAddr >= 0x80000000 && VAddr <= 0xBFFFFFFF)
    {
        return true;
    }

    for (i = 0; i < 64; i++)
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

void CTLB::Probe()
{
    int Counter;

    WriteTrace(TraceTLB, TraceDebug, "Start");
    g_Reg->INDEX_REGISTER |= 0x80000000;
    for (Counter = 0; Counter < 32; Counter++)
    {
        if (!m_tlb[Counter].EntryDefined)
        {
            continue;
        }

        uint32_t & TlbEntryHiValue = m_tlb[Counter].EntryHi.Value;
        uint32_t Mask = ~m_tlb[Counter].PageMask.Mask << 13;
        uint32_t TlbValueMasked = TlbEntryHiValue & Mask;
        uint32_t EntryHiMasked = g_Reg->ENTRYHI_REGISTER & Mask;

        if (TlbValueMasked == EntryHiMasked)
        {
            if ((TlbEntryHiValue & 0x100) != 0 || // Global
                ((TlbEntryHiValue & 0xFF) == (g_Reg->ENTRYHI_REGISTER & 0xFF))) // SameAsid
            {
                g_Reg->INDEX_REGISTER = Counter;
                int FastIndx = Counter << 1;
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
    uint32_t index = g_Reg->INDEX_REGISTER & 0x1F;

    g_Reg->PAGE_MASK_REGISTER = m_tlb[index].PageMask.Value;
    g_Reg->ENTRYHI_REGISTER = (m_tlb[index].EntryHi.Value & ~m_tlb[index].PageMask.Value);
    g_Reg->ENTRYLO0_REGISTER = m_tlb[index].EntryLo0.Value;
    g_Reg->ENTRYLO1_REGISTER = m_tlb[index].EntryLo1.Value;
}

void CTLB::WriteEntry(int index, bool Random)
{
    int FastIndx;

    WriteTrace(TraceTLB, TraceDebug, "%02d %d %08X %08X %08X %08X ", index, Random, g_Reg->PAGE_MASK_REGISTER, g_Reg->ENTRYHI_REGISTER, g_Reg->ENTRYLO0_REGISTER, g_Reg->ENTRYLO1_REGISTER);

    // Check to see if entry is unmapping itself
    if (m_tlb[index].EntryDefined)
    {
        FastIndx = index << 1;
        if (*_PROGRAM_COUNTER >= m_FastTlb[FastIndx].VSTART &&
            *_PROGRAM_COUNTER < m_FastTlb[FastIndx].VEND &&
            m_FastTlb[FastIndx].ValidEntry && m_FastTlb[FastIndx].VALID)
        {
            WriteTrace(TraceTLB, TraceDebug, "Ignored PC: %X VAddr Start: %X VEND: %X", *_PROGRAM_COUNTER, m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].VEND);
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
    if (m_tlb[index].EntryDefined)
    {
        for (FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++)
        {
            if (!m_FastTlb[FastIndx].ValidEntry)
            {
                continue;
            }
            if (!m_FastTlb[FastIndx].VALID)
            {
                continue;
            }
            if (m_tlb[index].PageMask.Value == g_Reg->PAGE_MASK_REGISTER &&
                m_tlb[index].EntryHi.Value == g_Reg->ENTRYHI_REGISTER)
            {
                if (FastIndx == (index << 1) && m_tlb[index].EntryLo0.Value == g_Reg->ENTRYLO0_REGISTER)
                {
                    continue;
                }
                if (FastIndx != (index << 1) && m_tlb[index].EntryLo1.Value == g_Reg->ENTRYLO1_REGISTER)
                {
                    continue;
                }
            }
            m_CB->TLB_Unmaped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length);
        }
    }

    // Fill in m_tlb entry
    m_tlb[index].PageMask.Value = (uint32_t)g_Reg->PAGE_MASK_REGISTER;
    m_tlb[index].EntryHi.Value = (uint32_t)g_Reg->ENTRYHI_REGISTER;
    m_tlb[index].EntryLo0.Value = (uint32_t)g_Reg->ENTRYLO0_REGISTER;
    m_tlb[index].EntryLo1.Value = (uint32_t)g_Reg->ENTRYLO1_REGISTER;
    m_tlb[index].EntryDefined = true;
    SetupTLB_Entry(index, Random);
    m_CB->TLB_Changed();
}

void CTLB::SetupTLB_Entry(int index, bool Random)
{
    // Fix up fast TLB entries
    if (!m_tlb[index].EntryDefined)
    {
        return;
    }

    int FastIndx = index << 1;
    if (m_FastTlb[FastIndx].VALID)
    {
        m_CB->TLB_Unmaped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length);
    }
    m_FastTlb[FastIndx].Length = (m_tlb[index].PageMask.Mask << 12) + 0xFFF;
    m_FastTlb[FastIndx].VSTART = m_tlb[index].EntryHi.VPN2 << 13;
    m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].PHYSSTART = m_tlb[index].EntryLo0.PFN << 12;
    m_FastTlb[FastIndx].PHYSEND = m_FastTlb[FastIndx].PHYSSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].VALID = m_tlb[index].EntryLo0.V;
    m_FastTlb[FastIndx].DIRTY = m_tlb[index].EntryLo0.D;
    m_FastTlb[FastIndx].GLOBAL = m_tlb[index].EntryLo0.GLOBAL & m_tlb[index].EntryLo1.GLOBAL;
    m_FastTlb[FastIndx].ValidEntry = false;
    m_FastTlb[FastIndx].Random = Random;
    m_FastTlb[FastIndx].Probed = false;

    FastIndx = (index << 1) + 1;
    if (m_FastTlb[FastIndx].VALID)
    {
        m_CB->TLB_Unmaped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length);
    }
    m_FastTlb[FastIndx].Length = (m_tlb[index].PageMask.Mask << 12) + 0xFFF;
    m_FastTlb[FastIndx].VSTART = (m_tlb[index].EntryHi.VPN2 << 13) + (m_FastTlb[FastIndx].Length + 1);
    m_FastTlb[FastIndx].VEND = m_FastTlb[FastIndx].VSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].PHYSSTART = m_tlb[index].EntryLo1.PFN << 12;
    m_FastTlb[FastIndx].PHYSEND = m_FastTlb[FastIndx].PHYSSTART + m_FastTlb[FastIndx].Length;
    m_FastTlb[FastIndx].VALID = m_tlb[index].EntryLo1.V;
    m_FastTlb[FastIndx].DIRTY = m_tlb[index].EntryLo1.D;
    m_FastTlb[FastIndx].GLOBAL = m_tlb[index].EntryLo0.GLOBAL & m_tlb[index].EntryLo1.GLOBAL;
    m_FastTlb[FastIndx].ValidEntry = false;
    m_FastTlb[FastIndx].Random = Random;
    m_FastTlb[FastIndx].Probed = false;

    // Test both entries to see if they are valid
    for (FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++)
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
        m_CB->TLB_Mapped(m_FastTlb[FastIndx].VSTART, m_FastTlb[FastIndx].Length, m_FastTlb[FastIndx].PHYSSTART, !m_FastTlb[FastIndx].DIRTY);
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

void CTLB::RecordDifference(CLog &LogFile, const CTLB& rTLB)
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

bool CTLB::operator == (const CTLB& rTLB) const
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

bool CTLB::operator != (const CTLB& rTLB) const
{
    return !(*this == rTLB);
}