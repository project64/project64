// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#ifndef TEXBUFFER_H
#define TEXBUFFER_H

int OpenTextureBuffer(COLOR_IMAGE & cimage);

int CloseTextureBuffer(int draw = FALSE);

int CopyTextureBuffer(COLOR_IMAGE & fb_from, COLOR_IMAGE & fb_to);

int CopyDepthBuffer();

int SwapTextureBuffer();

int FindTextureBuffer(uint32_t addr, uint16_t width);

#endif  // ifndef TEXBUFFER
