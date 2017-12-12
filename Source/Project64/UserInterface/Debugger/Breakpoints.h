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
#include <Common\stdtypes.h>
#include <map>

class CBreakpoints :
    private CDebugSettings
{
public:
    typedef std::map<uint32_t /*address*/, bool /*bTemporary*/> breakpoints_t;
    typedef breakpoints_t::const_iterator breakpoint_t;

    enum BPSTATE
    {
        BP_NOT_SET,
        BP_SET,
        BP_SET_TEMP
    };

    CBreakpoints();

    const breakpoints_t & ReadMem(void) const { return m_ReadMem; }
    const breakpoints_t & WriteMem(void) const { return m_WriteMem; }
    const breakpoints_t & Execution(void) const { return m_Execution; }

    BPSTATE ReadBPExists(uint32_t address, bool bRemoveTemp = false);
    BPSTATE WriteBPExists(uint32_t address, bool bRemoveTemp = false);
    BPSTATE ExecutionBPExists(uint32_t address, bool bRemoveTemp = false);

    void Pause();
    void Resume();
    void Skip();

    bool isDebugging();
    void KeepDebugging();
    void StopDebugging();
    bool isSkipping();

    bool RBPAdd(uint32_t address, bool bTemporary = false);
    void RBPRemove(uint32_t address);
    void RBPToggle(uint32_t address, bool bTemporary = false);
    void RBPClear();

    bool WBPAdd(uint32_t address, bool bTemporary = false);
    void WBPRemove(uint32_t address);
    void WBPToggle(uint32_t address, bool bTemporary = false);
    void WBPClear();

    bool AddExecution(uint32_t address, bool bTemporary = false);
    void RemoveExecution(uint32_t address);
    void EBPToggle(uint32_t address, bool bTemporary = false);
    void EBPClear();

    void BPClear();

private:
    breakpoints_t m_ReadMem;
    breakpoints_t m_WriteMem;
    breakpoints_t m_Execution;

    bool m_Debugging;
    bool m_Skipping;
};