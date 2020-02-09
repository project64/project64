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

#include <windows.h>

#include "settings.h"

#ifndef _NRINTERNATIONAL_
#define _NRINTERNATIONAL_

typedef struct LANGINFO_DEF
{
	int		Count;
	LANGID	LangID;
} LANGINFO;
typedef LANGINFO *PLANGINFO;

// only export these functions if UNICODE support is enabled
#ifdef _UNICODE
LANGID DetectLanguage();
HMODULE	LoadLanguageDLL(LANGID DesiredLanguage);
#endif // #ifdef _UNICODE

#endif // #ifndef _NRINTERNATIONAL_
