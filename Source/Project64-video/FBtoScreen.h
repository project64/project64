/***************************************************************************
*                                                                          *
* Project64-video - A Nintendo 64 gfx plugin.                              *
* http://www.pj64-emu.com/                                                 *
* Copyright (C) 2017 Project64. All rights reserved.                       *
* Copyright (C) 2003-2009  Sergey 'Gonetz' Lipski                          *
* Copyright (C) 2002 Dave2001                                              *
*                                                                          *
* License:                                                                 *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                       *
* version 2 of the License, or (at your option) any later version.         *
*                                                                          *
****************************************************************************/
#ifndef FBtoSCREEN_H
#define FBtoSCREEN_H

typedef struct
{
    uint32_t addr;   //color image address
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t ul_x;
    uint32_t ul_y;
    uint32_t lr_x;
    uint32_t lr_y;
    uint32_t opaque;
} FB_TO_SCREEN_INFO;

bool DrawFrameBufferToScreen(FB_TO_SCREEN_INFO & fb_info);
void DrawDepthBufferToScreen(FB_TO_SCREEN_INFO & fb_info);

#endif  // #ifndef FBtoSCREEN_H
