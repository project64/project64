#include "Platform.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifndef _WIN32
int _vscprintf(const char * format, va_list pargs)
{
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(nullptr, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}
#endif