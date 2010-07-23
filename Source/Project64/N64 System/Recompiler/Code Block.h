class CCodeBlock :
	private CRecompilerOps
{
public:
	CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos, bool bDelaySlot );
	
	bool Compile ( void );

	inline DWORD    VAddrEnter ( void ) const { return m_VAddrEnter; }
	inline DWORD	VAddrFirst ( void ) const { return m_VAddrFirst; }
	inline DWORD	VAddrLast  ( void ) const { return m_VAddrLast;  }
	inline BYTE *   CompiledLocation ( void ) const { return m_CompiledLocation; }
	inline int      NoOfSections ( void ) const { return m_NoOfSections; }
	inline const CCodeSection & EnterSection ( void ) const { return m_EnterSection; }
	inline DWORD    NextTest   ( void ) const { return m_EnterSection.m_Test + 1; }
	inline bool     bDelaySlot ( void ) const { return m_bDelaySlot; }
	inline const MD5Digest & Hash ( void ) const { return m_Hash; }

	inline void	SetVAddrFirst ( DWORD VAddr ) { m_VAddrFirst = VAddr; }
	inline void	SetVAddrLast  ( DWORD VAddr ) { m_VAddrLast  = VAddr; }

	inline void    IncSectionCount ( void ) {  m_NoOfSections += 1; }

	CCodeSection * ExistingSection ( DWORD Addr ) { return m_EnterSection.ExistingSection(Addr,NextTest()); }

	EXIT_LIST       m_ExitInfo;

private:
	bool AnalyseBlock    ( void );
	void CompileExitCode ( void );

	DWORD	 	    m_VAddrEnter;
	DWORD	 	    m_VAddrFirst;       // the address of the first opcode in the block
	DWORD	 	    m_VAddrLast;        // the address of the first opcode in the block
	BYTE *		    m_CompiledLocation; // What address is this compiled at
	int             m_NoOfSections;     // The number of sections this block uses
	bool            m_bDelaySlot;
	CCodeSection    m_EnterSection;
	MD5Digest       m_Hash;
};
