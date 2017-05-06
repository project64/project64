/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

#include <Common/SyncEvent.h>
#include <Common/Thread.h>
#include <Project64-core/Settings/N64SystemSettings.h>
#include <Project64-core/N64System/ProfilingClass.h>
#include <Project64-core/N64System/Recompiler/RecompilerClass.h>
#include <Project64-core/N64System/Mips/Audio.h>
#include <Project64-core/N64System/Mips/MemoryVirtualMem.h>
#include <Project64-core/N64System/Mips/SystemEvents.h>
#include <Project64-core/N64System/Mips/SystemTiming.h>
#include <Project64-core/N64System/Mips/Mempak.h>
#include <Project64-core/Settings/DebugSettings.h>
#include <Project64-core/Plugin.h>
#include <Project64-core/Logging.h>

#include "Mips/TLBClass.h"
#include "CheatClass.h"
#include "FramePerSecondClass.h"
#include "SpeedLimiterClass.h"

typedef std::list<SystemEvent>   EVENT_LIST;

typedef std::map<uint32_t, uint32_t> FUNC_CALLS;

class CPlugins;
class CRSP_Plugin;
class CRecompiler;

//#define TEST_SP_TRACKING  //track the SP to make sure all ops pick it up fine

class CN64System :
    public CLogging,
    public CTLB_CB,
    private CSystemEvents,
    protected CN64SystemSettings,
    public CGameSettings,
    protected CDebugSettings
{
public:
    CN64System(CPlugins * Plugins, bool SavesReadOnly, bool SyncSystem);
    virtual ~CN64System(void);

    CCheats    m_Cheats;
    bool  m_EndEmulation;
    SAVE_CHIP_TYPE m_SaveUsing;

    //Methods
    static bool LoadFileImage(const char * FileLoc);
    static bool RunFileImage(const char * FileLoc);
    static bool RunFileImageIPL(const char * FileLoc);
    static bool RunDiskImage(const char * FileLoc);
    static void RunLoadedImage(void);
    static void CloseSystem(void);

    void   CloseCpu();
    void   ExternalEvent(SystemEvent action); //covers gui interacting and timers etc..
    void   StartEmulation(bool NewThread);
    void   EndEmulation();
    void   SyncToAudio();
    void   AlterSpeed(const CSpeedLimiter::ESpeedChange SpeedChange) { m_Limiter.AlterSpeed(SpeedChange); }
    void   SetSpeed(int Speed) { m_Limiter.SetSpeed(Speed);  }
    int    GetSpeed(void) const { return m_Limiter.GetSpeed(); }
    int    GetBaseSpeed(void) const { return m_Limiter.GetBaseSpeed(); }
    void   Reset(bool bInitReg, bool ClearMenory);
    void   GameReset();
    void   PluginReset();

    void   Pause();
    void   RunRSP();
    bool   SaveState();
    bool   LoadState(const char * FileName);
    bool   LoadState();

    bool   DmaUsed() const { return m_DMAUsed; }
    void   SetDmaUsed(bool DMAUsed) { m_DMAUsed = DMAUsed; }
    void   SetCheatsSlectionChanged(bool changed) { m_CheatsSlectionChanged = changed; }
    bool   HasCheatsSlectionChanged(void) const { return m_CheatsSlectionChanged; }
    uint32_t  GetButtons(int32_t Control) const { return m_Buttons[Control]; }
    CPlugins * GetPlugins() { return m_Plugins; }

    //Variable used to track that the SP is being handled and stays the same as the real SP in sync core
#ifdef TEST_SP_TRACKING
    uint32_t m_CurrentSP;
#endif
    //For Sync CPU
    void   UpdateSyncCPU(CN64System * const SecondCPU, uint32_t const Cycles);
    void   SyncCPU(CN64System * const SecondCPU);
    void   SyncCPUPC(CN64System * const SecondCPU);
    void   SyncSystem();
    void   SyncSystemPC();
private:
    //Make sure plugins can directly access this information
    friend class CGfxPlugin;
    friend class CAudioPlugin;
    friend class CRSP_Plugin;
    friend class CControl_Plugin;

    //Recompiler has access to manipulate and call functions
    friend class CSystemTimer;
    friend class CRecompiler;
    friend class CMipsMemoryVM;

    //Used for loading and potentially executing the CPU in its own thread.
    static void StartEmulationThread(CThread * thread);
    static bool EmulationStarting(CThread * thread);
    static void StartEmulationThead();

    void   ExecuteCPU();
    void   RefreshScreen();
    void   DumpSyncErrors(CN64System * SecondCPU);
    void   StartEmulation2(bool NewThread);
    bool   SetActiveSystem(bool bActive = true);
    void   InitRegisters(bool bPostPif, CMipsMemoryVM & MMU);
    void   DisplayRSPListCount();

    //CPU Methods
    void   ExecuteRecompiler();
    void   ExecuteInterpret();
    void   ExecuteSyncCPU();

    //Mark information saying that the CPU has stopped
    void   CpuStopped();

    //Functions in CTLB_CB
    void TLB_Mapped(uint32_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly);
    void TLB_Unmaped(uint32_t VAddr, uint32_t Len);
    void TLB_Changed();

    CPlugins      * const m_Plugins;  //The plugin container
    CPlugins      * m_SyncPlugins;
    CN64System    * m_SyncCPU;
    CMipsMemoryVM   m_MMU_VM;   //Memory of the n64
    CTLB            m_TLB;
    CRegisters      m_Reg;
    CMempak         m_Mempak;
    CFramePerSecond m_FPS;
    CProfiling      m_CPU_Usage; //used to track the cpu usage
    CRecompiler   * m_Recomp;
    CAudio          m_Audio;
    CSpeedLimiter   m_Limiter;
    bool            m_InReset;
    int32_t         m_NextTimer;
    CSystemTimer    m_SystemTimer;
    bool            m_bCleanFrameBox;
    bool            m_RspBroke;
    bool            m_DMAUsed;
    uint32_t        m_Buttons[4];
    bool            m_TestTimer;
    uint32_t        m_NextInstruction;
    uint32_t        m_JumpToLocation;
    uint32_t        m_TLBLoadAddress;
    uint32_t        m_TLBStoreAddress;
    uint32_t        m_SyncCount;
    bool            m_SyncCpu;
    bool            m_CheatsSlectionChanged;

    //When Syncing cores this is the PC where it last Sync'ed correctly
    uint32_t m_LastSuccessSyncPC[10];
    int32_t  m_CyclesToSkip;

    //Handle to the cpu thread
    CThread * m_thread;

    //Handle to pause mutex
    SyncEvent m_hPauseEvent;

    //No of Alist and Dlist sent to the RSP
    uint32_t m_AlistCount, m_DlistCount, m_UnknownCount;

    //list of function that have been called .. used in profiling
    FUNC_CALLS m_FunctionCalls;
};
