#include "stdafx.h"
#include "DMALog.h"

void CDMALog::AddEntry(uint32_t romAddr, uint32_t ramAddr, uint32_t length)
{
    DMALOGENTRY entry = { romAddr, ramAddr, length };
    m_Log.push_back(entry);
}

void CDMALog::ClearEntries()
{
    m_Log.clear();
}

size_t CDMALog::GetNumEntries()
{
    return m_Log.size();
}

DMALOGENTRY* CDMALog::GetEntryByIndex(uint32_t index)
{
    if (index < m_Log.size())
    {
        return &m_Log[index];
    }
    return NULL;
}

DMALOGENTRY* CDMALog::GetEntryByRamAddress(uint32_t ramAddr)
{
    uint32_t nEntries = GetNumEntries();

    if (nEntries == 0)
    {
        return NULL;
    }

    for (uint32_t i = nEntries - 1; i-- > 0;)
    {
        uint32_t min = m_Log[i].ramAddr;
        uint32_t max = min + m_Log[i].length - 1;

        if (ramAddr >= min && ramAddr <= max)
        {
            return &m_Log[i];
        }
    }
    return NULL;
}

DMALOGENTRY* CDMALog::GetEntryByRamAddress(uint32_t ramAddr, uint32_t* lpRomAddr, uint32_t* lpOffset)
{
    DMALOGENTRY* lpEntry = GetEntryByRamAddress(ramAddr);

    if (lpEntry == NULL)
    {
        return NULL;
    }

    *lpOffset = ramAddr - lpEntry->ramAddr;
    *lpRomAddr = lpEntry->romAddr + *lpOffset;

    return lpEntry;
}

DMALOGENTRY* CDMALog::GetEntryByRomAddress(uint32_t romAddr)
{
    uint32_t nEntries = GetNumEntries();

    if (nEntries == 0)
    {
        return NULL;
    }

    for (uint32_t i = nEntries - 1; i-- > 0; )
    {
        uint32_t min = m_Log[i].romAddr;
        uint32_t max = min + m_Log[i].length - 1;

        if (romAddr >= min && romAddr <= max)
        {
            return &m_Log[i];
        }
    }
    return NULL;
}

DMALOGENTRY* CDMALog::GetEntryByRomAddress(uint32_t romAddr, uint32_t* lpRamAddr, uint32_t* lpOffset)
{
    DMALOGENTRY* lpEntry = GetEntryByRomAddress(romAddr);

    if (lpEntry == NULL)
    {
        return NULL;
    }

    *lpOffset = romAddr - lpEntry->romAddr;
    *lpRamAddr = lpEntry->ramAddr + *lpOffset;

    return lpEntry;
}