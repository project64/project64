class CDelaySlotFunctionMap
{
	typedef std::map<DWORD,CCompiledFunc *> FUNCTION_MAP;

	FUNCTION_MAP FunctionMap;

public:
	CDelaySlotFunctionMap ( void );
	~CDelaySlotFunctionMap ( void );

	CCompiledFunc * AddFunctionInfo ( DWORD vAddr, DWORD pAddr );
	CCompiledFunc * FindFunction    ( DWORD vAddr, int Length );
	CCompiledFunc * FindFunction    ( DWORD vAddr ) const;

	void Remove ( CCompiledFunc * info );
	void Reset  ( void );
};
