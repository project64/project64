#pragma once
#include <stdint.h>

class CMipsMemoryVM;

class CInstructionCache
{
public:
    enum
    {
        LineCount = 512,
        LineSize = 32,
    };

    CInstructionCache(CMipsMemoryVM & MMU);

    void Reset();

    uint32_t Fetch(uint32_t VAddr, uint32_t PAddr);

    void IndexInvalidate(uint32_t VAddr, uint32_t PAddr);
    uint32_t IndexLoadTag(uint32_t VAddr) const;
    void IndexStoreTag(uint32_t VAddr, uint32_t TagLo);
    void HitInvalidate(uint32_t VAddr, uint32_t PAddr);
    void Fill(uint32_t VAddr, uint32_t PAddr);
    void HitWriteBack(uint32_t VAddr, uint32_t PAddr);

private:
    CInstructionCache();
    CInstructionCache(const CInstructionCache &);
    CInstructionCache & operator=(const CInstructionCache &);

    struct CacheLine
    {
        uint32_t PTag;
        bool Valid;
        uint8_t Data[LineSize];
    };

    static uint32_t LineIndex(uint32_t VAddr);
    bool IsHit(uint32_t Index, uint32_t PAddr) const;
    void FillLine(uint32_t Index, uint32_t PAddr);

    CMipsMemoryVM & m_MMU;
    CacheLine m_Lines[LineCount];
};
