#include "stdafx.h"
#include "VideoInterfaceHandler.h"
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include <Project64-core\N64System\Mips\SystemTiming.h>
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\Plugin.h>
#include <Project64-core\N64System\SystemGlobals.h>

VideoInterfaceReg::VideoInterfaceReg(uint32_t * VideoInterface) :
    VI_STATUS_REG(VideoInterface[0]),
    VI_CONTROL_REG(VideoInterface[0]),
    VI_ORIGIN_REG(VideoInterface[1]),
    VI_DRAM_ADDR_REG(VideoInterface[1]),
    VI_WIDTH_REG(VideoInterface[2]),
    VI_H_WIDTH_REG(VideoInterface[2]),
    VI_INTR_REG(VideoInterface[3]),
    VI_V_INTR_REG(VideoInterface[3]),
    VI_CURRENT_REG(VideoInterface[4]),
    VI_V_CURRENT_LINE_REG(VideoInterface[4]),
    VI_BURST_REG(VideoInterface[5]),
    VI_TIMING_REG(VideoInterface[5]),
    VI_V_SYNC_REG(VideoInterface[6]),
    VI_H_SYNC_REG(VideoInterface[7]),
    VI_LEAP_REG(VideoInterface[8]),
    VI_H_SYNC_LEAP_REG(VideoInterface[8]),
    VI_H_START_REG(VideoInterface[9]),
    VI_H_VIDEO_REG(VideoInterface[9]),
    VI_V_START_REG(VideoInterface[10]),
    VI_V_VIDEO_REG(VideoInterface[10]),
    VI_V_BURST_REG(VideoInterface[11]),
    VI_X_SCALE_REG(VideoInterface[12]),
    VI_Y_SCALE_REG(VideoInterface[13])
{
}

VideoInterfaceHandler::VideoInterfaceHandler(CN64System & System, CMipsMemoryVM & MMU, CRegisters & Reg) :
    VideoInterfaceReg(Reg.m_Video_Interface),
    m_System(System),
    m_MMU(MMU),
    m_Plugins(System.GetPlugins()),
    m_Reg(Reg),
    m_SystemTimer(System.m_SystemTimer),
    m_NextTimer(System.m_NextTimer),
    m_PC(Reg.m_PROGRAM_COUNTER),
    m_FieldSerration(0),
    m_HalfLine(0),
    m_HalfLineCheck(false)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

