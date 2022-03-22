#include "stdafx.h"
#include "CartridgeDomain2Address1Handler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\Mips\Disk.h>

DiskInterfaceReg::DiskInterfaceReg(uint32_t * DiskInterface) :
    ASIC_DATA(DiskInterface[0]),
    ASIC_MISC_REG(DiskInterface[1]),
    ASIC_STATUS(DiskInterface[2]),
    ASIC_CUR_TK(DiskInterface[3]),
    ASIC_BM_STATUS(DiskInterface[4]),
    ASIC_ERR_SECTOR(DiskInterface[5]),
    ASIC_SEQ_STATUS(DiskInterface[6]),
    ASIC_CUR_SECTOR(DiskInterface[7]),
    ASIC_HARD_RESET(DiskInterface[8]),
    ASIC_C1_S0(DiskInterface[9]),
    ASIC_HOST_SECBYTE(DiskInterface[10]),
    ASIC_C1_S2(DiskInterface[11]),
    ASIC_SEC_BYTE(DiskInterface[12]),
    ASIC_C1_S4(DiskInterface[13]),
    ASIC_C1_S6(DiskInterface[14]),
    ASIC_CUR_ADDR(DiskInterface[15]),
    ASIC_ID_REG(DiskInterface[16]),
    ASIC_TEST_REG(DiskInterface[17]),
    ASIC_TEST_PIN_SEL(DiskInterface[18]),
    ASIC_CMD(DiskInterface[19]),
    ASIC_BM_CTL(DiskInterface[20]),
    ASIC_SEQ_CTL(DiskInterface[21])
{
}

CartridgeDomain2Address1Handler::CartridgeDomain2Address1Handler(CRegisters & Reg) :
    DiskInterfaceReg(Reg.m_DiskInterface)
{
}

bool CartridgeDomain2Address1Handler::Read32(uint32_t Address, uint32_t & Value)
{
    // 64DD registers
    if (EnableDisk())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x05000500: Value = ASIC_DATA; break;
        case 0x05000504: Value = ASIC_MISC_REG; break;
        case 0x05000508:
            Value = ASIC_STATUS;
            DiskGapSectorCheck();
            break;
        case 0x0500050C: Value = ASIC_CUR_TK; break;
        case 0x05000510: Value = ASIC_BM_STATUS; break;
        case 0x05000514: Value = ASIC_ERR_SECTOR; break;
        case 0x05000518: Value = ASIC_SEQ_STATUS; break;
        case 0x0500051C: Value = ASIC_CUR_SECTOR; break;
        case 0x05000520: Value = ASIC_HARD_RESET; break;
        case 0x05000524: Value = ASIC_C1_S0; break;
        case 0x05000528: Value = ASIC_HOST_SECBYTE; break;
        case 0x0500052C: Value = ASIC_C1_S2; break;
        case 0x05000530: Value = ASIC_SEC_BYTE; break;
        case 0x05000534: Value = ASIC_C1_S4; break;
        case 0x05000538: Value = ASIC_C1_S6; break;
        case 0x0500053C: Value = ASIC_CUR_ADDR; break;
        case 0x05000540: Value = ASIC_ID_REG; break;
        case 0x05000544: Value = ASIC_TEST_REG; break;
        case 0x05000548: Value = ASIC_TEST_PIN_SEL; break;
        default:
            Value = (Address << 16) | (Address & 0xFFFF);
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    else
    {
        Value = (Address << 16) | (Address & 0xFFFF);
    }
    return true;
}

bool CartridgeDomain2Address1Handler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (EnableDisk())
    {
        switch (Address & 0xFFFFFFF)
        {
        case 0x05000500: ASIC_DATA = (ASIC_DATA & ~Mask) | (Value & Mask); break;
        case 0x05000508:
            ASIC_CMD = (ASIC_CMD & ~Mask) | (Value & Mask);
            DiskCommand();
            break;
        case 0x05000510:
            //ASIC_BM_STATUS_CTL
            ASIC_BM_CTL = (ASIC_BM_CTL & ~Mask) | (Value & Mask);
            DiskBMControl();
            break;
        case 0x05000518:
            //ASIC_SEQ_STATUS_CTL
            break;
        case 0x05000520: DiskReset(); break;
        case 0x05000528: ASIC_HOST_SECBYTE = (ASIC_HOST_SECBYTE & ~Mask) | (Value & Mask); break;
        case 0x05000530: ASIC_SEC_BYTE = (ASIC_SEC_BYTE & ~Mask) | (Value & Mask); break;
        case 0x05000548: ASIC_TEST_PIN_SEL = (ASIC_TEST_PIN_SEL & ~Mask) | (Value & Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}
