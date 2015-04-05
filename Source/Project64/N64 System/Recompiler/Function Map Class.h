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

class CFunctionMap
{
protected:
	typedef CCompiledFunc *  PCCompiledFunc;
	typedef PCCompiledFunc * PCCompiledFunc_TABLE;

	CFunctionMap();
	~CFunctionMap();

	bool AllocateMemory ( void );
	void Reset          ( bool bAllocate);

public:
	inline PCCompiledFunc_TABLE * FunctionTable  ( void ) const
	{
		return m_FunctionTable;
	}
	inline PCCompiledFunc       * JumpTable      ( void ) const
	{
		return m_JumpTable;
	}

private:
	void CleanBuffers  ( void );

	PCCompiledFunc       * m_JumpTable;
	PCCompiledFunc_TABLE * m_FunctionTable;
};
