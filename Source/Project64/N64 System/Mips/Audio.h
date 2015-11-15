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

class CAudio
{
    enum
    {
        ai_full = 0x80000000,
        ai_busy = 0x40000000,
    };
public:
    CAudio();
    ~CAudio();

    uint32_t GetLength         ();
    uint32_t GetStatus         ();
    void  LenChanged        ();
    void  InterruptTimerDone();
    void  BusyTimerDone     ();
    void  Reset             ();
    void  SetViIntr         ( uint32_t VI_INTR_TIME );
    void  SetFrequency      ( uint32_t Dacrate, uint32_t System );

private:
    CAudio(const CAudio&);            // Disable copy constructor
    CAudio& operator=(const CAudio&); // Disable assignment

    uint32_t  m_SecondBuff;
    uint32_t  m_Status;
    uint32_t  m_BytesPerSecond;
    int32_t   m_CountsPerByte;
    int32_t   m_FramesPerSecond;
};
