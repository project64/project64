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

#ifndef _COMMONINCLUDES_H_
#define _COMMONINCLUDES_H_

/*
#undef WIN32_LEAN_AND_MEAN
#pragma warning(disable:4201)
*/

#include <tchar.h>

#include "settings.h"
#include "resource.h"
#include "Debug.h"

#include "./ControllerSpecs/Controller #1.1.h"

#define P_malloc( size ) HeapAlloc( g_hHeap, 0, size )
#define P_free( memory ) HeapFree( g_hHeap, 0, memory )
#define P_realloc( memory, size ) \
    ( (memory == NULL) ? P_malloc(size) : HeapReAlloc( g_hHeap, 0, memory, size ) )

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif //ARRAYSIZE

#define ARRAYSIZE(array)        (sizeof(array) / sizeof((array)[0]))

#endif // #ifndef _COMMONINCLUDES_H_
