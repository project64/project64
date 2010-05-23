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
	void      StopTimer            ( TimerType Type );
	void      UpdateTimers         ( void ); 
	void      TimerDone            ( void ); 
	void      Reset                ( void );
	void      UpdateCompareTimer   ( void );
	
	inline TimerType CurrentType ( void ) const { return m_Current; }

	/*	          CSystemTimer         ( void );
	void      CheckTimer           ( void );
	void      ChangeTimerRelative  ( TimerType Type, DWORD Cycles );
	void      ChangeTimerFixed     ( TimerType Type, DWORD Cycles );
	void      DeactiateTimer       ( TimerType Type );
	void      ResetTimer           ( int NextVITimer );
	void      UpdateTimer          ( int StepIncrease );	
	double    GetTimer             ( TimerType Type ) const;
	
	inline int       GetCurrentTimer      ( void ) const { return m_Timer; }
*/
private:	
	TIMER_DETAILS m_TimerDetatils[MaxTimer];
	int           m_Timer; //How many cycles to the next event
	int         & m_NextTimer;  
	TimerType     m_Current;

	void FixTimers   ( void );
};
