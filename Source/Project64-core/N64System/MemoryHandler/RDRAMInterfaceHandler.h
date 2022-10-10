#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Logging.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <stdint.h>

class RDRAMInterfaceReg
{
protected:
    RDRAMInterfaceReg(uint32_t * RdramInterface);

public:
    uint32_t & RI_MODE_REG;
    uint32_t & RI_CONFIG_REG;
    uint32_t & RI_CURRENT_LOAD_REG;
    uint32_t & RI_SELECT_REG;
    uint32_t & RI_COUNT_REG;
    uint32_t & RI_REFRESH_REG;
    uint32_t & RI_LATENCY_REG;
    uint32_t & RI_RERROR_REG;
    uint32_t & RI_WERROR_REG;

private:
    RDRAMInterfaceReg();
    RDRAMInterfaceReg(const RDRAMInterfaceReg &);
    RDRAMInterfaceReg & operator=(const RDRAMInterfaceReg &);
};

class CRegisters;

class RDRAMInterfaceHandler :
    public MemoryHandler,
    private RDRAMInterfaceReg,
    private CDebugSettings,
    private CLogging
{
public:
    RDRAMInterfaceHandler(CRegisters & Reg);
    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    RDRAMInterfaceHandler();
    RDRAMInterfaceHandler(const RDRAMInterfaceHandler &);
    RDRAMInterfaceHandler & operator=(const RDRAMInterfaceHandler &);

    uint32_t & m_PC;
};
