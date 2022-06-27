// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#pragma once

#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <stddef.h>		// offsetof
#include <Common/MemTest.h>
#include "Config.h"
#include "Settings.h"
#include "rdp.h"

#if defined __VISUALC__
#define GLIDE64_TRY __try
#define GLIDE64_CATCH __except (EXCEPTION_EXECUTE_HANDLER)
#else
#define GLIDE64_TRY try
#define GLIDE64_CATCH catch (...)
#endif

#include <Project64-plugin-spec/Video.h>

//********
// Logging

// ********************************
// ** TAKE OUT BEFORE RELEASE!!! **
//#define LOG_UCODE

//  note that some of these things are inserted/removed
//  from within the code & may not be changed by this define.

// ********************************

//#define CATCH_EXCEPTIONS	// catch exceptions so it doesn't freeze and will report
// "The gfx plugin has caused an exception" instead.

// Usually enabled
#define LARGE_TEXTURE_HANDLING	// allow large-textured objects to be split?

extern unsigned int BMASK;

extern uint32_t update_screen_count;

extern int GfxInitDone;
extern bool g_romopen;
extern int to_fullscreen;

extern int ev_fullscreen;

extern int exception;

int InitGfx();
void ReleaseGfx();

// The highest 8 bits are the segment # (1-16), and the lower 24 bits are the offset to
// add to it.
__inline uint32_t segoffset(uint32_t so)
{
    return (rdp.segment[(so >> 24) & 0x0f] + (so&BMASK))&BMASK;
}

extern GFX_INFO gfx;
extern bool no_dlist;