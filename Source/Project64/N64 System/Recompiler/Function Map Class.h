class CFunctionMap
{
protected:
	typedef CCompiledFunc *  PCCompiledFunc;
	typedef PCCompiledFunc * PCCompiledFunc_TABLE;

	CFunctionMap();
	~CFunctionMap();

	bool AllocateMemory ( void );
	void Reset          ( void );

public:
	inline PCCompiledFunc_TABLE * FunctionTable  ( void ) const { return m_FunctionTable; }
	inline PCCompiledFunc       * JumpTable      ( void ) const { return m_JumpTable; }

private:
	void CleanBuffers  ( void );

	PCCompiledFunc       * m_JumpTable;
	PCCompiledFunc_TABLE * m_FunctionTable;
};
