//#include <windows.h>

#ifdef toremove
enum MemorySize { _8Bit, _16Bit, _32Bit, _64Bit };

class CC_Core;
#endif

class CMipsMemory_CallBack {
public:
	
	//Protected memory has been written to, returns true if that memory has been unprotected
	virtual bool WriteToProtectedMemory (DWORD Address, int length) = 0;
};

class CCodeSection;
class CRegInfo;

class CMipsMemory
{
public:
	virtual BYTE * Rdram        ( void ) = 0;
	virtual DWORD  RdramSize    ( void ) = 0;
	virtual BYTE * Dmem         ( void ) = 0;
	virtual BYTE * Imem         ( void ) = 0;
	virtual BYTE * PifRam       ( void ) = 0;
	
	virtual BOOL  LB_VAddr     ( DWORD VAddr, BYTE & Value ) = 0;
	virtual BOOL  LH_VAddr     ( DWORD VAddr, WORD & Value ) = 0; 
	virtual BOOL  LW_VAddr     ( DWORD VAddr, DWORD & Value ) = 0;
	virtual BOOL  LD_VAddr     ( DWORD VAddr, QWORD & Value ) = 0;

	virtual BOOL  SB_VAddr     ( DWORD VAddr, BYTE Value ) = 0;
	virtual BOOL  SH_VAddr     ( DWORD VAddr, WORD Value ) = 0;
	virtual BOOL  SW_VAddr     ( DWORD VAddr, DWORD Value ) = 0;
	virtual BOOL  SD_VAddr     ( DWORD VAddr, QWORD Value ) = 0;

	virtual bool  ValidVaddr   ( DWORD VAddr ) const = 0;

	virtual int   MemoryFilter ( DWORD dwExptCode, void * lpExceptionPointer ) = 0;
	
	//Protect the Memory from being written to
	virtual void  ProtectMemory    ( DWORD StartVaddr, DWORD EndVaddr ) = 0;
	virtual void  UnProtectMemory  ( DWORD StartVaddr, DWORD EndVaddr ) = 0;

	//Compilation Functions
	virtual void ResetMemoryStack    ( CRegInfo	& RegInfo ) = 0;
	virtual void Compile_LB          ( CX86Ops::x86Reg Reg, DWORD Addr, BOOL SignExtend ) = 0;
	virtual void Compile_LH          ( CX86Ops::x86Reg Reg, DWORD Addr, BOOL SignExtend ) = 0;
	virtual void Compile_LW          ( CCodeSection * Section, CX86Ops::x86Reg Reg, DWORD Addr ) = 0;
	virtual void Compile_SB_Const    ( BYTE Value, DWORD Addr ) = 0;
	virtual void Compile_SB_Register ( CX86Ops::x86Reg Reg, DWORD Addr ) = 0;
	virtual void Compile_SH_Const    ( WORD Value, DWORD Addr ) = 0;
	virtual void Compile_SH_Register ( CX86Ops::x86Reg Reg, DWORD Addr ) = 0;
	virtual void Compile_SW_Const    ( DWORD Value, DWORD Addr ) = 0;
	virtual void Compile_SW_Register ( CRegInfo	& RegInfo, CX86Ops::x86Reg Reg, DWORD Addr ) = 0;

};

class CRSP_Plugin;

class CMipsMemoryVM :
	public CMipsMemory,
	public CTransVaddr,
	private CX86Ops
#ifdef toremove
		,

	public CTLB, 
	public CMemoryLabel/*,
	private CPIFRam*/
