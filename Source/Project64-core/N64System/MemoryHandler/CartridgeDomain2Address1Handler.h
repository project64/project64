#pragma once
#include "MemoryHandler.h"
#include <Project64-core\Settings\DebugSettings.h>
#include <Project64-core\Settings\GameSettings.h>

enum
{
    DD_STATUS_DATA_RQ = 0x40000000,
    DD_STATUS_C2_XFER = 0x10000000,
    DD_STATUS_BM_ERR = 0x08000000,
    DD_STATUS_BM_INT = 0x04000000,
    DD_STATUS_MECHA_INT = 0x02000000,
    DD_STATUS_DISK_PRES = 0x01000000,
    DD_STATUS_BUSY_STATE = 0x00800000,
    DD_STATUS_RST_STATE = 0x00400000,
    DD_STATUS_MTR_N_SPIN = 0x00100000,
    DD_STATUS_HEAD_RTRCT = 0x00080000,
    DD_STATUS_WR_PR_ERR = 0x00040000,
    DD_STATUS_MECHA_ERR = 0x00020000,
    DD_STATUS_DISK_CHNG = 0x00010000,

    DD_BM_STATUS_RUNNING = 0x80000000,
    DD_BM_STATUS_ERROR = 0x04000000,
    DD_BM_STATUS_MICRO = 0x02000000,
    DD_BM_STATUS_BLOCK = 0x01000000,

    DD_BM_CTL_START = 0x80000000,
    DD_BM_CTL_MNGRMODE = 0x40000000,
    DD_BM_CTL_INTMASK = 0x20000000,
    DD_BM_CTL_RESET = 0x10000000,
    DD_BM_CTL_BLK_TRANS = 0x02000000,
    DD_BM_CTL_MECHA_RST = 0x01000000
};

class DiskInterfaceReg
{
protected:
    DiskInterfaceReg(uint32_t * DiskInterface);

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
    DiskInterfaceReg();
    DiskInterfaceReg(const DiskInterfaceReg&);
    DiskInterfaceReg& operator=(const DiskInterfaceReg&);
};

class CRegisters;

class CartridgeDomain2Address1Handler :
    public MemoryHandler,
    private DiskInterfaceReg,
    private CDebugSettings,
    private CGameSettings
{
public:
    CartridgeDomain2Address1Handler(CRegisters & Reg);

    bool Read32(uint32_t Address, uint32_t & Value);
    bool Write32(uint32_t Address, uint32_t Value, uint32_t Mask);

private:
    CartridgeDomain2Address1Handler();
    CartridgeDomain2Address1Handler(const CartridgeDomain2Address1Handler &);
    CartridgeDomain2Address1Handler & operator=(const CartridgeDomain2Address1Handler &);
};
