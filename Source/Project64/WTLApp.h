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

#pragma warning(push)
#pragma warning(disable : 4091) // warning C4091: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4302) // warning C4302: 'type cast': truncation from 'LPCTSTR' to 'WORD'
#pragma warning(disable : 4458) // warning C4458: declaration of 'dwCommonButtons' hides class member
#pragma warning(disable : 4838) // warning C4838: conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable : 4996) // warning C4996: 'GetVersionExA': was declared deprecated
#define _ATL_DISABLE_NOTHROW_NEW
#include <shellapi.h>
#include <atlbase.h>

#include <wtl/atlapp.h>
#include <wtl/atldlgs.h>
#include <wtl/atlframe.h>
#include <wtl/atlctrls.h>

#define _WTL_NO_CSTRING

#include <atlwin.h>
#include <wtl/atlmisc.h>
#include <wtl/atlcrack.h>

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

#include <Common/StdString.h>

#include "UserInterface/resource.h"
#include "UserInterface/WTLControls/numberctrl.h"
#include "UserInterface/WTLControls/ClistCtrl/ListCtrl.h"
#include "UserInterface/WTLControls/ModifiedComboBox.h"
#include "UserInterface/WTLControls/PartialGroupBox.h"
#include "UserInterface/WTLControls/ModifiedEditBox.h"
#include "UserInterface/WTLControls/ModifiedCheckBox.h"
