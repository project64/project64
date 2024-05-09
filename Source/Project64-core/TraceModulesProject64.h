#pragma once
#include <Common/TraceModulesCommon.h>

enum TraceModuleProject64
{
    TraceSettings = MaxTraceModuleCommon,
    TraceUnknown,
    TraceAppInit,
    TraceAppCleanup,
    TraceN64System,
    TracePlugins,
    TraceVideoPlugin,
    TraceAudioPlugin,
    TraceControllerPlugin,
    TraceRSPPlugin,
    TraceRSP,
    TraceAudio,
    TraceRegisterCache,
    TraceRecompiler,
    TraceTLB,
    TraceUserInterface,
    TraceRomList,
    TraceExceptionHandler,
    MaxTraceModuleProject64,
};
