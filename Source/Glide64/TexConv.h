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

extern "C" void asmTexConv_ARGB1555_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int size);
extern "C" void asmTexConv_AI88_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int size);
extern "C" void asmTexConv_AI44_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int size);
extern "C" void asmTexConv_A8_ARGB4444(wxUIntPtr src, wxUIntPtr dst, int size);

void TexConv_ARGB1555_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 1;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 2 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 2
  asmTexConv_ARGB1555_ARGB4444(src, dst, size);
}

void TexConv_AI88_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 1;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 2 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 2
  asmTexConv_AI88_ARGB4444(src, dst, size);
}

void TexConv_AI44_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 2;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 4 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 4
  asmTexConv_AI44_ARGB4444(src, dst, size);
}

void TexConv_A8_ARGB4444 (wxUIntPtr src, wxUIntPtr dst, int width, int height)
{
  int size = (width * height) >> 2;	// Hiroshi Morii <koolsmoky@users.sourceforge.net>
  // 4 pixels are converted in one loop
  // NOTE: width * height must be a multiple of 4
  asmTexConv_A8_ARGB4444(src, dst, size);
}
