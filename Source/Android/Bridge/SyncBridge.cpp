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
#include "SyncBridge.h"

#ifdef ANDROID
SyncBridge::SyncBridge(JavaBridge * javaBridge) :
    m_JavaBridge(javaBridge)
{
}

void SyncBridge::GfxThreadInit()
{
}

void SyncBridge::GfxThreadDone()
{
}

void SyncBridge::SwapWindow()
{
    if (m_JavaBridge)
    {
        m_JavaBridge->SwapWindow();
    }
}


#endif