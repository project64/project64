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

extern unsigned long swap32by8(unsigned long word);

class CMipsMemoryVM :
	public CMipsMemory,
	public CTransVaddr,
	private CRecompilerOps,
	private R4300iOp,
	private CPifRam,
	private CFlashram,
	private CSram,
	private CDMA
{
public:
	CMipsMemoryVM(CMipsMemory_CallBack * CallBack, bool SavesReadOnly);
	~CMipsMemoryVM();
	
	static void ReserveMemory();
	static void FreeReservedMemory();

	bool   Initialize   ();
	void   Reset        ( bool EraseMemory );
	
	BYTE * Rdram        ();
	DWORD  RdramSize    ();
	BYTE * Dmem         ();
	BYTE * Imem         ();
	BYTE * PifRam       ();

	bool  LB_VAddr     ( DWORD VAddr, BYTE & Value );
	bool  LH_VAddr     ( DWORD VAddr, WORD & Value ); 
	bool  LW_VAddr     ( DWORD VAddr, DWORD & Value );
	bool  LD_VAddr     ( DWORD VAddr, QWORD & Value );

	bool  LB_PAddr     ( DWORD PAddr, BYTE & Value );
	bool  LH_PAddr     ( DWORD PAddr, WORD & Value ); 
	bool  LW_PAddr     ( DWORD PAddr, DWORD & Value );
	bool  LD_PAddr     ( DWORD PAddr, QWORD & Value );

	bool  SB_VAddr     ( DWORD VAddr, BYTE Value );
	bool  SH_VAddr     ( DWORD VAddr, WORD Value );
	bool  SW_VAddr     ( DWORD VAddr, DWORD Value );
	bool  SD_VAddr     ( DWORD VAddr, QWORD Value );

	bool  SB_PAddr     ( DWORD PAddr, BYTE Value );
	bool  SH_PAddr     ( DWORD PAddr, WORD Value );
	bool  SW_PAddr     ( DWORD PAddr, DWORD Value );
	bool  SD_PAddr     ( DWORD PAddr, QWORD Value );

	int   MemoryFilter(DWORD dwExptCode, void * lpExceptionPointer);
	void  UpdateFieldSerration(unsigned int interlaced);
	
	//Protect the Memory from being written to
	void  ProtectMemory(DWORD StartVaddr, DWORD EndVaddr);
	void  UnProtectMemory(DWORD StartVaddr, DWORD EndVaddr);

	//Compilation Functions
	void ResetMemoryStack();

	void Compile_LB();
	void Compile_LBU();
	void Compile_LH();
	void Compile_LHU();
	void Compile_LW();
	void Compile_LL();
	void Compile_LWC1();
	void Compile_LWU();
	void Compile_LWL();
	void Compile_LWR();
	void Compile_LD();
	void Compile_LDC1();
	void Compile_LDL();
	void Compile_LDR();
	void Compile_SB();
	void Compile_SH();
	void Compile_SW();
	void Compile_SWL();
	void Compile_SWR();
	void Compile_SD();
	void Compile_SDL();
	void Compile_SDR();
	void Compile_SC();
	void Compile_SWC1();
	void Compile_SDC1();

	void ResetMemoryStack    ( CRegInfo& RegInfo );
	void Compile_LB          ( CX86Ops::x86Reg Reg, DWORD Addr, bool SignExtend );
	void Compile_LH          ( CX86Ops::x86Reg Reg, DWORD Addr, bool SignExtend );
	void Compile_LW          ( CX86Ops::x86Reg Reg, DWORD Addr );
	void Compile_SB_Const    ( BYTE Value, DWORD Addr );
	void Compile_SB_Register ( CX86Ops::x86Reg Reg, DWORD Addr );
	void Compile_SH_Const    ( WORD Value, DWORD Addr );
	void Compile_SH_Register ( CX86Ops::x86Reg Reg, DWORD Addr );
	void Compile_SW_Const    ( DWORD Value, DWORD Addr );

	void Compile_SW_Register ( CX86Ops::x86Reg Reg, DWORD Addr );
	  
	//Functions for TLB notification
	void TLB_Mapped(DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly);
	void TLB_Unmaped(DWORD Vaddr, DWORD Len);
		  
	// CTransVaddr interface
	bool TranslateVaddr(DWORD VAddr, DWORD &PAddr) const;
	bool ValidVaddr(DWORD VAddr) const;
	bool VAddrToRealAddr(DWORD VAddr, void * &RealAddress) const;
	
	// Labels
	LPCTSTR LabelName(DWORD Address) const;

private:
	CMipsMemoryVM();                                // Disable default constructor
	CMipsMemoryVM(const CMipsMemoryVM&);            // Disable copy constructor
	CMipsMemoryVM& operator=(const CMipsMemoryVM&); // Disable assignment

	void Compile_LW          ( bool ResultSigned, bool bRecordLLbit );
	void Compile_SW          ( bool bCheckLLbit );

	static void RdramChanged    ( CMipsMemoryVM * _this );
	static void ChangeSpStatus  ();
	static void ChangeMiIntrMask();

	bool LB_NonMemory         ( DWORD PAddr, DWORD * Value, bool SignExtend );
	bool LH_NonMemory         ( DWORD PAddr, DWORD * Value, bool SignExtend );
	bool LW_NonMemory         ( DWORD PAddr, DWORD * Value );

	bool SB_NonMemory         ( DWORD PAddr, BYTE Value );
	bool SH_NonMemory         ( DWORD PAddr, WORD Value );
	bool SW_NonMemory         ( DWORD PAddr, DWORD Value );

	void Compile_StoreInstructClean (x86Reg AddressReg, int Length );

	CMipsMemory_CallBack * const m_CBClass;

	//Memory Locations
	static BYTE   * m_Reserve1, * m_Reserve2;
	BYTE          * m_RDRAM, * m_DMEM, * m_IMEM;
	DWORD         m_AllocatedRdramSize;

	//Rom Information
	bool          m_RomMapped;
	BYTE *        m_Rom;
	DWORD         m_RomSize;
	bool          m_RomWrittenTo;
	DWORD         m_RomWroteValue;

	//Current Half line
	void UpdateHalfLine();
	DWORD         m_HalfLine;
	DWORD         m_HalfLineCheck;
	DWORD         m_FieldSerration;
	DWORD         m_TempValue;

	//Initializing and resetting information about the memory system
	void FreeMemory();

	mutable char m_strLabelName[100];

	//BIG look up table to quickly translate the tlb to real mem address
	size_t * m_TLB_ReadMap;
	size_t * m_TLB_WriteMap;
};
