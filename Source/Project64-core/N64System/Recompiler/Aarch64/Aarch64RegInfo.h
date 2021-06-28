#pragma once
#if defined(__aarch64__)
#include <Project64-core/N64System/Recompiler/RegBase.h>

class CAarch64RegInfo :
    public CRegBase
{
public:
    CAarch64RegInfo();
    ~CAarch64RegInfo();

private:
};
#endif
