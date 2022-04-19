#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Logging.h>
#include <stdint.h>

class CMipsMemoryVM;
class CRegisters;

class PifRamHandler :
    public MemoryHandler,
    private CDebugSettings,
    private CLogging
{
public:
    PifRamHandler(CMipsMemoryVM & MMU, CRegisters & Reg);
    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    PifRamHandler();
    PifRamHandler(const PifRamHandler &);
    PifRamHandler & operator=(const PifRamHandler &);

    static uint32_t swap32by8(uint32_t word);

    CMipsMemoryVM & m_MMU;
    uint8_t * m_PifRam;
    uint32_t & m_PC;
};