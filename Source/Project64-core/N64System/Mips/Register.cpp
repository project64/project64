#include "stdafx.h"

#include <Project64-core/Logging.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/N64System/N64Disk.h>
#include <Project64-core/N64System/N64Rom.h>
#include <Project64-core/N64System/N64System.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <fenv.h>

const char * CRegName::GPR[32] = {
    "R0",
    "AT",
    "V0",
    "V1",
    "A0",
    "A1",
    "A2",
    "A3",
    "T0",
    "T1",
    "T2",
    "T3",
    "T4",
    "T5",
    "T6",
    "T7",
    "S0",
    "S1",
    "S2",
    "S3",
    "S4",
    "S5",
    "S6",
    "S7",
    "T8",
    "T9",
    "K0",
    "K1",
    "GP",
    "SP",
    "FP",
    "RA",
};

const char * CRegName::GPR_Hi[32] = {
    "r0.HI",
    "at.HI",
    "v0.HI",
    "v1.HI",
    "a0.HI",
    "a1.HI",
    "a2.HI",
    "a3.HI",
    "t0.HI",
    "t1.HI",
    "t2.HI",
    "t3.HI",
    "t4.HI",
    "t5.HI",
    "t6.HI",
    "t7.HI",
    "s0.HI",
    "s1.HI",
    "s2.HI",
    "s3.HI",
    "s4.HI",
    "s5.HI",
    "s6.HI",
    "s7.HI",
    "t8.HI",
    "t9.HI",
    "k0.HI",
    "k1.HI",
    "gp.HI",
    "sp.HI",
    "fp.HI",
    "ra.HI",
};

const char * CRegName::GPR_Lo[32] = {
    "r0.LO",
    "at.LO",
    "v0.LO",
    "v1.LO",
    "a0.LO",
    "a1.LO",
    "a2.LO",
    "a3.LO",
    "t0.LO",
    "t1.LO",
    "t2.LO",
    "t3.LO",
    "t4.LO",
    "t5.LO",
    "t6.LO",
    "t7.LO",
    "s0.LO",
    "s1.LO",
    "s2.LO",
    "s3.LO",
    "s4.LO",
    "s5.LO",
    "s6.LO",
    "s7.LO",
    "t8.LO",
    "t9.LO",
    "k0.LO",
    "k1.LO",
    "gp.LO",
    "sp.LO",
    "fp.LO",
    "ra.LO",
};

const char * CRegName::Cop0[32] = {
    "Index",
    "Random",
    "EntryLo0",
    "EntryLo1",
    "Context",
    "PageMask",
    "Wired",
    "Reg7",
    "BadVAddr",
    "Count",
    "EntryHi",
    "Compare",
    "Status",
    "Cause",
    "EPC",
    "PRId",
    "Config",
    "LLAddr",
    "WatchLo",
    "WatchHi",
    "XContext",
    "Reg21",
    "Reg22",
    "Reg23",
    "Reg24",
    "Reg25",
    "ECC",
    "CacheErr",
    "TagLo",
    "TagHi",
    "ErrEPC",
    "Reg31",
};

const char * CRegName::FPR[32] = {
    "F0",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "F25",
    "F26",
    "F27",
    "F28",
    "F29",
    "F30",
    "F31",
};

const char * CRegName::FPR_Ctrl[32] = {
    "Revision",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "FCSR",
};

uint32_t * CSystemRegisters::_PROGRAM_COUNTER = nullptr;
MIPS_DWORD * CSystemRegisters::_GPR = nullptr;
MIPS_DWORD * CSystemRegisters::_FPR = nullptr;
uint64_t * CSystemRegisters::_CP0 = nullptr;
MIPS_DWORD * CSystemRegisters::_RegHI = nullptr;
MIPS_DWORD * CSystemRegisters::_RegLO = nullptr;
float ** CSystemRegisters::_FPR_S;
double ** CSystemRegisters::_FPR_D;
uint32_t * CSystemRegisters::_FPCR = nullptr;
uint32_t * CSystemRegisters::_LLBit = nullptr;

