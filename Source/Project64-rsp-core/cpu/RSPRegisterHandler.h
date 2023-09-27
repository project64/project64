#pragma once
#include <stdint.h>

enum RSPRegister
{
    RSPRegister_MEM_ADDR,
    RSPRegister_DRAM_ADDR,
    RSPRegister_RD_LEN,
    RSPRegister_WR_LEN,
    RSPRegister_STATUS,
    RSPRegister_SEMAPHORE,
    RSPRegister_PC,
};

struct _RSP_INFO;

class RSPRegisterHandler
{
public:
    RSPRegisterHandler(uint32_t * SignalProcessorInterface, uint8_t *& Rdram, const uint32_t & RdramSize, uint8_t * IMEM, uint8_t * DMEM);

    void SP_DMA_READ(void);
    void SP_DMA_WRITE(void);

    uint32_t ReadReg(RSPRegister Reg);
    void WriteReg(RSPRegister Reg, uint32_t Value);

protected:
    virtual void ClearSPInterrupt(void) = 0;
    virtual void SetSPInterrupt(void) = 0;
    virtual void SetHalt(void) = 0;

    uint32_t & SP_MEM_ADDR_REG;
    uint32_t & SP_DRAM_ADDR_REG;
    uint32_t & SP_RD_LEN_REG;
    uint32_t & SP_WR_LEN_REG;
    uint32_t & SP_STATUS_REG;
    uint32_t & SP_DMA_FULL_REG;
    uint32_t & SP_DMA_BUSY_REG;
    uint32_t & SP_SEMAPHORE_REG;
    uint32_t & SP_PC_REG;
    uint8_t *& m_Rdram;
    const uint32_t & m_RdramSize;
    uint8_t * m_IMEM;
    uint8_t * m_DMEM;
    uint32_t m_PendingSPMemAddr;
    uint32_t m_PendingSPDramAddr;
    bool m_ExecutedDMARead;
};
