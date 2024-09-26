#include "stdafx.h"

#include "RDRAMRegistersHandler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\SystemGlobals.h>

RDRAMRegistersReg::RDRAMRegistersReg(uint32_t * RdramInterface) :
    RDRAM_CONFIG_REG(RdramInterface[0]),
    RDRAM_DEVICE_TYPE_REG(RdramInterface[0]),
    RDRAM_DEVICE_ID_REG(RdramInterface[1]),
    RDRAM_DELAY_REG(RdramInterface[2]),
    RDRAM_MODE_REG(RdramInterface[3]),
    RDRAM_REF_INTERVAL_REG(RdramInterface[4]),
    RDRAM_REF_ROW_REG(RdramInterface[5]),
    RDRAM_RAS_INTERVAL_REG(RdramInterface[6]),
    RDRAM_MIN_INTERVAL_REG(RdramInterface[7]),
    RDRAM_ADDR_SELECT_REG(RdramInterface[8]),
    RDRAM_DEVICE_MANUF_REG(RdramInterface[9])
{
}

RDRAMRegistersHandler::RDRAMRegistersHandler(CRegisters & Reg) :
    RDRAMRegistersReg(Reg.m_RDRAM_Registers),
    m_PC(Reg.m_PROGRAM_COUNTER)
{
    memset(m_Device, 0, sizeof(m_Device));
}

bool RDRAMRegistersHandler::Read32(uint32_t Address, uint32_t & Value)
{
    uint32_t DeviceID = Address >> 13 & 3;
    RDRAM_DEVICE & Device = m_Device[DeviceID];

    switch (Address & 0x3fc)
    {
    case 0x00: Value = Device.DeviceType; break;
    case 0x04: Value = Device.DeviceId; break;
    case 0x08: Value = Device.Delay; break;
    case 0x0C: Value = Device.Mode ^ 0xc0c0c0c0; break;
    case 0x10: Value = Device.RefreshInterval; break;
    case 0x14: Value = Device.RefreshRow; break;
    case 0x18: Value = Device.RasInterval; break;
    case 0x1C: Value = Device.MinInterval; break;
    case 0x20: Value = Device.AddressSelect; break;
    case 0x24: Value = Device.DeviceManufacturer; break;
    case 0x28: Value = Device.CurrentControl; break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (LogRDRamRegisters())
    {
        switch (Address & 0x3fc)
        {
        case 0x00: LogMessage("%016llX: read from Device[%d].DeviceType (%08X)", m_PC, DeviceID, Value); break;
        case 0x04: LogMessage("%016llX: read from Device[%d].DeviceId (%08X)", m_PC, DeviceID, Value); break;
        case 0x08: LogMessage("%016llX: read from Device[%d].Delay (%08X)", m_PC, DeviceID, Value); break;
        case 0x0C: LogMessage("%016llX: read from Device[%d].Mode (%08X)", m_PC, DeviceID, Value); break;
        case 0x10: LogMessage("%016llX: read from Device[%d].RefreshInterval (%08X)", m_PC, DeviceID, Value); break;
        case 0x14: LogMessage("%016llX: read from Device[%d].RefreshRow (%08X)", m_PC, DeviceID, Value); break;
        case 0x18: LogMessage("%016llX: read from Device[%d].RasInterval (%08X)", m_PC, DeviceID, Value); break;
        case 0x1C: LogMessage("%016llX: read from Device[%d].MinInterval (%08X)", m_PC, DeviceID, Value); break;
        case 0x20: LogMessage("%016llX: read from Device[%d].AddressSelect (%08X)", m_PC, DeviceID, Value); break;
        case 0x24: LogMessage("%016llX: read from Device[%d].DeviceManufacturer (%08X)", m_PC, DeviceID, Value); break;
        case 0x28: LogMessage("%016llX: read from Device[%d].CurrentControl (%08X)", m_PC, DeviceID, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool RDRAMRegistersHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    uint32_t DeviceID = Address >> 13 & 3;
    RDRAM_DEVICE & Device = m_Device[DeviceID];

    if (LogRDRamRegisters())
    {
        switch (Address & 0x3fc)
        {
        case 0x00: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].DeviceType", m_PC, Value, Mask, DeviceID); break;
        case 0x04: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].DeviceId", m_PC, Value, Mask, DeviceID); break;
        case 0x08: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].Delay", m_PC, Value, Mask, DeviceID); break;
        case 0x0C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].Mode", m_PC, Value, Mask, DeviceID); break;
        case 0x10: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].RefreshInterval", m_PC, Value, Mask, DeviceID); break;
        case 0x14: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].RefreshRow", m_PC, Value, Mask, DeviceID); break;
        case 0x18: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].RasInterval", m_PC, Value, Mask, DeviceID); break;
        case 0x1C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].MinInterval", m_PC, Value, Mask, DeviceID); break;
        case 0x20: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].AddressSelect", m_PC, Value, Mask, DeviceID); break;
        case 0x24: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].DeviceManufacturer", m_PC, Value, Mask, DeviceID); break;
        case 0x28: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to Device[%d].CurrentControl", m_PC, Value, Mask, DeviceID); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    switch (Address & 0x3fc)
    {
    case 0x00: Device.DeviceType = (Device.DeviceType & ~Mask) | (Value & Mask); break;
    case 0x04: Device.DeviceId = (Device.DeviceId & ~Mask) | (Value & Mask); break;
    case 0x08: Device.Delay = (Device.Delay & ~Mask) | (Value & Mask); break;
    case 0x0C: Device.Mode = (Device.Mode & ~Mask) | (Value & Mask); break;
    case 0x10: Device.RefreshInterval = (Device.RefreshInterval & ~Mask) | (Value & Mask); break;
    case 0x14: Device.RefreshRow = (Device.RefreshRow & ~Mask) | (Value & Mask); break;
    case 0x18: Device.RasInterval = (Device.RasInterval & ~Mask) | (Value & Mask); break;
    case 0x1C: Device.MinInterval = (Device.MinInterval & ~Mask) | (Value & Mask); break;
    case 0x20: Device.AddressSelect = (Device.AddressSelect & ~Mask) | (Value & Mask); break;
    case 0x24: Device.DeviceManufacturer = (Device.DeviceManufacturer & ~Mask) | (Value & Mask); break;
    case 0x28: Device.CurrentControl = (Device.CurrentControl & ~Mask) | (Value & Mask); break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}
