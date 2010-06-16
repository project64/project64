class CRecompMemory :
	protected CX86Ops
{
protected:
	CRecompMemory();
	~CRecompMemory();

	bool AllocateMemory ( void );
	void CheckRecompMem ( void );
	void Reset          ( void );
	void ShowMemUsed    ( void );
	
	inline BYTE * RecompPos ( void ) const { return m_RecompPos; }

private:
	BYTE          * m_RecompCode;
	DWORD           m_RecompSize;
	
	enum { MaxCompileBufferSize      = 0x03C00000 };
	enum { InitialCompileBufferSize  = 0x00500000 };
	enum { IncreaseCompileBufferSize = 0x00100000 };
};