#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Logging.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <stdint.h>

class RDRAMRegistersReg
{
protected:
    RDRAMRegistersReg(uint32_t * RdramInterface);

public:
    uint32_t & RDRAM_CONFIG_REG;
    uint32_t & RDRAM_DEVICE_TYPE_REG;
    uint32_t & RDRAM_DEVICE_ID_REG;
    uint32_t & RDRAM_DELAY_REG;
    uint32_t & RDRAM_MODE_REG;
    uint32_t & RDRAM_REF_INTERVAL_REG;
    uint32_t & RDRAM_REF_ROW_REG;
    uint32_t & RDRAM_RAS_INTERVAL_REG;
    uint32_t & RDRAM_MIN_INTERVAL_REG;
    uint32_t & RDRAM_ADDR_SELECT_REG;
    uint32_t & RDRAM_DEVICE_MANUF_REG;

private:
    RDRAMRegistersReg();
    RDRAMRegistersReg(const RDRAMRegistersReg &);
    RDRAMRegistersReg & operator=(const RDRAMRegistersReg &);
};

class CRegisters;

class RDRAMRegistersHandler :
    public MemoryHandler,
    public RDRAMRegistersReg,
    private CDebugSettings,
    private CLogging
{
public:
    RDRAMRegistersHandler(CRegisters & Reg);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    RDRAMRegistersHandler();
    RDRAMRegistersHandler(const RDRAMRegistersHandler &);
    RDRAMRegistersHandler & operator=(const RDRAMRegistersHandler &);

    uint32_t & m_PC;
};