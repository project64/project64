#ifdef _DEBUG
#define MEM_LEAK_TEST

#ifdef MEM_LEAK_TEST

#ifndef __MEM_TEST__H__
#define __MEM_TEST__H__
#pragma warning(disable:4786)
#pragma warning(disable:4291)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0x0500
#endif

#include <xdebug>
#include <xlocale>
#include <xiosbase>
#include <xtree>

#define new				new(__FILE__, __LINE__)
#include <map>
#include <windows.h>
#undef new

/*
#include <crtdbg.h>
#include <malloc.h>
#include <fstream>
#include <xlocnum>
#include <xlocmon>
#include <sstream>
#include <list>
*/


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
	HMODULE m_hModule;

	enum   INIT { Initialized = 123, NotInitialized };
	INIT   State;
	int    order;
	bool   LogAllocate;

	void Insert(void *res, DEBUG_LOCATION &info);
	void Remove(void *ptr);

public:
	CMemList();
	~CMemList();
	void * AddItem       ( size_t size, char * filename, int line );
	void * ReAllocItem   ( void * ptr, size_t size, const char * filename, int line );
	void   removeItem    ( void * ptr, bool bFree );
	void   RecordAddItem ( void * ptr, size_t size, const char * filename, int line );
};

void* operator new (size_t size );
void* operator new (size_t size, char* filename, int line);
void* operator new [] (size_t size );
void* operator new [] (size_t size, char* filename, int line);
void operator delete ( void* ptr);
void operator delete[](void* ptr);

void* MemTest_malloc(size_t size, char* filename, int line);
void* MemTest_realloc(void* ptr, size_t size, char* filename, int line);
void  MemTest_AddLeak(char* Comment);
LPVOID MemTest_VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect, LPCSTR filename, int line);
BOOL MemTest_VirtualFree( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType );


#endif

#undef new
#undef malloc
#undef free
#define new				new(__FILE__, __LINE__)
#define malloc(x)		(MemTest_malloc((x),__FILE__, __LINE__))
#define realloc(mem,x)	(MemTest_realloc((mem),(x),__FILE__, __LINE__))
#define free(x)			(delete (x))
#define VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect ) \
   (MemTest_VirtualAlloc((lpAddress), (dwSize), (flAllocationType), (flProtect),__FILE__, __LINE__))
#define VirtualFree(lpAddress, dwSize, dwFreeType ) \
   (MemTest_VirtualFree((lpAddress), (dwSize), (dwFreeType)))

#endif

#endif
