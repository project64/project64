#pragma once
#include "MemoryHandler.h"
#include <Project64-core/Logging.h>
#include <Project64-core/N64System/MemoryHandler/MIPSInterfaceHandler.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Settings/GameSettings.h>
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
    PeripheralInterfaceReg(const PeripheralInterfaceReg &);
    PeripheralInterfaceReg & operator=(const PeripheralInterfaceReg &);
};

class CN64System;
class CRegisters;
class CMipsMemoryVM;
class CartridgeDomain2Address2Handler;

// Peripheral interface flags
enum
{
    PI_STATUS_DMA_BUSY = 0x01,
    PI_STATUS_IO_BUSY = 0x02,
    PI_STATUS_ERROR = 0x04,
    PI_STATUS_INTERRUPT = 0x08,

    PI_SET_RESET = 0x01,
    PI_CLR_INTR = 0x02,
};

class PeripheralInterfaceHandler :
    public MemoryHandler,
    private CGameSettings,
    private CDebugSettings,
    private CLogging,
    private PeripheralInterfaceReg,
    private MIPSInterfaceReg
{
public:
    PeripheralInterfaceHandler(CN64System & System, CMipsMemoryVM & MMU, CRegisters & Reg, CartridgeDomain2Address2Handler & Domain2Address2Handler);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    PeripheralInterfaceHandler();
    PeripheralInterfaceHandler(const PeripheralInterfaceHandler &);
    PeripheralInterfaceHandler & operator=(const PeripheralInterfaceHandler &);

    static void stSystemReset(PeripheralInterfaceHandler * _this)
    {
        _this->SystemReset();
    }
    static void stLoadedGameState(PeripheralInterfaceHandler * _this)
    {
        _this->LoadedGameState();
    }

    void PI_DMA_READ();
    void PI_DMA_WRITE();
    void LoadedGameState(void);
    void SystemReset(void);
    void OnFirstDMA();
    void ReadBlock(uint32_t Address, uint8_t * Block, uint32_t BlockLen);

    CartridgeDomain2Address2Handler & m_Domain2Address2Handler;
    CMipsMemoryVM & m_MMU;
    CRegisters & m_Reg;
    uint32_t & m_PC;

    bool m_DMAUsed;
};
