#pragma once
#if defined(__aarch64__)
#include <Project64-core/N64System/Recompiler/RegBase.h>

class CCodeBlock;
class CAarch64Ops;

class CAarch64RegInfo :
    public CRegBase
{
public:
    CAarch64RegInfo(CCodeBlock & CodeBlock, CAarch64Ops & Assembler);
    CAarch64RegInfo(const CAarch64RegInfo &);
    ~CAarch64RegInfo();

    CAarch64RegInfo & operator=(const CAarch64RegInfo &);

    bool operator==(const CAarch64RegInfo & right) const;
    bool operator!=(const CAarch64RegInfo & right) const;
};

#endif
