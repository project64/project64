#include "DynamicLibrary.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#endif

DynLibHandle DynamicLibraryOpen(const char *pccLibraryPath, bool ShowErrors)
{
    if (pccLibraryPath == nullptr)
    {
        return nullptr;
    }
#ifdef _WIN32
    UINT LastErrorMode = SetErrorMode(ShowErrors ? 0 : SEM_FAILCRITICALERRORS);
    DynLibHandle Lib = (DynLibHandle)LoadLibraryA(pccLibraryPath);
    SetErrorMode(LastErrorMode);
#else
    DynLibHandle Lib = (DynLibHandle)dlopen(pccLibraryPath, RTLD_NOW);
#endif
    return Lib;
}

void DynamicLibraryClose(DynLibHandle Lib)
{
    if (Lib != nullptr)
    {
#ifdef _WIN32
        FreeLibrary((HMODULE)Lib);
#else
        dlclose(Lib);
#endif
    }
}

void * DynamicLibraryGetProc(DynLibHandle Lib, const char * ProcedureName)
{
    if (ProcedureName == nullptr)
    {
        return nullptr;
    }

#ifdef _WIN32
    return GetProcAddress((HMODULE)Lib, ProcedureName);
#else
    return dlsym(Lib, ProcedureName);
#endif
}

