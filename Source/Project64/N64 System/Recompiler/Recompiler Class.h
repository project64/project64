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

class CRecompiler :
	protected CDebugSettings,
	public CRecompilerSettings,
	public CFunctionMap,
	private CRecompMemory,
	private CSystemRegisters
{
public:

	enum REMOVE_REASON
	{
		Remove_InitialCode,
		Remove_Cache,
		Remove_ProtectedMem,
		Remove_ValidateFunc,
		Remove_TLB,
		Remove_DMA,
		Remove_StoreInstruc,
	};

	typedef void (* DelayFunc)(void);

public:
	CRecompiler (CRegisters & Registers, CProfiling & Profile, bool & EndEmulation );
	~CRecompiler (void);

	void Run             ( void );
	void Reset           ( void );
	void ResetRecompCode ( bool bAllocate );

	bool GenerateX86Code (CCodeBlock & BlockInfo, CCodeSection * Section, DWORD Test );

	//Self modifying code methods
	void ClearRecompCode_Virt ( DWORD VirtualAddress, int length, REMOVE_REASON Reason );
	void ClearRecompCode_Phys ( DWORD PhysicalAddress, int length, REMOVE_REASON Reason );
	
	void ResetMemoryStackPos  ( void );

	inline DWORD & MemoryStackPos ( void )
	{
		return m_MemoryStack;
	}

private:
	CRecompiler(void);							// Disable default constructor
	CRecompiler(const CRecompiler&);			// Disable copy constructor
	CRecompiler& operator=(const CRecompiler&);	// Disable assignment
	
	CCompiledFunc * CompilerCode        ( void );
	bool            Compiler4300iBlock  ( CCompiledFunc * info );

	// Compiling code
	bool CreateSectionLinkage ( CCodeSection * Section );
	bool DisplaySectionInformation (CCodeSection * Section, DWORD ID, DWORD Test);

	// Main loops for the different look up methods
	void RecompilerMain_VirtualTable          ( void );
	void RecompilerMain_VirtualTable_validate ( void );
	void RecompilerMain_ChangeMemory          ( void );
	void RecompilerMain_Lookup                ( void );
	void RecompilerMain_Lookup_TLB            ( void );
	void RecompilerMain_Lookup_validate       ( void );
	void RecompilerMain_Lookup_validate_TLB   ( void );

	void RemoveFunction (CCompiledFunc * FunInfo, bool DelaySlot, REMOVE_REASON Reason );

	CCompiledFuncList  m_Functions;
	CRegisters       & m_Registers;
	CProfiling       & m_Profile; 
	bool             & m_EndEmulation;
	DWORD              m_MemoryStack;

	//Quick access to registers
	DWORD            & PROGRAM_COUNTER;
};
