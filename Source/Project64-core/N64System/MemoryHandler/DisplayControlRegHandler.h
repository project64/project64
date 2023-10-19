#pragma once
#include "MemoryHandler.h"
#include "SPRegistersHandler.h"
#include <Project64-core\Logging.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <stdint.h>

enum
{
    DPC_CLR_XBUS_DMEM_DMA = 0x0001, // Bit 0: Clear xbus_dmem_dma
    DPC_SET_XBUS_DMEM_DMA = 0x0002, // Bit 1: Set xbus_dmem_dma
    DPC_CLR_FREEZE = 0x0004,        // Bit 2: Clear freeze
    DPC_SET_FREEZE = 0x0008,        // Bit 3: Set freeze
    DPC_CLR_FLUSH = 0x0010,         // Bit 4: Clear flush
    DPC_SET_FLUSH = 0x0020,         // Bit 5: Set flush
    DPC_CLR_TMEM_CTR = 0x0040,      // Bit 6: Clear TMEM CTR
    DPC_CLR_PIPE_CTR = 0x0080,      // Bit 7: Clear pipe CTR
    DPC_CLR_CMD_CTR = 0x0100,       // Bit 8: Clear CMD CTR
    DPC_CLR_CLOCK_CTR = 0x0200,     // Bit 9: Clear clock CTR

    DPC_STATUS_XBUS_DMEM_DMA = 0x001, // Bit  0: xbus_dmem_dma
    DPC_STATUS_FREEZE = 0x002,        // Bit  1: Freeze
    DPC_STATUS_FLUSH = 0x004,         // Bit  2: Flush
    DPC_STATUS_START_GCLK = 0x008,    // Bit  3: Start GCLK
    DPC_STATUS_TMEM_BUSY = 0x010,     // Bit  4: TMEM busy
    DPC_STATUS_PIPE_BUSY = 0x020,     // Bit  5: Pipe busy
    DPC_STATUS_CMD_BUSY = 0x040,      // Bit  6: CMD busy
    DPC_STATUS_CBUF_READY = 0x080,    // Bit  7: CBUF ready
    DPC_STATUS_DMA_BUSY = 0x100,      // Bit  8: DMA busy
    DPC_STATUS_END_VALID = 0x200,     // Bit  9: End valid
    DPC_STATUS_START_VALID = 0x400,   // Bit 10: Start valid
};

class DisplayControlReg
{
protected:
    DisplayControlReg(uint32_t * _DisplayProcessor);

public:
    uint32_t & DPC_START_REG;
    uint32_t & DPC_END_REG;
    uint32_t & DPC_CURRENT_REG;
    uint32_t & DPC_STATUS_REG;
    uint32_t & DPC_CLOCK_REG;
    uint32_t & DPC_BUFBUSY_REG;
    uint32_t & DPC_PIPEBUSY_REG;
    uint32_t & DPC_TMEM_REG;

private:
    DisplayControlReg();
    DisplayControlReg(const DisplayControlReg &);
    DisplayControlReg & operator=(const DisplayControlReg &);
};

class CN64System;
class CPlugins;
class CRegisters;

class DisplayControlRegHandler :
    public MemoryHandler,
    private CDebugSettings,
    private CLogging,
    private DisplayControlReg,
    private SPRegistersReg
{
public:
    DisplayControlRegHandler(CN64System & N64System, CPlugins * Plugins, CRegisters & Reg);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    DisplayControlRegHandler();
    DisplayControlRegHandler(const DisplayControlRegHandler &);
    DisplayControlRegHandler & operator=(const DisplayControlRegHandler &);

    void ProcessRDPList(void);

    CN64System & m_System;
    CPlugins * m_Plugins;
    uint32_t & m_PC;
};