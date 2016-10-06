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
#include <Project64-core/N64System/Recompiler/FunctionInfo.h>
#include <Project64-core/Settings/GameSettings.h>

class CFunctionMap :
    private CGameSettings
{
protected:
    typedef CCompiledFunc *  PCCompiledFunc;
    typedef PCCompiledFunc * PCCompiledFunc_TABLE;

    CFunctionMap();
    ~CFunctionMap();

    bool AllocateMemory();
    void Reset(bool bAllocate);

public:
    PCCompiledFunc_TABLE * FunctionTable() const { return m_FunctionTable; }
    PCCompiledFunc       * JumpTable() const { return m_JumpTable; }

private:
    void CleanBuffers();

    PCCompiledFunc       * m_JumpTable;
    PCCompiledFunc_TABLE * m_FunctionTable;
};
