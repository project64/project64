#pragma once

#ifndef _WIN32
#include <alloca.h>
#include <stdarg.h>

#define stricmp strcasecmp
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _isnan isnan

int _vscprintf (const char * format, va_list pargs);

#endif

// FPU rounding code
#ifdef _WIN32
typedef enum { FE_TONEAREST = 0, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD } eRoundType;
int fesetround(int RoundType);
#else
#include <fenv.h>
#endif
