#include "stdafx.h"

#pragma warning(disable:4355) // Disable 'this' : used in base member initializer list

#include <windows.h>

void InitializeCPUCore ( void ) 
{
	switch (_Rom->GetCountry())
	{
		case Germany: case french:  case Italian:
		case Europe:  case Spanish: case Australia:
		case X_PAL:   case Y_PAL:
			Timer_Initialize(50);
			g_SystemType = SYSTEM_PAL;
			break;
		default:
			Timer_Initialize(60);
			g_SystemType = SYSTEM_NTSC;
			break;
	}
#ifndef EXTERNAL_RELEASE
	LogOptions.GenerateLog = _Settings->LoadDword(Debugger_GenerateDebugLog);
	LoadLogOptions(&LogOptions, FALSE);
	StartLog();
#endif
}

CN64System::CN64System ( CPlugins * Plugins, bool SavesReadOnly ) :
	m_MMU_VM(this,SavesReadOnly),
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
	m_bInitilized(false),
	m_SystemTimer(m_NextTimer),
	m_DMAUsed(false),
	m_CPU_Handle(NULL),
	m_CPU_ThreadID(0)
{
	m_hPauseEvent = CreateEvent(NULL,true,false,NULL);
	m_Limitor.SetHertz(_Settings->LoadDword(Game_ScreenHertz));
	m_Cheats.LoadCheats(!_Settings->LoadDword(Setting_RememberCheats));
}

CN64System::~CN64System ( void ) {
}

void CN64System::ExternalEvent ( SystemEvent action ) 
{
	switch (action) {
	case SysEvent_Profile_GenerateLogs: 
	case SysEvent_Profile_StartStop: 
	case SysEvent_Profile_ResetLogs: 
	case SysEvent_ExecuteInterrupt: 
	case SysEvent_SaveMachineState: 
	case SysEvent_LoadMachineState: 
	case SysEvent_ChangePlugins: 
	case SysEvent_ChangingFullScreen:
	case SysEvent_GSButtonPressed:
	case SysEvent_ResetCPU_SoftDone:
	case SysEvent_Interrupt_SP:
	case SysEvent_Interrupt_SI:
	case SysEvent_Interrupt_AI:
	case SysEvent_Interrupt_VI:
	case SysEvent_Interrupt_PI:
	case SysEvent_Interrupt_DP:
		QueueEvent(action);
		break;
	case SysEvent_ResetCPU_Soft:
		QueueEvent(action);
		if (m_SyncCPU)
		{
			m_SyncCPU->QueueEvent(action);
		}
		break;
	case SysEvent_PauseCPU_FromMenu: 
	case SysEvent_PauseCPU_AppLostFocus: 
	case SysEvent_PauseCPU_AppLostActive: 
	case SysEvent_PauseCPU_SaveGame: 
	case SysEvent_PauseCPU_LoadGame: 
	case SysEvent_PauseCPU_DumpMemory: 
	case SysEvent_PauseCPU_SearchMemory: 
		if (!_Settings->LoadBool(GameRunning_CPU_Paused))
		{
			QueueEvent(action);			
		}
		break;
	case SysEvent_ResumeCPU_FromMenu:
		// always resume if from menu
		SetEvent(m_hPauseEvent);
		break;
	case SysEvent_ResumeCPU_AppGainedFocus:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostFocus )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_AppGainedActive:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_AppLostActive )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_SaveGame:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SaveGame )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_LoadGame:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_LoadGame )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_DumpMemory:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_DumpMemory )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResumeCPU_SearchMemory:
		if (_Settings->LoadDword(GameRunning_CPU_PausedType) == PauseType_SearchMemory )
		{
			SetEvent(m_hPauseEvent);
		}
		break;
	case SysEvent_ResetCPU_Hard:
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix				
			m_InReset = true;
		
			CN64Rom * TempRom = _Rom;
			_Rom = 0;
			Reset();
			_Rom = TempRom;
			
			m_Limitor.SetHertz(_Settings->LoadDword(Game_ScreenHertz)); //Is set in LoadRomSettings

			//Recreate Memory
			m_Reg = new CRegisters(this, _Notify);
			_MMU = new CMipsMemoryVM(this);
			_MMU->FixRDramSize();
			
			m_Audio.Reset();

			m_InReset = false;
			StartEmulation(true);
