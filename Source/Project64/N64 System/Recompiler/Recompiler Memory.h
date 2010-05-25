class CRecompMemory
{
protected:
	CRecompMemory();
	~CRecompMemory();

	bool AllocateMemory ( void );

	BYTE          * m_RecompCode;
	DWORD           m_RecompSize;
	
	enum { MaxCompileBufferSize      = 0x03C00000 };
	enum { InitialCompileBufferSize  = 0x00500000 };
	enum { IncreaseCompileBufferSize = 0x00100000 };
};