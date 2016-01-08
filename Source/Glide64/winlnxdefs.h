/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
* Copyright (c) 2012-2013 balrog, wahrhaft
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


#ifndef WINLNXDEFS_H
#define WINLNXDEFS_H

#define wxNO_GL_LIB
#define wxNO_HTML_LIB
#define wxNO_ADV_LIB
#define wxNO_ZLIB_LIB
#define wxNO_TIFF_LIB
#define wxNO_EXPAT_LIB
#define wxNO_REGEX_LIB
#define wxNO_XML_LIB
#define wxNO_NET_LIB
#define wxNO_QA_LIB
#define wxNO_XRC_LIB
#define wxNO_AUI_LIB
#define wxNO_PROPGRID_LIB
#define wxNO_RIBBON_LIB
#define wxNO_RICHTEXT_LIB
#define wxNO_MEDIA_LIB
#define wxNO_STC_LIB


#include <wx/wx.h>
#include <wx/dynlib.h>
#include <wx/filename.h>
#include <wx/datetime.h>

#define TRUE 1
#define FALSE 0

#include <Common/stdtypes.h>

#ifndef _WIN32

typedef union _LARGE_INTEGER
{
   struct
     {
    uint32_t LowPart;
    uint32_t HighPart;
     } s;
   struct
     {
    uint32_t LowPart;
    uint32_t HighPart;
     } u;
   long long QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#define WINAPI

#endif

#endif
