#pragma once
#include <stdint.h>

class pjutil
{
public:
    typedef void * DynLibHandle;

    static DynLibHandle DynLibOpen(const char *pccLibraryPath, bool ShowErrors = true);
    static void * DynLibGetProc(DynLibHandle LibHandle, const char * ProcedureName);
    static void DynLibClose(DynLibHandle LibHandle);
    static void Sleep(uint32_t timeout);
    static bool TerminatedExistingExe();

private:
    pjutil(void);
    pjutil(const pjutil&);
    pjutil& operator=(const pjutil&);
};
