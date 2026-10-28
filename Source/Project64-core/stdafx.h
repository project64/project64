#if defined(_M_ARM64)
// intrin.h includes arm64_neon.h, which typedefs float32_t and float64_t to
// hardware floats. SoftFloat uses struct wrappers with those names. Rename the
// NEON typedefs in every translation unit that uses this precompiled header.
// InterpreterOps.cpp undefines the macros before including softfloat.h.
#define float32_t float32_t_neon
#define float64_t float64_t_neon
#endif

#include <string.h>

#include <Common/MemTest.h>
#include <Common/StdString.h>
#include <Common/Trace.h>
#include <stdint.h>

#include "Version.h"
#include <Project64-core/Multilanguage.h>
#include <Project64-core/Notification.h>
#include <Project64-core/Settings.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Settings/GameSettings.h>
#include <Project64-core/Settings/LoggingSettings.h>
#include <Project64-core/Settings/SystemSettings.h>
#include <Project64-core/TraceModulesProject64.h>
