#ifndef __REGISTER_CLASS__H__
#define __REGISTER_CLASS__H__

#include "System Timing.h" //base class

enum ROUNDING_MODE {
	ROUND_NEAR = 0x00000000, 
	ROUND_CHOP = 0x00000300, 
	ROUND_UP   = 0x00000200, 
	ROUND_DOWN = 0x00000100,	
};

//registers general come from a mapping from the memory class to a pointer
//inside Classes. To make the code cleaner with out using global variables 
//we just use this pointer.

//CPO registers by name
class CP0registers
{
protected:
	CP0registers (DWORD * _CP0);

public:
	DWORD & INDEX_REGISTER;
	DWORD & RANDOM_REGISTER;
	DWORD & ENTRYLO0_REGISTER;
	DWORD & ENTRYLO1_REGISTER;
	DWORD & CONTEXT_REGISTER;
	DWORD & PAGE_MASK_REGISTER;
	DWORD & WIRED_REGISTER;
	DWORD & BAD_VADDR_REGISTER;
	DWORD & COUNT_REGISTER;
	DWORD & ENTRYHI_REGISTER;
	DWORD & COMPARE_REGISTER;
	DWORD & STATUS_REGISTER;
	DWORD & CAUSE_REGISTER;
	DWORD & EPC_REGISTER;
	DWORD & CONFIG_REGISTER;
	DWORD & TAGLO_REGISTER;
	DWORD & TAGHI_REGISTER;
	DWORD & ERROREPC_REGISTER;
	DWORD & FAKE_CAUSE_REGISTER;
};

//CPO register flags
enum {
	//Status Register
	STATUS_IE  = 0x00000001, STATUS_EXL = 0x00000002, STATUS_ERL = 0x00000004,
	STATUS_IP0 = 0x00000100, STATUS_IP1 = 0x00000200, STATUS_IP2 = 0x00000400,
	STATUS_IP3 = 0x00000800, STATUS_IP4 = 0x00001000, STATUS_IP5 = 0x00002000,
	STATUS_IP6 = 0x00004000, STATUS_IP7 = 0x00008000, STATUS_BEV = 0x00400000,
	STATUS_FR  = 0x04000000, STATUS_CU0 = 0x10000000, STATUS_CU1 = 0x20000000,

	//Cause Flags
	CAUSE_EXC_CODE		=	0xFF,
	CAUSE_IP0			=	0x100,
	CAUSE_IP1			=	0x200,
	CAUSE_IP2			=	0x400,
	CAUSE_IP3			=	0x800,
	CAUSE_IP4			=	0x1000,
	CAUSE_IP5			=	0x2000,
	CAUSE_IP6			=	0x4000,
	CAUSE_IP7			=	0x8000,
	CAUSE_BD			=	0x80000000,

	//Cause exception ID's
 	EXC_INT				= 0,	/* interrupt */
 	EXC_MOD				= 4,	/* TLB mod */
 	EXC_RMISS			= 8,	/* Read TLB Miss */
 	EXC_WMISS			= 12,	/* Write TLB Miss */
 	EXC_RADE			= 16,	/* Read Address Error */
 	EXC_WADE			= 20,	/* Write Address Error */
 	EXC_IBE				= 24,	/* Instruction Bus Error */
 	EXC_DBE				= 28,	/* Data Bus Error */
 	EXC_SYSCALL			= 32,	/* SYSCALL */
 	EXC_BREAK			= 36,	/* BREAKpoint */
 	EXC_II				= 40, /* Illegal Instruction */
 	EXC_CPU				= 44, /* CoProcessor Unusable */
 	EXC_OV				= 48, /* OVerflow */
 	EXC_TRAP			= 52, /* Trap exception */
 	EXC_VCEI			= 56, /* Virt. Coherency on Inst. fetch */
 	EXC_FPE				= 60, /* Floating Point Exception */
 	EXC_WATCH			= 92, /* Watchpoint reference */
 	EXC_VCED			= 124,/* Virt. Coherency on data read */
};

