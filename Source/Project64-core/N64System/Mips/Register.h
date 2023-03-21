#pragma once

#include <Common/Platform.h>
#include <Project64-core\Logging.h>
#include <Project64-core\N64System\MemoryHandler\AudioInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\CartridgeDomain2Address1Handler.h>
#include <Project64-core\N64System\MemoryHandler\DisplayControlRegHandler.h>
#include <Project64-core\N64System\MemoryHandler\MIPSInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\PeripheralInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\SPRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\SerialInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\VideoInterfaceHandler.h>
#include <Project64-core\N64System\N64Types.h>
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Settings\GameSettings.h>

#pragma warning(push)
#pragma warning(disable : 4201) // Non-standard extension used: nameless struct/union

union COP0Cause
{
    uint64_t Value;

    struct
    {
        unsigned : 2;
        unsigned ExceptionCode : 5;
        unsigned : 1;
        unsigned PendingInterrupts : 8;
        unsigned : 12;
        unsigned CoprocessorUnitNumber : 2;
        unsigned : 1;
        unsigned BranchDelay : 1;
    };
};

union COP0Context
{
    uint64_t Value;

    struct
    {
        unsigned : 4;
        unsigned BadVPN2 : 19;
        unsigned PTEBaseHi : 9;
        unsigned PTEBaseLo : 32;
    };
};

union COP0XContext
{
    uint64_t Value;

    struct
    {
        uint64_t : 4;
        uint64_t BadVPN2 : 27;
        uint64_t R : 2;
        uint64_t PTEBase : 31;
    };
};

union FPStatusReg
{
    uint32_t Value;

    struct
    {
        unsigned RoundingMode : 2;
        unsigned : 22;
        unsigned FlushSubnormals : 1;
        unsigned : 7;
    };

    struct
    {
        unsigned : 2;
        unsigned Inexact : 1;
        unsigned Underflow : 1;
        unsigned Overflow : 1;
        unsigned DivisionByZero : 1;
        unsigned InvalidOperation : 1;
        unsigned : 25;
    } Flags;

    struct
    {
        unsigned : 7;
        unsigned Inexact : 1;
        unsigned Underflow : 1;
        unsigned Overflow : 1;
        unsigned DivisionByZero : 1;
        unsigned InvalidOperation : 1;
        unsigned : 20;
    } Enable;

    struct
    {
        unsigned : 12;
        unsigned Inexact : 1;
        unsigned Underflow : 1;
        unsigned Overflow : 1;
        unsigned DivisionByZero : 1;
        unsigned InvalidOperation : 1;
        unsigned UnimplementedOperation : 1;
        unsigned : 14;
    } Cause;
};

#pragma warning(pop)

// CPO registers by name
class CP0registers
{
protected:
    CP0registers(uint64_t * _CP0);

public:
    uint64_t & INDEX_REGISTER;
    uint64_t & RANDOM_REGISTER;
    uint64_t & ENTRYLO0_REGISTER;
    uint64_t & ENTRYLO1_REGISTER;
    COP0Context & CONTEXT_REGISTER;
    uint64_t & PAGE_MASK_REGISTER;
    uint64_t & WIRED_REGISTER;
    uint64_t & BAD_VADDR_REGISTER;
    uint64_t & COUNT_REGISTER;
    uint64_t & ENTRYHI_REGISTER;
    uint64_t & COMPARE_REGISTER;
    uint64_t & STATUS_REGISTER;
    COP0Cause & CAUSE_REGISTER;
    uint64_t & EPC_REGISTER;
    uint64_t & PREVID_REGISTER;
    uint64_t & CONFIG_REGISTER;
    COP0XContext & XCONTEXT_REGISTER;
    uint64_t & TAGLO_REGISTER;
    uint64_t & TAGHI_REGISTER;
    uint64_t & ERROREPC_REGISTER;

private:
    CP0registers();
    CP0registers(const CP0registers &);
    CP0registers & operator=(const CP0registers &);
};

// CPO register flags
enum
{
    // Status register
    STATUS_IE = 0x00000001,
    STATUS_EXL = 0x00000002,
    STATUS_ERL = 0x00000004,
    STATUS_IP0 = 0x00000100,
    STATUS_IP1 = 0x00000200,
    STATUS_IP2 = 0x00000400,
    STATUS_IP3 = 0x00000800,
    STATUS_IP4 = 0x00001000,
    STATUS_IP5 = 0x00002000,
    STATUS_IP6 = 0x00004000,
    STATUS_IP7 = 0x00008000,
    STATUS_BEV = 0x00400000,
    STATUS_FR = 0x04000000,
    STATUS_CU0 = 0x10000000,
    STATUS_CU1 = 0x20000000,
    STATUS_CU2 = 0x40000000,
    STATUS_CU3 = 0x80000000,

    // Cause flags
    CAUSE_IP0 = 0x1,
    CAUSE_IP1 = 0x2,
    CAUSE_IP2 = 0x4,
    CAUSE_IP3 = 0x8,
    CAUSE_IP4 = 0x10,
    CAUSE_IP5 = 0x20,
    CAUSE_IP6 = 0x40,
    CAUSE_IP7 = 0x80,

