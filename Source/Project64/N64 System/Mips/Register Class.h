//CPO registers by name
class CP0registers
{
	CP0registers (void);

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

//Rdram Registers
class Rdram_InterfaceReg
{
	Rdram_InterfaceReg (void);

protected:
	Rdram_InterfaceReg (DWORD * _RdramInterface);

public:
	DWORD & RDRAM_CONFIG_REG;
	DWORD & RDRAM_DEVICE_TYPE_REG;
	DWORD & RDRAM_DEVICE_ID_REG;
	DWORD & RDRAM_DELAY_REG;
	DWORD & RDRAM_MODE_REG;
	DWORD & RDRAM_REF_INTERVAL_REG;
	DWORD & RDRAM_REF_ROW_REG;
	DWORD & RDRAM_RAS_INTERVAL_REG;
	DWORD & RDRAM_MIN_INTERVAL_REG;
	DWORD & RDRAM_ADDR_SELECT_REG;
	DWORD & RDRAM_DEVICE_MANUF_REG;
};

//Mips interface registers
class Mips_InterfaceReg
{
	Mips_InterfaceReg ();

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
	MI_MODE_INIT			= 0x0080,		/* Bit  7: init mode */
	MI_MODE_EBUS			= 0x0100,		/* Bit  8: ebus test mode */
	MI_MODE_RDRAM			= 0x0200,		/* Bit  9: RDRAM reg mode */

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
	Video_InterfaceReg (void);

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
	DisplayControlReg (void);

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
	AudioInterfaceReg (void);

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

//Audio Interface registers;

class PeripheralInterfaceReg
{
	PeripheralInterfaceReg (void);
	
protected:
	PeripheralInterfaceReg (DWORD * PeripheralInterface);

public:
	DWORD & PI_DRAM_ADDR_REG;
	DWORD & PI_CART_ADDR_REG;
	DWORD & PI_RD_LEN_REG;
	DWORD & PI_WR_LEN_REG;
	DWORD & PI_STATUS_REG;
	DWORD & PI_BSD_DOM1_LAT_REG;
	DWORD & PI_DOMAIN1_REG;
	DWORD & PI_BSD_DOM1_PWD_REG;
	DWORD & PI_BSD_DOM1_PGS_REG;
	DWORD & PI_BSD_DOM1_RLS_REG;
	DWORD & PI_BSD_DOM2_LAT_REG;
	DWORD & PI_DOMAIN2_REG;
	DWORD & PI_BSD_DOM2_PWD_REG;
	DWORD & PI_BSD_DOM2_PGS_REG;
	DWORD & PI_BSD_DOM2_RLS_REG;
};

class RDRAMInt_InterfaceReg
{
	RDRAMInt_InterfaceReg (void);

protected:
	RDRAMInt_InterfaceReg (DWORD * RdramInterface);

public:
	DWORD & RI_MODE_REG;
	DWORD & RI_CONFIG_REG;
	DWORD & RI_CURRENT_LOAD_REG;
	DWORD & RI_SELECT_REG;
	DWORD & RI_COUNT_REG;
	DWORD & RI_REFRESH_REG;
	DWORD & RI_LATENCY_REG;
	DWORD & RI_RERROR_REG;
	DWORD & RI_WERROR_REG;
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

//Peripheral Interface flags
enum {
	PI_STATUS_DMA_BUSY	=	0x01,
	PI_STATUS_IO_BUSY	=	0x02,
	PI_STATUS_ERROR		=	0x04,

	PI_SET_RESET		=	0x01,
	PI_CLR_INTR			=	0x02,
};


class Serial_InterfaceReg
{
	Serial_InterfaceReg (void);

protected:
	Serial_InterfaceReg (DWORD * SerialInterface);

public:
	DWORD & SI_DRAM_ADDR_REG;
	DWORD & SI_PIF_ADDR_RD64B_REG;
	DWORD & SI_PIF_ADDR_WR64B_REG;
	DWORD & SI_STATUS_REG;
};

//Serial Interface flags
enum {
	SI_STATUS_DMA_BUSY	=	0x0001,
	SI_STATUS_RD_BUSY   =	0x0002,
	SI_STATUS_DMA_ERROR	=	0x0008,
	SI_STATUS_INTERRUPT	=	0x1000,
};


enum ROUNDING_MODE {
	ROUND_NEAR = _RC_NEAR, 
	ROUND_DOWN = _RC_DOWN,	
	ROUND_UP   = _RC_UP, 
	ROUND_CHOP = _RC_CHOP, 
};

class CRegName  {
public:
	static const char *GPR[32];
	static const char *GPR_Hi[32];
	static const char *GPR_Lo[32];
	static const char *Cop0[32];
	static const char *FPR[32];
	static const char *FPR_Ctrl[32];
};

class CSystemRegisters
{
protected:
	static DWORD         * _PROGRAM_COUNTER;
	static MIPS_DWORD    * _GPR;
	static MIPS_DWORD    * _FPR;
	static DWORD         * _CP0;
	static MIPS_DWORD    * _RegHI;
	static MIPS_DWORD    * _RegLO;
	static float         ** _FPR_S;		
	static double        ** _FPR_D;
	static DWORD         * _FPCR;
	static DWORD         * _LLBit;
	static ROUNDING_MODE * _RoundingModel;
};

class CRegisters: 
	protected CSystemRegisters,
	protected CGameSettings,
	public CP0registers,
	public Rdram_InterfaceReg,
	public Mips_InterfaceReg,
	public Video_InterfaceReg,
	public AudioInterfaceReg,
	public PeripheralInterfaceReg,
	public RDRAMInt_InterfaceReg,
	public SigProcessor_InterfaceReg,
	public DisplayControlReg,
	public Serial_InterfaceReg
{
public:
	CRegisters();

	//General Registers
	DWORD           m_PROGRAM_COUNTER;
    MIPS_DWORD      m_GPR[32];
	DWORD           m_CP0[33];
	MIPS_DWORD      m_HI;
	MIPS_DWORD      m_LO;
	DWORD           m_LLBit;
	DWORD           m_LLAddr;
	
	//Floating point registers/information
	DWORD           m_FPCR[32];
	ROUNDING_MODE   m_RoundingModel;
	MIPS_DWORD      m_FPR[32];
	float         * m_FPR_S[32];		
	double        * m_FPR_D[32];

	//Memory Mapped N64 registers
	DWORD           m_RDRAM_Registers[10];
	DWORD           m_SigProcessor_Interface[10];
	DWORD           m_Display_ControlReg[10];
	DWORD           m_Mips_Interface[4];
	DWORD           m_Video_Interface[14];
	DWORD           m_Audio_Interface[6];
	DWORD           m_Peripheral_Interface[13];
	DWORD           m_RDRAM_Interface[8];
	DWORD           m_SerialInterface[4];
	DWORD           m_AudioIntrReg;
	DWORD           m_GfxIntrReg;
	DWORD           m_RspIntrReg;


	void CheckInterrupts        ( void );
	void DoAddressError         ( BOOL DelaySlot, DWORD BadVaddr, BOOL FromRead ); 
	void DoBreakException       ( BOOL DelaySlot ); 
	void DoCopUnusableException ( BOOL DelaySlot, int Coprocessor );
	BOOL DoIntrException        ( BOOL DelaySlot );
	void DoTLBReadMiss          ( BOOL DelaySlot, DWORD BadVaddr );
	void DoSysCallException     ( BOOL DelaySlot);
	void FixFpuLocations        ( void );
	void Reset                  ( void );
	void SetAsCurrentSystem     ( void );

private:
	bool            m_FirstInterupt;

};
