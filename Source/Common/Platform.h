#pragma once

#ifndef _WIN32
#include <alloca.h>
#include <stdarg.h>

#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define GetCurrentThreadId pthread_self

int _vscprintf (const char * format, va_list pargs);

#endif