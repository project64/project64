#include <crtdbg.h>
#include "stdafx.h"
#ifdef _DEBUG

#pragma warning(disable:4786)	//Disable std library warning
#pragma warning(disable:4530)	//Disable std library warning
#include <string>
#include <malloc.h>

#include "MemTest.h"
#ifdef MEM_LEAK_TEST

typedef struct {
	char        File[300];
	int         line;
	int         size;
	int         order;
} DEBUG_LOCATION;

typedef std::map<void *, DEBUG_LOCATION> MEMLIST;
typedef MEMLIST::iterator                MEMLIST_ITER;

class CMemList {
	MEMLIST MemList;
	HANDLE  hSemaphone;
	DWORD   ThreadID;

	enum   INIT { Initialized = 123, NotInitialized };
	INIT   State;
	int    order;

public:
	CMemList();
	~CMemList();
	void * AddItem     ( size_t size, LPCSTR filename, int line );
	void * AddItem     ( size_t size, LPCSTR filename, int line, void * MemoryPtr );
	void * ReAllocItem ( void * ptr, size_t size, LPCSTR filename, int line );
	void   removeItem  ( void * ptr, bool FreePointer );
};

#include <shellapi.h>                //Needed for ShellExecute
#pragma comment(lib, "shell32.lib")  //Needed for ShellExecute

#undef new
#undef malloc
#undef realloc
#undef free
#undef VirtualAlloc
#undef VirtualFree

#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION          0x00200000L
#endif

CMemList *MemList ( void ) {
	static CMemList m_MemList;

	return &m_MemList;
}

CMemList::CMemList() { 
	MemList.clear(); 
	hSemaphone = CreateSemaphore(NULL, 1,1, NULL);
	State = Initialized;
	order = 0;
}

CMemList::~CMemList() {
	int ItemsLeft = MemList.size();
	if (ItemsLeft > 0) {
		char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
		char fname[_MAX_FNAME],ext[_MAX_EXT], LogFileName[_MAX_PATH];

		GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
		_splitpath( path_buffer, drive, dir, fname, ext );
		_makepath( LogFileName, drive, dir, fname, "leak.csv" );
			
		
		HANDLE hLogFile;
		do 
		{
			hLogFile = CreateFile(LogFileName,GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
				CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			
			if (hLogFile == INVALID_HANDLE_VALUE) 
			{
				if (GetLastError() == ERROR_SHARING_VIOLATION) {
					std::string Msg = std::string(LogFileName) + "\nCan not be opened for writing please close app using this file\n\nTry Again ?";
					int Result = MessageBox(NULL,Msg.c_str(),"Memory Leak",MB_YESNO|MB_ICONQUESTION|MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION);
					if (Result == IDNO) {
						break;
					}
				}
			}
		} while (hLogFile == INVALID_HANDLE_VALUE);

		if (hLogFile != INVALID_HANDLE_VALUE) 
		{
			SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);

			DWORD dwWritten;
			char  Msg[800];
			_snprintf(Msg,sizeof(Msg),"Order, Source File, Line Number, Mem Size\r\n");			
			WriteFile( hLogFile,Msg,strlen(Msg),&dwWritten,NULL );

			for (MEMLIST_ITER item = MemList.begin(); item != MemList.end(); item++) 
			{
				_snprintf(Msg,sizeof(Msg),"%d,%s, %d, %d\r\n",
					(*item).second.order,
					(*item).second.File,
					(*item).second.line, 
					(*item).second.size);			
				WriteFile( hLogFile,Msg,strlen(Msg),&dwWritten,NULL );				
			}
			CloseHandle(hLogFile);
		}
		std::string Msg = std::string(fname) + std::string(ext) + "\n\nMemory Leaks detected\n\nOpen the Log File ?";
		int Result = MessageBox(NULL,Msg.c_str(),"Memory Leak",MB_YESNO|MB_ICONQUESTION|MB_SETFOREGROUND| MB_SERVICE_NOTIFICATION);
		if (Result == IDYES) {
			ShellExecute(NULL,"open",LogFileName,NULL,NULL,SW_SHOW);
		}
	}
	CloseHandle(hSemaphone);
	hSemaphone = NULL;
	State = NotInitialized;
}
	


void * CMemList::AddItem ( size_t size, LPCSTR filename, int line)
{
	void *res = malloc(size);		
	if (res == NULL) {
		_asm int 3
	}
	return AddItem(size,filename,line,res);
}

