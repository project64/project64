#include "stdafx.h"
#include <Tlhelp32.h>
//#pragma comment(linker,"/merge:.rdata=.text")

void FixUPXIssue ( BYTE * ProgramLocation )
{
	typedef struct 
	{
		DWORD VirtualAddress;
		DWORD SizeOfRawData;
		DWORD Characteristics;
	} ORIGINAL_SECTION;

	char FileName[MAX_PATH];
	GetModuleFileName((HMODULE)ProgramLocation,FileName,sizeof(FileName));
	

	HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,0,NULL);
	DWORD TestID, NoOfSections, dwRead;
	SetFilePointer(hFile,-4,0,FILE_END);
	ReadFile(hFile,&TestID,4,&dwRead,NULL);
	
	if (TestID != 0x3345505a)
	{
/*		//Read Dos Header from file    
		IMAGE_DOS_HEADER * DosHeader = (IMAGE_DOS_HEADER *)ProgramLocation;
		if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) 
		{ 
			return;
		}

		//Read NT (PE) Header
		IMAGE_NT_HEADERS * NTHeader = (IMAGE_NT_HEADERS *)(ProgramLocation + DosHeader->e_lfanew);
		if (NTHeader->Signature != IMAGE_NT_SIGNATURE ) 
		{	
			return;
		}

		IMAGE_SECTION_HEADER * Sections = (IMAGE_SECTION_HEADER *)(ProgramLocation + DosHeader->e_lfanew + 
			sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + NTHeader->FileHeader.SizeOfOptionalHeader);

		for (int count = 0; count < NTHeader->FileHeader.NumberOfSections; count ++ ) 
		{
			IMAGE_SECTION_HEADER & Section = Sections[count];
			LPVOID Address = ProgramLocation + Section.VirtualAddress;
			MEMORY_BASIC_INFORMATION Buffer;
			if (VirtualQuery(Address,&Buffer,sizeof(Buffer)))
			{
				DWORD OldProtect = Buffer.Protect;
				//VirtualProtect(Address, Section.SizeOfRawData,PAGE_READONLY,&OldProtect);
			}
		}*/
		CloseHandle(hFile);
		return;
	}
	SetFilePointer(hFile,-8,0,FILE_END);
	ReadFile(hFile,&NoOfSections,4,&dwRead,NULL);

	ORIGINAL_SECTION * Section = new ORIGINAL_SECTION[NoOfSections];
	DWORD SizeOfSections = (NoOfSections * sizeof(ORIGINAL_SECTION));

	SetFilePointer(hFile,(8 + SizeOfSections) * -1,0,FILE_END);
	ReadFile(hFile,Section,SizeOfSections,&dwRead,NULL);

	for (DWORD count = 0; count < NoOfSections; count ++ ) 
	{
		LPVOID Address = ProgramLocation + Section[count].VirtualAddress;
		MEMORY_BASIC_INFORMATION Buffer;

		if (VirtualQuery(Address,&Buffer,sizeof(Buffer)))
		{
			DWORD MemoryProctect = PAGE_EXECUTE_READWRITE;
			switch (Section[count].Characteristics & 0xF0000000)
			{
			case 0x20000000: MemoryProctect = PAGE_EXECUTE; break;
			case 0x40000000: MemoryProctect = PAGE_READONLY; break;
			case 0x60000000: MemoryProctect = PAGE_EXECUTE_READ; break;
			case 0x80000000: MemoryProctect = PAGE_READWRITE; break;
			case 0xA0000000: MemoryProctect = PAGE_EXECUTE_READWRITE; break;
			case 0xC0000000: MemoryProctect = PAGE_READWRITE; break;
			case 0xE0000000: MemoryProctect = PAGE_EXECUTE_READWRITE; break;
			}

			if (Buffer.Protect != MemoryProctect)
			{
				DWORD OldProtect;
				VirtualProtect(Address, Section[count].SizeOfRawData,MemoryProctect,&OldProtect);
			}
		}
	}

	delete [] Section;
	CloseHandle(hFile);
	/*
	
	//Read Dos Header from file    
	IMAGE_DOS_HEADER * DosHeader = (IMAGE_DOS_HEADER *)ProgramLocation;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) 
	{ 
		return;
	}

	//Read NT (PE) Header
    IMAGE_NT_HEADERS * NTHeader = (IMAGE_NT_HEADERS *)(ProgramLocation + DosHeader->e_lfanew);
	if (NTHeader->Signature != IMAGE_NT_SIGNATURE ) 
	{	
		return;
	}

	DWORD a = sizeof(DWORD);
	DWORD b = sizeof(IMAGE_FILE_HEADER);
	IMAGE_SECTION_HEADER * Sections = (IMAGE_SECTION_HEADER *)(ProgramLocation + DosHeader->e_lfanew + 
		sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + NTHeader->FileHeader.SizeOfOptionalHeader);

	for (int count = 0; count < NTHeader->FileHeader.NumberOfSections; count ++ ) 
	{
		IMAGE_SECTION_HEADER & Section = Sections[count];
		if (_stricmp((char *)Section.Name,".rdata") == 0)
		{
			LPVOID Address = ProgramLocation + Section.VirtualAddress;
			MEMORY_BASIC_INFORMATION Buffer;
			if (VirtualQuery(Address,&Buffer,sizeof(Buffer)))
			{
				if (Buffer.Protect != 2)
				{
					DWORD OldProtect;
					VirtualProtect(Address, Section.SizeOfRawData,PAGE_READONLY,&OldProtect);
				}
			}
			//break;
		}
	}*/
}

