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

class CSettingTypeApplicationPath :
	public CSettingTypeApplication
{
private:
	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, LPCSTR DefaultValue );
	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, bool DefaultValue );
	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, DWORD DefaultValue );

public:
	virtual ~CSettingTypeApplicationPath();

	CSettingTypeApplicationPath(LPCSTR Section, LPCSTR Name, SettingID DefaultSetting );

	//return the values
	virtual bool Load   ( int Index, stdstr & Value ) const;
};

