enum SystemEvent {
	SysEvent_ExecuteInterrupt,
	SysEvent_GSButtonPressed,
	SysEvent_ResetCPU_Soft,
	SysEvent_ResetCPU_SoftDone,
	SysEvent_ResetCPU_Hard,
	SysEvent_CloseCPU,
	SysEvent_PauseCPU_FromMenu,
	SysEvent_PauseCPU_AppLostActive,
	SysEvent_PauseCPU_AppLostActiveDelay,
	SysEvent_PauseCPU_AppLostFocus,
	SysEvent_PauseCPU_SaveGame,
	SysEvent_PauseCPU_LoadGame,
	SysEvent_PauseCPU_DumpMemory,
	SysEvent_PauseCPU_SearchMemory,
	SysEvent_ResumeCPU_FromMenu,
	SysEvent_ResumeCPU_AppGainedActive,
	SysEvent_ResumeCPU_AppGainedFocus,
	SysEvent_ResumeCPU_SaveGame,
	SysEvent_ResumeCPU_LoadGame,
	SysEvent_ResumeCPU_DumpMemory,
	SysEvent_ResumeCPU_SearchMemory,
	SysEvent_ChangingFullScreen,
	SysEvent_ChangePlugins,
	SysEvent_SaveMachineState,
	SysEvent_LoadMachineState,
	SysEvent_Interrupt_SP,
	SysEvent_Interrupt_SI,
	SysEvent_Interrupt_AI,
	SysEvent_Interrupt_VI,
	SysEvent_Interrupt_PI,
	SysEvent_Interrupt_DP,
	SysEvent_Profile_StartStop,
	SysEvent_Profile_ResetLogs,
	SysEvent_Profile_GenerateLogs,
};

class CSystemEvents
{
	typedef std::vector<SystemEvent> EventList;

protected:
	CSystemEvents(CN64System * System);
	virtual ~CSystemEvents();

public:
	void ExecuteEvents ( void );
	void QueueEvent    ( SystemEvent action);

	inline const BOOL & DoSomething ( void ) const { return m_bDoSomething; }

private:
	CSystemEvents(void);							// Disable default constructor
	CSystemEvents(const CSystemEvents&);			// Disable copy constructor
	CSystemEvents& operator=(const CSystemEvents&);	// Disable assignment

	void ChangePluginFunc( void );

	CN64System    * m_System;
	EventList       m_Events;
	BOOL            m_bDoSomething;
	CriticalSection m_CS;
};