#endif
		}
		break;
	case SysEvent_CPUUsageTimerChanged:
		g_ShowCPUPer = _Settings->LoadDword(UserInterface_ShowCPUPer);
		break;
	default:
		WriteTraceF(TraceError,"CN64System::ExternalEvent - Unknown event %d",action);
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

bool CN64System::EmulationStarting ( HANDLE hThread, DWORD ThreadId )
{
	bool bRes = true;

	if (_BaseSystem)
	{
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Destroying old N64 system");
		delete _BaseSystem;
		_BaseSystem = NULL;
	}
	WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Creating N64 system");
	_BaseSystem = new CN64System(_Plugins,false);
	WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Setting N64 system as active");
	if (_BaseSystem->SetActiveSystem(true))
	{
		_BaseSystem->m_CPU_Handle   = hThread;
		_BaseSystem->m_CPU_ThreadID = ThreadId;
		WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Setting up N64 system done");
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->RefreshMenu();
		try
		{
			WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Game set to auto start, starting");
			_BaseSystem->StartEmulation2(false);			
			WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Game Done");
		} 
		catch (...)
		{
			WriteTraceF(TraceError,"Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
			char Message[600];
			sprintf(Message,"CN64System::LoadFileImage - Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__);
			MessageBox(NULL,Message,"Exception",MB_OK);
		}
		_BaseSystem->m_CPU_Handle   = NULL;
		_BaseSystem->m_CPU_ThreadID = 0;
	} else {
		WriteTrace(TraceError,"CN64System::stLoadFileImage: SetActiveSystem failed");
 		_Notify->DisplayError("Failed to Initialize N64 System");
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->RefreshMenu();
		bRes = false;
	}

	if (_BaseSystem)
	{
		delete _BaseSystem;
		_BaseSystem = NULL;
	}
	return bRes;
}

void CN64System::stLoadFileImage (  FileImageInfo * Info ) 
{
	CoInitialize(NULL);

	WriteTrace(TraceDebug,"CN64System::stLoadFileImage: Copy thread Info");

	FileImageInfo ImageInfo = *Info;
	HANDLE ThreadHandle = *(ImageInfo.ThreadHandle);
	delete ImageInfo.ThreadHandle;
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
		_Notify->SetWindowCaption(_Settings->LoadString(Game_GoodName).c_str());
		if (_Settings->LoadDword(Setting_AutoStart) != 0)
		{
			EmulationStarting(*((HANDLE *)ImageInfo.ThreadHandle),ImageInfo.ThreadID);
		}
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->RefreshMenu();
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
		
		*hThread  = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartEmulationThread,Info,0, &Info->ThreadID);	
		return;
	}
	WriteTrace(TraceDebug,"CN64System::StartEmulation2: Starting");

	_Notify->HideRomBrowser();
	//RefreshSettings();

	if (!SetActiveSystem())
	{
		_Settings->SaveBool(GameRunning_LoadingInProgress,false);
		_Notify->DisplayError(MSG_PLUGIN_NOT_INIT);

		//Set handle to NULL so this thread is not terminated
		m_CPU_Handle   = NULL;
		m_CPU_ThreadID = 0;

		_Notify->RefreshMenu();
		_Notify->ShowRomBrowser();
	}


	_Notify->MakeWindowOnTop(_Settings->LoadBool(UserInterface_AlwaysOnTop));
	if (!_Settings->LoadBool(Beta_IsValidExe))
	{
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
	
	EmulationStarting(Info->ThreadHandle,Info->ThreadID);
	delete Info->ThreadHandle;
	delete Info;

	CoUninitialize();
}

void CN64System::CloseCpu ( void ) 
{
	if (m_CPU_Handle == NULL) 
	{
		return; 
	}

	m_EndEmulation = true;
	if (_Settings->LoadBool(GameRunning_CPU_Paused))
	{
		SetEvent(m_hPauseEvent);
	}
	
	if (GetCurrentThreadId() == m_CPU_ThreadID)
	{
		return;
	}
	
	HANDLE hThread = m_CPU_Handle;
	for (int count = 0; count < 200; count ++ ) 
	{
		Sleep(100);
		if (_Notify->ProcessGuiMessages())
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
			CpuStopped();
		}
	}
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

void CN64System::Reset (bool bInitReg, bool ClearMenory) 
{
	if (m_Recomp)
	{
		m_Recomp->ResetRecompCode(); 
	}
	if (_Plugins) { _Plugins->GameReset(); }
	m_Audio.Reset();
	m_MMU_VM.Reset(ClearMenory);
	Debug_Reset();
	CloseSaveChips();

	m_CyclesToSkip = 0;
	m_AlistCount   = 0;
	m_DlistCount   = 0;
	m_UnknownCount = 0;
	m_DMAUsed = false;
	
	m_Reg.Reset();
	m_SystemTimer.Reset();
	m_SystemTimer.SetTimer(CSystemTimer::CompareTimer,m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER,false);

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
			memcpy((m_MMU_VM.Dmem()+0x40), (_Rom->GetRomAddress() + 0x040), 0xFBC);
		}
	}
}

