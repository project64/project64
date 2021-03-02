#pragma once

#include <stdafx.h>

struct DMALOGENTRY
{
    uint32_t romAddr;
    uint32_t ramAddr;
    uint32_t length;
};

class CDMALog
{
private:
    vector<DMALOGENTRY> m_Log;

public:
    void         AddEntry(uint32_t romAddr, uint32_t ramAddr, uint32_t length);
    void         ClearEntries();
    size_t       GetNumEntries();
    DMALOGENTRY* GetEntryByIndex(uint32_t index);
    DMALOGENTRY* GetEntryByRamAddress(uint32_t ramAddr);
    DMALOGENTRY* GetEntryByRamAddress(uint32_t ramAddr, uint32_t* lpRomAddr, uint32_t* lpOffset);
    DMALOGENTRY* GetEntryByRomAddress(uint32_t romAddr);
    DMALOGENTRY* GetEntryByRomAddress(uint32_t romAddr, uint32_t* lpRamAddr, uint32_t* lpOffset);
};