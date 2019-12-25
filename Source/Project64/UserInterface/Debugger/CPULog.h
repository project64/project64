/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                       *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <stdafx.h>

typedef struct
{
    uint32_t pc;
    OPCODE opcode;
    MIPS_DWORD gpr[32];
    MIPS_DWORD gprHi;
    MIPS_DWORD gprLo;
    float fpr[32];
    uint32_t fpcr;
} CPUState;

class CCPULog
{
    size_t m_Size;
    bool m_bMaxed;
    size_t m_Index;
    CPUState* m_Array;

public:
    CCPULog(size_t size = 0);
    ~CCPULog(void);
    void PushState(void);
    size_t GetCount(void);
    size_t GetSize(void);
    CPUState* GetEntry(size_t index);
    void Reset(void);
    CCPULog* Clone(void);
    void DumpToFile(const char* path);
};
