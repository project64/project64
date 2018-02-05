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

CBreakpoints::CBreakpoints()
{
}

bool CBreakpoints::RBPAdd(uint32_t address)
{
    if (!ReadBPExists8(address))
    {
        m_ReadMem.insert(breakpoints_t::value_type(address, false));
        UpdateAlignedReadBP();
        if (!HaveReadBP())
        {
            g_Settings->SaveBool(Debugger_ReadBPExists, true);
        }
        return true;
    }
    return false;
}

bool CBreakpoints::WBPAdd(uint32_t address)
{
    if (!WriteBPExists8(address))
    {
        m_WriteMem.insert(breakpoints_t::value_type(address, false));
        UpdateAlignedWriteBP();
        if (!HaveWriteBP())
        {
            g_Settings->SaveBool(Debugger_WriteBPExists, true);
        }
        return true;
    }
    return false;
}

bool CBreakpoints::AddExecution(uint32_t address, bool bTemporary)
{
    breakpoints_t::_Pairib res = m_Execution.insert(breakpoint_t::value_type(address, bTemporary));
    if (!res.second && !bTemporary)
    {
        res.first->second = true;
    }
    if (!HaveExecutionBP())
    {
        g_Settings->SaveBool(Debugger_HaveExecutionBP, true);
    }
    g_BaseSystem->ExternalEvent(SysEvent_ResetRecompilerCode);
    return !res.second;
}

void CBreakpoints::RBPRemove(uint32_t address)
{
    breakpoints_t::iterator itr = m_ReadMem.find(address);
    if (itr != m_ReadMem.end())
    {
        m_ReadMem.erase(itr);
        UpdateAlignedWriteBP();
        if (m_ReadMem.size() == 0)
        {
            g_Settings->SaveBool(Debugger_ReadBPExists, false);
        }
    }
}

void CBreakpoints::WBPRemove(uint32_t address)
{
    breakpoints_t::iterator itr = m_WriteMem.find(address);
    if (itr != m_WriteMem.end())
    {
        m_WriteMem.erase(itr);
        UpdateAlignedWriteBP();
        if (m_WriteMem.size() == 0)
        {
            g_Settings->SaveBool(Debugger_WriteBPExists, false);
        }
    }
}

void CBreakpoints::RemoveExecution(uint32_t address)
{
    breakpoints_t::iterator itr = m_Execution.find(address);
    if (itr != m_Execution.end())
    {
        m_Execution.erase(itr);
        if (m_Execution.size() == 0)
        {
            g_Settings->SaveBool(Debugger_HaveExecutionBP, false);
        }
    }
}

void CBreakpoints::RBPToggle(uint32_t address)
{
    if (RBPAdd(address) == false)
    {
        RBPRemove(address);
    }
}

void CBreakpoints::WBPToggle(uint32_t address)
{
    if (WBPAdd(address) == false)
    {
        WBPRemove(address);
    }
}

void CBreakpoints::EBPToggle(uint32_t address, bool bTemporary)
{
    if (AddExecution(address, bTemporary) == false)
    {
        RemoveExecution(address);
    }
}

void CBreakpoints::RBPClear()
{
    m_ReadMem.clear();
    UpdateAlignedReadBP();
    g_Settings->SaveBool(Debugger_ReadBPExists, false);
}

void CBreakpoints::WBPClear()
{
    m_WriteMem.clear();
    UpdateAlignedWriteBP();
    g_Settings->SaveBool(Debugger_WriteBPExists, false);
}

void CBreakpoints::EBPClear()
{
    m_Execution.clear();
    g_Settings->SaveBool(Debugger_HaveExecutionBP, false);
}

void CBreakpoints::BPClear()
{
    RBPClear();
    WBPClear();
    EBPClear();
}

CBreakpoints::BPSTATE CBreakpoints::ReadBPExists8(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_ReadMem.find(address);
    if (itr != m_ReadMem.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::ReadBPExists16(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_ReadMem16.find(address);
    if (itr != m_ReadMem16.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::ReadBPExists32(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_ReadMem32.find(address);
    if (itr != m_ReadMem32.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::ReadBPExists64(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_ReadMem64.find(address);
    if (itr != m_ReadMem64.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::WriteBPExists8(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_WriteMem.find(address);
    if (itr != m_WriteMem.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::WriteBPExists16(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_WriteMem16.find(address);
    if (itr != m_WriteMem16.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::WriteBPExists32(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_WriteMem16.find(address);
    if (itr != m_WriteMem32.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::WriteBPExists64(uint32_t address)
{
    breakpoints_t::const_iterator itr = m_WriteMem16.find(address);
    if (itr != m_WriteMem64.end())
    {
        return BP_SET;
    }
    return BP_NOT_SET;
}

CBreakpoints::BPSTATE CBreakpoints::ExecutionBPExists(uint32_t address, bool bRemoveTemp)
{
    breakpoints_t::const_iterator itr = m_Execution.find(address);
    if (itr != m_Execution.end())
    {
        if (itr->second)
        {
            if (bRemoveTemp)
            {
                m_Execution.erase(itr);
            }
            return BP_SET_TEMP;
        }
        return BP_SET;
    }
    return BP_NOT_SET;
}

void CBreakpoints::UpdateAlignedReadBP()
{
    m_ReadMem16.clear();
    m_ReadMem32.clear();
    m_ReadMem64.clear();

    for (breakpoints_t::const_iterator itr = m_ReadMem.begin(); itr != m_ReadMem.end(); itr++)
    {
        m_ReadMem16.insert(breakpoints_t::value_type((itr->first & ~0x1), false));
        m_ReadMem32.insert(breakpoints_t::value_type((itr->first & ~0x3), false));
        m_ReadMem64.insert(breakpoints_t::value_type((itr->first & ~0x7), false));
    }
}

void CBreakpoints::UpdateAlignedWriteBP()
{
    m_WriteMem16.clear();
    m_WriteMem32.clear();
    m_WriteMem64.clear();

    for (breakpoints_t::const_iterator itr = m_WriteMem.begin(); itr != m_WriteMem.end(); itr++)
    {
        m_WriteMem16.insert(breakpoints_t::value_type((itr->first & ~0x1), false));
        m_WriteMem32.insert(breakpoints_t::value_type((itr->first & ~0x3), false));
        m_WriteMem64.insert(breakpoints_t::value_type((itr->first & ~0x7), false));
    }
}