//Float point control status register flags
enum {
	FPCSR_FS			= 0x01000000, /* flush denorm to zero */
	FPCSR_C				= 0x00800000, /* condition bit */	
	FPCSR_CE			= 0x00020000, /* cause: unimplemented operation */
	FPCSR_CV			= 0x00010000, /* cause: invalid operation */
	FPCSR_CZ			= 0x00008000, /* cause: division by zero */
	FPCSR_CO			= 0x00004000, /* cause: overflow */
	FPCSR_CU			= 0x00002000, /* cause: underflow */
	FPCSR_CI			= 0x00001000, /* cause: inexact operation */
	FPCSR_EV			= 0x00000800, /* enable: invalid operation */
	FPCSR_EZ			= 0x00000400, /* enable: division by zero */
	FPCSR_EO			= 0x00000200, /* enable: overflow */
	FPCSR_EU			= 0x00000100, /* enable: underflow */
	FPCSR_EI			= 0x00000080, /* enable: inexact operation */
	FPCSR_FV			= 0x00000040, /* flag: invalid operation */
	FPCSR_FZ			= 0x00000020, /* flag: division by zero */
	FPCSR_FO			= 0x00000010, /* flag: overflow */
	FPCSR_FU			= 0x00000008, /* flag: underflow */
	FPCSR_FI			= 0x00000004, /* flag: inexact operation */
	FPCSR_RM_MASK		= 0x00000003, /* rounding mode mask */
	FPCSR_RM_RN			= 0x00000000, /* round to nearest */
	FPCSR_RM_RZ			= 0x00000001, /* round to zero */
	FPCSR_RM_RP			= 0x00000002, /* round to positive infinity */
	FPCSR_RM_RM			= 0x00000003, /* round to negative infinity */
};

/*
//Rdram Interface Registers
#define RDRAM_Interface			_Reg->_RDRAMInterface
#define RI_MODE_REG				_Reg->_RDRAMInterface[0]
#define RI_CONFIG_REG			_Reg->_RDRAMInterface[1]
#define RI_CURRENT_LOAD_REG		_Reg->_RDRAMInterface[2]
#define RI_SELECT_REG			_Reg->_RDRAMInterface[3]
#define RI_COUNT_REG			_Reg->_RDRAMInterface[4]
#define RI_REFRESH_REG			_Reg->_RDRAMInterface[4]
#define RI_LATENCY_REG			_Reg->_RDRAMInterface[5]
#define RI_RERROR_REG			_Reg->_RDRAMInterface[6]
#define RI_WERROR_REG			_Reg->_RDRAMInterface[7]

//Rdram registers
#define RDRAM_Registers			_Reg->_RDRAMRegisters
#define RDRAM_CONFIG_REG		_Reg->_RDRAMRegisters[0]
#define RDRAM_DEVICE_TYPE_REG	_Reg->_RDRAMRegisters[0]
#define RDRAM_DEVICE_ID_REG		_Reg->_RDRAMRegisters[1]
#define RDRAM_DELAY_REG			_Reg->_RDRAMRegisters[2]
#define RDRAM_MODE_REG			_Reg->_RDRAMRegisters[3]
#define RDRAM_REF_INTERVAL_REG	_Reg->_RDRAMRegisters[4]
#define RDRAM_REF_ROW_REG		_Reg->_RDRAMRegisters[5]
#define RDRAM_RAS_INTERVAL_REG	_Reg->_RDRAMRegisters[6]
#define RDRAM_MIN_INTERVAL_REG	_Reg->_RDRAMRegisters[7]
#define RDRAM_ADDR_SELECT_REG	_Reg->_RDRAMRegisters[8]
#define RDRAM_DEVICE_MANUF_REG	_Reg->_RDRAMRegisters[9]
*/
//Mips interface registers
class Mips_InterfaceReg
{
protected:
	Mips_InterfaceReg (DWORD * _MipsInterface);

public:
	DWORD & MI_INIT_MODE_REG;
	DWORD & MI_MODE_REG;
	DWORD & MI_VERSION_REG;
	DWORD & MI_NOOP_REG;
	DWORD & MI_INTR_REG;
	DWORD & MI_INTR_MASK_REG;
};


//Mips interface flags
enum {
	MI_CLR_INIT				= 0x0080,		/* Bit  7: clear init mode */
	MI_SET_INIT				= 0x0100,		/* Bit  8: set init mode */
	MI_CLR_EBUS				= 0x0200,		/* Bit  9: clear ebus test */
	MI_SET_EBUS				= 0x0400,		/* Bit 10: set ebus test mode */
	MI_CLR_DP_INTR			= 0x0800,		/* Bit 11: clear dp interrupt */
	MI_CLR_RDRAM			= 0x1000,		/* Bit 12: clear RDRAM reg */
	MI_SET_RDRAM			= 0x2000,		/* Bit 13: set RDRAM reg mode */

