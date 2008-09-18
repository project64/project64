enum MemorySize { _8Bit, _16Bit, _32Bit, _64Bit };

class CRSP_Plugin;
class CC_Core;


extern "C" {
int  r4300i_SB_NonMemory         ( DWORD PAddr, BYTE Value );
int  r4300i_SH_NonMemory         ( DWORD PAddr, WORD Value );
int  r4300i_SW_NonMemory         ( DWORD PAddr, DWORD Value );
int  r4300i_LB_NonMemory         ( DWORD PAddr, DWORD * Value, int SignExtend );
int  r4300i_LH_NonMemory         ( DWORD PAddr, DWORD * Value, int SignExtend );
int  r4300i_LW_NonMemory         ( DWORD PAddr, DWORD * Value );
}

class CMipsMemory_CallBack {
public:
	
	//Protected memory has been written to, returns true if that memory has been unprotected
	virtual bool WriteToProtectedMemory (DWORD Address, int length) = 0;
};

class CMipsMemory :
	public CTLB, 
	public CMemoryLabel/*,
	private CPIFRam*/
{
	//Make sure plugins can directly access this information
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CRSP_Plugin;
	friend CControl_Plugin;
	friend CN64System; //Need to manipulate all memory in loading/saveing save state
	friend CC_Core;

	CNotification * const _Notify; //Original Notify member used to notify the user when something occurs
	CN64System    * const _System;
	CN64Rom       * const _Rom; //Current loaded ROM
	CRegisters    * const _Reg;
	CMipsMemory_CallBack * const CBClass;

	//Save Chips accessed by memory
	/*CSram         * m_Sram;
	CFlashRam     * m_FlashRam;
	bool          m_SavesReadOnly;

	//Writing to the rom
	bool          m_WrittenToRom;
	DWORD         m_WroteToRom;
*/
	//Memory Locations
	BYTE          * RDRAM, * DMEM, * IMEM, * ROM, PIF_Ram[0x40];
	DWORD         m_RomFileSize;
	DWORD         m_AllocatedRdramSize;

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

	//Searching memory
	BYTE  *       m_MemoryState;
	DWORD         m_MemoryStateSize;

	//Initilizing and reseting information about the memory system
	void AllocateSystemMemory ( void );
	void InitalizeSystem      ( bool PostPif );
	void FixRDramSize         ( void );

	//Loading from/Storing to Non Memory
	bool LoadByte_NonMemory   ( DWORD PAddr, BYTE * Value )
	{ 
		DWORD dwValue;
		int Result = r4300i_LB_NonMemory(PAddr,&dwValue, false);
		*Value = (BYTE)dwValue;
		return Result != 0;
	}

	bool LoadHalf_NonMemory   ( DWORD PAddr, WORD * Value )  
	{ 
		DWORD dwValue;
		int Result = r4300i_LH_NonMemory(PAddr,&dwValue, false);
		*Value = (WORD)dwValue;
		return Result != 0;
	}
	bool LoadWord_NonMemory   ( DWORD PAddr, DWORD * Value ) { return r4300i_LW_NonMemory(PAddr,Value) != 0; }
	bool StoreByte_NonMemory  ( DWORD PAddr, BYTE Value ) { return r4300i_SB_NonMemory(PAddr,Value) != 0; }
	bool StoreHalf_NonMemory  ( DWORD PAddr, WORD Value ) { return r4300i_SH_NonMemory(PAddr,Value) != 0; }
	bool StoreWord_NonMemory  ( DWORD PAddr, DWORD Value ) { return r4300i_SW_NonMemory(PAddr,Value) != 0; }

	//DMAing data around memory
	/*void PI_DMA_Read          ( void );
	void PI_DMA_Write         ( void );
	void SI_DMA_READ          ( void );
	void SI_DMA_WRITE         ( void );
	void SP_DMA_READ          ( void );
	void SP_DMA_WRITE         ( void );

	//Fix up
	void WriteRDRAMSize       ( void );*/
public:
	       CMipsMemory        ( CMipsMemory_CallBack * CallBack, CN64System * System, CN64Rom * CurrentRom, CNotification * Notify, CRegisters * RegSet, bool SavesReadOnly = false );
	      ~CMipsMemory        ( void );
	
	//Get a pointer to the system registers
	CRegisters * SystemRegisters ( void ) { return _Reg; }
	
	// Recompiler Memory
	bool AllocateRecompilerMemory ( bool AllocateJumpTable );
	inline void ** GetJumpTable ( void ) const { return JumpTable; }
	inline BYTE * GetRecompCode ( void ) const { return m_RecompCode; }
	inline DWORD GetRecompBufferSize ( void ) const { return m_RecompSize; }

	void CheckRecompMem ( BYTE * RecompPos );

	//Accessing Memory
	bool   LoadPhysical32     ( DWORD PAddr, DWORD & Variable, MemorySize Size, bool SignExtend );
	bool   Load32             ( DWORD VAddr, DWORD & Variable, MemorySize Size, bool SignExtend );
	bool   Load64             ( DWORD VAddr, QWORD & Variable, MemorySize Size, bool SignExtend );
	bool   Store64            ( DWORD VAddr, QWORD Value, MemorySize Size );
	bool   StorePhysical64    ( DWORD PAddr, QWORD Value, MemorySize Size );

	//Protect the Memory from being written to
	void   ProtectMemory      ( DWORD StartVaddr, DWORD EndVaddr );
	void   UnProtectMemory    ( DWORD StartVaddr, DWORD EndVaddr );
	
	inline DWORD RomFileSize ( void ) { return m_RomFileSize; }
	inline DWORD RdramSize   ( void ) { return m_AllocatedRdramSize; }

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
};
