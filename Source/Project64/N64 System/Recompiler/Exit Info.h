class CExitInfo
{
public:
	enum EXIT_REASON
	{
		Normal					= 0,
		Normal_NoSysCheck		= 1,
		DoCPU_Action			= 2,
		COP1_Unuseable			= 3,
		DoSysCall				= 4,
		TLBReadMiss				= 5,
		TLBWriteMiss			= 6,
		DivByZero				= 7,
		ExitResetRecompCode		= 8,
	};

	DWORD       ID;
	DWORD       TargetPC;
	CRegInfo    ExitRegSet;
	EXIT_REASON reason;
	STEP_TYPE   NextInstruction;
	DWORD *     JumpLoc; //32bit jump
};

typedef std::list<CExitInfo> EXIT_LIST;
