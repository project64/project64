#include "stdafx.h"

#include "DataCache.h"
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>

CDataCache::CDataCache(CMipsMemoryVM & MMU) :
    m_MMU(MMU)
{
    Reset();
}

void CDataCache::Reset()
{
    memset(m_Lines, 0, sizeof(m_Lines));
}

void CDataCache::WriteBackAll()
{
    for (uint32_t i = 0; i < LineCount; i++)
    {
        if (m_Lines[i].Valid && m_Lines[i].Dirty)
        {
            WriteBack(i);
            m_Lines[i].Dirty = false;
        }
    }
}

uint8_t * CDataCache::Line(uint32_t VAddr, uint32_t PAddr, bool Write)
{
    uint32_t Index = LineIndex(VAddr);
    CacheLine & Entry = m_Lines[Index];
    if (!IsHit(Index, PAddr))
    {
        if (Entry.Valid && Entry.Dirty)
        {
            WriteBack(Index);
        }
        Fill(Index, PAddr);
    }
    if (Write)
    {
        Entry.Dirty = true;
    }
    return Entry.Data;
}

void CDataCache::IndexWriteBackInvalidate(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    CacheLine & Entry = m_Lines[Index];
    if (!Entry.Valid)
    {
        return;
    }
    if (Entry.Dirty)
    {
        WriteBack(Index);
    }
    Entry.PTag = PAddr >> 12;
    Entry.Valid = false;
}

uint32_t CDataCache::IndexLoadTag(uint32_t VAddr) const
{
    const CacheLine & Entry = m_Lines[LineIndex(VAddr)];
    return (Entry.PTag << 8) | (Entry.Valid ? 0xC0 : 0x00);
}

void CDataCache::IndexStoreTag(uint32_t VAddr, uint32_t TagLo)
{
    CacheLine & Entry = m_Lines[LineIndex(VAddr)];
    Entry.PTag = (TagLo >> 8) & 0xFFFFF;
    Entry.Valid = (TagLo & 0x80) != 0;
}

void CDataCache::CreateDirtyExclusive(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (IsHit(Index, PAddr))
    {
        return;
    }
    CacheLine & Entry = m_Lines[Index];
    if (Entry.Valid && Entry.Dirty)
    {
        WriteBack(Index);
    }
    Entry.PTag = PAddr >> 12;
    Entry.Valid = true;
    Entry.Dirty = false;
}

void CDataCache::HitInvalidate(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (IsHit(Index, PAddr))
    {
        m_Lines[Index].Valid = false;
    }
}

void CDataCache::HitWriteBackInvalidate(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (!IsHit(Index, PAddr))
    {
        return;
    }
    CacheLine & Entry = m_Lines[Index];
    if (Entry.Dirty)
    {
        WriteBack(Index);
    }
    Entry.Valid = false;
}

void CDataCache::HitWriteBack(uint32_t VAddr, uint32_t PAddr)
{
    uint32_t Index = LineIndex(VAddr);
    if (IsHit(Index, PAddr) && m_Lines[Index].Dirty)
    {
        WriteBack(Index);
        m_Lines[Index].Dirty = false;
    }
}

uint32_t CDataCache::LineIndex(uint32_t VAddr)
{
    return (VAddr >> 4) & (LineCount - 1);
}

bool CDataCache::IsHit(uint32_t Index, uint32_t PAddr) const
{
    return m_Lines[Index].Valid && m_Lines[Index].PTag == (PAddr >> 12);
}

void CDataCache::WriteBack(uint32_t Index)
{
    const CacheLine & Entry = m_Lines[Index];
    uint32_t PAddr = (Entry.PTag << 12) | ((Index & 0xFF) << 4);
    if (PAddr < m_MMU.RdramSize())
    {
        memcpy(m_MMU.Rdram() + PAddr, Entry.Data, LineSize);
    }
}

void CDataCache::Fill(uint32_t Index, uint32_t PAddr)
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
    Entry.Dirty = false;
}
