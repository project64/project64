#ifndef __N64_CLASS__H__
#define __N64_CLASS__H__

#include "N64 Types.h"
#include  "../Settings/N64System Settings.h"

typedef std::list<SystemEvent>   EVENT_LIST;

typedef struct {
	CN64System * _this;
	stdstr       FileName;
	void       * ThreadHandle;
	DWORD        ThreadID;
} FileImageInfo;

typedef std::map<DWORD, DWORD> FUNC_CALLS;

class CPlugins;
class CRSP_Plugin;
class CC_Core;

//#define TEST_SP_TRACKING  //track the SP to make sure all ops pick it up fine

class CN64System :
	private CMipsMemory_CallBack,
	public CMipsTLB_CallBack,
	protected CN64SystemSettings,
	public CDebugger
{
	//Make sure plugins can directly access this information
	friend CGfxPlugin;
	friend CAudioPlugin;
	friend CRSP_Plugin;
	friend CControl_Plugin;
	
	//Recompiler has access to manipulate and call functions
	friend CC_Core;

	CNotification * const _Notify;   //Original Notify member used to notify the user when something occurs
    CPlugins      * const _Plugins;  //The plugin container 
	CN64System    * _SyncCPU;
	CMipsMemory   * _MMU;      //Memory of the n64 
	CRegisters    * _Reg;      //Current Register Set attacted to the _MMU

	CN64Rom       * _Rom;      //The current rom that this system is executing.. it can only execute one file at the time
	bool            m_OwnRomObject;
	CCheats       * _Cheats;
	CFramePerSecond FPS;
	CProfiling      m_CPU_Usage, m_Profile; //used to track the cpu usage
//	CInterpreterOps * InterpreterOpcode; //Pointer to the interpter Opcode
	CRecompiler     * _Recomp;
	CAudio          * _Audio;
	CSpeedLimitor   m_Limitor;
	bool            m_InReset;
	SystemType      m_SystemType;
	bool            m_bCleanFrameBox;
	
	//When Syncing cores this is the PC where it last Sync'ed correctly
	DWORD m_LastSuccessSyncPC[10];
	int   m_CyclesToSkip;

	//List of Internal events that need to be acted on by CPU
	EVENT_LIST    EventList;
	DWORD         NoOfEvents;

	//Handle to the cpu thread
	void * CPU_Handle;
	DWORD  CPU_ThreadID;
	
	//Handle to pause mutex
	void * m_hPauseEvent;

	//No of Alist and Dlist sent to the RSP
	DWORD m_AlistCount, m_DlistCount, m_UnknownCount;

	//list of function that have been called .. used in profiling
	FUNC_CALLS m_FunctionCalls;

	//Used for loading and potentialy executing the CPU in its own thread.
	static void LoadFileImage        ( FileImageInfo * Info );
	static void StartEmulationThread ( FileImageInfo * Info );

	void   ExecuteCycles    ( DWORD Cycles );
	void   ExecuteCPU       ( void );
	void   RefreshScreen    ( void );
	bool   InternalEvent    ( void );
	bool   InPermLoop       ( void );
	void   Reset            ( void );
	void   RunRSP           ( void );
	void   SetupSystem      ( CN64Rom * Rom, bool OwnRomObject, bool SavesReadOnly = false );
	bool   SaveState        ( void );
	bool   LoadState        ( LPCSTR FileName );
	bool   LoadState        ( void );
	void   DumpSyncErrors   ( CN64System * SecondCPU );
	void   StartEmulation2  ( bool NewThread );

	//CPU Methods
	void   ExecuteRecompiler ( CC_Core & C_Core );
	void   ExecuteInterpret  ( CC_Core & C_Core );
	void   ExecuteSyncCPU    ( CC_Core & C_Core );

	void   AddEvent          ( SystemEvent Event);

	//Notification of changing conditions
	void   FunctionStarted ( DWORD NewFuncAddress, DWORD OldFuncAddress, DWORD ReturnAddress );
	void   FunctionEnded   ( DWORD ReturnAddress, DWORD StackPos );

	//Mark information saying that the CPU has stoped
	void   CpuStopped      ( void );
	void   Pause           ( void );


	//Function in CMipsMemory_CallBack
	virtual bool WriteToProtectedMemory (DWORD Address, int length);

	//Functions in CMipsTLB_CallBack
	virtual void TLB_Changed   ( void );
	virtual void TLB_Unmapping ( int TlbEntry, int FastTlbEntry, DWORD Vaddr, DWORD Len );

public:
         CN64System ( CNotification * Notify, CPlugins * Plugins );
        ~CN64System ( void );

	//Methods
	void   CloseCpu         ( void );
	void   RunFileImage     ( const char * FileLoc );
	void   ExternalEvent    ( SystemEvent Event ); //covers gui interacting and timers etc..
	stdstr ChooseFileToOpen ( WND_HANDLE hParent );
	void   DisplayRomInfo   ( WND_HANDLE hParent );
	void   SelectCheats     ( WND_HANDLE hParent );
	void   StartEmulation   ( bool NewThread );
	void   SyncToAudio      ( void );
	bool   IsDialogMsg      ( MSG * msg );
	void   IncreaseSpeed    ( void ) { m_Limitor.IncreaeSpeed(10); }
	void   DecreaeSpeed     ( void ) { m_Limitor.DecreaeSpeed(10); }
	void   SoftReset        ( void );

	bool  EndEmulation;

	//Get the pointer to the Internal Classes
	CRecompiler * GetRecompiler ( void ) { return _Recomp; }
	CN64Rom     * GetCurrentRom ( void ) { return _Rom; }

	inline CPlugins * Plugins ( void ) const { return _Plugins; }

	//Variable used to track that the SP is being handled and stays the same as the real SP in sync core
#ifdef TEST_SP_TRACKING
	DWORD m_CurrentSP;
#endif
	//For Sync CPU
	void   UpdateSyncCPU    ( CN64System * const SecondCPU, DWORD const Cycles );
	void   SyncCPU          ( CN64System * const SecondCPU );

};

#endif
