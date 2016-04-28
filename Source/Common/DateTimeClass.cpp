#include "stdafx.h"
#include "DateTimeClass.h"
#include <sys/timeb.h>
#include <time.h>

CDateTime::CDateTime()
{
    m_time = 0;
}

CDateTime & CDateTime::SetToNow(void)
{
    struct timeb now;
    (void)::ftime(&now);
    m_time = (now.time * 1000l) + now.millitm;
    return *this;
}

std::string CDateTime::Format (const char * format)
{
    char buffer[100];
    time_t TimeValue = m_time / 1000l;
    strftime(buffer, sizeof(buffer), format, localtime(&TimeValue));
    return std::string(buffer);
}

double CDateTime::DiffernceMilliseconds (const CDateTime & compare)
{
    double diff = (double)(m_time - compare.m_time);
    return diff / 1000.0;
}

uint64_t CDateTime::Value ( void )
{
    return m_time;
}
