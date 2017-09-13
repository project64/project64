/****************************************************************************
*                                                                           *
* Project64-audio - A Nintendo 64 audio plugin.                             *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
* Copyright (C) 2008-2012 Tillin9, Richard42                                *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

/* Default start-time size of primary buffer (in equivalent output samples).
This is the buffer where audio is loaded after it's extracted from n64's memory. */
enum { PRIMARY_BUFFER_SIZE = 16384 };

/* Size of a single secondary buffer, in output samples. This is the requested size of OpenSLES's
hardware buffer, this should be a power of two. */
enum { SECONDARY_BUFFER_SIZE = 1024 };

/* This is the requested number of OpenSLES's hardware buffers */
enum { SECONDARY_BUFFER_NBR = 2 };

/* This sets default frequency what is used if rom doesn't want to change it.
Probably only game that needs this is Zelda: Ocarina Of Time Master Quest
*NOTICE* We should try to find out why Demos' frequencies are always wrong
They tend to rely on a default frequency, apparently, never the same one ;) */
enum { DEFAULT_FREQUENCY = 33600 };

extern bool g_SwapChannels;
extern uint32_t g_GameFreq;