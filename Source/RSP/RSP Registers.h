#include "Types.h"

#define SP_STATUS_HALT			0x001		// Bit  0: halt
#define SP_STATUS_BROKE			0x002		// Bit  1: broke
#define SP_STATUS_DMA_BUSY		0x004		// Bit  2: DMA busy
#define SP_STATUS_DMA_FULL		0x008		// Bit  3: DMA full
#define SP_STATUS_IO_FULL		0x010		// Bit  4: IO full
#define SP_STATUS_SSTEP			0x020		// Bit  5: single step
#define SP_STATUS_INTR_BREAK	0x040		// Bit  6: interrupt on break
#define SP_STATUS_SIG0			0x080		// Bit  7: signal 0 set
#define SP_STATUS_SIG1			0x100		// Bit  8: signal 1 set
#define SP_STATUS_SIG2			0x200		// Bit  9: signal 2 set
#define SP_STATUS_SIG3			0x400		// Bit 10: signal 3 set
#define SP_STATUS_SIG4			0x800		// Bit 11: signal 4 set
#define SP_STATUS_SIG5	       0x1000		// Bit 12: signal 5 set
#define SP_STATUS_SIG6	       0x2000		// Bit 13: signal 6 set
#define SP_STATUS_SIG7	       0x4000		// Bit 14: signal 7 set

#define SP_CLR_HALT				0x00001	    // Bit  0: clear halt
#define SP_SET_HALT				0x00002	    // Bit  1: set halt
#define SP_CLR_BROKE			0x00004	    // Bit  2: clear broke
#define SP_CLR_INTR				0x00008	    // Bit  3: clear INTR
#define SP_SET_INTR				0x00010	    // Bit  4: set INTR
#define SP_CLR_SSTEP			0x00020	    // Bit  5: clear SSTEP
#define SP_SET_SSTEP			0x00040	    // Bit  6: set SSTEP
#define SP_CLR_INTR_BREAK		0x00080	    // Bit  7: clear INTR on break
#define SP_SET_INTR_BREAK		0x00100	    // Bit  8: set INTR on break
#define SP_CLR_SIG0				0x00200	    // Bit  9: clear signal 0
#define SP_SET_SIG0				0x00400	    // Bit 10: set signal 0
#define SP_CLR_SIG1				0x00800	    // Bit 11: clear signal 1
#define SP_SET_SIG1				0x01000	    // Bit 12: set signal 1
#define SP_CLR_SIG2				0x02000	    // Bit 13: clear signal 2
#define SP_SET_SIG2				0x04000	    // Bit 14: set signal 2
#define SP_CLR_SIG3				0x08000	    // Bit 15: clear signal 3
#define SP_SET_SIG3				0x10000	    // Bit 16: set signal 3
#define SP_CLR_SIG4				0x20000	    // Bit 17: clear signal 4
#define SP_SET_SIG4				0x40000	    // Bit 18: set signal 4
#define SP_CLR_SIG5				0x80000	    // Bit 19: clear signal 5
#define SP_SET_SIG5				0x100000	// Bit 20: set signal 5
#define SP_CLR_SIG6				0x200000	// Bit 21: clear signal 6
#define SP_SET_SIG6				0x400000	// Bit 22: set signal 6
#define SP_CLR_SIG7				0x800000	// Bit 23: clear signal 7
#define SP_SET_SIG7				0x1000000   // Bit 24: set signal 7

#define DPC_CLR_XBUS_DMEM_DMA	0x0001		// Bit 0: clear xbus_dmem_dma
#define DPC_SET_XBUS_DMEM_DMA	0x0002		// Bit 1: set xbus_dmem_dma
#define DPC_CLR_FREEZE			0x0004		// Bit 2: clear freeze
#define DPC_SET_FREEZE			0x0008		// Bit 3: set freeze
#define DPC_CLR_FLUSH			0x0010		// Bit 4: clear flush
#define DPC_SET_FLUSH			0x0020		// Bit 5: set flush
#define DPC_CLR_TMEM_CTR		0x0040		// Bit 6: clear TMEM CTR
#define DPC_CLR_PIPE_CTR		0x0080		// Bit 7: clear pipe CTR
#define DPC_CLR_CMD_CTR			0x0100		// Bit 8: clear CMD CTR
#define DPC_CLR_CLOCK_CTR		0x0200		// Bit 9: clear clock CTR

#define DPC_STATUS_XBUS_DMEM_DMA	0x001	// Bit  0: xbus_dmem_dma
#define DPC_STATUS_FREEZE			0x002	// Bit  1: freeze
#define DPC_STATUS_FLUSH			0x004	// Bit  2: flush
#define DPC_STATUS_START_GCLK		0x008	// Bit  3: start GCLK
#define DPC_STATUS_TMEM_BUSY		0x010	// Bit  4: TMEM busy
#define DPC_STATUS_PIPE_BUSY		0x020	// Bit  5: pipe busy
#define DPC_STATUS_CMD_BUSY			0x040	// Bit  6: CMD busy
#define DPC_STATUS_CBUF_READY		0x080	// Bit  7: CBUF ready
#define DPC_STATUS_DMA_BUSY			0x100	// Bit  8: DMA busy
#define DPC_STATUS_END_VALID		0x200	// Bit  9: end valid
#define DPC_STATUS_START_VALID		0x400	// Bit 10: start valid

#define R4300i_SP_Intr			0x1

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

#define COP0_Name(Reg)\
	(Reg) == 0  ? "SP memory address" :\
	(Reg) == 1  ? "SP DRAM DMA address" :\
	(Reg) == 2  ? "SP read DMA length" :\
	(Reg) == 3  ? "SP write DMA length" :\
	(Reg) == 4  ? "SP status" :\
	(Reg) == 5  ? "SP DMA full" :\
	(Reg) == 6  ? "SP DMA busy" :\
	(Reg) == 7  ? "SP semaphore" :\
	(Reg) == 8  ? "DP CMD DMA start" :\
	(Reg) == 9  ? "DP CMD DMA end" :\
	(Reg) == 10 ? "DP CMD DMA current" :\
	(Reg) == 11 ? "DP CMD status" :\
	(Reg) == 12 ? "DP clock counter" :\
	(Reg) == 13 ? "DP buffer busy counter" :\
	(Reg) == 14 ? "DP pipe busy counter" :\
	(Reg) == 15 ? "DP TMEM load counter" :\
	"Unknown Register"

#define ElementSpecifier(Elem)\
	(Elem) == 0  ? "" : (Elem) == 1  ? "" : (Elem) == 2  ? " [0q]" :\
	(Elem) == 3  ? " [1q]" : (Elem) == 4  ? " [0h]" : (Elem) == 5  ? " [1h]" :\
    (Elem) == 6  ? " [2h]" : (Elem) == 7  ? " [3h]" : (Elem) == 8  ? " [0]" :\
	(Elem) == 9  ? " [1]" : (Elem) == 10 ? " [2]" : (Elem) == 11 ? " [3]" :\
	(Elem) == 12 ? " [4]" : (Elem) == 13 ? " [5]" : (Elem) == 14 ? " [6]" :\
	(Elem) == 15 ? " [7]" : "Unknown Element"

void Enter_RSP_Register_Window ( void );
void InitilizeRSPRegisters (void);
void UpdateRSPRegistersScreen ( void );

// RSP registers
extern UWORD32   RSP_GPR[32], RSP_Flags[4];
extern UDWORD  RSP_ACCUM[8];
extern VECTOR  RSP_Vect[32];
