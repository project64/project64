// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#ifndef TEXCACHE_H
#define TEXCACHE_H

void TexCacheInit();
void TexCache();
void ClearCache();

extern uint8_t * texture_buffer;

#endif //TEXCACHE_H
