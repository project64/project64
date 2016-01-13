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
#include <Project64-core/N64System/Recompiler/X86ops.h>

class CRecompMemory :
    protected CX86Ops
{
protected:
    CRecompMemory();
    ~CRecompMemory();

    bool AllocateMemory();
    void CheckRecompMem();
    void Reset();
    void ShowMemUsed();

    uint8_t* RecompPos() const { return m_RecompPos; }

private:
    CRecompMemory(const CRecompMemory&);				// Disable copy constructor
    CRecompMemory& operator=(const CRecompMemory&);		// Disable assignment

    uint8_t * m_RecompCode;
    uint32_t  m_RecompSize;

    enum { MaxCompileBufferSize = 0x03C00000 };
    enum { InitialCompileBufferSize = 0x00500000 };
    enum { IncreaseCompileBufferSize = 0x00100000 };
};
