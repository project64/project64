/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <Common/Platform.h>
#include <Project64-core/N64System/N64Types.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Settings/GameSettings.h>
#include <Project64-core/Logging.h>

//CPO registers by name
class CP0registers
{
protected:
    CP0registers (uint32_t * _CP0);

public:
    uint32_t & INDEX_REGISTER;
    uint32_t & RANDOM_REGISTER;
    uint32_t & ENTRYLO0_REGISTER;
    uint32_t & ENTRYLO1_REGISTER;
    uint32_t & CONTEXT_REGISTER;
    uint32_t & PAGE_MASK_REGISTER;
    uint32_t & WIRED_REGISTER;
    uint32_t & BAD_VADDR_REGISTER;
    uint32_t & COUNT_REGISTER;
    uint32_t & ENTRYHI_REGISTER;
    uint32_t & COMPARE_REGISTER;
    uint32_t & STATUS_REGISTER;
    uint32_t & CAUSE_REGISTER;
    uint32_t & EPC_REGISTER;
    uint32_t & CONFIG_REGISTER;
    uint32_t & TAGLO_REGISTER;
    uint32_t & TAGHI_REGISTER;
    uint32_t & ERROREPC_REGISTER;
    uint32_t & FAKE_CAUSE_REGISTER;

private:
    CP0registers();									// Disable default constructor
    CP0registers(const CP0registers&);				// Disable copy constructor
    CP0registers& operator=(const CP0registers&);	// Disable assignment
};

