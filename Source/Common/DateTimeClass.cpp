#include "DateTimeClass.h"
#include <time.h>

CDateTime::CDateTime()
{
    m_time = time(nullptr);
}

std::string CDateTime::Format(const char * format)
{
    char buffer[100];
    strftime(buffer, sizeof(buffer), format, localtime(&m_time));
    return std::string(buffer);
}

CDateTime & CDateTime::SetToNow(void)
{
    m_time = time(nullptr);
    return *this;
}
