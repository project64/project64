class CDelaySlotFunctionMap
{
	typedef std::map<DWORD,FUNCTION_INFO *> FUNCTION_MAP;

	FUNCTION_MAP FunctionMap;

public:
	CDelaySlotFunctionMap ( void );
	~CDelaySlotFunctionMap ( void );

	FUNCTION_INFO * AddFunctionInfo ( DWORD vAddr, DWORD pAddr );
	FUNCTION_INFO * FindFunction    ( DWORD vAddr, int Length );
	FUNCTION_INFO * FindFunction    ( DWORD vAddr ) const;

	void Remove ( FUNCTION_INFO * info );
	void Reset  ( void );
};