    // Cause exception ID's
    EXC_INT = 0,     // Interrupt
    EXC_MOD = 1,     // TLB mod
    EXC_RMISS = 2,   // Read TLB miss
    EXC_WMISS = 3,   // Write TLB miss
    EXC_RADE = 4,    // Read address error
    EXC_WADE = 5,    // Write address error
    EXC_IBE = 6,     // Instruction bus error
    EXC_DBE = 7,     // Data bus error
    EXC_SYSCALL = 8, // Syscall
    EXC_BREAK = 9,   // Breakpoint
    EXC_II = 10,     // Illegal instruction
    EXC_CPU = 11,    // Co-processor unusable
    EXC_OV = 12,     // Overflow
    EXC_TRAP = 13,   // Trap exception
    EXC_VCEI = 14,   // Virtual coherency on instruction fetch
    EXC_FPE = 15,    // Floating point exception
    EXC_WATCH = 23,  // Watchpoint reference
    EXC_VCED = 31,   // Virtual coherency on data read
};

// Float point control status register flags
enum
{
    FPCSR_FS = 0x01000000,      // Flush denormalization to zero
    FPCSR_C = 0x00800000,       // Condition bit
    FPCSR_CE = 0x00020000,      // Cause: unimplemented operation
    FPCSR_CV = 0x00010000,      // Cause: invalid operation
    FPCSR_CZ = 0x00008000,      // Cause: division by zero
    FPCSR_CO = 0x00004000,      // Cause: overflow
    FPCSR_CU = 0x00002000,      // Cause: underflow
    FPCSR_CI = 0x00001000,      // Cause: inexact operation
    FPCSR_EV = 0x00000800,      // Enable: invalid operation
    FPCSR_EZ = 0x00000400,      // Enable: division by zero
    FPCSR_EO = 0x00000200,      // Enable: overflow
    FPCSR_EU = 0x00000100,      // Enable: underflow
    FPCSR_EI = 0x00000080,      // Enable: inexact operation
    FPCSR_FV = 0x00000040,      // Flag: invalid operation
    FPCSR_FZ = 0x00000020,      // Flag: division by zero
    FPCSR_FO = 0x00000010,      // Flag: overflow
    FPCSR_FU = 0x00000008,      // Flag: underflow
    FPCSR_FI = 0x00000004,      // Flag: inexact operation
    FPCSR_RM_MASK = 0x00000003, // Rounding mode mask
    FPCSR_RM_RN = 0x00000000,   // Round to nearest
    FPCSR_RM_RZ = 0x00000001,   // Round to zero
    FPCSR_RM_RP = 0x00000002,   // Round to positive infinity
    FPCSR_RM_RM = 0x00000003,   // Round to negative infinity
};

// MIPS interface flags
enum
{
    MI_MODE_INIT = 0x0080,  // Bit  7: Initialization mode
    MI_MODE_EBUS = 0x0100,  // Bit  8: EBUS test mode
    MI_MODE_RDRAM = 0x0200, // Bit  9: RDRAM register mode

    MI_CLR_INIT = 0x0080,    // Bit  7: Clear initialization mode
    MI_SET_INIT = 0x0100,    // Bit  8: Set initialization mode
    MI_CLR_EBUS = 0x0200,    // Bit  9: Clear EBUS test
    MI_SET_EBUS = 0x0400,    // Bit 10: Set EBUS test mode
    MI_CLR_DP_INTR = 0x0800, // Bit 11: Clear DP interrupt
    MI_CLR_RDRAM = 0x1000,   // Bit 12: Clear RDRAM register
    MI_SET_RDRAM = 0x2000,   // Bit 13: Set RDRAM register mode

    // Flags for writing to MI_INTR_MASK_REG
    MI_INTR_MASK_CLR_SP = 0x0001, // Bit  0: Clear SP mask
    MI_INTR_MASK_SET_SP = 0x0002, // Bit  1: Set SP mask
    MI_INTR_MASK_CLR_SI = 0x0004, // Bit  2: Clear SI mask
    MI_INTR_MASK_SET_SI = 0x0008, // Bit  3: Set SI mask
    MI_INTR_MASK_CLR_AI = 0x0010, // Bit  4: Clear AI mask
    MI_INTR_MASK_SET_AI = 0x0020, // Bit  5: Set AI mask
    MI_INTR_MASK_CLR_VI = 0x0040, // Bit  6: Clear VI mask
    MI_INTR_MASK_SET_VI = 0x0080, // Bit  7: Set VI mask
    MI_INTR_MASK_CLR_PI = 0x0100, // Bit  8: Clear PI mask
    MI_INTR_MASK_SET_PI = 0x0200, // Bit  9: Set PI mask
    MI_INTR_MASK_CLR_DP = 0x0400, // Bit 10: Clear DP mask
    MI_INTR_MASK_SET_DP = 0x0800, // Bit 11: Set DP mask

