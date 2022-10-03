#pragma once
#include <stdint.h>

class HighResTimeStamp
{
public:
    HighResTimeStamp();
    HighResTimeStamp & SetToNow(void);
    uint64_t GetMicroSeconds(void);
    void SetMicroSeconds(uint64_t MicroSeconds);

private:
#ifdef _WIN32
    static bool m_GotFreq;
    static uint64_t m_Freq;
#endif
    uint64_t m_time;
};
