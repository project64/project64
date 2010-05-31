class CInterpreterCPU :
	private R4300iOp
{
	 CInterpreterCPU();
	~CInterpreterCPU();

public:
	static void BuildCPU   ( void );
	static void ExecuteCPU ( void );
	static void ExecuteOps ( int Cycles );

private:
	static R4300iOp::Func * m_R4300i_Opcode;
	static DWORD m_CountPerOp;
};