    // Flags for reading from MI_INTR_MASK_REG
    MI_INTR_MASK_SP = 0x01, // Bit 0: SP INTR mask
    MI_INTR_MASK_SI = 0x02, // Bit 1: SI INTR mask
    MI_INTR_MASK_AI = 0x04, // Bit 2: AI INTR mask
    MI_INTR_MASK_VI = 0x08, // Bit 3: VI INTR mask
    MI_INTR_MASK_PI = 0x10, // Bit 4: PI INTR mask
    MI_INTR_MASK_DP = 0x20, // Bit 5: DP INTR mask

    MI_INTR_SP = 0x01, // Bit 0: SP INTR
    MI_INTR_SI = 0x02, // Bit 1: SI INTR
    MI_INTR_AI = 0x04, // Bit 2: AI INTR
    MI_INTR_VI = 0x08, // Bit 3: VI INTR
    MI_INTR_PI = 0x10, // Bit 4: PI INTR
    MI_INTR_DP = 0x20, // Bit 5: DP INTR
};

// Signal processor interface flags
enum
{
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
};

class CRegName
{
public:
    static const char * GPR[32];
    static const char * GPR_Hi[32];
    static const char * GPR_Lo[32];
    static const char * Cop0[32];
    static const char * FPR[32];
    static const char * FPR_Ctrl[32];
};

class CSystemRegisters
{
protected:
    static uint32_t * _PROGRAM_COUNTER;
    static MIPS_DWORD * _GPR;
    static MIPS_DWORD * _FPR;
    static uint64_t * _CP0;
    static MIPS_DWORD * _RegHI;
    static MIPS_DWORD * _RegLO;
    static float ** _FPR_S;
    static double ** _FPR_D;
    static uint32_t * _FPCR;
    static uint32_t * _LLBit;
    static int32_t * _RoundingModel;
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
    public MIPSInterfaceReg,
    public VideoInterfaceReg,
    public AudioInterfaceReg,
    public PeripheralInterfaceReg,
    public RDRAMInterfaceReg,
    public SPRegistersReg,
    public DisplayControlReg,
    public SerialInterfaceReg,
    public DiskInterfaceReg
{
public:
    enum COP0Reg : uint32_t
    {
        COP0Reg_Index = 0,
        COP0Reg_Random = 1,
        COP0Reg_EntryLo0 = 2,
        COP0Reg_EntryLo1 = 3,
        COP0Reg_Context = 4,
        COP0Reg_PageMask = 5,
        COP0Reg_Wired = 6,
        COP0Reg_7 = 7,
        COP0Reg_BadVAddr = 8,
        COP0Reg_Count = 9,
        COP0Reg_EntryHi = 10,
        COP0Reg_Compare = 11,
        COP0Reg_Status = 12,
        COP0Reg_Cause = 13,
        COP0Reg_EPC = 14,
        COP0Reg_PRId = 15,
        COP0Reg_Config = 16,
        COP0Reg_LLAddr = 17,
        COP0Reg_WatchLo = 18,
        COP0Reg_WatchHi = 19,
        COP0Reg_XContext = 20,
        COP0Reg_21 = 21,
        COP0Reg_22 = 22,
        COP0Reg_23 = 23,
        COP0Reg_24 = 24,
        COP0Reg_25 = 25,
        COP0Reg_ParityError = 26,
        COP0Reg_CacheErr = 27,
        COP0Reg_TagLo = 28,
        COP0Reg_TagHi = 29,
        COP0Reg_ErrEPC = 30,
        COP0Reg_31 = 31,
    };
    CRegisters(CN64System * System, CSystemEvents * SystemEvents);

    void CheckInterrupts();
    void DoAddressError(bool DelaySlot, uint64_t BadVaddr, bool FromRead);
    void DoBreakException(bool DelaySlot);
    void DoFloatingPointException(bool DelaySlot);
    void DoTrapException(bool DelaySlot);
    void DoCopUnusableException(bool DelaySlot, int32_t Coprocessor);
    bool DoIntrException(bool DelaySlot);
    void DoIllegalInstructionException(bool DelaySlot);
    void DoOverflowException(bool DelaySlot);
    void DoTLBReadMiss(bool DelaySlot, uint64_t BadVaddr);
    void DoTLBWriteMiss(bool DelaySlot, uint64_t BadVaddr);
    void DoSysCallException(bool DelaySlot);
    void FixFpuLocations();
    void Reset();
    void SetAsCurrentSystem();

    uint64_t Cop0_MF(COP0Reg Reg);
    void Cop0_MT(COP0Reg Reg, uint64_t Value);
    void Cop1_CT(uint32_t Reg, uint32_t Value);

    // General registers
    uint32_t m_PROGRAM_COUNTER;
    MIPS_DWORD m_GPR[32];
    uint64_t m_CP0[32];
    uint64_t m_CP0Latch;
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
    CRegisters(const CRegisters &);
    CRegisters & operator=(const CRegisters &);

    bool m_FirstInterupt;
    CN64System * m_System;
    CSystemEvents * m_SystemEvents;
};
