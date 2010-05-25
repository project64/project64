typedef void (* RECOMP_FUNC)(void);

class CCompiledFunc
{
	//Information
	DWORD       m_VStartPC;			//The Virtual Address that the jump is going to
	DWORD       m_PStartPC;			//The Physical Address that the jump is going to
	DWORD       m_VEndPC;

	//From querying the recompiler get information about the function
	RECOMP_FUNC m_Function;
	
public:
	//constructor
	CCompiledFunc (DWORD VirtualStartAddress, DWORD PhysicalStartAddress );

	//Get Private Information
	inline const DWORD       VStartPC  ( void ) const { return m_VStartPC; }
	inline const DWORD       PStartPC  ( void ) const { return m_PStartPC; }
	inline const DWORD       VEndPC    ( void ) const { return m_VEndPC;   }
	inline const RECOMP_FUNC Function  ( void ) const { return m_Function; }
	
	//Set Private Information
	inline void  SetVEndPC        ( DWORD VEndPC ) { m_VEndPC = VEndPC;  }
	inline void  SetFunctionAddr  ( RECOMP_FUNC FunctionAddr ) { m_Function = FunctionAddr;  }


	//Functions
	bool  CompilerCodeBlock ( void );

	//Validation
	QWORD MemContents[2], * MemLocation[2];

	CCompiledFunc * Next;
};
