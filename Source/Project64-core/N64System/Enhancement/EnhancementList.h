#pragma once
#include <Common/Platform.h>
#include <Project64-core/N64System/Enhancement/Enhancement.h>
#include <string>
#include <vector>

struct EnhancementItemList_compare
{
    bool operator()(const std::string & a, const std::string & b) const
    {
        return _stricmp(a.c_str(), b.c_str()) < 0;
    }
};

class CEnhancementList :
    public std::map<std::string, CEnhancement, EnhancementItemList_compare>
{
public:
    CEnhancementList();

    void AddItem(const CEnhancement & Details);
    iterator FindItem(const std::string & Name);
};