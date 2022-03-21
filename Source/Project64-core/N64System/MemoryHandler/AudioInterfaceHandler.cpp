#include "stdafx.h"
#include "AudioInterfaceHandler.h"
#include <Project64-core\N64System\N64System.h>
#include <Project64-core\N64System\Mips\Register.h>
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
    m_PC(Reg.m_PROGRAM_COUNTER)
{
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
            Value = m_Audio.GetLength();
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
            Value = m_Audio.GetStatus();
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
        case 0x04500000: LogMessage("%08X: read from AI_DRAM_ADDR_REG (%08X)", m_PC, Value); break;
        case 0x04500004: LogMessage("%08X: read from AI_LEN_REG (%08X)", m_PC, Value); break;
        case 0x04500008: LogMessage("%08X: read from AI_CONTROL_REG (%08X)", m_PC, Value); break;
        case 0x0450000C: LogMessage("%08X: read from AI_STATUS_REG (%08X)", m_PC, Value); break;
        case 0x04500010: LogMessage("%08X: read from AI_DACRATE_REG (%08X)", m_PC, Value); break;
        case 0x04500014: LogMessage("%08X: read from AI_BITRATE_REG (%08X)", m_PC, Value); break;
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
        case 0x04500000: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to AI_DRAM_ADDR_REG", m_PC, Value, Mask); break;
        case 0x04500004: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to AI_LEN_REG", m_PC, Value, Mask); break;
        case 0x04500008: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to AI_CONTROL_REG", m_PC, Value, Mask); break;
        case 0x0450000C: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to AI_STATUS_REG", m_PC, Value, Mask); break;
        case 0x04500010: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to AI_DACRATE_REG", m_PC, Value, Mask); break;
        case 0x04500014: LogMessage("%08X: Writing 0x%08X (Mask: 0x%08X) to AI_BITRATE_REG", m_PC, Value, Mask); break;
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
            m_Audio.LenChanged();
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
            m_Audio.SetFrequency(AI_DACRATE_REG, m_System.SystemType());
        }
        break;
    case 0x04500014: AI_DACRATE_REG = (AI_BITRATE_REG & ~Mask) | (MaskedValue); break;
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
    m_Audio.InterruptTimerDone();
}

void AudioInterfaceHandler::TimerBusy(void)
{
    m_Audio.BusyTimerDone();
}

void AudioInterfaceHandler::SetViIntr(uint32_t VI_INTR_TIME)
{
    m_Audio.SetViIntr(VI_INTR_TIME);
}

void AudioInterfaceHandler::SetFrequency(uint32_t Dacrate, uint32_t System)
{
    m_Audio.SetFrequency(Dacrate, System);
}

void AudioInterfaceHandler::LoadedGameState(void)
{
    if (bFixedAudio())
    {
        m_Audio.SetFrequency(m_Reg.AI_DACRATE_REG, SystemType());
    }
}

void AudioInterfaceHandler::SystemReset(void)
{
    m_Audio.Reset();
}

uint32_t AudioInterfaceHandler::GetLength(void)
{
    return m_Audio.GetLength();
}

uint32_t AudioInterfaceHandler::GetStatus(void)
{
    return m_Audio.GetStatus();
}
