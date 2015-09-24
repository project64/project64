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

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

#include <windows.h>
#include <commdlg.h>

CN64System::CN64System ( CPlugins * Plugins, bool SavesReadOnly ) :
	CSystemEvents(this, Plugins),
	m_Cheats(NULL),
	m_EndEmulation(false),
	m_SaveUsing((SAVE_CHIP_TYPE)g_Settings->LoadDword(Game_SaveChip)),
	m_Plugins(Plugins),
	m_SyncCPU(NULL),
	m_SyncPlugins(NULL),
	m_SyncWindow(NULL),
	m_MMU_VM(this,SavesReadOnly),
	m_TLB(this),
	m_Reg(this,this),
	m_FPS(g_Notify),
	m_Recomp(NULL),
	m_InReset(false),
	m_NextTimer(0),
	m_SystemTimer(m_NextTimer),
	m_bCleanFrameBox(true),
	m_bInitialized(false),
	m_RspBroke(true),
	m_DMAUsed(false),
	m_TestTimer(false),
	m_NextInstruction(0),
	m_JumpToLocation(0),
	m_TLBLoadAddress(0),
	m_TLBStoreAddress(0),
	m_SyncCount(0),
	m_CPU_Handle(NULL),
	m_CPU_ThreadID(0)
{
	DWORD gameHertz = g_Settings->LoadDword(Game_ScreenHertz);
	if (gameHertz == 0)
	{
		gameHertz = (SystemType() == SYSTEM_PAL) ? 50 : 60;
	}
	m_hPauseEvent = CreateEvent(NULL,true,false,NULL);
	m_Limitor.SetHertz(gameHertz);
	g_Settings->SaveDword(GameRunning_ScreenHertz,gameHertz);
	m_Cheats.LoadCheats(!g_Settings->LoadDword(Setting_RememberCheats), Plugins);
}

CN64System::~CN64System()
{
	SetActiveSystem(false);
	Mempak::Close();
	if (m_SyncCPU)
	{
		m_SyncCPU->CpuStopped();
		delete m_SyncCPU;
		m_SyncCPU = NULL;
	}
	if (m_Recomp)
	{
		delete m_Recomp;
		m_Recomp = NULL;
	}
	if (m_SyncPlugins)
	{
		delete m_SyncPlugins;
		m_SyncPlugins = NULL;
	}
	if (m_SyncWindow)
	{
		delete m_SyncWindow;
		m_SyncWindow = NULL;
	}
}

void CN64System::ExternalEvent ( SystemEvent action ) 
{
	switch (action) 
	{
	case SysEvent_Profile_GenerateLogs: 
	case SysEvent_Profile_StartStop: 
	case SysEvent_Profile_ResetLogs: 
	case SysEvent_ExecuteInterrupt: 
	case SysEvent_SaveMachineState: 
	case SysEvent_LoadMachineState: 
	case SysEvent_ChangingFullScreen:
	case SysEvent_GSButtonPressed:
	case SysEvent_ResetCPU_SoftDone:
	case SysEvent_Interrupt_SP:
	case SysEvent_Interrupt_SI:
	case SysEvent_Interrupt_AI:
	case SysEvent_Interrupt_VI:
	case SysEvent_Interrupt_PI:
	case SysEvent_Interrupt_DP:
	case SysEvent_ResetCPU_Hard:
	case SysEvent_ResetCPU_Soft:
	case SysEvent_CloseCPU:	
	case SysEvent_ChangePlugins:
		QueueEvent(action);
		break;
	case SysEvent_PauseCPU_FromMenu: 
	case SysEvent_PauseCPU_AppLostFocus: 
	case SysEvent_PauseCPU_AppLostActive: 
	case SysEvent_PauseCPU_SaveGame: 
	case SysEvent_PauseCPU_LoadGame: 
	case SysEvent_PauseCPU_DumpMemory: 
	case SysEvent_PauseCPU_SearchMemory: 
	case SysEvent_PauseCPU_Settings: 
		if (!g_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			QueueEvent(action);			
		}
		break;
	case SysEvent_ResumeCPU_FromMenu:
		// always resume if from menu
		SetEvent(m_hPauseEvent);
		break;
	case SysEvent_ResumeCPU_AppGainedFocus:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostFocus )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_AppGainedActive:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostActive )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_SaveGame:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SaveGame )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_LoadGame:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_LoadGame )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_DumpMemory:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_DumpMemory )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_SearchMemory:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SearchMemory )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_Settings:
		if (g_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_Settings )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	default:
		WriteTraceF(TraceError,__FUNCTION__ ": Unknown event %d",action);
		g_Notify->BreakPoint(__FILEW__,__LINE__);
	}
}

bool CN64System::RunFileImage ( const char * FileLoc ) 
{
	CloseSystem();
	if (g_Settings->LoadBool(GameRunning_LoadingInProgress))
	{
		return false;
	}
	g_Settings->SaveBool(GameRunning_LoadingInProgress,true);

	WriteTrace(TraceDebug,__FUNCTION__ ": Mark Rom as loading");

	//Mark the rom as loading
	g_Settings->SaveBool(GameRunning_LoadingInProgress,true);
	g_Notify->RefreshMenu();

	//Try to load the passed N64 rom
	if (g_Rom == NULL)
	{
		WriteTrace(TraceDebug,__FUNCTION__ ": Allocating global rom object");
		g_Rom = new CN64Rom();
	} 
	else 
	{
		WriteTrace(TraceDebug,__FUNCTION__ ": Use existing global rom object");
	}

	WriteTraceF(TraceDebug,__FUNCTION__ ": Loading \"%s\"",FileLoc);
	if (g_Rom->LoadN64Image(FileLoc)) 
	{
		g_System->RefreshGameSettings();

		WriteTrace(TraceDebug,__FUNCTION__ ": Add Recent Rom");
		g_Notify->AddRecentRom(FileLoc);
        g_Notify->SetWindowCaption(g_Settings->LoadString(Game_GoodName).ToUTF16().c_str());

		g_Settings->SaveBool(GameRunning_LoadingInProgress, false);
		g_Notify->RefreshMenu();

		if (g_Settings->LoadDword(Setting_AutoStart) != 0)
		{
			g_BaseSystem = new CN64System(g_Plugins,false);
			if (g_BaseSystem)
			{
				g_BaseSystem->StartEmulation(true);
			}
		}
	}
	else
	{
		WriteTraceF(TraceError,__FUNCTION__ ": LoadN64Image failed (\"%s\")",FileLoc);
		g_Notify->DisplayError(g_Rom->GetError());
		delete g_Rom;
		g_Rom = NULL;
		g_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		g_Notify->RefreshMenu();
		return false;
	}
	return true;
}

void CN64System::CloseSystem()
{
	if (g_BaseSystem)
	{
		g_BaseSystem->CloseCpu();
		delete g_BaseSystem;
		g_BaseSystem = NULL;
	}
}

bool CN64System::EmulationStarting ( HANDLE hThread, DWORD ThreadId )
{
	bool bRes = true;

	WriteTrace(TraceDebug, __FUNCTION__ ": Setting N64 system as active");
	if (g_BaseSystem->SetActiveSystem(true))
	{
		g_BaseSystem->m_CPU_Handle   = hThread;
		g_BaseSystem->m_CPU_ThreadID = ThreadId;
		WriteTrace(TraceDebug,__FUNCTION__ ": Setting up N64 system done");
		g_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		g_Notify->RefreshMenu();
		try
		{
			WriteTrace(TraceDebug,__FUNCTION__ ": Game set to auto start, starting");
			g_BaseSystem->StartEmulation2(false);			
			WriteTrace(TraceDebug,__FUNCTION__ ": Game Done");
		} 
		catch (...)
		{
			WriteTraceF(TraceError,__FUNCTION__ ": Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
			char Message[600];
			sprintf(Message,__FUNCTION__ ": Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
			MessageBox(NULL,Message,"Exception",MB_OK);
		}
	}
	else 
	{
		WriteTrace(TraceError,__FUNCTION__ ": SetActiveSystem failed");
		g_Notify->DisplayError(__FUNCTIONW__ L": Failed to Initialize N64 System");
		g_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		g_Notify->RefreshMenu();
		bRes = false;
	}
	return bRes;
}
	
void  CN64System::StartEmulation2   ( bool NewThread )
{
	if (NewThread)
	{
		WriteTrace(TraceDebug,__FUNCTION__ ": Starting");

		g_Notify->HideRomBrowser();

		if (bHaveDebugger()) 
		{
			LogOptions.GenerateLog = g_Settings->LoadDword(Debugger_GenerateDebugLog);
			LoadLogOptions(&LogOptions, FALSE);
			StartLog();
		}

		CInterpreterCPU::BuildCPU();

		DWORD CpuType = g_Settings->LoadDword(Game_CpuType);

		if(CpuType == CPU_SyncCores && !g_Settings->LoadBool(Debugger_Enabled))
		{
			g_Settings->SaveDword(Game_CpuType, CPU_Recompiler);
			CpuType = CPU_Recompiler;
		}

		if (CpuType == CPU_SyncCores)
		{
			g_Notify->DisplayMessage(5,L"Copy Plugins");
			g_Plugins->CopyPlugins(g_Settings->LoadString(Directory_PluginSync));
#if defined(WINDOWS_UI)
			m_SyncWindow = new CMainGui(false);
			m_SyncPlugins = new CPlugins( g_Settings->LoadString(Directory_PluginSync) ); 
			m_SyncPlugins->SetRenderWindows(m_SyncWindow,m_SyncWindow);

			m_SyncCPU = new CN64System(m_SyncPlugins, true);
#else
			g_Notify -> BreakPoint(__FILEW__, __LINE__);
#endif
		}

		if (CpuType == CPU_Recompiler || CpuType == CPU_SyncCores)
		{
			m_Recomp = new CRecompiler(m_Reg,m_Profile,m_EndEmulation);
		}

		bool bSetActive = true;
		if (m_SyncCPU)
		{
			bSetActive = m_SyncCPU->SetActiveSystem();
		}

		if (bSetActive)
		{
			bSetActive = SetActiveSystem();
		}

		if (!bSetActive)
		{
			g_Settings->SaveBool(GameRunning_LoadingInProgress,false);
			g_Notify->DisplayError(MSG_PLUGIN_NOT_INIT);

			g_Notify->RefreshMenu();
			g_Notify->ShowRomBrowser();
		}

		g_Notify->MakeWindowOnTop(g_Settings->LoadBool(UserInterface_AlwaysOnTop));

		ThreadInfo * Info = new ThreadInfo;
		HANDLE  * hThread = new HANDLE;
		*hThread = NULL;

		//create the needed info into a structure to pass as one parameter
		//for creating a thread
		Info->ThreadHandle = hThread;
		
		*hThread  = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartEmulationThread,Info,0, &Info->ThreadID);
	}
	else 
	{
		//mark the emulation as starting and fix up menus
		g_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);

		if (g_Settings->LoadBool(Setting_AutoFullscreen)) 
		{
			WriteTrace(TraceDebug,__FUNCTION__ " 15");
			CIniFile RomIniFile(g_Settings->LoadString(SupportFile_RomDatabase).c_str());
			stdstr Status = g_Settings->LoadString(Rdb_Status);

			char String[100];
			RomIniFile.GetString("Rom Status",stdstr_f("%s.AutoFullScreen", Status.c_str()).c_str(),"true",String,sizeof(String));
			if (_stricmp(String,"true") == 0)
			{
				g_Notify->ChangeFullScreen();
			}
		}
		ExecuteCPU();
	}
}

