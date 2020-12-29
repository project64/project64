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
#include "stdafx.h"
#include <Project64-core\N64System\Enhancement\EnhancementList.h>

CEnhancementList::CEnhancementList()
{
}

void CEnhancementList::AddItem(const CEnhancement & Details)
{
    insert(value_type(Details.GetName(), Details));
    //push_back(std::pair<std::string, CEnhancement>(Details.GetName(), Details));
}

CEnhancementList::iterator CEnhancementList::FindItem(const std::string & Name)
{
    return find(Name);
    /*iterator itr = end();
    for (iterator itr2 = begin(); itr2 != end(); itr2++)
    {
        if (itr2->first == Name)
        {
            itr = itr2;
        }
    }
    return itr;*/
}