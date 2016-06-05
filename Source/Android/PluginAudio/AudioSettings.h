/****************************************************************************
 *                                                                          *
 * Project64 - A Nintendo 64 emulator.                                      *
 * http://www.pj64-emu.com/                                                 *
 * Copyright (C) 2016 Project64. All rights reserved.                       *
 *                                                                          *
 * License:                                                                 *
 * GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
 * version 2 of the License, or (at your option) any later version.         *
 *                                                                          *
 ****************************************************************************/
#pragma once
#include <Settings/Settings.h>

enum AudioSettingID
{
    Output_SwapChannels,
    Output_DefaultFrequency,
    Buffer_PrimarySize,
    Buffer_SecondarySize,
    Buffer_SecondaryNbr,
    Logging_LogAudioInitShutdown,
    Logging_LogAudioInterface,
};

void SetupAudioSettings ( void );