void  CN64System::StartEmulation   ( bool NewThread )
{
	__try 
	{
		StartEmulation2(NewThread);
	}
	__except( g_MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()))
	{
		char Message[600];
		sprintf(Message,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
		MessageBox(NULL,Message,"Exception",MB_OK);
	}
}

void CN64System::StartEmulationThread (  ThreadInfo * Info ) 
{
	CoInitialize(NULL);
	
	EmulationStarting(*Info->ThreadHandle,Info->ThreadID);
	delete Info->ThreadHandle;
	delete Info;

	CoUninitialize();
}

void CN64System::CloseCpu()
{
	if (m_CPU_Handle == NULL) 
	{
		return; 
	}

	m_EndEmulation = true;
	if (g_Settings->LoadBool(GameRunning_CPU_Paused))
	{
		SetEvent(m_hPauseEvent);
	}
	
	if (GetCurrentThreadId() == m_CPU_ThreadID)
	{
		ExternalEvent(SysEvent_CloseCPU);
		return;
	}
	
	HANDLE hThread = m_CPU_Handle;
	for (int count = 0; count < 200; count ++ ) 
	{
		Sleep(100);
		if (g_Notify->ProcessGuiMessages())
		{
			return;
		}
	
		DWORD ExitCode;
		if (GetExitCodeThread(hThread,&ExitCode))
		{
			if (ExitCode != STILL_ACTIVE) 
			{
				break;
			}
		}
	}

	if (hThread)
	{ 
		DWORD ExitCode;
		GetExitCodeThread(hThread,&ExitCode);
		if (ExitCode == STILL_ACTIVE) 
		{
			TerminateThread(hThread,0); 
		}
	}
	CpuStopped();
}

void CN64System::SelectCheats ( HWND hParent ) 
{
	m_Cheats.SelectCheats(hParent,false);
}

void CN64System::DisplayRomInfo ( HWND hParent )
{
	if (!g_Rom) { return; }
	
	RomInformation Info(g_Rom);
	Info.DisplayInformation(hParent);
}

void CN64System::Pause()
{
	if (m_EndEmulation)
	{
		return;
	}
	ResetEvent(m_hPauseEvent);
	g_Settings->SaveBool(GameRunning_CPU_Paused,true);
	g_Notify->RefreshMenu();
	g_Notify->DisplayMessage(5,MSG_CPU_PAUSED);
	WaitForSingleObject(m_hPauseEvent, INFINITE);
	ResetEvent(m_hPauseEvent);
	g_Settings->SaveBool(GameRunning_CPU_Paused,(DWORD)false);
	g_Notify->RefreshMenu();
	g_Notify->DisplayMessage(5,MSG_CPU_RESUMED);
}

stdstr CN64System::ChooseFileToOpen ( HWND hParent ) 
{
	OPENFILENAME openfilename;
	char FileName[_MAX_PATH],Directory[_MAX_PATH];

	memset(&FileName, 0, sizeof(FileName));
	memset(&openfilename, 0, sizeof(openfilename));

	strcpy(Directory,g_Settings->LoadString(Directory_Game).c_str());

	openfilename.lStructSize  = sizeof( openfilename );
	openfilename.hwndOwner    = (HWND)hParent;
	openfilename.lpstrFilter = "N64 ROMs (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
	openfilename.lpstrFile    = FileName;
	openfilename.lpstrInitialDir    = Directory;
	openfilename.nMaxFile     = MAX_PATH;
	openfilename.Flags        = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileName (&openfilename)) 
	{							
		return stdstr(FileName);
	}
	return stdstr("");
}

bool CN64System::IsDialogMsg( MSG * msg )
{
	if (m_Cheats.IsCheatMessage(msg))
	{
		return true;
	}
	return false;
}

void CN64System::GameReset()
{
	m_SystemTimer.SetTimer(CSystemTimer::SoftResetTimer,0x3000000,false);
	m_Plugins->Gfx()->ShowCFB();
	m_Reg.FAKE_CAUSE_REGISTER |= CAUSE_IP4;
	m_Plugins->Gfx()->SoftReset();
	if (m_SyncCPU)
	{
		m_SyncCPU->GameReset();
	}
}

void CN64System::PluginReset()
{
	if (!m_Plugins->ResetInUiThread(this))
	{
		g_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
		if (g_BaseSystem)
		{
			g_BaseSystem->m_EndEmulation = true;
		}
	}
	if (m_SyncCPU)
	{
		if (!m_SyncCPU->m_Plugins->ResetInUiThread(m_SyncCPU))
		{
			g_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
			if (g_BaseSystem)
			{
				g_BaseSystem->m_EndEmulation = true;
			}
		}
	}
	g_Notify->RefreshMenu();
	if (m_Recomp)
	{
		m_Recomp->Reset();
	}
	m_Plugins->RomOpened();
	if (m_SyncCPU)
	{
		m_SyncCPU->m_Plugins->RomOpened();
	}
}