void * CMemList::AddItem ( size_t size, LPCSTR filename, int line, void * MemoryPtr )
{
	__try {
		if (State == Initialized && hSemaphone != NULL) {
			DWORD CurrentThread = GetCurrentThreadId();
			DWORD Result = WaitForSingleObject(hSemaphone,CurrentThread != ThreadID ? 30000: 0);
			if (Result != WAIT_TIMEOUT) {
				ThreadID = CurrentThread;
				
				DEBUG_LOCATION info;
				strncpy(info.File,filename,sizeof(info.File));
				info.line = line;
				info.size = size;
				info.order = order++;

				MemList.insert(MEMLIST::value_type(MemoryPtr,info));
				long dwSemCount = 0;
				ThreadID = -1;
				ReleaseSemaphore(hSemaphone,1,&dwSemCount);
			}
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		_asm int 3
	}
	return MemoryPtr;
}

void * CMemList::ReAllocItem ( void * ptr, size_t size, LPCSTR filename, int line)
{
	void *res = realloc(ptr, size);		
	if (res == NULL) 
	{
		_asm int 3
	}
	if (ptr != res) {
		__try {
			if (State == Initialized && hSemaphone != NULL) {
				DWORD CurrentThread = GetCurrentThreadId();
				DWORD Result = WaitForSingleObject(hSemaphone,CurrentThread != ThreadID ? 30000 : 0);
				if (Result != WAIT_TIMEOUT) {
					ThreadID = CurrentThread;
					//Add new pointer
					DEBUG_LOCATION info;
					strncpy(info.File,filename,sizeof(info.File));
					info.line = line;
					info.size = size;
					info.order = order++;

					MemList.insert(MEMLIST::value_type(res,info));

					//remove old pointer
					MEMLIST_ITER item = MemList.find(ptr);
					if (item != MemList.end()) {
						MemList.erase(ptr);
					}
					
					long dwSemCount = 0;
					ThreadID = -1;
					ReleaseSemaphore(hSemaphone,1,&dwSemCount);
				}
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			_asm int 3
		}
	}
	return res;
}

void CMemList::removeItem (void * ptr, bool FreePointer ) 
{
	if (FreePointer)
	{
		free(ptr);
	}
	__try {
		if (State == Initialized && hSemaphone != NULL) {
			DWORD CurrentThread = GetCurrentThreadId();
			DWORD Result = WaitForSingleObject(hSemaphone,CurrentThread != ThreadID ? 30000 : 0);
			if (Result != WAIT_TIMEOUT) {
				ThreadID = CurrentThread;
				MEMLIST_ITER item = MemList.find(ptr);
				if (item != MemList.end()) {
					MemList.erase(ptr);
				}
				long dwSemCount = 0;
				ThreadID = -1;
				ReleaseSemaphore(hSemaphone,1,&dwSemCount);
			}
		}
	}__except(EXCEPTION_EXECUTE_HANDLER ){
		_asm int 3			
	}
}

void  MemTest_AddLeak(char* Comment) {
	MemList()->AddItem(1,Comment,-1);
}

void* MemTest_malloc (size_t size, char* filename, int line) {
	return MemList()->AddItem(size,filename,line);
}

void* MemTest_realloc (void* ptr, size_t size, char* filename, int line) {
	return MemList()->ReAllocItem(ptr, size,filename,line);
}

void  MemTest_free(void* ptr)
{
	MemList()->removeItem(ptr,true);
}

void* operator new (size_t size, char* filename, int line)
{
	return MemList()->AddItem(size,filename,line);
}

void* operator new (size_t size)
{
	return MemList()->AddItem(size,"Unknown",0);
}

void* operator new [] (size_t size, char* filename, int line)
{
	return MemList()->AddItem(size,filename,line);
}

void* operator new [] (size_t size)
{
	return MemList()->AddItem(size,"Unknown",0);
}

void operator delete ( void* ptr)
{
	MemList()->removeItem(ptr,true);
}

void operator delete[](void* ptr)
{
	delete ptr;
}

LPVOID MemTest_VirtualAlloc(
  LPVOID lpAddress,        // region to reserve or commit
  SIZE_T dwSize,           // size of region
  DWORD flAllocationType,  // type of allocation
  DWORD flProtect,          // type of access protection
  LPCSTR filename, 
  int line)
{
	LPVOID ptr = VirtualAlloc(lpAddress,dwSize,flAllocationType,flProtect);
	
	if (ptr && lpAddress == NULL && (flAllocationType & MEM_RESERVE) != 0)
	{
		MemList()->AddItem(dwSize,filename,line,ptr);
	}
	return ptr;
}

BOOL MemTest_VirtualFree( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType )
{
	if ((dwFreeType & MEM_RELEASE) != 0)
	{
		MemList()->removeItem(lpAddress,false);
	}
	return VirtualFree(lpAddress,dwSize,dwFreeType);
}

#endif
#endif