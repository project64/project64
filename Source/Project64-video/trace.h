/****************************************************************************
*                                                                           *
* Project64-video - A Nintendo 64 gfx plugin.                               *
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

enum TraceModuleGlide64
{
    TraceSettings = MaxTraceModuleCommon,
    TraceUnknown,
    TraceGlide64,
    TraceInterface,
    TraceResolution,
    TraceGlitch,
    TraceRDP,
    TraceTLUT,
    TracePNG,
    TraceOGLWrapper,
    TraceRDPCommands,
    MaxTraceModuleGlide64,
};

void SetupTrace(void);
void StartTrace(void);