CP0registers::CP0registers(uint64_t * _CP0) :
    INDEX_REGISTER(_CP0[0]),
    RANDOM_REGISTER(_CP0[1]),
    ENTRYLO0_REGISTER(_CP0[2]),
    ENTRYLO1_REGISTER(_CP0[3]),
    CONTEXT_REGISTER((COP0Context &)_CP0[4]),
    PAGE_MASK_REGISTER(_CP0[5]),
    WIRED_REGISTER(_CP0[6]),
    BAD_VADDR_REGISTER(_CP0[8]),
    COUNT_REGISTER(_CP0[9]),
    ENTRYHI_REGISTER(_CP0[10]),
    COMPARE_REGISTER(_CP0[11]),
    STATUS_REGISTER((COP0Status &)_CP0[12]),
    CAUSE_REGISTER((COP0Cause &)_CP0[13]),
    EPC_REGISTER(_CP0[14]),
    PREVID_REGISTER(_CP0[15]),
    CONFIG_REGISTER(_CP0[16]),
    XCONTEXT_REGISTER((COP0XContext &)_CP0[20]),
    TAGLO_REGISTER(_CP0[28]),
    TAGHI_REGISTER(_CP0[29]),
    ERROREPC_REGISTER(_CP0[30])
{
}

DisplayControlReg::DisplayControlReg(uint32_t * _DisplayProcessor) :
    DPC_START_REG(_DisplayProcessor[0]),
    DPC_END_REG(_DisplayProcessor[1]),
    DPC_CURRENT_REG(_DisplayProcessor[2]),
    DPC_STATUS_REG(_DisplayProcessor[3]),
    DPC_CLOCK_REG(_DisplayProcessor[4]),
    DPC_BUFBUSY_REG(_DisplayProcessor[5]),
    DPC_PIPEBUSY_REG(_DisplayProcessor[6]),
    DPC_TMEM_REG(_DisplayProcessor[7])
{
}

CRegisters::CRegisters(CN64System & System, CSystemEvents & SystemEvents) :
    CP0registers(m_CP0),
    RDRAMRegistersReg(m_RDRAM_Registers),
    MIPSInterfaceReg(m_Mips_Interface),
    VideoInterfaceReg(m_Video_Interface),
    AudioInterfaceReg(m_Audio_Interface),
    PeripheralInterfaceReg(m_Peripheral_Interface),
    RDRAMInterfaceReg(m_RDRAM_Interface),
    SPRegistersReg(m_SigProcessor_Interface),
    DisplayControlReg(m_Display_ControlReg),
    SerialInterfaceReg(m_SerialInterface),
    DiskInterfaceReg(m_DiskInterface),
    m_System(System),
    m_SystemEvents(SystemEvents),
    m_SystemTimer(System.m_SystemTimer)
{
    Init();
}

void CRegisters::Init(void)
{
    m_FirstInterupt = true;

    memset(m_GPR, 0, sizeof(m_GPR));
    memset(m_CP0, 0, sizeof(m_CP0));
    memset(m_FPR, 0, sizeof(m_FPR));
    memset(m_FPCR, 0, sizeof(m_FPCR));
    m_FPCR[0] = 0xA00;
    m_CP0Latch = 0;
    m_CP2Latch = 0;
    m_HI.DW = 0;
    m_LO.DW = 0;
    m_LLBit = 0;

    // Reset system registers
    memset(m_RDRAM_Interface, 0, sizeof(m_RDRAM_Interface));
    memset(m_RDRAM_Registers, 0, sizeof(m_RDRAM_Registers));
    memset(m_Mips_Interface, 0, sizeof(m_Mips_Interface));
    memset(m_Video_Interface, 0, sizeof(m_Video_Interface));
    memset(m_Display_ControlReg, 0, sizeof(m_Display_ControlReg));
    memset(m_Audio_Interface, 0, sizeof(m_Audio_Interface));
    memset(m_SigProcessor_Interface, 0, sizeof(m_SigProcessor_Interface));
    memset(m_Peripheral_Interface, 0, sizeof(m_Peripheral_Interface));
    memset(m_SerialInterface, 0, sizeof(m_SerialInterface));
    memset(m_DiskInterface, 0, sizeof(m_DiskInterface));

    m_AudioIntrReg = 0;
    m_GfxIntrReg = 0;
    m_RspIntrReg = 0;

    FixFpuLocations();
}

