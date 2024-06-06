#pragma once
#include "MIPSInterfaceHandler.h"
#include "MemoryHandler.h"
#include <Project64-core\Logging.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <stdint.h>

enum
{
    SI_STATUS_DMA_BUSY = 0x0001,
    SI_STATUS_RD_BUSY = 0x0002,
    SI_STATUS_DMA_ERROR = 0x0008,
    SI_STATUS_INTERRUPT = 0x1000,
};

class PifRamHandler;

class SerialInterfaceReg
{
protected:
    SerialInterfaceReg(uint32_t * Interface);

public:
    uint32_t & SI_DRAM_ADDR_REG;
    uint32_t & SI_PIF_ADDR_RD64B_REG;
    uint32_t & SI_PIF_ADDR_WR64B_REG;
    uint32_t & SI_STATUS_REG;

private:
    SerialInterfaceReg();
    SerialInterfaceReg(const SerialInterfaceReg &);
    SerialInterfaceReg & operator=(const SerialInterfaceReg &);
};

class CMipsMemoryVM;
class CRegisters;

class SerialInterfaceHandler :
    public MemoryHandler,
    public SerialInterfaceReg,
    private MIPSInterfaceReg,
    private CDebugSettings,
    private CLogging
{
public:
    SerialInterfaceHandler(CMipsMemoryVM & MMU, CRegisters & Reg);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    SerialInterfaceHandler();
    SerialInterfaceHandler(const SerialInterfaceHandler &);
    SerialInterfaceHandler & operator=(const SerialInterfaceHandler &);

    PifRamHandler & m_PifRamHandler;
    CMipsMemoryVM & m_MMU;
    CRegisters & m_Reg;
    uint64_t & m_PC;
};