	//Flags for writing to MI_INTR_MASK_REG
	MI_INTR_MASK_CLR_SP		= 0x0001,		/* Bit  0: clear SP mask */
	MI_INTR_MASK_SET_SP		= 0x0002,		/* Bit  1: set SP mask */
	MI_INTR_MASK_CLR_SI		= 0x0004,		/* Bit  2: clear SI mask */
	MI_INTR_MASK_SET_SI		= 0x0008,		/* Bit  3: set SI mask */
	MI_INTR_MASK_CLR_AI		= 0x0010,		/* Bit  4: clear AI mask */
	MI_INTR_MASK_SET_AI		= 0x0020,		/* Bit  5: set AI mask */
	MI_INTR_MASK_CLR_VI		= 0x0040,		/* Bit  6: clear VI mask */
	MI_INTR_MASK_SET_VI		= 0x0080,		/* Bit  7: set VI mask */
	MI_INTR_MASK_CLR_PI		= 0x0100,		/* Bit  8: clear PI mask */
	MI_INTR_MASK_SET_PI		= 0x0200,		/* Bit  9: set PI mask */
	MI_INTR_MASK_CLR_DP		= 0x0400,		/* Bit 10: clear DP mask */
	MI_INTR_MASK_SET_DP		= 0x0800,		/* Bit 11: set DP mask */

	//Flags for reading from MI_INTR_MASK_REG
	MI_INTR_MASK_SP			= 0x01,		/* Bit 0: SP intr mask */
	MI_INTR_MASK_SI			= 0x02,		/* Bit 1: SI intr mask */
	MI_INTR_MASK_AI			= 0x04,		/* Bit 2: AI intr mask */
	MI_INTR_MASK_VI			= 0x08,		/* Bit 3: VI intr mask */
	MI_INTR_MASK_PI			= 0x10,		/* Bit 4: PI intr mask */
	MI_INTR_MASK_DP			= 0x20,		/* Bit 5: DP intr mask */

	MI_INTR_SP				= 0x01,		/* Bit 0: SP intr */
	MI_INTR_SI				= 0x02,		/* Bit 1: SI intr */
	MI_INTR_AI				= 0x04,		/* Bit 2: AI intr */
	MI_INTR_VI				= 0x08,		/* Bit 3: VI intr */
	MI_INTR_PI				= 0x10,		/* Bit 4: PI intr */
	MI_INTR_DP				= 0x20,		/* Bit 5: DP intr */
};

//Mips interface registers
class Video_InterfaceReg
{
protected:
	Video_InterfaceReg (DWORD * _VideoInterface);

public:
	DWORD & VI_STATUS_REG;
	DWORD & VI_CONTROL_REG;
	DWORD & VI_ORIGIN_REG;
	DWORD & VI_DRAM_ADDR_REG;
	DWORD & VI_WIDTH_REG;
	DWORD & VI_H_WIDTH_REG;
	DWORD & VI_INTR_REG;
	DWORD & VI_V_INTR_REG;
	DWORD & VI_CURRENT_REG;
	DWORD & VI_V_CURRENT_LINE_REG;
	DWORD & VI_BURST_REG;
	DWORD & VI_TIMING_REG;
	DWORD & VI_V_SYNC_REG;
	DWORD & VI_H_SYNC_REG;
	DWORD & VI_LEAP_REG;
	DWORD & VI_H_SYNC_LEAP_REG;
	DWORD & VI_H_START_REG;
	DWORD & VI_H_VIDEO_REG;
	DWORD & VI_V_START_REG;
	DWORD & VI_V_VIDEO_REG;
	DWORD & VI_V_BURST_REG;
	DWORD & VI_X_SCALE_REG;
	DWORD & VI_Y_SCALE_REG;
};

//Display Processor Control Registers
class DisplayControlReg
{
protected:
	DisplayControlReg (DWORD * _DisplayProcessor);

public:
	DWORD & DPC_START_REG;
	DWORD & DPC_END_REG;
	DWORD & DPC_CURRENT_REG;
	DWORD & DPC_STATUS_REG;
	DWORD & DPC_CLOCK_REG;
	DWORD & DPC_BUFBUSY_REG;
	DWORD & DPC_PIPEBUSY_REG;
	DWORD & DPC_TMEM_REG;
};

/*#define DisplayControlReg		_Reg->_DisplayProcessor
*/

