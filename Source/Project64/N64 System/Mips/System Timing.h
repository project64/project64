#include "..\\N64 Types.h"

class CC_Core;

class CSystemTimer
{
public:
	enum TimerType {
		UnknownTimer, 
		CompareTimer, 
		SoftResetTimer, 
		ViTimer, 
		AiTimer, 
		AiTimerDMA, 
		SiTimer, 
		PiTimer, 
		RspTimer, 
		RSPTimerDlist, 
		MaxTimer
	};

	typedef struct {
		bool    Active;
		__int64 CyclesToTimer;
	} TIMER_DETAILS;

public:
	          CSystemTimer         ( int & NextTimer );
	void      SetTimer             ( TimerType Type, DWORD Cycles, bool bRelative );
	DWORD     GetTimer             ( TimerType Type );
	void      StopTimer            ( TimerType Type );
	void      UpdateTimers         ( void ); 
	void      TimerDone            ( void ); 
	void      Reset                ( void );
	void      UpdateCompareTimer   ( void );
	bool      SaveAllowed          ( void );
	
	inline TimerType CurrentType ( void ) const { return m_Current; }

private:	
	TIMER_DETAILS m_TimerDetatils[MaxTimer];
	int           m_Timer; //How many cycles to the next event
	int         & m_NextTimer;  
	TimerType     m_Current;

	void FixTimers   ( void );
};
