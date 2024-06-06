#include "stdafx.h"

#include "AudioInterfaceHandler.h"
#include <Project64-core\N64System\Mips\Register.h>
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\SystemGlobals.h>

AudioInterfaceReg::AudioInterfaceReg(uint32_t * _AudioInterface) :
    AI_DRAM_ADDR_REG(_AudioInterface[0]),
    AI_LEN_REG(_AudioInterface[1]),
    AI_CONTROL_REG(_AudioInterface[2]),
    AI_STATUS_REG(_AudioInterface[3]),
    AI_DACRATE_REG(_AudioInterface[4]),
    AI_BITRATE_REG(_AudioInterface[5])
{
}

AudioInterfaceHandler::AudioInterfaceHandler(CN64System & System, CRegisters & Reg) :
    AudioInterfaceReg(Reg.m_Audio_Interface),
    m_System(System),
    m_Reg(Reg),
    m_Plugins(System.GetPlugins()),
    m_PC(Reg.m_PROGRAM_COUNTER),
    m_Status(0),
    m_SecondBuff(0),
    m_BytesPerSecond(0),
    m_CountsPerByte(System.AiCountPerBytes()),
    m_FramesPerSecond(60)
{
    SystemReset();
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

AudioInterfaceHandler::~AudioInterfaceHandler()
{
    m_System.UnregisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    m_System.UnregisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

bool AudioInterfaceHandler::Read32(uint32_t Address, uint32_t & Value)
{
    switch (Address & 0x1FFFFFFF)
    {
    case 0x04500004:
        if (bFixedAudio())
        {
            Value = GetLength();
        }
        else
        {
            if (m_Plugins->Audio()->AiReadLength != nullptr)
            {
                Value = m_Plugins->Audio()->AiReadLength();
            }
            else
            {
                Value = 0;
            }
        }
        break;
    case 0x0450000C:
        if (bFixedAudio())
        {
            Value = GetStatus();
        }
        else
        {
            Value = AI_STATUS_REG;
        }
        break;
    default:
        Value = 0;
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }

    if (GenerateLog() && LogAudioInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04500000: LogMessage("%016llX: read from AI_DRAM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04500004: LogMessage("%016llX: read from AI_LEN_REG (%08X)", m_PC, Value); break;
        case 0x04500008: LogMessage("%016llX: read from AI_CONTROL_REG (%08X)", m_PC, Value); break;
        case 0x0450000C: LogMessage("%016llX: read from AI_STATUS_REG (%08X)", m_PC, Value); break;
        case 0x04500010: LogMessage("%016llX: read from AI_DACRATE_REG (%08X)", m_PC, Value); break;
        case 0x04500014: LogMessage("%016llX: read from AI_BITRATE_REG (%08X)", m_PC, Value); break;
        default:
            if (HaveDebugger())
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    return true;
}

bool AudioInterfaceHandler::Write32(uint32_t Address, uint32_t Value, uint32_t Mask)
{
    if (GenerateLog() && LogAudioInterface())
    {
        switch (Address & 0x1FFFFFFF)
        {
        case 0x04500000: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to AI_DRAM_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04500004: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to AI_LEN_REG", m_PC, Value, Mask); break;
        case 0x04500008: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to AI_CONTROL_REG", m_PC, Value, Mask); break;
        case 0x0450000C: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to AI_STATUS_REG", m_PC, Value, Mask); break;
        case 0x04500010: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to AI_DACRATE_REG", m_PC, Value, Mask); break;
        case 0x04500014: LogMessage("%016llX: Writing 0x%08X (Mask: 0x%08X) to AI_BITRATE_REG", m_PC, Value, Mask); break;
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
    case 0x04500000: AI_DRAM_ADDR_REG = (AI_DRAM_ADDR_REG & ~Mask) | (MaskedValue); break;
    case 0x04500004:
        AI_LEN_REG = (AI_LEN_REG & ~Mask) | (MaskedValue);
        if (bFixedAudio())
        {
            LenChanged();
        }
        else
        {
            if (m_Plugins->Audio()->AiLenChanged != nullptr)
            {
                m_Plugins->Audio()->AiLenChanged();
            }
        }
        break;
    case 0x04500008: AI_CONTROL_REG = (MaskedValue & 1); break;
    case 0x0450000C:
        m_Reg.MI_INTR_REG &= ~MI_INTR_AI;
        m_Reg.m_AudioIntrReg &= ~MI_INTR_AI;
        m_Reg.CheckInterrupts();
        break;
    case 0x04500010:
        AI_DACRATE_REG = (AI_DACRATE_REG & ~Mask) | (MaskedValue);
        m_Plugins->Audio()->DacrateChanged(m_System.SystemType());
        if (bFixedAudio())
        {
            SetFrequency(AI_DACRATE_REG, m_System.SystemType());
        }
        break;
    case 0x04500014: AI_BITRATE_REG = (AI_BITRATE_REG & ~Mask) | (MaskedValue); break;
    default:
        if (HaveDebugger())
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
    }
    return true;
}

void AudioInterfaceHandler::TimerInterrupt(void)
{
    WriteTrace(TraceAudio, TraceDebug, "Start (m_SecondBuff = %d)", m_SecondBuff);
    m_Status &= ~AI_STATUS_FIFO_FULL;
    g_Reg->MI_INTR_REG |= MI_INTR_AI;
    g_Reg->CheckInterrupts();
    if (m_SecondBuff != 0)
    {
        g_SystemTimer->SetTimer(CSystemTimer::AiTimerInterrupt, m_SecondBuff * m_CountsPerByte, false);
        m_SecondBuff = 0;
    }
    else
    {
        m_Status &= ~AI_STATUS_DMA_BUSY;
    }
    WriteTrace(TraceAudio, TraceDebug, "Done");
}

void AudioInterfaceHandler::TimerBusy(void)
{
    WriteTrace(TraceAudio, TraceDebug, "Start (m_SecondBuff = %d)", m_SecondBuff);
    g_Notify->BreakPoint(__FILE__, __LINE__);
    m_Status &= ~AI_STATUS_DMA_BUSY;
}

void AudioInterfaceHandler::SetViIntr(uint32_t VI_INTR_TIME)
{
    double CountsPerSecond = (uint32_t)((double)VI_INTR_TIME * m_FramesPerSecond);
    if (m_BytesPerSecond != 0 && (g_System->AiCountPerBytes() == 0))
    {
        m_CountsPerByte = (int32_t)((double)CountsPerSecond / (double)m_BytesPerSecond);
    }
}

void AudioInterfaceHandler::SetFrequency(uint32_t Dacrate, uint32_t System)
{
    WriteTrace(TraceAudio, TraceDebug, "(Dacrate: %X System: %d): AI_BITRATE_REG = %X", Dacrate, System, g_Reg->AI_BITRATE_REG);
    uint32_t Frequency;

    switch (System)
    {
    case SYSTEM_PAL: Frequency = 49656530 / (Dacrate + 1); break;
    case SYSTEM_MPAL: Frequency = 48628316 / (Dacrate + 1); break;
    default: Frequency = 48681812 / (Dacrate + 1); break;
    }

    //nBlockAlign = 16 / 8 * 2;
    m_BytesPerSecond = Frequency * 4;
    //m_BytesPerSecond = 194532;
    //m_BytesPerSecond = 128024;

    m_FramesPerSecond = System == SYSTEM_PAL ? 50 : 60;
}

void AudioInterfaceHandler::LoadedGameState(void)
{
    if (bFixedAudio())
    {
        SetFrequency(m_Reg.AI_DACRATE_REG, SystemType());
    }
}

void AudioInterfaceHandler::SystemReset(void)
{
    m_Status = 0;
    m_SecondBuff = 0;
    m_BytesPerSecond = 0;
    m_CountsPerByte = g_System->AiCountPerBytes();
    if (m_CountsPerByte == 0)
    {
        m_CountsPerByte = 500;
    }
    m_FramesPerSecond = 60;
}

uint32_t AudioInterfaceHandler::GetLength(void)
{
    WriteTrace(TraceAudio, TraceDebug, "Start (m_SecondBuff = %d)", m_SecondBuff);
    uint32_t TimeLeft = g_SystemTimer->GetTimer(CSystemTimer::AiTimerInterrupt), Res = 0;
    if (TimeLeft > 0)
    {
        Res = (TimeLeft / m_CountsPerByte) & ~7;
    }
    WriteTrace(TraceAudio, TraceDebug, "Done (res = %d, TimeLeft = %d)", Res, TimeLeft);
    return Res;
}

uint32_t AudioInterfaceHandler::GetStatus(void)
{
    WriteTrace(TraceAudio, TraceDebug, "m_Status = %X", m_Status);
    return m_Status;
}

void AudioInterfaceHandler::LenChanged()
{
    WriteTrace(TraceAudio, TraceDebug, "Start (g_Reg->AI_LEN_REG = %d)", g_Reg->AI_LEN_REG);
    if (g_Reg->AI_LEN_REG != 0)
    {
        if (g_Reg->AI_LEN_REG >= 0x40000)
        {
            WriteTrace(TraceAudio, TraceDebug, "Ignoring write, to large (%X)", g_Reg->AI_LEN_REG);
        }
        else
        {
            m_Status |= AI_STATUS_DMA_BUSY;
            uint32_t AudioLeft = g_SystemTimer->GetTimer(CSystemTimer::AiTimerInterrupt);
            if (m_SecondBuff == 0)
            {
                if (AudioLeft == 0)
                {
                    WriteTrace(TraceAudio, TraceDebug, "Set timer  AI_LEN_REG: %d m_CountsPerByte: %d", g_Reg->AI_LEN_REG, m_CountsPerByte);
                    g_SystemTimer->SetTimer(CSystemTimer::AiTimerInterrupt, g_Reg->AI_LEN_REG * m_CountsPerByte, false);
                }
                else
                {
                    WriteTrace(TraceAudio, TraceDebug, "Increasing second buffer (m_SecondBuff %d Increase: %d)", m_SecondBuff, g_Reg->AI_LEN_REG);
                    m_SecondBuff += g_Reg->AI_LEN_REG;
                    m_Status |= AI_STATUS_FIFO_FULL;
                }
            }
            else
            {
                g_Notify->BreakPoint(__FILE__, __LINE__);
            }
        }
    }
    else
    {
        WriteTrace(TraceAudio, TraceDebug, "Reset timer to 0");
        g_SystemTimer->StopTimer(CSystemTimer::AiTimerBusy);
        g_SystemTimer->StopTimer(CSystemTimer::AiTimerInterrupt);
        m_SecondBuff = 0;
        m_Status = 0;
    }

    if (g_Plugins->Audio()->AiLenChanged != nullptr)
    {
        WriteTrace(TraceAudio, TraceDebug, "Calling plugin AiLenChanged");
        g_Plugins->Audio()->AiLenChanged();
        WriteTrace(TraceAudio, TraceDebug, "Plugin AiLenChanged Done");
    }
    WriteTrace(TraceAudio, TraceDebug, "Done");
}