enum {
	DPC_CLR_XBUS_DMEM_DMA	    = 0x0001,	/* Bit 0: clear xbus_dmem_dma */
	DPC_SET_XBUS_DMEM_DMA	    = 0x0002,	/* Bit 1: set xbus_dmem_dma */
	DPC_CLR_FREEZE			    = 0x0004,	/* Bit 2: clear freeze */
	DPC_SET_FREEZE			    = 0x0008,	/* Bit 3: set freeze */
	DPC_CLR_FLUSH			    = 0x0010,	/* Bit 4: clear flush */
	DPC_SET_FLUSH			    = 0x0020,	/* Bit 5: set flush */
	DPC_CLR_TMEM_CTR		    = 0x0040,	/* Bit 6: clear tmem ctr */
	DPC_CLR_PIPE_CTR		    = 0x0080,	/* Bit 7: clear pipe ctr */
	DPC_CLR_CMD_CTR			    = 0x0100,	/* Bit 8: clear cmd ctr */
	DPC_CLR_CLOCK_CTR		    = 0x0200,	/* Bit 9: clear clock ctr */
	
	DPC_STATUS_XBUS_DMEM_DMA    = 0x001,	/* Bit  0: xbus_dmem_dma */
	DPC_STATUS_FREEZE			= 0x002,	/* Bit  1: freeze */
	DPC_STATUS_FLUSH			= 0x004,	/* Bit  2: flush */
	DPC_STATUS_START_GCLK		= 0x008,	/* Bit  3: start gclk */
	DPC_STATUS_TMEM_BUSY		= 0x010,	/* Bit  4: tmem busy */
	DPC_STATUS_PIPE_BUSY		= 0x020,	/* Bit  5: pipe busy */
	DPC_STATUS_CMD_BUSY			= 0x040,	/* Bit  6: cmd busy */
	DPC_STATUS_CBUF_READY		= 0x080,	/* Bit  7: cbuf ready */
	DPC_STATUS_DMA_BUSY			= 0x100,	/* Bit  8: dma busy */
	DPC_STATUS_END_VALID		= 0x200,	/* Bit  9: end valid */
	DPC_STATUS_START_VALID		= 0x400,	/* Bit 10: start valid */
};

/*
//Audio Interface registers;
*/
class AudioInterfaceReg
{
protected:
	AudioInterfaceReg (DWORD * _AudioInterface);

public:
	DWORD & AI_DRAM_ADDR_REG;
	DWORD & AI_LEN_REG;
	DWORD & AI_CONTROL_REG;
	DWORD & AI_STATUS_REG;
	DWORD & AI_DACRATE_REG;
	DWORD & AI_BITRATE_REG;
};

enum {
	AI_STATUS_FIFO_FULL			= 0x80000000,	/* Bit 31: full */
	AI_STATUS_DMA_BUSY			= 0x40000000,	/* Bit 30: busy */
};

//Signal Processor Interface;
class SigProcessor_InterfaceReg
{
protected:
	SigProcessor_InterfaceReg (DWORD * _SignalProcessorInterface);

public:
	DWORD & SP_MEM_ADDR_REG;
	DWORD & SP_DRAM_ADDR_REG;
	DWORD & SP_RD_LEN_REG;
	DWORD & SP_WR_LEN_REG;
	DWORD & SP_STATUS_REG;
	DWORD & SP_DMA_FULL_REG;
	DWORD & SP_DMA_BUSY_REG;
	DWORD & SP_SEMAPHORE_REG;
	DWORD & SP_PC_REG;
	DWORD & SP_IBIST_REG;
};

