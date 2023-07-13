#include "cpu/RspTypes.h"

#define SP_STATUS_HALT 0x001       // Bit  0: Halt
#define SP_STATUS_BROKE 0x002      // Bit  1: Broke
#define SP_STATUS_DMA_BUSY 0x004   // Bit  2: DMA busy
#define SP_STATUS_DMA_FULL 0x008   // Bit  3: DMA full
#define SP_STATUS_IO_FULL 0x010    // Bit  4: IO full
#define SP_STATUS_SSTEP 0x020      // Bit  5: Single step
#define SP_STATUS_INTR_BREAK 0x040 // Bit  6: Interrupt on break
#define SP_STATUS_SIG0 0x080       // Bit  7: Signal 0 set
#define SP_STATUS_SIG1 0x100       // Bit  8: Signal 1 set
#define SP_STATUS_SIG2 0x200       // Bit  9: Signal 2 set
#define SP_STATUS_SIG3 0x400       // Bit 10: Signal 3 set
#define SP_STATUS_SIG4 0x800       // Bit 11: Signal 4 set
#define SP_STATUS_SIG5 0x1000      // Bit 12: Signal 5 set
#define SP_STATUS_SIG6 0x2000      // Bit 13: Signal 6 set
#define SP_STATUS_SIG7 0x4000      // Bit 14: Signal 7 set

#define SP_CLR_HALT 0x00001       // Bit  0: Clear halt
#define SP_SET_HALT 0x00002       // Bit  1: Set halt
#define SP_CLR_BROKE 0x00004      // Bit  2: Clear broke
#define SP_CLR_INTR 0x00008       // Bit  3: Clear INTR
#define SP_SET_INTR 0x00010       // Bit  4: Set INTR
#define SP_CLR_SSTEP 0x00020      // Bit  5: Clear SSTEP
#define SP_SET_SSTEP 0x00040      // Bit  6: Set SSTEP
#define SP_CLR_INTR_BREAK 0x00080 // Bit  7: Clear INTR on break
#define SP_SET_INTR_BREAK 0x00100 // Bit  8: Set INTR on break
#define SP_CLR_SIG0 0x00200       // Bit  9: Clear signal 0
#define SP_SET_SIG0 0x00400       // Bit 10: Set signal 0
#define SP_CLR_SIG1 0x00800       // Bit 11: Clear signal 1
#define SP_SET_SIG1 0x01000       // Bit 12: Set signal 1
#define SP_CLR_SIG2 0x02000       // Bit 13: Clear signal 2
#define SP_SET_SIG2 0x04000       // Bit 14: Set signal 2
#define SP_CLR_SIG3 0x08000       // Bit 15: Clear signal 3
#define SP_SET_SIG3 0x10000       // Bit 16: Set signal 3
#define SP_CLR_SIG4 0x20000       // Bit 17: Clear signal 4
#define SP_SET_SIG4 0x40000       // Bit 18: Set signal 4
#define SP_CLR_SIG5 0x80000       // Bit 19: Clear signal 5
#define SP_SET_SIG5 0x100000      // Bit 20: Set signal 5
#define SP_CLR_SIG6 0x200000      // Bit 21: Clear signal 6
#define SP_SET_SIG6 0x400000      // Bit 22: Set signal 6
#define SP_CLR_SIG7 0x800000      // Bit 23: Clear signal 7
#define SP_SET_SIG7 0x1000000     // Bit 24: Set signal 7

#define DPC_CLR_XBUS_DMEM_DMA 0x0001 // Bit 0: Clear xbus_dmem_dma
#define DPC_SET_XBUS_DMEM_DMA 0x0002 // Bit 1: Set xbus_dmem_dma
#define DPC_CLR_FREEZE 0x0004        // Bit 2: Clear freeze
#define DPC_SET_FREEZE 0x0008        // Bit 3: Set freeze
#define DPC_CLR_FLUSH 0x0010         // Bit 4: Clear flush
#define DPC_SET_FLUSH 0x0020         // Bit 5: Set flush
#define DPC_CLR_TMEM_CTR 0x0040      // Bit 6: Clear TMEM CTR
#define DPC_CLR_PIPE_CTR 0x0080      // Bit 7: Clear pipe CTR
#define DPC_CLR_CMD_CTR 0x0100       // Bit 8: Clear CMD CTR
#define DPC_CLR_CLOCK_CTR 0x0200     // Bit 9: Clear clock CTR

#define DPC_STATUS_XBUS_DMEM_DMA 0x001 // Bit  0: xbus_dmem_dma
#define DPC_STATUS_FREEZE 0x002        // Bit  1: Freeze
#define DPC_STATUS_FLUSH 0x004         // Bit  2: Flush
#define DPC_STATUS_START_GCLK 0x008    // Bit  3: Start GCLK
#define DPC_STATUS_TMEM_BUSY 0x010     // Bit  4: TMEM busy
#define DPC_STATUS_PIPE_BUSY 0x020     // Bit  5: Pipe busy
#define DPC_STATUS_CMD_BUSY 0x040      // Bit  6: CMD busy
#define DPC_STATUS_CBUF_READY 0x080    // Bit  7: CBUF ready
#define DPC_STATUS_DMA_BUSY 0x100      // Bit  8: DMA busy
#define DPC_STATUS_END_VALID 0x200     // Bit  9: End valid
#define DPC_STATUS_START_VALID 0x400   // Bit 10: Start valid

#define R4300i_SP_Intr 0x1

extern char * x86_Strings[8];
extern char * GPR_Strings[32];

#define x86_Name(Reg) (x86_Strings[(Reg)])
#define GPR_Name(Reg) (GPR_Strings[(Reg)])

/*
#define GPR_Name(Reg)\
	(Reg) == 0  ? "R0" : (Reg) == 1  ? "AT" : (Reg) == 2  ? "V0" : (Reg) == 3  ? "V1" :\
	(Reg) == 4  ? "A0" : (Reg) == 5  ? "A1" : (Reg) == 6  ? "A2" : (Reg) == 7  ? "A3" :\
	(Reg) == 8  ? "T0" : (Reg) == 9  ? "T1" : (Reg) == 10 ? "T2" : (Reg) == 11 ? "T3" :\
	(Reg) == 12 ? "T4" : (Reg) == 13 ? "T5" : (Reg) == 14 ? "T6" : (Reg) == 15 ? "T7" :\
	(Reg) == 16 ? "S0" : (Reg) == 17 ? "S1" : (Reg) == 18 ? "S2" : (Reg) == 19 ? "S3" :\
	(Reg) == 20 ? "S4" : (Reg) == 21 ? "S5" : (Reg) == 22 ? "S6" : (Reg) == 23 ? "S7" :\
	(Reg) == 24 ? "T8" : (Reg) == 25 ? "T9" : (Reg) == 26 ? "K0" : (Reg) == 27 ? "K1" :\
	(Reg) == 28 ? "GP" : (Reg) == 29 ? "SP" : (Reg) == 30 ? "S8" :\
	(Reg) == 31 ? "RA" : "Unknown Register"
*/

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
