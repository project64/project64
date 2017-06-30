/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/Mips/Disk.h>
#include <Project64-core/N64System/N64Class.h>
#include <Project64-core/3rdParty/zip.h>

CSystemTimer::CSystemTimer(CRegisters &Reg, int32_t & NextTimer) :
    m_LastUpdate(0),
    m_NextTimer(NextTimer),
    m_Current(UnknownTimer),
    m_inFixTimer(false),
    m_Reg(Reg)
{
    memset(m_TimerDetatils, 0, sizeof(m_TimerDetatils));
}

void CSystemTimer::Reset()
{
    //initialise Structure
    for (int i = 0; i < MaxTimer; i++)
    {
        m_TimerDetatils[i].Active = false;
        m_TimerDetatils[i].CyclesToTimer = 0;
    }
    m_Current = UnknownTimer;
    m_LastUpdate = 0;
    m_NextTimer = 0;

    SetTimer(ViTimer, 50000, false);
    SetCompareTimer();
}

void CSystemTimer::SetTimer(TimerType Type, uint32_t Cycles, bool bRelative)
{
    Cycles *= CGameSettings::OverClockModifier();
    if (Type >= MaxTimer || Type == UnknownTimer)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    UpdateTimers();

    m_TimerDetatils[Type].Active = true;
    if (bRelative)
    {
        if (m_TimerDetatils[Type].Active)
        {
            m_TimerDetatils[Type].CyclesToTimer += Cycles; //Add to the timer
        }
        else
        {
            m_TimerDetatils[Type].CyclesToTimer = (int64_t)Cycles - (int64_t)m_NextTimer;  //replace the new cycles
        }
    }
    else
    {
        m_TimerDetatils[Type].CyclesToTimer = (int64_t)Cycles - (int64_t)m_NextTimer;  //replace the new cycles
    }
    FixTimers();
}

uint32_t CSystemTimer::GetTimer(TimerType Type)
{
    if (Type >= MaxTimer || Type == UnknownTimer)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return 0;
    }
    if (!m_TimerDetatils[Type].Active)
    {
        return 0;
    }
    int64_t CyclesToTimer = m_TimerDetatils[Type].CyclesToTimer + m_NextTimer;
    if (CyclesToTimer < 0)
    {
        return 0;
    }
    if (CyclesToTimer > 0x7FFFFFFF)
    {
        return 0x7FFFFFFF;
    }
    return (uint32_t)(CyclesToTimer / CGameSettings::OverClockModifier());
}

void CSystemTimer::StopTimer(TimerType Type)
{
    if (Type >= MaxTimer || Type == UnknownTimer)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    m_TimerDetatils[Type].Active = false;
    FixTimers();
}

void CSystemTimer::FixTimers()
{
    if (m_inFixTimer)
    {
        return;
    }
    m_inFixTimer = true;

    UpdateTimers();
    if (GetTimer(CompareTimer) > 0x60000000)
    {
        SetCompareTimer();
    }

    //Update the cycles for the remaining number of cycles to timer
    int count;
    for (count = 0; count < MaxTimer; count++)
    {
        if (!m_TimerDetatils[count].Active)
        {
            continue;
        }
        m_TimerDetatils[count].CyclesToTimer += m_NextTimer;
    }

    //Set Max timer
    m_NextTimer = 0x7FFFFFFF;

    //Find the smallest timer left to go
    for (count = 0; count < MaxTimer; count++)
    {
        if (!m_TimerDetatils[count].Active)
        {
            continue;
        }
        if (m_TimerDetatils[count].CyclesToTimer >= m_NextTimer)
        {
            continue;
        }
        m_NextTimer = (int)m_TimerDetatils[count].CyclesToTimer;
        m_Current = (TimerType)count;
    }

    //Move the timer back this value
    for (count = 0; count < MaxTimer; count++)
    {
        if (!m_TimerDetatils[count].Active)
        {
            continue;
        }
        m_TimerDetatils[count].CyclesToTimer -= m_NextTimer;
    }
    m_LastUpdate = m_NextTimer;
    m_inFixTimer = false;
}

void CSystemTimer::UpdateTimers()
{
    int TimeTaken = m_LastUpdate - m_NextTimer;
    if (TimeTaken != 0)
    {
        int32_t random, wired;
        m_LastUpdate = m_NextTimer;
        m_Reg.COUNT_REGISTER += (TimeTaken / CGameSettings::OverClockModifier());
        random = m_Reg.RANDOM_REGISTER - (TimeTaken / g_System->CountPerOp());
        wired = m_Reg.WIRED_REGISTER;
        if (random < wired)
        {
            if (wired == 0)
            {
                random &= 31;
            }
            else
            {
                uint32_t increment = 32 - wired;
                random += ((wired - random + increment - 1) / increment) * increment;
            }
        }
        m_Reg.RANDOM_REGISTER = random;
    }
}

