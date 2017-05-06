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
#ifndef TEXBUFFER_H
#define TEXBUFFER_H

int OpenTextureBuffer(COLOR_IMAGE & cimage);

int CloseTextureBuffer(int draw = FALSE);

int CopyTextureBuffer(COLOR_IMAGE & fb_from, COLOR_IMAGE & fb_to);

int CopyDepthBuffer();

int SwapTextureBuffer();

int FindTextureBuffer(uint32_t addr, uint16_t width);

#endif  // ifndef TEXBUFFER
