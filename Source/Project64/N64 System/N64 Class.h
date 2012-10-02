#ifndef _N64CLASS_H_
#define _N64CLASS_H_

typedef std::list<SystemEvent>   EVENT_LIST;

typedef struct {
	stdstr   FileName;
	HANDLE * ThreadHandle;
	DWORD    ThreadID;
} FileImageInfo;

typedef std::map<DWORD, DWORD> FUNC_CALLS;

class CPlugins;
class CRSP_Plugin;
class CC_Core;

//#define TEST_SP_TRACKING  //track the SP to make sure all ops pick it up fine

class CN64System :
	private CMipsMemory_CallBack,
	private CTLB_CB,
	private CSystemEvents,
	protected CN64SystemSettings,
	protected CGameSettings,
	protected CDebugSettings,
	public CDebugger
{
public:
    CN64System ( CPlugins * Plugins, bool SavesReadOnly );
    virtual ~CN64System ( void );

	CProfiling m_Profile;
	CCheats    m_Cheats;
	bool  m_EndEmulation;
	enum SAVE_CHIP_TYPE m_SaveUsing;
	enum SystemType m_SystemType;;

	//Methods
	static bool CN64System::RunFileImage ( const char * FileLoc );
		
	void   CloseCpu         ( void );
	void   ExternalEvent    ( SystemEvent action ); //covers gui interacting and timers etc..
	stdstr ChooseFileToOpen ( WND_HANDLE hParent );
	void   DisplayRomInfo   ( WND_HANDLE hParent );
	void   SelectCheats     ( WND_HANDLE hParent );
	void   StartEmulation   ( bool NewThread );
	void   SyncToAudio      ( void );
	bool   IsDialogMsg      ( MSG * msg );
	void   IncreaseSpeed    ( void ) { m_Limitor.IncreaeSpeed(10); }
	void   DecreaeSpeed     ( void ) { m_Limitor.DecreaeSpeed(10); }
	void   Reset            ( bool bInitReg, bool ClearMenory );

	void   Pause           ( void );
	void   RunRSP           ( void );
	bool   SaveState        ( void );
	bool   LoadState        ( LPCSTR FileName );
	bool   LoadState        ( void );	
//	inline CPlugins * Plugins ( void ) const { return m_Plugins; }
	inline bool   DmaUsed     ( void ) const { return m_DMAUsed; }
	inline void   SetDmaUsed  ( bool DMAUsed) { m_DMAUsed = DMAUsed; }
	inline DWORD  GetButtons  ( int Control ) { return m_Buttons[Control]; }

	//Variable used to track that the SP is being handled and stays the same as the real SP in sync core
#ifdef TEST_SP_TRACKING
	DWORD m_CurrentSP;
#endif
	//For Sync CPU
	void   UpdateSyncCPU    ( CN64System * const SecondCPU, DWORD const Cycles );
	void   SyncCPU          ( CN64System * const SecondCPU );
	void   SyncSystem		( void );
private:
	//Make sure plugins can directly access this information
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CRSP_Plugin;
	friend CControl_Plugin;
	
	//Recompiler has access to manipulate and call functions
	friend CC_Core;
	friend CSystemTimer;

	//Used for loading and potentialy executing the CPU in its own thread.
	static void stLoadFileImage      ( FileImageInfo * Info );
	static void StartEmulationThread ( FileImageInfo * Info );
	static bool EmulationStarting    ( HANDLE hThread, DWORD ThreadId );

	void   InitializeCPUCore( void );
	void   ExecuteCPU       ( void );
	void   RefreshScreen    ( void );
	bool   InternalEvent    ( void );
	void   DumpSyncErrors   ( CN64System * SecondCPU );
	void   StartEmulation2  ( bool NewThread );
	bool   SetActiveSystem  ( bool bActive = true );
	void   InitRegisters    ( bool bPostPif, CMipsMemory & MMU );

	//CPU Methods
	void   ExecuteRecompiler ( );
	void   ExecuteInterpret  (  );
	void   ExecuteSyncCPU    ();

	void   AddEvent          ( SystemEvent Event);

	//Notification of changing conditions
	void   FunctionStarted ( DWORD NewFuncAddress, DWORD OldFuncAddress, DWORD ReturnAddress );
	void   FunctionEnded   ( DWORD ReturnAddress, DWORD StackPos );

	//Mark information saying that the CPU has stoped
	void   CpuStopped      ( void );

	//Function in CMipsMemory_CallBack
	virtual bool WriteToProtectedMemory (DWORD Address, int length);

	//Functions in CTLB_CB
	void TLB_Mapped  ( DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly );
	void TLB_Unmaped ( DWORD VAddr, DWORD Len );
	void TLB_Changed ( void );

    CPlugins      * const m_Plugins;  //The plugin container 
	CN64System    * m_SyncCPU;
	CMipsMemoryVM  m_MMU_VM;   //Memory of the n64 
	CTLB           m_TLB;
	CRegisters     m_Reg;   
	CFramePerSecond m_FPS;
	CProfiling		m_CPU_Usage; //used to track the cpu usage
	CRecompiler     * m_Recomp;
	CAudio          m_Audio;
	CSpeedLimitor   m_Limitor;
	bool            m_InReset;
	CSystemTimer    m_SystemTimer;
	bool            m_bCleanFrameBox;
	bool            m_bInitilized;
	int             m_NextTimer;
	bool            m_DMAUsed;
	DWORD           m_Buttons[4];
	BOOL            m_TestTimer;
	DWORD           m_NextInstruction;
	DWORD           m_JumpToLocation;
	DWORD           m_TLBLoadAddress;
	DWORD           m_TLBStoreAddress;

	//When Syncing cores this is the PC where it last Sync'ed correctly
	DWORD m_LastSuccessSyncPC[10];
	int   m_CyclesToSkip;

	//Handle to the cpu thread
	HANDLE m_CPU_Handle;
	DWORD  m_CPU_ThreadID;
	
	//Handle to pause mutex
	void * m_hPauseEvent;

	//No of Alist and Dlist sent to the RSP
	DWORD m_AlistCount, m_DlistCount, m_UnknownCount;

	//list of function that have been called .. used in profiling
	FUNC_CALLS m_FunctionCalls;
};

#endif