VideoInterfaceHandler::~VideoInterfaceHandler()
{
    m_System.UnregisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    m_System.UnregisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

bool VideoInterfaceHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04400000: Value = VI_STATUS_REG; break;
    case 0x04400004: Value = VI_ORIGIN_REG; break;
    case 0x04400008: Value = VI_WIDTH_REG; break;
    case 0x0440000C: Value = VI_INTR_REG; break;
    case 0x04400010:
        UpdateHalfLine();
        Value = m_HalfLine;
        break;
    case 0x04400014: Value = VI_BURST_REG; break;
    case 0x04400018: Value = VI_V_SYNC_REG; break;
    case 0x0440001C: Value = VI_H_SYNC_REG; break;
    case 0x04400020: Value = VI_LEAP_REG; break;
    case 0x04400024: Value = VI_H_START_REG; break;
    case 0x04400028: Value = VI_V_START_REG; break;
    case 0x0440002C: Value = VI_V_BURST_REG; break;
    case 0x04400030: Value = VI_X_SCALE_REG; break;
    case 0x04400034: Value = VI_Y_SCALE_REG; break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (GenerateLog() && LogVideoInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04400000: LogMessage("%08X: read from VI_STATUS_REG/VI_CONTROL_REG (%08X)", m_PC, Value); break;
        case 0x04400004: LogMessage("%08X: read from VI_ORIGIN_REG/VI_DRAM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04400008: LogMessage("%08X: read from VI_WIDTH_REG/VI_H_WIDTH_REG (%08X)", m_PC, Value); break;
        case 0x0440000C: LogMessage("%08X: read from VI_INTR_REG/VI_V_INTR_REG (%08X)", m_PC, Value); break;
        case 0x04400010: LogMessage("%08X: read from VI_CURRENT_REG/VI_V_CURRENT_LINE_REG (%08X)", m_PC, Value); break;
        case 0x04400014: LogMessage("%08X: read from VI_BURST_REG/VI_TIMING_REG (%08X)", m_PC, Value); break;
        case 0x04400018: LogMessage("%08X: read from VI_V_SYNC_REG (%08X)", m_PC, Value); break;
        case 0x0440001C: LogMessage("%08X: read from VI_H_SYNC_REG (%08X)", m_PC, Value); break;
        case 0x04400020: LogMessage("%08X: read from VI_LEAP_REG/VI_H_SYNC_LEAP_REG (%08X)", m_PC, Value); break;
        case 0x04400024: LogMessage("%08X: read from VI_H_START_REG/VI_H_VIDEO_REG (%08X)", m_PC, Value); break;
        case 0x04400028: LogMessage("%08X: read from VI_V_START_REG/VI_V_VIDEO_REG (%08X)", m_PC, Value); break;
        case 0x0440002C: LogMessage("%08X: read from VI_V_BURST_REG (%08X)", m_PC, Value); break;
        case 0x04400030: LogMessage("%08X: read from VI_X_SCALE_REG (%08X)", m_PC, Value); break;
        case 0x04400034: LogMessage("%08X: read from VI_Y_SCALE_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool VideoInterfaceHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (GenerateLog() && LogVideoInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04400000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_STATUS_REG/VI_CONTROL_REG", m_PC, Value, Mask); break;
        case 0x04400004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_ORIGIN_REG/VI_DRAM_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04400008: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_WIDTH_REG/VI_H_WIDTH_REG", m_PC, Value, Mask); break;
        case 0x0440000C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_INTR_REG/VI_V_INTR_REG", m_PC, Value, Mask); break;
        case 0x04400010: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_CURRENT_REG/VI_V_CURRENT_LINE_REG", m_PC, Value, Mask); break;
        case 0x04400014: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_BURST_REG/VI_TIMING_REG", m_PC, Value, Mask); break;
        case 0x04400018: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_V_SYNC_REG", m_PC, Value, Mask); break;
        case 0x0440001C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_H_SYNC_REG", m_PC, Value, Mask); break;
        case 0x04400020: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_LEAP_REG/VI_H_SYNC_LEAP_REG", m_PC, Value, Mask); break;
        case 0x04400024: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_H_START_REG/VI_H_VIDEO_REG", m_PC, Value, Mask); break;
        case 0x04400028: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_V_START_REG/VI_V_VIDEO_REG", m_PC, Value, Mask); break;
        case 0x0440002C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_V_BURST_REG", m_PC, Value, Mask); break;
        case 0x04400030: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_X_SCALE_REG", m_PC, Value, Mask); break;
        case 0x04400034: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to VI_Y_SCALE_REG", m_PC, Value, Mask); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }

    uint32_t MaskedValue = Value & Mask;
    switch (Address & 0xFFFFFFF)
    {
    case 0x04400000:
        if (VI_STATUS_REG != ((VI_STATUS_REG & ~Mask) | (MaskedValue)))
        {
            VI_STATUS_REG = (VI_STATUS_REG & ~Mask) | (MaskedValue);
            if (m_Plugins->Gfx()->ViStatusChanged != nullptr)
            {
                m_Plugins->Gfx()->ViStatusChanged();
            }
        }
        break;
    case 0x04400004:
        VI_ORIGIN_REG = ((VI_ORIGIN_REG & ~Mask) | (MaskedValue)) & 0xFFFFFF;
        break;
    case 0x04400008:
        if (VI_WIDTH_REG != ((VI_WIDTH_REG & ~Mask) | (MaskedValue)))
        {
            VI_WIDTH_REG = (VI_WIDTH_REG & ~Mask) | (MaskedValue);
            if (m_Plugins->Gfx()->ViWidthChanged != nullptr)
            {
                m_Plugins->Gfx()->ViWidthChanged();
            }
        }
        break;
    case 0x0440000C: VI_INTR_REG = (VI_INTR_REG & ~Mask) | (MaskedValue); break;
    case 0x04400010:
        m_Reg.MI_INTR_REG &= ~MI_INTR_VI;
        m_Reg.CheckInterrupts();
        break;
    case 0x04400014: VI_BURST_REG = (VI_BURST_REG & ~Mask) | (MaskedValue); break;
    case 0x04400018: VI_V_SYNC_REG = (VI_V_SYNC_REG & ~Mask) | (MaskedValue); break;
    case 0x0440001C: VI_H_SYNC_REG = (VI_H_SYNC_REG & ~Mask) | (MaskedValue); break;
    case 0x04400020: VI_LEAP_REG = (VI_LEAP_REG & ~Mask) | (MaskedValue); break;
    case 0x04400024: VI_H_START_REG = (VI_H_START_REG & ~Mask) | (MaskedValue); break;
    case 0x04400028: VI_V_START_REG = (VI_V_START_REG & ~Mask) | (MaskedValue); break;
    case 0x0440002C: VI_V_BURST_REG = (VI_V_BURST_REG & ~Mask) | (MaskedValue); break;
    case 0x04400030: VI_X_SCALE_REG = (VI_X_SCALE_REG & ~Mask) | (MaskedValue); break;
    case 0x04400034: VI_Y_SCALE_REG = (VI_Y_SCALE_REG & ~Mask) | (MaskedValue); break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}

void VideoInterfaceHandler::UpdateFieldSerration(uint32_t interlaced)
{
    m_FieldSerration ^= 1;
    m_FieldSerration &= interlaced;
}

void VideoInterfaceHandler::UpdateHalfLine()
{
    uint32_t NextViTimer = m_SystemTimer.GetTimer(CSystemTimer::ViTimer);

    if (m_NextTimer < 0)
    {
        m_HalfLine = 0;
        return;
    }

    int32_t check_value = (int32_t)(m_HalfLineCheck - NextViTimer);
    if (check_value > 0 && check_value < 40)
    {
        m_NextTimer -= ViRefreshRate();
        if (m_NextTimer < 0)
        {
            m_NextTimer = 0 - CountPerOp();
        }
        m_SystemTimer.UpdateTimers();
        NextViTimer = m_SystemTimer.GetTimer(CSystemTimer::ViTimer);
    }
    m_HalfLine = (uint32_t)(m_NextTimer / ViRefreshRate());
    m_HalfLine &= ~1;
    m_HalfLine |= m_FieldSerration;
    VI_V_CURRENT_LINE_REG = m_HalfLine;
    m_HalfLineCheck = NextViTimer;
}

void VideoInterfaceHandler::LoadedGameState(void)
{
    SystemReset();
}

void VideoInterfaceHandler::SystemReset(void)
{
    m_FieldSerration = 0;
    m_HalfLine = 0;
    m_HalfLineCheck = false;
    UpdateFieldSerration((VI_STATUS_REG & 0x40) != 0);
    UpdateHalfLine();
}
