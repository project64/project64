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

class CDelaySlotFunctionMap
{
	typedef std::map<DWORD,CCompiledFunc *> FUNCTION_MAP;

	FUNCTION_MAP FunctionMap;

public:
	CDelaySlotFunctionMap();
	~CDelaySlotFunctionMap();

	CCompiledFunc* AddFunctionInfo(DWORD vAddr, DWORD pAddr);
	CCompiledFunc* FindFunction(DWORD vAddr, int Length);
	CCompiledFunc* FindFunction(DWORD vAddr) const;

	void Remove(CCompiledFunc* info);
	void Reset();
};
