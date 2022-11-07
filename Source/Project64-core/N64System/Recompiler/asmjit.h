#pragma once

#define ASMJIT_STATIC

#ifdef new
#pragma push_macro("new")
#undef new
#include <asmjit\asmjit.h>
#pragma pop_macro("new")
#else
#include <asmjit\asmjit.h>
#endif
