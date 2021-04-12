#include "HighResTimeStamp.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <math.h>
#endif

#ifdef _WIN32
bool HighResTimeStamp::m_GotFreq = false;
uint64_t HighResTimeStamp::m_Freq = { 0 };
#endif

HighResTimeStamp::HighResTimeStamp()
{
#ifndef _WIN32
    m_time = 0;
#else
    if (!m_GotFreq)
    {
        LARGE_INTEGER value;
        QueryPerformanceFrequency(&value);
        m_Freq = value.QuadPart;
    }
    m_time = 0;
#endif
}

HighResTimeStamp & HighResTimeStamp::SetToNow(void)
{
#ifndef _WIN32
    struct timespec now;
    memset(&now, 0, sizeof(now));
    clock_gettime(CLOCK_MONOTONIC, &now);
    m_time = ((uint64_t)now.tv_sec * (uint64_t)1000000l) + (now.tv_nsec / 1000);
#else
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    m_time = value.QuadPart;
#endif
    return *this;
}

uint64_t HighResTimeStamp::GetMicroSeconds(void)
{
#ifndef _WIN32
    return m_time;
#else
    return (m_time * 1000000) / m_Freq;
#endif
}

void HighResTimeStamp::SetMicroSeconds(uint64_t MicroSeconds)
{
#ifndef _WIN32
    m_time = MicroSeconds;
#else
    m_time = (MicroSeconds * m_Freq) / 1000000;
#endif
}
