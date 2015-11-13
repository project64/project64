/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#pragma warning(disable:4247)
#pragma warning(disable:4786)

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <common/MemTest.h>
#include <common/CriticalSection.h>
#include <windows.h>
#include <exception>
#include <shellapi.h>
#include "Multilanguage.h"
#include "User Interface.h"
#include "N64 System.h"
#include "Plugin.h"
#include "Support.h"
#include "Version.h"
#include <windows.h>
#include <mmsystem.h>
#include <Aclapi.h>

#include "3rd Party/Zip.h"
#include "3rd Party/7zip.h"
