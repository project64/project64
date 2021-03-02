// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
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