void CN64System::Reset (bool bInitReg, bool ClearMenory) 
{
	RefreshGameSettings();
	m_Audio.Reset();
	m_MMU_VM.Reset(ClearMenory);
#if defined(WINDOWS_UI)
	Debug_Reset();
#else
	g_Notify -> BreakPoint(__FILEW__, __LINE__);
#endif
	Mempak::Close();

	m_CyclesToSkip = 0;
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;
	m_DMAUsed      = false;
	m_RspBroke     = true;
	m_SyncCount    = 0;

	for (int i = 0, n = (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i < n; i++)
	{
		m_LastSuccessSyncPC[i] = 0;
	}

	if (bInitReg)
	{
		bool PostPif = true;
		
		InitRegisters(PostPif,m_MMU_VM);
		if (PostPif) 
		{
			memcpy((m_MMU_VM.Dmem()+0x40), (g_Rom->GetRomAddress() + 0x040), 0xFBC);
		}
	}
	else 
	{
		m_Reg.Reset();
	}

	m_SystemTimer.Reset();
	m_SystemTimer.SetTimer(CSystemTimer::CompareTimer,m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER,false);

	if (m_Recomp)
	{
		m_Recomp->Reset();
	}
	if (m_Plugins) { m_Plugins->GameReset(); }
	if (m_SyncCPU)
	{
		m_SyncCPU->Reset(bInitReg,ClearMenory);
	}
}

bool CN64System::SetActiveSystem( bool bActive )
{
	bool bInitPlugin = false;
	bool bReset = false;
	bool bRes = true;

	if (bActive && g_System == this)
	{
		return true;
	}

	if (bActive)
	{		
		m_Reg.SetAsCurrentSystem();

		if (g_System)
		{
			g_System->m_TestTimer = R4300iOp::m_TestTimer;
			g_System->m_NextInstruction = R4300iOp::m_NextInstruction;
			g_System->m_JumpToLocation = R4300iOp::m_JumpToLocation;
		}

		g_System    = this;
		if (g_BaseSystem == this)
		{
			g_SyncSystem   = m_SyncCPU;
		}
		g_Recompiler   = m_Recomp;
		g_MMU          = &m_MMU_VM;
		g_TLB          = &m_TLB;
		g_Reg          = &m_Reg;
		g_Audio        = &m_Audio;
		g_SystemTimer  = &m_SystemTimer;
		g_TransVaddr   = &m_MMU_VM;
		g_SystemEvents = this;
		g_NextTimer    = &m_NextTimer;		
		g_Plugins      = m_Plugins;
		g_TLBLoadAddress = &m_TLBLoadAddress;
		g_TLBStoreAddress = &m_TLBStoreAddress;
		R4300iOp::m_TestTimer = m_TestTimer;
		R4300iOp::m_NextInstruction = m_NextInstruction;
		R4300iOp::m_JumpToLocation = m_JumpToLocation;

		if (!m_bInitialized)
		{
			if (!m_MMU_VM.Initialize())
			{
				return false;
			}
			bReset = true;
			m_bInitialized = true;
			bInitPlugin = true;
		}
	}
	else
	{
		if (this == g_BaseSystem)
		{
			g_System          = NULL;
			g_SyncSystem      = NULL;
			g_Recompiler      = NULL;
			g_MMU             = NULL;
			g_TLB             = NULL;
			g_Reg             = NULL;
			g_Audio           = NULL;
			g_SystemTimer     = NULL;
			g_TransVaddr      = NULL;
			g_SystemEvents    = NULL;
			g_NextTimer       = NULL;
			g_Plugins         = m_Plugins;
			g_TLBLoadAddress  = NULL;
			g_TLBStoreAddress = NULL;
		}
	}

	if (bInitPlugin)
	{
		WriteTrace(TraceDebug,__FUNCTION__ ": Reseting Plugins");
		g_Notify->DisplayMessage(5,MSG_PLUGIN_INIT);
		m_Plugins->CreatePlugins();
		bRes = m_Plugins->Initiate(this);
		if (!bRes)
		{
			WriteTrace(TraceError, __FUNCTION__ ": g_Plugins->Initiate Failed");
		}
	}

	if (bReset)
	{
		Reset(true,true);
	}
	return bRes;
}

void CN64System::InitRegisters( bool bPostPif, CMipsMemory & MMU )
{
	m_Reg.Reset();

	//COP0 Registers
	m_Reg.RANDOM_REGISTER	  = 0x1F;
	m_Reg.COUNT_REGISTER	  = 0x5000;
	m_Reg.MI_VERSION_REG	  = 0x02020102;
	m_Reg.SP_STATUS_REG       = 0x00000001;
	m_Reg.CAUSE_REGISTER	  = 0x0000005C;
	m_Reg.CONTEXT_REGISTER    = 0x007FFFF0;
	m_Reg.EPC_REGISTER        = 0xFFFFFFFF;
	m_Reg.BAD_VADDR_REGISTER  = 0xFFFFFFFF;
	m_Reg.ERROREPC_REGISTER   = 0xFFFFFFFF;
	m_Reg.CONFIG_REGISTER     = 0x0006E463;
	m_Reg.STATUS_REGISTER     = 0x34000000;

	//m_Reg.REVISION_REGISTER   = 0x00000511;
	m_Reg.FixFpuLocations();

	if (bPostPif) 
	{
		m_Reg.m_PROGRAM_COUNTER	  = 0xA4000040;	
		
		m_Reg.m_GPR[0].DW=0x0000000000000000;
		m_Reg.m_GPR[6].DW=0xFFFFFFFFA4001F0C;
		m_Reg.m_GPR[7].DW=0xFFFFFFFFA4001F08;
		m_Reg.m_GPR[8].DW=0x00000000000000C0;
		m_Reg.m_GPR[9].DW=0x0000000000000000;
		m_Reg.m_GPR[10].DW=0x0000000000000040;
		m_Reg.m_GPR[11].DW=0xFFFFFFFFA4000040;
		m_Reg.m_GPR[16].DW=0x0000000000000000;
		m_Reg.m_GPR[17].DW=0x0000000000000000;
		m_Reg.m_GPR[18].DW=0x0000000000000000;
		m_Reg.m_GPR[19].DW=0x0000000000000000;
		m_Reg.m_GPR[21].DW=0x0000000000000000; 
		m_Reg.m_GPR[26].DW=0x0000000000000000;
		m_Reg.m_GPR[27].DW=0x0000000000000000;
		m_Reg.m_GPR[28].DW=0x0000000000000000;
		m_Reg.m_GPR[29].DW=0xFFFFFFFFA4001FF0;
		m_Reg.m_GPR[30].DW=0x0000000000000000;
		
		switch (g_Rom->GetCountry())
		{
		case Germany: case french:  case Italian:
		case Europe:  case Spanish: case Australia:
		case X_PAL:   case Y_PAL:
			switch (g_Rom->CicChipID())
			{
			case CIC_UNKNOWN:
			case CIC_NUS_6102:
				m_Reg.m_GPR[5].DW=0xFFFFFFFFC0F1D859;
				m_Reg.m_GPR[14].DW=0x000000002DE108EA;
				m_Reg.m_GPR[24].DW=0x0000000000000000;
				break;
			case CIC_NUS_6103:
				m_Reg.m_GPR[5].DW=0xFFFFFFFFD4646273;
				m_Reg.m_GPR[14].DW=0x000000001AF99984;
				m_Reg.m_GPR[24].DW=0x0000000000000000;
				break;
			case CIC_NUS_6105:
				MMU.SW_VAddr(0xA4001004,0xBDA807FC);
				m_Reg.m_GPR[5].DW=0xFFFFFFFFDECAAAD1;
				m_Reg.m_GPR[14].DW=0x000000000CF85C13;
				m_Reg.m_GPR[24].DW=0x0000000000000002;
				break;
			case CIC_NUS_6106:
				m_Reg.m_GPR[5].DW=0xFFFFFFFFB04DC903;
				m_Reg.m_GPR[14].DW=0x000000001AF99984;
				m_Reg.m_GPR[24].DW=0x0000000000000002;
				break;
			}
			m_Reg.m_GPR[20].DW=0x0000000000000000;
			m_Reg.m_GPR[23].DW=0x0000000000000006;
			m_Reg.m_GPR[31].DW=0xFFFFFFFFA4001554;
			break;
		case NTSC_BETA: case X_NTSC: case USA: case Japan:
		default:
			switch (g_Rom->CicChipID()) 
			{
			case CIC_UNKNOWN:
			case CIC_NUS_6102:
				m_Reg.m_GPR[5].DW=0xFFFFFFFFC95973D5;
				m_Reg.m_GPR[14].DW=0x000000002449A366;
				break;
			case CIC_NUS_6103:
				m_Reg.m_GPR[5].DW=0xFFFFFFFF95315A28;
				m_Reg.m_GPR[14].DW=0x000000005BACA1DF;
				break;
			case CIC_NUS_6105:
				MMU.SW_VAddr(0xA4001004,0x8DA807FC);
				m_Reg.m_GPR[5].DW=0x000000005493FB9A;
				m_Reg.m_GPR[14].DW=0xFFFFFFFFC2C20384;
			case CIC_NUS_6106:
				m_Reg.m_GPR[5].DW=0xFFFFFFFFE067221F;
				m_Reg.m_GPR[14].DW=0x000000005CD2B70F;
				break;
			}
			m_Reg.m_GPR[20].DW=0x0000000000000001;
			m_Reg.m_GPR[23].DW=0x0000000000000000;
			m_Reg.m_GPR[24].DW=0x0000000000000003;
			m_Reg.m_GPR[31].DW=0xFFFFFFFFA4001550;
		}

		switch (g_Rom->CicChipID())
		{
		case CIC_NUS_6101: 
			m_Reg.m_GPR[22].DW=0x000000000000003F; 
			break;
		case CIC_NUS_8303:		//64DD IPL CIC
		case CIC_NUS_5167:		//64DD CONVERSION CIC
			m_Reg.m_GPR[22].DW=0x00000000000000DD;
			break;
		case CIC_UNKNOWN:
		case CIC_NUS_6102: 
			m_Reg.m_GPR[1].DW=0x0000000000000001;
			m_Reg.m_GPR[2].DW=0x000000000EBDA536;
			m_Reg.m_GPR[3].DW=0x000000000EBDA536;
			m_Reg.m_GPR[4].DW=0x000000000000A536;
			m_Reg.m_GPR[12].DW=0xFFFFFFFFED10D0B3;
			m_Reg.m_GPR[13].DW=0x000000001402A4CC;
			m_Reg.m_GPR[15].DW=0x000000003103E121;
			m_Reg.m_GPR[22].DW=0x000000000000003F; 
			m_Reg.m_GPR[25].DW=0xFFFFFFFF9DEBB54F;
			break;
		case CIC_NUS_6103: 
			m_Reg.m_GPR[1].DW=0x0000000000000001;
			m_Reg.m_GPR[2].DW=0x0000000049A5EE96;
			m_Reg.m_GPR[3].DW=0x0000000049A5EE96;
			m_Reg.m_GPR[4].DW=0x000000000000EE96;
			m_Reg.m_GPR[12].DW=0xFFFFFFFFCE9DFBF7;
			m_Reg.m_GPR[13].DW=0xFFFFFFFFCE9DFBF7;
			m_Reg.m_GPR[15].DW=0x0000000018B63D28;
			m_Reg.m_GPR[22].DW=0x0000000000000078; 
			m_Reg.m_GPR[25].DW=0xFFFFFFFF825B21C9;
			break;
		case CIC_NUS_6105: 
			MMU.SW_VAddr(0xA4001000,0x3C0DBFC0);
			MMU.SW_VAddr(0xA4001008,0x25AD07C0);
			MMU.SW_VAddr(0xA400100C,0x31080080);
			MMU.SW_VAddr(0xA4001010,0x5500FFFC);
			MMU.SW_VAddr(0xA4001014,0x3C0DBFC0);
			MMU.SW_VAddr(0xA4001018,0x8DA80024);
			MMU.SW_VAddr(0xA400101C,0x3C0BB000);
			m_Reg.m_GPR[1].DW=0x0000000000000000;
			m_Reg.m_GPR[2].DW=0xFFFFFFFFF58B0FBF;
			m_Reg.m_GPR[3].DW=0xFFFFFFFFF58B0FBF;
			m_Reg.m_GPR[4].DW=0x0000000000000FBF;
			m_Reg.m_GPR[12].DW=0xFFFFFFFF9651F81E;
			m_Reg.m_GPR[13].DW=0x000000002D42AAC5;
			m_Reg.m_GPR[15].DW=0x0000000056584D60;
			m_Reg.m_GPR[22].DW=0x0000000000000091; 
			m_Reg.m_GPR[25].DW=0xFFFFFFFFCDCE565F;
			break;
		case CIC_NUS_6106: 
			m_Reg.m_GPR[1].DW=0x0000000000000000;
			m_Reg.m_GPR[2].DW=0xFFFFFFFFA95930A4;
			m_Reg.m_GPR[3].DW=0xFFFFFFFFA95930A4;
			m_Reg.m_GPR[4].DW=0x00000000000030A4;
			m_Reg.m_GPR[12].DW=0xFFFFFFFFBCB59510;
			m_Reg.m_GPR[13].DW=0xFFFFFFFFBCB59510;
			m_Reg.m_GPR[15].DW=0x000000007A3C07F4;
			m_Reg.m_GPR[22].DW=0x0000000000000085; 
			m_Reg.m_GPR[25].DW=0x00000000465E3F72;
			break;
		}
	}
	else
	{
		m_Reg.m_PROGRAM_COUNTER = 0xBFC00000;			
/*		PIF_Ram[36] = 0x00; PIF_Ram[39] = 0x3F; //common pif ram start values

		switch (g_Rom->CicChipID()) {
		case CIC_NUS_6101: PIF_Ram[37] = 0x06; PIF_Ram[38] = 0x3F; break;
		case CIC_UNKNOWN:
		case CIC_NUS_6102: PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x3F; break;
		case CIC_NUS_6103:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x78; break;
		case CIC_NUS_6105:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x91; break;
		case CIC_NUS_6106:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x85; break;
		}*/
	}
}

void CN64System::ExecuteCPU()
{
	//reset code
	g_Settings->SaveBool(GameRunning_CPU_Running,true);
	g_Settings->SaveBool(GameRunning_CPU_Paused,false);
	g_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);
	
	m_EndEmulation = false;
	g_Notify->RefreshMenu();

	m_Plugins->RomOpened();
	if (m_SyncCPU)
	{
		m_SyncCPU->m_Plugins->RomOpened();
	}

	switch ((CPU_TYPE)g_Settings->LoadDword(Game_CpuType))
	{
// Currently the compiler is 32-bit only.  We might have to ignore that RDB setting for now.
#ifndef _WIN64
	case CPU_Recompiler: ExecuteRecompiler(); break;
	case CPU_SyncCores:  ExecuteSyncCPU();    break;
#endif
	default:             ExecuteInterpret();  break;
	}
	g_Settings->SaveBool(GameRunning_CPU_Running,(DWORD)false);
	g_Notify->WindowMode();
	m_Plugins->RomClosed();
	if (m_SyncCPU)
	{
		m_SyncCPU->m_Plugins->RomClosed();
	}
}

