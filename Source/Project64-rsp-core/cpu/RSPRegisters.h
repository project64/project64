#pragma once
#include "RspTypes.h"

enum
{
    SP_STATUS_HALT = 0x001,       // Bit  0: Halt
    SP_STATUS_BROKE = 0x002,      // Bit  1: Broke
    SP_STATUS_DMA_BUSY = 0x004,   // Bit  2: DMA busy
    SP_STATUS_DMA_FULL = 0x008,   // Bit  3: DMA full
    SP_STATUS_IO_FULL = 0x010,    // Bit  4: IO full
    SP_STATUS_SSTEP = 0x020,      // Bit  5: Single step
    SP_STATUS_INTR_BREAK = 0x040, // Bit  6: Interrupt on break
    SP_STATUS_SIG0 = 0x080,       // Bit  7: Signal 0 set
    SP_STATUS_SIG1 = 0x100,       // Bit  8: Signal 1 set
    SP_STATUS_SIG2 = 0x200,       // Bit  9: Signal 2 set
    SP_STATUS_SIG3 = 0x400,       // Bit 10: Signal 3 set
    SP_STATUS_SIG4 = 0x800,       // Bit 11: Signal 4 set
    SP_STATUS_SIG5 = 0x1000,      // Bit 12: Signal 5 set
    SP_STATUS_SIG6 = 0x2000,      // Bit 13: Signal 6 set
    SP_STATUS_SIG7 = 0x4000,      // Bit 14: Signal 7 set

    SP_CLR_HALT = 0x00001,       // Bit  0: Clear halt
    SP_SET_HALT = 0x00002,       // Bit  1: Set halt
    SP_CLR_BROKE = 0x00004,      // Bit  2: Clear broke
    SP_CLR_INTR = 0x00008,       // Bit  3: Clear INTR
    SP_SET_INTR = 0x00010,       // Bit  4: Set INTR
    SP_CLR_SSTEP = 0x00020,      // Bit  5: Clear SSTEP
    SP_SET_SSTEP = 0x00040,      // Bit  6: Set SSTEP
    SP_CLR_INTR_BREAK = 0x00080, // Bit  7: Clear INTR on break
    SP_SET_INTR_BREAK = 0x00100, // Bit  8: Set INTR on break
    SP_CLR_SIG0 = 0x00200,       // Bit  9: Clear signal 0
    SP_SET_SIG0 = 0x00400,       // Bit 10: Set signal 0
    SP_CLR_SIG1 = 0x00800,       // Bit 11: Clear signal 1
    SP_SET_SIG1 = 0x01000,       // Bit 12: Set signal 1
    SP_CLR_SIG2 = 0x02000,       // Bit 13: Clear signal 2
    SP_SET_SIG2 = 0x04000,       // Bit 14: Set signal 2
    SP_CLR_SIG3 = 0x08000,       // Bit 15: Clear signal 3
    SP_SET_SIG3 = 0x10000,       // Bit 16: Set signal 3
    SP_CLR_SIG4 = 0x20000,       // Bit 17: Clear signal 4
    SP_SET_SIG4 = 0x40000,       // Bit 18: Set signal 4
    SP_CLR_SIG5 = 0x80000,       // Bit 19: Clear signal 5
    SP_SET_SIG5 = 0x100000,      // Bit 20: Set signal 5
    SP_CLR_SIG6 = 0x200000,      // Bit 21: Clear signal 6
    SP_SET_SIG6 = 0x400000,      // Bit 22: Set signal 6
    SP_CLR_SIG7 = 0x800000,      // Bit 23: Clear signal 7
    SP_SET_SIG7 = 0x1000000,     // Bit 24: Set signal 7

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

    MI_INTR_SP = 0x01, // Bit 0: SP INTR
};

extern char * x86_Strings[8];
extern char * GPR_Strings[32];

#define x86_Name(Reg) (x86_Strings[(Reg)])
#define GPR_Name(Reg) (GPR_Strings[(Reg)])

#define COP0_Name(Reg)                                                       \
    (Reg) == 0 ? "SP memory address" : (Reg) == 1 ? "SP DRAM DMA address"    \
                                   : (Reg) == 2   ? "SP read DMA length"     \
                                   : (Reg) == 3   ? "SP write DMA length"    \
                                   : (Reg) == 4   ? "SP status"              \
                                   : (Reg) == 5   ? "SP DMA full"            \
                                   : (Reg) == 6   ? "SP DMA busy"            \
                                   : (Reg) == 7   ? "SP semaphore"           \
                                   : (Reg) == 8   ? "DP CMD DMA start"       \
                                   : (Reg) == 9   ? "DP CMD DMA end"         \
                                   : (Reg) == 10  ? "DP CMD DMA current"     \
                                   : (Reg) == 11  ? "DP CMD status"          \
                                   : (Reg) == 12  ? "DP clock counter"       \
                                   : (Reg) == 13  ? "DP buffer busy counter" \
                                   : (Reg) == 14  ? "DP pipe busy counter"   \
                                   : (Reg) == 15  ? "DP TMEM load counter"   \
                                                  : "Unknown Register"

void Enter_RSP_Register_Window(void);
void InitilizeRSPRegisters(void);
void UpdateRSPRegistersScreen(void);

// RSP registers
extern UWORD32 RSP_GPR[32], RSP_Flags[4];
extern UDWORD RSP_ACCUM[8];
extern RSPVector RSP_Vect[32];

extern RSPFlag VCOL, VCOH;
extern RSPFlag VCCL, VCCH;
extern RSPFlag VCE;