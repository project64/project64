#pragma once

typedef void * DynLibHandle;

DynLibHandle DynamicLibraryOpen(const char * LibraryPath, bool ShowErrors = true);
void DynamicLibraryClose(DynLibHandle LibHandle);
void * DynamicLibraryGetProc(DynLibHandle LibHandle, const char * ProcedureName);