void CRegisters::Reset(bool bPostPif, CMipsMemoryVM & MMU)
{
    Init();

    // COP0 registers
    RANDOM_REGISTER = 0x1F;
    COUNT_REGISTER = 0x5000;
    MI_VERSION_REG = 0x02020102;
    SP_STATUS_REG = 0x00000001;
    CAUSE_REGISTER.Value = 0x0000005C;
    CONTEXT_REGISTER.Value = 0x007FFFF0;
    EPC_REGISTER = 0xFFFFFFFFFFFFFFFF;
    BAD_VADDR_REGISTER = 0xFFFFFFFFFFFFFFFF;
    ERROREPC_REGISTER = 0xFFFFFFFFFFFFFFFF;
    PREVID_REGISTER = 0x00000B22;
    CONFIG_REGISTER = 0x7006E463;
    STATUS_REGISTER.Value = 0x34000000;

    // N64DD registers

    // Start N64DD in reset state and motor not spinning
    ASIC_STATUS = DD_STATUS_RST_STATE | DD_STATUS_MTR_N_SPIN;
    ASIC_ID_REG = 0x00030000;
    if (g_DDRom && (g_DDRom->CicChipID() == CIC_NUS_8401 || (g_Disk && g_Disk->GetCountry() == Country_Unknown)))
        ASIC_ID_REG = 0x00040000;

    //REVISION_REGISTER   = 0x00000511;
    FixFpuLocations();

    if (bPostPif)
    {
        m_PROGRAM_COUNTER = 0xA4000040;

        m_GPR[0].DW = 0x0000000000000000;
        m_GPR[6].DW = 0xFFFFFFFFA4001F0C;
        m_GPR[7].DW = 0xFFFFFFFFA4001F08;
        m_GPR[8].DW = 0x00000000000000C0;
        m_GPR[9].DW = 0x0000000000000000;
        m_GPR[10].DW = 0x0000000000000040;
        m_GPR[11].DW = 0xFFFFFFFFA4000040;
        m_GPR[16].DW = 0x0000000000000000;
        m_GPR[17].DW = 0x0000000000000000;
        m_GPR[18].DW = 0x0000000000000000;
        m_GPR[19].DW = 0x0000000000000000;
        m_GPR[21].DW = 0x0000000000000000;
        m_GPR[26].DW = 0x0000000000000000;
        m_GPR[27].DW = 0x0000000000000000;
        m_GPR[28].DW = 0x0000000000000000;
        m_GPR[29].DW = 0xFFFFFFFFA4001FF0;
        m_GPR[30].DW = 0x0000000000000000;

        if (g_Rom->IsPal())
        {
            switch (g_Rom->CicChipID())
            {
            case CIC_UNKNOWN:
            case CIC_NUS_6102:
            case CIC_MINI_IPL3:
                m_GPR[5].DW = 0xFFFFFFFFC0F1D859;
                m_GPR[14].DW = 0x000000002DE108EA;
                m_GPR[24].DW = 0x0000000000000000;
                break;
            case CIC_NUS_6103:
                m_GPR[5].DW = 0xFFFFFFFFD4646273;
                m_GPR[14].DW = 0x000000001AF99984;
                m_GPR[24].DW = 0x0000000000000000;
                break;
            case CIC_NUS_6105:
                MMU.UpdateMemoryValue32(0xA4001004, 0xBDA807FC);
                m_GPR[5].DW = 0xFFFFFFFFDECAAAD1;
                m_GPR[14].DW = 0x000000000CF85C13;
                m_GPR[24].DW = 0x0000000000000002;
                break;
            case CIC_NUS_6106:
                m_GPR[5].DW = 0xFFFFFFFFB04DC903;
                m_GPR[14].DW = 0x000000001AF99984;
                m_GPR[24].DW = 0x0000000000000002;
                break;
            }
            m_GPR[20].DW = 0x0000000000000000;
            m_GPR[23].DW = 0x0000000000000006;
            m_GPR[31].DW = 0xFFFFFFFFA4001554;
        }
        else
        {
            switch (g_Rom->CicChipID())
            {
            case CIC_UNKNOWN:
            case CIC_NUS_6102:
            case CIC_MINI_IPL3:
                m_GPR[5].DW = 0xFFFFFFFFC95973D5;
                m_GPR[14].DW = 0x000000002449A366;
                break;
            case CIC_NUS_6103:
                m_GPR[5].DW = 0xFFFFFFFF95315A28;
                m_GPR[14].DW = 0x000000005BACA1DF;
                break;
            case CIC_NUS_6105:
                MMU.UpdateMemoryValue32(0xA4001004, 0x8DA807FC);
                m_GPR[5].DW = 0x000000005493FB9A;
                m_GPR[14].DW = 0xFFFFFFFFC2C20384;
            case CIC_NUS_6106:
                m_GPR[5].DW = 0xFFFFFFFFE067221F;
                m_GPR[14].DW = 0x000000005CD2B70F;
                break;
            case CIC_NUS_6101:
            case CIC_NUS_6104:
            case CIC_NUS_5167:
            case CIC_NUS_8303:
            case CIC_NUS_DDUS:
            case CIC_NUS_8401:
            case CIC_NUS_5101:
            default:
                // No specific values
                break;
            }
            m_GPR[20].DW = 0x0000000000000001;
            m_GPR[23].DW = 0x0000000000000000;
            m_GPR[24].DW = 0x0000000000000003;
            m_GPR[31].DW = 0xFFFFFFFFA4001550;
        }

        switch (g_Rom->CicChipID())
        {
        case CIC_NUS_6101:
            m_GPR[22].DW = 0x000000000000003F;
            break;
        case CIC_NUS_8303: // 64DD IPL CIC
        case CIC_NUS_8401: // 64DD IPL tool CIC
        case CIC_NUS_5167: // 64DD conversion CIC
            m_GPR[22].DW = 0x00000000000000DD;
            break;
        case CIC_NUS_DDUS: // 64DD US IPL CIC
            m_GPR[22].DW = 0x00000000000000DE;
            break;
        case CIC_NUS_5101: // Aleck64 CIC
            m_GPR[22].DW = 0x00000000000000AC;
            break;
        case CIC_UNKNOWN:
        case CIC_NUS_6102:
        case CIC_MINI_IPL3:
            m_GPR[1].DW = 0x0000000000000001;
            m_GPR[2].DW = 0x000000000EBDA536;
            m_GPR[3].DW = 0x000000000EBDA536;
            m_GPR[4].DW = 0x000000000000A536;
            m_GPR[12].DW = 0xFFFFFFFFED10D0B3;
            m_GPR[13].DW = 0x000000001402A4CC;
            m_GPR[15].DW = 0x000000003103E121;
            m_GPR[22].DW = 0x000000000000003F;
            m_GPR[25].DW = 0xFFFFFFFF9DEBB54F;
            break;
        case CIC_NUS_6103:
            m_GPR[1].DW = 0x0000000000000001;
            m_GPR[2].DW = 0x0000000049A5EE96;
            m_GPR[3].DW = 0x0000000049A5EE96;
            m_GPR[4].DW = 0x000000000000EE96;
            m_GPR[12].DW = 0xFFFFFFFFCE9DFBF7;
            m_GPR[13].DW = 0xFFFFFFFFCE9DFBF7;
            m_GPR[15].DW = 0x0000000018B63D28;
            m_GPR[22].DW = 0x0000000000000078;
            m_GPR[25].DW = 0xFFFFFFFF825B21C9;
            break;
        case CIC_NUS_6105:
            MMU.UpdateMemoryValue32(0xA4001000, 0x3C0DBFC0);
            MMU.UpdateMemoryValue32(0xA4001008, 0x25AD07C0);
            MMU.UpdateMemoryValue32(0xA400100C, 0x31080080);
            MMU.UpdateMemoryValue32(0xA4001010, 0x5500FFFC);
            MMU.UpdateMemoryValue32(0xA4001014, 0x3C0DBFC0);
            MMU.UpdateMemoryValue32(0xA4001018, 0x8DA80024);
            MMU.UpdateMemoryValue32(0xA400101C, 0x3C0BB000);
            m_GPR[1].DW = 0x0000000000000000;
            m_GPR[2].DW = 0xFFFFFFFFF58B0FBF;
            m_GPR[3].DW = 0xFFFFFFFFF58B0FBF;
            m_GPR[4].DW = 0x0000000000000FBF;
            m_GPR[12].DW = 0xFFFFFFFF9651F81E;
            m_GPR[13].DW = 0x000000002D42AAC5;
            m_GPR[15].DW = 0x0000000056584D60;
            m_GPR[22].DW = 0x0000000000000091;
            m_GPR[25].DW = 0xFFFFFFFFCDCE565F;
            break;
        case CIC_NUS_6106:
            m_GPR[1].DW = 0x0000000000000000;
            m_GPR[2].DW = 0xFFFFFFFFA95930A4;
            m_GPR[3].DW = 0xFFFFFFFFA95930A4;
            m_GPR[4].DW = 0x00000000000030A4;
            m_GPR[12].DW = 0xFFFFFFFFBCB59510;
            m_GPR[13].DW = 0xFFFFFFFFBCB59510;
            m_GPR[15].DW = 0x000000007A3C07F4;
            m_GPR[22].DW = 0x0000000000000085;
            m_GPR[25].DW = 0x00000000465E3F72;
            break;
        }
    }
    else
    {
        m_PROGRAM_COUNTER = 0xBFC00000;
        /*        PIF_Ram[36] = 0x00; PIF_Ram[39] = 0x3F; // Common PIF RAM start values

        switch (g_Rom->CicChipID()) {
        case CIC_NUS_6101: PIF_Ram[37] = 0x06; PIF_Ram[38] = 0x3F; break;
        case CIC_UNKNOWN:
        case CIC_NUS_6102: PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x3F; break;
        case CIC_NUS_6103:    PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x78; break;
        case CIC_NUS_6105:    PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x91; break;
        case CIC_NUS_6106:    PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x85; break;
        }*/
    }
}

