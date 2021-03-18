#pragma once
#include <Project64-core/RomList/RomList.h>

class CJavaRomList :
    public CRomList
{
public:
    void RomListReset(void);
    void RomAddedToList(int32_t ListPos);
    void RomListLoaded(void);
};
