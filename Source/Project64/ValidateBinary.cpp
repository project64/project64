#include "stdafx.h"
#include <Wininet.h>
#pragma comment(lib, "Wininet.lib")

LPSTR ValidateDecryptString (LPSTR String, int Len)
{
	BYTE PreviousChar = 0xAA;
	for (int x = 0; x < Len; x++)
	{
		String[x] ^= PreviousChar;
		PreviousChar = String[x];
	}
	return String;
}

LPSTR ValidateEncryptString (LPSTR String, int Len)
{
	BYTE EncryptChar = 0xAA;
	for (int x = 0; x < Len; x++)
	{
		BYTE PreviousChar = String[x];
		String[x] ^= EncryptChar;
		EncryptChar = PreviousChar;
	}
	return String;
}

void TestValidBinaryThread ( )
{
	typedef struct 
	{
		BYTE File_md5[16];
		int  RunTimes;
	} INVALID_RUN_ITEM;

	enum { MAX_BAD_DATA = 50, FAILED_CHECK = 0x7FFFFFFF };
	
	AUTO_PTR<BYTE> RunData;
	INVALID_RUN_ITEM * RunItems = NULL;
	int RunItemCount = 0;
	bool DefaultResult = true;

	CPath ModuleFileName(CPath::MODULE_FILE);
	MD5 File_md5(ModuleFileName);
#ifdef VALIDATE_DEBUG
	WriteTraceF(TraceValidate,"v1: %s",File_md5.hex_digest());
#endif

	// see if already invalid
	char Subkey[] = "\x84\x5E\x1A\x5C\x02\x4E"; // ".pj64z";
	ValidateDecryptString(Subkey,sizeof(Subkey) - 1);

	char Subkey2[] = "\xF9\x1C\x09\x12\x03\x16\x13\x17\x19\x0C\x1A\x7C\x02"; // "SOFTWARE\PJ64";
	ValidateDecryptString(Subkey2,sizeof(Subkey2) - 1);

	CRegistry Registry;
	BOOL bRes = Registry.Open(HKEY_LOCAL_MACHINE,Subkey2,KEY_ALL_ACCESS,true);
	DWORD Length = 0;
	if (bRes)
	{
		Length = Registry.GetValueSize("data");

		if (Length == -1)
		{
			Length = 0;
			bRes = false;
		}
		
		if (bRes && Length > 0)
		{
			DWORD Type;
			RunData = AUTO_PTR<BYTE>(new BYTE[Length]);
			bRes = Registry.GetValue("data",RunData.get(),Length,&Type);
			if (!bRes && Type != REG_BINARY)
			{
				bRes = false;
			}
		}
	}

	if (!bRes)
	{
		bRes = Registry.Open(HKEY_CLASSES_ROOT,Subkey,KEY_ALL_ACCESS,true);
		if (bRes)
		{
			Length = Registry.GetValueSize("data");

			if (Length == -1)
			{
				Length = 0;
				bRes = false;
			}
			
			if (bRes)
			{
				DWORD Type;
				RunData = AUTO_PTR<BYTE>(new BYTE[Length]);
				bRes = Registry.GetValue("data",RunData.get(),Length,&Type);
				if (!bRes && Type != REG_BINARY)
				{
					bRes = false;
				}
			}
		}
	}
	
	if (bRes && Length > 0)
	{
		RunItems = (INVALID_RUN_ITEM *)RunData.get();
		RunItemCount = Length / sizeof(INVALID_RUN_ITEM);

		for (int i = 0; i < RunItemCount; i ++)
		{
			if (memcmp(RunItems[i].File_md5,File_md5.raw_digest(),sizeof(RunItems[i].File_md5)) == 0)
			{
#ifdef VALIDATE_DEBUG
				WriteTraceF(TraceValidate,"v2: %d",RunItems[i].RunTimes);
#endif
				if (RunItems[i].RunTimes >= MAX_BAD_DATA)
				{
					DefaultResult = false;
				}
				break;
			}
		}
	}


	//test to see if file has already been disabled
	HINTERNET hSession = InternetOpen("project64", INTERNET_OPEN_TYPE_PRECONFIG,NULL, NULL, NULL);
	if (hSession == NULL)
	{
#ifdef VALIDATE_DEBUG
		WriteTrace(TraceValidate,"v3");
#endif
		_Settings->SaveBool(Beta_IsValidExe,DefaultResult);
		return;
	}

#ifdef VALIDATE_BIN_LOCAL
	char Site[] = { "local.pj64.net" };
	ValidateEncryptString(Site,sizeof(Site) - 1);
#else
	char Site[] = { "\xda\x1a\x5c\x02\x19\x48\x08\x18\x5b\x4d\x0c\x02" }; //"pj64-emu.com"
#endif
	ValidateDecryptString(Site,sizeof(Site) - 1);
	HINTERNET hConnect = InternetConnect(hSession, Site, 80, "", "", INTERNET_SERVICE_HTTP, 0, (LPARAM)0);

	if (hConnect == NULL)
	{
#ifdef VALIDATE_DEBUG
		WriteTrace(TraceValidate,"v4");
#endif
		_Settings->SaveBool(Beta_IsValidExe,DefaultResult);
		InternetCloseHandle (hSession);
		hSession = NULL;
		return;
	}

	char AcceptText[] = "\xDE\x11\x1D\x0C\x5B\x05"; // "text/*";
	ValidateDecryptString(AcceptText,sizeof(AcceptText)  - 1);

	LPCTSTR lpszAcceptTypes[] = 
	{ 
		AcceptText,
		NULL
	};

	char szPost[] = { "\xFA\x1F\x1C\x07" }; // "POST"
	ValidateDecryptString(szPost,sizeof(szPost)  - 1);

	// "index.php?option=com_validate_pj64"
	char SiteLoc[] = { "\xc3\x07\x0a\x01\x1d\x56\x5e\x18\x18\x4f\x50\x1f\x04\x1d\x06\x01\x53\x5e\x0c\x02\x32\x29\x17\x0d\x05\x0d\x05\x15\x11\x3a\x2f\x1a\x5c\x02" };
//	char SiteLoc[] = { "index.php?option=com_validate_pj64" };

//	ValidateEncryptString(SiteLoc,sizeof(SiteLoc)  - 1);
	ValidateDecryptString(SiteLoc,sizeof(SiteLoc)  - 1);


	HINTERNET hRequest = HttpOpenRequest(hConnect, szPost, SiteLoc, NULL, NULL, lpszAcceptTypes, INTERNET_FLAG_PRAGMA_NOCACHE, (LPARAM)0);
	if (hRequest == NULL)
	{
#ifdef VALIDATE_DEBUG
		WriteTrace(TraceValidate,"v5");
#endif
		_Settings->SaveBool(Beta_IsValidExe,DefaultResult);
		InternetCloseHandle (hRequest);
		return;
	}

	stdstr ComputerName;
	Length = 260;
	ComputerName.resize(Length);
	GetComputerName((char *)ComputerName.c_str(),&Length);

	ComputerName.ToLower();

	stdstr_f PostInfo("1,%s,%s,%s,%s,%s,%s",VALIDATE_BIN_APP,File_md5.hex_digest(),ComputerName.c_str(),VersionInfo(VERSION_PRODUCT_VERSION).c_str(),_Settings->LoadString(Beta_UserName).c_str(),_Settings->LoadString(Beta_EmailAddress).c_str());
	
	//"Content-Type: application/x-www-form-urlencoded"
    char ContentType[] = { "\xE9\x2C\x01\x1A\x11\x0B\x1A\x59\x79\x2D\x09\x15\x5F\x1A\x41\x11\x00\x1C\x05\x0A\x02\x15\x1D\x06\x01\x41\x57\x55\x5A\x00\x00\x5A\x4B\x09\x1D\x1F\x40\x58\x07\x1E\x09\x0B\x0D\x0C\x0B\x01\x01" }; 
	ValidateDecryptString(ContentType,sizeof(ContentType) - 1);


	//encrypt PostData
	stdstr_f PostData("%s=","Data");
	BYTE * Input = (BYTE *)PostInfo.c_str();
	BYTE PreviousChar = 0xAA;
	for (int x = 0; x < PostInfo.size(); x++)
	{
		PostData += stdstr_f("%02X",(BYTE)(Input[x] ^ PreviousChar));
		PreviousChar = Input[x];
	}

	BOOL Success = HttpSendRequest(hRequest, ContentType, sizeof(ContentType) - 1, (LPVOID)PostData.c_str(), PostData.length());
	if (!Success)
	{
#ifdef VALIDATE_DEBUG
		WriteTrace(TraceValidate,"v6");
#endif
		_Settings->SaveBool(Beta_IsValidExe,DefaultResult);
		InternetCloseHandle (hRequest);
		return;
	}
	
	std::auto_ptr<BYTE> WebSiteData;
	DWORD dwRead = 0;
	ULONG DataLen = 0;
	do{
		BYTE SiteData[0x1000];
		memset(SiteData,0,sizeof(SiteData));
		if (!InternetReadFile(hRequest,SiteData,sizeof(SiteData),&dwRead))
		{
#ifdef VALIDATE_DEBUG
			WriteTrace(TraceValidate,"v7");
#endif
			_Settings->SaveBool(Beta_IsValidExe,DefaultResult);
			InternetCloseHandle (hRequest);
			return;
		}

		if (dwRead == 0)
		{
			break;
		}
		
		std::auto_ptr<BYTE> NewSiteData(new BYTE[DataLen + dwRead + 1]);
		if (DataLen > 0)
		{
			memcpy(NewSiteData.get(),WebSiteData.get(),DataLen);
		} 
		memcpy(NewSiteData.get() + DataLen,SiteData,dwRead);
		NewSiteData.get()[DataLen + dwRead] = 0;
		WebSiteData = NewSiteData;
		DataLen += dwRead;

	} while (dwRead > 0);

#ifdef VALIDATE_DEBUG
	WriteTraceF(TraceValidate,"v8: %s",WebSiteData.get());
#endif

	MD5 Result_md5(WebSiteData.get(),DataLen);

	int  LastRunItem  = -1;
	bool bSaveRunInfo = false;
	for (int run = 0; run < RunItemCount; run ++)
	{
		if (memcmp(RunItems[run].File_md5,File_md5.raw_digest(),sizeof(RunItems[run].File_md5)) == 0)
		{
			LastRunItem = run;
			break;
		}
	}

#ifdef VALIDATE_DEBUG
	WriteTraceF(TraceValidate,"v9: %s",Result_md5.hex_digest());
#endif
	//if good MD5
	if (stricmp(Result_md5.hex_digest(),"FB2CDD258756A5472BD24BABF2EC9F66") == 0) // Good Md5
	{
		if (LastRunItem > 0)
		{
			for (int i = (LastRunItem + 1); i < RunItemCount; i++)
			{
				memcpy(&RunItems[i - 1],&RunItems[i],sizeof(RunItems[i]));
			}
			RunItemCount -= 1;
			bSaveRunInfo  = true;
		}
		DefaultResult = true;
	}
	else if (stricmp(Result_md5.hex_digest(),"9030FF575A9B687DC868B966CB7C02D4") == 0) // Bad MD5
	{
		if (LastRunItem > 0)
		{
			if (RunItems[LastRunItem].RunTimes != FAILED_CHECK)
			{
				RunItems[LastRunItem].RunTimes = FAILED_CHECK;
				bSaveRunInfo  = true;
			}
		} else {
			AUTO_PTR<BYTE> NewRunData(new BYTE[(RunItemCount + 1) * sizeof(INVALID_RUN_ITEM)]);
			INVALID_RUN_ITEM * NewRunItems = (INVALID_RUN_ITEM *)NewRunData.get();
			for (int i = 0; i < RunItemCount; i++)
			{
				NewRunItems[i] = RunItems[i];
			}
			LastRunItem = RunItemCount;
			memcpy(NewRunItems[LastRunItem].File_md5,File_md5.raw_digest(),sizeof(NewRunItems[LastRunItem].File_md5));
			NewRunItems[LastRunItem].RunTimes = FAILED_CHECK;
				
			RunData       = NewRunData;
			RunItems      = (INVALID_RUN_ITEM *)RunData.get();
			RunItemCount += 1; 
			bSaveRunInfo  = true;
		}
		DefaultResult = false;
	}
	else
	{
		if (LastRunItem >= 0)
		{
			if (RunItems[LastRunItem].RunTimes != FAILED_CHECK)
			{
				RunItems[LastRunItem].RunTimes += 1;
				if (RunItems[LastRunItem].RunTimes >= MAX_BAD_DATA)
				{
					DefaultResult = false;
					RunItems[LastRunItem].RunTimes = MAX_BAD_DATA;
				}
				bSaveRunInfo  = true;
			}
		} else {
			AUTO_PTR<BYTE> NewRunData(new BYTE[(RunItemCount + 1) * sizeof(INVALID_RUN_ITEM)]);
			INVALID_RUN_ITEM * NewRunItems = (INVALID_RUN_ITEM *)NewRunData.get();
			for (int i = 0; i < RunItemCount; i++)
			{
				NewRunItems[i] = RunItems[i];
			}
			LastRunItem = RunItemCount;
			memcpy(NewRunItems[LastRunItem].File_md5,File_md5.raw_digest(),sizeof(NewRunItems[LastRunItem].File_md5));
			NewRunItems[LastRunItem].RunTimes = 1;
				
			RunData       = NewRunData;
			RunItems      = (INVALID_RUN_ITEM *)RunData.get();
			RunItemCount += 1; 
			bSaveRunInfo  = true;
		}
		if (RunItems[LastRunItem].RunTimes >= MAX_BAD_DATA)
		{
			DefaultResult = false;
		}

	}
	
	bSaveRunInfo = true;
	if (bSaveRunInfo)
	{
		bRes = Registry.Open(HKEY_CLASSES_ROOT,Subkey,KEY_ALL_ACCESS,true);
		if (bRes)
		{
			bRes = Registry.SetValue("data",(BYTE *)RunData.get(),RunItemCount * sizeof(INVALID_RUN_ITEM),REG_BINARY);
		}

		if (bRes)
		{
			bRes = Registry.Open(HKEY_LOCAL_MACHINE,Subkey2,KEY_ALL_ACCESS);
			if (bRes)
			{
				Registry.DeleteValue("data");
			}
		}else {
			bRes = Registry.Open(HKEY_LOCAL_MACHINE,Subkey2,KEY_ALL_ACCESS,true);
			if (bRes)
			{
				bRes = Registry.SetValue("data",(BYTE *)RunData.get(),RunItemCount * sizeof(INVALID_RUN_ITEM),REG_BINARY);
			}
		} 
	}
	_Settings->SaveBool(Beta_IsValidExe,DefaultResult);
}

void TestValidBinary ( )
{
#if defined(EXTERNAL_RELEASE) || defined(VALIDATE_BIN_LOCAL)
	static DWORD ThreadID = 0;
	if (ThreadID == 0)
	{
		HANDLE hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)TestValidBinaryThread,(LPVOID)NULL,0,&ThreadID);
		CloseHandle(hThread);
	}
#endif
}
