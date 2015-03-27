/*
 * RSP Compiler plug in for Project 64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
 *
 * pj64 homepage: www.pj64.net
 * 
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#ifndef __Types_h 
#define __Types_h 

/*
 * Some versions of Microsoft Visual C/++ compilers before Visual Studio 2010
 * have <stdint.h> removed in favor of these nonstandard built-in types:
 */
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

/*
 * pointer to RSP operation code functions or "func"
 * This is the type of all RSP interpreter and recompiler functions.
 */
typedef void(*p_func)(void);

typedef union tagUWORD {
    int32_t     W;
    uint32_t    UW;
    int16_t     HW[2];
    uint16_t    UHW[2];
    int8_t      B[4];
    uint8_t     UB[4];

    float       F;
} UWORD32;

typedef union tagUDWORD {
    int64_t     DW;
    uint64_t    UDW;
    int32_t     W[2];
    uint32_t    UW[2];
    int16_t     HW[4];
    uint16_t    UHW[4];
    int8_t      B[8];
    uint8_t     UB[8];

    double      D;
    float       F[2];
} UDWORD;

typedef union tagVect {
    int64_t     DW[2];
    uint64_t    UDW[2];
    int32_t     W[4];
    uint32_t    UW[4];
    int16_t     HW[8];
    uint16_t    UHW[8];
    int8_t      B[16];
    uint8_t     UB[16];

    double      FD[2];
    float       FS[4];
} VECTOR;

#endif
