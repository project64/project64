class CRecompiler :
	public CRecompilerSettings,
	private CRecompMemory,
	private CFunctionMap
{
public:

	enum REMOVE_REASON {
		Remove_InitialCode,
		Remove_Cache,
		Remove_ProtectedMem,
		Remove_ValidateFunc,
		Remove_TLB,
		Remove_DMA,
	};

public:
	CRecompiler (CProfiling & Profile, bool & EndEmulation, bool SyncSystem);
	~CRecompiler (void);

	void Run             ( void );
	void ResetRecompCode ( void );

	void CompileSystemCheck (DWORD TargetPC, CRegInfo &RegSet);
	void UpdateCounters ( DWORD * Cycles, DWORD * RandomMod, BOOL CheckTimer);
	void CompileExit ( CCodeSection * Section, DWORD JumpPC, DWORD TargetPC, CRegInfo ExitRegSet, CExitInfo::EXIT_REASON reason, int CompileNow, void (*x86Jmp)(char * Label, DWORD Value));
	bool GenerateX86Code (CBlockInfo & BlockInfo, CCodeSection * Section, DWORD Test );
	void GenerateSectionLinkage (CCodeSection * Section);

	//Self modifying code methods
	bool ClearRecompCode_Virt ( DWORD VirtualAddress, int length, REMOVE_REASON Reason );
	bool ClearRecompCode_Phys ( DWORD PhysicalAddress, int length, REMOVE_REASON Reason );

private:
	bool               const m_SyncSystem;
	CProfiling       & m_Profile; 
	bool             & m_EndEmulation;

	//Quick access to registers
	DWORD            & PROGRAM_COUNTER;
		
	//Functions
	CDelaySlotFunctionMap m_FunctionsDelaySlot;
	//CFunctionMap          m_Functions;
	
	CCompiledFunc * CompilerCode        ( void );
	CCompiledFunc * CompileDelaySlot    ( DWORD PC );
	bool            Compiler4300iBlock  ( CCompiledFunc * info );

	void CheckRecompMem ( void );

	// Compiling code
	bool AnalyseBlock         ( CBlockInfo & BlockInfo  );
	bool CreateSectionLinkage ( CCodeSection * Section );
	void CompileExitCode      ( CBlockInfo & BlockInfo );
	void DetermineLoop        ( CCodeSection * Section, DWORD Test, DWORD Test2, DWORD TestID);
	bool DisplaySectionInformation (CCodeSection * Section, DWORD ID, DWORD Test);
	bool FixConstants         ( CCodeSection * Section, DWORD Test );
	bool InheritParentInfo    ( CCodeSection * Section );
	void InheritConstants     ( CCodeSection * Section );
//	bool IsAllParentLoops     ( CCodeSection * Section, CCodeSection * Parent, bool IgnoreIfCompiled, DWORD Test );
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