void CSystemTimer::TimerDone()
{
    UpdateTimers();

    switch (m_Current)
    {
    case CSystemTimer::CompareTimer:
        m_Reg.FAKE_CAUSE_REGISTER |= CAUSE_IP7;
        m_Reg.CheckInterrupts();
        UpdateCompareTimer();
        break;
    case CSystemTimer::SoftResetTimer:
        g_SystemTimer->StopTimer(CSystemTimer::SoftResetTimer);
        g_System->ExternalEvent(SysEvent_ResetCPU_SoftDone);
        break;
    case CSystemTimer::SiTimer:
        g_SystemTimer->StopTimer(CSystemTimer::SiTimer);
        m_Reg.MI_INTR_REG |= MI_INTR_SI;
        m_Reg.SI_STATUS_REG |= SI_STATUS_INTERRUPT;
        m_Reg.CheckInterrupts();
        break;
    case CSystemTimer::PiTimer:
        g_SystemTimer->StopTimer(CSystemTimer::PiTimer);
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        m_Reg.MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        break;
    case CSystemTimer::DDPiTimer:
        g_SystemTimer->StopTimer(CSystemTimer::DDPiTimer);
        m_Reg.PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
        DiskBMUpdate();
        m_Reg.MI_INTR_REG |= MI_INTR_PI;
        m_Reg.CheckInterrupts();
        break;
    case CSystemTimer::ViTimer:
        try
        {
            g_System->RefreshScreen();
        }
        catch (...)
        {
            WriteTrace(TraceN64System, TraceError, "Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__);
        }
        m_Reg.MI_INTR_REG |= MI_INTR_VI;
        m_Reg.CheckInterrupts();
        break;
    case CSystemTimer::RspTimer:
        g_SystemTimer->StopTimer(CSystemTimer::RspTimer);
        try
        {
            g_System->RunRSP();
        }
        catch (...)
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case CSystemTimer::RSPTimerDlist:
        g_SystemTimer->StopTimer(CSystemTimer::RSPTimerDlist);
        m_Reg.m_GfxIntrReg |= MI_INTR_DP;
        m_Reg.CheckInterrupts();
        break;
    case CSystemTimer::AiTimerInterrupt:
        g_SystemTimer->StopTimer(CSystemTimer::AiTimerInterrupt);
        g_Audio->InterruptTimerDone();
        break;
    case CSystemTimer::AiTimerBusy:
        g_SystemTimer->StopTimer(CSystemTimer::AiTimerBusy);
        g_Audio->BusyTimerDone();
        break;
    default:
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }
    //CheckTimer();
    /*if (Profiling)
    {
    StartTimer(LastTimer);
    }*/
}

void CSystemTimer::SetCompareTimer()
{
    uint32_t NextCompare = 0x7FFFFFFF;
    if (g_Reg)
    {
        NextCompare = m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER;
        if ((NextCompare & 0x80000000) != 0)
        {
            NextCompare = 0x7FFFFFFF;
        }
    }
    SetTimer(CompareTimer, NextCompare, false);
}

void CSystemTimer::UpdateCompareTimer(void)
{
    SetCompareTimer();
}

bool CSystemTimer::SaveAllowed(void)
{
    if (GetTimer(CompareTimer) <= 0)
    {
        return false;
    }
    for (int i = 0; i < MaxTimer; i++)
    {
        if (i == CompareTimer)
        {
            continue;
        }
        if (i == ViTimer)
        {
            continue;
        }
        if (m_TimerDetatils[i].Active)
        {
            return false;
        }
    }
    return true;
}

void CSystemTimer::SaveData(zipFile & file) const
{
    uint32_t TimerDetailsSize = sizeof(TIMER_DETAILS);
    uint32_t Entries = sizeof(m_TimerDetatils) / sizeof(m_TimerDetatils[0]);
    zipWriteInFileInZip(file, &TimerDetailsSize, sizeof(TimerDetailsSize));
    zipWriteInFileInZip(file, &Entries, sizeof(Entries));
    zipWriteInFileInZip(file, (void *)&m_TimerDetatils, sizeof(m_TimerDetatils));
    zipWriteInFileInZip(file, (void *)&m_LastUpdate, sizeof(m_LastUpdate));
    zipWriteInFileInZip(file, &m_NextTimer, sizeof(m_NextTimer));
    zipWriteInFileInZip(file, (void *)&m_Current, sizeof(m_Current));
}

void CSystemTimer::SaveData(CFile & file) const
{
    uint32_t TimerDetailsSize = sizeof(TIMER_DETAILS);
    uint32_t Entries = sizeof(m_TimerDetatils) / sizeof(m_TimerDetatils[0]);

    file.Write(&TimerDetailsSize, sizeof(TimerDetailsSize));
    file.Write(&Entries, sizeof(Entries));
    file.Write((void *)&m_TimerDetatils, sizeof(m_TimerDetatils));
    file.Write((void *)&m_LastUpdate, sizeof(m_LastUpdate));
    file.Write(&m_NextTimer, sizeof(m_NextTimer));
    file.Write((void *)&m_Current, sizeof(m_Current));
}

void CSystemTimer::LoadData(zipFile & file)
{
    uint32_t TimerDetailsSize, Entries;

    unzReadCurrentFile(file, &TimerDetailsSize, sizeof(TimerDetailsSize));
    unzReadCurrentFile(file, &Entries, sizeof(Entries));

    if (TimerDetailsSize != sizeof(TIMER_DETAILS))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (Entries != sizeof(m_TimerDetatils) / sizeof(m_TimerDetatils[0]))
    {
        if (Entries < (sizeof(m_TimerDetatils) / sizeof(m_TimerDetatils[0])))
        {
            memset((void *)&m_TimerDetatils, 0, sizeof(m_TimerDetatils));
            unzReadCurrentFile(file, (void *)&m_TimerDetatils, Entries * sizeof(m_TimerDetatils[0]));
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
            return;
        }
    }
    else
    {
        unzReadCurrentFile(file, (void *)&m_TimerDetatils, sizeof(m_TimerDetatils));
    }
    unzReadCurrentFile(file, (void *)&m_LastUpdate, sizeof(m_LastUpdate));
    unzReadCurrentFile(file, &m_NextTimer, sizeof(m_NextTimer));
    unzReadCurrentFile(file, (void *)&m_Current, sizeof(m_Current));
}

void CSystemTimer::LoadData(CFile & file)
{
    uint32_t TimerDetailsSize, Entries;

    file.Read(&TimerDetailsSize, sizeof(TimerDetailsSize));
    file.Read(&Entries, sizeof(Entries));

    if (TimerDetailsSize != sizeof(TIMER_DETAILS))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }
    if (Entries != sizeof(m_TimerDetatils) / sizeof(m_TimerDetatils[0]))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
        return;
    }

    file.Read((void *)&m_TimerDetatils, sizeof(m_TimerDetatils));
    file.Read((void *)&m_LastUpdate, sizeof(m_LastUpdate));
    file.Read(&m_NextTimer, sizeof(m_NextTimer));
    file.Read((void *)&m_Current, sizeof(m_Current));
}

