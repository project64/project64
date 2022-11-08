#pragma once
#include "MemoryHandler.h"
#include <Common/File.h>
#include <memory>
#include <stdint.h>
#include <vector>

class CN64System;
class RomMemoryHandler;
class CN64Rom;

class ISViewerHandler :
    public MemoryHandler
{
public:
    ISViewerHandler(CN64System & System, RomMemoryHandler & RomHandler, CN64Rom & Rom);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    ISViewerHandler();
    ISViewerHandler(const ISViewerHandler &);
    ISViewerHandler & operator=(const ISViewerHandler &);

    static void stSystemReset(ISViewerHandler * _this)
    {
        _this->SystemReset();
    }
    static uint32_t Swap32by8(uint32_t Value);

    void SystemReset(void);

    RomMemoryHandler & m_RomMemoryHandler;
    CN64Rom & m_Rom;
    std::unique_ptr<CFile> m_hLogFile;
    std::vector<uint8_t> m_Data;
    char m_Buffer[0x1000];
    uint32_t m_BufferPos;
};
