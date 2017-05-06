#pragma once
#include "stdtypes.h"

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
    pjutil(void);                       // Disable default constructor
    pjutil(const pjutil&);              // Disable copy constructor
    pjutil& operator=(const pjutil&);   // Disable assignment
};
