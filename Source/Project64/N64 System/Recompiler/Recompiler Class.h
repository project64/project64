class CRecompiler :
	public CRecompilerSettings
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

private:
	bool               const m_SyncSystem;
	CProfiling       & m_Profile; 
	bool             & m_EndEmulation;

	//Quick access to registers
	DWORD            & PROGRAM_COUNTER;
		
	//Functions
	CDelaySlotFunctionMap m_FunctionsDelaySlot;
	CFunctionMap          m_Functions;
	
	FUNCTION_INFO * CompilerCode        ( void );
	FUNCTION_INFO * CompileDelaySlot    ( DWORD PC );
	bool            Compiler4300iBlock  ( FUNCTION_INFO * info );

	// Compiling code
	bool AnalyseBlock         ( CBlockInfo & BlockInfo  );
	bool CreateSectionLinkage ( CBlockSection * Section );
	void CompileExitCode      ( CBlockInfo & BlockInfo );
	void DetermineLoop        ( CBlockSection * Section, DWORD Test, DWORD Test2, DWORD TestID);
	bool DisplaySectionInformation (CBlockSection * Section, DWORD ID, DWORD Test);
	bool FixConstants         ( CBlockSection * Section, DWORD Test );
	bool InheritParentInfo    ( CBlockSection * Section );
	void InheritConstants     ( CBlockSection * Section );
//	bool IsAllParentLoops     ( CBlockSection * Section, CBlockSection * Parent, bool IgnoreIfCompiled, DWORD Test );
	bool FillSectionInfo      ( CBlockSection * Section, STEP_TYPE StartStepType );
	void SyncRegState         ( CBlockSection * Section, CRegInfo * SyncTo );
	CBlockSection * ExistingSection( CBlockSection * StartSection, DWORD Addr, DWORD Test);

	// Main loops for the different look up methods
	void RecompilerMain_VirtualTable          ( void );
	void RecompilerMain_VirtualTable_validate ( void );
	void RecompilerMain_ChangeMemory ( void );
	void RecompilerMain_Lookup       ( void );

	void RemoveFunction (FUNCTION_INFO * FunInfo, bool DelaySlot, REMOVE_REASON Reason );

public:
	CRecompiler (CProfiling & Profile, bool & EndEmulation, bool SyncSystem);
	~CRecompiler (void);

	void Run             ( void );
	void ResetRecompCode ( void );

	void CompileSystemCheck (DWORD TargetPC, CRegInfo &RegSet);
	void UpdateCounters ( DWORD * Cycles, DWORD * RandomMod, BOOL CheckTimer);
	void CompileExit ( CBlockSection * Section, DWORD JumpPC, DWORD TargetPC, CRegInfo ExitRegSet, CExitInfo::EXIT_REASON reason, int CompileNow, void (*x86Jmp)(char * Label, DWORD Value));
	bool GenerateX86Code (CBlockInfo & BlockInfo, CBlockSection * Section, DWORD Test );
	void GenerateSectionLinkage (CBlockSection * Section);

	//Self modifying code methods
	bool ClearRecompCode_Virt ( DWORD VirtualAddress, int length, REMOVE_REASON Reason );
	bool ClearRecompCode_Phys ( DWORD PhysicalAddress, int length, REMOVE_REASON Reason );

};
