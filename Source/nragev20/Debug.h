/*
N-Rage`s Dinput8 Plugin
(C) 2002, 2006  Norbert Wladyka

Author`s Email: norbert.wladyka@chello.at
Website: http://go.to/nrage

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <windows.h>

#ifdef _DEBUG

extern bool bDebug;

void _DebugAnsiFileWrite( LPCSTR szRemark );
void _WriteDatasA( LPCSTR Header, const int Control, const unsigned char * Data, const HRESULT hr );
void _cdecl _DebugWriteA( LPCSTR szFormat, ... );
void _cdecl _DebugWriteW( LPCWSTR szFormat, ... );
void _CloseDebugFile();
void _DebugFlush();

#define DebugWriteA _DebugWriteA
#define DebugWriteW _DebugWriteW
#define WriteDatasA( header, control, data, hr ) _WriteDatasA( header, control, data, hr )
#define CloseDebugFile() _CloseDebugFile()
#define DebugWriteByteA(str) DebugWriteA("%02X", str)
#define DebugWriteWordA(str) DebugWriteA("%04X", str)
#define DebugFlush() _DebugFlush()

#else // #ifndef _DEBUG
#define DebugWriteByteA(str)
#define DebugWriteWordA(str)
#define DebugWriteA ;//
#define DebugWriteW ;//
#define WriteDatasA(header,control,data,hr)
#define CloseDebugFile()
#define DebugFlush()

#endif // #ifdef _DEBUG

#ifdef _UNICODE
#define DebugWrite DebugWriteW
#else
#define DebugWrite DebugWriteA
#endif

#endif // #ifndef _DEBUG_H_
