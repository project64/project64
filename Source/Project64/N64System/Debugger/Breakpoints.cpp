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
#include "Breakpoints.h"

#include <Project64-core/N64System/Mips/RegisterClass.h>
#include <Project64-core/N64System/SystemGlobals.h>
#include <Project64-core/N64System/Mips/OpcodeName.h>
#include <Project64-core/N64System/N64Class.h>

//BOOL CBreakpoints::m_Debugging = FALSE;
//BOOL CBreakpoints::m_Skipping = FALSE;
//
//std::vector<uint32_t> CBreakpoints::m_RBP;
//std::vector<uint32_t> CBreakpoints::m_WBP;
//std::vector<uint32_t> CBreakpoints::m_EBP;
//
//int CBreakpoints::m_nRBP = 0;
//int CBreakpoints::m_nWBP = 0;
//int CBreakpoints::m_nEBP = 0;

CBreakpoints::CBreakpoints()
{
	m_Debugging = FALSE;
	m_Skipping = FALSE;
	m_nRBP = 0;
	m_nWBP = 0;
	m_nEBP = 0;
}

void CBreakpoints::Pause()
{
	KeepDebugging();
	g_System->Pause();
}

void CBreakpoints::Resume()
{
	g_System->ExternalEvent(SysEvent_ResumeCPU_FromMenu);
}

BOOL CBreakpoints::isDebugging()
{
	return m_Debugging;
}

void CBreakpoints::KeepDebugging()
{
	m_Debugging = TRUE;
}

void CBreakpoints::StopDebugging()
{
	m_Debugging = FALSE;
}

void CBreakpoints::Skip()
{
	m_Skipping = TRUE;
}

bool CBreakpoints::RBPAdd(uint32_t address, bool bTemporary)
{
	if (!RBPExists(address))
	{
		m_RBP.push_back({ address, bTemporary });
		m_nRBP = m_RBP.size();
		return true;
	}
	return false;
}

bool CBreakpoints::WBPAdd(uint32_t address, bool bTemporary)
{
	if (!WBPExists(address))
	{
		m_WBP.push_back({ address, bTemporary });
		m_nWBP = m_WBP.size();
		return true;
	}
	return false;
}

bool CBreakpoints::EBPAdd(uint32_t address, bool bTemporary)
{
	if (!EBPExists(address))
	{
		m_EBP.push_back({ address, bTemporary });
		m_nEBP = m_EBP.size();
		return true;
	}
	return false;
}

void CBreakpoints::RBPRemove(uint32_t address)
{
	for (int i = 0; i < m_nRBP; i++)
	{
		if (m_RBP[i].address == address)
		{
			m_RBP.erase(m_RBP.begin() + i);
			m_nRBP = m_RBP.size();
			return;
		}
	}
}

void CBreakpoints::WBPRemove(uint32_t address)
{
	for (int i = 0; i < m_nWBP; i++)
	{
		if (m_WBP[i].address == address)
		{
			m_WBP.erase(m_WBP.begin() + i);
			m_nWBP = m_WBP.size();
			return;
		}
	}
}

void CBreakpoints::EBPRemove(uint32_t address)
{
	for (int i = 0; i < m_nEBP; i++)
	{
		if (m_EBP[i].address == address)
		{
			m_EBP.erase(m_EBP.begin() + i);
			m_nEBP = m_EBP.size();
			return;
		}
	}
}

void CBreakpoints::RBPToggle(uint32_t address, bool bTemporary)
{
	if (RBPAdd(address, bTemporary) == false)
	{
		RBPRemove(address);
	}
}

void CBreakpoints::WBPToggle(uint32_t address, bool bTemporary)
{
	if (WBPAdd(address, bTemporary) == false)
	{
		WBPRemove(address);
	}
}

void CBreakpoints::EBPToggle(uint32_t address, bool bTemporary)
{
	if (EBPAdd(address, bTemporary) == false)
	{
		EBPRemove(address);
	}
}

void CBreakpoints::RBPClear()
{
	m_RBP.clear();
	m_nRBP = 0;
}

void CBreakpoints::WBPClear()
{
	m_WBP.clear();
	m_nWBP = 0;
}

void CBreakpoints::EBPClear()
{
	m_EBP.clear();
	m_nEBP = 0;
}

void CBreakpoints::BPClear()
{
	RBPClear();
	WBPClear();
	EBPClear();
}