void CN64System::ExecuteInterpret()
{
	SetActiveSystem();
	CInterpreterCPU::ExecuteCPU();
}

void CN64System::ExecuteRecompiler()
{	
	m_Recomp->Run();
}

void CN64System::ExecuteSyncCPU()
{
	g_Notify->BringToTop();
	m_Recomp->Run();
}

void CN64System::CpuStopped()
{
	g_Settings->SaveBool(GameRunning_CPU_Running,(DWORD)false);
	g_Notify->WindowMode();
	if (!m_InReset)
	{
		if (m_hPauseEvent)
		{
			CloseHandle(m_hPauseEvent);
			m_hPauseEvent = NULL;
		}

		g_Notify->RefreshMenu();
		g_Notify->MakeWindowOnTop(false);
		g_Notify->DisplayMessage(5,MSG_EMULATION_ENDED);
		if (g_Settings->LoadDword(RomBrowser_Enabled))
		{
			g_Notify->ShowRomBrowser(); 
		}	
	}
	if (m_SyncCPU)
	{
		m_SyncCPU->CpuStopped();
	}
}

void CN64System::UpdateSyncCPU (CN64System * const SecondCPU, DWORD const Cycles)
{
	int CyclesToExecute = Cycles - m_CyclesToSkip;
	
	//Update the number of cycles to skip
	m_CyclesToSkip -= Cycles;
	if (m_CyclesToSkip < 0) { m_CyclesToSkip = 0; }

	//Run the other CPU For the same amount of cycles
	if (CyclesToExecute < 0) { return; }
	
	SecondCPU->SetActiveSystem(true);
	
	CInterpreterCPU::ExecuteOps(Cycles);

	SetActiveSystem(true);
}

void CN64System::SyncCPUPC (CN64System * const SecondCPU) 
{
	bool ErrorFound = false;

	g_SystemTimer->UpdateTimers();
	if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER) 
	{
		ErrorFound = true;
	}

	if (m_TLB != SecondCPU->m_TLB) { ErrorFound = true; }
	if (m_SystemTimer != SecondCPU->m_SystemTimer) { ErrorFound = true; }
	if (m_NextTimer != SecondCPU->m_NextTimer) { ErrorFound = true; }

	if (ErrorFound) { DumpSyncErrors(SecondCPU); }

	for (int i = (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--) 
	{
		m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
	}
	m_LastSuccessSyncPC[0] = m_Reg.m_PROGRAM_COUNTER;
}

void CN64System::SyncCPU (CN64System * const SecondCPU) 
{
	bool ErrorFound = false;

	m_SyncCount += 1;
	//WriteTraceF(TraceError,"SyncCPU PC = %08X",m_Reg.m_PROGRAM_COUNTER);
	g_SystemTimer->UpdateTimers();
	
#ifdef TEST_SP_TRACKING
	if (m_CurrentSP != GPR[29].UW[0]) {
		ErrorFound = true;
	}
#endif
	if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER) 
	{
		ErrorFound = true;
	}
	if (b32BitCore())
	{
		for (int count = 0; count < 32; count ++)
		{
			if (m_Reg.m_GPR[count].W[0] != SecondCPU->m_Reg.m_GPR[count].W[0])
			{
				ErrorFound = true;
			}
			if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW)
			{
				ErrorFound = true;
			}
			if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count])
			{
				ErrorFound = true;
			}
		}
	}
	else
	{
		for (int count = 0; count < 32; count ++) 
		{
			if (m_Reg.m_GPR[count].DW != SecondCPU->m_Reg.m_GPR[count].DW)
			{
				ErrorFound = true;
			}
			if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW) 
			{
				ErrorFound = true;
			}
			if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count])
			{
				ErrorFound = true;
			}
		}
	}
	
	if (m_TLB != SecondCPU->m_TLB) { ErrorFound = true; }
	if (m_Reg.m_FPCR[0] != SecondCPU->m_Reg.m_FPCR[0]) { ErrorFound = true; }
	if (m_Reg.m_FPCR[31] != SecondCPU->m_Reg.m_FPCR[31]) { ErrorFound = true; }
	if (m_Reg.m_HI.DW != SecondCPU->m_Reg.m_HI.DW) { ErrorFound = true; }
	if (m_Reg.m_LO.DW != SecondCPU->m_Reg.m_LO.DW) { ErrorFound = true; }
	/*if (m_SyncCount > 4788000)
	{
		if (memcmp(m_MMU_VM.Rdram(),SecondCPU->m_MMU_VM.Rdram(),RdramSize()) != 0) 
		{
			ErrorFound = true; 
		}
	}
	if (memcmp(m_MMU_VM.Imem(),SecondCPU->m_MMU_VM.Imem(),0x1000) != 0) 
	{
		ErrorFound = true; 
	}
	if (memcmp(m_MMU_VM.Dmem(),SecondCPU->m_MMU_VM.Dmem(),0x1000) != 0) 
	{
		ErrorFound = true; 
	}*/

	/*for (int z = 0; z < 0x100; z++)
	{	
		if (m_MMU_VM.Rdram()[0x00206970 + z] !=  SecondCPU->m_MMU_VM.Rdram()[0x00206970 + z]) 
		{
			ErrorFound = true;
			break;
		}
	}*/
	
	if (bFastSP() && m_Recomp) 
	{
		if (m_Recomp->MemoryStackPos() != (DWORD)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)))
		{
			ErrorFound = true;
		}
	}

	if (m_SystemTimer != SecondCPU->m_SystemTimer) { ErrorFound = true; }
	if (m_NextTimer != SecondCPU->m_NextTimer) { ErrorFound = true; }
	if (m_Reg.m_RoundingModel != SecondCPU->m_Reg.m_RoundingModel) { ErrorFound = true; }
	
	for (int i = 0, n = sizeof(m_Reg.m_Mips_Interface) / sizeof(m_Reg.m_Mips_Interface[0]); i < n; i ++) 
	{
		if (m_Reg.m_Mips_Interface[i] != SecondCPU->m_Reg.m_Mips_Interface[i])
		{
			ErrorFound = true;
		}
	}

	for (int i = 0, n = sizeof(m_Reg.m_SigProcessor_Interface) / sizeof(m_Reg.m_SigProcessor_Interface[0]); i < n; i ++) 
	{
		if (m_Reg.m_SigProcessor_Interface[i] != SecondCPU->m_Reg.m_SigProcessor_Interface[i])
		{
			ErrorFound = true;
		}
	}

	for (int i = 0, n = sizeof(m_Reg.m_Display_ControlReg) / sizeof(m_Reg.m_Display_ControlReg[0]); i < n; i ++) 
	{
		if (m_Reg.m_Display_ControlReg[i] != SecondCPU->m_Reg.m_Display_ControlReg[i])
		{
			ErrorFound = true;
		}
	}
	
	if (ErrorFound) { DumpSyncErrors(SecondCPU); }

	for (int i = (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--) 
	{
		m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
	}
	m_LastSuccessSyncPC[0] = m_Reg.m_PROGRAM_COUNTER;
//	if (PROGRAM_COUNTER == 0x8009BBD8) {
//		g_Notify->BreakPoint(__FILEW__,__LINE__);
//	}
}

void CN64System::SyncSystem()
{
	SyncCPU(g_SyncSystem);
}

void CN64System::SyncSystemPC()
{
	SyncCPUPC(g_SyncSystem);
}