//Signal Processor interface flags
enum {
	SP_CLR_HALT				= 0x00001,	    /* Bit  0: clear halt */
	SP_SET_HALT				= 0x00002,	    /* Bit  1: set halt */
	SP_CLR_BROKE			= 0x00004,	    /* Bit  2: clear broke */
	SP_CLR_INTR				= 0x00008,	    /* Bit  3: clear intr */
	SP_SET_INTR				= 0x00010,	    /* Bit  4: set intr */
	SP_CLR_SSTEP			= 0x00020,	    /* Bit  5: clear sstep */
	SP_SET_SSTEP			= 0x00040,	    /* Bit  6: set sstep */
	SP_CLR_INTR_BREAK		= 0x00080,	    /* Bit  7: clear intr on break */
	SP_SET_INTR_BREAK		= 0x00100,	    /* Bit  8: set intr on break */
	SP_CLR_SIG0				= 0x00200,	    /* Bit  9: clear signal 0 */
	SP_SET_SIG0				= 0x00400,	    /* Bit 10: set signal 0 */
	SP_CLR_SIG1				= 0x00800,	    /* Bit 11: clear signal 1 */
	SP_SET_SIG1				= 0x01000,	    /* Bit 12: set signal 1 */
	SP_CLR_SIG2				= 0x02000,	    /* Bit 13: clear signal 2 */
	SP_SET_SIG2				= 0x04000,	    /* Bit 14: set signal 2 */
	SP_CLR_SIG3				= 0x08000,	    /* Bit 15: clear signal 3 */
	SP_SET_SIG3				= 0x10000,	    /* Bit 16: set signal 3 */
	SP_CLR_SIG4				= 0x20000,	    /* Bit 17: clear signal 4 */
	SP_SET_SIG4				= 0x40000,	    /* Bit 18: set signal 4 */
	SP_CLR_SIG5				= 0x80000,	    /* Bit 19: clear signal 5 */
	SP_SET_SIG5				= 0x100000,	/* Bit 20: set signal 5 */
	SP_CLR_SIG6				= 0x200000,	/* Bit 21: clear signal 6 */
	SP_SET_SIG6				= 0x400000,	/* Bit 22: set signal 6 */
	SP_CLR_SIG7				= 0x800000,	/* Bit 23: clear signal 7 */
	SP_SET_SIG7				= 0x1000000,   /* Bit 24: set signal 7 */

	SP_STATUS_HALT			= 0x001,		/* Bit  0: halt */
	SP_STATUS_BROKE			= 0x002,		/* Bit  1: broke */
	SP_STATUS_DMA_BUSY		= 0x004,		/* Bit  2: dma busy */
	SP_STATUS_DMA_FULL		= 0x008,		/* Bit  3: dma full */
	SP_STATUS_IO_FULL		= 0x010,		/* Bit  4: io full */
	SP_STATUS_SSTEP			= 0x020,		/* Bit  5: single step */
	SP_STATUS_INTR_BREAK	= 0x040,		/* Bit  6: interrupt on break */
	SP_STATUS_SIG0			= 0x080,		/* Bit  7: signal 0 set */
	SP_STATUS_SIG1			= 0x100,		/* Bit  8: signal 1 set */
	SP_STATUS_SIG2			= 0x200,		/* Bit  9: signal 2 set */
	SP_STATUS_SIG3			= 0x400,		/* Bit 10: signal 3 set */
	SP_STATUS_SIG4			= 0x800,		/* Bit 11: signal 4 set */
	SP_STATUS_SIG5	       = 0x1000,		/* Bit 12: signal 5 set */
	SP_STATUS_SIG6	       = 0x2000,		/* Bit 13: signal 6 set */
	SP_STATUS_SIG7	       = 0x4000,		/* Bit 14: signal 7 set */
};

//Peripheral Interface
/*#define Peripheral_Interface	_Reg->_PeripheralInterface
#define PI_DRAM_ADDR_REG		_Reg->_PeripheralInterface[0]
#define PI_CART_ADDR_REG		_Reg->_PeripheralInterface[1]
#define PI_RD_LEN_REG			_Reg->_PeripheralInterface[2]
#define PI_WR_LEN_REG			_Reg->_PeripheralInterface[3]
#define PI_STATUS_REG			_Reg->_PeripheralInterface[4]
#define PI_BSD_DOM1_LAT_REG 	_Reg->_PeripheralInterface[5]
#define PI_DOMAIN1_REG		 	_Reg->_PeripheralInterface[5]
#define PI_BSD_DOM1_PWD_REG	 	_Reg->_PeripheralInterface[6]
#define PI_BSD_DOM1_PGS_REG	 	_Reg->_PeripheralInterface[7]
#define PI_BSD_DOM1_RLS_REG	 	_Reg->_PeripheralInterface[8]
#define PI_BSD_DOM2_LAT_REG	 	_Reg->_PeripheralInterface[9]
#define PI_DOMAIN2_REG		 	_Reg->_PeripheralInterface[9]
#define PI_BSD_DOM2_PWD_REG	 	_Reg->_PeripheralInterface[10]
#define PI_BSD_DOM2_PGS_REG	 	_Reg->_PeripheralInterface[11]
#define PI_BSD_DOM2_RLS_REG	 	_Reg->_PeripheralInterface[12]
*/
//Peripheral Interface flags
enum {
	PI_STATUS_DMA_BUSY	=	0x01,
	PI_STATUS_IO_BUSY	=	0x02,
	PI_STATUS_ERROR		=	0x04,

