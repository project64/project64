#pragma once
#include "stdtypes.h"

class CDateTime
{
public:
    CDateTime();
    CDateTime & SetToNow (void);
    std::string Format (const char * format);
    double DiffernceMilliseconds (const CDateTime & compare);
    uint64_t Value ( void );

private:
    uint64_t m_time;
}; 