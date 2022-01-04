#pragma once
#include <Project64-core\Settings\GameSettings.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Logging.h>
#include "MemoryHandler.h"
#include <stdint.h>

class PeripheralInterfaceReg
{
protected:
    PeripheralInterfaceReg(uint32_t * PeripheralInterface);

public:
    uint32_t & PI_DRAM_ADDR_REG;
    uint32_t & PI_CART_ADDR_REG;
    uint32_t & PI_RD_LEN_REG;
    uint32_t & PI_WR_LEN_REG;
    uint32_t & PI_STATUS_REG;
    uint32_t & PI_BSD_DOM1_LAT_REG;
    uint32_t & PI_DOMAIN1_REG;
    uint32_t & PI_BSD_DOM1_PWD_REG;
    uint32_t & PI_BSD_DOM1_PGS_REG;
    uint32_t & PI_BSD_DOM1_RLS_REG;
    uint32_t & PI_BSD_DOM2_LAT_REG;
    uint32_t & PI_DOMAIN2_REG;
    uint32_t & PI_BSD_DOM2_PWD_REG;
    uint32_t & PI_BSD_DOM2_PGS_REG;
    uint32_t & PI_BSD_DOM2_RLS_REG;

private:
    PeripheralInterfaceReg();
    PeripheralInterfaceReg(const PeripheralInterfaceReg&);
    PeripheralInterfaceReg& operator=(const PeripheralInterfaceReg&);
};

class CRegisters;
class CMipsMemoryVM;

class PeripheralInterfaceHandler :
    public MemoryHandler,
    private CGameSettings,
    private CDebugSettings,
    private CLogging,
    private PeripheralInterfaceReg
{
public:
    PeripheralInterfaceHandler(CMipsMemoryVM & MMU, CRegisters & Reg);
    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    PeripheralInterfaceHandler();
    PeripheralInterfaceHandler(const PeripheralInterfaceHandler &);
    PeripheralInterfaceHandler & operator=(const PeripheralInterfaceHandler &);

    CMipsMemoryVM & m_MMU;
    CRegisters & m_Reg;
    uint32_t & m_PC;
};