CTraceFileLog * LogFile = NULL;

void LogLevelChanged (CTraceFileLog * LogFile)
{
	LogFile->SetTraceLevel((TraceLevel)g_Settings->LoadDword(Debugger_AppLogLevel));
}

void LogFlushChanged (CTraceFileLog * LogFile)
{
	LogFile->SetFlushFile(g_Settings->LoadDword(Debugger_AppLogFlush) != 0);
}


void InitializeLog ( void) 
{
	CPath LogFilePath(CPath::MODULE_DIRECTORY);
	LogFilePath.AppendDirectory("Logs");
	if (!LogFilePath.DirectoryExists())
	{
		LogFilePath.CreateDirectory();
	}
	LogFilePath.SetNameExtension(_T("Project64.log"));

	LogFile = new CTraceFileLog(LogFilePath, g_Settings->LoadDword(Debugger_AppLogFlush) != 0, Log_New,500);
#ifdef VALIDATE_DEBUG
	LogFile->SetTraceLevel((TraceLevel)(g_Settings->LoadDword(Debugger_AppLogLevel) | TraceValidate));
#else
	LogFile->SetTraceLevel((TraceLevel)g_Settings->LoadDword(Debugger_AppLogLevel));
#endif
	AddTraceModule(LogFile);
	
	g_Settings->RegisterChangeCB(Debugger_AppLogLevel,LogFile,(CSettings::SettingChangedFunc)LogLevelChanged);
	g_Settings->RegisterChangeCB(Debugger_AppLogFlush,LogFile,(CSettings::SettingChangedFunc)LogFlushChanged);
}