void CN64System::DumpSyncErrors (CN64System * SecondCPU) 
{
	int count;
	
	{
		CPath ErrorFile (CPath::MODULE_DIRECTORY);
		ErrorFile.AppendDirectory("Logs");
		ErrorFile.SetNameExtension("Sync Errors.txt");

		CLog Error;
		Error.Open(ErrorFile);
		Error.Log("Errors:\r\n");
		Error.Log("Register,        Recompiler,         Interpter\r\n");
	#ifdef TEST_SP_TRACKING
		if (m_CurrentSP != GPR[29].UW[0]) {
			Error.Log("m_CurrentSP,%X,%X\r\n",m_CurrentSP,GPR[29].UW[0]);
		}
	#endif
		if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER) {
			Error.LogF("PROGRAM_COUNTER 0x%X,         0x%X\r\n",m_Reg.m_PROGRAM_COUNTER,SecondCPU->m_Reg.m_PROGRAM_COUNTER);
		}
		if (b32BitCore())
		{
			for (count = 0; count < 32; count ++) 
			{
				if (m_Reg.m_GPR[count].UW[0] != SecondCPU->m_Reg.m_GPR[count].UW[0])
				{
					Error.LogF("GPR[%s] 0x%08X%08X, 0x%08X%08X\r\n",CRegName::GPR[count],
						m_Reg.m_GPR[count].W[1],m_Reg.m_GPR[count].W[0],
						SecondCPU->m_Reg.m_GPR[count].W[1],SecondCPU->m_Reg.m_GPR[count].W[0]);
				}
			}
		}
		else 
		{
			for (count = 0; count < 32; count ++) 
			{
				if (m_Reg.m_GPR[count].DW != SecondCPU->m_Reg.m_GPR[count].DW)
				{
					Error.LogF("GPR[%s] 0x%08X%08X, 0x%08X%08X\r\n",CRegName::GPR[count],
						m_Reg.m_GPR[count].W[1],m_Reg.m_GPR[count].W[0],
						SecondCPU->m_Reg.m_GPR[count].W[1],SecondCPU->m_Reg.m_GPR[count].W[0]);
				}
			}
		}
		for (count = 0; count < 32; count ++) 
		{
			if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW)
			{
				Error.LogF("FPR[%s] 0x%08X%08X, 0x%08X%08X\r\n",CRegName::FPR[count],
					m_Reg.m_FPR[count].W[1],m_Reg.m_FPR[count].W[0],
					SecondCPU->m_Reg.m_FPR[count].W[1],SecondCPU->m_Reg.m_FPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) 
		{
			if (m_Reg.m_FPCR[count] != SecondCPU->m_Reg.m_FPCR[count])
			{
				Error.LogF("FPCR[%s] 0x%08X, 0x%08X\r\n",CRegName::FPR_Ctrl[count],
					m_Reg.m_FPCR[count], SecondCPU->m_Reg.m_FPCR[count]);
			}
		}	
		for (count = 0; count < 32; count ++) 
		{
			if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count])
			{
				Error.LogF("CP0[%s] 0x%08X, 0x%08X\r\n",CRegName::Cop0[count],
					m_Reg.m_CP0[count], SecondCPU->m_Reg.m_CP0[count]);
			}
		}	
		if (m_Reg.m_HI.DW != SecondCPU->m_Reg.m_HI.DW) 
		{
			Error.LogF("HI Reg 0x%08X%08X, 0x%08X%08X\r\n",m_Reg.m_HI.UW[1],m_Reg.m_HI.UW[0],SecondCPU->m_Reg.m_HI.UW[1],SecondCPU->m_Reg.m_HI.UW[0]);
		}
		if (m_Reg.m_LO.DW != SecondCPU->m_Reg.m_LO.DW)
		{
			Error.LogF("LO Reg 0x%08X%08X, 0x%08X%08X\r\n",m_Reg.m_LO.UW[1],m_Reg.m_LO.UW[0], SecondCPU->m_Reg.m_LO.UW[1],SecondCPU->m_Reg.m_LO.UW[0]);
		}
		for (int i = 0, n = sizeof(m_Reg.m_Mips_Interface) / sizeof(m_Reg.m_Mips_Interface[0]); i < n; i ++) 
		{
			if (m_Reg.m_Mips_Interface[i] != SecondCPU->m_Reg.m_Mips_Interface[i])
			{
				Error.LogF("Mips_Interface[%d] 0x%08X, 0x%08X\r\n",i, m_Reg.m_Mips_Interface[i], SecondCPU->m_Reg.m_Mips_Interface[i]);
			}
		}

		for (int i = 0, n = sizeof(m_Reg.m_SigProcessor_Interface) / sizeof(m_Reg.m_SigProcessor_Interface[0]); i < n; i ++) 
		{
			if (m_Reg.m_SigProcessor_Interface[i] != SecondCPU->m_Reg.m_SigProcessor_Interface[i])
			{
				Error.LogF("SigProcessor_Interface[%d] 0x%08X, 0x%08X\r\n",i, m_Reg.m_SigProcessor_Interface[i], SecondCPU->m_Reg.m_SigProcessor_Interface[i]);
			}
		}
		for (int i = 0, n = sizeof(m_Reg.m_Display_ControlReg) / sizeof(m_Reg.m_Display_ControlReg[0]); i < n; i ++) 
		{
			if (m_Reg.m_Display_ControlReg[i] != SecondCPU->m_Reg.m_Display_ControlReg[i])
			{
				Error.LogF("Display_ControlReg[%d] 0x%08X, 0x%08X\r\n",i, m_Reg.m_Display_ControlReg[i], SecondCPU->m_Reg.m_Display_ControlReg[i]);
			}
		}

		if (m_NextTimer     != SecondCPU->m_NextTimer) 
		{ 
			Error.LogF("Current Time: %X %X\r\n",(DWORD)m_NextTimer,(DWORD)SecondCPU->m_NextTimer);
		}
		m_TLB.RecordDifference(Error,SecondCPU->m_TLB);
		m_SystemTimer.RecordDifference(Error,SecondCPU->m_SystemTimer);
		if (m_Reg.m_RoundingModel != SecondCPU->m_Reg.m_RoundingModel) 
		{ 
			Error.LogF("RoundingModel: %X %X\r\n",m_Reg.m_RoundingModel,SecondCPU->m_Reg.m_RoundingModel);
		}
		if (bFastSP() && m_Recomp) 
		{
			if (m_Recomp->MemoryStackPos() != (DWORD)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)))
			{
				Error.LogF("MemoryStack = %X  should be: %X\r\n",m_Recomp->MemoryStackPos(), (DWORD)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)));
			}
		}

		DWORD * Rdram = (DWORD *)m_MMU_VM.Rdram(), * Rdram2 = (DWORD *)SecondCPU->m_MMU_VM.Rdram();
		for (int z = 0, n = (RdramSize() >> 2); z < n; z ++)
		{	
			if (Rdram[z] != Rdram2[z]) 
			{
				Error.LogF("Rdram[%X]: %X %X\r\n",z << 2,Rdram[z],Rdram2[z]);
			}
		}

		DWORD * Imem = (DWORD *)m_MMU_VM.Imem(), * Imem2 = (DWORD *)SecondCPU->m_MMU_VM.Imem();
		for (int z = 0; z < (0x1000 >> 2); z ++)
		{	
			if (Imem[z] != Imem2[z]) 
			{
				Error.LogF("Imem[%X]: %X %X\r\n",z << 2,Imem[z],Imem2[z]);
			}
		}
		DWORD * Dmem = (DWORD *)m_MMU_VM.Dmem(), * Dmem2 = (DWORD *)SecondCPU->m_MMU_VM.Dmem();
		for (int z = 0; z < (0x1000 >> 2); z ++)
		{	
			if (Dmem[z] != Dmem2[z]) 
			{
				Error.LogF("Dmem[%X]: %X %X\r\n",z << 2,Dmem[z],Dmem2[z]);
			}
		}
		Error.Log("\r\n");
		Error.Log("Information:\r\n");
		Error.Log("\r\n");
		Error.LogF("PROGRAM_COUNTER,0x%X\r\n",m_Reg.m_PROGRAM_COUNTER);
		Error.LogF("Current Timer,0x%X\r\n",m_NextTimer);
		Error.LogF("Timer Type,0x%X\r\n",m_SystemTimer.CurrentType());
		Error.Log("\r\n");
		for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) 
		{
			Error.LogF("LastSuccessSyncPC[%d],0x%X\r\n",i,m_LastSuccessSyncPC[i]);
		}
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) 
		{
			Error.LogF("GPR[%s],         0x%08X%08X, 0x%08X%08X\r\n",CRegName::GPR[count],
				m_Reg.m_GPR[count].W[1],m_Reg.m_GPR[count].W[0],
				SecondCPU->m_Reg.m_GPR[count].W[1],SecondCPU->m_Reg.m_GPR[count].W[0]);
		}	
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) 
		{
			Error.LogF("FPR[%s],%*s0x%08X%08X, 0x%08X%08X\r\n",CRegName::FPR[count],
				count < 10 ? 9 : 8," ",m_Reg.m_FPR[count].W[1],m_Reg.m_FPR[count].W[0],
				SecondCPU->m_Reg.m_FPR[count].W[1],SecondCPU->m_Reg.m_FPR[count].W[0]);
		}	
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) 
		{
			Error.LogF("FPR_S[%s],%*s%f, %f\r\n",CRegName::FPR[count],
				count < 10 ? 7 : 6," ",*(m_Reg.m_FPR_S[count]),*(SecondCPU->m_Reg.m_FPR_S[count]));
		}	
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) 
		{
			Error.LogF("FPR_D[%s],%*s%f, %f\r\n",CRegName::FPR[count],
				count < 10 ? 7 : 6," ",*(m_Reg.m_FPR_D[count]),*(SecondCPU->m_Reg.m_FPR_D[count]));
		}	
		Error.Log("\r\n");
		Error.LogF("Rounding Model,   0x%08X, 0x%08X\r\n",m_Reg.m_RoundingModel,SecondCPU->m_Reg.m_RoundingModel);
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++)
		{
			Error.LogF("CP0[%s],%*s0x%08X, 0x%08X\r\n",CRegName::Cop0[count],
				12 - strlen(CRegName::Cop0[count]),"",
				m_Reg.m_CP0[count],SecondCPU->m_Reg.m_CP0[count]);
		}	
		Error.Log("\r\n");
		Error.LogF("HI                0x%08X%08X, 0x%08X%08X\r\n",m_Reg.m_HI.UW[1],m_Reg.m_HI.UW[0],
			SecondCPU->m_Reg.m_HI.UW[1],SecondCPU->m_Reg.m_HI.UW[0]);
		Error.LogF("LO                0x%08X%08X, 0x%08X%08X\r\n",m_Reg.m_LO.UW[1],m_Reg.m_LO.UW[0],
			SecondCPU->m_Reg.m_LO.UW[1],SecondCPU->m_Reg.m_LO.UW[0]);
		bool bHasTlb = false;
		for (count = 0; count < 32; count ++) 
		{
			if (!m_TLB.TlbEntry(count).EntryDefined) { continue; }
			if (!bHasTlb)
			{
				Error.Log("\r\n");
				Error.Log("         Hi Recomp, PageMask, Hi Interp, PageMask\r\n");
				bHasTlb = true;
			}
			Error.LogF("TLB[%2d], %08X,  %08X, %08X,  %08X\r\n", count,
				m_TLB.TlbEntry(count).EntryHi.Value,m_TLB.TlbEntry(count).PageMask.Value,
				SecondCPU->m_TLB.TlbEntry(count).EntryHi.Value,SecondCPU->m_TLB.TlbEntry(count).PageMask.Value
			);	
		}
		Error.Log("\r\n");
		Error.Log("Code at PC:\r\n");
		for (count = -10; count < 10; count++)
		{
			DWORD OpcodeValue, Addr = m_Reg.m_PROGRAM_COUNTER + (count << 2);
			if (g_MMU->LW_VAddr(Addr,OpcodeValue))
			{
				Error.LogF("%X: %s\r\n",Addr,R4300iOpcodeName(OpcodeValue,Addr));
			}

		}
		Error.Log("\r\n");
		Error.Log("Code at Last Sync PC:\r\n");
		for (count = 0; count < 50; count++)
		{
			DWORD OpcodeValue, Addr = m_LastSuccessSyncPC[0] + (count << 2);
			if (g_MMU->LW_VAddr(Addr,OpcodeValue))
			{
				Error.LogF("%X: %s\r\n",Addr,R4300iOpcodeName(OpcodeValue,Addr));
			}

		}
	}

	g_Notify->DisplayError(L"Sync Error");
	g_Notify->BreakPoint(__FILEW__,__LINE__);
}

