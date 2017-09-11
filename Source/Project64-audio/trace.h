/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#include <Common/TraceModulesCommon.h>
#include <Common/Trace.h>

enum TraceModuleAndroidAudio
{
    TraceAudioInitShutdown = MaxTraceModuleCommon,
    TraceAudioInterface,
    TraceAudioDriver,
    MaxTraceModulePluginAudio,
};

void SetupTrace(void);
void StartTrace(void);
