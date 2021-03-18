#pragma once
#include "stdtypes.h"

class CDateTime
{
public:
    CDateTime();
    CDateTime & SetToNow (void);
    std::string Format (const char * format);

private:
    time_t m_time;
};
