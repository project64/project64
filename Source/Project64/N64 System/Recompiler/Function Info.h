class CCompiledFunc
{
	//constructor
	CCompiledFunc ( void ); // not implemented

public:
	CCompiledFunc ( const CCodeBlock & CodeBlock );

	typedef void (* Func)(void);

	//Get Private Information
	inline const DWORD  EnterPC   ( void ) const { return m_EnterPC; }
	inline const DWORD  MinPC     ( void ) const { return m_MinPC; }
	inline const DWORD  MaxPC     ( void ) const { return m_MaxPC; }
	inline const Func   Function  ( void ) const { return m_Function; }

private:
	//Information
	DWORD m_EnterPC;		// The Entry PC
	DWORD m_MinPC;			// The Lowest PC in the function
	DWORD m_MaxPC;			// The Highest PC in the function

	//From querying the recompiler get information about the function
	Func  m_Function;
	
	//Validation
	//QWORD MemContents[2], * MemLocation[2];
};
