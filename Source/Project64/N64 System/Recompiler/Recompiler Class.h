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

private:
	CCompiledFuncList  m_Functions;
	CProfiling       & m_Profile; 
	bool             & m_EndEmulation;

	//Quick access to registers
	DWORD            & PROGRAM_COUNTER;
	
	CCompiledFunc * CompilerCode        ( void );
	bool            Compiler4300iBlock  ( CCompiledFunc * info );

	// Compiling code
	bool AnalyseBlock         ( CCodeBlock & BlockInfo  );
	bool CreateSectionLinkage ( CCodeSection * Section );
	void DetermineLoop        ( CCodeSection * Section, DWORD Test, DWORD Test2, DWORD TestID);
	bool DisplaySectionInformation (CCodeSection * Section, DWORD ID, DWORD Test);
	bool FixConstants         ( CCodeSection * Section, DWORD Test );
	bool InheritParentInfo    ( CCodeSection * Section );
	void InheritConstants     ( CCodeSection * Section );
	bool FillSectionInfo      ( CCodeSection * Section, STEP_TYPE StartStepType );
	void SyncRegState         ( CCodeSection * Section, CRegInfo * SyncTo );
	CCodeSection * ExistingSection( CCodeSection * StartSection, DWORD Addr, DWORD Test);

	// Main loops for the different look up methods
	void RecompilerMain_VirtualTable          ( void );
	void RecompilerMain_VirtualTable_validate ( void );
	void RecompilerMain_ChangeMemory ( void );
	void RecompilerMain_Lookup       ( void );

	void RemoveFunction (CCompiledFunc * FunInfo, bool DelaySlot, REMOVE_REASON Reason );
};
