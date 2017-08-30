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

class CDumpMemory;
class CDebugMemoryView;
class CDebugMemorySearch;
class CDebugTlb;
class CDebugCommandsView;
class CDebugScripts;
class CDebugSymbols;
class CDebugDMALogView;
class CDebugStackView;
class CDebugStackTrace;

class CDMALog;
class CBreakpoints;
class CScriptSystem;

__interface CDebugger
{
    virtual void TLBChanged ( void ) = 0;
	virtual bool CPUStepStarted ( void ) = 0;
	virtual void CPUStep ( void ) = 0;
	virtual void FrameDrawn ( void ) = 0;
};

class CDebuggerUI :
	public CDebugger
{
    CDumpMemory         * m_MemoryDump;
    CDebugMemoryView    * m_MemoryView;
    CDebugMemorySearch  * m_MemorySearch;
    CDebugTlb           * m_DebugTLB;
	CDebugCommandsView  * m_CommandsView;
	CDebugScripts       * m_Scripts;
	CDebugSymbols       * m_Symbols;
	CDebugDMALogView    * m_DMALogView;
	CDebugStackTrace    * m_StackTrace;
	CDebugStackView     * m_StackView;

	CBreakpoints        * m_Breakpoints;
	CScriptSystem       * m_ScriptSystem;
	CDMALog             * m_DMALog;

	void BreakpointHit(void);

protected:
    CDebuggerUI();
    virtual ~CDebuggerUI();

	void TLBChanged         ( void );
	bool CPUStepStarted     ( void );
	void CPUStep            ( void );
	void FrameDrawn         ( void );
	
public:
    void Debug_Reset                   ( void );
    void Debug_ShowMemoryDump          ( void );
    void Debug_ShowMemoryWindow        ( void );
    void Debug_ShowMemoryLocation      ( uint32_t Address, bool VAddr );
    void Debug_ShowMemorySearch        ( void );
    void Debug_ShowTLBWindow           ( void );
    void Debug_RefreshTLBWindow        ( void );
    void Debug_ShowCommandsWindow      ( void );
    void Debug_ShowCommandsLocation    ( uint32_t address, bool top );
    void Debug_ShowScriptsWindow       ( void );
    void Debug_LogScriptsWindow        ( const char* text );
    void Debug_ClearScriptsWindow      ( void );
    void Debug_RefreshScriptsWindow    ( void );
    void Debug_RefreshSymbolsWindow    ( void );
    void Debug_ShowSymbolsWindow       ( void );
    void Debug_ShowStackTrace          ( void );
    void Debug_ShowStackWindow         ( void );
    void Debug_RefreshStackWindow      ( void );
    void Debug_RefreshStackTraceWindow ( void );
    void Debug_ShowDMALogWindow        ( void );

	CBreakpoints* Breakpoints();
	CDebugSymbols* Symbols();
	CScriptSystem* ScriptSystem();
	CDebugScripts* ScriptConsole();
	CDMALog* DMALog();

    static void GameReset ( CDebuggerUI * _this );
};