/*bool ChangeDirPermission ( const CPath & Dir)
{
	if (Dir.DirectoryExists())
	{
		HANDLE hDir = CreateFile(Dir,READ_CONTROL|WRITE_DAC,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
		if (hDir != INVALID_HANDLE_VALUE)
		{
			ACL * pOldDACL = NULL;
			PSECURITY_DESCRIPTOR pSD = NULL;

			if (GetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&pOldDACL,NULL,&pSD) == ERROR_SUCCESS)
			{
				bool bAdd = true;

				PEXPLICIT_ACCESS_W pListOfExplictEntries;
				ULONG cCountOfExplicitEntries;
				if (GetExplicitEntriesFromAclW(pOldDACL,&cCountOfExplicitEntries,&pListOfExplictEntries) == ERROR_SUCCESS)
				{
					for (int i = 0; i < cCountOfExplicitEntries; i ++)
					{
						EXPLICIT_ACCESS_W &ea = pListOfExplictEntries[i];
						if (ea.grfAccessMode != GRANT_ACCESS) { continue; }
						if (ea.grfAccessPermissions != GENERIC_ALL) { continue; }
						if ((ea.grfInheritance & (CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE)) != (CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE)) { continue; }

						if (ea.Trustee.TrusteeType == TRUSTEE_IS_SID)
						{
							
						}
						bAdd = false;
					}
				}

				if (bAdd)
				{
					EXPLICIT_ACCESS ea = {0};
					ea.grfAccessMode = GRANT_ACCESS;
					ea.grfAccessPermissions = GENERIC_ALL;
					ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
					ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
					ea.Trustee.ptstrName = TEXT("Users");

					ACL * pNewDACL = NULL;
					SetEntriesInAcl(1,&ea,pOldDACL,&pNewDACL);

					SetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL);
					LocalFree(pNewDACL);
				}
				LocalFree(pSD);
			}
			CloseHandle(hDir);
		}
	}
	return true;
}*/

void FixDirectories ( void )
{

	CPath Directory(CPath::MODULE_DIRECTORY);
	Directory.AppendDirectory(_T("Config"));
	Directory.CreateDirectory();

	Directory.UpDirectory();
	Directory.AppendDirectory("Logs");
	Directory.CreateDirectory();

	Directory.UpDirectory();
	Directory.AppendDirectory("Save");
	Directory.CreateDirectory();

	Directory.UpDirectory();
	Directory.AppendDirectory("Screenshots");
	Directory.CreateDirectory();
}

