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
#include <Project64-core/N64System/Mips/SystemEvents.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/N64Class.h>

const char * SystemEventName(SystemEvent event)
{
    switch (event)
    {
    case SysEvent_ExecuteInterrupt: return "SysEvent_ExecuteInterrupt";
    case SysEvent_GSButtonPressed: return "SysEvent_GSButtonPressed";
    case SysEvent_ResetCPU_Soft: return "SysEvent_ResetCPU_Soft";
    case SysEvent_ResetCPU_SoftDone: return "SysEvent_ResetCPU_SoftDone";
    case SysEvent_ResetCPU_Hard: return "SysEvent_ResetCPU_Hard";
    case SysEvent_CloseCPU: return "SysEvent_CloseCPU";
    case SysEvent_PauseCPU_FromMenu: return "SysEvent_PauseCPU_FromMenu";
    case SysEvent_PauseCPU_AppLostActive: return "SysEvent_PauseCPU_AppLostActive";
    case SysEvent_PauseCPU_AppLostActiveDelay: return "SysEvent_PauseCPU_AppLostActiveDelay";
    case SysEvent_PauseCPU_AppLostFocus: return "SysEvent_PauseCPU_AppLostFocus";
    case SysEvent_PauseCPU_SaveGame: return "SysEvent_PauseCPU_SaveGame";
    case SysEvent_PauseCPU_LoadGame: return "SysEvent_PauseCPU_LoadGame";
    case SysEvent_PauseCPU_DumpMemory: return "SysEvent_PauseCPU_DumpMemory";
    case SysEvent_PauseCPU_SearchMemory: return "SysEvent_PauseCPU_SearchMemory";
    case SysEvent_PauseCPU_Settings: return "SysEvent_PauseCPU_Settings";
    case SysEvent_PauseCPU_Cheats: return "SysEvent_PauseCPU_Cheats";
    case SysEvent_ResumeCPU_FromMenu: return "SysEvent_ResumeCPU_FromMenu";
    case SysEvent_ResumeCPU_AppGainedActive: return "SysEvent_ResumeCPU_AppGainedActive";
    case SysEvent_ResumeCPU_AppGainedFocus: return "SysEvent_ResumeCPU_AppGainedFocus";
    case SysEvent_ResumeCPU_SaveGame: return "SysEvent_ResumeCPU_SaveGame";
    case SysEvent_ResumeCPU_LoadGame: return "SysEvent_ResumeCPU_LoadGame";
    case SysEvent_ResumeCPU_DumpMemory: return "SysEvent_ResumeCPU_DumpMemory";
    case SysEvent_ResumeCPU_SearchMemory: return "SysEvent_ResumeCPU_SearchMemory";
    case SysEvent_ResumeCPU_Settings: return "SysEvent_ResumeCPU_Settings";
    case SysEvent_ResumeCPU_Cheats: return "SysEvent_ResumeCPU_Cheats";
    case SysEvent_ChangingFullScreen: return "SysEvent_ChangingFullScreen";
    case SysEvent_ChangePlugins: return "SysEvent_ChangePlugins";
    case SysEvent_SaveMachineState: return "SysEvent_SaveMachineState";
    case SysEvent_LoadMachineState: return "SysEvent_LoadMachineState";
    case SysEvent_Interrupt_SP: return "SysEvent_Interrupt_SP";
    case SysEvent_Interrupt_SI: return "SysEvent_Interrupt_SI";
    case SysEvent_Interrupt_AI: return "SysEvent_Interrupt_AI";
    case SysEvent_Interrupt_VI: return "SysEvent_Interrupt_VI";
    case SysEvent_Interrupt_PI: return "SysEvent_Interrupt_PI";
    case SysEvent_Interrupt_DP: return "SysEvent_Interrupt_DP";
    case SysEvent_ResetFunctionTimes: return "SysEvent_ResetFunctionTimes";
    case SysEvent_DumpFunctionTimes: return "SysEvent_DumpFunctionTimes";
    }
    static char unknown[100];
    sprintf(unknown, "unknown(%d)", event);
    return unknown;
}

CSystemEvents::CSystemEvents(CN64System * System, CPlugins * Plugins) :
    m_System(System),
    m_Plugins(Plugins),
    m_bDoSomething(false)
{
}

