/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

typedef std::list<SystemEvent>   EVENT_LIST;

typedef std::map<DWORD, DWORD> FUNC_CALLS;

class CPlugins;
class CRSP_Plugin;

//#define TEST_SP_TRACKING  //track the SP to make sure all ops pick it up fine

class CN64System :
	private CMipsMemory_CallBack,
	private CTLB_CB,
	private CSystemEvents,
	protected CN64SystemSettings,
	public CGameSettings,
	protected CDebugSettings,
	public CDebugger
{
public:
    CN64System ( CPlugins * Plugins, bool SavesReadOnly );
    virtual ~CN64System ( void );

	typedef struct {
		HANDLE * ThreadHandle;
		DWORD    ThreadID;
	} ThreadInfo;

	CProfiling m_Profile;
	CCheats    m_Cheats;
	bool  m_EndEmulation;
	SAVE_CHIP_TYPE m_SaveUsing;

	//Methods
	static bool RunFileImage ( const char * FileLoc );
	static void CloseSystem ( void );
		
	void   CloseCpu         ( void );
	void   ExternalEvent    ( SystemEvent action ); //covers gui interacting and timers etc..
	stdstr ChooseFileToOpen ( HWND hParent );
	void   DisplayRomInfo   ( HWND hParent );
	void   SelectCheats     ( HWND hParent );
	void   StartEmulation   ( bool NewThread );
	void   SyncToAudio      ( void );
	bool   IsDialogMsg      ( MSG * msg );
	void   IncreaseSpeed    ( void ) { m_Limitor.IcreasedSpeed(); }
	void   DecreaeSpeed     ( void ) { m_Limitor.DecreaeSpeed(); }
	void   Reset            ( bool bInitReg, bool ClearMenory );
	void   GameReset        ( void );
	void   PluginReset      ( void );

	void   Pause           ( void );
	void   RunRSP           ( void );
	bool   SaveState        ( void );
	bool   LoadState        ( LPCSTR FileName );
	bool   LoadState        ( void );	

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
	void   SyncCPUPC        ( CN64System * const SecondCPU );
	void   SyncSystem		( void );
	void   SyncSystemPC		( void );
private:
	//Make sure plugins can directly access this information
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CRSP_Plugin;
	friend CControl_Plugin;
	
	//Recompiler has access to manipulate and call functions
	friend CSystemTimer;

	//Used for loading and potentialy executing the CPU in its own thread.
	static void StartEmulationThread ( ThreadInfo * Info );
	static bool EmulationStarting    ( HANDLE hThread, DWORD ThreadId );

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
	CPlugins      * m_SyncPlugins;
	CMainGui      * m_SyncWindow;
	CMipsMemoryVM  m_MMU_VM;   //Memory of the n64 
	CTLB           m_TLB;
	CRegisters     m_Reg;   
	CFramePerSecond m_FPS;
	CProfiling		m_CPU_Usage; //used to track the cpu usage
	CRecompiler     * m_Recomp;
	CAudio          m_Audio;
	CSpeedLimitor   m_Limitor;
	bool            m_InReset;
	int             m_NextTimer;
	CSystemTimer    m_SystemTimer;
	bool            m_bCleanFrameBox;
	bool            m_bInitialized;
	bool            m_RspBroke;
	bool            m_DMAUsed;
	DWORD           m_Buttons[4];
	BOOL            m_TestTimer;
	DWORD           m_NextInstruction;
	DWORD           m_JumpToLocation;
	DWORD           m_TLBLoadAddress;
	DWORD           m_TLBStoreAddress;
	DWORD           m_SyncCount;

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