bool CN64System::SetActiveSystem( bool bActive )
{
	bool bInitPlugin = false;
	bool bRes = true;

	if (bActive)
	{		
		m_Reg.SetAsCurrentSystem();

		_System    = this;
		if (_BaseSystem == this)
		{
			_SyncSystem   = m_SyncCPU;
		}
		_Recompiler   = m_Recomp;
		_MMU          = &m_MMU_VM;
		_TLB          = &m_TLB;
		_Reg          = &m_Reg;
		_Audio        = &m_Audio;
		//_Labels       = NULL; //???
		_SystemTimer  = &m_SystemTimer;
		_TransVaddr   = &m_MMU_VM;
		_SystemEvents = this;
		_NextTimer    = &m_NextTimer;		
		_Plugins      = m_Plugins;

		if (!m_bInitilized)
		{
			if (!m_MMU_VM.Initialize())
			{
				return false;
			}
			Reset(true,true);
			m_bInitilized = true;
			bInitPlugin = true;
		}
	} else {
		if (this == _BaseSystem)
		{
			_System       = NULL;
			_SyncSystem   = NULL;
			_Recompiler   = NULL;
			_MMU          = NULL;
			_TLB          = NULL;
			_Reg          = NULL;
			_Audio        = NULL;
			_Labels       = NULL;
			_SystemTimer  = NULL;
			_TransVaddr   = NULL;
			_SystemEvents = NULL;
			_NextTimer    = NULL;
			_Plugins      = m_Plugins;
		}
	}

	if (bInitPlugin)
	{
		WriteTrace(TraceDebug,"CN64System::SetActiveSystem: Reseting Plugins");
		_Notify->DisplayMessage(5,MSG_PLUGIN_INIT);
		_Plugins->Reset();
		bRes = _Plugins->Initiate();
		if (!bRes)
		{
			WriteTrace(TraceError,"CN64System::SetActiveSystem: _Plugins->Initiate Failed");
		}
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
		
		switch (_Rom->GetCountry()) {
		case Germany: case french:  case Italian:
		case Europe:  case Spanish: case Australia:
		case X_PAL:   case Y_PAL:
			switch (_Rom->CicChipID()) {
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
			switch (_Rom->CicChipID()) {
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

		switch (_Rom->CicChipID()) {
		case CIC_NUS_6101: 
			m_Reg.m_GPR[22].DW=0x000000000000003F; 
			break;
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
	} else {
		m_Reg.m_PROGRAM_COUNTER = 0xBFC00000;			
/*		PIF_Ram[36] = 0x00; PIF_Ram[39] = 0x3F; //common pif ram start values

		switch (_Rom->CicChipID()) {
		case CIC_NUS_6101: PIF_Ram[37] = 0x06; PIF_Ram[38] = 0x3F; break;
		case CIC_NUS_6102: PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x3F; break;
		case CIC_NUS_6103:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x78; break;
		case CIC_NUS_6105:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x91; break;
		case CIC_NUS_6106:	PIF_Ram[37] = 0x02; PIF_Ram[38] = 0x85; break;
		}*/
	}
}

void CN64System::ExecuteCPU ( void ) 
{
	_Settings->SaveBool(GameRunning_CPU_Running,true);
	_Settings->SaveBool(GameRunning_CPU_Paused,false);
	_Notify->DisplayMessage(5,MSG_EMULATION_STARTED);
	
	m_EndEmulation = false;
	_Notify->RefreshMenu();

	g_RomFileSize = _Rom->GetRomSize();

	CC_Core C_Core;
	C_Core.SetSettings();
	
	switch ((CPU_TYPE)_Settings->LoadDword(Game_CpuType)) {
	case CPU_Recompiler: ExecuteRecompiler(C_Core); break;
	case CPU_SyncCores:  ExecuteSyncCPU(C_Core);    break;
	default:             ExecuteInterpret(C_Core);  break;
	}
	CpuStopped();
	SetActiveSystem(false);
}

void CN64System::ExecuteInterpret (CC_Core & C_Core) {
	InitializeCPUCore();
	SetActiveSystem();
	CInterpreterCPU::ExecuteCPU();
}

void CN64System::ExecuteRecompiler (CC_Core & C_Core)
{	
	//execute opcodes while no errors	
	InitializeCPUCore();
	m_Recomp = new CRecompiler(m_Profile,m_EndEmulation);
	SetActiveSystem();
	m_Recomp->Run();
}

void CN64System::ExecuteSyncCPU (CC_Core & C_Core) 
{
	_Notify->DisplayMessage(5,"Copy Plugins");
	_Plugins->CopyPlugins(_Settings->LoadString(Directory_PluginSync));
	CMainGui  SyncWindow(false);
	CPlugins  SyncPlugins ( _Settings->LoadString(Directory_PluginSync) ); 
	SyncPlugins.SetRenderWindows(&SyncWindow,&SyncWindow);

	m_SyncCPU = new CN64System(&SyncPlugins, true);
	m_Recomp = new CRecompiler(m_Profile,m_EndEmulation);

	SetActiveSystem();
	m_SyncCPU->SetActiveSystem();
	SetActiveSystem();

	InitializeCPUCore();
	CInterpreterCPU::BuildCPU();
	m_Recomp->Run();
}

void CN64System::CpuStopped ( void ) {
	void * lCPU_Handle = m_CPU_Handle;
	_Settings->SaveBool(GameRunning_CPU_Running,(DWORD)false);
	_Notify->WindowMode();
	if (!m_InReset)
	{
		CloseCpu();
		if (_Plugins)
		{
			_Plugins->ShutDownPlugins();
		}
		if (m_hPauseEvent)
		{
			CloseHandle(m_hPauseEvent);
			m_hPauseEvent = NULL;
		}

		_Notify->RefreshMenu();
		_Notify->MakeWindowOnTop(false);
		_Notify->DisplayMessage(5,MSG_EMULATION_ENDED);
		if (_Settings->LoadDword(RomBrowser_Enabled)) {
			_Notify->ShowRomBrowser(); 
		}	
	}
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
	m_CPU_ThreadID = 0;
	m_CPU_Handle   = 0;
}

void CN64System::UpdateSyncCPU (CN64System * const SecondCPU, DWORD const Cycles) {
	int CyclesToExecute = Cycles - m_CyclesToSkip;
	
	//Update the number of cycles to skip
	m_CyclesToSkip -= Cycles;
	if (m_CyclesToSkip < 0) { m_CyclesToSkip = 0; }

	//Run the other CPU For the same amount of cycles
	if (CyclesToExecute < 0) { return; }
	
	SecondCPU->SetActiveSystem(true);
	//CC_Core::SetCurrentSystem(SecondCPU);
	
	CInterpreterCPU::ExecuteOps(Cycles);

	SetActiveSystem(true);
	//CC_Core::SetCurrentSystem(this);
}

void CN64System::SyncCPU (CN64System * const SecondCPU) {
	bool ErrorFound = false;

	_SystemTimer->UpdateTimers();
	
#ifdef TEST_SP_TRACKING
	if (m_CurrentSP != GPR[29].UW[0]) {
		ErrorFound = true;
	}
#endif
	if (m_Reg.m_PROGRAM_COUNTER != SecondCPU->m_Reg.m_PROGRAM_COUNTER) {
		ErrorFound = true;
	}
	for (int count = 0; count < 32; count ++) {
		if (m_Reg.m_GPR[count].DW != SecondCPU->m_Reg.m_GPR[count].DW) {
			ErrorFound = true;
		}
		if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW) {
			ErrorFound = true;
		}
		if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count]) {
			ErrorFound = true;
		}
	}
	
	if (m_Reg.m_FPCR[0] != SecondCPU->m_Reg.m_FPCR[0]) { ErrorFound = true; }
	if (m_Reg.m_FPCR[31] != SecondCPU->m_Reg.m_FPCR[31]) { ErrorFound = true; }
	if (m_Reg.m_HI.DW != SecondCPU->m_Reg.m_HI.DW) { ErrorFound = true; }
	if (m_Reg.m_LO.DW != SecondCPU->m_Reg.m_LO.DW) { ErrorFound = true; }

	/*for (int z = 0; z < 0x100; z++)
	{	
		if (m_MMU_VM.Rdram()[0x00206970 + z] !=  SecondCPU->m_MMU_VM.Rdram()[0x00206970 + z]) 
		{
			ErrorFound = true;
		}
	}*/
	
	if (bFastSP() && m_Recomp) 
	{
		if (m_Recomp->MemoryStackPos() != (DWORD)(m_MMU_VM.Rdram() + (m_Reg.m_GPR[29].W[0] & 0x1FFFFFFF)))
		{
			ErrorFound = true;
		}
	}

	if (m_SystemTimer.CurrentType() != SecondCPU->m_SystemTimer.CurrentType()) { ErrorFound = true; }
	if (m_NextTimer     != SecondCPU->m_NextTimer) { ErrorFound = true; }
	if (m_Reg.m_RoundingModel != SecondCPU->m_Reg.m_RoundingModel) { ErrorFound = true; }
	
	if (ErrorFound) { DumpSyncErrors(SecondCPU); }

	for (int i = (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])) - 1; i > 0; i--) {
		m_LastSuccessSyncPC[i] = m_LastSuccessSyncPC[i - 1];
	}
	m_LastSuccessSyncPC[0] = m_Reg.m_PROGRAM_COUNTER;