//CPO register flags
enum
{
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
enum
{
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
protected:
    Rdram_InterfaceReg (uint32_t * _RdramInterface);

public:
    uint32_t & RDRAM_CONFIG_REG;
    uint32_t & RDRAM_DEVICE_TYPE_REG;
    uint32_t & RDRAM_DEVICE_ID_REG;
    uint32_t & RDRAM_DELAY_REG;
    uint32_t & RDRAM_MODE_REG;
    uint32_t & RDRAM_REF_INTERVAL_REG;
    uint32_t & RDRAM_REF_ROW_REG;
    uint32_t & RDRAM_RAS_INTERVAL_REG;
    uint32_t & RDRAM_MIN_INTERVAL_REG;
    uint32_t & RDRAM_ADDR_SELECT_REG;
    uint32_t & RDRAM_DEVICE_MANUF_REG;

private:
    Rdram_InterfaceReg();										// Disable default constructor
    Rdram_InterfaceReg(const Rdram_InterfaceReg&);				// Disable copy constructor
    Rdram_InterfaceReg& operator=(const Rdram_InterfaceReg&);	// Disable assignment
};

//Mips interface registers
class Mips_InterfaceReg
{
protected:
    Mips_InterfaceReg (uint32_t * _MipsInterface);

public:
    uint32_t & MI_INIT_MODE_REG;
    uint32_t & MI_MODE_REG;
    uint32_t & MI_VERSION_REG;
    uint32_t & MI_NOOP_REG;
    uint32_t & MI_INTR_REG;
    uint32_t & MI_INTR_MASK_REG;

private:
    Mips_InterfaceReg();									// Disable default constructor
    Mips_InterfaceReg(const Mips_InterfaceReg&);			// Disable copy constructor
    Mips_InterfaceReg& operator=(const Mips_InterfaceReg&);	// Disable assignment
};

//Mips interface flags
enum
{
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
protected:
    Video_InterfaceReg (uint32_t * _VideoInterface);

public:
    uint32_t & VI_STATUS_REG;
    uint32_t & VI_CONTROL_REG;
    uint32_t & VI_ORIGIN_REG;
    uint32_t & VI_DRAM_ADDR_REG;
    uint32_t & VI_WIDTH_REG;
    uint32_t & VI_H_WIDTH_REG;
    uint32_t & VI_INTR_REG;
    uint32_t & VI_V_INTR_REG;
    uint32_t & VI_CURRENT_REG;
    uint32_t & VI_V_CURRENT_LINE_REG;
    uint32_t & VI_BURST_REG;
    uint32_t & VI_TIMING_REG;
    uint32_t & VI_V_SYNC_REG;
    uint32_t & VI_H_SYNC_REG;
    uint32_t & VI_LEAP_REG;
    uint32_t & VI_H_SYNC_LEAP_REG;
    uint32_t & VI_H_START_REG;
    uint32_t & VI_H_VIDEO_REG;
    uint32_t & VI_V_START_REG;
    uint32_t & VI_V_VIDEO_REG;
    uint32_t & VI_V_BURST_REG;
    uint32_t & VI_X_SCALE_REG;
    uint32_t & VI_Y_SCALE_REG;

private:
    Video_InterfaceReg();										// Disable default constructor
    Video_InterfaceReg(const Video_InterfaceReg&);				// Disable copy constructor
    Video_InterfaceReg& operator=(const Video_InterfaceReg&);	// Disable assignment
};

//Display Processor Control Registers
class DisplayControlReg
{
protected:
    DisplayControlReg (uint32_t * _DisplayProcessor);

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
    DisplayControlReg();										// Disable default constructor
    DisplayControlReg(const DisplayControlReg&);				// Disable copy constructor
    DisplayControlReg& operator=(const DisplayControlReg&);	// Disable assignment
};

enum
{
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
    AudioInterfaceReg (uint32_t * _AudioInterface);

public:
    uint32_t & AI_DRAM_ADDR_REG;
    uint32_t & AI_LEN_REG;
    uint32_t & AI_CONTROL_REG;
    uint32_t & AI_STATUS_REG;
    uint32_t & AI_DACRATE_REG;
    uint32_t & AI_BITRATE_REG;

private:
    AudioInterfaceReg();										// Disable default constructor
    AudioInterfaceReg(const AudioInterfaceReg&);				// Disable copy constructor
    AudioInterfaceReg& operator=(const AudioInterfaceReg&);	// Disable assignment
};

enum
{
    AI_STATUS_FIFO_FULL			= 0x80000000,	/* Bit 31: full */
    AI_STATUS_DMA_BUSY			= 0x40000000,	/* Bit 30: busy */
};

//Audio Interface registers;

class PeripheralInterfaceReg
{
protected:
    PeripheralInterfaceReg (uint32_t * PeripheralInterface);

public:
    uint32_t & PI_DRAM_ADDR_REG;
    uint32_t & PI_CART_ADDR_REG;
    uint32_t & PI_RD_LEN_REG;
    uint32_t & PI_WR_LEN_REG;
    uint32_t & PI_STATUS_REG;
    uint32_t & PI_BSD_DOM1_LAT_REG;
    uint32_t & PI_DOMAIN1_REG;
    uint32_t & PI_BSD_DOM1_PWD_REG;
    uint32_t & PI_BSD_DOM1_PGS_REG;
    uint32_t & PI_BSD_DOM1_RLS_REG;
    uint32_t & PI_BSD_DOM2_LAT_REG;
    uint32_t & PI_DOMAIN2_REG;
    uint32_t & PI_BSD_DOM2_PWD_REG;
    uint32_t & PI_BSD_DOM2_PGS_REG;
    uint32_t & PI_BSD_DOM2_RLS_REG;

private:
    PeripheralInterfaceReg();											// Disable default constructor
    PeripheralInterfaceReg(const PeripheralInterfaceReg&);				// Disable copy constructor
    PeripheralInterfaceReg& operator=(const PeripheralInterfaceReg&);	// Disable assignment
};

class RDRAMInt_InterfaceReg
{
protected:
    RDRAMInt_InterfaceReg (uint32_t * RdramInterface);

public:
    uint32_t & RI_MODE_REG;
    uint32_t & RI_CONFIG_REG;
    uint32_t & RI_CURRENT_LOAD_REG;
    uint32_t & RI_SELECT_REG;
    uint32_t & RI_COUNT_REG;
    uint32_t & RI_REFRESH_REG;
    uint32_t & RI_LATENCY_REG;
    uint32_t & RI_RERROR_REG;
    uint32_t & RI_WERROR_REG;

private:
    RDRAMInt_InterfaceReg();											// Disable default constructor
    RDRAMInt_InterfaceReg(const RDRAMInt_InterfaceReg&);				// Disable copy constructor
    RDRAMInt_InterfaceReg& operator=(const RDRAMInt_InterfaceReg&);	// Disable assignment
};

//Signal Processor Interface;
class SigProcessor_InterfaceReg
{
protected:
    SigProcessor_InterfaceReg (uint32_t * _SignalProcessorInterface);

public:
    uint32_t & SP_MEM_ADDR_REG;
    uint32_t & SP_DRAM_ADDR_REG;
    uint32_t & SP_RD_LEN_REG;
    uint32_t & SP_WR_LEN_REG;
    uint32_t & SP_STATUS_REG;
    uint32_t & SP_DMA_FULL_REG;
    uint32_t & SP_DMA_BUSY_REG;
    uint32_t & SP_SEMAPHORE_REG;
    uint32_t & SP_PC_REG;
    uint32_t & SP_IBIST_REG;

private:
    SigProcessor_InterfaceReg();											// Disable default constructor
    SigProcessor_InterfaceReg(const SigProcessor_InterfaceReg&);			// Disable copy constructor
    SigProcessor_InterfaceReg& operator=(const SigProcessor_InterfaceReg&);	// Disable assignment
};

//Signal Processor interface flags
enum
{
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
enum
{
    PI_STATUS_DMA_BUSY	=	0x01,
    PI_STATUS_IO_BUSY	=	0x02,
    PI_STATUS_ERROR		=	0x04,

