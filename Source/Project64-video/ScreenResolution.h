/****************************************************************************
*                                                                           *
* Project64-video - A Nintendo 64 gfx plugin.                               *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2017 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#include <Common/stdtypes.h>

#ifdef ANDROID
void UpdateScreenResolution(int ScreenWidth, int ScreenHeight);
#else
const char ** getFullScreenResList(int32_t * Size);
uint32_t getFullScreenRes(uint32_t * width, uint32_t * height);
#endif

uint32_t GetScreenResolutionCount();
uint32_t GetDefaultScreenRes();
uint32_t GetScreenResWidth(uint32_t index);
uint32_t GetScreenResHeight(uint32_t index);
const char * GetScreenResolutionName(uint32_t index);

int GetCurrentResIndex(void);
uint32_t GetFullScreenResWidth(uint32_t index);
uint32_t GetFullScreenResHeight(uint32_t index);
bool EnterFullScreen(uint32_t index);
