class CFunctionMap
{
protected:
	typedef CCompiledFunc *  PCCompiledFunc;
	typedef PCCompiledFunc * PCCompiledFunc_TABLE;

	CFunctionMap();
	~CFunctionMap();

	bool AllocateMemory ( void );

	PCCompiledFunc_TABLE * m_FunctionTable;
	BYTE                ** m_DelaySlotTable;
};