//	if (PROGRAM_COUNTER == 0x8009BBD8) {
//		_Notify->BreakPoint(__FILE__,__LINE__);
//	}
}

void CN64System::DumpSyncErrors (CN64System * SecondCPU) {
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
		for (count = 0; count < 32; count ++) {
			if (m_Reg.m_GPR[count].DW != SecondCPU->m_Reg.m_GPR[count].DW) {
				Error.LogF("GPR[%s] 0x%08X%08X, 0x%08X%08X\r\n",CRegName::GPR[count],
					m_Reg.m_GPR[count].W[1],m_Reg.m_GPR[count].W[0],
					SecondCPU->m_Reg.m_GPR[count].W[1],SecondCPU->m_Reg.m_GPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (m_Reg.m_FPR[count].DW != SecondCPU->m_Reg.m_FPR[count].DW) {
				Error.LogF("FPR[%s] 0x%08X%08X, 0x%08X%08X\r\n",CRegName::FPR[count],
					m_Reg.m_FPR[count].W[1],m_Reg.m_FPR[count].W[0],
					SecondCPU->m_Reg.m_FPR[count].W[1],SecondCPU->m_Reg.m_FPR[count].W[0]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (m_Reg.m_FPCR[count] != SecondCPU->m_Reg.m_FPCR[count]) {
				Error.LogF("FPCR[%s] 0x%08X, 0x%08X\r\n",CRegName::FPR_Ctrl[count],
					m_Reg.m_FPCR[count], SecondCPU->m_Reg.m_FPCR[count]);
			}
		}	
		for (count = 0; count < 32; count ++) {
			if (m_Reg.m_CP0[count] != SecondCPU->m_Reg.m_CP0[count]) {
				Error.LogF("CP0[%s] 0x%08X, 0x%08X\r\n",CRegName::Cop0[count],
					m_Reg.m_CP0[count], SecondCPU->m_Reg.m_CP0[count]);
			}
		}	
		if (m_Reg.m_HI.DW != SecondCPU->m_Reg.m_HI.DW) {
			Error.LogF("HI Reg 0x%08X%08X, 0x%08X%08X\r\n",m_Reg.m_HI.UW[1],m_Reg.m_HI.UW[0],SecondCPU->m_Reg.m_HI.UW[1],SecondCPU->m_Reg.m_HI.UW[0]);
		}
		if (m_Reg.m_LO.DW != SecondCPU->m_Reg.m_LO.DW) {
			Error.LogF("LO Reg 0x%08X%08X, 0x%08X%08X\r\n",m_Reg.m_LO.UW[1],m_Reg.m_LO.UW[0], SecondCPU->m_Reg.m_LO.UW[1],SecondCPU->m_Reg.m_LO.UW[0]);
		}
		if (m_NextTimer     != SecondCPU->m_NextTimer) 
		{ 
			Error.LogF("Current Time: %X %X\r\n",(DWORD)m_NextTimer,(DWORD)SecondCPU->m_NextTimer);
		}
		if (m_SystemTimer.CurrentType() != SecondCPU->m_SystemTimer.CurrentType()) 
		{ 
			Error.LogF("Current Time Type: %X %X\r\n",m_SystemTimer.CurrentType(),SecondCPU->m_SystemTimer.CurrentType());
		}
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
		Error.Log("\r\n");
		Error.Log("Information:\r\n");
		Error.Log("\r\n");
		Error.LogF("PROGRAM_COUNTER,0x%X\r\n",m_Reg.m_PROGRAM_COUNTER);
		Error.LogF("Current Timer,0x%X\r\n",m_NextTimer);
		Error.LogF("Timer Type,0x%X\r\n",m_SystemTimer.CurrentType());
		Error.Log("\r\n");
		for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) {
			Error.LogF("LastSuccessSyncPC[%d],0x%X\r\n",i,m_LastSuccessSyncPC[i]);
		}
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) {
			Error.LogF("GPR[%s],         0x%08X%08X, 0x%08X%08X\r\n",CRegName::GPR[count],
				m_Reg.m_GPR[count].W[1],m_Reg.m_GPR[count].W[0],
				SecondCPU->m_Reg.m_GPR[count].W[1],SecondCPU->m_Reg.m_GPR[count].W[0]);
		}	
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) {
			Error.LogF("FPR[%s],         0x%08X%08X, 0x%08X%08X\r\n",CRegName::FPR[count],
				m_Reg.m_FPR[count].W[1],m_Reg.m_FPR[count].W[0],
				SecondCPU->m_Reg.m_FPR[count].W[1],SecondCPU->m_Reg.m_FPR[count].W[0]);
		}	
		Error.Log("\r\n");
		Error.LogF("Rounding Model,   0x%08X, 0x%08X\r\n",m_Reg.m_RoundingModel,SecondCPU->m_Reg.m_RoundingModel);
		Error.Log("\r\n");
		for (count = 0; count < 32; count ++) {
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
		for (count = 0; count < 32; count ++) {
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
			if (_MMU->LW_VAddr(Addr,OpcodeValue))
			{
				Error.LogF("%X: %s\r\n",Addr,R4300iOpcodeName(OpcodeValue,Addr));
			}

		}
		Error.Log("\r\n");
		Error.Log("Code at Last Sync PC:\r\n");
		for (count = 0; count < 50; count++)
		{
			DWORD OpcodeValue, Addr = m_LastSuccessSyncPC[0] + (count << 2);
			if (_MMU->LW_VAddr(Addr,OpcodeValue))
			{
				Error.LogF("%X: %s\r\n",Addr,R4300iOpcodeName(OpcodeValue,Addr));
			}

		}
	}

	_Notify->DisplayError("Sync Error");
	_Notify->BreakPoint(__FILE__,__LINE__);
//	AddEvent(CloseCPU);
}

bool CN64System::SaveState(void) 
{
	WriteTrace(TraceDebug,"CN64System::SaveState 1");

	if (!m_SystemTimer.SaveAllowed()) { return false; }
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
		if (m_Recomp)
		{
			m_Recomp->ResetRecompCode(); 
		}
	}

	DWORD dwWritten, SaveID_0 = 0x23D8A6C8;
	DWORD RdramSize   = _Settings->LoadDword(Game_RDRamSize);
	DWORD MiInterReg  = _Reg->MI_INTR_REG;
	DWORD NextViTimer = m_SystemTimer.GetTimer(CSystemTimer::ViTimer);
	if (_Settings->LoadDword(Setting_AutoZipInstantSave)) {
		zipFile			file;

		file = zipOpen(FileName.c_str(),0);
		zipOpenNewFileInZip(file,CurrentSaveName.c_str(),NULL,NULL,0,NULL,0,NULL,Z_DEFLATED,Z_DEFAULT_COMPRESSION);
		zipWriteInFileInZip(file,&SaveID_0,sizeof(SaveID_0));
		zipWriteInFileInZip(file,&RdramSize,sizeof(DWORD));
		zipWriteInFileInZip(file,_Rom->GetRomAddress(),0x40);	
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
		zipWriteInFileInZip(file,(void *const)&_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32);
		zipWriteInFileInZip(file,_MMU->PifRam(),0x40);
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
		WriteFile( hSaveFile,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER),&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_GPR,sizeof(_int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_FPR,sizeof(_int64)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_CP0,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_FPCR,sizeof(DWORD)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.m_HI,sizeof(_int64),&dwWritten,NULL);
		WriteFile( hSaveFile,&m_Reg.m_LO,sizeof(_int64),&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Mips_Interface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Video_Interface,sizeof(DWORD)*14,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Audio_Interface,sizeof(DWORD)*6,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8,&dwWritten,NULL);
		WriteFile( hSaveFile,m_Reg.m_SerialInterface,sizeof(DWORD)*4,&dwWritten,NULL);
		WriteFile( hSaveFile,&_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32,&dwWritten,NULL);
		WriteFile( hSaveFile,_MMU->PifRam(),0x40,&dwWritten,NULL);
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
			Reset(false,true);

			_MMU->UnProtectMemory(0x80000000,0x80000000 + _Settings->LoadDword(Game_RDRamSize) - 4);
			_MMU->UnProtectMemory(0xA4000000,0xA4001FFC);
			_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);
			unzReadCurrentFile(file,&NextVITimer,sizeof(NextVITimer));
			unzReadCurrentFile(file,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER));
			unzReadCurrentFile(file,m_Reg.m_GPR,sizeof(_int64)*32);
			unzReadCurrentFile(file,m_Reg.m_FPR,sizeof(_int64)*32);
			unzReadCurrentFile(file,m_Reg.m_CP0,sizeof(DWORD)*32);
			unzReadCurrentFile(file,m_Reg.m_FPCR,sizeof(DWORD)*32);
			unzReadCurrentFile(file,&m_Reg.m_HI,sizeof(_int64));
			unzReadCurrentFile(file,&m_Reg.m_LO,sizeof(_int64));
			unzReadCurrentFile(file,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10);
			unzReadCurrentFile(file,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10);
			unzReadCurrentFile(file,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10);
			unzReadCurrentFile(file,m_Reg.m_Mips_Interface,sizeof(DWORD)*4);
			unzReadCurrentFile(file,m_Reg.m_Video_Interface,sizeof(DWORD)*14);
			unzReadCurrentFile(file,m_Reg.m_Audio_Interface,sizeof(DWORD)*6);
			unzReadCurrentFile(file,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13);
			unzReadCurrentFile(file,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8);
			unzReadCurrentFile(file,m_Reg.m_SerialInterface,sizeof(DWORD)*4);
			unzReadCurrentFile(file,(void *const)&_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32);
			unzReadCurrentFile(file,m_MMU_VM.PifRam(),0x40);
			unzReadCurrentFile(file,m_MMU_VM.Rdram(),SaveRDRAMSize);
			unzReadCurrentFile(file,m_MMU_VM.Dmem(),0x1000);
			unzReadCurrentFile(file,m_MMU_VM.Imem(),0x1000);
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
		Reset(false,true);
		m_MMU_VM.UnProtectMemory(0x80000000,0x80000000 + _Settings->LoadDword(Game_RDRamSize) - 4);
		m_MMU_VM.UnProtectMemory(0xA4000000,0xA4001FFC);
		_Settings->SaveDword(Game_RDRamSize,SaveRDRAMSize);

		ReadFile( hSaveFile,&NextVITimer,sizeof(NextVITimer),&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.m_PROGRAM_COUNTER,sizeof(m_Reg.m_PROGRAM_COUNTER),&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_GPR,sizeof(_int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_FPR,sizeof(_int64)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_CP0,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_FPCR,sizeof(DWORD)*32,&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.m_HI,sizeof(_int64),&dwRead,NULL);
		ReadFile( hSaveFile,&m_Reg.m_LO,sizeof(_int64),&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_RDRAM_Registers,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_SigProcessor_Interface,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Display_ControlReg,sizeof(DWORD)*10,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Mips_Interface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Video_Interface,sizeof(DWORD)*14,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Audio_Interface,sizeof(DWORD)*6,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_Peripheral_Interface,sizeof(DWORD)*13,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_RDRAM_Interface,sizeof(DWORD)*8,&dwRead,NULL);
		ReadFile( hSaveFile,m_Reg.m_SerialInterface,sizeof(DWORD)*4,&dwRead,NULL);
		ReadFile( hSaveFile,(void *const)&_TLB->TlbEntry(0),sizeof(CTLB::TLB_ENTRY)*32,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.PifRam(),0x40,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.Rdram(),SaveRDRAMSize,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.Dmem(),0x1000,&dwRead,NULL);
		ReadFile( hSaveFile,m_MMU_VM.Imem(),0x1000,&dwRead,NULL);
		CloseHandle(hSaveFile);
	}

	//Fix Random Register
	while ((int)m_Reg.RANDOM_REGISTER < (int)m_Reg.WIRED_REGISTER) {
		m_Reg.RANDOM_REGISTER += 32 - m_Reg.WIRED_REGISTER;
	}
	//Fix up timer
	WriteTrace(TraceDebug,"CN64System::LoadState 2");
	m_SystemTimer.SetTimer(CSystemTimer::CompareTimer,m_Reg.COMPARE_REGISTER - m_Reg.COUNT_REGISTER,false);
	m_SystemTimer.SetTimer(CSystemTimer::ViTimer,NextVITimer,false);
	m_Reg.FixFpuLocations();
	WriteTrace(TraceDebug,"CN64System::LoadState 5");
	m_TLB.Reset(false);
	WriteTrace(TraceDebug,"CN64System::LoadState 6");
	m_CPU_Usage.ResetCounters();
	WriteTrace(TraceDebug,"CN64System::LoadState 7");
	m_Profile.ResetCounters();
	WriteTrace(TraceDebug,"CN64System::LoadState 8");
	m_FPS.Reset(true);
	WriteTrace(TraceDebug,"CN64System::LoadState 9");
	ResetX86Logs();
	WriteTrace(TraceDebug,"CN64System::LoadState 12");

