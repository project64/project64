#pragma once 

#if (defined(_MSC_VER) && _DEBUG)
#define MEM_LEAK_TEST
#endif

#ifdef MEM_LEAK_TEST

void* operator new (size_t size, const char* filename, unsigned int line);
void* operator new[](size_t size, const char* filename, unsigned int line);
void operator delete (void* ptr, const char* filename, unsigned int line);
void operator delete[](void* ptr, const char* filename, unsigned int line);

#define new new(__FILE__, __LINE__)

#endif
