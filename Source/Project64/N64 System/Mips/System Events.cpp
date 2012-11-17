#include "stdafx.h"

CSystemEvents::CSystemEvents() :
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

void CSystemEvents::ExecuteEvents ( void )
{
	CGuard Guard(m_CS);

	m_bDoSomething = false;
	if (m_Events.size() == 0)
	{
		return;
	}
	
	EventList Events = m_Events;
	m_Events.clear();
	bool bPause = false, bLoadedSave = false;
	for (EventList::const_iterator iter = Events.begin(); !bLoadedSave && iter != Events.end(); iter++ )
	{
		switch (*iter)
		{
		case SysEvent_CloseCPU:
			g_System->m_EndEmulation = true;
			break;
		case SysEvent_ResetCPU_Soft:
			_SystemTimer->SetTimer(CSystemTimer::SoftResetTimer,0x3000000,false);
			g_Plugins->Gfx()->ShowCFB();
			g_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP4;
			g_Reg->CheckInterrupts();
			g_Plugins->Gfx()->SoftReset();
			break;
		case SysEvent_ResetCPU_SoftDone:
			g_System->Reset(true,false);
			break;
		case SysEvent_ResetCPU_Hard:
			g_System->Reset(true,true);
			break;
		case SysEvent_Profile_GenerateLogs:
			g_BaseSystem->m_Profile.GenerateLog();
			break;
		case SysEvent_Profile_StartStop:
		case SysEvent_Profile_ResetLogs:
			g_System->m_Profile.ResetCounters();
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
			if (!g_System->SaveState()) 
			{
				m_Events.push_back(SysEvent_SaveMachineState);
				m_bDoSomething = true;
			}
			break;
		case SysEvent_LoadMachineState:
			if (g_System->LoadState())
			{
				bLoadedSave = true;
			}
			break;
		case SysEvent_ChangePlugins:
			ChangePluginFunc();
			break;
		case SysEvent_ChangingFullScreen:
			g_Notify->ChangeFullScreen();
			break;
		case SysEvent_GSButtonPressed:
			if (g_BaseSystem == NULL)
				return;
			if (g_BaseSystem->m_Cheats.CheatsSlectionChanged())
				g_BaseSystem->m_Cheats.LoadCheats(false);
			g_BaseSystem->m_Cheats.ApplyGSButton(g_MMU);
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
		default:
			g_Notify->BreakPoint(__FILE__,__LINE__);
			break;
		}
	}

	if (bPause)
	{
		g_BaseSystem->Pause();
	}
}

void CSystemEvents::ChangePluginFunc ( void )
{
	g_Notify->DisplayMessage(0,MSG_PLUGIN_INIT);
	if (g_Settings->LoadBool(Plugin_GFX_Changed))
	{
		g_Plugins->Reset(PLUGIN_TYPE_GFX);
	}
	if (g_Settings->LoadBool(Plugin_AUDIO_Changed))
	{
		g_Plugins->Reset(PLUGIN_TYPE_AUDIO);
	}	
	if (g_Settings->LoadBool(Plugin_CONT_Changed))
	{
		g_Plugins->Reset(PLUGIN_TYPE_CONTROLLER);
	}	
	if (g_Settings->LoadBool(Plugin_RSP_Changed) || 
		g_Settings->LoadBool(Plugin_AUDIO_Changed) || 
		g_Settings->LoadBool(Plugin_GFX_Changed))
	{
		g_Plugins->Reset(PLUGIN_TYPE_RSP);
	}
	g_Settings->SaveBool(Plugin_RSP_Changed,  false);
	g_Settings->SaveBool(Plugin_AUDIO_Changed,false);
	g_Settings->SaveBool(Plugin_GFX_Changed,  false);
	g_Settings->SaveBool(Plugin_CONT_Changed, false);
	g_Notify->RefreshMenu();
	if (!g_Plugins->Initiate()) 
	{
		g_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
		g_BaseSystem->m_EndEmulation = true;
	}
	g_Recompiler->ResetRecompCode();
}
