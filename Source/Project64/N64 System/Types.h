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
