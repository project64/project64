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

class Mempak
{
public:
    static uint8_t CalculateCrc(uint8_t * DataToCrc);
    static void Load(int32_t Control);
    static void Format(int32_t Control);
    static void ReadFrom(int32_t Control, uint32_t address, uint8_t * data);
    static void WriteTo(int32_t Control, uint32_t address, uint8_t * data);
};