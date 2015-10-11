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

#include <common/stdtypes.h>

/*
 * To do:  Use fixed-size types for MIPS union members, not WINAPI types.
 * On Win32 x86, `unsigned long DWORD` versus `int32_t` is the only conflict.
 */
typedef unsigned char    BYTE;
typedef unsigned short   WORD;
typedef unsigned long    DWORD;
typedef unsigned __int64 QWORD;
typedef void *           HANDLE;
typedef int              BOOL;

union MIPS_WORD {
	long           W;
	unsigned long  UW;
	short          HW[2];
	unsigned short UHW[2];
	char           B[4];
	unsigned char  UB[4];

    float       F;
};

union MIPS_DWORD {
	__int64          DW;
	unsigned __int64 UDW;
	long             W[2];
	unsigned long    UW[2];
	short            HW[4];
	unsigned short   UHW[4];
	char             B[8];
	unsigned char    UB[8];

    double      D;
    float       F[2];
};
