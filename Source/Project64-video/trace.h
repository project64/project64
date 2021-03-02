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
void StopTrace(void);