void CRegisters::SetAsCurrentSystem()
{
    _PROGRAM_COUNTER = &m_PROGRAM_COUNTER;
    _GPR = m_GPR;
    _FPR = m_FPR;
    _CP0 = m_CP0;
    _RegHI = &m_HI;
    _RegLO = &m_LO;
    _FPR_S = m_FPR_S;
    _FPR_D = m_FPR_D;
    _FPCR = m_FPCR;
    _LLBit = &m_LLBit;
}

uint64_t CRegisters::Cop0_MF(COP0Reg Reg)
{
    if (LogCP0reads() && Reg <= COP0Reg_31)
    {
        LogMessage("%08X: R4300i read from %s (0x%08X)", (*_PROGRAM_COUNTER), CRegName::Cop0[Reg], m_CP0[Reg]);
    }

    if (Reg == COP0Reg_Count || Reg == COP0Reg_Wired || Reg == COP0Reg_Random)
    {
        m_SystemTimer.UpdateTimers();
    }
    else if (Reg == COP0Reg_7 || Reg == COP0Reg_21 || Reg == COP0Reg_22 || Reg == COP0Reg_23 || Reg == COP0Reg_24 || Reg == COP0Reg_25 || Reg == COP0Reg_31)
    {
        // Unused registers
        return m_CP0Latch;
    }
    return Reg <= COP0Reg_31 ? m_CP0[Reg] : 0;
}

