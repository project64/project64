#include "..\N64 System.h"
#include "..\Plugin.h"
#include "..\3rd Party\Zip.h"
#include "C Core\c core.h"

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

#include <windows.h>

CN64System::CN64System ( CNotification * Notify, CPlugins * Plugins ):
	CDebugger(this,_MMU),_Notify(Notify),FPS(Notify),m_CPU_Usage(Notify),m_Profile(Notify), m_Limitor(Notify),
	_Plugins(Plugins),_Cheats(NULL),_SyncCPU(NULL),_Recomp(NULL),
	m_InReset(false), EndEmulation(false), m_OwnRomObject(false), _Audio(NULL),
	m_bCleanFrameBox(true)
{
	_Rom      = 0;    //No rom loaded
	_MMU      = 0;    //Since no rom can be loaded, then no system has been created
		
	CPU_Handle   = 0;
	CPU_ThreadID = 0;

	_Settings->RegisterChangeCB(Plugin_RSP_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_GFX_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_AUDIO_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_CONT_Current,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_UseHleGfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Plugin_UseHleAudio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_Plugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_Plugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_Plugin_Controller,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->RegisterChangeCB(Game_Plugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);

	//InterpreterOpcode = NULL;
	m_hPauseEvent = CreateEvent(NULL,true,false,NULL);
	Reset();
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
	_Settings->UnregisterChangeCB(Game_Plugin_Gfx,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_Plugin_Audio,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_Plugin_Controller,this,(CSettings::SettingChangedFunc)PluginChanged);
	_Settings->UnregisterChangeCB(Game_Plugin_RSP,this,(CSettings::SettingChangedFunc)PluginChanged);
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
		if (g_Notify)
		{
			g_Notify->RefreshMenu();
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
			_MMU = new CMipsMemory(this,this,_Rom,_Notify, new CRegisters(this, _Notify));
			_MMU->FixRDramSize();
			_Reg = _MMU->SystemRegisters();
			
			_Audio = new CAudio(_Reg);
			_Audio->ResetAudioSettings();
			_Audio->AiSetFrequency(_Reg->AI_DACRATE_REG,m_SystemType);

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

void CN64System::RunFileImage ( const char * FileLoc ) 
{
	bool RomLoading  = _Settings->LoadBool(GameRunning_LoadingInProgress);
	if (!RomLoading)
	{
		_Settings->SaveBool(GameRunning_LoadingInProgress,true);

		HANDLE  * hThread = new HANDLE;
		*hThread = NULL;

		//create the needed info into a structure to pass as one paramater
		//for createing a thread
		FileImageInfo * Info = new FileImageInfo;
		Info->_this        = this;
		Info->FileName     = FileLoc;
		Info->ThreadHandle = hThread;
		
		*hThread  = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)LoadFileImage,Info,0, &(Info->ThreadID));
	}
}

void CN64System::LoadFileImage (  FileImageInfo * Info ) 
{
	CoInitialize(NULL);

	//Make a copy of the passed information on to the stack, then free
	//the memory used
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 1");
	FileImageInfo ImageInfo = *Info;
	HANDLE ThreadHandle = *((HANDLE *)ImageInfo.ThreadHandle);
	delete (HANDLE *)ImageInfo.ThreadHandle;
	ImageInfo.ThreadHandle = &ThreadHandle; 
	CN64System * _this = ImageInfo._this;
	delete Info;
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 2");

	//Mark the rom as loading
	_Settings->SaveBool(GameRunning_LoadingInProgress,true);
	_this->_Notify->RefreshMenu();
		
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 3");
	//Try to load the passed N64 rom
	CN64Rom * Rom = new CN64Rom(_this->_Notify);
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 4");
	if (!Rom->LoadN64Image(ImageInfo.FileName.c_str())) {
		_this->_Notify->DisplayError(Rom->GetError());
		delete Rom; 
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_this->_Notify->RefreshMenu();
		return;
	}
	_this->CloseCpu();
	
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 5");
	Rom->SaveRomSettingID();

	if (_this->_Plugins)
	{
		_this->_Plugins->Reset();
	}

	/*{
		stdstr &Name = Rom->GetRomName();
		_Settings->SaveString(ROM_InternalName,Name.c_str());
	}*/
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 6");
	_this->_Notify->AddRecentRom(ImageInfo.FileName.c_str());
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 7");
	_this->SetupSystem(Rom,true);
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 8");
	_Settings->SaveBool(GameRunning_LoadingInProgress,false);
	_this->_Notify->RefreshMenu();
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 9");
	if (_Settings->LoadDword(Setting_AutoStart) != 0)
	{
		_this->CPU_Handle = (void *)(*((DWORD *)ImageInfo.ThreadHandle));
		_this->CPU_ThreadID = ImageInfo.ThreadID;
		
#ifndef _DEBUG
		try
		{
#endif
			_this->StartEmulation(false);
#ifndef _DEBUG
			
		} 
		catch (...)
		{
			WriteTraceF(TraceError,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
			char Message[600];
			sprintf(Message,"CN64System::LoadFileImage - Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
			MessageBox(NULL,Message,"Exception",MB_OK);
		}
#endif
	} else {
		delete ImageInfo.ThreadHandle;
		//Load rom settings
		_this->_Notify->DisplayMessage(5,MSG_WAITING_FOR_START);
	}
	WriteTrace(TraceDebug,"CN64System::LoadFileImage 10");
	CoUninitialize();
}	
	
void  CN64System::StartEmulation2   ( bool NewThread )
{
	WriteTrace(TraceDebug,"CN64System::StartEmulation 1");
	if (NewThread)
	{
		FileImageInfo * Info = new FileImageInfo;
		HANDLE  * hThread = new HANDLE;
		*hThread = NULL;

		//create the needed info into a structure to pass as one paramater
		//for createing a thread
		Info->_this        = this;
		Info->ThreadHandle = hThread;
		
		DWORD ThreadID;
		*hThread  = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartEmulationThread,Info,0, &ThreadID);	
		return;
	}

	WriteTrace(TraceDebug,"CN64System::StartEmulation 2");
	_Notify->HideRomBrowser();
	WriteTrace(TraceDebug,"CN64System::StartEmulation 3");

	//RefreshSettings();
	WriteTrace(TraceDebug,"CN64System::StartEmulation 4");
	m_Limitor.SetHertz(_Settings->LoadDword(Game_ScreenHertz));
	WriteTrace(TraceDebug,"CN64System::StartEmulation 5");
	_MMU->FixRDramSize();

	_Notify->SetWindowCaption(_Settings->LoadString(Game_GoodName).c_str());

	WriteTrace(TraceDebug,"CN64System::StartEmulation 6");
	_Notify->DisplayMessage(5,MSG_PLUGIN_INIT);
	WriteTrace(TraceDebug,"CN64System::StartEmulation 7");
//	_Plugins->Reset();
	WriteTrace(TraceDebug,"CN64System::StartEmulation 8");
	if (!_Plugins->Initiate(this)) {
		WriteTrace(TraceDebug,"CN64System::StartEmulation 8a");
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->DisplayError(MSG_PLUGIN_NOT_INIT);
		//Set handle to NULL so this thread is not terminated
		CPU_Handle = NULL;
		CPU_ThreadID = 0;

	//	Reset();
		_Notify->RefreshMenu();
		_Notify->ShowRomBrowser();
		return;
	}

	WriteTrace(TraceDebug,"CN64System::StartEmulation 9");
	_Cheats = new CCheats(_Rom,_Notify);
	WriteTrace(TraceDebug,"CN64System::StartEmulation 10");
	_Cheats->LoadCheats(!_Settings->LoadDword(Setting_RememberCheats));

	WriteTrace(TraceDebug,"CN64System::StartEmulation 11");
	_Notify->DisplayMessage(5,"Done");
	WriteTrace(TraceDebug,"CN64System::StartEmulation 12");
	_Notify->MakeWindowOnTop(_Settings->LoadBool(UserInterface_AlwaysOnTop));

	WriteTrace(TraceDebug,"CN64System::StartEmulation 13");
	if (!_Settings->LoadBool(Beta_IsValidExe))
	{
		Reset();
		return;
	}
	WriteTrace(TraceDebug,"CN64System::StartEmulation 14");
	//mark the emulation as starting and fix up menus
	_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);
	WriteTrace(TraceDebug,"CN64System::StartEmulation 15");
	if (_Settings->LoadBool(Setting_AutoFullscreen)) 
	{
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
	} __except( _MMU->SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
		char Message[600];
		sprintf(Message,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
		MessageBox(NULL,Message,"Exception",MB_OK);
	}
}

void CN64System::StartEmulationThread (  FileImageInfo * Info ) {
	CoInitialize(NULL);

	//Make a copy of the passed infomation on to the stack, then free
	//the memory used
	FileImageInfo ImageInfo = *Info;
	CN64System * _this = ImageInfo._this;
	delete Info;

	_this->CPU_Handle = (void *)(*((DWORD *)ImageInfo.ThreadHandle));
	_this->CPU_ThreadID = ImageInfo.ThreadID;
	delete ImageInfo.ThreadHandle;
	
	_this->StartEmulation(false);
	
	CoUninitialize();
}

void CN64System::CloseCpu ( void ) {
	if (CPU_Handle == NULL) { return; }
	Debug_Reset();

	if (_Settings->LoadBool(GameRunning_CPU_Paused))
	{
		SetEvent(m_hPauseEvent);
	}
	
	void * lCPU_Handle = CPU_Handle;
	if (GetCurrentThreadId() == CPU_ThreadID)
	{
		Close_C_CPU();
		return;
	}
	Close_C_CPU();
	for (int count = 0; count < 40 && (lCPU_Handle == CPU_Handle); count ++ ) {
		Sleep(500);
		if (_Notify->ProcessGuiMessages())
		{
			return;
		}
	
		DWORD ExitCode;
		if (GetExitCodeThread(lCPU_Handle,&ExitCode))
		{
			if (ExitCode != STILL_ACTIVE) {
				if (lCPU_Handle == CPU_Handle)
				{
					CPU_Handle = NULL;
				}
				break;
			}
		}
	}
	if (lCPU_Handle != CPU_Handle) { return; }
#ifdef _DEBUG
	_Notify->DisplayError("Terminate Thread");
#endif
	TerminateThread(CPU_Handle,0); 
	CPU_Handle = NULL;
	CPU_ThreadID = 0;
	CpuStopped();
}

void CN64System::SelectCheats ( WND_HANDLE hParent ) {
	if (!_Cheats) { return; }
	
	_Cheats->SelectCheats(hParent,false);
}

void CN64System::DisplayRomInfo ( WND_HANDLE hParent ) {
	if (!_Rom) { return; }
	
	RomInformation Info(_Rom,_Notify);
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
	if (_Cheats)
	{
		if (_Cheats->IsCheatMessage(msg))
		{
			return true;
		}
	}
	return false;
}

void  CN64System::SoftReset()
{
	_MMU->InitalizeSystem(true);
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
	if (_Rom && m_OwnRomObject)
	{ 
		delete _Rom;  //Delete Current Open Rom class
		_Rom = 0; 
	} 
	if (_Audio)
	{
		delete _Audio;
		_Audio = NULL;
	}
	Debug_Reset();
	CloseSaveChips();
	if (_MMU)     { delete _MMU; _MMU = 0; } //Delete System Memory class
	if (_Cheats)  { delete _Cheats; _Cheats = 0; }
	_Reg           = 0;
	m_CyclesToSkip = 0;
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;
	m_OwnRomObject = false;
	m_SystemType   = SYSTEM_NTSC;
}

void CN64System::SetupSystem (  CN64Rom * Rom, bool OwnRomObject, bool SavesReadOnly ) {
	//stop CPU and destroy current MMU
	Reset();
	
	m_OwnRomObject = OwnRomObject;
	_Rom = Rom;	

	//Create New system
	_MMU   = new CMipsMemory(this,this,_Rom,_Notify,new CRegisters(this, _Notify),SavesReadOnly);
	_Reg   = _MMU->SystemRegisters();	

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
	_Audio = new CAudio(_Reg);
	_Audio->ResetAudioSettings();
	_Audio->AiSetFrequency(_Reg->AI_DACRATE_REG,m_SystemType);

	
}

void CN64System::ExecuteCPU ( void ) 
{
	_Settings->SaveBool(GameRunning_CPU_Running,true);
	_Settings->SaveBool(GameRunning_CPU_Paused,false);
	_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);
	
	EndEmulation = false;
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
}

void CN64System::ExecuteInterpret (CC_Core & C_Core) {
	C_Core.SetN64System(this);
	InitializeCPUCore();
	ExecuteCycles(-1);
}

void CN64System::ExecuteRecompiler (CC_Core & C_Core) {	
	//execute opcodes while no errors	
	CRecompiler Recompiler(_MMU,m_Profile,EndEmulation,false);
	_Recomp = &Recompiler;
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
	_SyncCPU = &SyncSystem;
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

		CRecompiler Recompiler(_MMU,m_Profile,EndEmulation,true);
		_Recomp = &Recompiler;
		C_Core.SetN64System(this);
		InitializeCPUCore();
		Recompiler.Run();
		SyncSystem._Rom = NULL;
	}*/
}

