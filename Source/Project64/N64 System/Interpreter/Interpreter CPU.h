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

class CInterpreterCPU :
    private R4300iOp
{
public:
    static void BuildCPU();
    static void ExecuteCPU();
    static void ExecuteOps(int Cycles);
    static void InPermLoop();

private:
    CInterpreterCPU();                                  // Disable default constructor
    CInterpreterCPU(const CInterpreterCPU&);            // Disable copy constructor
    CInterpreterCPU& operator=(const CInterpreterCPU&); // Disable assignment

    static R4300iOp::Func * m_R4300i_Opcode;
};
