class CInterpreterCPU :
	private R4300iOp
{
public:
	static void BuildCPU   ( void );
	static void ExecuteCPU ( void );
	static void ExecuteOps ( int Cycles );
	static void InPermLoop ( void );

private:
	CInterpreterCPU(void);									// Disable default constructor
	CInterpreterCPU(const CInterpreterCPU&);				// Disable copy constructor
	CInterpreterCPU& operator=(const CInterpreterCPU&);		// Disable assignment

	static R4300iOp::Func * m_R4300i_Opcode;
};