    PI_SET_RESET		=	0x01,
    PI_CLR_INTR			=	0x02,
};

class Serial_InterfaceReg
{
protected:
    Serial_InterfaceReg (uint32_t * SerialInterface);

public:
    uint32_t & SI_DRAM_ADDR_REG;
    uint32_t & SI_PIF_ADDR_RD64B_REG;
    uint32_t & SI_PIF_ADDR_WR64B_REG;
    uint32_t & SI_STATUS_REG;

private:
    Serial_InterfaceReg();										// Disable default constructor
    Serial_InterfaceReg(const Serial_InterfaceReg&);			// Disable copy constructor
    Serial_InterfaceReg& operator=(const Serial_InterfaceReg&);	// Disable assignment
};

//Serial Interface flags
enum
{
    SI_STATUS_DMA_BUSY	=	0x0001,
    SI_STATUS_RD_BUSY   =	0x0002,
    SI_STATUS_DMA_ERROR	=	0x0008,
    SI_STATUS_INTERRUPT	=	0x1000,
};

//Disk Interface
class Disk_InterfaceReg
{
protected:
    Disk_InterfaceReg (uint32_t * Disk_Interface);

public:
    uint32_t & ASIC_DATA;
    uint32_t & ASIC_MISC_REG;
    uint32_t & ASIC_STATUS;
    uint32_t & ASIC_CMD;
    uint32_t & ASIC_CUR_TK;
    uint32_t & ASIC_BM_STATUS;
    uint32_t & ASIC_BM_CTL;
    uint32_t & ASIC_ERR_SECTOR;
    uint32_t & ASIC_SEQ_STATUS;
    uint32_t & ASIC_SEQ_CTL;
    uint32_t & ASIC_CUR_SECTOR;
    uint32_t & ASIC_HARD_RESET;
    uint32_t & ASIC_C1_S0;
    uint32_t & ASIC_HOST_SECBYTE;
    uint32_t & ASIC_C1_S2;
    uint32_t & ASIC_SEC_BYTE;
    uint32_t & ASIC_C1_S4;
    uint32_t & ASIC_C1_S6;
    uint32_t & ASIC_CUR_ADDR;
    uint32_t & ASIC_ID_REG;
    uint32_t & ASIC_TEST_REG;
    uint32_t & ASIC_TEST_PIN_SEL;

private:
    Disk_InterfaceReg();										// Disable default constructor
    Disk_InterfaceReg(const Disk_InterfaceReg&);			// Disable copy constructor
    Disk_InterfaceReg& operator=(const Disk_InterfaceReg&);	// Disable assignment
};

//Disk Interface Flags
enum
{
    DD_STATUS_DATA_RQ    =	0x40000000,
    DD_STATUS_C2_XFER    =	0x10000000,
    DD_STATUS_BM_ERR     =	0x08000000,
    DD_STATUS_BM_INT     =	0x04000000,
    DD_STATUS_MECHA_INT  =	0x02000000,
    DD_STATUS_DISK_PRES  =	0x01000000,
    DD_STATUS_BUSY_STATE =	0x00800000,
    DD_STATUS_RST_STATE  =	0x00400000,
    DD_STATUS_MTR_N_SPIN =	0x00100000,
    DD_STATUS_HEAD_RTRCT =	0x00080000,
    DD_STATUS_WR_PR_ERR  =	0x00040000,
    DD_STATUS_MECHA_ERR  =	0x00020000,
    DD_STATUS_DISK_CHNG  =	0x00010000,

