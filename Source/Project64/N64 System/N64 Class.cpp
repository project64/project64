#include "..\N64 System.h"
#include "..\Plugin.h"
#include "..\3rd Party\Zip.h"
#include "C Core\c core.h"

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

#include <windows.h>

CN64System::CN64System ( CPlugins * Plugins, bool SavesReadOnly ) :
	m_MMU_VM(this),
	m_TLB(this),
	m_FPS(_Notify),
	m_CPU_Usage(_Notify),
	m_Profile(_Notify),
	m_Limitor(_Notify),
	m_Plugins(Plugins),
	m_Cheats(NULL),
	m_SyncCPU(NULL),
	m_Recomp(NULL),
	m_InReset(false),
	m_EndEmulation(false),
	m_bCleanFrameBox(true),
	m_bInitilized(false)
{		
	m_CPU_Handle   = 0;
	m_CPU_ThreadID = 0;

	_Settings->RegisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_Contr,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_EditPlugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);

	//InterpreterOpcode = NULL;
	m_hPauseEvent = CreateEvent(NULL,true,false,NULL);

	//stop CPU and destroy current MMU
	Reset();
	
	switch (_Rom->GetCountry())
	{
		case Germany: case french:  case Italian:
		case Europe:  case Spanish: case Australia:
		case X_PAL:   case Y_PAL:
			m_SystemType = SYSTEM_PAL;
			break;
		default:
			m_SystemType = SYSTEM_NTSC;
			break;
	}
	m_Audio.AiSetFrequency(m_Reg.AI_DACRATE_REG,m_SystemType);
	m_Limitor.SetHertz(_Settings->LoadDword(Game_ScreenHertz));
	_Notify->SetWindowCaption(_Settings->LoadString(Game_GoodName).c_str());

	m_Cheats.LoadCheats(!_Settings->LoadDword(Setting_RememberCheats));
}

CN64System::~CN64System ( void ) {
	CloseCpu();
	if (_Plugins)
	{
		_Plugins->ShutDownPlugins();
	}
	Reset();
	CloseHandle(m_hPauseEvent);

	_Settings->UnregisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_Contr,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_EditPlugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);
}

void CN64System::PluginChanged ( CN64System * _this )
{
	if (_Settings->LoadBool(GameRunning_LoadingInProgress))
	{
		return;
	}
	if (_Settings->LoadBool(GameRunning_CPU_Running) != 0) {
		_this->ExternalEvent(ChangePlugins);
	} else {
		_this->Plugins()->Reset();
		if (_Notify)
		{
			_Notify->RefreshMenu();
		}
	}
}

void CN64System::ExternalEvent ( SystemEvent Event ) {
	if (g_CPU_Action == NULL)
	{
		return;
	}

	switch (Event) {
	case Profile_GenerateLogs:
		g_CPU_Action->ProfileGenerateLogs = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Profile_StartStop:
		g_CPU_Action->ProfileStartStop = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Profile_ResetLogs:
		g_CPU_Action->ProfileResetStats = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case ExecuteInterrupt:
		g_CPU_Action->DoInterrupt = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case SaveMachineState:
		g_Main_CPU_Action->SaveState = TRUE;
		g_Main_CPU_Action->DoSomething = TRUE;
		break;
	case LoadMachineState:
		g_Main_CPU_Action->RestoreState = TRUE;
		g_Main_CPU_Action->DoSomething = TRUE;
		break;
	case ChangePlugins:
		g_CPU_Action->ChangePlugin = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case ChangingFullScreen:
		g_CPU_Action->ChangeWindow = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case GSButtonPressed:
		g_CPU_Action->GSButton = TRUE;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case CPUUsageTimerChanged:
		g_ShowCPUPer = _Settings->LoadDword(UserInterface_ShowCPUPer);
		break;
	case PauseCPU_FromMenu: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_FromMenu);
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
		break;
	case PauseCPU_AppLostFocus: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused) && !g_CPU_Action->Pause)
		{
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostFocus);
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
		break;
	case PauseCPU_AppLostActive: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_AppLostActive);
		}
		break;
	case PauseCPU_SaveGame: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SaveGame);
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
		break;
	case PauseCPU_LoadGame: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_LoadGame);
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
		break;
	case PauseCPU_DumpMemory: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_DumpMemory);
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
		break;
	case PauseCPU_SearchMemory: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			_Settings->SaveDword(GameRunning_CPU_PausedType, PauseType_SearchMemory);
			g_CPU_Action->Pause = TRUE;
			g_CPU_Action->DoSomething = TRUE;
		}
		break;
	case ResumeCPU_FromMenu:
		// always resume if from menu
		SetEvent(m_hPauseEvent);
		break;
	case ResumeCPU_AppGainedFocus:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostFocus )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case ResumeCPU_AppGainedActive:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostActive )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case ResumeCPU_SaveGame:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SaveGame )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case ResumeCPU_LoadGame:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_LoadGame )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case ResumeCPU_DumpMemory:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_DumpMemory )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case ResumeCPU_SearchMemory:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SearchMemory )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case ResetCPU_Soft:
        g_CPU_Action->SoftReset = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case ResetCPU_Hard:
		{
			m_InReset = true;
		
			CN64Rom * TempRom = _Rom;
			_Rom = 0;
			Reset();
			_Rom = TempRom;
			
			m_Limitor.SetHertz(_Settings->LoadDword(Game_ScreenHertz)); //Is set in LoadRomSettings

			//Recreate Memory
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix				
			m_Reg = new CRegisters(this, _Notify);
			_MMU = new CMipsMemoryVM(this);
			_MMU->FixRDramSize();
#endif
			
			m_Audio.ResetAudioSettings();
			m_Audio.AiSetFrequency(m_Reg.AI_DACRATE_REG,m_SystemType);

			m_InReset = false;
			StartEmulation(true);
		}
		break;
	case Interrupt_SP:
		g_CPU_Action->InterruptFlag = MI_INTR_SP;
		g_CPU_Action->GenerateInterrupt = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Interrupt_SI:
		g_CPU_Action->InterruptFlag = MI_INTR_SI;
		g_CPU_Action->GenerateInterrupt = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Interrupt_AI:
		g_CPU_Action->InterruptFlag = MI_INTR_AI;
		g_CPU_Action->GenerateInterrupt = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Interrupt_VI:
		g_CPU_Action->InterruptFlag = MI_INTR_VI;
		g_CPU_Action->GenerateInterrupt = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Interrupt_PI:
		g_CPU_Action->InterruptFlag = MI_INTR_PI;
		g_CPU_Action->GenerateInterrupt = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	case Interrupt_DP:
		g_CPU_Action->InterruptFlag = MI_INTR_DP;
		g_CPU_Action->GenerateInterrupt = true;
		g_CPU_Action->DoSomething = TRUE;
		break;
	default:
		WriteTraceF(TraceError,"CN64System::ExternalEvent - Unknown event %d",Event);
		_Notify->BreakPoint(__FILE__,__LINE__);
	}
}

bool CN64System::RunFileImage ( const char * FileLoc ) 
{
	if (_Settings->LoadBool(GameRunning_LoadingInProgress))
	{
		return false;
	}
	_Settings->SaveBool(GameRunning_LoadingInProgress,true);

	HANDLE  *hThread = new HANDLE;
	*hThread = NULL;

	//create the needed info into a structure to pass as one paramater
	//for createing a thread
	FileImageInfo * Info = new FileImageInfo;
	Info->FileName     = FileLoc;
	Info->ThreadHandle = hThread;
	
	*hThread  = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)stLoadFileImage,Info,0, &(Info->ThreadID));
	if (*hThread == NULL)
	{
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		delete Info;
		delete hThread;
		return false;
	}
	return true;
}

