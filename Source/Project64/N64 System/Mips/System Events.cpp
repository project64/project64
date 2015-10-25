/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "stdafx.h"

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
	m_bDoSomething = true;

	CGuard Guard(m_CS);
	for (EventList::const_iterator iter = m_Events.begin(); iter != m_Events.end(); iter++)
	{
		if (*iter == action)
		{
			return;
		}
	}
	m_Events.push_back(action);
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
	for (EventList::const_iterator iter = Events.begin(); !bLoadedSave && iter != Events.end(); iter++ )
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
			m_System->Reset(true,false);
			break;
		case SysEvent_ResetCPU_Hard:
			m_System->Reset(true,true);
			break;
		case SysEvent_Profile_GenerateLogs:
			m_System->m_Profile.GenerateLog();
			break;
		case SysEvent_Profile_StartStop:
		case SysEvent_Profile_ResetLogs:
			m_System->m_Profile.ResetCounters();
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
		case SysEvent_ChangingFullScreen:
			Notify().ChangeFullScreen();
			break;
		case SysEvent_GSButtonPressed:
			if (m_System->m_Cheats.CheatsSlectionChanged())
			{
				m_System->m_Cheats.LoadCheats(false, m_Plugins);
			}
			m_System->m_Cheats.ApplyGSButton(g_MMU);
			break;
		case SysEvent_PauseCPU_FromMenu:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_FromMenu);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_AppLostFocus:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostFocus);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_AppLostActive:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostActive);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_SaveGame:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SaveGame);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_LoadGame:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_LoadGame);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_DumpMemory:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_DumpMemory);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_SearchMemory:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SearchMemory);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_Settings:
			if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				g_Settings->SaveBool(GameRunning_CPU_Paused,true);
				g_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_Settings);
				bPause = true;
			}
			break;
		default:
			g_Notify->BreakPoint(__FILEW__,__LINE__);
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
	g_Notify->DisplayMessage(0,MSG_PLUGIN_INIT);
	m_System->PluginReset();
}
