class CCodeBlock :
	private CRecompilerOps
{
public:
	CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos );
	
	bool Compile ( void );

	inline DWORD    VAddrEnter ( void ) const { return m_VAddrEnter; }
	inline DWORD	VAddrFirst ( void ) const { return m_VAddrFirst; }
	inline DWORD	VAddrLast  ( void ) const { return m_VAddrLast;  }
	inline BYTE *   CompiledLocation ( void ) const { return m_CompiledLocation; }
	inline int      NoOfSections ( void ) const { return m_NoOfSections; }
	inline const CCodeSection & EnterSection ( void ) const { return *m_EnterSection; }
	inline const MD5Digest & Hash ( void ) const { return m_Hash; }

	inline void	SetVAddrFirst ( DWORD VAddr ) { m_VAddrFirst = VAddr; }
	inline void	SetVAddrLast  ( DWORD VAddr ) { m_VAddrLast  = VAddr; }

	inline void    IncSectionCount ( void ) {  m_NoOfSections += 1; }

	CCodeSection * ExistingSection ( DWORD Addr ) { return m_EnterSection->ExistingSection(Addr,NextTest()); }
	bool SectionAccessible ( DWORD m_SectionID ) { return m_EnterSection->SectionAccessible(m_SectionID,NextTest()); }

	inline QWORD   MemContents(int i) const { return m_MemContents[i]; }
	inline QWORD * MemLocation(int i) const { return m_MemLocation[i]; }

	EXIT_LIST       m_ExitInfo;
	DWORD    NextTest   ( void );

private:
	bool AnalyseBlock    ( void );
	void CompileExitCode ( void );

	bool CreateBlockLinkage ( CCodeSection * EnterSection ) ;
	void DetermineLoops     ( void ) ;
	void LogSectionInfo     ( void ) ;
	bool SetSection         ( CCodeSection * & Section, CCodeSection * CurrentSection, DWORD TargetPC, bool LinkAllowed, DWORD CurrentPC );
	bool SetJumpInfo        ( CCodeSection * & Section, DWORD TargetPC, DWORD CurrentPC );
	bool AnalyzeInstruction ( DWORD PC, DWORD & TargetPC, DWORD & ContinuePC, bool & LikelyBranch, bool & IncludeDelaySlot, bool & EndBlock );

	DWORD	 	    m_VAddrEnter;
	DWORD	 	    m_VAddrFirst;       // the address of the first opcode in the block
	DWORD	 	    m_VAddrLast;        // the address of the first opcode in the block
	BYTE *		    m_CompiledLocation; // What address is this compiled at
	int             m_NoOfSections;     // The number of sections this block uses
	
	typedef std::map<DWORD,CCodeSection *> SectionMap;
	typedef std::list<CCodeSection *>      SectionList;

	SectionMap      m_SectionMap;
	SectionList     m_Sections;
	CCodeSection  * m_EnterSection;
	long            m_Test;
	MD5Digest       m_Hash;
	QWORD           m_MemContents[2];
	QWORD *         m_MemLocation[2];
};
