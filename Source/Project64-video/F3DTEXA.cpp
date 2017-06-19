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
#pragma once
#include <Project64-video/rdp.h>
#include <Project64-video/Gfx_1.3.h>
#include "F3DTEXA.h"

void f3dttexa_loadtex()
{
    uint32_t cmd0, cmd1;
    cmd0 = rdp.cmd0;
    cmd1 = rdp.cmd1;

    rdp.cmd0 = 0x3d100000;
    rdp_settextureimage();

    rdp.cmd0 = 0x35100000;
    rdp.cmd1 = 0x07000000;
    rdp_settile();

    rdp.cmd0 = 0x33000000;
    rdp.cmd1 = 0x27000000 | (cmd0 & 0x00FFFFFF);
    rdp_loadblock();

    rdp.cmd0 = cmd0; //restore to original values
    rdp.cmd1 = cmd1;
}

void f3dttexa_settilesize()
{
    uint32_t cmd0, cmd1, firstHalf;
    cmd0 = rdp.cmd0;
    cmd1 = rdp.cmd1;

    firstHalf = (cmd1 & 0xFF000000) >> 15;

    rdp.cmd0 = 0x35400000 | firstHalf;
    rdp.cmd1 = cmd0 & 0x00FFFFFF;
    rdp_settile();

    rdp.cmd0 = 0x32000000;
    rdp.cmd1 = cmd1 & 0x00FFFFFF;
    rdp_settilesize();

    rdp.cmd0 = cmd0; //restore to original values
    rdp.cmd1 = cmd1;
}