bool CN64System::SaveState()
{
	WriteTrace(TraceDebug,__FUNCTION__ ": Start");

//	if (!m_SystemTimer.SaveAllowed()) { return false; }
	if ((m_Reg.STATUS_REGISTER & STATUS_EXL) != 0) { return false; }
	
	//Get the file Name
	stdstr FileName, ExtraInfoFileName, CurrentSaveName = g_Settings->LoadString(GameRunning_InstantSaveFile);
	if (CurrentSaveName.empty())
	{
		int Slot = g_Settings->LoadDword(Game_CurrentSaveState);
		if (Slot != 0) 
		{ 
			CurrentSaveName.Format("%s.pj%d",g_Settings->LoadString(Game_GoodName).c_str(), Slot);
		}
		else 
		{
			CurrentSaveName.Format("%s.pj",g_Settings->LoadString(Game_GoodName).c_str());
		}
		FileName.Format("%s%s",g_Settings->LoadString(Directory_InstantSave).c_str(),CurrentSaveName.c_str());
		stdstr_f ZipFileName("%s.zip",FileName.c_str());
		//Make sure the target dir exists	
		CreateDirectory(g_Settings->LoadString(Directory_InstantSave).c_str(),NULL);
		//delete any old save
		DeleteFile(FileName.c_str());
		DeleteFile(ZipFileName.c_str());
		ExtraInfoFileName.Format("%s.dat",CurrentSaveName.c_str());
	
		//If ziping save add .zip on the end
		if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
		{
			FileName = ZipFileName;
		}
		g_Settings->SaveDword(Game_LastSaveSlot,g_Settings->LoadDword(Game_CurrentSaveState));
	} 
	else
	{
		FileName.Format("%s%s",CurrentSaveName.c_str(), g_Settings->LoadDword(Setting_AutoZipInstantSave) ? ".pj.zip" : ".pj");
		ExtraInfoFileName.Format("%s.dat",FileName.c_str());
	}
	if (FileName.empty()) { return true; }

	//Open the file
	if (g_Settings->LoadDword(Game_FuncLookupMode) == FuncFind_ChangeMemory) 
	{
		if (m_Recomp)
		{
			m_Recomp->ResetRecompCode(true); 
		}
	}

	DWORD dwWritten, SaveID_0 = 0x23D8A6C8, SaveID_1 = 0x56D2CD23;
	DWORD RdramSize   = g_Settings->LoadDword(Game_RDRamSize);
	DWORD MiInterReg  = g_Reg->MI_INTR_REG;
	DWORD NextViTimer = m_SystemTimer.GetTimer(CSystemTimer::ViTimer);
	if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
	{
		zipFile			file;

		file = zipOpen(FileName.c_str(),0);
		zipOpenNewFileInZip(file,CurrentSaveName.c_str(),NULL,NULL,0,NULL,0,NULL,Z_DEFLATED,Z_DEFAULT_COMPRESSION);
		zipWriteInFileInZip(file,&SaveID_0,sizeof(SaveID_0));
		zipWriteInFileInZip(file,&RdramSize,sizeof(DWORD));
		zipWriteInFileInZip(file,g_Rom->GetRomAddress(),0x40);	
		zipWriteInFileInZip(file,&NextViTimer,sizeof(DWORD));
		zipWriteInFileInZip(file,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER));
		zipWriteInFileInZip(file,m_Reg.m_GPR,sizeof(__int64)*32);
		zipWriteInFileInZip(file,m_Reg.m_FPR,sizeof(__int64)*32);
		zipWriteInFileInZip(file,m_Reg.m_CP0,sizeof(DWORD)*32);
		zipWriteInFileInZip(file,m_Reg.m_FPCR,sizeof(DWORD)*32);
		zipWriteInFileInZip(file,&m_Reg.m_HI,sizeof(__int64));
		zipWriteInFileInZip(file,&m_Reg.m_LO,sizeof(__int64));
		zipWriteInFileInZip(file,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,m_Reg.m_Mips_Interface,sizeof(DWORD)*4);
		zipWriteInFileInZip(file,m_Reg.m_Video_Interface,sizeof(DWORD)*14);
		zipWriteInFileInZip(file,m_Reg.m_Audio_Interface,sizeof(DWORD)*6);
		zipWriteInFileInZip(file,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13);
		zipWriteInFileInZip(file,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8);
		zipWriteInFileInZip(file,m_Reg.m_SerialInterface,sizeof(DWORD)*4);
		zipWriteInFileInZip(file,(void *const)&m_TLB.TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32);
		zipWriteInFileInZip(file,m_MMU_VM.PifRam(),0x40);
		zipWriteInFileInZip(file,m_MMU_VM.Rdram(),RdramSize);
		zipWriteInFileInZip(file,m_MMU_VM.Dmem(),0x1000);
		zipWriteInFileInZip(file,m_MMU_VM.Imem(),0x1000);
		zipCloseFileInZip(file);
		
		zipOpenNewFileInZip(file,ExtraInfoFileName.c_str(),NULL,NULL,0,NULL,0,NULL,Z_DEFLATED,Z_DEFAULT_COMPRESSION);
		zipWriteInFileInZip(file,&SaveID_1,sizeof(SaveID_1));
		m_SystemTimer.SaveData(file);
		zipCloseFileInZip(file);

		zipClose(file,"");
	} 
	else
	{
		HANDLE hSaveFile = CreateFile(FileName.c_str(),GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
			NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hSaveFile == INVALID_HANDLE_VALUE) 
		{
			g_Notify->DisplayError(GS(MSG_FAIL_OPEN_SAVE));
			m_Reg.MI_INTR_REG = MiInterReg;
			return true;
		}

		//Write info to file
		SetFilePointer(hSaveFile,0,NULL,FILE_BEGIN);	
		WriteFile( hSaveFile,&SaveID_0,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,&RdramSize,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,g_Rom->GetRomAddress(),0x40,&dwWritten,NULL);	
		WriteFile( hSaveFile,&NextViTimer,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER),&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_GPR,sizeof(__int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_FPR,sizeof(__int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_CP0,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_FPCR,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.m_HI,sizeof(__int64),&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.m_LO,sizeof(__int64),&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Mips_Interface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Video_Interface,sizeof(DWORD)*14,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Audio_Interface,sizeof(DWORD)*6,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_SerialInterface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,&g_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,g_MMU->PifRam(),0x40,&dwWritten,NULL);
		WriteFile( hSaveFile,g_MMU->Rdram(),RdramSize,&dwWritten,NULL);
		WriteFile( hSaveFile,g_MMU->Dmem(),0x1000,&dwWritten,NULL);
		WriteFile( hSaveFile,g_MMU->Imem(),0x1000,&dwWritten,NULL);

		CloseHandle(hSaveFile);
	}
	m_Reg.MI_INTR_REG = MiInterReg;
	g_Settings->SaveString(GameRunning_InstantSaveFile,"");
	std::wstring SaveMessage = g_Lang->GetString(MSG_SAVED_STATE);

	CPath SavedFileName(FileName);
	
    g_Notify->DisplayMessage(5,L"%s %s",SaveMessage.c_str(),SavedFileName.GetNameExtension().ToUTF16().c_str());
	g_Notify->RefreshMenu();
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");
	return true;
}

bool CN64System::LoadState()
{
	stdstr InstantFileName = g_Settings->LoadString(GameRunning_InstantSaveFile);
	if (!InstantFileName.empty())
	{
		bool Result = LoadState(InstantFileName.c_str());
		g_Settings->SaveString(GameRunning_InstantSaveFile,"");
		return Result;
	}

	CPath FileName;
	FileName.SetDriveDirectory(g_Settings->LoadString(Directory_InstantSave).c_str());
	if (g_Settings->LoadDword(Game_CurrentSaveState) != 0) 
	{
		FileName.SetNameExtension(stdstr_f("%s.pj%d",g_Settings->LoadString(Game_GoodName).c_str(),g_Settings->LoadDword(Game_CurrentSaveState)).c_str());
	} 
	else 
	{
		FileName.SetNameExtension(stdstr_f("%s.pj",g_Settings->LoadString(Game_GoodName).c_str()).c_str());
	}

	CPath ZipFileName;
	ZipFileName = (stdstr&)FileName + ".zip";

	if ((g_Settings->LoadDword(Setting_AutoZipInstantSave) && ZipFileName.Exists()) || FileName.Exists())
	{
		if (LoadState(FileName))
		{
			return true;
		}
	}

	//Use old file Name
	if (g_Settings->LoadDword(Game_CurrentSaveState) != 0) 
	{ 
		FileName.SetNameExtension(stdstr_f("%s.pj%d",g_Settings->LoadString(Game_GameName).c_str(),g_Settings->LoadDword(Game_CurrentSaveState)).c_str());
	}
	else
	{
		FileName.SetNameExtension(stdstr_f("%s.pj",g_Settings->LoadString(Game_GameName).c_str()).c_str());
	}
	return LoadState(FileName);
}