void CSystemTimer::RecordDifference(CLog &LogFile, const CSystemTimer& rSystemTimer)
{
    if (m_LastUpdate != rSystemTimer.m_LastUpdate)
    {
        LogFile.LogF("Timer-LastUpdate: %X %X\r\n", m_LastUpdate, rSystemTimer.m_LastUpdate);
    }
    if (m_NextTimer != rSystemTimer.m_NextTimer)
    {
        LogFile.LogF("Timer-NextTimer: %X %X\r\n", m_NextTimer, rSystemTimer.m_NextTimer);
    }
    if (m_Current != rSystemTimer.m_Current)
    {
        LogFile.LogF("Timer-Current %X %X\r\n", m_Current, rSystemTimer.m_Current);
    }
    if (m_inFixTimer != rSystemTimer.m_inFixTimer)
    {
        LogFile.LogF("Timer-inFixTimer %X %X\r\n", (int)m_inFixTimer, (int)rSystemTimer.m_inFixTimer);
    }

    for (int i = 0; i < MaxTimer; i++)
    {
        if (m_TimerDetatils[i].Active != rSystemTimer.m_TimerDetatils[i].Active)
        {
            LogFile.LogF("Timer-m_TimerDetatils[%d] %X %X\r\n", i, (int)m_TimerDetatils[i].Active, (int)rSystemTimer.m_TimerDetatils[i].Active);
        }
        if (m_TimerDetatils[i].CyclesToTimer != rSystemTimer.m_TimerDetatils[i].CyclesToTimer)
        {
            LogFile.LogF("Timer-m_TimerDetatils[%d] 0x%08X, 0x%08X\r\n", i, (uint32_t)m_TimerDetatils[i].CyclesToTimer, (uint32_t)rSystemTimer.m_TimerDetatils[i].CyclesToTimer);
        }
    }
}

bool CSystemTimer::operator == (const CSystemTimer& rSystemTimer) const
{
    if (m_LastUpdate != rSystemTimer.m_LastUpdate)
    {
        return false;
    }
    if (m_NextTimer != rSystemTimer.m_NextTimer)
    {
        return false;
    }
    if (m_Current != rSystemTimer.m_Current)
    {
        return false;
    }
    if (m_inFixTimer != rSystemTimer.m_inFixTimer)
    {
        return false;
    }

    for (int i = 0; i < MaxTimer; i++)
    {
        if (m_TimerDetatils[i].Active != rSystemTimer.m_TimerDetatils[i].Active)
        {
            return false;
        }
        if (m_TimerDetatils[i].CyclesToTimer != rSystemTimer.m_TimerDetatils[i].CyclesToTimer)
        {
            return false;
        }
    }
    return true;
}

bool CSystemTimer::operator != (const CSystemTimer& rSystemTimer) const
{
    return !(*this == rSystemTimer);
}