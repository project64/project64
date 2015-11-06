#pragma once
#include "stdtypes.h"

class pjutil
{
public:
    static void Sleep(unsigned long timeout);
    static bool TerminatedExistingExe();

private:
    pjutil(void);                       // Disable default constructor
    pjutil(const pjutil&);              // Disable copy constructor
    pjutil& operator=(const pjutil&);   // Disable assignment
};
