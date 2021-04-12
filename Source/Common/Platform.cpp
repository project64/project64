#include "Platform.h"
#include <stdint.h>

#ifndef _WIN32
int _vscprintf(const char * format, va_list pargs)
{
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}
#endif

#ifdef _MSC_VER
#include <float.h>

int fesetround(int RoundType)
{
    static const unsigned int msRound[4] = { _RC_NEAR, _RC_CHOP, _RC_UP, _RC_DOWN };
    int32_t res = _controlfp(msRound[RoundType], _MCW_RC);
    if (res == _RC_NEAR) { return FE_TONEAREST; }
    if (res == _RC_CHOP) { return FE_TOWARDZERO; }
    if (res == _RC_UP) { return FE_UPWARD; }
    if (res == _RC_DOWN) { return FE_DOWNWARD; }
    return FE_TONEAREST;
}
#endif