CSystemEvents::~CSystemEvents()
{
}

void CSystemEvents::QueueEvent(SystemEvent action)
{
    CGuard Guard(m_CS);
    for (EventList::const_iterator iter = m_Events.begin(); iter != m_Events.end(); iter++)
    {
        if (*iter == action)
        {
            return;
        }
    }
    m_Events.push_back(action);
    m_bDoSomething = true;
}

void CSystemEvents::ExecuteEvents()
{
    EventList Events;
    {
        CGuard Guard(m_CS);

        m_bDoSomething = false;
        if (m_Events.size() == 0)
        {
            return;
        }

        Events = m_Events;
        m_Events.clear();
    }

    bool bPause = false, bLoadedSave = false;
    for (EventList::const_iterator iter = Events.begin(); !bLoadedSave && iter != Events.end(); iter++)
    {
        switch (*iter)
        {
        case SysEvent_CloseCPU:
            m_System->m_EndEmulation = true;
            break;
        case SysEvent_ResetCPU_Soft:
            m_System->GameReset();
            break;
        case SysEvent_ResetCPU_SoftDone:
            m_System->Reset(true, false);
            break;
        case SysEvent_ResetCPU_Hard:
            m_System->Reset(true, true);
            break;
        case SysEvent_ExecuteInterrupt:
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_Interrupt_SP:
            g_Reg->MI_INTR_REG |= MI_INTR_SP;
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_Interrupt_SI:
            g_Reg->MI_INTR_REG |= MI_INTR_SI;
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_Interrupt_AI:
            g_Reg->MI_INTR_REG |= MI_INTR_AI;
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_Interrupt_VI:
            g_Reg->MI_INTR_REG |= MI_INTR_VI;
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_Interrupt_PI:
            g_Reg->MI_INTR_REG |= MI_INTR_PI;
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_Interrupt_DP:
            g_Reg->MI_INTR_REG |= MI_INTR_DP;
            g_Reg->DoIntrException(false);
            break;
        case SysEvent_SaveMachineState:
            if (!m_System->SaveState())
            {
                m_Events.push_back(SysEvent_SaveMachineState);
                m_bDoSomething = true;
            }
            break;
        case SysEvent_LoadMachineState:
            if (m_System->LoadState())
            {
                bLoadedSave = true;
            }
            break;
        case SysEvent_ChangePlugins:
            ChangePluginFunc();
            break;
        case SysEvent_ResetFunctionTimes:
            if (g_Recompiler)
            {
                g_Recompiler->ResetFunctionTimes();
            }
            break;
        case SysEvent_DumpFunctionTimes:
            if (g_Recompiler)
            {
                g_Recompiler->DumpFunctionTimes();
            }
            break;
        case SysEvent_ChangingFullScreen:
            g_Notify->ChangeFullScreen();
            break;
        case SysEvent_GSButtonPressed:
            if (m_System->HasCheatsSlectionChanged())
            {
                m_System->SetCheatsSlectionChanged(false);
                m_System->m_Cheats.LoadCheats(false, m_Plugins);
            }
            m_System->m_Cheats.ApplyGSButton();
            break;
        case SysEvent_PauseCPU_FromMenu:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                bPause = true;
            }
            g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_FromMenu);
            break;
        case SysEvent_PauseCPU_AppLostFocus:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostFocus);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_AppLostActive:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostActive);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_SaveGame:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SaveGame);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_LoadGame:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_LoadGame);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_DumpMemory:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_DumpMemory);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_SearchMemory:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SearchMemory);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_Settings:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_Settings);
                bPause = true;
            }
            break;
        case SysEvent_PauseCPU_Cheats:
            if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
            {
                g_Settings->SaveBool(GameRunning_CPU_Paused, true);
                g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_Cheats);
                bPause = true;
            }
            break;
        default:
            g_Notify->BreakPoint(__FILE__, __LINE__);
            break;
        }
    }

    if (bPause)
    {
        m_System->Pause();
    }
}

void CSystemEvents::ChangePluginFunc()
{
    g_Notify->DisplayMessage(0, MSG_PLUGIN_INIT);
    m_System->PluginReset();
}