    DD_BM_STATUS_RUNNING =	0x80000000,
    DD_BM_STATUS_ERROR   =	0x04000000,
    DD_BM_STATUS_MICRO   =	0x02000000,
    DD_BM_STATUS_BLOCK   =	0x01000000,

    DD_BM_CTL_START      =	0x80000000,
    DD_BM_CTL_MNGRMODE   =	0x40000000,
    DD_BM_CTL_INTMASK    =	0x20000000,
    DD_BM_CTL_RESET      =	0x10000000,
    DD_BM_CTL_BLK_TRANS  =	0x02000000,
    DD_BM_CTL_MECHA_RST  =	0x01000000
};

class CRegName
{
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
    static uint32_t      * _PROGRAM_COUNTER;
    static MIPS_DWORD    * _GPR;
    static MIPS_DWORD    * _FPR;
    static uint32_t      * _CP0;
    static MIPS_DWORD    * _RegHI;
    static MIPS_DWORD    * _RegLO;
    static float        ** _FPR_S;
    static double       ** _FPR_D;
    static uint32_t      * _FPCR;
    static uint32_t      * _LLBit;
    static int32_t       * _RoundingModel;
};

class CN64System;
class CSystemEvents;

class CRegisters :
    public CLogging,
    private CDebugSettings,
    private CGameSettings,
    protected CSystemRegisters,
    public CP0registers,
    public Rdram_InterfaceReg,
    public Mips_InterfaceReg,
    public Video_InterfaceReg,
    public AudioInterfaceReg,
    public PeripheralInterfaceReg,
    public RDRAMInt_InterfaceReg,
    public SigProcessor_InterfaceReg,
    public DisplayControlReg,
    public Serial_InterfaceReg,
    public Disk_InterfaceReg
{
public:
    CRegisters(CN64System * System, CSystemEvents * SystemEvents);

    //General Registers
    uint32_t    m_PROGRAM_COUNTER;
    MIPS_DWORD  m_GPR[32];
    uint32_t    m_CP0[33];
    MIPS_DWORD  m_HI;
    MIPS_DWORD  m_LO;
    uint32_t    m_LLBit;

    //Floating point registers/information
    uint32_t        m_FPCR[32];
    int32_t         m_RoundingModel;
    MIPS_DWORD      m_FPR[32];
    float         * m_FPR_S[32];
    double        * m_FPR_D[32];

    //Memory Mapped N64 registers
    uint32_t           m_RDRAM_Registers[10];
    uint32_t           m_SigProcessor_Interface[10];
    uint32_t           m_Display_ControlReg[10];
    uint32_t           m_Mips_Interface[4];
    uint32_t           m_Video_Interface[14];
    uint32_t           m_Audio_Interface[6];
    uint32_t           m_Peripheral_Interface[13];
    uint32_t           m_RDRAM_Interface[8];
    uint32_t           m_SerialInterface[4];
    uint32_t           m_DiskInterface[22];
    uint32_t           m_AudioIntrReg;
    uint32_t           m_GfxIntrReg;
    uint32_t           m_RspIntrReg;

    void CheckInterrupts        ();
    void DoAddressError         ( bool DelaySlot, uint32_t BadVaddr, bool FromRead );
    void DoBreakException       ( bool DelaySlot );
    void DoTrapException        ( bool DelaySlot );
    void DoCopUnusableException ( bool DelaySlot, int32_t Coprocessor );
    bool DoIntrException        ( bool DelaySlot );
    void DoTLBReadMiss          ( bool DelaySlot, uint32_t BadVaddr );
    void DoSysCallException     ( bool DelaySlot);
    void FixFpuLocations        ();
    void Reset                  ();
    void SetAsCurrentSystem     ();

private:
    CRegisters();                             // Disable default constructor
    CRegisters(const CRegisters&);            // Disable copy constructor
    CRegisters& operator=(const CRegisters&); // Disable assignment

    bool            m_FirstInterupt;
    CN64System    * m_System;
    CSystemEvents * m_SystemEvents;
};
