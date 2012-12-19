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
#include "stdafx.h"
#include "SettingsType-Application.h"
#include "SettingsType-ApplicationPath.h"

CSettingTypeApplicationPath::CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting ) :
	CSettingTypeApplication(Section,Name,DefaultSetting)
{

}

CSettingTypeApplicationPath::~CSettingTypeApplicationPath()
{
}

bool CSettingTypeApplicationPath::Load ( int Index, stdstr & Value ) const
{
	bool bRes = CSettingTypeApplication::Load(Index,Value);
	if (bRes)
	{
		if (Value.substr(0,2) == ".\\" || Value.substr(0,2) == "./" ||
			Value.substr(0,3) == "..\\" || Value.substr(0,3) == "../")
		{
			CPath FullFilePath(CPath::MODULE_DIRECTORY), RelativePath(Value);
			FullFilePath.SetNameExtension(RelativePath.GetNameExtension().c_str());
			FullFilePath.AppendDirectory(RelativePath.GetDirectory().c_str());

			Value = FullFilePath;
		}
	}
	return bRes;
}
