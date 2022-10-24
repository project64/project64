#pragma once
#if defined(__arm__) || defined(_M_ARM)
#include <Project64-core/N64System/Recompiler/RegBase.h>

class CCodeBlock;
class CArmOps;

class CArmRegInfo :
    public CRegBase
{
public:
    CArmRegInfo(CCodeBlock & CodeBlock, CArmOps & Assembler);
    CArmRegInfo(const CArmRegInfo &);
    ~CArmRegInfo();

    CArmRegInfo & operator=(const CArmRegInfo &);

    bool operator==(const CArmRegInfo & right) const;
    bool operator!=(const CArmRegInfo & right) const;
};

#endif