#endif
{
	//Make sure plugins can directly access this information
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CRSP_Plugin;
	friend CControl_Plugin;
	friend CN64System; //Need to manipulate all memory in loading/saveing save state
//	friend CC_Core;

#ifdef toremove
	CNotification * const _Notify; //Original Notify member used to notify the user when something occurs
	CN64System    * const _System;
	CN64Rom       * const _Rom2; //Current loaded ROM
	CRegisters    * const _Reg;
#endif
	CMipsMemory_CallBack * const m_CBClass;

#ifdef toremove
	//Save Chips accessed by memory
	/*CSram         * m_Sram;
	CFlashRam     * m_FlashRam;
	bool          m_SavesReadOnly;

*/
#endif

	//Memory Locations
	BYTE          * m_RDRAM, * m_DMEM, * m_IMEM, m_PIF_Ram[0x40];
	DWORD         m_AllocatedRdramSize;

	//Rom Information
	bool          m_RomMapped;
	BYTE *        m_Rom;
	DWORD         m_RomSize;
	bool          m_RomWrittenTo;
	DWORD         m_RomWroteValue;

	// Recompiler
	void          ** JumpTable/*, ** DelaySlotTable*/;
	BYTE          * m_RecompCode;
	DWORD           m_RecompSize;
	
	enum { MaxCompileBufferSize      = 0x03C00000 };
	enum { InitialCompileBufferSize  = 0x00500000 };
	enum { IncreaseCompileBufferSize = 0x00100000 };

	//Current Half line
	void UpdateHalfLine       ( void );
	DWORD         m_HalfLine;
	DWORD         m_MemoryStack;
	DWORD         m_TempValue;

	//Searching memory
	BYTE  *       m_MemoryState;
	DWORD         m_MemoryStateSize;

	//Initilizing and reseting information about the memory system
	void AllocateSystemMemory ( void );
	void InitalizeSystem      ( bool PostPif );
	void FixRDramSize         ( void );
	void FreeMemory           ( void );

public:
	       CMipsMemoryVM        ( CMipsMemory_CallBack * CallBack );
	      ~CMipsMemoryVM        ( void );
	
	BOOL   Initialize   ( void );
	
	BYTE * Rdram        ( void );
	DWORD  RdramSize    ( void );
	BYTE * Dmem         ( void );
	BYTE * Imem         ( void );
	BYTE * PifRam       ( void );

	BOOL  LB_VAddr     ( DWORD VAddr, BYTE & Value );
	BOOL  LH_VAddr     ( DWORD VAddr, WORD & Value ); 
	BOOL  LW_VAddr     ( DWORD VAddr, DWORD & Value );
	BOOL  LD_VAddr     ( DWORD VAddr, QWORD & Value );

	BOOL  SB_VAddr     ( DWORD VAddr, BYTE Value );
	BOOL  SH_VAddr     ( DWORD VAddr, WORD Value );
	BOOL  SW_VAddr     ( DWORD VAddr, DWORD Value );
	BOOL  SD_VAddr     ( DWORD VAddr, QWORD Value );

	int   MemoryFilter ( DWORD dwExptCode, void * lpExceptionPointer );
	
	//Protect the Memory from being written to
	void  ProtectMemory    ( DWORD StartVaddr, DWORD EndVaddr );
	void  UnProtectMemory  ( DWORD StartVaddr, DWORD EndVaddr );

	//Compilation Functions
	void ResetMemoryStack    ( CRegInfo	& RegInfo );
	void Compile_LB          ( CX86Ops::x86Reg Reg, DWORD Addr, BOOL SignExtend );
	void Compile_LH          ( CX86Ops::x86Reg Reg, DWORD Addr, BOOL SignExtend );
	void Compile_LW          ( CCodeSection * Section, CX86Ops::x86Reg Reg, DWORD Addr );
	void Compile_SB_Const    ( BYTE Value, DWORD Addr );
	void Compile_SB_Register ( CX86Ops::x86Reg Reg, DWORD Addr );
	void Compile_SH_Const    ( WORD Value, DWORD Addr );
	void Compile_SH_Register ( CX86Ops::x86Reg Reg, DWORD Addr );
	void Compile_SW_Const    ( DWORD Value, DWORD Addr );
	void Compile_SW_Register ( CRegInfo	& RegInfo, CX86Ops::x86Reg Reg, DWORD Addr );
	  
	//Functions for TLB notification
	void TLB_Mapped ( DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly );
	void TLB_Unmaped ( DWORD Vaddr, DWORD Len );
		  
	// CTransVaddr interface
	bool TranslateVaddr ( DWORD VAddr, DWORD &PAddr) const;
	bool ValidVaddr  ( DWORD VAddr ) const;
		  
		  
		  
		  
		  
	
	// Recompiler Memory
	bool AllocateRecompilerMemory ( bool AllocateJumpTable );
	inline void ** GetJumpTable ( void ) const { return JumpTable; }
	inline BYTE * GetRecompCode ( void ) const { return m_RecompCode; }
	inline DWORD GetRecompBufferSize ( void ) const { return m_RecompSize; }

	void CheckRecompMem ( BYTE * RecompPos );

#ifdef toremove
	bool   LoadPhysical32     ( DWORD PAddr, DWORD & Variable, MemorySize Size, bool SignExtend );
	bool   Load32             ( DWORD VAddr, DWORD & Variable, MemorySize Size, bool SignExtend );
	bool   Load64             ( DWORD VAddr, QWORD & Variable, MemorySize Size, bool SignExtend );
	bool   Store64            ( DWORD VAddr, QWORD Value, MemorySize Size );
	bool   StorePhysical64    ( DWORD PAddr, QWORD Value, MemorySize Size );
	
	inline DWORD RomFileSize ( void ) { return m_RomFileSize; }

	//Win32 exception handler
	void   MemoryFilterFailed      ( char * FailureType, DWORD MipsAddress,  DWORD x86Address, DWORD Value);
	int    SystemMemoryFilter      ( DWORD dwExptCode, void * lpExceptionPointer );
	DWORD  GetExceptionCodeFn        ( void );
	void * GetExceptionInformationFn ( void );


	//Searching for value
	enum SearchMemChangeState
	{
		SearchChangeState_Reset,
		SearchChangeState_Changed,
		SearchChangeState_Unchanged,
		SearchChangeState_Greater,
		SearchChangeState_Lessthan,
	};

	bool  SearchSetBaseForChanges ( void );
	bool  SearchForChanges        ( SearchMemChangeState SearchType, MemorySize Size, 
		                            DWORD &StartAddress, DWORD &Len,
									DWORD &OldValue,     DWORD &NewValue );
	bool  SearchForValue (DWORD Value, MemorySize Size, DWORD &StartAddress, DWORD &Len);
#endif
	
	// Labels
	LPCTSTR LabelName      ( DWORD Address ) const;

private:
	int  LB_NonMemory         ( DWORD PAddr, DWORD * Value, BOOL SignExtend );
	int  LH_NonMemory         ( DWORD PAddr, DWORD * Value, int SignExtend );
	int  LW_NonMemory         ( DWORD PAddr, DWORD * Value );

	int  SB_NonMemory         ( DWORD PAddr, BYTE Value );
	int  SH_NonMemory         ( DWORD PAddr, WORD Value );
	int  SW_NonMemory         ( DWORD PAddr, DWORD Value );

	mutable char m_strLabelName[100];

	//BIG look up table to quickly translate the tlb to real mem address
	DWORD * m_TLB_ReadMap;
	DWORD * m_TLB_WriteMap;
};

extern void ** JumpTable;
