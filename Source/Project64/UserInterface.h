/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
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

#include <Project64-core/Multilanguage.h>
#include <Project64-core/Settings/SettingsClass.h>

typedef unsigned char    BYTE;
typedef unsigned short   WORD;
typedef unsigned long    DWORD;
typedef unsigned __int64 QWORD;
typedef void *           HANDLE;
typedef const char *     LPCSTR;

struct RECT_STRUCT
{
	long left;
	long top;
	long right;
	long bottom;
};

struct WINDOWS_PAINTSTRUCT
{
	HDC         hdc;
	int         fErase;
	RECT_STRUCT rcPaint;
	int         fRestore;
	int         fIncUpdate;
	BYTE        rgbReserved[32];
};

#define CALLBACK    __stdcall

class CN64System;

#ifndef BYPASS_WINDOWS_GUI
#define WINDOWS_UI
// Remove this to test compilation outside of the Windows ATL environment.
#endif

#include <WTLApp.h>
#include "UserInterface/MenuShortCuts.h"
#include "UserInterface/RomBrowser.h"
#include "UserInterface/GuiClass.h"
#include "UserInterface/MenuClass.h"
#include "UserInterface/MainMenuClass.h"
#include "UserInterface/NotificationClass.h"
#include <Project64-core/N64System/FramePerSecondClass.h>
#include "UserInterface/resource.h"
#include "UserInterface/SettingsConfig.h"
#include "UserInterface/CheatClassUI.h"
