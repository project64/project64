/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#include "JavaRomList.h"
#include "JavaBridge.h"

#ifdef ANDROID
extern JavaBridge * g_JavaBridge;

void CJavaRomList::RomListReset(void)
{
    if (g_JavaBridge)
    {
        g_JavaBridge->RomListReset();
    }
}

void CJavaRomList::RomAddedToList(int32_t ListPos)
{
    if (g_JavaBridge)
    {
        ROM_INFO * pRomInfo = &m_RomInfo[ListPos];
        g_JavaBridge->RomListAddItem(pRomInfo->szFullFileName, pRomInfo->FileName, pRomInfo->GoodName, pRomInfo->TextColor);
    }
}

void CJavaRomList::RomListLoaded(void)
{
    if (g_JavaBridge)
    {
        g_JavaBridge->RomListLoaded();
    }
}

#endif