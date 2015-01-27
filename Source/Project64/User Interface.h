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

#pragma warning(disable:4786)
#include "Support.h"

#include "Multilanguage.h"
#include "Settings.h"

typedef unsigned char    BYTE;
typedef unsigned short   WORD;
typedef unsigned long    DWORD;
typedef unsigned __int64 QWORD;
typedef void *           HANDLE;
typedef const char *     LPCSTR;

typedef struct
{
	long left;
	long top;
	long right;
	long bottom;
} RECT_STRUCT;

typedef struct {
    HDC   hdc;
    int        fErase;
    RECT_STRUCT rcPaint;
    int        fRestore;
    int        fIncUpdate;
    BYTE        rgbReserved[32];
} WINDOWS_PAINTSTRUCT;

#define CALLBACK    __stdcall

class CN64System;

#include <WTL App.h>
#include <User Interface/MenuShortCuts.h>
#include ".\\User Interface\\Rom Browser.h"
#include ".\\User Interface\\Gui Class.h"
#include ".\\User Interface\\Menu Class.h"
#include ".\\User Interface\\Menu Class.h"
#include ".\\User Interface\\Main Menu Class.h"
#include ".\\User Interface\\Notification Class.h"
#include ".\\User Interface\\Frame Per Second Class.h"
#include ".\\User Interface\\resource.h"
#include ".\\User Interface\\Settings Config.h"
