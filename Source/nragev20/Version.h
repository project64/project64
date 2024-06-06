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

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#ifndef VERSION_MAJOR
#define VERSION_MAJOR               4
#endif
#ifndef VERSION_MINOR
#define VERSION_MINOR               0
#endif
#ifndef VERSION_REVISION
#define VERSION_REVISION            0
#endif
#ifndef VERSION_BUILD
#define VERSION_BUILD               9999
#endif
#ifndef VERSION_PREFIX
#define VERSION_PREFIX              "Dev-"
#endif
#ifndef VERSION_BUILD_YEAR
#define VERSION_BUILD_YEAR          2022
#endif

#ifndef GIT_REVISION
#define GIT_REVISION                ""
#endif
#ifndef GIT_REVISION_SHORT
#define GIT_REVISION_SHORT          ""
#endif
#ifndef GIT_DIRTY
#define GIT_DIRTY                   ""
#endif
#ifndef GIT_VERSION
#define GIT_VERSION                 Unknown
#endif

#define VER_FILE_DESCRIPTION_STR    "N-Rage for Project64"
#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VER_FILE_VERSION_STR        VERSION_PREFIX STRINGIZE(VERSION_MAJOR)        \
                                    "." STRINGIZE(VERSION_MINOR)    \
                                    "." STRINGIZE(VERSION_REVISION) \
                                    "." STRINGIZE(VERSION_BUILD)    \
                                    "-" STRINGIZE(GIT_VERSION)

#define VER_PRODUCTNAME_STR         "N-Rage"
#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR   VER_PRODUCTNAME_STR ".dll"
#define VER_INTERNAL_NAME_STR       VER_PRODUCTNAME_STR
#define VER_COPYRIGHT_STR           "Copyright (C) " STRINGIZE(VERSION_BUILD_YEAR)

#ifdef _DEBUG
#define VER_VER_DEBUG             VS_FF_DEBUG
#else
#define VER_VER_DEBUG             0
#endif

#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               VER_VER_DEBUG
#define VER_FILETYPE                VFT_DLL
