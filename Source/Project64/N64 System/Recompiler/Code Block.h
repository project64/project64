class CCodeBlock :
	private CRecompilerOps
{
public:
	CCodeBlock(DWORD VAddrEnter, BYTE * RecompPos);
	
	bool Compile ( void );

	inline DWORD    VAddrEnter ( void ) const { return m_VAddrEnter; }
	inline DWORD	VAddrFirst ( void ) const { return m_VAddrFirst; }
	inline DWORD	VAddrLast  ( void ) const { return m_VAddrLast;  }
	inline BYTE *   CompiledLocation ( void ) const { return m_CompiledLocation; }
	inline int      NoOfSections ( void ) const { return m_NoOfSections; }
	inline const CCodeSection & EnterSection ( void ) const { return m_EnterSection; }
	inline DWORD    NextTest   ( void ) const { return m_EnterSection.m_Test + 1; }

	inline void	SetVAddrFirst ( DWORD VAddr ) { m_VAddrFirst = VAddr; }
	inline void	SetVAddrLast  ( DWORD VAddr ) { m_VAddrLast  = VAddr; }

	EXIT_LIST       m_ExitInfo;

private:
	void AnalyseBlock    ( void );
	void CompileExitCode ( void );

	DWORD	 	    m_VAddrEnter;
	DWORD	 	    m_VAddrFirst;       // the address of the first opcode in the block
	DWORD	 	    m_VAddrLast;        // the address of the first opcode in the block
	BYTE *		    m_CompiledLocation; // What address is this compiled at
	int             m_NoOfSections;     // The number of sections this block uses
	CCodeSection    m_EnterSection;        
};
