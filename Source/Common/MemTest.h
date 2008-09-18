#ifdef _DEBUG
#ifndef EXTERNAL_RELEASE
#define MEM_LEAK_TEST
#endif

#ifdef MEM_LEAK_TEST

#ifndef __MEM_TEST__H__
#define __MEM_TEST__H__
#pragma warning(disable:4786)
#pragma warning(disable:4291)

#include <windows.h>

#ifdef __cplusplus
#include <new>
#include <memory>
#include <malloc.h>

extern "C" {
#endif
void* MemTest_malloc(size_t size, char* filename, int line);
void* MemTest_realloc(void* ptr, size_t size, char* filename, int line);
void  MemTest_AddLeak(char* Comment);
void  MemTest_free(void* ptr);
LPVOID MemTest_VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize,DWORD flAllocationType,DWORD flProtect, LPCSTR filename, int line);
BOOL MemTest_VirtualFree( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType );
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void* operator new (size_t size );
void* operator new (size_t size, char* filename, int line);
void* operator new [] (size_t size );
void* operator new [] (size_t size, char* filename, int line);
void operator delete ( void* ptr);
void operator delete[](void* ptr);

#undef new
#define new				new(__FILE__, __LINE__)

#endif

#endif

#undef malloc
#undef free
#define malloc(x)		(MemTest_malloc((x),__FILE__, __LINE__))
#define realloc(mem,x)	(MemTest_realloc((mem),(x),__FILE__, __LINE__))
#define free(x)			(MemTest_free (x))
#define VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect ) \
   (MemTest_VirtualAlloc((lpAddress), (dwSize), (flAllocationType), (flProtect),__FILE__, __LINE__))
#define VirtualFree(lpAddress, dwSize, dwFreeType ) \
   (MemTest_VirtualFree((lpAddress), (dwSize), (dwFreeType)))

#endif

#endif