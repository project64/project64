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
