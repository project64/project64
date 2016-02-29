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
//#define _WIN32_WINNT 0x0500

#define _ATL_DISABLE_NOTHROW_NEW
#include <shellapi.h>
#include <atlbase.h>

#pragma warning(push)
#pragma warning(disable : 4996) // warning C4996: 'GetVersionExA': was declared deprecated
#include <wtl/atlapp.h> 
#pragma warning(pop)

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

#include <Common/StdString.h>

#include "UserInterface/resource.h"
#include "UserInterface/WTLControls/numberctrl.h"
#include "UserInterface/WTLControls/ClistCtrl/ListCtrl.h"
#include "UserInterface/WTLControls/ModifiedComboBox.h"
#include "UserInterface/WTLControls/PartialGroupBox.h"
#include "UserInterface/WTLControls/ModifiedEditBox.h"
#include "UserInterface/WTLControls/ModifiedCheckBox.h"
