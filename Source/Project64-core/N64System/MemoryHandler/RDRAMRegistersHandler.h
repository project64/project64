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
    struct RDRAM_DEVICE
    {
        uint32_t DeviceType;
        uint32_t DeviceId;
        uint32_t Delay;
        uint32_t Mode;
        uint32_t RefreshInterval;
        uint32_t RefreshRow;
        uint32_t RasInterval;
        uint32_t MinInterval;
        uint32_t AddressSelect;
        uint32_t DeviceManufacturer;
        uint32_t CurrentControl;
    };

public:
    RDRAMRegistersHandler(CRegisters & Reg);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    RDRAMRegistersHandler();
    RDRAMRegistersHandler(const RDRAMRegistersHandler &);
    RDRAMRegistersHandler & operator=(const RDRAMRegistersHandler &);

    RDRAM_DEVICE m_Device[4];
    uint64_t & m_PC;
};