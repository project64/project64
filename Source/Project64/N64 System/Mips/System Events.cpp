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
			_System->m_EndEmulation = true;
			break;
		case SysEvent_ResetCPU_Soft:
			_SystemTimer->SetTimer(CSystemTimer::SoftResetTimer,0x3000000,false);
			_Plugins->Gfx()->ShowCFB();
			_Reg->FAKE_CAUSE_REGISTER |= CAUSE_IP4;
			_Reg->CheckInterrupts();
			_Plugins->Gfx()->SoftReset();
			break;
		case SysEvent_ResetCPU_SoftDone:
			_System->Reset(true,false);
			break;
		case SysEvent_ResetCPU_Hard:
			_System->Reset(true,true);
			break;
		case SysEvent_Profile_GenerateLogs:
			GenerateProfileLog();
			break;
		case SysEvent_Profile_StartStop:
		case SysEvent_Profile_ResetLogs:
			ResetTimer();
			break;
		case SysEvent_ExecuteInterrupt:
			_Reg->DoIntrException(false);
			break;
		case SysEvent_Interrupt_SP:
			_Reg->MI_INTR_REG |= MI_INTR_SP;
			_Reg->DoIntrException(false);
			break;
		case SysEvent_Interrupt_SI:
			_Reg->MI_INTR_REG |= MI_INTR_SI;
			_Reg->DoIntrException(false);
			break;
		case SysEvent_Interrupt_AI:
			_Reg->MI_INTR_REG |= MI_INTR_AI;
			_Reg->DoIntrException(false);
			break;
		case SysEvent_Interrupt_VI:
			_Reg->MI_INTR_REG |= MI_INTR_VI;
			_Reg->DoIntrException(false);
			break;
		case SysEvent_Interrupt_PI:
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->DoIntrException(false);
			break;
		case SysEvent_Interrupt_DP:
			_Reg->MI_INTR_REG |= MI_INTR_DP;
			_Reg->DoIntrException(false);
			break;
		case SysEvent_SaveMachineState:
			if (!Machine_SaveState()) 
			{
				m_Events.push_back(SysEvent_SaveMachineState);
				m_bDoSomething = true;
			}
			break;
		case SysEvent_LoadMachineState:
			if (Machine_LoadState())
			{
				bLoadedSave = true;
			}
			break;
		case SysEvent_ChangePlugins:
			ChangePluginFunc();
			break;
		case SysEvent_ChangingFullScreen:
			ChangeFullScreenFunc();
			break;
		case SysEvent_GSButtonPressed:
			ApplyGSButtonCheats();
			break;
		case SysEvent_PauseCPU_FromMenu:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_FromMenu);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_AppLostFocus:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostFocus);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_AppLostActive:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostActive);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_SaveGame:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SaveGame);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_LoadGame:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_LoadGame);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_DumpMemory:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_DumpMemory);
				bPause = true;
			}
			break;
		case SysEvent_PauseCPU_SearchMemory:
			if (!_Settings->LoadBool(GameRunning_CPU_Paused))
			{
				_Settings->SaveBool(GameRunning_CPU_Paused,true);
				_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SearchMemory);
				bPause = true;
			}
			break;
		default:
			_Notify->BreakPoint(__FILE__,__LINE__);
			break;
		}
	}

	if (bPause)
	{
		PauseExecution();
	}
}

void CSystemEvents::ChangePluginFunc ( void )
{
	_Notify->DisplayMessage(0,MSG_PLUGIN_INIT);
	if (_Settings->LoadBool(Plugin_GFX_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_GFX);
	}
	if (_Settings->LoadBool(Plugin_AUDIO_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_AUDIO);
	}	
	if (_Settings->LoadBool(Plugin_CONT_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_CONTROLLER);
	}	
	if (_Settings->LoadBool(Plugin_RSP_Changed) || 
		_Settings->LoadBool(Plugin_AUDIO_Changed) || 
		_Settings->LoadBool(Plugin_GFX_Changed))
	{
		_Plugins->Reset(PLUGIN_TYPE_RSP);
	}
	_Settings->SaveBool(Plugin_RSP_Changed,  false);
	_Settings->SaveBool(Plugin_AUDIO_Changed,false);
	_Settings->SaveBool(Plugin_GFX_Changed,  false);
	_Settings->SaveBool(Plugin_CONT_Changed, false);
	_Notify->RefreshMenu();
	if (!_Plugins->Initiate()) 
	{
		_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
		_BaseSystem->m_EndEmulation = true;
	} else {
		//CC_Core::SetCurrentSystem(_N64System);
	}
	_Recompiler->ResetRecompCode();
}
