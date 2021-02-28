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
#include <Common/stdtypes.h>
#include <map>

class CBreakpoints :
    private CDebugSettings
{
public:
    typedef std::map<uint32_t /*address*/, bool /*bTemporary*/> breakpoints_t;
    typedef breakpoints_t::const_iterator breakpoint_t;

    typedef std::set<uint32_t> memlocks_t;

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

    BPSTATE ReadBPExists8(uint32_t address);
    BPSTATE ReadBPExists16(uint32_t address);
    BPSTATE ReadBPExists32(uint32_t address);
    BPSTATE ReadBPExists64(uint32_t address);
    BPSTATE WriteBPExists8(uint32_t address);
    BPSTATE WriteBPExists16(uint32_t address);
    BPSTATE WriteBPExists32(uint32_t address);
    BPSTATE WriteBPExists64(uint32_t address);
    BPSTATE WriteBPExistsInChunk(uint32_t address, uint32_t nBytes);
    BPSTATE ExecutionBPExists(uint32_t address, bool bRemoveTemp = false);

    bool RBPAdd(uint32_t address);
    void RBPRemove(uint32_t address);
    void RBPToggle(uint32_t address);
    void RBPClear();

    bool WBPAdd(uint32_t address);
    void WBPRemove(uint32_t address);
    void WBPToggle(uint32_t address);
    void WBPClear();

    bool AddExecution(uint32_t address, bool bTemporary = false);
    void RemoveExecution(uint32_t address);
    void EBPToggle(uint32_t address, bool bTemporary = false);
    void EBPClear();

    void BPClear();

    void ToggleMemLock(uint32_t address);
    bool MemLockExists(uint32_t address, int nBytes);
    void ClearMemLocks(void);
    size_t NumMemLocks(void);

    inline bool HaveRegBP(void) { return m_bHaveRegBP; }
    inline bool HaveAnyGPRWriteBP(void) { return m_GPRWriteBP != 0; }
    inline bool HaveAnyGPRReadBP(void) { return m_GPRReadBP != 0; }
    inline bool HaveGPRWriteBP(int nReg) { return (m_GPRWriteBP & (1 << nReg)) != 0; }
    inline bool HaveGPRReadBP(int nReg) { return (m_GPRReadBP  & (1 << nReg)) != 0; }
    inline bool HaveHIWriteBP(void) { return m_HIWriteBP; }
    inline bool HaveHIReadBP(void) { return m_HIReadBP; }
    inline bool HaveLOWriteBP(void) { return m_LOWriteBP; }
    inline bool HaveLOReadBP(void) { return m_LOReadBP; }

    void UpdateHaveRegBP(void);
    void ToggleGPRWriteBP(int nReg);
    void ToggleGPRReadBP(int nReg);
    void ToggleHIWriteBP(void);
    void ToggleHIReadBP(void);
    void ToggleLOWriteBP(void);
    void ToggleLOReadBP(void);
   
private:
    void PreUpdateBP();
    void PostUpdateBP();
    void UpdateAlignedWriteBP(void);
    void UpdateAlignedReadBP(void);

    breakpoints_t m_ReadMem, m_ReadMem16, m_ReadMem32, m_ReadMem64;
    breakpoints_t m_WriteMem, m_WriteMem16, m_WriteMem32, m_WriteMem64;
    breakpoints_t m_Execution;

    memlocks_t m_MemLocks;

    bool m_bHaveRegBP;
    uint32_t m_GPRWriteBP, m_GPRReadBP;
    bool m_HIWriteBP, m_HIReadBP, m_LOWriteBP, m_LOReadBP;
};