void CN64System::stLoadFileImage (  FileImageInfo * Info ) 
{
	CoInitialize(NULL);

	WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Copy thread Info");

	FileImageInfo ImageInfo = *Info;
	HANDLE ThreadHandle = *((HANDLE *)ImageInfo.ThreadHandle);
	delete (HANDLE *)ImageInfo.ThreadHandle;
	ImageInfo.ThreadHandle = &ThreadHandle; 
	delete Info;

	WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Mark Rom as loading");

	//Mark the rom as loading
	_Settings->SaveBool(GameRunning_LoadingInProgress,true);
	_Notify->RefreshMenu();
	
	//Try to load the passed N64 rom
	if (_Rom == NULL)
	{
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Allocating global rom object");
		_Rom = new CN64Rom();
	} else {
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Use existing global rom object");
	}
	
	WriteTraceF(TraceDebug,"CN64System::stLoadFileImage: Loading \"%s\"",ImageInfo.FileName.c_str());
	if (_Rom->LoadN64Image(ImageInfo.FileName.c_str())) 
	{
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Add Recent Rom");
		_Notify->AddRecentRom(ImageInfo.FileName.c_str());

//		_Plugins->Reset();
		if (_N64System)
		{
			WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Destroying old N64 system");
			delete _N64System;
			_N64System = NULL;
		}
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Creating N64 system");
		CN64System * System = new CN64System(_Plugins,false);
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Setting N64 system as active");
		if (System->SetActiveSystem(true))
		{
			WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Setting up N64 system done");
			_Settings->SaveBool(GameRunning_LoadingInProgress,false);
			_Notify->RefreshMenu();
			if (_Settings->LoadDword(Setting_AutoStart) != 0)
			{
				System->m_CPU_Handle   = *((HANDLE *)ImageInfo.ThreadHandle);
				System->m_CPU_ThreadID = ImageInfo.ThreadID;

				try
				{
					WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Game set to auto start, starting");
					System->StartEmulation2(false);			
					WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Game Done");
				} 
				catch (...)
				{
					WriteTraceF(TraceError,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
					char Message[600];
					sprintf(Message,"CN64System::LoadFileImage - Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
					MessageBox(NULL,Message,"Exception",MB_OK);
				}
				System->m_CPU_Handle   = NULL;
				System->m_CPU_ThreadID = 0;
			}
		} else {
			WriteTrace(TraceError,"CN64System::stLoadFileImage: SetActiveSystem failed");
 			_Notify->DisplayError("Failed to Initialize N64 System");
			delete System;
			System = NULL;
			delete _Rom;
			_Rom = NULL;
			_Settings->SaveBool(GameRunning_LoadingInProgress,false);
			_Notify->RefreshMenu();
			return;
		}
	} else {
		WriteTraceF(TraceError,"CN64System::stLoadFileImage: LoadN64Image failed (\"%s\")",ImageInfo.FileName.c_str());
 		_Notify->DisplayError(_Rom->GetError());
		delete _Rom;
		_Rom = NULL;
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->RefreshMenu();
		return;
	}
	CoUninitialize();
	WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Done");
}	
	
void  CN64System::StartEmulation2   ( bool NewThread )
{
	if (NewThread)
	{
		FileImageInfo * Info = new FileImageInfo;
		HANDLE  * hThread = new HANDLE;
		*hThread = NULL;

		//create the needed info into a structure to pass as one paramater
		//for createing a thread
		//Info->_this        = this;
		Info->ThreadHandle = hThread;
		
		DWORD ThreadID;
		*hThread  = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartEmulationThread,Info,0, &ThreadID);	
		return;
	}
	WriteTrace(TraceDebug,"CN64System::StartEmulation2: Starting");

	_Notify->HideRomBrowser();
	//RefreshSettings();

	WriteTrace(TraceDebug,"CN64System::StartEmulation2: Reseting Plugins");
	_Notify->DisplayMessage(5,MSG_PLUGIN_INIT);
	_Plugins->Reset();
	if (!_Plugins->Initiate())
	{
		WriteTrace(TraceError,"CN64System::StartEmulation2: _Plugins->Initiate Failed");
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->DisplayError(MSG_PLUGIN_NOT_INIT);
		//Set handle to NULL so this thread is not terminated
		m_CPU_Handle = NULL;
		m_CPU_ThreadID = 0;

	//	Reset();
		_Notify->RefreshMenu();
		_Notify->ShowRomBrowser();
		return;
	}

	_Notify->MakeWindowOnTop(_Settings->LoadBool(UserInterface_AlwaysOnTop));
	if (!_Settings->LoadBool(Beta_IsValidExe))
	{
		Reset();
		return;
	}

	//mark the emulation as starting and fix up menus
	_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);

	if (_Settings->LoadBool(Setting_AutoFullscreen)) 
	{
		WriteTrace(TraceDebug,"CN64System::StartEmulation 15");
		CIniFile RomIniFile(_Settings->LoadString(SupportFile_RomDatabase).c_str());
		stdstr Status = _Settings->LoadString(Rdb_Status);

		char String[100];
		RomIniFile.GetString("Rom Status",stdstr_f("%s.AutoFullScreen", Status.c_str).c_str(),"true",String,sizeof(String));
		if (_stricmp(String,"true") == 0)
		{
			_Notify->ChangeFullScreen();
		}
	}
	ExecuteCPU();
}

void  CN64System::StartEmulation   ( bool NewThread )
{
	__try 
	{
		StartEmulation2(NewThread);
	} __except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		char Message[600];
		sprintf(Message,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
		MessageBox(NULL,Message,"Exception",MB_OK);
	}
}

void CN64System::StartEmulationThread (  FileImageInfo * Info ) 
{
	CoInitialize(NULL);
#ifdef tofix
	//Make a copy of the passed infomation on to the stack, then free
	//the memory used
	FileImageInfo ImageInfo = *Info;
	CN64System * _this = ImageInfo._this;
	delete Info;

	_this->m_CPU_Handle = (void *)(*((DWORD *)ImageInfo.ThreadHandle));
	_this->m_CPU_ThreadID = ImageInfo.ThreadID;
	delete ImageInfo.ThreadHandle;
	
	_this->StartEmulation(false);
#endif
	CoUninitialize();
}

void CN64System::CloseCpu ( void ) {
	if (m_CPU_Handle == NULL) { return; }
	Debug_Reset();

	if (_Settings->LoadBool(GameRunning_CPU_Paused))
	{
		SetEvent(m_hPauseEvent);
	}
	
	void * lCPU_Handle = m_CPU_Handle;
	if (GetCurrentThreadId() == m_CPU_ThreadID)
	{
		Close_C_CPU();
		return;
	}
	Close_C_CPU();
	for (int count = 0; count < 40 && (lCPU_Handle == m_CPU_Handle); count ++ ) {
		Sleep(500);
		if (_Notify->ProcessGuiMessages())
		{
			return;
		}
	
		DWORD ExitCode;
		if (GetExitCodeThread(lCPU_Handle,&ExitCode))
		{
			if (ExitCode != STILL_ACTIVE) {
				if (lCPU_Handle == m_CPU_Handle)
				{
					m_CPU_Handle = NULL;
				}
				break;
			}
		}
	}
	if (lCPU_Handle != m_CPU_Handle) { return; }
#ifdef _DEBUG
	_Notify->DisplayError("Terminate Thread");
#endif
	TerminateThread(m_CPU_Handle,0); 
	m_CPU_Handle = NULL;
	m_CPU_ThreadID = 0;
	CpuStopped();
}