bool CN64System::LoadState(LPCSTR FileName) 
{
	DWORD dwRead, Value,SaveRDRAMSize, NextVITimer = 0, old_status, old_width, old_dacrate;
	bool LoadedZipFile = false, AudioResetOnLoad;
	old_status = g_Reg->VI_STATUS_REG;
	old_width = g_Reg->VI_WIDTH_REG;
	old_dacrate = g_Reg->AI_DACRATE_REG;
	
	WriteTraceF((TraceType)(TraceDebug | TraceRecompiler),__FUNCTION__ "(%s): Start",FileName);

	char drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
	_splitpath(FileName, drive, dir, fname, ext);

	stdstr FileNameStr(FileName);
	if (g_Settings->LoadDword(Setting_AutoZipInstantSave) || _stricmp(ext,".zip") == 0) 
	{
		//If ziping save add .zip on the end
		if (_stricmp(ext,".zip") != 0)
		{
			FileNameStr += ".zip";
		}
		unzFile file = unzOpen(FileNameStr.c_str());
		int port = -1;
		if (file != NULL) 
		{
			port = unzGoToFirstFile(file);
		}
		DWORD Value;
		while (port == UNZ_OK) 
		{
			unz_file_info info;
			char zname[132];

			unzGetCurrentFileInfo(file, &info, zname, 128, NULL,0, NULL,0);
			if (unzLocateFile(file, zname, 1) != UNZ_OK ) 
			{
				unzClose(file);
				port = -1;
				continue;
			}
			if( unzOpenCurrentFile(file) != UNZ_OK ) 
			{
				unzClose(file);
				port = -1;
				continue;
			}
			unzReadCurrentFile(file,&Value,4);
			if (Value != 0x23D8A6C8 && Value != 0x56D2CD23) 
			{
				unzCloseCurrentFile(file);
				port = unzGoToNextFile(file);
				continue;
			}
			if (!LoadedZipFile && Value == 0x23D8A6C8 && port == UNZ_OK) 
			{
				unzReadCurrentFile(file,&SaveRDRAMSize,sizeof(SaveRDRAMSize));
				//Check header

				BYTE LoadHeader[64];
				unzReadCurrentFile(file,LoadHeader,0x40);
				if (memcmp(LoadHeader,g_Rom->GetRomAddress(),0x40) != 0)
				{
					//if (inFullScreen) { return false; }
					int result = MessageBoxW(NULL,GS(MSG_SAVE_STATE_HEADER),GS(MSG_MSGBOX_TITLE),
						MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);

					if (result == IDNO)
						return false;
				}
				Reset(false,true);

				g_MMU->UnProtectMemory(0x80000000,0x80000000 + g_Settings->LoadDword(Game_RDRamSize) - 4);
				g_MMU->UnProtectMemory(0xA4000000,0xA4001FFC);
				g_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
				unzReadCurrentFile(file,&NextVITimer,sizeof(NextVITimer));
				unzReadCurrentFile(file,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER));
				unzReadCurrentFile(file,m_Reg.m_GPR,sizeof(__int64)*32);
				unzReadCurrentFile(file,m_Reg.m_FPR,sizeof(__int64)*32);
				unzReadCurrentFile(file,m_Reg.m_CP0,sizeof(DWORD)*32);
				unzReadCurrentFile(file,m_Reg.m_FPCR,sizeof(DWORD)*32);
				unzReadCurrentFile(file,&m_Reg.m_HI,sizeof(__int64));
				unzReadCurrentFile(file,&m_Reg.m_LO,sizeof(__int64));
				unzReadCurrentFile(file,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10);
				unzReadCurrentFile(file,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10);
				unzReadCurrentFile(file,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10);
				unzReadCurrentFile(file,m_Reg.m_Mips_Interface,sizeof(DWORD)*4);
				unzReadCurrentFile(file,m_Reg.m_Video_Interface,sizeof(DWORD)*14);
				unzReadCurrentFile(file,m_Reg.m_Audio_Interface,sizeof(DWORD)*6);
				unzReadCurrentFile(file,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13);
				unzReadCurrentFile(file,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8);
				unzReadCurrentFile(file,m_Reg.m_SerialInterface,sizeof(DWORD)*4);
				unzReadCurrentFile(file,(void *const)&g_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32);
				unzReadCurrentFile(file,m_MMU_VM.PifRam(),0x40);
				unzReadCurrentFile(file,m_MMU_VM.Rdram(),SaveRDRAMSize);
				unzReadCurrentFile(file,m_MMU_VM.Dmem(),0x1000);
				unzReadCurrentFile(file,m_MMU_VM.Imem(),0x1000);
				unzCloseCurrentFile(file);
				port = unzGoToFirstFile(file);
				LoadedZipFile = true;
				continue;
			}
			if (LoadedZipFile && Value == 0x56D2CD23 && port == UNZ_OK) 
			{
				m_SystemTimer.LoadData(file);
			}
			unzCloseCurrentFile(file);
			port = unzGoToNextFile(file);
		}
		unzClose(file);
	}
	if (!LoadedZipFile) 
	{
		HANDLE hSaveFile = CreateFile(FileNameStr.c_str(),GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,NULL,
			OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hSaveFile == INVALID_HANDLE_VALUE) 
		{
			g_Notify->DisplayMessage(5,L"%s %s",GS(MSG_UNABLED_LOAD_STATE),FileNameStr.ToUTF16().c_str());
			return false;
		}

		SetFilePointer(hSaveFile,0,NULL,FILE_BEGIN);
		ReadFile( hSaveFile,&Value,sizeof(Value),&dwRead,NULL);
		if (Value != 0x23D8A6C8)
			return false;

		ReadFile( hSaveFile,&SaveRDRAMSize,sizeof(SaveRDRAMSize),&dwRead,NULL);
		//Check header
		BYTE LoadHeader[64];
		ReadFile( hSaveFile,LoadHeader,0x40,&dwRead,NULL);
		if (memcmp(LoadHeader,g_Rom->GetRomAddress(),0x40) != 0)
		{
			//if (inFullScreen) { return false; }
			int result = MessageBoxW(NULL,GS(MSG_SAVE_STATE_HEADER),GS(MSG_MSGBOX_TITLE),
				MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);

			if (result == IDNO)
				return false;
		}
		Reset(false,true);
		m_MMU_VM.UnProtectMemory(0x80000000,0x80000000 + g_Settings->LoadDword(Game_RDRamSize) - 4);
		m_MMU_VM.UnProtectMemory(0xA4000000,0xA4001FFC);
		g_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);

		ReadFile( hSaveFile,&NextVITimer,sizeof(NextVITimer),&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER),&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_GPR,sizeof(__int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_FPR,sizeof(__int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_CP0,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_FPCR,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.m_HI,sizeof(__int64),&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.m_LO,sizeof(__int64),&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Mips_Interface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Video_Interface,sizeof(DWORD)*14,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Audio_Interface,sizeof(DWORD)*6,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_SerialInterface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,(void *const)&g_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.PifRam(),0x40,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.Rdram(),SaveRDRAMSize,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.Dmem(),0x1000,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.Imem(),0x1000,&dwRead,NULL);
		CloseHandle(hSaveFile);
	}
	
	//Fix losing audio in certain games with certain plugins
	AudioResetOnLoad = g_Settings->LoadBool(Game_AudioResetOnLoad);
	if (AudioResetOnLoad)
	{
		m_Reg.m_AudioIntrReg |= MI_INTR_AI;
		m_Reg.AI_STATUS_REG &= ~AI_STATUS_FIFO_FULL;
		g_Reg->MI_INTR_REG |= MI_INTR_AI;
	}
	
	if (bFixedAudio())
	{
		m_Audio.SetFrequency(m_Reg.AI_DACRATE_REG, g_System->SystemType());
	}
	
	if (old_status != g_Reg->VI_STATUS_REG)
	{
		g_Plugins->Gfx()->ViStatusChanged();
	}
	
	if (old_width != g_Reg->VI_WIDTH_REG)
	{
		g_Plugins->Gfx()->ViWidthChanged();
	}
	
	if (old_dacrate != g_Reg->AI_DACRATE_REG)
	{
		g_Plugins->Audio()->DacrateChanged(g_System->SystemType());
	}
	
	//Fix Random Register
	while ((int)m_Reg.RANDOM_REGISTER < (int)m_Reg.WIRED_REGISTER)
	{
		m_Reg.RANDOM_REGISTER += 32 - m_Reg.WIRED_REGISTER;
	}
	//Fix up timer
	WriteTrace(TraceDebug,__FUNCTION__ ": 2");
	m_SystemTimer.SetTimer(CSystemTimer::CompareTimer,m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER,false);
	m_SystemTimer.SetTimer(CSystemTimer::ViTimer,NextVITimer,false);
	m_Reg.FixFpuLocations();
	WriteTrace(TraceDebug,__FUNCTION__ ": 5");
	m_TLB.Reset(false);
	WriteTrace(TraceDebug,__FUNCTION__ ": 6");
	m_CPU_Usage.ResetCounters();
	WriteTrace(TraceDebug,__FUNCTION__ ": 7");
	m_Profile.ResetCounters();
	WriteTrace(TraceDebug,__FUNCTION__ ": 8");
	m_FPS.Reset(true);
	WriteTrace(TraceDebug,__FUNCTION__ ": 9");
	if (bLogX86Code())
	{
		Stop_x86_Log();
		Start_x86_Log();
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");

#ifdef TEST_SP_TRACKING
	m_CurrentSP = GPR[29].UW[0];
#endif
	if (bFastSP() && m_Recomp) { m_Recomp->ResetMemoryStackPos(); }

	if (g_Settings->LoadDword(Game_CpuType) == CPU_SyncCores) 
	{
		if (m_SyncCPU)
		{
			for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) 
			{
				m_LastSuccessSyncPC[i] = 0;
			}
			m_SyncCPU->SetActiveSystem(true);
			m_SyncCPU->LoadState(FileNameStr.c_str());
			SetActiveSystem(true);
			SyncCPU(m_SyncCPU);
		}
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": 13");
	std::wstring LoadMsg = g_Lang->GetString(MSG_LOADED_STATE);
	g_Notify->DisplayMessage(5,L"%s %s",LoadMsg.c_str(),CPath(FileNameStr).GetNameExtension().ToUTF16().c_str());
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");
	return true;
}

