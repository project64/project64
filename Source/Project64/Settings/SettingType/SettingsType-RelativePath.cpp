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
#include "SettingsType-RelativePath.h"

CSettingTypeRelativePath::CSettingTypeRelativePath(LPCSTR Path, LPCSTR FileName)
{
	m_FileName = CPath(CPath::MODULE_DIRECTORY,FileName);
	m_FileName.AppendDirectory(Path);
}

CSettingTypeRelativePath::~CSettingTypeRelativePath ( void )
{
}

bool CSettingTypeRelativePath::Load ( int /*Index*/, stdstr & value ) const
{
	value = (LPCSTR)m_FileName;
	return true;
}

//return the default values
void CSettingTypeRelativePath::LoadDefault ( int /*Index*/, bool & /*Value*/   ) const
{
	Notify().BreakPoint(__FILEW__,__LINE__);
}

void CSettingTypeRelativePath::LoadDefault ( int /*Index*/, ULONG & /*Value*/  ) const
{
	Notify().BreakPoint(__FILEW__,__LINE__); 
}

void CSettingTypeRelativePath::LoadDefault ( int /*Index*/, stdstr & /*Value*/ ) const
{
	Notify().BreakPoint(__FILEW__,__LINE__); 
}

void CSettingTypeRelativePath::Save ( int /*Index*/, bool /*Value*/ )
{
	Notify().BreakPoint(__FILEW__,__LINE__); 
}

void CSettingTypeRelativePath::Save ( int /*Index*/, ULONG /*Value*/ )
{
	Notify().BreakPoint(__FILEW__,__LINE__); 
}

void CSettingTypeRelativePath::Save ( int /*Index*/, const stdstr & Value )
{
	m_FileName = CPath(CPath::MODULE_DIRECTORY,Value.c_str());
}

void CSettingTypeRelativePath::Save ( int /*Index*/, const char * Value )
{
	m_FileName = CPath(CPath::MODULE_DIRECTORY,Value);
}

void CSettingTypeRelativePath::Delete ( int /*Index*/ )
{
	Notify().BreakPoint(__FILEW__,__LINE__); 
}
