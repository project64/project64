#include "stdafx.h"

#include "InstructionCache.h"
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>

CInstructionCache::CInstructionCache(CMipsMemoryVM & MMU) :
    m_MMU(MMU)
{
    Reset();
}

void CInstructionCache::Reset()
{
    memset(m_Lines, 0, sizeof(m_Lines));
}

uint32_t CInstructionCache::Fetch(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (!IsHit(Index, PAddr))
    {
        FillLine(Index, PAddr);
    }
    return *(uint32_t *)(m_Lines[Index].Data + (VAddr & 0x1C));
}

void CInstructionCache::IndexInvalidate(uint32_t VAddr, uint32_t PAddr)
{
    CacheLine & Entry = m_Lines[LineIndex(VAddr)];
    Entry.PTag = PAddr >> 12;
    Entry.Valid = false;
}

uint32_t CInstructionCache::IndexLoadTag(uint32_t VAddr) const
{
    const CacheLine & Entry = m_Lines[LineIndex(VAddr)];
    return (Entry.PTag << 8) | (Entry.Valid ? 0x80 : 0x00);
}

void CInstructionCache::IndexStoreTag(uint32_t VAddr, uint32_t TagLo)
{
    CacheLine & Entry = m_Lines[LineIndex(VAddr)];
    Entry.PTag = (TagLo >> 8) & 0xFFFFF;
    Entry.Valid = (TagLo & 0x80) != 0;
}

void CInstructionCache::HitInvalidate(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (IsHit(Index, PAddr))
    {
        m_Lines[Index].Valid = false;
    }
}

void CInstructionCache::Fill(uint32_t VAddr, uint32_t PAddr)
{
    FillLine(LineIndex(VAddr), PAddr);
}

void CInstructionCache::HitWriteBack(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (!IsHit(Index, PAddr))
    {
        return;
    }
    uint32_t LinePAddr = PAddr & ~(LineSize - 1);
    if (LinePAddr < m_MMU.RdramSize())
    {
        memcpy(m_MMU.Rdram() + LinePAddr, m_Lines[Index].Data, LineSize);
    }
}

uint32_t CInstructionCache::LineIndex(uint32_t VAddr)
{
    return (VAddr >> 5) & (LineCount - 1);
}

bool CInstructionCache::IsHit(uint32_t Index, uint32_t PAddr) const
{
    return m_Lines[Index].Valid && m_Lines[Index].PTag == (PAddr >> 12);
}

void CInstructionCache::FillLine(uint32_t Index, uint32_t PAddr)
{
    CacheLine & Entry = m_Lines[Index];
    PAddr &= ~(LineSize - 1);
    if (PAddr < m_MMU.RdramSize())
    {
        memcpy(Entry.Data, m_MMU.Rdram() + PAddr, LineSize);
    }
    else
    {
        memset(Entry.Data, 0, LineSize);
    }
    Entry.PTag = PAddr >> 12;
    Entry.Valid = true;
}
