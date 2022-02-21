#pragma once

#include <Common/Platform.h>
#include <Project64-core\N64System\N64Types.h>
#include <Project64-core\N64System\MemoryHandler\PeripheralInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\SPRegistersHandler.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Settings\GameSettings.h>
#include <Project64-core\Logging.h>

// CPO registers by name
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
    CP0registers();
    CP0registers(const CP0registers&);
    CP0registers& operator=(const CP0registers&);
};

// CPO register flags
enum
{
    // Status register
    STATUS_IE  = 0x00000001, STATUS_EXL = 0x00000002, STATUS_ERL = 0x00000004,
    STATUS_IP0 = 0x00000100, STATUS_IP1 = 0x00000200, STATUS_IP2 = 0x00000400,
    STATUS_IP3 = 0x00000800, STATUS_IP4 = 0x00001000, STATUS_IP5 = 0x00002000,
    STATUS_IP6 = 0x00004000, STATUS_IP7 = 0x00008000, STATUS_BEV = 0x00400000,
    STATUS_FR  = 0x04000000, STATUS_CU0 = 0x10000000, STATUS_CU1 = 0x20000000,

    // Cause flags
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

    // Cause exception ID's
    EXC_INT				= 0,	// Interrupt
    EXC_MOD				= 4,	// TLB mod
    EXC_RMISS			= 8,	// Read TLB miss
    EXC_WMISS			= 12,	// Write TLB miss
    EXC_RADE			= 16,	// Read address error
    EXC_WADE			= 20,	// Write address error
    EXC_IBE				= 24,	// Instruction bus error
    EXC_DBE				= 28,	// Data bus error
    EXC_SYSCALL			= 32,	// Syscall
    EXC_BREAK			= 36,	// Breakpoint
    EXC_II				= 40,   // Illegal instruction
    EXC_CPU				= 44,   // Co-processor unusable
    EXC_OV				= 48,   // Overflow
    EXC_TRAP			= 52,   // Trap exception
    EXC_VCEI			= 56,   // Virtual coherency on instruction fetch
    EXC_FPE				= 60,   // Floating point exception
    EXC_WATCH			= 92,   // Watchpoint reference
    EXC_VCED			= 124,  // Virtual coherency on data read
};

// Float point control status register flags
enum
{
    FPCSR_FS			= 0x01000000, // Flush denormalization to zero
    FPCSR_C				= 0x00800000, // Condition bit
    FPCSR_CE			= 0x00020000, // Cause: unimplemented operation
    FPCSR_CV			= 0x00010000, // Cause: invalid operation
    FPCSR_CZ			= 0x00008000, // Cause: division by zero
    FPCSR_CO			= 0x00004000, // Cause: overflow
    FPCSR_CU			= 0x00002000, // Cause: underflow
    FPCSR_CI			= 0x00001000, // Cause: inexact operation
    FPCSR_EV			= 0x00000800, // Enable: invalid operation
    FPCSR_EZ			= 0x00000400, // Enable: division by zero
    FPCSR_EO			= 0x00000200, // Enable: overflow
    FPCSR_EU			= 0x00000100, // Enable: underflow
    FPCSR_EI			= 0x00000080, // Enable: inexact operation
    FPCSR_FV			= 0x00000040, // Flag: invalid operation
    FPCSR_FZ			= 0x00000020, // Flag: division by zero
    FPCSR_FO			= 0x00000010, // Flag: overflow
    FPCSR_FU			= 0x00000008, // Flag: underflow
    FPCSR_FI			= 0x00000004, // Flag: inexact operation
    FPCSR_RM_MASK		= 0x00000003, // Rounding mode mask
    FPCSR_RM_RN			= 0x00000000, // Round to nearest
    FPCSR_RM_RZ			= 0x00000001, // Round to zero
    FPCSR_RM_RP			= 0x00000002, // Round to positive infinity
    FPCSR_RM_RM			= 0x00000003, // Round to negative infinity
};

// MIPS interface registers
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
    Mips_InterfaceReg();
    Mips_InterfaceReg(const Mips_InterfaceReg&);
    Mips_InterfaceReg& operator=(const Mips_InterfaceReg&);
};

// MIPS interface flags
enum
{
    MI_MODE_INIT			= 0x0080,		// Bit  7: Initialization mode
    MI_MODE_EBUS			= 0x0100,		// Bit  8: EBUS test mode
    MI_MODE_RDRAM			= 0x0200,		// Bit  9: RDRAM register mode

