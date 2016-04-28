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
#include "DebuggerUI.h"

CPj64Module _Module;

CDebuggerUI::CDebuggerUI () :
    m_MemoryDump(NULL),
    m_MemoryView(NULL),
    m_MemorySearch(NULL),
    m_DebugTLB(NULL)
{
	g_Settings->RegisterChangeCB(GameRunning_InReset,this,(CSettings::SettingChangedFunc)GameReset);
	g_Debugger = this;
}

CDebuggerUI::~CDebuggerUI (void)
{
	g_Settings->UnregisterChangeCB(GameRunning_InReset,this,(CSettings::SettingChangedFunc)GameReset);
    Debug_Reset();
}

void CDebuggerUI::GameReset ( CDebuggerUI * _this )
{
	if (!g_Settings->LoadBool(GameRunning_InReset))
	{
		return;
	}
	_this->Debug_Reset();
}

void CDebuggerUI::Debug_Reset ( void )
{
    if (m_MemoryDump)
    {
        m_MemoryDump->HideWindow();
        delete m_MemoryDump;
        m_MemoryDump = NULL;
    }
    if (m_MemoryView)
    {
        m_MemoryView->HideWindow();
        delete m_MemoryView;
        m_MemoryView = NULL;
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->HideWindow();
        delete m_MemorySearch;
        m_MemorySearch = NULL;
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->HideWindow();
        delete m_DebugTLB;
        m_DebugTLB = NULL;
    }
}

void CDebuggerUI::Debug_ShowMemoryDump()
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_MemoryDump == NULL)
    {
        m_MemoryDump = new CDumpMemory(this);
    }
    if (m_MemoryDump)
    {
        m_MemoryDump->ShowWindow();
    }
}

void CDebuggerUI::Debug_ShowMemoryWindow ( void )
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_MemoryView == NULL)
    {
        m_MemoryView = new CDebugMemoryView(this);
    }
    if (m_MemoryView)
    {
        m_MemoryView->ShowWindow();
    }
}

void CDebuggerUI::Debug_ShowMemoryLocation ( uint32_t Address, bool VAddr )
{
    Debug_ShowMemoryWindow();
    if (m_MemoryView)
    {
        m_MemoryView->ShowAddress(Address,VAddr);
    }
}

void CDebuggerUI::Debug_ShowTLBWindow (void)
{
    if (g_MMU == NULL)
    {
        return;
    }
    if (m_DebugTLB == NULL)
    {
        m_DebugTLB = new CDebugTlb(this);
    }
    if (m_DebugTLB)
    {
        m_DebugTLB->ShowWindow();
    }
}

void CDebuggerUI::Debug_RefreshTLBWindow(void)
{
    if (m_DebugTLB)
    {
        m_DebugTLB->RefreshTLBWindow();
    }
}

void CDebuggerUI::Debug_ShowMemorySearch()
{
    if (m_MemorySearch == NULL)
    {
        m_MemorySearch = new CDebugMemorySearch(this);
    }
    if (m_MemorySearch)
    {
        m_MemorySearch->ShowWindow();
    }
}

void CDebuggerUI::TLBChanged()
{
    Debug_RefreshTLBWindow();
}