bool TerminatedExistingEmu()
{
	bool bTerminated = false;
	bool AskedUser = false;
	DWORD pid = GetCurrentProcessId();

	HANDLE nSearch = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(nSearch != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 lppe;

		memset(&lppe, 0, sizeof(PROCESSENTRY32));
		lppe.dwSize = sizeof(PROCESSENTRY32);
		stdstr ModuleName = CPath(CPath::MODULE_FILE).GetNameExtension();

		if (Process32First(nSearch, &lppe))
		{
			do 
			{
				if(_stricmp(lppe.szExeFile, ModuleName.c_str()) != 0 || 
					lppe.th32ProcessID == pid)
				{
					continue;
				}
				if (!AskedUser)
				{
					AskedUser = true;
					int res = MessageBox(NULL,stdstr_f("Project64.exe currently running\n\nTerminate pid %d now?",lppe.th32ProcessID).c_str(),"Terminate project64",MB_YESNO|MB_ICONEXCLAMATION);
					if (res != IDYES)
					{
						break;
					}
				}
				HANDLE hHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, lppe.th32ProcessID);
				if(hHandle != NULL)
				{
					if (TerminateProcess(hHandle, 0))
					{
						bTerminated = true;
					} else {
						MessageBox(NULL,stdstr_f("Failed to terminate pid %d",lppe.th32ProcessID).c_str(),"Terminate project64 failed!",MB_YESNO|MB_ICONEXCLAMATION);
					}
					CloseHandle(hHandle);
				}
			} while (Process32Next(nSearch, &lppe));
		}
		CloseHandle(nSearch);
	}
	return bTerminated;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpszArgs*/, int /*nWinMode*/) 
{
	FixDirectories();

	CoInitialize(NULL);
	try
	{
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
		LPCSTR AppName = "Project64 2.0";	
		_Lang = new CLanguage();

		g_Settings = new CSettings;
		g_Settings->Initilize(AppName);

		if (g_Settings->LoadBool(Setting_CheckEmuRunning) && 
			TerminatedExistingEmu())
		{
			delete g_Settings;
			g_Settings = new CSettings;
			g_Settings->Initilize(AppName);
		}

		InitializeLog();
		
		WriteTrace(TraceDebug,__FUNCTION__ ": Application Starting");
		FixUPXIssue((BYTE *)hInstance);

		CMipsMemoryVM::ReserveMemory();

		g_Notify = &Notify();

		//Create the plugin container
		WriteTrace(TraceDebug,__FUNCTION__ ": Create Plugins");
		g_Plugins = new CPlugins(g_Settings->LoadString(Directory_Plugin));

		//Select the language
		_Lang->LoadCurrentStrings(true);

		//Create the main window with Menu
		WriteTrace(TraceDebug,__FUNCTION__ ": Create Main Window");
		stdstr WinTitle(AppName);
#ifdef BETA_RELEASE
		WinTitle.Format("Project64 %s (%s)",VersionInfo(VERSION_PRODUCT_VERSION).c_str(),g_Settings->LoadString(Beta_UserName).c_str());
#else
		WinTitle.Format("Project64 %s",VersionInfo(VERSION_PRODUCT_VERSION).c_str());
#endif
		CMainGui  MainWindow(true,WinTitle.c_str()), HiddenWindow(false);
		CMainMenu MainMenu(&MainWindow);
		g_Plugins->SetRenderWindows(&MainWindow,&HiddenWindow);
		g_Notify->SetMainWindow(&MainWindow);

#ifdef BETA_RELEASE
		{
			stdstr_f User("%s",g_Settings->LoadString(Beta_UserName).c_str());
			stdstr_f Email("%s",g_Settings->LoadString(Beta_EmailAddress).c_str());

			if (MD5(User).hex_digest() != g_Settings->LoadString(Beta_UserNameMD5) ||
				MD5(Email).hex_digest() != g_Settings->LoadString(Beta_EmailAddressMD5))
			{
				return false;
			}
		}
#endif
		if (__argc > 1) {
			WriteTraceF(TraceDebug,__FUNCTION__ ": Cmd line found \"%s\"",__argv[1]);
			MainWindow.Show(true);	//Show the main window
			CN64System::RunFileImage(__argv[1]);
		} else {		
			if (g_Settings->LoadDword(RomBrowser_Enabled))
			{ 
				WriteTrace(TraceDebug,__FUNCTION__ ": Show Rom Browser");
				//Display the rom browser
				MainWindow.ShowRomList(); 
				MainWindow.Show(true);	//Show the main window
				MainWindow.HighLightLastRom();
			} else {
				WriteTrace(TraceDebug,__FUNCTION__ ": Show Main Window");
				MainWindow.Show(true);	//Show the main window
			}
		}
		
		//Process Messages till program is closed
		WriteTrace(TraceDebug,__FUNCTION__ ": Entering Message Loop");
		MainWindow.ProcessAllMessages();
		WriteTrace(TraceDebug,__FUNCTION__ ": Message Loop Finished");

		if (g_BaseSystem)
		{
			g_BaseSystem->CloseCpu();
			delete g_BaseSystem;
			g_BaseSystem = NULL;
		}
		WriteTrace(TraceDebug,__FUNCTION__ ": System Closed");
		
		g_Settings->UnregisterChangeCB(Debugger_AppLogLevel,LogFile,(CSettings::SettingChangedFunc)LogLevelChanged);
		g_Settings->UnregisterChangeCB(Debugger_AppLogFlush,LogFile,(CSettings::SettingChangedFunc)LogFlushChanged);
	}
	catch(...)
	{
		WriteTraceF(TraceError,__FUNCTION__ ": Exception caught (File: \"%s\" Line: %d)",__FILE__,__LINE__);
		MessageBox(NULL,stdstr_f("Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__).c_str(),"Exception",MB_OK);
	}
	WriteTrace(TraceDebug,__FUNCTION__ ": cleaning up global objects");
	
	if (g_Rom)      { delete g_Rom; g_Rom = NULL; }
	if (g_Plugins)  { delete g_Plugins; g_Plugins = NULL; }
	if (g_Settings) { delete g_Settings; g_Settings = NULL; }
	if (_Lang)     { delete _Lang; _Lang = NULL; }

	CMipsMemoryVM::FreeReservedMemory();

	CoUninitialize();
	WriteTrace(TraceDebug,__FUNCTION__ ": Done");
	CloseTrace();
	return true;
}
