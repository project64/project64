// Project64 - A Nintendo 64 emulator
// http://www.pj64-emu.com/
// Copyright(C) 2001-2021 Project64
// Copyright(C) 2003-2009 Sergey 'Gonetz' Lipski
// Copyright(C) 2002 Dave2001
// GNU/GPLv2 licensed: https://gnu.org/licenses/gpl-2.0.html
#ifndef DEPTH_BUFFER_RENDER_H
#define DEPTH_BUFFER_RENDER_H

struct vertexi
{
    int x, y;       // Screen position in 16:16 bit fixed point
    int z;         // z value in 16:16 bit fixed point
};

extern uint16_t * zLUT;
void ZLUT_init();
void ZLUT_release();

void Rasterize(vertexi * vtx, int vertices, int dzdx);

#endif //DEPTH_BUFFER_RENDER_H
