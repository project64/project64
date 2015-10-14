/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************
//
// Keys, used by Glide64. 
// Since key codes are different for WinAPI and SDL, this difference is managed here
// Created by Sergey 'Gonetz' Lipski, July 2009
//
//****************************************************************

#include "Gfx_1.3.h"

Glide64Keys::Glide64Keys()
{
_keys[G64_VK_CONTROL] = 306;
_keys[G64_VK_ALT]     = 308;
_keys[G64_VK_INSERT]  = 277;
_keys[G64_VK_LBUTTON] =   1;
_keys[G64_VK_UP]      = 273;
_keys[G64_VK_DOWN]    = 274;
_keys[G64_VK_LEFT]    = 276;
_keys[G64_VK_RIGHT]   = 275;
_keys[G64_VK_SPACE]   =  32;
_keys[G64_VK_BACK]    =   8;
_keys[G64_VK_SCROLL]  = 302;
_keys[G64_VK_1]       =  49;
_keys[G64_VK_2]       =  50;
_keys[G64_VK_3]       =  51;
_keys[G64_VK_4]       =  52;
_keys[G64_VK_5]       =  53;
_keys[G64_VK_6]       =  54;
_keys[G64_VK_7]       =  55;
_keys[G64_VK_8]       =  56;
_keys[G64_VK_9]       =  57;
_keys[G64_VK_0]       =  48;
_keys[G64_VK_A]       =  97;
_keys[G64_VK_B]       =  98;
_keys[G64_VK_D]       = 100;
_keys[G64_VK_G]       = 103;
_keys[G64_VK_Q]       = 113;
_keys[G64_VK_R]       = 114;
_keys[G64_VK_S]       = 115;
_keys[G64_VK_V]       = 118;
_keys[G64_VK_W]       = 119;
}
