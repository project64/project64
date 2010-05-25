class CFunctionMap
{
protected:
	typedef CCompiledFunc *  PCCompiledFunc;
	typedef PCCompiledFunc * PCCompiledFunc_TABLE;

	CFunctionMap();
	~CFunctionMap();

	bool AllocateMemory ( void );

	PCCompiledFunc_TABLE * m_FunctionTable;

/*public:
	typedef CCompiledFunc *  PCCompiledFunc;
	typedef PCCompiledFunc * PCCompiledFunc_TABLE;

private:
	void Reset          ( bool AllocateMemory );

	PCCompiledFunc_TABLE * m_FunctionTable;

public:
	CFunctionMap ( void );
	~CFunctionMap ( void );

	CCompiledFunc * AddFunctionInfo ( DWORD vAddr, DWORD pAddr );
	CCompiledFunc * FindFunction    ( DWORD vAddr, int Length );
	
	static void * __fastcall CFunctionMap::CompilerFindFunction( CFunctionMap * _this, DWORD vAddr );
	inline CCompiledFunc * CFunctionMap::FindFunction( DWORD vAddr ) const
	{
		PCCompiledFunc_TABLE table = m_FunctionTable[vAddr >> 0xC];
		if (table)
		{
			PCCompiledFunc & info = table[(vAddr & 0xFFF) >> 2];
			if (info != NULL)
			{
				return info;
			}
		}
		return NULL;
	}

	PCCompiledFunc_TABLE * GetFunctionTable ( void ) { return m_FunctionTable; }
	
	inline void Reset (void) { Reset(true); }
	void Remove (CCompiledFunc * info);*/
};
