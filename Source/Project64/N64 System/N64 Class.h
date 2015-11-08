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
class CRecompiler;

//#define TEST_SP_TRACKING  //track the SP to make sure all ops pick it up fine

class CN64System :
    public CMipsMemory_CallBack,
    public CTLB_CB,
    private CSystemEvents,
    protected CN64SystemSettings,
    public CGameSettings,
    protected CDebugSettings
{
public:
    CN64System(CPlugins * Plugins, bool SavesReadOnly);
    virtual ~CN64System(void);

	struct ThreadInfo {
		HANDLE * ThreadHandle;
		DWORD    ThreadID;
	};

    CProfiling m_Profile;
    CCheats    m_Cheats;
    bool  m_EndEmulation;
    SAVE_CHIP_TYPE m_SaveUsing;

    //Methods
    static bool RunFileImage(const char * FileLoc);
    static void CloseSystem(void);

	void   CloseCpu         ();
	void   ExternalEvent    ( SystemEvent action ); //covers gui interacting and timers etc..
	stdstr ChooseFileToOpen ( HWND hParent );
	void   DisplayRomInfo   ( HWND hParent );
	void   StartEmulation   ( bool NewThread );
	void   SyncToAudio      ();
	void   IncreaseSpeed    () { m_Limitor.IncreaseSpeed(); }
	void   DecreaseSpeed    () { m_Limitor.DecreaseSpeed(); }
	void   Reset            ( bool bInitReg, bool ClearMenory );
	void   GameReset        ();
	void   PluginReset      ();

	void   Pause            ();
	void   RunRSP           ();
	bool   SaveState        ();
	bool   LoadState        ( LPCSTR FileName );
	bool   LoadState        ();

	bool   DmaUsed() const { return m_DMAUsed; }
	void   SetDmaUsed(bool DMAUsed) { m_DMAUsed = DMAUsed; }
    void   SetCheatsSlectionChanged(bool changed) { m_CheatsSlectionChanged = changed; }
    bool   HasCheatsSlectionChanged(void) const { return m_CheatsSlectionChanged; }
	DWORD  GetButtons(int Control) const { return m_Buttons[Control]; }

	//Variable used to track that the SP is being handled and stays the same as the real SP in sync core
#ifdef TEST_SP_TRACKING
	DWORD m_CurrentSP;
#endif
	//For Sync CPU
	void   UpdateSyncCPU    ( CN64System * const SecondCPU, DWORD const Cycles );
	void   SyncCPU          ( CN64System * const SecondCPU );
	void   SyncCPUPC        ( CN64System * const SecondCPU );
	void   SyncSystem       ();
	void   SyncSystemPC     ();
private:
	//Make sure plugins can directly access this information
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CRSP_Plugin;
	friend CControl_Plugin;
	
	//Recompiler has access to manipulate and call functions
	friend CSystemTimer;

	//Used for loading and potentially executing the CPU in its own thread.
	static void StartEmulationThread ( ThreadInfo * Info );
	static bool EmulationStarting    ( HANDLE hThread, DWORD ThreadId );

	void   ExecuteCPU       ();
	void   RefreshScreen    ();
	void   DumpSyncErrors   ( CN64System * SecondCPU );
	void   StartEmulation2  ( bool NewThread );
	bool   SetActiveSystem  ( bool bActive = true );
	void   InitRegisters    ( bool bPostPif, CMipsMemory & MMU );
	void    DisplayRSPListCount();

	//CPU Methods
	void   ExecuteRecompiler();
	void   ExecuteInterpret();
	void   ExecuteSyncCPU();

	//Mark information saying that the CPU has stopped
	void   CpuStopped();

	//Function in CMipsMemory_CallBack
	virtual bool WriteToProtectedMemory(uint32_t Address, int length);

	//Functions in CTLB_CB
	void TLB_Mapped(DWORD VAddr, DWORD Len, DWORD PAddr, bool bReadOnly);
	void TLB_Unmaped(DWORD VAddr, DWORD Len);
	void TLB_Changed();

    CPlugins      * const m_Plugins;  //The plugin container 
	CN64System    * m_SyncCPU;
	CPlugins      * m_SyncPlugins;
	CMainGui      * m_SyncWindow;
	CMipsMemoryVM   m_MMU_VM;   //Memory of the n64 
	CTLB            m_TLB;
	CRegisters      m_Reg;
	CFramePerSecond m_FPS;
	CProfiling      m_CPU_Usage; //used to track the cpu usage
	CRecompiler   * m_Recomp;
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
	bool            m_TestTimer;
	DWORD           m_NextInstruction;
	DWORD           m_JumpToLocation;
	DWORD           m_TLBLoadAddress;
	DWORD           m_TLBStoreAddress;
	DWORD           m_SyncCount;
    bool            m_CheatsSlectionChanged;

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
