/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#include <vector>
#include <string>
#include <Project64-core\N64System\Enhancement\Enhancement.h>

struct EnhancementItemList_compare
{
    bool operator() (const std::string & a, const std::string & b) const
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