#pragma once

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Recompiler/x86/x86RegInfo.h>

typedef CX86RegInfo CRegInfo;

#elif defined(__amd64__) || defined(_M_X64)

#include <Project64-core/N64System/Recompiler/x64-86/x64RegInfo.h>

typedef CX64RegInfo CRegInfo;

#elif defined(__arm__) || defined(_M_ARM)

#include <Project64-core/N64System/Recompiler/Arm/ArmRegInfo.h>

typedef CArmRegInfo CRegInfo;

#elif defined(__aarch64__)

#include <Project64-core/N64System/Recompiler/Aarch64/Aarch64RegInfo.h>
typedef CAarch64RegInfo CRegInfo;

#endif
