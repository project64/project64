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

//****************************************************************
// 8-bit Horizontal Mirror

extern "C" void asmMirror8bS (int tex, int start, int width, int height, int mask, int line, int full, int count);
extern "C" void asmWrap8bS (int tex, int start, int height, int mask, int line, int full, int count);
extern "C" void asmClamp8bS (int tex, int constant, int height,int line, int full, int count);

void Mirror8bS (wxUint32 tex, wxUint32 mask, wxUint32 max_width, wxUint32 real_width, wxUint32 height)
{
  if (mask == 0) return;

  wxUint32 mask_width = (1 << mask);
  wxUint32 mask_mask = (mask_width-1);
  if (mask_width >= max_width) return;
  int count = max_width - mask_width;
  if (count <= 0) return;
  int line_full = real_width;
  int line = line_full - (count);
  if (line < 0) return;
  wxUint32 start = tex + (mask_width);
  asmMirror8bS (tex, start, mask_width, height, mask_mask, line, line_full, count);
}

//****************************************************************
// 8-bit Horizontal Wrap (like mirror) ** UNTESTED **

void Wrap8bS (wxUint32 tex, wxUint32 mask, wxUint32 max_width, wxUint32 real_width, wxUint32 height)
{
  if (mask == 0) return;

  wxUint32 mask_width = (1 << mask);
  wxUint32 mask_mask = (mask_width-1) >> 2;
  if (mask_width >= max_width) return;
  int count = (max_width - mask_width) >> 2;
  if (count <= 0) return;
  int line_full = real_width;
  int line = line_full - (count << 2);
  if (line < 0) return;
  wxUint32 start = tex + (mask_width);
  asmWrap8bS (tex, start, height, mask_mask, line, line_full, count);
}

//****************************************************************
// 8-bit Horizontal Clamp

void Clamp8bS (wxUint32 tex, wxUint32 width, wxUint32 clamp_to, wxUint32 real_width, wxUint32 real_height)
{
  if (real_width <= width) return;

  wxUint32 dest = tex + (width);
  wxUint32 constant = dest-1;
  int count = clamp_to - width;

  int line_full = real_width;
  int line = width;
  asmClamp8bS (dest, constant, real_height, line, line_full, count);
}

//****************************************************************
// 8-bit Vertical Mirror

void Mirror8bT (wxUint32 tex, wxUint32 mask, wxUint32 max_height, wxUint32 real_width)
{
  if (mask == 0) return;

  wxUint32 mask_height = (1 << mask);
  wxUint32 mask_mask = mask_height-1;
  if (max_height <= mask_height) return;
  int line_full = real_width;

  wxUint32 dst = tex + mask_height * line_full;

  for (wxUint32 y=mask_height; y<max_height; y++)
  {
    if (y & mask_height)
    {
      // mirrored
      memcpy ((void*)dst, (void*)(tex + (mask_mask - (y & mask_mask)) * line_full), line_full);
    }
    else
    {
      // not mirrored
      memcpy ((void*)dst, (void*)(tex + (y & mask_mask) * line_full), line_full);
    }

    dst += line_full;
  }
}

//****************************************************************
// 8-bit Vertical Wrap

void Wrap8bT (wxUint32 tex, wxUint32 mask, wxUint32 max_height, wxUint32 real_width)
{
  if (mask == 0) return;

  wxUint32 mask_height = (1 << mask);
  wxUint32 mask_mask = mask_height-1;
  if (max_height <= mask_height) return;
  int line_full = real_width;

  wxUint32 dst = tex + mask_height * line_full;

  for (wxUint32 y=mask_height; y<max_height; y++)
  {
    // not mirrored
    memcpy ((void*)dst, (void*)(tex + (y & mask_mask) * line_full), line_full);

    dst += line_full;
  }
}

//****************************************************************
// 8-bit Vertical Clamp

void Clamp8bT (wxUint32 tex, wxUint32 height, wxUint32 real_width, wxUint32 clamp_to)
{
  int line_full = real_width;
  wxUint32 dst = tex + height * line_full;
  wxUint32 const_line = dst - line_full;

  for (wxUint32 y=height; y<clamp_to; y++)
  {
    memcpy ((void*)dst, (void*)const_line, line_full);
    dst += line_full;
  }
}