void CRegisters::Cop0_MT(COP0Reg Reg, uint64_t Value)
{
    if (LogCP0changes() && Reg <= COP0Reg_31)
    {
        LogMessage("%08X: Writing 0x%I64U to %s register (originally: 0x%I64U)", (*_PROGRAM_COUNTER), Value, CRegName::Cop0[Reg], m_CP0[Reg]);
        if (Reg == 11) // Compare
        {
            LogMessage("%08X: Cause register changed from %08X to %08X", (*_PROGRAM_COUNTER), (uint32_t)CAUSE_REGISTER.Value, (uint32_t)(g_Reg->CAUSE_REGISTER.Value & ~CAUSE_IP7));
        }
    }
    m_CP0Latch = Value;

    switch (Reg)
    {
    case COP0Reg_EPC:
    case COP0Reg_WatchLo:
    case COP0Reg_WatchHi:
    case COP0Reg_TagLo:
    case COP0Reg_TagHi:
    case COP0Reg_ErrEPC:
    case COP0Reg_31:
        m_CP0[Reg] = Value;
        break;
    case COP0Reg_Index:
        m_CP0[Reg] = Value & 0x8000003F;
        break;
    case COP0Reg_Random:
        // Ignore - Read only
        break;
    case COP0Reg_EntryLo0:
    case COP0Reg_EntryLo1:
        m_CP0[Reg] = Value & 0x3FFFFFFF;
        break;
    case COP0Reg_Context:
        m_CP0[Reg] = (Value & 0xFFFFFFFFFF800000) | (m_CP0[Reg] & 0x7FFFF0);
        break;
    case COP0Reg_PageMask:
        m_CP0[Reg] = Value & 0x1FFE000;
        break;
    case COP0Reg_Wired:
        m_SystemTimer.UpdateTimers();
        m_CP0[Reg] = Value & 0x3F;
        break;
    case COP0Reg_7:
    case COP0Reg_BadVAddr:
        // Ignore - Read only
        break;
    case COP0Reg_Count:
        m_SystemTimer.UpdateTimers();
        m_CP0[Reg] = Value;
        m_SystemTimer.UpdateCompareTimer();
        break;
    case COP0Reg_EntryHi:
        m_CP0[Reg] = Value & 0xC00000FFFFFFE0FF;
        break;
    case COP0Reg_Compare:
        m_SystemTimer.UpdateTimers();
        m_CP0[Reg] = Value;
        CAUSE_REGISTER.PendingInterrupts &= ~CAUSE_IP7;
        m_SystemTimer.UpdateCompareTimer();
        break;
    case COP0Reg_Status:
    {
        bool FRBitChanged = STATUS_REGISTER.FR != ((COP0Status &)Value).FR;
        STATUS_REGISTER.Value = Value & 0xFFF7FFFF;
        if (FRBitChanged)
        {
            FixFpuLocations();
        }
        CheckInterrupts();
        break;
    }
    case COP0Reg_Cause:
        m_CP0[Reg] &= 0xFFFFCFF;
        if ((Value & 0x300) != 0 && HaveDebugger())
        {
            g_Notify->DisplayError("Set IP0 or IP1");
        }
        break;
    case COP0Reg_PRId:
        // Read only
        break;
    case COP0Reg_Config:
        m_CP0[Reg] = (Value & 0x0F00800F) | (m_CP0[Reg] & 0xF0FF7FF0);
        break;
    case COP0Reg_LLAddr:
        m_CP0[Reg] = (Value & 0xFFFFFFFF) | (m_CP0[Reg] & 0xFFFFFFFF00000000);
        break;
    case COP0Reg_XContext:
        m_CP0[Reg] = (Value & 0xFFFFFFFE00000000) | (m_CP0[Reg] & 0x00000001FFFFFFFF);
        break;
    case COP0Reg_21:
    case COP0Reg_22:
    case COP0Reg_23:
    case COP0Reg_24:
    case COP0Reg_25:
        // Unused
        break;
    case COP0Reg_ParityError:
        m_CP0[Reg] = Value & 0xFF;
        break;
    case COP0Reg_CacheErr:
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
}

void CRegisters::Cop1_CT(uint32_t Reg, uint32_t Value)
{
    if (Reg == 31)
    {
        FPStatusReg & StatusReg = (FPStatusReg &)_FPCR[31];
        StatusReg.Value = (Value & 0x183FFFF);

        if (((StatusReg.Cause.Inexact & StatusReg.Enable.Inexact) != 0) ||
            ((StatusReg.Cause.Underflow & StatusReg.Enable.Underflow) != 0) ||
            ((StatusReg.Cause.Overflow & StatusReg.Enable.Overflow) != 0) ||
            ((StatusReg.Cause.DivisionByZero & StatusReg.Enable.DivisionByZero) != 0) ||
            ((StatusReg.Cause.InvalidOperation & StatusReg.Enable.InvalidOperation) != 0) ||
            (StatusReg.Cause.UnimplementedOperation != 0))
        {
            TriggerException(EXC_FPE);
        }
    }
}

uint64_t CRegisters::Cop2_MF(uint32_t /*Reg*/)
{
    return m_CP2Latch;
}

void CRegisters::Cop2_MT(uint32_t /*Reg*/, uint64_t Value)
{
    m_CP2Latch = Value;
}

void CRegisters::CheckInterrupts()
{
    uint32_t MI_INTR_REG_Value = MI_INTR_REG;
    if (!m_System.bFixedAudio() && CpuType() != CPU_SyncCores)
    {
        MI_INTR_REG_Value &= ~MI_INTR_AI;
        MI_INTR_REG_Value |= (m_AudioIntrReg & MI_INTR_AI);
    }
    MI_INTR_REG_Value |= (m_RspIntrReg & MI_INTR_SP);
    MI_INTR_REG_Value |= (m_GfxIntrReg & MI_INTR_DP);
    if ((MI_INTR_MASK_REG & MI_INTR_REG_Value) != 0)
    {
        CAUSE_REGISTER.PendingInterrupts |= CAUSE_IP2;
    }
    else
    {
        CAUSE_REGISTER.PendingInterrupts &= ~CAUSE_IP2;
    }
    MI_INTR_REG = MI_INTR_REG_Value;
    COP0Status STATUS_REGISTER_Value = STATUS_REGISTER;

    if (STATUS_REGISTER_Value.InterruptEnable == 0 || STATUS_REGISTER_Value.ExceptionLevel != 0 || STATUS_REGISTER_Value.ErrorLevel != 0)
    {
        return;
    }

    if ((STATUS_REGISTER_Value.Value & CAUSE_REGISTER.Value & 0xFF00) != 0)
    {
        if (m_FirstInterupt)
        {
            m_FirstInterupt = false;
            if (g_Recompiler)
            {
                g_Recompiler->ClearRecompCode_Virt(0x80000000, 0x200, CRecompiler::Remove_InitialCode);
            }
        }
        m_SystemEvents.QueueEvent(SysEvent_ExecuteInterrupt);
    }
}

void CRegisters::DoAddressError(uint64_t BadVaddr, bool FromRead)
{
    if (BreakOnAddressError())
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

    if (FromRead)
    {
        CAUSE_REGISTER.ExceptionCode = EXC_RADE;
    }
    else
    {
        CAUSE_REGISTER.ExceptionCode = EXC_WADE;
    }
    CAUSE_REGISTER.CoprocessorUnitNumber = 0;
    BAD_VADDR_REGISTER = BadVaddr;
    CONTEXT_REGISTER.BadVPN2 = BadVaddr >> 13;
    XCONTEXT_REGISTER.BadVPN2 = BadVaddr >> 13;
    XCONTEXT_REGISTER.R = BadVaddr >> 61;

    if (m_System.m_PipelineStage == PIPELINE_STAGE_JUMP)
    {
        CAUSE_REGISTER.BranchDelay = 1;
        EPC_REGISTER = (int32_t)(m_PROGRAM_COUNTER - 4);
    }
    else
    {
        CAUSE_REGISTER.BranchDelay = 0;
        EPC_REGISTER = (int32_t)m_PROGRAM_COUNTER;
    }
    STATUS_REGISTER.ExceptionLevel = 1;
    m_System.m_JumpToLocation = 0x80000180;
    m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
}

void CRegisters::FixFpuLocations()
{
    if (STATUS_REGISTER.FR == 0)
    {
        for (int count = 0; count < 32; count++)
        {
            m_FPR_S[count] = &m_FPR[count & ~1].F[count & 1];
            m_FPR_D[count] = &m_FPR[count & ~1].D;
        }
    }
    else
    {
        for (int count = 0; count < 32; count++)
        {
            m_FPR_S[count] = &m_FPR[count].F[0];
            m_FPR_D[count] = &m_FPR[count].D;
        }
    }
}

bool CRegisters::DoIntrException()
{
    if (STATUS_REGISTER.InterruptEnable == 0 || STATUS_REGISTER.ExceptionLevel != 0 || STATUS_REGISTER.ErrorLevel != 0)
    {
        return false;
    }
    TriggerException(EXC_INT, 0);
    return true;
}

void CRegisters::DoTLBReadMiss(bool DelaySlot, uint64_t BadVaddr)
{
    CAUSE_REGISTER.ExceptionCode = EXC_RMISS;
    CAUSE_REGISTER.CoprocessorUnitNumber = 0;
    BAD_VADDR_REGISTER = BadVaddr;
    CONTEXT_REGISTER.BadVPN2 = BadVaddr >> 13;
    ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
    if ((STATUS_REGISTER.ExceptionLevel) == 0)
    {
        if (DelaySlot)
        {
            CAUSE_REGISTER.BranchDelay = 1;
            EPC_REGISTER = (int64_t)((int32_t)m_PROGRAM_COUNTER - 4);
        }
        else
        {
            CAUSE_REGISTER.BranchDelay = 0;
            EPC_REGISTER = (int64_t)((int32_t)m_PROGRAM_COUNTER);
        }
        if (g_TLB->AddressDefined((uint32_t)BadVaddr))
        {
            m_System.m_JumpToLocation = 0x80000180;
        }
        else
        {
            m_System.m_JumpToLocation = 0x80000000;
        }
        STATUS_REGISTER.ExceptionLevel = 1;
    }
    else
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("TLBMiss - EXL set\nBadVaddr = %X\nAddress defined: %s", (uint32_t)BadVaddr, g_TLB->AddressDefined((uint32_t)BadVaddr) ? "true" : "false").c_str());
        }
        m_System.m_JumpToLocation = 0x80000180;
    }
    m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
}

