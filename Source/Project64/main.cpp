#include "stdafx.h"
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
	LogFile->SetTraceLevel((TraceLevel)_Settings->LoadDword(Debugger_AppLogLevel));
}

void LogFlushChanged (CTraceFileLog * LogFile)
{
	LogFile->SetFlushFile(_Settings->LoadDword(Debugger_AppLogFlush) != 0);
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

	LogFile = new CTraceFileLog(LogFilePath, _Settings->LoadDword(Debugger_AppLogFlush) != 0, Log_New,500);
#ifdef VALIDATE_DEBUG
	LogFile->SetTraceLevel((TraceLevel)(_Settings->LoadDword(Debugger_AppLogLevel) | TraceValidate));
#else
	LogFile->SetTraceLevel((TraceLevel)_Settings->LoadDword(Debugger_AppLogLevel));
#endif
	AddTraceModule(LogFile);
	
	_Settings->RegisterChangeCB(Debugger_AppLogLevel,LogFile,(CSettings::SettingChangedFunc)LogLevelChanged);
	_Settings->RegisterChangeCB(Debugger_AppLogFlush,LogFile,(CSettings::SettingChangedFunc)LogFlushChanged);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgs, int nWinMode) 
{
	FixDirectories();

	CoInitialize(NULL);
	try
	{
		LPCSTR AppName = "Project64 1.7";	
		_Lang = new CLanguage();

		_Settings = new CSettings;
		_Settings->Initilize(AppName);

		InitializeLog();
		
		WriteTrace(TraceDebug,"WinMain - Application Starting");
		FixUPXIssue((BYTE *)hInstance);

		_Notify = &Notify();

		//Create the plugin container
		WriteTrace(TraceDebug,"WinMain - Create Plugins");
		_Plugins = new CPlugins(_Settings->LoadString(Directory_Plugin));

		//Select the language
		_Lang->LoadCurrentStrings(true);

		//Create the main window with Menu
		WriteTrace(TraceDebug,"WinMain - Create Main Window");
		stdstr WinTitle(AppName);
		if (_Settings->LoadBool(Beta_IsBetaVersion))
		{
			WinTitle.Format("Project64 %s (%s)",VersionInfo(VERSION_PRODUCT_VERSION).c_str(),_Settings->LoadString(Beta_UserName).c_str());
		}
		CMainGui  MainWindow(true,WinTitle.c_str()), HiddenWindow(false);
		CMainMenu MainMenu(&MainWindow);
		_Plugins->SetRenderWindows(&MainWindow,&HiddenWindow);
		_Notify->SetMainWindow(&MainWindow);

		{
			stdstr_f User("%s",_Settings->LoadString(Beta_UserName).c_str());
			stdstr_f Email("%s",_Settings->LoadString(Beta_EmailAddress).c_str());

			if (MD5(User).hex_digest() != _Settings->LoadString(Beta_UserNameMD5) ||
				MD5(Email).hex_digest() != _Settings->LoadString(Beta_EmailAddressMD5))
			{
				return false;
			}
		}
				
		if (__argc > 1) {
			WriteTraceF(TraceDebug,"WinMain - Cmd line found \"%s\"",__argv[1]);
			MainWindow.Show(true);	//Show the main window
			CN64System::RunFileImage(__argv[1]);
		} else {		
			if (_Settings->LoadDword(RomBrowser_Enabled))
			{ 
				WriteTrace(TraceDebug,"WinMain - Show Rom Browser");
				//Display the rom browser
				MainWindow.ShowRomList(); 
				MainWindow.Show(true);	//Show the main window
				MainWindow.HighLightLastRom();
			} else {
				WriteTrace(TraceDebug,"WinMain - Show Main Window");
				MainWindow.Show(true);	//Show the main window
			}
		}
		
		//Process Messages till program is closed
		WriteTrace(TraceDebug,"WinMain - Entering Message Loop");
		MainWindow.ProcessAllMessages();
		WriteTrace(TraceDebug,"WinMain - Message Loop Finished");

		if (_BaseSystem)
		{
			_BaseSystem->CloseCpu();
			delete _BaseSystem;
			_BaseSystem = NULL;
		}
		WriteTrace(TraceDebug,"WinMain - System Closed");
		
		_Settings->UnregisterChangeCB(Debugger_AppLogLevel,LogFile,(CSettings::SettingChangedFunc)LogLevelChanged);
		_Settings->UnregisterChangeCB(Debugger_AppLogFlush,LogFile,(CSettings::SettingChangedFunc)LogFlushChanged);
	}
	catch(...)
	{
		WriteTraceF(TraceError,"WinMain - Exception caught (File: \"%s\" Line: %d)",__FILE__,__LINE__);
		MessageBox(NULL,stdstr_f("Exception caught\nFile: %s\nLine: %d",__FILE__,__LINE__).c_str(),"Exception",MB_OK);
	}
	WriteTrace(TraceDebug,"WinMain - cleaning up global objects");
	
	if (_Rom)      { delete _Rom; _Rom = NULL; }
	if (_Plugins)  { delete _Plugins; _Plugins = NULL; }
	if (_Settings) { delete _Settings; _Settings = NULL; }
	if (_Lang)     { delete _Lang; _Lang = NULL; }

	CoUninitialize();
	WriteTrace(TraceDebug,"WinMain - Done");
	CloseTrace();
	return true;
}