void CN64System::SelectCheats ( WND_HANDLE hParent ) 
{
	m_Cheats.SelectCheats(hParent,false);
}

void CN64System::DisplayRomInfo ( WND_HANDLE hParent ) {
	if (!_Rom) { return; }
	
	RomInformation Info(_Rom);
	Info.DisplayInformation(hParent);
}

void CN64System::Pause(void)
{
	ResetEvent(m_hPauseEvent);
	_Settings->SaveBool(GameRunning_CPU_Paused,true);
	_Notify->RefreshMenu();
	_Notify->DisplayMessage(5,MSG_CPU_PAUSED);
	WaitForSingleObject(m_hPauseEvent, INFINITE);
	ResetEvent(m_hPauseEvent);
	_Settings->SaveBool(GameRunning_CPU_Paused,(DWORD)false);
	_Notify->RefreshMenu();
	_Notify->DisplayMessage(5,MSG_CPU_RESUMED);
}

stdstr CN64System::ChooseFileToOpen ( WND_HANDLE hParent ) {
	OPENFILENAME openfilename;
	char FileName[_MAX_PATH],Directory[_MAX_PATH];

	memset(&FileName, 0, sizeof(FileName));
	memset(&openfilename, 0, sizeof(openfilename));

	strcpy(Directory,_Settings->LoadString(Directory_Game).c_str());

	openfilename.lStructSize  = sizeof( openfilename );
	openfilename.hwndOwner    = (HWND)hParent;
	openfilename.lpstrFilter  = "N64 ROMs (*.zip, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
	openfilename.lpstrFile    = FileName;
	openfilename.lpstrInitialDir    = Directory;
	openfilename.nMaxFile     = MAX_PATH;
	openfilename.Flags        = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileName (&openfilename)) {							
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

void  CN64System::SoftReset()
{
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	_MMU->InitalizeSystem(true);
#endif
	if (GetRecompiler())
	{
		GetRecompiler()->ResetRecompCode(); 
	}
	g_CPU_Action->DoSomething = TRUE;
	if (_Plugins) { _Plugins->GameReset(); }
	Debug_Reset();
	CloseSaveChips();
	m_CyclesToSkip = 0;
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;

	g_CPU_Action->DMAUsed = false;
	g_CPU_Action->InterruptExecuted = false;
}

void CN64System::Reset (void) {
	CloseCpu();

	if (_Plugins) { _Plugins->GameReset(); }
	m_Audio.ResetAudioSettings();
	Debug_Reset();
	CloseSaveChips();

	m_CyclesToSkip = 0;
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;
	m_SystemType   = SYSTEM_NTSC;
}

bool CN64System::SetActiveSystem( bool bActive )
{
	bool bInitPlugin = false;

	if (!m_bInitilized)
	{
		if (!m_MMU_VM.Initialize())
		{
			return false;
		}
		bool PostPif = true;
		
		m_Reg.InitalizeR4300iRegisters(m_MMU_VM, PostPif, _Rom->GetCountry(), _Rom->CicChipID());
		if (PostPif) 
		{
			memcpy((m_MMU_VM.Dmem()+0x40), (_Rom->GetRomAddress() + 0x040), 0xFBC);
		}
		bInitPlugin = true;
	}

	_N64System = this;
	_MMU = &m_MMU_VM;
	_Reg = &m_Reg;

	if (bInitPlugin)
	{
		_Plugins->Initiate();
	}
	return true;
}

void CN64System::ExecuteCPU ( void ) 
{
	_Settings->SaveBool(GameRunning_CPU_Running,true);
	_Settings->SaveBool(GameRunning_CPU_Paused,false);
	_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);
	
	m_EndEmulation = false;
	_Notify->RefreshMenu();

	CC_Core C_Core;
	C_Core.SetSettings();
//	C_Core.SetN64System(NULL);
	C_Core.SetSyncCpu(NULL);
	
	switch ((CPU_TYPE)_Settings->LoadDword(Game_CpuType)) {
	case CPU_Recompiler: ExecuteRecompiler(C_Core); break;
	case CPU_SyncCores:  ExecuteSyncCPU(C_Core);    break;
	default:             ExecuteInterpret(C_Core);  break;
	}
	CpuStopped();
	SetActiveSystem(false);
}

void CN64System::ExecuteInterpret (CC_Core & C_Core) {
	C_Core.SetN64System(this);
	InitializeCPUCore();
	ExecuteCycles(-1);
}

void CN64System::ExecuteRecompiler (CC_Core & C_Core) {	
	//execute opcodes while no errors	
	CRecompiler Recompiler(m_Profile,m_EndEmulation,false);
	m_Recomp = &Recompiler;
	C_Core.SetN64System(this);
	InitializeCPUCore();
	Recompiler.Run();
}

void CN64System::ExecuteSyncCPU (CC_Core & C_Core) 
{
	Notify().BreakPoint(__FILE__,__LINE__);
/*	//execute opcodes while no errors		
	_Notify->DisplayMessage(5,"Copy Plugins");
	_Plugins->CopyPlugins(_Settings->LoadString(SyncPluginDir));
	//copy the plugins

	CPlugins    SyncPlugins ( _Settings->LoadString(SyncPluginDir) ); 
	CN64System  SyncSystem  ( _Notify,&SyncPlugins);   //Create the backend n64 system
	m_SyncCPU = &SyncSystem;
	SyncSystem.SetupSystem(_Rom,false,true);
	SyncSystem._MMU->FixRDramSize();

	SyncPlugins.Control()->SetControl(_Plugins->Control());
	
	{
		CMainGui  SyncWindow("Sync CPU",_Notify,&SyncSystem);

		SyncPlugins.SetRenderWindows(&SyncWindow,&SyncWindow);

		_Notify->DisplayMessage(5,"Initilizing Sync Plugins");
		if (!SyncPlugins.Initiate(&SyncSystem)) {
			_Notify->DisplayMessage(5,MSG_PLUGIN_NOT_INIT);
			return;
		}

		C_Core.SetSyncCpu(&SyncSystem);

		CRecompiler Recompiler(_MMU,m_Profile,m_EndEmulation,true);
		m_Recomp = &Recompiler;
		C_Core.SetN64System(this);
		InitializeCPUCore();
		Recompiler.Run();
		SyncSystem._Rom = NULL;
	}*/
}

void CN64System::CpuStopped ( void ) {
	void * lCPU_Handle = m_CPU_Handle;
	_Settings->SaveBool(GameRunning_CPU_Running,(DWORD)false);
	CleanCMemory();
	_Notify->WindowMode();
	if (!m_InReset)
	{
		_Notify->RefreshMenu();
		_Notify->MakeWindowOnTop(false);
		_Notify->DisplayMessage(5,MSG_EMULATION_ENDED);
		if (_Settings->LoadDword(RomBrowser_Enabled)) {
			_Notify->ShowRomBrowser(); 
		}	
	}
	if (lCPU_Handle == m_CPU_Handle)
	{
		m_CPU_ThreadID = 0;
		m_CPU_Handle   = 0;
	}
}

