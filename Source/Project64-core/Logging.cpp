#include "stdafx.h"
#include <Project64-core/Logging.h>

#include <stdio.h>
#include <stdarg.h>
#include <Common/path.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/TranslateVaddr.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/N64RomClass.h>

CFile * CLogging::m_hLogFile = NULL;

void CLogging::Log_LW(uint32_t PC, uint32_t VAddr)
{
    if (!GenerateLog())
    {
        return;
    }

    if (VAddr < 0xA0000000 || VAddr >= 0xC0000000)
    {
        uint32_t PAddr;
        if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
        {
            if (LogUnknown())
            {
                LogMessage("%08X: read from unknown ??? (%08X)", PC, VAddr);
            }
            return;
        }
        VAddr = PAddr + 0xA0000000;
    }

    uint32_t Value;
    if (VAddr >= 0xA0000000 && VAddr < (0xA0000000 + g_MMU->RdramSize()))
    {
        return;
    }
    if (VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024)
    {
        if (!LogRDRamRegisters())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA3F00000: LogMessage("%08X: read from RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG (%08X)", PC, Value); return;
        case 0xA3F00004: LogMessage("%08X: read from RDRAM_DEVICE_ID_REG (%08X)", PC, Value); return;
        case 0xA3F00008: LogMessage("%08X: read from RDRAM_DELAY_REG (%08X)", PC, Value); return;
        case 0xA3F0000C: LogMessage("%08X: read from RDRAM_MODE_REG (%08X)", PC, Value); return;
        case 0xA3F00010: LogMessage("%08X: read from RDRAM_REF_INTERVAL_REG (%08X)", PC, Value); return;
        case 0xA3F00014: LogMessage("%08X: read from RDRAM_REF_ROW_REG (%08X)", PC, Value); return;
        case 0xA3F00018: LogMessage("%08X: read from RDRAM_RAS_INTERVAL_REG (%08X)", PC, Value); return;
        case 0xA3F0001C: LogMessage("%08X: read from RDRAM_MIN_INTERVAL_REG (%08X)", PC, Value); return;
        case 0xA3F00020: LogMessage("%08X: read from RDRAM_ADDR_SELECT_REG (%08X)", PC, Value); return;
        case 0xA3F00024: LogMessage("%08X: read from RDRAM_DEVICE_MANUF_REG (%08X)", PC, Value); return;
        }
    }

    if (VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC)
    {
        return;
    }
    if (VAddr >= 0xA4040000 && VAddr <= 0xA404001C)
    {
        if (!LogSPRegisters())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4040000: LogMessage("%08X: read from SP_MEM_ADDR_REG (%08X)", PC, Value); break;
        case 0xA4040004: LogMessage("%08X: read from SP_DRAM_ADDR_REG (%08X)", PC, Value); break;
        case 0xA4040008: LogMessage("%08X: read from SP_RD_LEN_REG (%08X)", PC, Value); break;
        case 0xA404000C: LogMessage("%08X: read from SP_WR_LEN_REG (%08X)", PC, Value); break;
        case 0xA4040010: LogMessage("%08X: read from SP_STATUS_REG (%08X)", PC, Value); break;
        case 0xA4040014: LogMessage("%08X: read from SP_DMA_FULL_REG (%08X)", PC, Value); break;
        case 0xA4040018: LogMessage("%08X: read from SP_DMA_BUSY_REG (%08X)", PC, Value); break;
        case 0xA404001C: LogMessage("%08X: read from SP_SEMAPHORE_REG (%08X)", PC, Value); break;
        }
        return;
    }
    if (VAddr == 0xA4080000)
    {
        if (!LogSPRegisters())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SP_PC (%08X)", PC, Value);
        return;
    }
    if (VAddr >= 0xA4100000 && VAddr <= 0xA410001C)
    {
        if (!LogDPCRegisters())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4100000: LogMessage("%08X: read from DPC_START_REG (%08X)", PC, Value); return;
        case 0xA4100004: LogMessage("%08X: read from DPC_END_REG (%08X)", PC, Value); return;
        case 0xA4100008: LogMessage("%08X: read from DPC_CURRENT_REG (%08X)", PC, Value); return;
        case 0xA410000C: LogMessage("%08X: read from DPC_STATUS_REG (%08X)", PC, Value); return;
        case 0xA4100010: LogMessage("%08X: read from DPC_CLOCK_REG (%08X)", PC, Value); return;
        case 0xA4100014: LogMessage("%08X: read from DPC_BUFBUSY_REG (%08X)", PC, Value); return;
        case 0xA4100018: LogMessage("%08X: read from DPC_PIPEBUSY_REG (%08X)", PC, Value); return;
        case 0xA410001C: LogMessage("%08X: read from DPC_TMEM_REG (%08X)", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4300000 && VAddr <= 0xA430000C)
    {
        if (!LogMIPSInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4300000: LogMessage("%08X: read from MI_INIT_MODE_REG/MI_MODE_REG (%08X)", PC, Value); return;
        case 0xA4300004: LogMessage("%08X: read from MI_VERSION_REG/MI_NOOP_REG (%08X)", PC, Value); return;
        case 0xA4300008: LogMessage("%08X: read from MI_INTR_REG (%08X)", PC, Value); return;
        case 0xA430000C: LogMessage("%08X: read from MI_INTR_MASK_REG (%08X)", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4400000 && VAddr <= 0xA4400034)
    {
        if (!LogVideoInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4400000: LogMessage("%08X: read from VI_STATUS_REG/VI_CONTROL_REG (%08X)", PC, Value); return;
        case 0xA4400004: LogMessage("%08X: read from VI_ORIGIN_REG/VI_DRAM_ADDR_REG (%08X)", PC, Value); return;
        case 0xA4400008: LogMessage("%08X: read from VI_WIDTH_REG/VI_H_WIDTH_REG (%08X)", PC, Value); return;
        case 0xA440000C: LogMessage("%08X: read from VI_INTR_REG/VI_V_INTR_REG (%08X)", PC, Value); return;
        case 0xA4400010: LogMessage("%08X: read from VI_CURRENT_REG/VI_V_CURRENT_LINE_REG (%08X)", PC, Value); return;
        case 0xA4400014: LogMessage("%08X: read from VI_BURST_REG/VI_TIMING_REG (%08X)", PC, Value); return;
        case 0xA4400018: LogMessage("%08X: read from VI_V_SYNC_REG (%08X)", PC, Value); return;
        case 0xA440001C: LogMessage("%08X: read from VI_H_SYNC_REG (%08X)", PC, Value); return;
        case 0xA4400020: LogMessage("%08X: read from VI_LEAP_REG/VI_H_SYNC_LEAP_REG (%08X)", PC, Value); return;
        case 0xA4400024: LogMessage("%08X: read from VI_H_START_REG/VI_H_VIDEO_REG (%08X)", PC, Value); return;
        case 0xA4400028: LogMessage("%08X: read from VI_V_START_REG/VI_V_VIDEO_REG (%08X)", PC, Value); return;
        case 0xA440002C: LogMessage("%08X: read from VI_V_BURST_REG (%08X)", PC, Value); return;
        case 0xA4400030: LogMessage("%08X: read from VI_X_SCALE_REG (%08X)", PC, Value); return;
        case 0xA4400034: LogMessage("%08X: read from VI_Y_SCALE_REG (%08X)", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4500000 && VAddr <= 0xA4500014)
    {
        if (!LogAudioInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4500000: LogMessage("%08X: read from AI_DRAM_ADDR_REG (%08X)", PC, Value); return;
        case 0xA4500004: LogMessage("%08X: read from AI_LEN_REG (%08X)", PC, Value); return;
        case 0xA4500008: LogMessage("%08X: read from AI_CONTROL_REG (%08X)", PC, Value); return;
        case 0xA450000C: LogMessage("%08X: read from AI_STATUS_REG (%08X)", PC, Value); return;
        case 0xA4500010: LogMessage("%08X: read from AI_DACRATE_REG (%08X)", PC, Value); return;
        case 0xA4500014: LogMessage("%08X: read from AI_BITRATE_REG (%08X)", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4600000 && VAddr <= 0xA4600030)
    {
        if (!LogPerInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4600000: LogMessage("%08X: read from PI_DRAM_ADDR_REG (%08X)", PC, Value); return;
        case 0xA4600004: LogMessage("%08X: read from PI_CART_ADDR_REG (%08X)", PC, Value); return;
        case 0xA4600008: LogMessage("%08X: read from PI_RD_LEN_REG (%08X)", PC, Value); return;
        case 0xA460000C: LogMessage("%08X: read from PI_WR_LEN_REG (%08X)", PC, Value); return;
        case 0xA4600010: LogMessage("%08X: read from PI_STATUS_REG (%08X)", PC, Value); return;
        case 0xA4600014: LogMessage("%08X: read from PI_BSD_DOM1_LAT_REG/PI_DOMAIN1_REG (%08X)", PC, Value); return;
        case 0xA4600018: LogMessage("%08X: read from PI_BSD_DOM1_PWD_REG (%08X)", PC, Value); return;
        case 0xA460001C: LogMessage("%08X: read from PI_BSD_DOM1_PGS_REG (%08X)", PC, Value); return;
        case 0xA4600020: LogMessage("%08X: read from PI_BSD_DOM1_RLS_REG (%08X)", PC, Value); return;
        case 0xA4600024: LogMessage("%08X: read from PI_BSD_DOM2_LAT_REG/PI_DOMAIN2_REG (%08X)", PC, Value); return;
        case 0xA4600028: LogMessage("%08X: read from PI_BSD_DOM2_PWD_REG (%08X)", PC, Value); return;
        case 0xA460002C: LogMessage("%08X: read from PI_BSD_DOM2_PGS_REG (%08X)", PC, Value); return;
        case 0xA4600030: LogMessage("%08X: read from PI_BSD_DOM2_RLS_REG (%08X)", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4700000 && VAddr <= 0xA470001C)
    {
        if (!LogRDRAMInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);

        switch (VAddr)
        {
        case 0xA4700000: LogMessage("%08X: read from RI_MODE_REG (%08X)", PC, Value); return;
        case 0xA4700004: LogMessage("%08X: read from RI_CONFIG_REG (%08X)", PC, Value); return;
        case 0xA4700008: LogMessage("%08X: read from RI_CURRENT_LOAD_REG (%08X)", PC, Value); return;
        case 0xA470000C: LogMessage("%08X: read from RI_SELECT_REG (%08X)", PC, Value); return;
        case 0xA4700010: LogMessage("%08X: read from RI_REFRESH_REG/RI_COUNT_REG (%08X)", PC, Value); return;
        case 0xA4700014: LogMessage("%08X: read from RI_LATENCY_REG (%08X)", PC, Value); return;
        case 0xA4700018: LogMessage("%08X: read from RI_RERROR_REG (%08X)", PC, Value); return;
        case 0xA470001C: LogMessage("%08X: read from RI_WERROR_REG (%08X)", PC, Value); return;
        }
    }
    if (VAddr == 0xA4800000)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_DRAM_ADDR_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr == 0xA4800004)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_PIF_ADDR_RD64B_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr == 0xA4800010)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_PIF_ADDR_WR64B_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr == 0xA4800018)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read from SI_STATUS_REG (%08X)", PC, Value);
        return;
    }
    if (VAddr >= 0xBFC00000 && VAddr <= 0xBFC007C0)
    {
        return;
    }
    if (VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC)
    {
        if (!LogPRDirectMemLoads())
        {
            return;
        }
        g_MMU->LW_VAddr(VAddr, Value);
        LogMessage("%08X: read word from PIF RAM at 0x%X (%08X)", PC, VAddr - 0xBFC007C0, Value);
        return;
    }
    if (VAddr >= 0xB0000040 && ((VAddr - 0xB0000000) < g_Rom->GetRomSize()))
    {
        return;
    }
    if (VAddr >= 0xB0000000 && VAddr < 0xB0000040)
    {
        if (!LogRomHeader())
        {
            return;
        }

        g_MMU->LW_VAddr(VAddr, Value);
        switch (VAddr)
        {
        case 0xB0000004: LogMessage("%08X: read from ROM clock rate (%08X)", PC, Value); break;
        case 0xB0000008: LogMessage("%08X: read from ROM boot address offset (%08X)", PC, Value); break;
        case 0xB000000C: LogMessage("%08X: read from ROM release offset (%08X)", PC, Value); break;
        case 0xB0000010: LogMessage("%08X: read from ROM CRC1 (%08X)", PC, Value); break;
        case 0xB0000014: LogMessage("%08X: read from ROM CRC2 (%08X)", PC, Value); break;
        default: LogMessage("%08X: read from ROM header 0x%X (%08X)", PC, VAddr & 0xFF, Value);  break;
        }
        return;
    }
    if (!LogUnknown())
    {
        return;
    }
    LogMessage("%08X: read from unknown ??? (%08X)", PC, VAddr);
}

void CLogging::Log_SW(uint32_t PC, uint32_t VAddr, uint32_t Value)
{
    if (!GenerateLog())
    {
        return;
    }

    if (VAddr < 0xA0000000 || VAddr >= 0xC0000000)
    {
        uint32_t PAddr;
        if (!g_TransVaddr->TranslateVaddr(VAddr, PAddr))
        {
            if (LogUnknown())
            {
                LogMessage("%08X: Writing 0x%08X to %08X", PC, Value, VAddr);
            }
            return;
        }
        VAddr = PAddr + 0xA0000000;
    }

    if (VAddr >= 0xA0000000 && VAddr < (0xA0000000 + g_MMU->RdramSize()))
    {
        return;
    }
    if (VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024)
    {
        if (!LogRDRamRegisters())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA3F00000: LogMessage("%08X: Writing 0x%08X to RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG", PC, Value); return;
        case 0xA3F00004: LogMessage("%08X: Writing 0x%08X to RDRAM_DEVICE_ID_REG", PC, Value); return;
        case 0xA3F00008: LogMessage("%08X: Writing 0x%08X to RDRAM_DELAY_REG", PC, Value); return;
        case 0xA3F0000C: LogMessage("%08X: Writing 0x%08X to RDRAM_MODE_REG", PC, Value); return;
        case 0xA3F00010: LogMessage("%08X: Writing 0x%08X to RDRAM_REF_INTERVAL_REG", PC, Value); return;
        case 0xA3F00014: LogMessage("%08X: Writing 0x%08X to RDRAM_REF_ROW_REG", PC, Value); return;
        case 0xA3F00018: LogMessage("%08X: Writing 0x%08X to RDRAM_RAS_INTERVAL_REG", PC, Value); return;
        case 0xA3F0001C: LogMessage("%08X: Writing 0x%08X to RDRAM_MIN_INTERVAL_REG", PC, Value); return;
        case 0xA3F00020: LogMessage("%08X: Writing 0x%08X to RDRAM_ADDR_SELECT_REG", PC, Value); return;
        case 0xA3F00024: LogMessage("%08X: Writing 0x%08X to RDRAM_DEVICE_MANUF_REG", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC)
    {
        return;
    }

    if (VAddr >= 0xA4040000 && VAddr <= 0xA404001C)
    {
        if (!LogSPRegisters())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4040000: LogMessage("%08X: Writing 0x%08X to SP_MEM_ADDR_REG", PC, Value); return;
        case 0xA4040004: LogMessage("%08X: Writing 0x%08X to SP_DRAM_ADDR_REG", PC, Value); return;
        case 0xA4040008: LogMessage("%08X: Writing 0x%08X to SP_RD_LEN_REG", PC, Value); return;
        case 0xA404000C: LogMessage("%08X: Writing 0x%08X to SP_WR_LEN_REG", PC, Value); return;
        case 0xA4040010: LogMessage("%08X: Writing 0x%08X to SP_STATUS_REG", PC, Value); return;
        case 0xA4040014: LogMessage("%08X: Writing 0x%08X to SP_DMA_FULL_REG", PC, Value); return;
        case 0xA4040018: LogMessage("%08X: Writing 0x%08X to SP_DMA_BUSY_REG", PC, Value); return;
        case 0xA404001C: LogMessage("%08X: Writing 0x%08X to SP_SEMAPHORE_REG", PC, Value); return;
        }
    }
    if (VAddr == 0xA4080000)
    {
        if (!LogSPRegisters())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SP_PC", PC, Value); return;
    }

    if (VAddr >= 0xA4100000 && VAddr <= 0xA410001C)
    {
        if (!LogDPCRegisters())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4100000: LogMessage("%08X: Writing 0x%08X to DPC_START_REG", PC, Value); return;
        case 0xA4100004: LogMessage("%08X: Writing 0x%08X to DPC_END_REG", PC, Value); return;
        case 0xA4100008: LogMessage("%08X: Writing 0x%08X to DPC_CURRENT_REG", PC, Value); return;
        case 0xA410000C: LogMessage("%08X: Writing 0x%08X to DPC_STATUS_REG", PC, Value); return;
        case 0xA4100010: LogMessage("%08X: Writing 0x%08X to DPC_CLOCK_REG", PC, Value); return;
        case 0xA4100014: LogMessage("%08X: Writing 0x%08X to DPC_BUFBUSY_REG", PC, Value); return;
        case 0xA4100018: LogMessage("%08X: Writing 0x%08X to DPC_PIPEBUSY_REG", PC, Value); return;
        case 0xA410001C: LogMessage("%08X: Writing 0x%08X to DPC_TMEM_REG", PC, Value); return;
        }
    }

    if (VAddr >= 0xA4200000 && VAddr <= 0xA420000C)
    {
        if (!LogDPSRegisters())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4200000: LogMessage("%08X: Writing 0x%08X to DPS_TBIST_REG", PC, Value); return;
        case 0xA4200004: LogMessage("%08X: Writing 0x%08X to DPS_TEST_MODE_REG", PC, Value); return;
        case 0xA4200008: LogMessage("%08X: Writing 0x%08X to DPS_BUFTEST_ADDR_REG", PC, Value); return;
        case 0xA420000C: LogMessage("%08X: Writing 0x%08X to DPS_BUFTEST_DATA_REG", PC, Value); return;
        }
    }

    if (VAddr >= 0xA4300000 && VAddr <= 0xA430000C)
    {
        if (!LogMIPSInterface())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4300000: LogMessage("%08X: Writing 0x%08X to MI_INIT_MODE_REG/MI_MODE_REG", PC, Value); return;
        case 0xA4300004: LogMessage("%08X: Writing 0x%08X to MI_VERSION_REG/MI_NOOP_REG", PC, Value); return;
        case 0xA4300008: LogMessage("%08X: Writing 0x%08X to MI_INTR_REG", PC, Value); return;
        case 0xA430000C: LogMessage("%08X: Writing 0x%08X to MI_INTR_MASK_REG", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4400000 && VAddr <= 0xA4400034)
    {
        if (!LogVideoInterface())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4400000: LogMessage("%08X: Writing 0x%08X to VI_STATUS_REG/VI_CONTROL_REG", PC, Value); return;
        case 0xA4400004: LogMessage("%08X: Writing 0x%08X to VI_ORIGIN_REG/VI_DRAM_ADDR_REG", PC, Value); return;
        case 0xA4400008: LogMessage("%08X: Writing 0x%08X to VI_WIDTH_REG/VI_H_WIDTH_REG", PC, Value); return;
        case 0xA440000C: LogMessage("%08X: Writing 0x%08X to VI_INTR_REG/VI_V_INTR_REG", PC, Value); return;
        case 0xA4400010: LogMessage("%08X: Writing 0x%08X to VI_CURRENT_REG/VI_V_CURRENT_LINE_REG", PC, Value); return;
        case 0xA4400014: LogMessage("%08X: Writing 0x%08X to VI_BURST_REG/VI_TIMING_REG", PC, Value); return;
        case 0xA4400018: LogMessage("%08X: Writing 0x%08X to VI_V_SYNC_REG", PC, Value); return;
        case 0xA440001C: LogMessage("%08X: Writing 0x%08X to VI_H_SYNC_REG", PC, Value); return;
        case 0xA4400020: LogMessage("%08X: Writing 0x%08X to VI_LEAP_REG/VI_H_SYNC_LEAP_REG", PC, Value); return;
        case 0xA4400024: LogMessage("%08X: Writing 0x%08X to VI_H_START_REG/VI_H_VIDEO_REG", PC, Value); return;
        case 0xA4400028: LogMessage("%08X: Writing 0x%08X to VI_V_START_REG/VI_V_VIDEO_REG", PC, Value); return;
        case 0xA440002C: LogMessage("%08X: Writing 0x%08X to VI_V_BURST_REG", PC, Value); return;
        case 0xA4400030: LogMessage("%08X: Writing 0x%08X to VI_X_SCALE_REG", PC, Value); return;
        case 0xA4400034: LogMessage("%08X: Writing 0x%08X to VI_Y_SCALE_REG", PC, Value); return;
        }
    }

    if (VAddr >= 0xA4500000 && VAddr <= 0xA4500014)
    {
        if (!LogAudioInterface())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4500000: LogMessage("%08X: Writing 0x%08X to AI_DRAM_ADDR_REG", PC, Value); return;
        case 0xA4500004: LogMessage("%08X: Writing 0x%08X to AI_LEN_REG", PC, Value); return;
        case 0xA4500008: LogMessage("%08X: Writing 0x%08X to AI_CONTROL_REG", PC, Value); return;
        case 0xA450000C: LogMessage("%08X: Writing 0x%08X to AI_STATUS_REG", PC, Value); return;
        case 0xA4500010: LogMessage("%08X: Writing 0x%08X to AI_DACRATE_REG", PC, Value); return;
        case 0xA4500014: LogMessage("%08X: Writing 0x%08X to AI_BITRATE_REG", PC, Value); return;
        }
    }

    if (VAddr >= 0xA4600000 && VAddr <= 0xA4600030)
    {
        if (!LogPerInterface())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4600000: LogMessage("%08X: Writing 0x%08X to PI_DRAM_ADDR_REG", PC, Value); return;
        case 0xA4600004: LogMessage("%08X: Writing 0x%08X to PI_CART_ADDR_REG", PC, Value); return;
        case 0xA4600008: LogMessage("%08X: Writing 0x%08X to PI_RD_LEN_REG", PC, Value); return;
        case 0xA460000C: LogMessage("%08X: Writing 0x%08X to PI_WR_LEN_REG", PC, Value); return;
        case 0xA4600010: LogMessage("%08X: Writing 0x%08X to PI_STATUS_REG", PC, Value); return;
        case 0xA4600014: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_LAT_REG/PI_DOMAIN1_REG", PC, Value); return;
        case 0xA4600018: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_PWD_REG", PC, Value); return;
        case 0xA460001C: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_PGS_REG", PC, Value); return;
        case 0xA4600020: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_RLS_REG", PC, Value); return;
        case 0xA4600024: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_LAT_REG/PI_DOMAIN2_REG", PC, Value); return;
        case 0xA4600028: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_PWD_REG", PC, Value); return;
        case 0xA460002C: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_PGS_REG", PC, Value); return;
        case 0xA4600030: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_RLS_REG", PC, Value); return;
        }
    }
    if (VAddr >= 0xA4700000 && VAddr <= 0xA470001C)
    {
        if (!LogRDRAMInterface())
        {
            return;
        }
        switch (VAddr)
        {
        case 0xA4700000: LogMessage("%08X: Writing 0x%08X to RI_MODE_REG", PC, Value); return;
        case 0xA4700004: LogMessage("%08X: Writing 0x%08X to RI_CONFIG_REG", PC, Value); return;
        case 0xA4700008: LogMessage("%08X: Writing 0x%08X to RI_CURRENT_LOAD_REG", PC, Value); return;
        case 0xA470000C: LogMessage("%08X: Writing 0x%08X to RI_SELECT_REG", PC, Value); return;
        case 0xA4700010: LogMessage("%08X: Writing 0x%08X to RI_REFRESH_REG/RI_COUNT_REG", PC, Value); return;
        case 0xA4700014: LogMessage("%08X: Writing 0x%08X to RI_LATENCY_REG", PC, Value); return;
        case 0xA4700018: LogMessage("%08X: Writing 0x%08X to RI_RERROR_REG", PC, Value); return;
        case 0xA470001C: LogMessage("%08X: Writing 0x%08X to RI_WERROR_REG", PC, Value); return;
        }
    }
    if (VAddr == 0xA4800000)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_DRAM_ADDR_REG", PC, Value); return;
    }
    if (VAddr == 0xA4800004)
    {
        if (LogPRDMAOperations())
        {
            LogMessage("%08X: A DMA transfer from the PIF RAM has occurred", PC);
        }
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_PIF_ADDR_RD64B_REG", PC, Value); return;
    }
    if (VAddr == 0xA4800010)
    {
        if (LogPRDMAOperations())
        {
            LogMessage("%08X: A DMA transfer to the PIF RAM has occurred", PC);
        }
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_PIF_ADDR_WR64B_REG", PC, Value); return;
    }
    if (VAddr == 0xA4800018)
    {
        if (!LogSerialInterface())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to SI_STATUS_REG", PC, Value); return;
    }

    if (VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC)
    {
        if (!LogPRDirectMemStores())
        {
            return;
        }
        LogMessage("%08X: Writing 0x%08X to PIF RAM at 0x%X", PC, Value, VAddr - 0xBFC007C0);
        return;
    }
    if (!LogUnknown())
    {
        return;
    }
    LogMessage("%08X: Writing 0x%08X to %08X ????", PC, Value, VAddr);
}

void CLogging::LogMessage(const char * Message, ...)
{
    char Msg[400];
    va_list ap;

    if (!g_Settings->LoadBool(Debugger_Enabled))
    {
        return;
    }
    if (m_hLogFile == NULL)
    {
        return;
    }

    va_start(ap, Message);
    vsprintf(Msg, Message, ap);
    va_end(ap);

    strcat(Msg, "\r\n");

    m_hLogFile->Write(Msg, strlen(Msg));
}

void CLogging::StartLog(void)
{
    if (!GenerateLog())
    {
        StopLog();
        return;
    }
    if (m_hLogFile != NULL)
    {
        return;
    }

    CPath LogFile(g_Settings->LoadStringVal(Directory_Log).c_str(), "cpudebug.log");
    m_hLogFile = new CFile(LogFile, CFileBase::modeCreate | CFileBase::modeWrite);
}

void CLogging::StopLog(void)
{
    if (m_hLogFile)
    {
        delete m_hLogFile;
        m_hLogFile = NULL;
    }
}