void CN64System::CpuStopped ( void ) {
	void * lCPU_Handle = CPU_Handle;
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
	if (lCPU_Handle == CPU_Handle)
	{
		CPU_ThreadID = 0;
		CPU_Handle   = 0;
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
	if (_Reg->PROGRAM_COUNTER != SecondCPU->_Reg->PROGRAM_COUNTER) {
		ErrorFound = true;
	}
	for (int count = 0; count < 32; count ++) {
		if (_Reg->GPR[count].DW != SecondCPU->_Reg->GPR[count].DW) {
			ErrorFound = true;
		}
		if (_Reg->FPR[count].DW != SecondCPU->_Reg->FPR[count].DW) {
			ErrorFound = true;
		}
		if (_Reg->CP0[count] != SecondCPU->_Reg->CP0[count]) {
			ErrorFound = true;
		}
	}

	for (int z = 0; z < 0x100; z++)
	{	
		if (_MMU->RDRAM[0x00325044 + z] !=  SecondCPU->_MMU->RDRAM[0x00325044 + z]) 
		{
			ErrorFound = true;
		}
	}
	
	if (bSPHack()) 
	{
		if (_MMU->m_MemoryStack != (DWORD)(_MMU->RDRAM + (_Reg->GPR[29].W[0] & 0x1FFFFFFF)))
		{
			ErrorFound = true;
		}
	}

	if (_Reg->GetCurrentTimer()     != SecondCPU->_Reg->GetCurrentTimer()) { ErrorFound = true; }
	if (_Reg->GetCurrentTimerType() != SecondCPU->_Reg->GetCurrentTimerType()) { ErrorFound = true; }
	
	if (ErrorFound) { DumpSyncErrors(SecondCPU); }

	for (int i = (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--) {
		m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
	}
	m_LastSuccessSyncPC[0] = _Reg->PROGRAM_COUNTER;
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
		if (_Reg->PROGRAM_COUNTER != SecondCPU->_Reg->PROGRAM_COUNTER) {
			Error.LogF("PROGRAM_COUNTER, 0x%X,         0x%X\r\n",_Reg->PROGRAM_COUNTER,SecondCPU->_Reg->PROGRAM_COUNTER);
		}
		for (count = 0; count < 32; count ++) {
			if (_Reg->GPR[count].DW != SecondCPU->_Reg->GPR[count].DW) {
				Error.LogF("GPR[%s] Different,0x%08X%08X, 0x%08X%08X\r\n",_Reg->GPR_Name[count],
					_Reg->GPR[count].W[1],_Reg->GPR[count].W[0],
					SecondCPU->_Reg->GPR[count].W[1],SecondCPU->_Reg->GPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (_Reg->FPR[count].DW != SecondCPU->_Reg->FPR[count].DW) {
				Error.LogF("FPR[%s] Different,0x%08X%08X, 0x%08X%08X\r\n",_Reg->FPR_Name[count],
					_Reg->FPR[count].W[1],_Reg->FPR[count].W[0],
					SecondCPU->_Reg->FPR[count].W[1],SecondCPU->_Reg->FPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (_Reg->CP0[count] != SecondCPU->_Reg->CP0[count]) {
				Error.LogF("CP0[%s] Different,0x%08X, 0x%08X\r\n",_Reg->Cop0_Name[count],
					_Reg->CP0[count],	SecondCPU->_Reg->CP0[count]);
			}
		}	
		if (_Reg->GetCurrentTimer()     != SecondCPU->_Reg->GetCurrentTimer()) 
		{ 
			Error.LogF("Current Time is Different: %X %X\r\n",(DWORD)_Reg->GetCurrentTimer(),(DWORD)SecondCPU->_Reg->GetCurrentTimer());
		}
		if (_Reg->GetCurrentTimerType() != SecondCPU->_Reg->GetCurrentTimerType()) 
		{ 
			Error.LogF("Current Time Type is Different: %X %X\r\n",_Reg->GetCurrentTimerType(),SecondCPU->_Reg->GetCurrentTimerType());
		}
		if (_Settings->LoadDword(Game_SPHack)) 
		{
			if (_MMU->m_MemoryStack != (DWORD)(_MMU->RDRAM + (_Reg->GPR[29].W[0] & 0x1FFFFFFF)))
			{
				Error.LogF("MemoryStack = %X  should be: %X\r\n",_MMU->m_MemoryStack, (DWORD)(_MMU->RDRAM + (_Reg->GPR[29].W[0] & 0x1FFFFFFF)));
			}
		}
		Error.Log("\r\n");
		Error.Log("Information:\r\n");
		Error.Log("\r\n");
		Error.LogF("PROGRAM_COUNTER,0x%X\r\n",_Reg->PROGRAM_COUNTER);
		Error.LogF("Current Timer,0x%X\r\n",_Reg->GetCurrentTimer());
		Error.LogF("Timer Type,0x%X\r\n",_Reg->GetCurrentTimerType());
		Error.Log("\r\n");
		for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) {
			Error.LogF("LastSuccessSyncPC[%d],0x%X\r\n",i,m_LastSuccessSyncPC[i]);
		}
		Error.Log("");
		for (count = 0; count < 32; count ++) {
			Error.LogF("GPR[%s],         0x%08X%08X, 0x%08X%08X\r\n",_Reg->GPR_Name[count],
				_Reg->GPR[count].W[1],_Reg->GPR[count].W[0],
				SecondCPU->_Reg->GPR[count].W[1],SecondCPU->_Reg->GPR[count].W[0]);
		}	
		Error.Log("");
		for (count = 0; count < 32; count ++) {
			Error.LogF("CP0[%s],%*s0x%08X, 0x%08X\r\n",_Reg->Cop0_Name[count],
				12 - strlen(_Reg->Cop0_Name[count]),"",
				_Reg->CP0[count],SecondCPU->_Reg->CP0[count]);
		}	
		Error.Log("\r\n");
		Error.Log("         Hi Recomp, PageMask, Hi Interp, PageMask\r\n");
		for (count = 0; count < 32; count ++) {
			if (!_MMU->tlb[count].EntryDefined) { continue; }
			Error.LogF("TLB[%2d], %08X,  %08X, %08X,  %08X\r\n", count,
				_MMU->tlb[count].EntryHi.Value,_MMU->tlb[count].PageMask.Value,
				SecondCPU->_MMU->tlb[count].EntryHi.Value,SecondCPU->_MMU->tlb[count].PageMask.Value
			);	
		}
		Error.Log("\r\n");
		Error.LogF("Current Fiber,%X\r\n",GetCurrentFiber());
		Error.Log("\r\n");
		Error.Log("Code at PC:\r\n");
		COpcode Op(SecondCPU->_MMU,SecondCPU->_Reg->PROGRAM_COUNTER - (OpCode_Size * 10));
		for (;Op.PC() < SecondCPU->_Reg->PROGRAM_COUNTER + (OpCode_Size * 10); Op.Next()) {
			Error.LogF("%X,%s\r\n",Op.PC(),Op.Name().c_str());
		}
		Error.Log("\r\n");
		Error.Log("Code at Last Sync PC:\r\n");
		for (Op.SetPC(m_LastSuccessSyncPC[0]); Op.PC() < m_LastSuccessSyncPC[0] + (OpCode_Size * 50); Op.Next()) {
			Error.LogF("%X,%s\r\n",Op.PC(),Op.Name().c_str());
		}
	}

	_Notify->DisplayError("Sync Error");
	_Notify->BreakPoint(__FILE__,__LINE__);
//	AddEvent(CloseCPU);
}

bool CN64System::SaveState(void) 
{
	WriteTrace(TraceDebug,"CN64System::SaveState 1");

	if (_Reg->GetTimer(AiTimerDMA)     != 0)  { return false; }
	if (_Reg->GetTimer(SiTimer)        != 0)  { return false; }
	if (_Reg->GetTimer(PiTimer)        != 0)  { return false; }
	if (_Reg->GetTimer(RSPTimerDlist)  != 0)  { return false; }
	if ((_Reg->STATUS_REGISTER & STATUS_EXL) != 0) { return false; }
	
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
	DWORD NextViTimer = _Reg->GetTimer(ViTimer);
	DWORD MiInterReg  = _Reg->MI_INTR_REG;
	if (_Reg->GetTimer(AiTimer) != 0) { _Reg->MI_INTR_REG |= MI_INTR_AI; }
	if (_Settings->LoadDword(Setting_AutoZipInstantSave)) {
		zipFile			file;

		file = zipOpen(FileName.c_str(),0);
		zipOpenNewFileInZip(file,CurrentSaveName.c_str(),NULL,NULL,0,NULL,0,NULL,Z_DEFLATED,Z_DEFAULT_COMPRESSION);
		zipWriteInFileInZip(file,&SaveID_0,sizeof(SaveID_0));
		zipWriteInFileInZip(file,&RdramSize,sizeof(DWORD));
		zipWriteInFileInZip(file,_MMU->ROM,0x40);	
		zipWriteInFileInZip(file,&NextViTimer,sizeof(DWORD));
		zipWriteInFileInZip(file,&_Reg->PROGRAM_COUNTER,sizeof(_Reg->PROGRAM_COUNTER));
		zipWriteInFileInZip(file,_Reg->GPR,sizeof(_int64)*32);
		zipWriteInFileInZip(file,_Reg->FPR,sizeof(_int64)*32);
		zipWriteInFileInZip(file,_Reg->CP0,sizeof(DWORD)*32);
		zipWriteInFileInZip(file,_Reg->FPCR,sizeof(DWORD)*32);
		zipWriteInFileInZip(file,&_Reg->HI,sizeof(_int64));
		zipWriteInFileInZip(file,&_Reg->LO,sizeof(_int64));
		zipWriteInFileInZip(file,_Reg->RDRAM_Registers,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,_Reg->SigProcessor_Interface,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,_Reg->Display_ControlReg,sizeof(DWORD)*10);
		zipWriteInFileInZip(file,_Reg->Mips_Interface,sizeof(DWORD)*4);
		zipWriteInFileInZip(file,_Reg->Video_Interface,sizeof(DWORD)*14);
		zipWriteInFileInZip(file,_Reg->Audio_Interface,sizeof(DWORD)*6);
		zipWriteInFileInZip(file,_Reg->Peripheral_Interface,sizeof(DWORD)*13);
		zipWriteInFileInZip(file,_Reg->RDRAM_Interface,sizeof(DWORD)*8);
		zipWriteInFileInZip(file,_Reg->SerialInterface,sizeof(DWORD)*4);
		zipWriteInFileInZip(file,_MMU->tlb,sizeof(TLB)*32);
		zipWriteInFileInZip(file,_MMU->PIF_Ram,0x40);
		zipWriteInFileInZip(file,_MMU->RDRAM,RdramSize);
		zipWriteInFileInZip(file,_MMU->DMEM,0x1000);
		zipWriteInFileInZip(file,_MMU->IMEM,0x1000);
		zipCloseFileInZip(file);
		zipClose(file,"");
	} else {
		HANDLE hSaveFile = CreateFile(FileName.c_str(),GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
			NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hSaveFile == INVALID_HANDLE_VALUE) {
			_Notify->DisplayError(GS(MSG_FAIL_OPEN_SAVE));
			_Reg->MI_INTR_REG = MiInterReg;
			return true;
		}

		//Write info to file
		SetFilePointer(hSaveFile,0,NULL,FILE_BEGIN);	
		WriteFile( hSaveFile,&SaveID_0,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,&RdramSize,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->ROM,0x40,&dwWritten,NULL);	
		WriteFile( hSaveFile,&NextViTimer,sizeof(DWORD),&dwWritten,NULL);
		WriteFile( hSaveFile,&_Reg->PROGRAM_COUNTER,sizeof(_Reg->PROGRAM_COUNTER),&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->GPR,sizeof(_int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->FPR,sizeof(_int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->CP0,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->FPCR,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,&_Reg->HI,sizeof(_int64),&dwWritten,NULL);
		WriteFile( hSaveFile,&_Reg->LO,sizeof(_int64),&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->RDRAM_Registers,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->SigProcessor_Interface,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->Display_ControlReg,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->Mips_Interface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->Video_Interface,sizeof(DWORD)*14,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->Audio_Interface,sizeof(DWORD)*6,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->Peripheral_Interface,sizeof(DWORD)*13,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->RDRAM_Interface,sizeof(DWORD)*8,&dwWritten,NULL);
		WriteFile( hSaveFile,_Reg->SerialInterface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->tlb,sizeof(TLB)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->PIF_Ram,0x40,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->RDRAM,RdramSize,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->DMEM,0x1000,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->IMEM,0x1000,&dwWritten,NULL);

		CloseHandle(hSaveFile);
	}
	_Reg->MI_INTR_REG = MiInterReg;
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
			if (memcmp(LoadHeader,_MMU->ROM,0x40) != 0) {
				//if (inFullScreen) { return FALSE; }
				int result = MessageBox(NULL,GS(MSG_SAVE_STATE_HEADER),GS(MSG_MSGBOX_TITLE),
					MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
				if (result == IDNO) { return FALSE; }
			}
			_MMU->UnProtectMemory(0x80000000,0x80000000 + _Settings->LoadDword(Game_RDRamSize) - 4);
			_MMU->UnProtectMemory(0xA4000000,0xA4001FFC);
			if (SaveRDRAMSize != _Settings->LoadDword(Game_RDRamSize)) {
				_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
				_MMU->FixRDramSize();
			}
			unzReadCurrentFile(file,&NextVITimer,sizeof(NextVITimer));
			unzReadCurrentFile(file,&_Reg->PROGRAM_COUNTER,sizeof(_Reg->PROGRAM_COUNTER));
			unzReadCurrentFile(file,_Reg->GPR,sizeof(_int64)*32);
			unzReadCurrentFile(file,_Reg->FPR,sizeof(_int64)*32);
			unzReadCurrentFile(file,_Reg->CP0,sizeof(DWORD)*32);
			unzReadCurrentFile(file,_Reg->FPCR,sizeof(DWORD)*32);
			unzReadCurrentFile(file,&_Reg->HI,sizeof(_int64));
			unzReadCurrentFile(file,&_Reg->LO,sizeof(_int64));
			unzReadCurrentFile(file,_Reg->RDRAM_Registers,sizeof(DWORD)*10);
			unzReadCurrentFile(file,_Reg->SigProcessor_Interface,sizeof(DWORD)*10);
			unzReadCurrentFile(file,_Reg->Display_ControlReg,sizeof(DWORD)*10);
			unzReadCurrentFile(file,_Reg->Mips_Interface,sizeof(DWORD)*4);
			unzReadCurrentFile(file,_Reg->Video_Interface,sizeof(DWORD)*14);
			unzReadCurrentFile(file,_Reg->Audio_Interface,sizeof(DWORD)*6);
			unzReadCurrentFile(file,_Reg->Peripheral_Interface,sizeof(DWORD)*13);
			unzReadCurrentFile(file,_Reg->RDRAM_Interface,sizeof(DWORD)*8);
			unzReadCurrentFile(file,_Reg->SerialInterface,sizeof(DWORD)*4);
			unzReadCurrentFile(file,_MMU->tlb,sizeof(TLB)*32);
			unzReadCurrentFile(file,_MMU->PIF_Ram,0x40);
			unzReadCurrentFile(file,_MMU->RDRAM,SaveRDRAMSize);
			unzReadCurrentFile(file,_MMU->DMEM,0x1000);
			unzReadCurrentFile(file,_MMU->IMEM,0x1000);
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
		if (memcmp(LoadHeader,_MMU->ROM,0x40) != 0) {
			//if (inFullScreen) { return FALSE; }
			int result = MessageBox(NULL,GS(MSG_SAVE_STATE_HEADER),GS(MSG_MSGBOX_TITLE),
				MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2);
			if (result == IDNO) { return FALSE; }
		}
		_MMU->UnProtectMemory(0x80000000,0x80000000 + _Settings->LoadDword(Game_RDRamSize) - 4);
		_MMU->UnProtectMemory(0xA4000000,0xA4001FFC);
		if (SaveRDRAMSize != _Settings->LoadDword(Game_RDRamSize)) {
			_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
			_MMU->FixRDramSize();
		}
		ReadFile( hSaveFile,&NextVITimer,sizeof(NextVITimer),&dwRead,NULL);
		ReadFile( hSaveFile,&_Reg->PROGRAM_COUNTER,sizeof(_Reg->PROGRAM_COUNTER),&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->GPR,sizeof(_int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->FPR,sizeof(_int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->CP0,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->FPCR,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,&_Reg->HI,sizeof(_int64),&dwRead,NULL);
		ReadFile( hSaveFile,&_Reg->LO,sizeof(_int64),&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->RDRAM_Registers,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->SigProcessor_Interface,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->Display_ControlReg,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->Mips_Interface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->Video_Interface,sizeof(DWORD)*14,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->Audio_Interface,sizeof(DWORD)*6,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->Peripheral_Interface,sizeof(DWORD)*13,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->RDRAM_Interface,sizeof(DWORD)*8,&dwRead,NULL);
		ReadFile( hSaveFile,_Reg->SerialInterface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->tlb,sizeof(TLB)*32,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->PIF_Ram,0x40,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->RDRAM,SaveRDRAMSize,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->DMEM,0x1000,&dwRead,NULL);
		ReadFile( hSaveFile,_MMU->IMEM,0x1000,&dwRead,NULL);
		CloseHandle(hSaveFile);
	}

	//Fix Random Register
	while ((int)_Reg->RANDOM_REGISTER < (int)_Reg->WIRED_REGISTER) {
		_Reg->RANDOM_REGISTER += 32 - _Reg->WIRED_REGISTER;
	}
	WriteTrace(TraceDebug,"CN64System::LoadState 1");
	if (GetRecompiler())
	{
		GetRecompiler()->ResetRecompCode(); 
	}
	
	//Fix up timer
	_Reg->CP0[32] = 0;
	WriteTrace(TraceDebug,"CN64System::LoadState 2");
	_Reg->ResetTimer(NextVITimer);
	WriteTrace(TraceDebug,"CN64System::LoadState 3");
	_Reg->ChangeTimerFixed(CompareTimer,_Reg->COMPARE_REGISTER - _Reg->COUNT_REGISTER); 
	WriteTrace(TraceDebug,"CN64System::LoadState 4");
	_Reg->FixFpuLocations();
	WriteTrace(TraceDebug,"CN64System::LoadState 5");
	_MMU->TLB_Reset(false);
	WriteTrace(TraceDebug,"CN64System::LoadState 6");
	m_CPU_Usage.ResetCounters();
	WriteTrace(TraceDebug,"CN64System::LoadState 7");
	m_Profile.ResetCounters();
	WriteTrace(TraceDebug,"CN64System::LoadState 8");
	FPS.Reset(true);
	WriteTrace(TraceDebug,"CN64System::LoadState 9");
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;
	EventList.clear();
	NoOfEvents = EventList.size();
	WriteTrace(TraceDebug,"CN64System::LoadState 10");
	_Plugins->GameReset();
	WriteTrace(TraceDebug,"CN64System::LoadState 11");
	ResetX86Logs();
	WriteTrace(TraceDebug,"CN64System::LoadState 12");
	_Audio->ResetAudioSettings();
	_Audio->AiSetFrequency(_Reg->AI_DACRATE_REG,m_SystemType);
	_Plugins->Audio()->DacrateChanged(m_SystemType);

#ifdef TEST_SP_TRACKING
	m_CurrentSP = GPR[29].UW[0];
#endif
	_MMU->m_MemoryStack = (DWORD)(_MMU->RDRAM + (_Reg->GPR[29].W[0] & 0x1FFFFFFF));
	
	if (_Settings->LoadDword(Game_CpuType) == CPU_SyncCores) {
		if (_SyncCPU)
		{
			for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) {
				m_LastSuccessSyncPC[i] = 0;
			}
			_SyncCPU->LoadState(FileNameStr.c_str());
			SyncCPU(_SyncCPU);
		}

//		if (_Recomp->_Sync) {
//			_Recomp->_Sync->LoadState();
//			_Recomp->_Sync->_Plugins->Control()->SetControl(_Plugins->Control());
//			SyncCPU(_Recomp->_Sync);
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
	WriteTraceF(TraceRSP, "RunRSP: SP Status %X",_Reg->SP_STATUS_REG);
	if ( ( _Reg->SP_STATUS_REG & SP_STATUS_HALT ) == 0) {
		if ( ( _Reg->SP_STATUS_REG & SP_STATUS_BROKE ) == 0 ) {
			DWORD Task; _MMU->Load32(0xA4000FC0,Task,_32Bit,false);
			DWORD CPU_UsageAddr = Timer_None, ProfileAddr = Timer_None;
			
			if (Task == 1 && (_Reg->DPC_STATUS_REG & DPC_STATUS_FREEZE) != 0) 
			{
				WriteTrace(TraceRSP, "RunRSP: Dlist that is frozen");
				return;
			}
			
			switch (Task) {
			case 1:  
				WriteTrace(TraceRSP, "RunRSP: *** Display list ***");
				m_DlistCount   += 1; 
				FPS.UpdateDlCounter();
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
			} __except( _MMU->SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation()) ) {
				WriteTrace(TraceError, "RunRSP: exception generated");
				_Notify->FatalError("Unknown memory action\n\nEmulation stop");
			}
			/*if (Task == 1 && _Settings->LoadDword(DelayDlists)) {
				_Reg->ChangeTimerFixed(RSPTimerDlist,400);
				MI_INTR_REG   &= ~(MI_INTR_MASK_SP | MI_INTR_MASK_DP);
				SP_STATUS_REG &= ~SP_STATUS_SIG2;				
			}*/

			if (bShowCPUPer())  { m_CPU_Usage.StartTimer(CPU_UsageAddr); }
			//if (bProfiling) { m_Profile.StartTimer(ProfileAddr); }

			WriteTrace(TraceRSP, "RunRSP: check interrupts");
			_Reg->CheckInterrupts();
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
//	if (_Reg->GetCurrentTimerType() != AiTimer)
//	{
//		return;
//	}
	if (CAudio::AiGetLength(_Audio)  == 0)
	{
		return;
	}
	DWORD (__cdecl* AiReadLength) ( void );
	AiReadLength = g_Plugins->Audio()->ReadLength;
	if (AiReadLength() == 0) 
	{
		return;
	}

	DWORD CPU_UsageAddr = Timer_None;
	if (bShowCPUPer()) 
	{
		CPU_UsageAddr = m_CPU_Usage.StartTimer(Timer_Idel); 
	}
	while (!EndEmulation)
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
	if (_Reg->VI_V_SYNC_REG == 0) {
		VI_INTR_TIME = 500000;
	} else {
		VI_INTR_TIME = (_Reg->VI_V_SYNC_REG + 1) * ViRefreshRate();
		if ((_Reg->VI_V_SYNC_REG % 1) != 0) {
			VI_INTR_TIME -= 38;
		}
	}
	_Reg->ChangeTimerRelative(ViTimer,VI_INTR_TIME); 
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
	} __except (_MMU->SystemMemoryFilter( GetExceptionCode(), GetExceptionInformation())) {
		WriteTrace(TraceGfxPlugin,"UpdateScreen: Exception caught");
		WriteTrace(TraceError,"Exception caught in UpdateScreen");
	}

	if ((bBasicMode() || bLimitFPS() ) && !bSyncToAudio()) {
		if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_Idel); }
		DWORD FrameRate;
		if (m_Limitor.Timer_Process(&FrameRate) && bDisplayFrameRate()) {
			FPS.DisplayViCounter(FrameRate);
			m_bCleanFrameBox = true;
		}
	} else if (bDisplayFrameRate()) {
		if (bShowCPUPer()) { m_CPU_Usage.StartTimer(Timer_UpdateFPS); }
		FPS.UpdateViCounter();
		m_bCleanFrameBox = true;
	}
	
	if (m_bCleanFrameBox && !bDisplayFrameRate())
	{
		FPS.Reset (true);
		m_bCleanFrameBox = false;
	}

	if (bShowCPUPer()) {
		m_CPU_Usage.StopTimer();
		m_CPU_Usage.ShowCPU_Usage();
		m_CPU_Usage.StartTimer(CPU_UsageAddr != Timer_None ? CPU_UsageAddr : Timer_R4300 );
	}
	if ((_Reg->STATUS_REGISTER & STATUS_IE) != 0 ) { ApplyCheats(); }
//	if (bProfiling)    { m_Profile.StartTimer(ProfilingAddr != Timer_None ? ProfilingAddr : Timer_R4300); }
}

bool CN64System::WriteToProtectedMemory (DWORD Address, int length)
{
	WriteTraceF(TraceDebug,"WriteToProtectedMemory Addres: %X Len: %d",Address,length);
	if (_Recomp)
	{
		return _Recomp->ClearRecompCode_Phys(Address,length,CRecompiler::Remove_ProtectedMem);
	}
	return false;
}

void CN64System::TLB_Changed   ( void )
{
	Debug_RefreshTLBWindow();
}

void CN64System::TLB_Unmapping ( int TlbEntry, int FastTlbEntry, DWORD Vaddr, DWORD Len )
{
	if (_Recomp && _Recomp->bSMM_TLB())
	{
		_Recomp->ClearRecompCode_Virt(Vaddr,Len,CRecompiler::Remove_TLB);
	}
}