void CN64System::UpdateSyncCPU (CN64System * const SecondCPU, DWORD const Cycles) {
	int CyclesToExecute = Cycles - m_CyclesToSkip;
	
	//Update the number of cycles to skip
	m_CyclesToSkip -= Cycles;
	if (m_CyclesToSkip < 0) { m_CyclesToSkip = 0; }

	//Run the other CPU For the same amount of cycles
	if (CyclesToExecute < 0) { return; }
	CC_Core::SetCurrentSystem(SecondCPU);	
	SecondCPU->ExecuteCycles(CyclesToExecute);
	CC_Core::SetCurrentSystem(this);	
}

void CN64System::ExecuteCycles (DWORD Cycles) {
	::ExecuteCycles(Cycles);
}

void CN64System::SyncCPU (CN64System * const SecondCPU) {
	bool ErrorFound = false;
#ifdef TEST_SP_TRACKING
	if (m_CurrentSP != GPR[29].UW[0]) {
		ErrorFound = true;
	}
#endif
	if (m_Reg.PROGRAM_COUNTER != SecondCPU->m_Reg.PROGRAM_COUNTER) {
		ErrorFound = true;
	}
	for (int count = 0; count < 32; count ++) {
		if (m_Reg.GPR[count].DW != SecondCPU->m_Reg.GPR[count].DW) {
			ErrorFound = true;
		}
		if (m_Reg.FPR[count].DW != SecondCPU->m_Reg.FPR[count].DW) {
			ErrorFound = true;
		}
		if (m_Reg.CP0[count] != SecondCPU->m_Reg.CP0[count]) {
			ErrorFound = true;
		}
	}

	for (int z = 0; z < 0x100; z++)
	{	
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if (_MMU->RDRAM[0x00325044 + z] !=  SecondCPU->_MMU->RDRAM[0x00325044 + z]) 
		{
			ErrorFound = true;
		}
#endif
	}
	
	if (bSPHack()) 
	{
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		if (_MMU->m_MemoryStack != (DWORD)(RDRAM + (m_Reg.GPR[29].W[0] & 0x1FFFFFFF)))
		{
			ErrorFound = true;
		}
#endif
	}

	if (m_Reg.GetCurrentTimer()     != SecondCPU->m_Reg.GetCurrentTimer()) { ErrorFound = true; }
	if (m_Reg.GetCurrentTimerType() != SecondCPU->m_Reg.GetCurrentTimerType()) { ErrorFound = true; }
	
	if (ErrorFound) { DumpSyncErrors(SecondCPU); }

	for (int i = (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--) {
		m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
	}
	m_LastSuccessSyncPC[0] = m_Reg.PROGRAM_COUNTER;
//	if (PROGRAM_COUNTER == 0x8009BBD8) {
//		_Notify->BreakPoint(__FILE__,__LINE__);
//	}
}

void CN64System::DumpSyncErrors (CN64System * SecondCPU) {
	int count;
	
	{
		CLog Error;
		Error.Open("Sync Errors.txt");
		Error.Log("Errors:\r\n");
		Error.Log("Register,        Recompiler,         Interpter\r\n");
	#ifdef TEST_SP_TRACKING
		if (m_CurrentSP != GPR[29].UW[0]) {
			Error.Log("m_CurrentSP,%X,%X\r\n",m_CurrentSP,GPR[29].UW[0]);
		}
	#endif
		if (m_Reg.PROGRAM_COUNTER != SecondCPU->m_Reg.PROGRAM_COUNTER) {
			Error.LogF("PROGRAM_COUNTER, 0x%X,         0x%X\r\n",m_Reg.PROGRAM_COUNTER,SecondCPU->m_Reg.PROGRAM_COUNTER);
		}
		for (count = 0; count < 32; count ++) {
			if (m_Reg.GPR[count].DW != SecondCPU->m_Reg.GPR[count].DW) {
				Error.LogF("GPR[%s] Different,0x%08X%08X, 0x%08X%08X\r\n",m_Reg.GPR_Name[count],
					m_Reg.GPR[count].W[1],m_Reg.GPR[count].W[0],
					SecondCPU->m_Reg.GPR[count].W[1],SecondCPU->m_Reg.GPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (m_Reg.FPR[count].DW != SecondCPU->m_Reg.FPR[count].DW) {
				Error.LogF("FPR[%s] Different,0x%08X%08X, 0x%08X%08X\r\n",m_Reg.FPR_Name[count],
					m_Reg.FPR[count].W[1],m_Reg.FPR[count].W[0],
					SecondCPU->m_Reg.FPR[count].W[1],SecondCPU->m_Reg.FPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (m_Reg.CP0[count] != SecondCPU->m_Reg.CP0[count]) {
				Error.LogF("CP0[%s] Different,0x%08X, 0x%08X\r\n",m_Reg.Cop0_Name[count],
					m_Reg.CP0[count],	SecondCPU->m_Reg.CP0[count]);
			}
		}	
		if (m_Reg.GetCurrentTimer()     != SecondCPU->m_Reg.GetCurrentTimer()) 
		{ 
			Error.LogF("Current Time is Different: %X %X\r\n",(DWORD)m_Reg.GetCurrentTimer(),(DWORD)SecondCPU->m_Reg.GetCurrentTimer());
		}
		if (m_Reg.GetCurrentTimerType() != SecondCPU->m_Reg.GetCurrentTimerType()) 
		{ 
			Error.LogF("Current Time Type is Different: %X %X\r\n",m_Reg.GetCurrentTimerType(),SecondCPU->m_Reg.GetCurrentTimerType());
		}
		if (_Settings->LoadDword(Game_SPHack)) 
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			if (_MMU->m_MemoryStack != (DWORD)(RDRAM + (m_Reg.GPR[29].W[0] & 0x1FFFFFFF)))
			{
				Error.LogF("MemoryStack = %X  should be: %X\r\n",_MMU->m_MemoryStack, (DWORD)(_MMU->RDRAM + (m_Reg.GPR[29].W[0] & 0x1FFFFFFF)));
			}
#endif
		}
		Error.Log("\r\n");
		Error.Log("Information:\r\n");
		Error.Log("\r\n");
		Error.LogF("PROGRAM_COUNTER,0x%X\r\n",m_Reg.PROGRAM_COUNTER);
		Error.LogF("Current Timer,0x%X\r\n",m_Reg.GetCurrentTimer());
		Error.LogF("Timer Type,0x%X\r\n",m_Reg.GetCurrentTimerType());
		Error.Log("\r\n");
		for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) {
			Error.LogF("LastSuccessSyncPC[%d],0x%X\r\n",i,m_LastSuccessSyncPC[i]);
		}
		Error.Log("");
		for (count = 0; count < 32; count ++) {
			Error.LogF("GPR[%s],         0x%08X%08X, 0x%08X%08X\r\n",m_Reg.GPR_Name[count],
				m_Reg.GPR[count].W[1],m_Reg.GPR[count].W[0],
				SecondCPU->m_Reg.GPR[count].W[1],SecondCPU->m_Reg.GPR[count].W[0]);
		}	
		Error.Log("");
		for (count = 0; count < 32; count ++) {
			Error.LogF("CP0[%s],%*s0x%08X, 0x%08X\r\n",m_Reg.Cop0_Name[count],
				12 - strlen(m_Reg.Cop0_Name[count]),"",
				m_Reg.CP0[count],SecondCPU->m_Reg.CP0[count]);
		}	
		Error.Log("\r\n");
		Error.Log("         Hi Recomp, PageMask, Hi Interp, PageMask\r\n");
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		for (count = 0; count < 32; count ++) {
			if (!_MMU->tlb[count].EntryDefined) { continue; }
			Error.LogF("TLB[%2d], %08X,  %08X, %08X,  %08X\r\n", count,
				_MMU->tlb[count].EntryHi.Value,_MMU->tlb[count].PageMask.Value,
				SecondCPU->_MMU->tlb[count].EntryHi.Value,SecondCPU->_MMU->tlb[count].PageMask.Value
			);	
		}
#endif
		Error.Log("\r\n");
		Error.LogF("Current Fiber,%X\r\n",GetCurrentFiber());
		Error.Log("\r\n");
		Error.Log("Code at PC:\r\n");
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		COpcode Op(SecondCPU->_MMU,SecondCPU->m_Reg,SecondCPU->m_Reg.PROGRAM_COUNTER - (OpCode_Size * 10));
		for (;Op.PC() < SecondCPU->m_Reg.PROGRAM_COUNTER + (OpCode_Size * 10); Op.Next()) {
			Error.LogF("%X,%s\r\n",Op.PC(),Op.Name().c_str());
		}
		Error.Log("\r\n");
		Error.Log("Code at Last Sync PC:\r\n");
		for (Op.SetPC(m_LastSuccessSyncPC[0]); Op.PC() < m_LastSuccessSyncPC[0] + (OpCode_Size * 50); Op.Next()) {
			Error.LogF("%X,%s\r\n",Op.PC(),Op.Name().c_str());
		}
#endif
	}

	_Notify->DisplayError("Sync Error");
	_Notify->BreakPoint(__FILE__,__LINE__);
//	AddEvent(CloseCPU);
}

bool CN64System::SaveState(void) 
{
	WriteTrace(TraceDebug,"CN64System::SaveState 1");

	if (m_Reg.GetTimer(AiTimerDMA)     != 0)  { return false; }
	if (m_Reg.GetTimer(SiTimer)        != 0)  { return false; }
	if (m_Reg.GetTimer(PiTimer)        != 0)  { return false; }
	if (m_Reg.GetTimer(RSPTimerDlist)  != 0)  { return false; }
	if ((m_Reg.STATUS_REGISTER & STATUS_EXL) != 0) { return false; }
	
	//Get the file Name
	stdstr FileName, CurrentSaveName = _Settings->LoadString(GameRunning_InstantSaveFile);
	if (CurrentSaveName.empty())
	{
		int Slot = _Settings->LoadDword(Game_CurrentSaveState);
		if (Slot != 0) { 
			CurrentSaveName.Format("%s.pj%d",_Settings->LoadString(Game_GoodName).c_str(), Slot);
		} else {
			CurrentSaveName.Format("%s.pj",_Settings->LoadString(Game_GoodName).c_str());
		}
		FileName.Format("%s%s",_Settings->LoadString(Directory_InstantSave).c_str(),CurrentSaveName.c_str());
		stdstr_f ZipFileName("%s.zip",FileName.c_str());
		//Make sure the target dir exists	
		CreateDirectory(_Settings->LoadString(Directory_InstantSave).c_str(),NULL);
		//delete any old save
		DeleteFile(FileName.c_str());
		DeleteFile(ZipFileName.c_str());
	
		//If ziping save add .zip on the end
		if (_Settings->LoadDword(Setting_AutoZipInstantSave)) {
			FileName = ZipFileName;
		}
		_Settings->SaveDword(Game_LastSaveSlot,_Settings->LoadDword(Game_CurrentSaveState));
	} else {
		FileName.Format("%s%s",CurrentSaveName.c_str(), _Settings->LoadDword(Setting_AutoZipInstantSave) ? ".pj.zip" : ".pj");
	}
	if (FileName.empty()) { return true; }

	//Open the file
	if (_Settings->LoadDword(Game_FuncLookupMode) == FuncFind_ChangeMemory) 
	{
		if (GetRecompiler())
		{
			GetRecompiler()->ResetRecompCode(); 
		}
	}

	DWORD dwWritten, SaveID_0 = 0x23D8A6C8;
	DWORD RdramSize   = _Settings->LoadDword(Game_RDRamSize);
	DWORD NextViTimer = m_Reg.GetTimer(ViTimer);
	DWORD MiInterReg  = m_Reg.MI_INTR_REG;
	if (m_Reg.GetTimer(AiTimer) != 0) { m_Reg.MI_INTR_REG |= MI_INTR_AI; }
	if (_Settings->LoadDword(Setting_AutoZipInstantSave)) {
		zipFile			file;

		file = zipOpen(FileName.c_str(),0);
		zipOpenNewFileInZip(file,CurrentSaveName.c_str(),NULL,NULL,0,NULL,0,NULL,Z_DEFLATED,Z_DEFAULT_COMPRESSION);
		zipWriteInFileInZip(file,&SaveID_0,sizeof(SaveID_0));
		zipWriteInFileInZip(file,&RdramSize,sizeof(DWORD));
		zipWriteInFileInZip(file,_Rom->GetRomAddress(),0x40);	
		zipWriteInFileInZip(file,&NextViTimer,sizeof(DWORD));
		zipWriteInFileInZip(file,&m_Reg.PROGRAM_COUNTER,sizeof(m_Reg.PROGRAM_COUNTER));
		zipWriteInFileInZip(file,m_Reg.GPR,sizeof(_int64)*32);
		zipWriteInFileInZip(file,m_Reg.FPR,sizeof(_int64)*32);
		zipWriteInFileInZip(file,m_Reg.CP0,sizeof(DWORD)*32);
		zipWriteInFileInZip(file,m_Reg.FPCR,sizeof(DWORD)*32);
		zipWriteInFileInZip(file,&m_Reg.HI,sizeof(_int64));
		zipWriteInFileInZip(file,&m_Reg.LO,sizeof(_int64));
		zipWriteInFileInZip(file,m_Reg.RDRAM_Registers,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,m_Reg.SigProcessor_Interface,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,m_Reg.Display_ControlReg,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,m_Reg.Mips_Interface,sizeof(DWORD)*4);
		zipWriteInFileInZip(file,m_Reg.Video_Interface,sizeof(DWORD)*14);
		zipWriteInFileInZip(file,m_Reg.Audio_Interface,sizeof(DWORD)*6);
		zipWriteInFileInZip(file,m_Reg.Peripheral_Interface,sizeof(DWORD)*13);
		zipWriteInFileInZip(file,m_Reg.RDRAM_Interface,sizeof(DWORD)*8);
		zipWriteInFileInZip(file,m_Reg.SerialInterface,sizeof(DWORD)*4);
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		zipWriteInFileInZip(file,_MMU->tlb,sizeof(TLB)*32);
		zipWriteInFileInZip(file,_MMU->PIF_Ram,0x40);
#endif
		zipWriteInFileInZip(file,_MMU->Rdram(),RdramSize);
		zipWriteInFileInZip(file,_MMU->Dmem(),0x1000);
		zipWriteInFileInZip(file,_MMU->Imem(),0x1000);
		zipCloseFileInZip(file);
		zipClose(file,"");
	} else {
		HANDLE hSaveFile = CreateFile(FileName.c_str(),GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
			NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hSaveFile == INVALID_HANDLE_VALUE) {
			_Notify->DisplayError(GS(MSG_FAIL_OPEN_SAVE));
			m_Reg.MI_INTR_REG = MiInterReg;
			return true;
		}

		//Write info to file
		SetFilePointer(hSaveFile,0,NULL,FILE_BEGIN);	
		WriteFile( hSaveFile,&SaveID_0,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,&RdramSize,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,_Rom->GetRomAddress(),0x40,&dwWritten,NULL);	
		WriteFile( hSaveFile,&NextViTimer,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.PROGRAM_COUNTER,sizeof(m_Reg.PROGRAM_COUNTER),&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.GPR,sizeof(_int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.FPR,sizeof(_int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.CP0,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.FPCR,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.HI,sizeof(_int64),&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.LO,sizeof(_int64),&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.RDRAM_Registers,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.SigProcessor_Interface,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.Display_ControlReg,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.Mips_Interface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.Video_Interface,sizeof(DWORD)*14,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.Audio_Interface,sizeof(DWORD)*6,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.Peripheral_Interface,sizeof(DWORD)*13,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.RDRAM_Interface,sizeof(DWORD)*8,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.SerialInterface,sizeof(DWORD)*4,&dwWritten,NULL);
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		WriteFile( hSaveFile,_MMU->tlb,sizeof(TLB)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->PIF_Ram,0x40,&dwWritten,NULL);
#endif
		WriteFile( hSaveFile,_MMU->Rdram(),RdramSize,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->Dmem(),0x1000,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->Imem(),0x1000,&dwWritten,NULL);

		CloseHandle(hSaveFile);
	}
	m_Reg.MI_INTR_REG = MiInterReg;
	_Settings->SaveString(GameRunning_InstantSaveFile,"");
	stdstr SaveMessage = _Lang->GetString(MSG_SAVED_STATE);

	CPath SavedFileName(FileName);
	
	_Notify->DisplayMessage(5,"%s %s",SaveMessage.c_str(),SavedFileName.GetNameExtension().c_str());
	_Notify->RefreshMenu();
	WriteTrace(TraceDebug,"CN64System::SaveState 20");
	return true;
}

bool CN64System::LoadState(void) 
{
	stdstr InstantFileName = _Settings->LoadString(GameRunning_InstantSaveFile);
	if (!InstantFileName.empty())
	{
		bool Result = LoadState(InstantFileName.c_str());
		_Settings->SaveString(GameRunning_InstantSaveFile,"");
		return Result;
	}

	CPath FileName;
	FileName.SetDriveDirectory(_Settings->LoadString(Directory_InstantSave).c_str());
	if (_Settings->LoadDword(Game_CurrentSaveState) != 0) {
		FileName.SetNameExtension(stdstr_f("%s.pj%d",_Settings->LoadString(Game_GoodName).c_str(),_Settings->LoadDword(Game_CurrentSaveState)).c_str());
	} else {
		FileName.SetNameExtension(stdstr_f("%s.pj",_Settings->LoadString(Game_GoodName).c_str()).c_str());
	}

	CPath ZipFileName;
	ZipFileName = (stdstr&)FileName + ".zip";

	if ((_Settings->LoadDword(Setting_AutoZipInstantSave) && ZipFileName.Exists()) || FileName.Exists())
	{
		if (LoadState(FileName))
		{
			return true;
		}
	}

	//Use old file Name
	if (_Settings->LoadDword(Game_CurrentSaveState) != 0) { 
		FileName.SetNameExtension(stdstr_f("%s.pj%d",_Settings->LoadString(Game_GameName).c_str(),_Settings->LoadDword(Game_CurrentSaveState)).c_str());
	} else {
		FileName.SetNameExtension(stdstr_f("%s.pj",_Settings->LoadString(Game_GameName).c_str()).c_str());
	}
	return LoadState(FileName);
}

bool CN64System::LoadState(LPCSTR FileName) {
	DWORD dwRead, Value,SaveRDRAMSize, NextVITimer;
	bool LoadedZipFile = false;

	WriteTraceF((TraceType)(TraceDebug | TraceRecompiler),"CN64System::LoadState %s",FileName);

	char drive[_MAX_DRIVE] ,dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
	_splitpath(FileName, drive, dir, fname, ext);

	stdstr FileNameStr(FileName);
	if (_Settings->LoadDword(Setting_AutoZipInstantSave) || _stricmp(ext,".zip") == 0) 
	{
		//If ziping save add .zip on the end
		if (_stricmp(ext,".zip") != 0)
		{
			FileNameStr += ".zip";
		}
		unzFile file = unzOpen(FileNameStr.c_str());
		int port = -1;
		if (file != NULL) {
			port = unzGoToFirstFile(file);
		}
		while (port == UNZ_OK) {
			unz_file_info info;
			char zname[132];

			unzGetCurrentFileInfo(file, &info, zname, 128, NULL,0, NULL,0);
		    if (unzLocateFile(file, zname, 1) != UNZ_OK ) {
				unzClose(file);
				port = -1;
				continue;
			}
			if( unzOpenCurrentFile(file) != UNZ_OK ) {
				unzClose(file);
				port = -1;
				continue;
			}
			DWORD Value;
			unzReadCurrentFile(file,&Value,4);
			if (Value != 0x23D8A6C8) { 
				unzCloseCurrentFile(file);
				return false;
			}
			break;
		}
		if (port == UNZ_OK) {
			unzReadCurrentFile(file,&SaveRDRAMSize,sizeof(SaveRDRAMSize));
			//Check header

			BYTE LoadHeader[64];
			unzReadCurrentFile(file,LoadHeader,0x40);	
			if (memcmp(LoadHeader,_Rom->GetRomAddress(),0x40) != 0) {
				//if (inFullScreen) { return FALSE; }
				int result = MessageBox(NULL,GS(MSG_SAVE_STATE_HEADER),GS(MSG_MSGBOX_TITLE),
					MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
				if (result == IDNO) { return FALSE; }
			}
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			_MMU->UnProtectMemory(0x80000000,0x80000000 + _Settings->LoadDword(Game_RDRamSize) - 4);
			_MMU->UnProtectMemory(0xA4000000,0xA4001FFC);
			if (SaveRDRAMSize != _Settings->LoadDword(Game_RDRamSize)) {
				_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
				_MMU->FixRDramSize();
			}
#endif			
			unzReadCurrentFile(file,&NextVITimer,sizeof(NextVITimer));
			unzReadCurrentFile(file,&m_Reg.PROGRAM_COUNTER,sizeof(m_Reg.PROGRAM_COUNTER));
			unzReadCurrentFile(file,m_Reg.GPR,sizeof(_int64)*32);
			unzReadCurrentFile(file,m_Reg.FPR,sizeof(_int64)*32);
			unzReadCurrentFile(file,m_Reg.CP0,sizeof(DWORD)*32);
			unzReadCurrentFile(file,m_Reg.FPCR,sizeof(DWORD)*32);
			unzReadCurrentFile(file,&m_Reg.HI,sizeof(_int64));
			unzReadCurrentFile(file,&m_Reg.LO,sizeof(_int64));
			unzReadCurrentFile(file,m_Reg.RDRAM_Registers,sizeof(DWORD)*10);
			unzReadCurrentFile(file,m_Reg.SigProcessor_Interface,sizeof(DWORD)*10);
			unzReadCurrentFile(file,m_Reg.Display_ControlReg,sizeof(DWORD)*10);
			unzReadCurrentFile(file,m_Reg.Mips_Interface,sizeof(DWORD)*4);
			unzReadCurrentFile(file,m_Reg.Video_Interface,sizeof(DWORD)*14);
			unzReadCurrentFile(file,m_Reg.Audio_Interface,sizeof(DWORD)*6);
			unzReadCurrentFile(file,m_Reg.Peripheral_Interface,sizeof(DWORD)*13);
			unzReadCurrentFile(file,m_Reg.RDRAM_Interface,sizeof(DWORD)*8);
			unzReadCurrentFile(file,m_Reg.SerialInterface,sizeof(DWORD)*4);
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			unzReadCurrentFile(file,_MMU->tlb,sizeof(TLB)*32);
			unzReadCurrentFile(file,_MMU->PIF_Ram,0x40);
#endif
			unzReadCurrentFile(file,_MMU->Rdram(),SaveRDRAMSize);
			unzReadCurrentFile(file,_MMU->Dmem(),0x1000);
			unzReadCurrentFile(file,_MMU->Imem(),0x1000);
			unzCloseCurrentFile(file);
			unzClose(file);
			LoadedZipFile = true;
		}
	}
	if (!LoadedZipFile) {
		HANDLE hSaveFile = CreateFile(FileNameStr.c_str(),GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,NULL,
			OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hSaveFile == INVALID_HANDLE_VALUE) {
			_Notify->DisplayMessage(5,"%s %s",GS(MSG_UNABLED_LOAD_STATE),FileNameStr.c_str());
			return false;
		}
		SetFilePointer(hSaveFile,0,NULL,FILE_BEGIN);	
		ReadFile( hSaveFile,&Value,sizeof(Value),&dwRead,NULL);
		if (Value != 0x23D8A6C8) { return FALSE; }
		ReadFile( hSaveFile,&SaveRDRAMSize,sizeof(SaveRDRAMSize),&dwRead,NULL);		
		//Check header
		BYTE LoadHeader[64];
		ReadFile( hSaveFile,LoadHeader,0x40,&dwRead,NULL);	
		if (memcmp(LoadHeader,_Rom->GetRomAddress(),0x40) != 0) {
			//if (inFullScreen) { return FALSE; }
			int result = MessageBox(NULL,GS(MSG_SAVE_STATE_HEADER),GS(MSG_MSGBOX_TITLE),
				MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
			if (result == IDNO) { return FALSE; }
		}
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		_MMU->UnProtectMemory(0x80000000,0x80000000 + _Settings->LoadDword(Game_RDRamSize) - 4);
		_MMU->UnProtectMemory(0xA4000000,0xA4001FFC);
		if (SaveRDRAMSize != _Settings->LoadDword(Game_RDRamSize)) {
			_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
			_MMU->FixRDramSize();
		}
#endif
		ReadFile( hSaveFile,&NextVITimer,sizeof(NextVITimer),&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.PROGRAM_COUNTER,sizeof(m_Reg.PROGRAM_COUNTER),&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.GPR,sizeof(_int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.FPR,sizeof(_int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.CP0,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.FPCR,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.HI,sizeof(_int64),&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.LO,sizeof(_int64),&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.RDRAM_Registers,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.SigProcessor_Interface,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.Display_ControlReg,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.Mips_Interface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.Video_Interface,sizeof(DWORD)*14,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.Audio_Interface,sizeof(DWORD)*6,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.Peripheral_Interface,sizeof(DWORD)*13,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.RDRAM_Interface,sizeof(DWORD)*8,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.SerialInterface,sizeof(DWORD)*4,&dwRead,NULL);
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		ReadFile( hSaveFile,_MMU->tlb,sizeof(TLB)*32,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->PIF_Ram,0x40,&dwRead,NULL);
#endif
		ReadFile( hSaveFile,_MMU->Rdram(),SaveRDRAMSize,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->Dmem(),0x1000,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->Imem(),0x1000,&dwRead,NULL);
		CloseHandle(hSaveFile);
	}

	//Fix Random Register
	while ((int)m_Reg.RANDOM_REGISTER < (int)m_Reg.WIRED_REGISTER) {
		m_Reg.RANDOM_REGISTER += 32 - m_Reg.WIRED_REGISTER;
	}
	WriteTrace(TraceDebug,"CN64System::LoadState 1");
	if (GetRecompiler())
	{
		GetRecompiler()->ResetRecompCode(); 
	}
	
	//Fix up timer
	m_Reg.CP0[32] = 0;
	WriteTrace(TraceDebug,"CN64System::LoadState 2");
	m_Reg.ResetTimer(NextVITimer);
	WriteTrace(TraceDebug,"CN64System::LoadState 3");
	m_Reg.ChangeTimerFixed(CompareTimer,m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER); 
	WriteTrace(TraceDebug,"CN64System::LoadState 4");
	m_Reg.FixFpuLocations();
	WriteTrace(TraceDebug,"CN64System::LoadState 5");
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	_MMU->TLB_Reset(false);
#endif
	WriteTrace(TraceDebug,"CN64System::LoadState 6");
	m_CPU_Usage.ResetCounters();
	WriteTrace(TraceDebug,"CN64System::LoadState 7");
	m_Profile.ResetCounters();
	WriteTrace(TraceDebug,"CN64System::LoadState 8");
	m_FPS.Reset(true);
	WriteTrace(TraceDebug,"CN64System::LoadState 9");
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;
	m_EventList.clear();
	m_NoOfEvents = m_EventList.size();
	WriteTrace(TraceDebug,"CN64System::LoadState 10");
	_Plugins->GameReset();
	WriteTrace(TraceDebug,"CN64System::LoadState 11");
	ResetX86Logs();
	WriteTrace(TraceDebug,"CN64System::LoadState 12");
	_Audio->ResetAudioSettings();
	_Audio->AiSetFrequency(m_Reg.AI_DACRATE_REG,m_SystemType);
	_Plugins->Audio()->DacrateChanged(m_SystemType);

#ifdef TEST_SP_TRACKING
	m_CurrentSP = GPR[29].UW[0];
#endif
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	_MMU->m_MemoryStack = (DWORD)(_MMU->Rdram() + (m_Reg.GPR[29].W[0] & 0x1FFFFFFF));
#endif	

	if (_Settings->LoadDword(Game_CpuType) == CPU_SyncCores) {
		if (m_SyncCPU)
		{
			for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) {
				m_LastSuccessSyncPC[i] = 0;
			}
			m_SyncCPU->LoadState(FileNameStr.c_str());
			SyncCPU(m_SyncCPU);
		}

//		if (m_Recomp->_Sync) {
//			m_Recomp->_Sync->LoadState();
//			m_Recomp->_Sync->_Plugins->Control()->SetControl(_Plugins->Control());
//			SyncCPU(m_Recomp->_Sync);
//			_Settings->Save(RamSize,SaveRDRAMSize);
//		}
	}
	WriteTrace(TraceDebug,"CN64System::LoadState 13");
	_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
	WriteTrace(TraceDebug,"CN64System::LoadState 14");
	stdstr LoadMsg = _Lang->GetString(MSG_LOADED_STATE);
	WriteTrace(TraceDebug,"CN64System::LoadState 15");
	_Notify->DisplayMessage(5,"%s %s",LoadMsg.c_str(),CPath(FileNameStr).GetNameExtension().c_str());
	WriteTrace(TraceDebug,"CN64System::LoadState 16");
	return true;
}

void CN64System::RunRSP ( void ) {
	WriteTraceF(TraceRSP, "RunRSP: SP Status %X",m_Reg.SP_STATUS_REG);
	if ( ( m_Reg.SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
		if ( ( m_Reg.SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {
			DWORD Task; _MMU->LW_VAddr(0xA4000FC0,Task);
			DWORD CPU_UsageAddr = Timer_None, ProfileAddr = Timer_None;
			
			if (Task == 1 && (m_Reg.DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0) 
			{
				WriteTrace(TraceRSP, "RunRSP: Dlist that is frozen");
				return;
			}
			
			switch (Task) {
			case 1:  
				WriteTrace(TraceRSP, "RunRSP: *** Display list ***");
				m_DlistCount   += 1; 
				m_FPS.UpdateDlCounter();
				break;
			case 2:  
				WriteTrace(TraceRSP, "RunRSP: *** Audio list ***");
				m_AlistCount   += 1; 
				break;
			default: 
				WriteTrace(TraceRSP, "RunRSP: *** Unknown list ***");
				m_UnknownCount += 1; 
				break;
			}
			if (bShowDListAListCount()) {				
				_Notify->DisplayMessage(0,"Dlist: %d   Alist: %d   Unknown: %d",m_DlistCount,m_AlistCount,m_UnknownCount);
			}
			if (bShowCPUPer()) {
				switch (Task) {
				case 1:  CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RSP_Dlist); break;
				case 2:  CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RSP_Alist); break;
				default: CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RSP_Unknown); break;
				}
			}
//			if (bProfiling) {
//				switch (Task) {
//				case 1:  ProfileAddr = m_Profile.StartTimer(Timer_RSP_Dlist); break;
//				case 2:  ProfileAddr = m_Profile.StartTimer(Timer_RSP_Alist); break;
//				default: ProfileAddr = m_Profile.StartTimer(Timer_RSP_Unknown); break;
//				}
//			}
			__try {
				WriteTrace(TraceRSP, "RunRSP: do cycles - starting");
				_Plugins->RSP()->DoRspCycles(100);
				WriteTrace(TraceRSP, "RunRSP: do cycles - Done");
			} __except( _MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
				WriteTrace(TraceError, "RunRSP: exception generated");
				_Notify->FatalError("Unknown memory action\n\nEmulation stop");
			}
			/*if (Task == 1 && _Settings->LoadDword(DelayDlists)) {
				m_Reg.ChangeTimerFixed(RSPTimerDlist,400);
				MI_INTR_REG   &= ~(MI_INTR_MASK_SP | MI_INTR_MASK_DP);
				SP_STATUS_REG &= ~SP_STATUS_SIG2;				
			}*/

			if (bShowCPUPer())  { m_CPU_Usage.StartTimer(CPU_UsageAddr); }
			//if (bProfiling) { m_Profile.StartTimer(ProfileAddr); }

			WriteTrace(TraceRSP, "RunRSP: check interrupts");
			m_Reg.CheckInterrupts();
		}
	}
}

void CN64System::SyncToAudio ( void ) {
	if (!bBasicMode() && !bLimitFPS() ) 
	{
		return;
	}
	if (!bFixedAudio()) 
	{
		return;
	}
	if (!bSyncToAudio())
	{
		return;
	}
//	if (m_Reg.GetCurrentTimerType() != AiTimer)
//	{
//		return;
//	}
	if (_Audio->AiGetLength()  == 0)
	{
		return;
	}
	DWORD (__cdecl* AiReadLength) ( void );
	AiReadLength = _Plugins->Audio()->ReadLength;
	if (AiReadLength() == 0) 
	{
		return;
	}

	DWORD CPU_UsageAddr = Timer_None;
	if (bShowCPUPer()) 
	{
		CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_Idel); 
	}
	while (!m_EndEmulation)
	{
		Sleep(10);
		if (AiReadLength() == 0) 
		{ 
			break; 
		}
	}
	if (bShowCPUPer()) 
	{
		m_CPU_Usage.StartTimer(CPU_UsageAddr != Timer_None ? CPU_UsageAddr : Timer_R4300 );
	}

}

void CN64System::RefreshScreen ( void ) {
	DWORD CPU_UsageAddr = Timer_None, ProfilingAddr = Timer_None;
	DWORD OLD_VI_V_SYNC_REG = 0, VI_INTR_TIME = 500000;
	
	if (bShowCPUPer()) { CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_RefreshScreen); }
	//if (bProfiling)    { ProfilingAddr = m_Profile.StartTimer(Timer_RefreshScreen); }

	//Calculate how many cycles to next refresh
	if (m_Reg.VI_V_SYNC_REG == 0) {
		VI_INTR_TIME = 500000;
	} else {
		VI_INTR_TIME = (m_Reg.VI_V_SYNC_REG + 1) * ViRefreshRate();
		if ((m_Reg.VI_V_SYNC_REG % 1) != 0) {
			VI_INTR_TIME -= 38;
		}
	}
	m_Reg.ChangeTimerRelative(ViTimer,VI_INTR_TIME); 
	if (g_FixedAudio)
	{
		_Audio->UpdateAudioTimer (VI_INTR_TIME);	
	}
	
//	_Plugins->Control()->UpdateKeys();
	if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateScreen); }
//	if (bProfiling)    { m_Profile.StartTimer(Timer_UpdateScreen); }
	
	__try
	{
		WriteTrace(TraceGfxPlugin,"UpdateScreen: Starting");
		_Plugins->Gfx()->UpdateScreen();
		WriteTrace(TraceGfxPlugin,"UpdateScreen: Done");
	} __except (_MMU->MemoryFilter( GetExceptionCode(), GetExceptionInformation())) {
		WriteTrace(TraceGfxPlugin,"UpdateScreen: Exception caught");
		WriteTrace(TraceError,"Exception caught in UpdateScreen");
	}

	if ((bBasicMode() || bLimitFPS() ) && !bSyncToAudio()) {
		if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_Idel); }
		DWORD FrameRate;
		if (m_Limitor.Timer_Process(&FrameRate) && bDisplayFrameRate()) {
			m_FPS.DisplayViCounter(FrameRate);
			m_bCleanFrameBox = true;
		}
	} else if (bDisplayFrameRate()) {
		if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateFPS); }
		m_FPS.UpdateViCounter();
		m_bCleanFrameBox = true;
	}
	
	if (m_bCleanFrameBox && !bDisplayFrameRate())
	{
		m_FPS.Reset (true);
		m_bCleanFrameBox = false;
	}

	if (bShowCPUPer()) {
		m_CPU_Usage.StopTimer();
		m_CPU_Usage.ShowCPU_Usage();
		m_CPU_Usage.StartTimer(CPU_UsageAddr != Timer_None ? CPU_UsageAddr : Timer_R4300 );
	}
	if ((m_Reg.STATUS_REGISTER & STATUS_IE) != 0 ) { ApplyCheats(); }
//	if (bProfiling)    { m_Profile.StartTimer(ProfilingAddr != Timer_None ? ProfilingAddr : Timer_R4300); }
}

bool CN64System::WriteToProtectedMemory (DWORD Address, int length)
{
	WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",Address,length);
	if (m_Recomp)
	{
		return m_Recomp->ClearRecompCode_Phys(Address,length,CRecompiler::Remove_ProtectedMem);
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
	if (m_Recomp && m_Recomp->bSMM_TLB())
	{
		m_Recomp->ClearRecompCode_Virt(VAddr,Len,CRecompiler::Remove_TLB);
	}
}

void CN64System::TLB_Changed   ( void )
{
	Debug_RefreshTLBWindow();
}

