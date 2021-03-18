#pragma once

// Some versions of Microsoft Visual C/++ compilers before Visual Studio 2010
// have <stdint.h> removed in favor of these non-standard built-in types:

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef signed __int8           int8_t;
typedef signed __int16          int16_t;
typedef signed __int32          int32_t;
typedef signed __int64          int64_t;

typedef unsigned __int8         uint8_t;
typedef unsigned __int16        uint16_t;
typedef unsigned __int32        uint32_t;
typedef unsigned __int64        uint64_t;
#else
#include <stdint.h>
#endif