#ifdef TEST_SP_TRACKING
	m_CurrentSP = GPR[29].UW[0];
#endif
	if (bFastSP() && m_Recomp) { m_Recomp->ResetMemoryStackPos(); }

	if (_Settings->LoadDword(Game_CpuType) == CPU_SyncCores) {
		if (m_SyncCPU)
		{
			for (int i = 0; i < (sizeof(m_LastSuccessSyncPC)/sizeof(m_LastSuccessSyncPC[0])); i++) {
				m_LastSuccessSyncPC[i] = 0;
			}
			m_SyncCPU->SetActiveSystem(true);
			m_SyncCPU->LoadState(FileNameStr.c_str());
			SetActiveSystem(true);
			SyncCPU(m_SyncCPU);
		}
	}
	WriteTrace(TraceDebug,"CN64System::LoadState 13");
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
//	if (m_Reg.GetCurrentTimerType() != AiTimer)
//	{
//		return;
//	}
	if (_Audio->GetLength()  == 0)
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
	_SystemTimer->SetTimer(CSystemTimer::ViTimer,VI_INTR_TIME,true);
	if (bFixedAudio())
	{
		_Audio->SetViIntr (VI_INTR_TIME);	
	}
	if (_Plugins->Control()->GetKeys) 
	{
		BUTTONS Keys;

		for (int Control = 0; Control < 4; Control++)
		{	
			_Plugins->Control()->GetKeys(Control,&Keys);
			m_Buttons[Control] = Keys.Value;
		}
	}

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
		_Notify->BreakPoint(__FILE__,__LINE__);
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
	if (m_Recomp && m_Recomp->bSMM_TLB())
	{
		m_Recomp->ClearRecompCode_Virt(VAddr,Len,CRecompiler::Remove_TLB);
	}
}

void CN64System::TLB_Changed   ( void )
{
	Debug_RefreshTLBWindow();
}

