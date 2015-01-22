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
//#define _WIN32_WINNT 0x0500

#define _ATL_DISABLE_NOTHROW_NEW
#include <shellapi.h>
#include <atlbase.h>
#include <wtl/atlapp.h>

class CPj64Module :
	public CAppModule
{
public:
	CPj64Module(void)
	{
		Init(NULL, GetModuleHandle(NULL));
	}
	virtual ~CPj64Module(void)
	{
		Term();
	}
};


extern CPj64Module _Module;

#define _WTL_NO_CSTRING

#include <atlwin.h>
#include <wtl/atlmisc.h>
#include <wtl/atlcrack.h>

#include <wtl/atlframe.h>
#include <wtl/atlctrls.h>
#include <wtl/atldlgs.h>

#include <Common/std string.h>

#include "User Interface/resource.h"
#include "User Interface/WTL Controls/numberctrl.h"
#include "User Interface/WTL Controls/ClistCtrl/ListCtrl.h"
#include "User Interface/WTL Controls/ModifiedComboBox.h"
#include "User Interface/WTL Controls/PartialGroupBox.h"
#include "User Interface/WTL Controls/ModifiedEditBox.h"
#include "User Interface/WTL Controls/ModifiedCheckBox.h"
