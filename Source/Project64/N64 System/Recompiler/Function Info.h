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
	inline const MD5Digest & Hash ( void ) const { return m_Hash; }

	inline CCompiledFunc * Next ( void ) const { return m_Next; }
	inline void SetNext ( CCompiledFunc * Next ) { m_Next = Next; }
private:
	//Information
	DWORD m_EnterPC;		// The Entry PC
	DWORD m_MinPC;			// The Lowest PC in the function
	DWORD m_MaxPC;			// The Highest PC in the function

	MD5Digest m_Hash;
	//From querying the recompiler get information about the function
	Func  m_Function;

	CCompiledFunc * m_Next;
	
	//Validation
	//QWORD MemContents[2], * MemLocation[2];
};

typedef std::map<DWORD, CCompiledFunc *> CCompiledFuncList;