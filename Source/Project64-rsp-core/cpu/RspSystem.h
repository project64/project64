#pragma once
#include <Project64-rsp-core/RSPInfo.h>
#include <Project64-rsp-core/cpu/RSPInterpreterOps.h>
#include <Project64-rsp-core/cpu/RSPRegisters.h>
#include <Project64-rsp-core/cpu/RspPipelineStage.h>
#include <Project64-rsp-core/cpu/RspTypes.h>
#include <stdint.h>

class RSPRegisterHandlerPlugin;

class CRSPSystem
{
    friend class RSPOp;
    friend class CRSPRecompilerOps;
    friend class CRSPRecompiler;
    friend class RSPDebuggerUI;
    friend class CRDPLog;
    friend class RSPRegisterHandler;
    friend class RSPRegisterHandlerPlugin;

    friend void UpdateRSPRegistersScreen(void);

public:
    CRSPSystem();
    ~CRSPSystem();

    void Reset(RSP_INFO & Info);
    void RomClosed(void);

    void RunRecompiler(void);
    uint32_t RunInterpreterCPU(uint32_t Cycles);

private:
    CRSPSystem(const CRSPSystem &);
    CRSPSystem & operator=(const CRSPSystem &);

    CRSPRecompiler * m_Recompiler;
    RSPRegisterHandlerPlugin * m_RSPRegisterHandler;
    CRSPRegisters m_Reg;
    RSPOp m_Op;
    RSPOpcode m_OpCode;
    RSPPIPELINE_STAGE m_NextInstruction;
    uint32_t m_JumpTo;
    uint8_t * m_HEADER;
    uint8_t * m_RDRAM;
    uint8_t * m_DMEM;
    uint8_t * m_IMEM;
    uint32_t * m_MI_INTR_REG;
    uint32_t * m_SP_MEM_ADDR_REG;
    uint32_t * m_SP_DRAM_ADDR_REG;
    uint32_t * m_SP_RD_LEN_REG;
    uint32_t * m_SP_WR_LEN_REG;
    uint32_t * m_SP_STATUS_REG;
    uint32_t * m_SP_DMA_FULL_REG;
    uint32_t * m_SP_DMA_BUSY_REG;
    uint32_t * m_SP_PC_REG;
    uint32_t * m_SP_SEMAPHORE_REG;
    uint32_t * m_DPC_START_REG;
    uint32_t * m_DPC_END_REG;
    uint32_t * m_DPC_CURRENT_REG;
    uint32_t * m_DPC_STATUS_REG;
    uint32_t * m_DPC_CLOCK_REG;
    uint32_t * m_DPC_BUFBUSY_REG;
    uint32_t * m_DPC_PIPEBUSY_REG;
    uint32_t * m_DPC_TMEM_REG;
    uint32_t m_RdramSize;
    void (*CheckInterrupts)(void);
    void (*ProcessRdpList)(void);
};

extern CRSPSystem RSPSystem;