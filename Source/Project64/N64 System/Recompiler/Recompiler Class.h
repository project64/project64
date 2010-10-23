class CRecompiler :
	public CRecompilerSettings,
	public CFunctionMap,
	private CRecompMemory,
	private CSystemRegisters
{
public:

	enum REMOVE_REASON {
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
	CRecompiler (CProfiling & Profile, bool & EndEmulation );
	~CRecompiler (void);

	void Run             ( void );
	void ResetRecompCode ( void );

	bool GenerateX86Code (CCodeBlock & BlockInfo, CCodeSection * Section, DWORD Test );
	BYTE * CompileDelaySlot    ( DWORD PC );

	//Self modifying code methods
	void ClearRecompCode_Virt ( DWORD VirtualAddress, int length, REMOVE_REASON Reason );
	void ClearRecompCode_Phys ( DWORD PhysicalAddress, int length, REMOVE_REASON Reason );
	
	void ResetMemoryStackPos  ( void );

	inline DWORD & MemoryStackPos ( void ) { return m_MemoryStack; }

private:
	CCompiledFuncList  m_Functions;
	CProfiling       & m_Profile; 
	bool             & m_EndEmulation;
	DWORD              m_MemoryStack;

	//Quick access to registers
	DWORD            & PROGRAM_COUNTER;
	
	CCompiledFunc * CompilerCode        ( void );
	bool            Compiler4300iBlock  ( CCompiledFunc * info );

	// Compiling code
	bool CreateSectionLinkage ( CCodeSection * Section );
	bool DisplaySectionInformation (CCodeSection * Section, DWORD ID, DWORD Test);

	// Main loops for the different look up methods
	void RecompilerMain_VirtualTable          ( void );
	void RecompilerMain_VirtualTable_validate ( void );
	void RecompilerMain_ChangeMemory ( void );
	void RecompilerMain_Lookup       ( void );

	void RemoveFunction (CCompiledFunc * FunInfo, bool DelaySlot, REMOVE_REASON Reason );
};