    MI_CLR_INIT				= 0x0080,		// Bit  7: Clear initialization mode
    MI_SET_INIT				= 0x0100,		// Bit  8: Set initialization mode
    MI_CLR_EBUS				= 0x0200,		// Bit  9: Clear EBUS test
    MI_SET_EBUS				= 0x0400,		// Bit 10: Set EBUS test mode
    MI_CLR_DP_INTR			= 0x0800,		// Bit 11: Clear DP interrupt
    MI_CLR_RDRAM			= 0x1000,		// Bit 12: Clear RDRAM register
    MI_SET_RDRAM			= 0x2000,		// Bit 13: Set RDRAM register mode

    // Flags for writing to MI_INTR_MASK_REG
    MI_INTR_MASK_CLR_SP		= 0x0001,		// Bit  0: Clear SP mask
    MI_INTR_MASK_SET_SP		= 0x0002,		// Bit  1: Set SP mask
    MI_INTR_MASK_CLR_SI		= 0x0004,		// Bit  2: Clear SI mask
    MI_INTR_MASK_SET_SI		= 0x0008,		// Bit  3: Set SI mask
    MI_INTR_MASK_CLR_AI		= 0x0010,		// Bit  4: Clear AI mask
    MI_INTR_MASK_SET_AI		= 0x0020,		// Bit  5: Set AI mask
    MI_INTR_MASK_CLR_VI		= 0x0040,		// Bit  6: Clear VI mask
    MI_INTR_MASK_SET_VI		= 0x0080,		// Bit  7: Set VI mask
    MI_INTR_MASK_CLR_PI		= 0x0100,		// Bit  8: Clear PI mask
    MI_INTR_MASK_SET_PI		= 0x0200,		// Bit  9: Set PI mask
    MI_INTR_MASK_CLR_DP		= 0x0400,		// Bit 10: Clear DP mask
    MI_INTR_MASK_SET_DP		= 0x0800,		// Bit 11: Set DP mask

    // Flags for reading from MI_INTR_MASK_REG
    MI_INTR_MASK_SP			= 0x01,		// Bit 0: SP INTR mask
    MI_INTR_MASK_SI			= 0x02,		// Bit 1: SI INTR mask
    MI_INTR_MASK_AI			= 0x04,		// Bit 2: AI INTR mask
    MI_INTR_MASK_VI			= 0x08,		// Bit 3: VI INTR mask
    MI_INTR_MASK_PI			= 0x10,		// Bit 4: PI INTR mask
    MI_INTR_MASK_DP			= 0x20,		// Bit 5: DP INTR mask

    MI_INTR_SP				= 0x01,		// Bit 0: SP INTR
    MI_INTR_SI				= 0x02,		// Bit 1: SI INTR
    MI_INTR_AI				= 0x04,		// Bit 2: AI INTR
    MI_INTR_VI				= 0x08,		// Bit 3: VI INTR
    MI_INTR_PI				= 0x10,		// Bit 4: PI INTR
    MI_INTR_DP				= 0x20,		// Bit 5: DP INTR
};

// MIPS interface registers
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
    Video_InterfaceReg();
    Video_InterfaceReg(const Video_InterfaceReg&);
    Video_InterfaceReg& operator=(const Video_InterfaceReg&);
};

// Display processor control registers
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
    DisplayControlReg();
    DisplayControlReg(const DisplayControlReg&);
    DisplayControlReg& operator=(const DisplayControlReg&);
};

enum
{
    DPC_CLR_XBUS_DMEM_DMA	    = 0x0001,	// Bit 0: Clear xbus_dmem_dma
    DPC_SET_XBUS_DMEM_DMA	    = 0x0002,	// Bit 1: Set xbus_dmem_dma
    DPC_CLR_FREEZE			    = 0x0004,	// Bit 2: Clear freeze
    DPC_SET_FREEZE			    = 0x0008,	// Bit 3: Set freeze
    DPC_CLR_FLUSH			    = 0x0010,	// Bit 4: Clear flush
    DPC_SET_FLUSH			    = 0x0020,	// Bit 5: Set flush
    DPC_CLR_TMEM_CTR		    = 0x0040,	// Bit 6: Clear TMEM CTR
    DPC_CLR_PIPE_CTR		    = 0x0080,	// Bit 7: Clear pipe CTR
    DPC_CLR_CMD_CTR			    = 0x0100,	// Bit 8: Clear CMD CTR
    DPC_CLR_CLOCK_CTR		    = 0x0200,	// Bit 9: Clear clock CTR

