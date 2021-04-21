#pragma once
#include <stdint.h>

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
