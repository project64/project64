#include "stdafx.h"

#include "SPRegistersHandler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\SystemGlobals.h>
#include <Project64-core\Plugins\GFXPlugin.h>
#include <Project64-core\Plugins\Plugin.h>

DisplayControlRegHandler::DisplayControlRegHandler(CN64System & N64System, CPlugins * Plugins, CRegisters & Reg) :
    DisplayControlReg(Reg.m_Display_ControlReg),
    SPRegistersReg(Reg.m_SigProcessor_Interface),
    m_System(N64System),
    m_Plugins(Plugins),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
}

bool DisplayControlRegHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04100000: Value = DPC_START_REG; break;
    case 0x04100004: Value = DPC_END_REG; break;
    case 0x04100008: Value = DPC_CURRENT_REG; break;
    case 0x0410000C: Value = DPC_STATUS_REG; break;
    case 0x04100010: Value = DPC_CLOCK_REG; break;
    case 0x04100014: Value = DPC_BUFBUSY_REG; break;
    case 0x04100018: Value = DPC_PIPEBUSY_REG; break;
    case 0x0410001C: Value = DPC_TMEM_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (GenerateLog() && LogDPCRegisters())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04100000: LogMessage("%016llX: read from DPC_START_REG (%08X)", m_PC, Value); break;
        case 0x04100004: LogMessage("%016llX: read from DPC_END_REG (%08X)", m_PC, Value); break;
        case 0x04100008: LogMessage("%016llX: read from DPC_CURRENT_REG (%08X)", m_PC, Value); break;
        case 0x0410000C: LogMessage("%016llX: read from DPC_STATUS_REG (%08X)", m_PC, Value); break;
        case 0x04100010: LogMessage("%016llX: read from DPC_CLOCK_REG (%08X)", m_PC, Value); break;
        case 0x04100014: LogMessage("%016llX: read from DPC_BUFBUSY_REG (%08X)", m_PC, Value); break;
        case 0x04100018: LogMessage("%016llX: read from DPC_PIPEBUSY_REG (%08X)", m_PC, Value); break;
        case 0x0410001C: LogMessage("%016llX: read from DPC_TMEM_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool DisplayControlRegHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (GenerateLog() && LogDPCRegisters())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04100000: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_START_REG", m_PC, Value, Mask); break;
        case 0x04100004: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_END_REG", m_PC, Value, Mask); break;
        case 0x04100008: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_CURRENT_REG", m_PC, Value, Mask); break;
        case 0x0410000C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_STATUS_REG", m_PC, Value, Mask); break;
        case 0x04100010: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_CLOCK_REG", m_PC, Value, Mask); break;
        case 0x04100014: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_BUFBUSY_REG", m_PC, Value, Mask); break;
        case 0x04100018: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_PIPEBUSY_REG", m_PC, Value, Mask); break;
        case 0x0410001C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to DPC_TMEM_REG", m_PC, Value, Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    uint32_t MaskedValue = Value & Mask;
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04100000:
        if ((DPC_STATUS_REG & DPC_STATUS_START_VALID) == 0)
        {
            DPC_START_REG = MaskedValue & 0xFFFFF8;
            DPC_STATUS_REG |= DPC_STATUS_START_VALID;
        }
        break;
    case 0x04100004:
        DPC_END_REG = MaskedValue & 0xFFFFF8;
        if (DPC_STATUS_REG & DPC_STATUS_START_VALID)
        {
            DPC_CURRENT_REG = DPC_START_REG;
            DPC_STATUS_REG &= ~DPC_STATUS_START_VALID;
        }
        ProcessRDPList();
        break;
        //case 0x04100008: g_Reg->DPC_CURRENT_REG = Value; break;
    case 0x0410000C:
        if ((MaskedValue & DPC_CLR_XBUS_DMEM_DMA) != 0 && (Value & DPC_SET_XBUS_DMEM_DMA) == 0)
        {
            DPC_STATUS_REG &= ~DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((MaskedValue & DPC_SET_XBUS_DMEM_DMA) != 0 && (Value & DPC_CLR_XBUS_DMEM_DMA) == 0)
        {
            DPC_STATUS_REG |= DPC_STATUS_XBUS_DMEM_DMA;
        }
        if ((MaskedValue & DPC_CLR_FREEZE) != 0 && (Value & DPC_SET_FREEZE) == 0)
        {
            DPC_STATUS_REG &= ~DPC_STATUS_FREEZE;
            ProcessRDPList();
        }
        if ((MaskedValue & DPC_SET_FREEZE) != 0 && (Value & DPC_CLR_FREEZE) == 0)
        {
            DPC_STATUS_REG |= DPC_STATUS_FREEZE;
        }
        if ((MaskedValue & DPC_CLR_FLUSH) != 0 && (Value & DPC_SET_FLUSH) == 0)
        {
            DPC_STATUS_REG &= ~DPC_STATUS_FLUSH;
        }
        if ((MaskedValue & DPC_SET_FLUSH) != 0 && (Value & DPC_CLR_FLUSH) == 0)
        {
            DPC_STATUS_REG |= DPC_STATUS_FLUSH;
        }
        if ((MaskedValue & DPC_CLR_TMEM_CTR) != 0)
        {
            DPC_STATUS_REG &= ~DPC_STATUS_TMEM_BUSY;
        }
        if ((MaskedValue & DPC_CLR_PIPE_CTR) != 0)
        {
            DPC_STATUS_REG &= ~DPC_STATUS_PIPE_BUSY;
        }
        if ((MaskedValue & DPC_CLR_CMD_CTR) != 0)
        {
            DPC_STATUS_REG &= ~DPC_STATUS_CMD_BUSY;
        }
        if ((MaskedValue & DPC_CLR_FREEZE) != 0)
        {
            if ((SP_STATUS_REG & SP_STATUS_HALT) == 0)
            {
                if ((SP_STATUS_REG & SP_STATUS_BROKE) == 0)
                {
                    m_Plugins->RSP()->RunRSP();
                }
            }
        }
        break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}

void DisplayControlRegHandler::ProcessRDPList(void)
{
    if ((DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0)
    {
        return;
    }
    DPC_STATUS_REG |= DPC_STATUS_PIPE_BUSY | DPC_STATUS_START_GCLK;
    if (DPC_END_REG > DPC_CURRENT_REG && m_Plugins->Gfx()->ProcessRDPList != nullptr)
    {
        m_Plugins->Gfx()->ProcessRDPList();
    }
    DPC_STATUS_REG |= DPC_STATUS_CBUF_READY;
}
