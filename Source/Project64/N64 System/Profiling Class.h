
typedef std::map<DWORD, __int64 >     PROFILE_ENRTIES;
typedef PROFILE_ENRTIES::iterator     PROFILE_ENRTY;
typedef PROFILE_ENRTIES::value_type   PROFILE_VALUE;

class CProfiling {
	CNotification * _Notify;

	DWORD CurrentTimerAddr, CurrentDisplayCount;
	DWORD StartTimeHi, StartTimeLo; //The Current Timer start time
	PROFILE_ENRTIES Entries;

public:	
	CProfiling (CNotification * Notify);
	
	//recording timing against current timer, returns the address of the timer stoped
	DWORD StartTimer ( DWORD Address );
	DWORD StopTimer  ( void );

	//Display the CPU Usage
	void ShowCPU_Usage ( void ); 

	//Reset all the counters back to 0
	void ResetCounters ( void );

	//Generate a log file with the current results, this will also reset the counters
	void GenerateLog   ( void );
};
