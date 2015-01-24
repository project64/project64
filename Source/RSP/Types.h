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
 * pointer to RSP operation code functions or "func"
 * This is the type of all RSP interpreter and recompiler functions.
 */
typedef void(*p_func)(void);

typedef union tagUWORD {
	long				W;
	float				F;
	unsigned long		UW;
	short				HW[2];
	unsigned short		UHW[2];
	char				B[4];
	unsigned char		UB[4];
} UWORD32;

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
} UDWORD;

typedef union tagVect {
	double				FD[2];
	__int64				DW[2];
	unsigned __int64	UDW[2];
	long				W[4];
	float				FS[4];
	unsigned long		UW[4];
	short				HW[8];
	unsigned short		UHW[8];
	char				B[16];
	unsigned char		UB[16];
} VECTOR;

#endif