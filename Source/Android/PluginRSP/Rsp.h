#pragma once

#include "Common.h"

typedef struct
{
    void * hInst;
    int32_t MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
                              bswap on a dword (32 bits) boundry */
    uint8_t * RDRAM;
    uint8_t * DMEM;
    uint8_t * IMEM;

    uint32_t * MI_INTR_REG;

    uint32_t * SP_MEM_ADDR_REG;
    uint32_t * SP_DRAM_ADDR_REG;
    uint32_t * SP_RD_LEN_REG;
    uint32_t * SP_WR_LEN_REG;
    uint32_t * SP_STATUS_REG;
    uint32_t * SP_DMA_FULL_REG;
    uint32_t * SP_DMA_BUSY_REG;
    uint32_t * SP_PC_REG;
    uint32_t * SP_SEMAPHORE_REG;

    uint32_t * DPC_START_REG;
    uint32_t * DPC_END_REG;
    uint32_t * DPC_CURRENT_REG;
    uint32_t * DPC_STATUS_REG;
    uint32_t * DPC_CLOCK_REG;
    uint32_t * DPC_BUFBUSY_REG;
    uint32_t * DPC_PIPEBUSY_REG;
    uint32_t * DPC_TMEM_REG;

    void(*CheckInterrupts)(void);
    void(*ProcessDList)(void);
    void(*ProcessAList)(void);
    void(*ProcessRdpList)(void);
    void(*ShowCFB)(void);
} RSP_INFO;

EXPORT void CloseDLL(void);
EXPORT void DllAbout(void * hParent);
EXPORT uint32_t DoRspCycles(uint32_t Cycles);
EXPORT void GetDllInfo(PLUGIN_INFO * PluginInfo);
EXPORT void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * CycleCount);
EXPORT void RomOpen(void);
EXPORT void RomClosed(void);
EXPORT void DllConfig(void * hWnd);
EXPORT void PluginLoaded(void);
