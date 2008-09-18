class CFunctionMap
{
public:
	typedef FUNCTION_INFO *  PFUNCTION_INFO;
	typedef PFUNCTION_INFO * PFUNCTION_INFO_TABLE;

private:
	void Reset          ( bool AllocateMemory );

	PFUNCTION_INFO_TABLE * m_FunctionTable;

public:
	CFunctionMap ( void );
	~CFunctionMap ( void );

	FUNCTION_INFO * AddFunctionInfo ( DWORD vAddr, DWORD pAddr );
	FUNCTION_INFO * FindFunction    ( DWORD vAddr, int Length );
	
	static void * __fastcall CFunctionMap::CompilerFindFunction( CFunctionMap * _this, DWORD vAddr );
	inline FUNCTION_INFO * CFunctionMap::FindFunction( DWORD vAddr ) const
	{
		PFUNCTION_INFO_TABLE table = m_FunctionTable[vAddr >> 0xC];
		if (table)
		{
			PFUNCTION_INFO & info = table[(vAddr & 0xFFF) >> 2];
			if (info != NULL)
			{
				return info;
			}
		}
		return NULL;
	}

	PFUNCTION_INFO_TABLE * GetFunctionTable ( void ) { return m_FunctionTable; }
	
	inline void Reset (void) { Reset(true); }
	void Remove (FUNCTION_INFO * info);
};
