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
void StopTrace(void);
