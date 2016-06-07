#include "stdafx.h"
#include "DateTimeClass.h"
#ifdef ANDROID
#include <math.h>
#else
#include <sys/timeb.h>
#endif
#include <time.h>

CDateTime::CDateTime()
{
    m_time = 0;
}

CDateTime & CDateTime::SetToNow(void)
{
#ifdef ANDROID
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    m_time = (now.tv_sec * 1000l) + round(now.tv_nsec / 1.0e6);
#else
    struct timeb now;
    (void)::ftime(&now);
    m_time = (now.time * 1000l) + now.millitm;
#endif
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
