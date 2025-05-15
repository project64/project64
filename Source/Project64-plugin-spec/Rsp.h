#pragma once
#include "Base.h"

typedef struct _RSP_INFO
{
    void * hInst;
    int MemoryBswaped;    // If this is set to TRUE, then the memory has been pre-bswap'd on a DWORD (32-bit) boundary
    uint8_t * HEADER;
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

typedef struct {
    void(*UpdateBreakPoints)(void);
    void(*UpdateMemory)(void);
    void(*UpdateR4300iRegisters)(void);
    void(*Enter_BPoint_Window)(void);
    void(*Enter_R4300i_Commands_Window)(void);
    void(*Enter_R4300i_Register_Window)(void);
    void(*Enter_RSP_Commands_Window) (void);
    void(*Enter_Memory_Window)(void);
} DEBUG_INFO;

typedef struct {
    long left, top, right, bottom;
} rectangle; // <windows.h> equivalent: RECT

typedef struct 
{
    void * hdc;
    int32_t fErase;
    rectangle rcPaint;
    int32_t fRestore;
    int32_t fIncUpdate;
    uint8_t rgbReserved[32];
} window_paint; // <windows.h> equivalent: PAINTSTRUCT

typedef struct 
{
    // Menu
    // Items should have an ID between 5001 and 5100
    void * hRSPMenu;
    void(*ProcessMenuItem) (int32_t ID);

    // Breakpoints
    int UseBPoints;
    char BPPanelName[20];
    void(*Add_BPoint)(void);
    void(*CreateBPPanel)(void * hDlg, rectangle rcBox);
    void(*HideBPPanel)(void);
    void(*PaintBPPanel)(window_paint ps);
    void(*ShowBPPanel)(void);
    void(*RefreshBpoints)(void * hList);
    void(*RemoveBpoint)(void * hList, int index);
    void(*RemoveAllBpoint)(void);

    // RSP command window
    void(*Enter_RSP_Commands_Window) (void);
} RSPDEBUG_INFO;

EXPORT uint32_t DoRspCycles(uint32_t Cycles);
EXPORT void GetRspDebugInfo(RSPDEBUG_INFO * DebugInfo);
EXPORT void InitiateRSP(RSP_INFO Rsp_Info, uint32_t * CycleCount);
EXPORT void InitiateRSPDebugger(DEBUG_INFO Debug_Info);
EXPORT void EnableDebugging(int Enabled);