void CRegisters::DoTLBWriteMiss(bool DelaySlot, uint64_t BadVaddr)
{
    CAUSE_REGISTER.ExceptionCode = EXC_WMISS;
    CAUSE_REGISTER.CoprocessorUnitNumber = 0;
    BAD_VADDR_REGISTER = BadVaddr;
    CONTEXT_REGISTER.BadVPN2 = BadVaddr >> 13;
    ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
    if ((STATUS_REGISTER.ExceptionLevel) == 0)
    {
        if (DelaySlot)
        {
            CAUSE_REGISTER.BranchDelay = 1;
            EPC_REGISTER = (int64_t)((int32_t)m_PROGRAM_COUNTER - 4);
        }
        else
        {
            CAUSE_REGISTER.BranchDelay = 0;
            EPC_REGISTER = (int64_t)((int32_t)m_PROGRAM_COUNTER);
        }
        if (g_TLB->AddressDefined((uint32_t)BadVaddr))
        {
            m_PROGRAM_COUNTER = 0x80000180;
        }
        else
        {
            m_PROGRAM_COUNTER = 0x80000000;
        }
        STATUS_REGISTER.ExceptionLevel = 1;
    }
    else
    {
        if (HaveDebugger())
        {
            g_Notify->DisplayError(stdstr_f("TLBMiss - EXL set\nBadVaddr = %X\nAddress defined: %s", (uint32_t)BadVaddr, g_TLB->AddressDefined((uint32_t)BadVaddr) ? "true" : "false").c_str());
        }
        m_PROGRAM_COUNTER = 0x80000180;
    }
}

void CRegisters::TriggerException(uint32_t ExceptionCode, uint32_t Coprocessor)
{
    if (GenerateLog() && LogExceptions())
    {
        if (ExceptionCode != EXC_INT)
        {
            LogMessage("%08X: Exception %d", m_PROGRAM_COUNTER, ExceptionCode);
        }
        else if (!LogNoInterrupts())
        {
            LogMessage("%08X: Interrupt generated", m_PROGRAM_COUNTER);
        }
    }

    CAUSE_REGISTER.ExceptionCode = ExceptionCode;
    CAUSE_REGISTER.CoprocessorUnitNumber = Coprocessor;
    CAUSE_REGISTER.BranchDelay = m_System.m_PipelineStage == PIPELINE_STAGE_JUMP;
    EPC_REGISTER = (int64_t)((int32_t)m_PROGRAM_COUNTER - (CAUSE_REGISTER.BranchDelay ? 4 : 0));
    STATUS_REGISTER.ExceptionLevel = 1;
    m_System.m_PipelineStage = PIPELINE_STAGE_JUMP;
    m_System.m_JumpToLocation = 0x80000180;
}