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

class CCompiledFunc
{
	//constructor
	CCompiledFunc(); // not implemented

public:
	CCompiledFunc(const CCodeBlock & CodeBlock);

	typedef void (*Func)();

	//Get Private Information
	const DWORD  EnterPC   () const { return m_EnterPC; }
	const DWORD  MinPC     () const { return m_MinPC; }
	const DWORD  MaxPC     () const { return m_MaxPC; }
	const Func   Function  () const { return m_Function; }
	const MD5Digest&  Hash () const { return m_Hash; }

	CCompiledFunc*    Next () const { return m_Next; }
	void SetNext(CCompiledFunc* Next) { m_Next = Next; }

	QWORD MemContents(int i) { return m_MemContents[i]; }
	QWORD* MemLocation(int i) { return m_MemLocation[i]; }

private:
	//Information
	DWORD m_EnterPC; // The Entry PC
	DWORD m_MinPC;   // The Lowest PC in the function
	DWORD m_MaxPC;   // The Highest PC in the function

	MD5Digest m_Hash;
	//From querying the recompiler get information about the function
	Func  m_Function;

	CCompiledFunc* m_Next;
	
	//Validation
	QWORD m_MemContents[2], * m_MemLocation[2];
};

typedef std::map<DWORD, CCompiledFunc *> CCompiledFuncList;
