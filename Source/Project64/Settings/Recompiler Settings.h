class CRecompilerSettings
{
	static void ShowRecompMemSizeChanged (CRecompilerSettings * _this);
	static void RomInMemoryChanged (CRecompilerSettings * _this);
	static void ProfilingChanged (CRecompilerSettings * _this);

public:
	CRecompilerSettings();
	virtual ~CRecompilerSettings();

	//Settings that can be changed on the fly
	static bool bShowRecompMemSize;	
	static bool bSMM_Protect;
	static bool bSMM_ValidFunc;
	static bool bSMM_PIDMA;
	static bool bSMM_TLB;
	static bool bProfiling;
	static bool bRomInMemory;
};