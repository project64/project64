#pragma once
#include <stdint.h>

template<class T, class D, class S> static inline T safe_truncate_cast_assign(D*dst, const S&src)
{
    #if __cplusplus >= 201103L
    static_assert(sizeof(T) == sizeof(D), "Type changed, must update cast");
    #endif
    return *dst = (D) src; // Should trigger a warning if the D type has changed to something larger than T
}

class pjutil
{
public:
    static void Sleep(uint32_t timeout);
    static bool TerminatedExistingExe();

private:
    pjutil(void);
    pjutil(const pjutil&);
    pjutil& operator=(const pjutil&);
};
