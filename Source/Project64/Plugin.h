/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include "Support.h"

//Plugin controller
#include "Plugins\Plugin Class.h"

//Base Plugin class, all plugin derive from this, handles core  functions
#include "Plugins\Plugin Base.h"

#include "Plugins\GFX Plugin.h"
#include "Plugins\Audio Plugin.h"
#include "Plugins\Controller Plugin.h"
#include "Plugins\RSP Plugin.h"

#include "Plugins\Plugin List.h"