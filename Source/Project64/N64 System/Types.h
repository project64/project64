/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

/*
 * Some compilers have <stdint.h> types built in to them internally.
 * In those cases, the "dependency" on <stdint.h> can be overlooked.
 */
#if defined(__STDC__) && (__STDC_VERSION__ >= 199901L)
#define C99_COMPLETE
#include <stdint.h>
#endif

/*
 * standard integer types usable by the MIPS ISA
 * also used by the Ultra64 OS
 */
#if defined(_MSC_VER)
typedef __int8                  i8;
typedef signed __int8           s8;
typedef unsigned __int8         u8;

typedef __int16                 i16;
typedef __int32                 i32;
typedef __int64                 i64;
typedef unsigned __int16        u16;
typedef unsigned __int32        u32;
typedef unsigned __int64        u64;
#elif defined(C99_COMPLETE) || 1
typedef int8_t                  i8;
typedef int8_t                  s8;
typedef uint8_t                 u8;

typedef int16_t                 i16;
typedef int32_t                 i32;
typedef int64_t                 i64;
typedef uint16_t                u16;
typedef uint32_t                u32;
typedef uint64_t                u64;
#elif !defined(_STDINT_H) || defined(__LP64__)
typedef char                    i8;
typedef signed char             s8;
typedef unsigned char           u8;

typedef short                   i16;
typedef int                     i32;
typedef long                    i64;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long           u64;
#else
#error Unable to emulate all of the types from MIPS ISA.
#endif

typedef i16                     s16;
typedef i32                     s32;
typedef i64                     s64;

typedef float                   f32;
typedef double                  f64;

typedef unsigned char    BYTE;
typedef unsigned short   WORD;
typedef unsigned long    DWORD;
typedef unsigned __int64 QWORD;
typedef void *           HANDLE;
typedef int              BOOL;

typedef union tagUWORD {
	long				W;
	float				F;
	unsigned long		UW;
	short				HW[2];
	unsigned short		UHW[2];
	char				B[4];
	unsigned char		UB[4];
} MIPS_WORD;

typedef union tagUDWORD {
	double				D;
	__int64				DW;
	unsigned __int64	UDW;
	long				W[2];
	float				F[2];
	unsigned long		UW[2];
	short				HW[4];
	unsigned short		UHW[4];
	char				B[8];
	unsigned char		UB[8];
} MIPS_DWORD;