	PI_SET_RESET		=	0x01,
	PI_CLR_INTR			=	0x02,
};

//Serial Interface
/*#define SerialInterface			_Reg->_SerialInterface
#define SI_DRAM_ADDR_REG		_Reg->_SerialInterface[0]
#define SI_PIF_ADDR_RD64B_REG	_Reg->_SerialInterface[1]
#define SI_PIF_ADDR_WR64B_REG	_Reg->_SerialInterface[2]
#define SI_STATUS_REG			_Reg->_SerialInterface[3]
*/
//Serial Interface flags
enum {
	SI_STATUS_DMA_BUSY	=	0x0001,
	SI_STATUS_RD_BUSY   =	0x0002,
	SI_STATUS_DMA_ERROR	=	0x0008,
	SI_STATUS_INTERRUPT	=	0x1000,
};


class CRegistersName  {
public:
	static const char *GPR_Name[32];
	static const char *GPR_NameHi[32];
	static const char *GPR_NameLo[32];
	static const char *Cop0_Name[32];
	static const char *FPR_Name[32];
	static const char *FPR_Ctrl_Name[32];
};

class CMipsMemory;
class CRegisters: 
	public CP0registers,
	public Mips_InterfaceReg,
	public Video_InterfaceReg,
	public AudioInterfaceReg,
	public SigProcessor_InterfaceReg,
	public DisplayControlReg,
	public CSystemTimer,
	public CRegistersName
{
public:
	//Constructor/Deconstructor
	CRegisters ( void ) :
		CP0registers(CP0),
		AudioInterfaceReg(Audio_Interface),
		Mips_InterfaceReg(Mips_Interface),
		Video_InterfaceReg(Video_Interface),
		SigProcessor_InterfaceReg(SigProcessor_Interface),
		DisplayControlReg(Display_ControlReg)
	{ 
		FixFpuLocations();
	}
	
	//General Registers
	DWORD               PROGRAM_COUNTER;	
	MULTI_ACCESS_QWORD  GPR[32];
	DWORD               CP0[33];
	DWORD               FPCR[32];
	MULTI_ACCESS_QWORD  HI, LO; //High and Low registers used for mult and div
	DWORD               LLBit;
	DWORD               LLAddr;
	
	//Floating point registers/information
	ROUNDING_MODE       RoundingModel;
	MULTI_ACCESS_QWORD  FPR[32];
	float             * FPR_S[32];		
	double            * FPR_D[32];

	//Memory Mapped N64 registers
	DWORD				RDRAM_Interface[8];
	DWORD				RDRAM_Registers[10];
	DWORD               Mips_Interface[4];
	DWORD               Video_Interface[14];
	DWORD               Display_ControlReg[10];
	DWORD               Audio_Interface[6];
	DWORD               SigProcessor_Interface[10];
	DWORD               Peripheral_Interface[13];
	DWORD               SerialInterface[4];
	DWORD               AudioIntrReg;


	void InitalizeR4300iRegisters    ( CMipsMemory & MMU, bool PostPif, int Country, CICChip CIC_Chip);
	void CheckInterrupts             ( void );
	void ExecuteCopUnusableException ( bool DelaySlot, int Coprocessor );
	void ExecuteInterruptException   ( bool DelaySlot );
	void ExecuteTLBMissException     ( CMipsMemory * MMU, bool DelaySlot, DWORD BadVaddr );
	void ExecuteSysCallException     ( bool DelaySlot );
	void UpdateRegisterAfterOpcode   ( float StepIncrease );
	void FixFpuLocations             ( void );
	void SetCurrentRoundingModel     ( ROUNDING_MODE RoundMode );
	void ChangeDefaultRoundingModel  ( int Reg );
};

//Converting FPU
__inline void S_RoundToInteger32( int * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void S_RoundToInteger64( __int64 * Dest, float * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld dword ptr [esi]
		fistp qword ptr [edi]
	}
}

__inline void D_RoundToInteger32( int * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp dword ptr [edi]
	}
}

__inline void D_RoundToInteger64( __int64 * Dest, double * Source ) {
	_asm {
		mov esi, [Source]
		mov edi, [Dest]
		fld qword ptr [esi]
		fistp qword ptr [edi]
	}
}

#endif
