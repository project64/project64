#pragma once

#if defined(__amd64__) || defined(_M_X64)
#include <Project64-core/N64System/Recompiler/RegBase.h>

class CX64RegInfo :
    public CRegBase
{
public:
    bool operator==(const CX64RegInfo& right) const;
};

#endif
