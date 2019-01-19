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
#pragma once

__interface CDebugger
{
    virtual void OpenCommandWindow(void) = 0;
    virtual void OpenMemoryWindow(void) = 0;
    virtual void OpenMemoryDump(void) = 0;
    virtual void OpenMemorySearch(void) = 0;
    virtual void OpenTLBWindow(void) = 0;
    virtual void OpenScriptsWindow(void) = 0;
    virtual void OpenSymbolsWindow(void) = 0;
    virtual void OpenDMALogWindow(void) = 0;
    virtual void OpenCPULogWindow(void) = 0;
    virtual void OpenExcBreakpointsWindow(void) = 0;
    virtual void OpenStackTraceWindow(void) = 0;
    virtual void OpenStackViewWindow(void) = 0;
    virtual void TLBChanged(void) = 0;
    virtual void FrameDrawn(void) = 0;
    virtual void WaitForStep(void) = 0;
    virtual bool ExecutionBP(uint32_t address) = 0;
    virtual bool ReadBP8(uint32_t address) = 0;
    virtual bool ReadBP16(uint32_t address) = 0;
    virtual bool ReadBP32(uint32_t address) = 0;
    virtual bool ReadBP64(uint32_t address) = 0;
    virtual bool WriteBP8(uint32_t address) = 0;
    virtual bool WriteBP16(uint32_t address) = 0;
    virtual bool WriteBP32(uint32_t address) = 0;
    virtual bool WriteBP64(uint32_t address) = 0;

    virtual void CPUStepStarted(void) = 0;
    virtual void CPUStep(void) = 0;
    virtual void CPUStepEnded(void) = 0;
};