    DPC_STATUS_XBUS_DMEM_DMA    = 0x001,	// Bit  0: xbus_dmem_dma
    DPC_STATUS_FREEZE			= 0x002,	// Bit  1: Freeze
    DPC_STATUS_FLUSH			= 0x004,	// Bit  2: Flush
    DPC_STATUS_START_GCLK		= 0x008,	// Bit  3: Start GCLK
    DPC_STATUS_TMEM_BUSY		= 0x010,	// Bit  4: TMEM busy
    DPC_STATUS_PIPE_BUSY		= 0x020,	// Bit  5: Pipe busy
    DPC_STATUS_CMD_BUSY			= 0x040,	// Bit  6: CMD busy
    DPC_STATUS_CBUF_READY		= 0x080,	// Bit  7: CBUF ready
    DPC_STATUS_DMA_BUSY			= 0x100,	// Bit  8: DMA busy
    DPC_STATUS_END_VALID		= 0x200,	// Bit  9: End valid
    DPC_STATUS_START_VALID		= 0x400,	// Bit 10: Start valid
};

// Audio interface registers
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
    AudioInterfaceReg();
    AudioInterfaceReg(const AudioInterfaceReg&);
    AudioInterfaceReg& operator=(const AudioInterfaceReg&);
};

enum
{
    AI_STATUS_FIFO_FULL			= 0x80000000,	// Bit 31: Full
    AI_STATUS_DMA_BUSY			= 0x40000000,	// Bit 30: Busy
};

// Signal processor interface flags
enum
{
    SP_CLR_HALT				= 0x00001,	    // Bit  0: Clear halt
    SP_SET_HALT				= 0x00002,	    // Bit  1: Set halt
    SP_CLR_BROKE			= 0x00004,	    // Bit  2: Clear broke
    SP_CLR_INTR				= 0x00008,	    // Bit  3: Clear INTR
    SP_SET_INTR				= 0x00010,	    // Bit  4: Set INTR
    SP_CLR_SSTEP			= 0x00020,	    // Bit  5: Clear SSTEP
    SP_SET_SSTEP			= 0x00040,	    // Bit  6: Set SSTEP
    SP_CLR_INTR_BREAK		= 0x00080,	    // Bit  7: Clear INTR on break
    SP_SET_INTR_BREAK		= 0x00100,	    // Bit  8: Set INTR on break
    SP_CLR_SIG0				= 0x00200,	    // Bit  9: Clear signal 0
    SP_SET_SIG0				= 0x00400,	    // Bit 10: Set signal 0
    SP_CLR_SIG1				= 0x00800,	    // Bit 11: Clear signal 1
    SP_SET_SIG1				= 0x01000,	    // Bit 12: Set signal 1
    SP_CLR_SIG2				= 0x02000,	    // Bit 13: Clear signal 2
    SP_SET_SIG2				= 0x04000,	    // Bit 14: Set signal 2
    SP_CLR_SIG3				= 0x08000,	    // Bit 15: Clear signal 3
    SP_SET_SIG3				= 0x10000,	    // Bit 16: Set signal 3
    SP_CLR_SIG4				= 0x20000,	    // Bit 17: Clear signal 4
    SP_SET_SIG4				= 0x40000,	    // Bit 18: Set signal 4
    SP_CLR_SIG5				= 0x80000,	    // Bit 19: Clear signal 5
    SP_SET_SIG5				= 0x100000,	    // Bit 20: Set signal 5
    SP_CLR_SIG6				= 0x200000,	    // Bit 21: Clear signal 6
    SP_SET_SIG6				= 0x400000,	    // Bit 22: Set signal 6
    SP_CLR_SIG7				= 0x800000,	    // Bit 23: Clear signal 7
    SP_SET_SIG7				= 0x1000000,    // Bit 24: Set signal 7