void CN64System::RunRSP()
{
	WriteTraceF(TraceRSP, __FUNCTION__ ": Start (SP Status %X)",m_Reg.SP_STATUS_REG);
	if ( ( m_Reg.SP_STATUS_REG & SP_STATUS_HALT ) == 0) 
    {
		if ( ( m_Reg.SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) 
        {
			SPECIAL_TIMERS CPU_UsageAddr = Timer_None/*, ProfileAddr = Timer_None*/;
			
			DWORD Task = 0;
			if (m_RspBroke)
			{
				g_MMU->LW_VAddr(0xA4000FC0,Task);
				if (Task == 1 && (m_Reg.DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0) 
				{
					WriteTrace(TraceRSP, __FUNCTION__ ": Dlist that is frozen");
					return;
				}

				switch (Task)
				{
				case 1:  
					WriteTrace(TraceRSP, __FUNCTION__ ": *** Display list ***");
					m_DlistCount   += 1; 
					m_FPS.UpdateDlCounter();
					break;
				case 2:  
					WriteTrace(TraceRSP, __FUNCTION__ ": *** Audio list ***");
					m_AlistCount   += 1; 
					break;
				default: 
					WriteTrace(TraceRSP, __FUNCTION__ ": *** Unknown list ***");
					m_UnknownCount += 1; 
					break;
				}

				if (bShowDListAListCount())
				{				
					g_Notify->DisplayMessage(0,L"Dlist: %d   Alist: %d   Unknown: %d",m_DlistCount,m_AlistCount,m_UnknownCount);
				}
				if (bShowCPUPer()) 
				{
					switch (Task) 
					{
					case 1:  CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RSP_Dlist); break;
					case 2:  CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RSP_Alist); break;
					default: CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RSP_Unknown); break;
					}
				}
			}
			

			__try 
			{
				WriteTrace(TraceRSP, __FUNCTION__ ": do cycles - starting");
				g_Plugins->RSP()->DoRspCycles(100);
				WriteTrace(TraceRSP, __FUNCTION__ ": do cycles - Done");
			}
			__except( g_MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) 
			{
				WriteTrace(TraceError, __FUNCTION__ ": exception generated");
				g_Notify->FatalError(__FUNCTIONW__ L"\nUnknown memory action\n\nEmulation stop");
			}

			if (Task == 1 && bDelayDP() && ((m_Reg.m_GfxIntrReg & MI_INTR_DP) != 0))
			{
				g_SystemTimer->SetTimer(CSystemTimer::RSPTimerDlist,0x1000,false);
				m_Reg.m_GfxIntrReg &= ~MI_INTR_DP;
			}
			if (bShowCPUPer())  { m_CPU_Usage.StartTimer(CPU_UsageAddr); }
			//if (bProfiling) { m_Profile.StartTimer(ProfileAddr); }

			if ( ( m_Reg.SP_STATUS_REG & SP_STATUS_HALT ) == 0 && 
				( m_Reg.SP_STATUS_REG & SP_STATUS_BROKE ) == 0 && 
				m_Reg.m_RspIntrReg == 0) 
			{
				g_SystemTimer->SetTimer(CSystemTimer::RspTimer,0x200,false);
				m_RspBroke = false;
			}
			else
			{
				m_RspBroke = true;
			}
			WriteTrace(TraceRSP, __FUNCTION__ ": check interrupts");
			g_Reg->CheckInterrupts();
		}
	}
	WriteTraceF(TraceRSP, __FUNCTION__ ": Done (SP Status %X)",m_Reg.SP_STATUS_REG);
}

void CN64System::SyncToAudio()
{
	if (!bSyncToAudio() || !bLimitFPS())
	{
		return;
	}
	SPECIAL_TIMERS CPU_UsageAddr = Timer_None;

	if (bShowCPUPer()) { CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_Idel); }
	
	for (int i = 0; i < 50; i++)
	{
		if (g_Reg->m_AudioIntrReg != 0)
		{
			WriteTraceF(TraceAudio, __FUNCTION__ ": Audio Interrupt done (%d)",i);
			break;
		}
		Sleep(1);
	}
	if (bShowCPUPer()) 
	{
		m_CPU_Usage.StartTimer(CPU_UsageAddr != Timer_None ? CPU_UsageAddr : Timer_R4300 );
	}
}

void CN64System::RefreshScreen()
{
	SPECIAL_TIMERS CPU_UsageAddr = Timer_None/*, ProfilingAddr = Timer_None*/;
	DWORD VI_INTR_TIME = 500000;
	
	if (bShowCPUPer()) { CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RefreshScreen); }
	//if (bProfiling)    { ProfilingAddr = m_Profile.StartTimer(Timer_RefreshScreen); }

	//Calculate how many cycles to next refresh
	if (m_Reg.VI_V_SYNC_REG == 0) 
	{
		VI_INTR_TIME = 500000;
	}
	else
	{
		VI_INTR_TIME = (m_Reg.VI_V_SYNC_REG + 1) * ViRefreshRate();
		if ((m_Reg.VI_V_SYNC_REG % 1) != 0) 
		{
			VI_INTR_TIME -= 38;
		}
	}
	g_SystemTimer->SetTimer(CSystemTimer::ViTimer,VI_INTR_TIME,true);
	if (bFixedAudio())
	{
		g_Audio->SetViIntr (VI_INTR_TIME);	
	}
	if (g_Plugins->Control()->GetKeys) 
	{
		BUTTONS Keys;
		memset(&Keys,0,sizeof(Keys));

		for (int Control = 0; Control < 4; Control++)
		{	
			g_Plugins->Control()->GetKeys(Control,&Keys);
			m_Buttons[Control] = Keys.Value;
		}
	}

	if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateScreen); }
//	if (bProfiling)    { m_Profile.StartTimer(Timer_UpdateScreen); }
	
	__try
	{
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Starting");
		g_Plugins->Gfx()->UpdateScreen();
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Done");
	}
	__except (g_MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation())) 
	{
		WriteTrace(TraceGfxPlugin,__FUNCTION__ ": Exception caught");
		WriteTrace(TraceError,__FUNCTION__ ": Exception caught");
	}

	g_MMU->UpdateFieldSerration((m_Reg.VI_STATUS_REG & 0x40) != 0);
	
	if ((bBasicMode() || bLimitFPS() ) && !bSyncToAudio()) 
	{
		if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_Idel); }
		DWORD FrameRate;
		if (m_Limitor.Timer_Process(&FrameRate) && bDisplayFrameRate()) 
		{
			m_FPS.DisplayViCounter(FrameRate);
			m_bCleanFrameBox = true;
		}
	}
	else if (bDisplayFrameRate()) 
	{
		if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateFPS); }
		m_FPS.UpdateViCounter();
		m_bCleanFrameBox = true;
	}
	
	if (m_bCleanFrameBox && !bDisplayFrameRate())
	{
		m_FPS.Reset (true);
		m_bCleanFrameBox = false;
	}

	if (bShowCPUPer()) 
	{
		m_CPU_Usage.StopTimer();
		m_CPU_Usage.ShowCPU_Usage();
		m_CPU_Usage.StartTimer(CPU_UsageAddr != Timer_None ? CPU_UsageAddr : Timer_R4300 );
	}
	if ((m_Reg.STATUS_REGISTER & STATUS_IE) != 0 ) 
	{ 
		if (g_BaseSystem == NULL)
		{
			return;
		}
		if (g_BaseSystem->m_Cheats.CheatsSlectionChanged())
		{
			g_BaseSystem->m_Cheats.LoadCheats(false, g_BaseSystem->m_Plugins);
		}
		g_BaseSystem->m_Cheats.ApplyCheats(g_MMU);
	}
//	if (bProfiling)    { m_Profile.StartTimer(ProfilingAddr != Timer_None ? ProfilingAddr : Timer_R4300); }
}

bool CN64System::WriteToProtectedMemory (DWORD Address, int length)
{
	WriteTraceF(TraceDebug,__FUNCTION__ ": Address: %X Len: %d",Address,length);
	if (m_Recomp)
	{
		g_Notify->BreakPoint(__FILEW__,__LINE__);
#ifdef tofix
		return m_Recomp->ClearRecompCode_Phys(Address,length,CRecompiler::Remove_ProtectedMem);
#endif
	}
	return false;
}

void CN64System::TLB_Mapped ( DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly )
{
	m_MMU_VM.TLB_Mapped(VAddr,Len,PAddr,bReadOnly);
}

void CN64System::TLB_Unmaped ( DWORD VAddr, DWORD Len )
{
	m_MMU_VM.TLB_Unmaped(VAddr,Len);
	if (m_Recomp && bSMM_TLB())
	{
		m_Recomp->ClearRecompCode_Virt(VAddr,Len,CRecompiler::Remove_TLB);
	}
}

void CN64System::TLB_Changed()
{
#if defined(WINDOWS_UI)
	Debug_RefreshTLBWindow();
#else
	g_Notify -> BreakPoint(__FILEW__, __LINE__);
#endif
}
