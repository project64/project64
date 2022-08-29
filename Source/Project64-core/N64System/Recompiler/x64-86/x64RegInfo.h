#pragma once

#if defined(__amd64__) || defined(_M_X64)
#include <Project64-core/N64System/Recompiler/RegBase.h>

class CCodeBlock;
class CX64Ops;

class CX64RegInfo :
    public CRegBase
{
public:
    CX64RegInfo(CCodeBlock & CodeBlock, CX64Ops & Assembler);
    CX64RegInfo(const CX64RegInfo&);
    ~CX64RegInfo();

    CX64RegInfo& operator=(const CX64RegInfo&);

    bool operator==(const CX64RegInfo& right) const;
    bool operator!=(const CX64RegInfo& right) const;
};

#endif
