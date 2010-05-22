#ifndef __USER_INTERFACE__H__
#define __USER_INTERFACE__H__
#pragma warning(disable:4786)
#include "Support.h"

#include "Multilanguage.h"
#include "Settings.h"

#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name

DECLARE_HANDLE (WND_HANDLE);
DECLARE_HANDLE (MENU_HANDLE);
DECLARE_HANDLE (DC_HANDLE);


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
    DC_HANDLE   hdc;
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
#include ".\\User Interface\\Cheats.h"
#include ".\\User Interface\\resource.h"
#include ".\\User Interface\\Settings Config.h"

#endif

