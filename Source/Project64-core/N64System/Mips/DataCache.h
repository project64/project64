#pragma once
#include <stdint.h>

class CMipsMemoryVM;

class CDataCache
{
public:
    enum
    {
        LineCount = 512,
        LineSize = 16,
    };

    CDataCache(CMipsMemoryVM & MMU);

    void Reset();
    void WriteBackAll();

    uint8_t * Line(uint32_t VAddr, uint32_t PAddr, bool Write);

    void IndexWriteBackInvalidate(uint32_t VAddr, uint32_t PAddr);
    uint32_t IndexLoadTag(uint32_t VAddr) const;
    void IndexStoreTag(uint32_t VAddr, uint32_t TagLo);
    void CreateDirtyExclusive(uint32_t VAddr, uint32_t PAddr);
    void HitInvalidate(uint32_t VAddr, uint32_t PAddr);
    void HitWriteBackInvalidate(uint32_t VAddr, uint32_t PAddr);
    void HitWriteBack(uint32_t VAddr, uint32_t PAddr);

private:
    CDataCache();
    CDataCache(const CDataCache &);
    CDataCache & operator=(const CDataCache &);

    struct CacheLine
    {
        uint32_t PTag;
        bool Valid;
        bool Dirty; // Kept while the line is invalid
        uint8_t Data[LineSize];
    };

    static uint32_t LineIndex(uint32_t VAddr);
    bool IsHit(uint32_t Index, uint32_t PAddr) const;
    void WriteBack(uint32_t Index);
    void Fill(uint32_t Index, uint32_t PAddr);

    CMipsMemoryVM & m_MMU;
    CacheLine m_Lines[LineCount];
};