    SP_STATUS_HALT			= 0x001,		// Bit  0: Halt
    SP_STATUS_BROKE			= 0x002,		// Bit  1: Broke
    SP_STATUS_DMA_BUSY		= 0x004,		// Bit  2: DMA busy
    SP_STATUS_DMA_FULL		= 0x008,		// Bit  3: DMA full
    SP_STATUS_IO_FULL		= 0x010,		// Bit  4: IO full
    SP_STATUS_SSTEP			= 0x020,		// Bit  5: Single step
    SP_STATUS_INTR_BREAK	= 0x040,		// Bit  6: Interrupt on break
    SP_STATUS_SIG0			= 0x080,		// Bit  7: Signal 0 set
    SP_STATUS_SIG1			= 0x100,		// Bit  8: Signal 1 set
    SP_STATUS_SIG2			= 0x200,		// Bit  9: Signal 2 set
    SP_STATUS_SIG3			= 0x400,		// Bit 10: Signal 3 set
    SP_STATUS_SIG4			= 0x800,		// Bit 11: Signal 4 set
    SP_STATUS_SIG5	       = 0x1000,		// Bit 12: Signal 5 set
    SP_STATUS_SIG6	       = 0x2000,		// Bit 13: Signal 6 set
    SP_STATUS_SIG7	       = 0x4000,		// Bit 14: Signal 7 set
};

// Peripheral interface flags
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
    Serial_InterfaceReg();
    Serial_InterfaceReg(const Serial_InterfaceReg&);
    Serial_InterfaceReg& operator=(const Serial_InterfaceReg&);
};

// Serial interface flags
enum
{
    SI_STATUS_DMA_BUSY	=	0x0001,
    SI_STATUS_RD_BUSY   =	0x0002,
    SI_STATUS_DMA_ERROR	=	0x0008,
    SI_STATUS_INTERRUPT	=	0x1000,
};

// Disk interface
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
    Disk_InterfaceReg();
    Disk_InterfaceReg(const Disk_InterfaceReg&);
    Disk_InterfaceReg& operator=(const Disk_InterfaceReg&);
};

// Disk interface flags
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
    public RDRAMRegistersReg,
    public Mips_InterfaceReg,
    public Video_InterfaceReg,
    public AudioInterfaceReg,
    public PeripheralInterfaceReg,
    public RDRAMInterfaceReg,
    public SPRegistersReg,
    public DisplayControlReg,
    public Serial_InterfaceReg,
    public Disk_InterfaceReg
{
public:
    CRegisters(CN64System * System, CSystemEvents * SystemEvents);

    void CheckInterrupts();
    void DoAddressError( bool DelaySlot, uint32_t BadVaddr, bool FromRead );
    void DoBreakException( bool DelaySlot );
    void DoTrapException( bool DelaySlot );
    void DoCopUnusableException( bool DelaySlot, int32_t Coprocessor );
    bool DoIntrException( bool DelaySlot );
    void DoTLBReadMiss(bool DelaySlot, uint32_t BadVaddr);
    void DoTLBWriteMiss(bool DelaySlot, uint32_t BadVaddr);
    void DoSysCallException ( bool DelaySlot);
    void FixFpuLocations();
    void Reset();
    void SetAsCurrentSystem();

    // General registers
    uint32_t m_PROGRAM_COUNTER;
    MIPS_DWORD m_GPR[32];
    uint32_t m_CP0[33];
    MIPS_DWORD m_HI;
    MIPS_DWORD m_LO;
    uint32_t m_LLBit;

    // Floating point registers/information
    uint32_t m_FPCR[32];
    int32_t m_RoundingModel;
    MIPS_DWORD m_FPR[32];
    float * m_FPR_S[32];
    double * m_FPR_D[32];

    // Memory-mapped N64 registers
    uint32_t m_RDRAM_Registers[10];
    uint32_t m_SigProcessor_Interface[10];
    uint32_t m_Display_ControlReg[10];
    uint32_t m_Mips_Interface[4];
    uint32_t m_Video_Interface[14];
    uint32_t m_Audio_Interface[6];
    uint32_t m_Peripheral_Interface[13];
    uint32_t m_RDRAM_Interface[8];
    uint32_t m_SerialInterface[4];
    uint32_t m_DiskInterface[22];
    uint32_t m_AudioIntrReg;
    uint32_t m_GfxIntrReg;
    uint32_t m_RspIntrReg;

private:
    CRegisters();
    CRegisters(const CRegisters&);
    CRegisters& operator=(const CRegisters&);

    bool            m_FirstInterupt;
    CN64System    * m_System;
    CSystemEvents * m_SystemEvents;
};
