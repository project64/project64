#include "..\\N64 Types.h"

typedef struct {
	bool   Active;
	double CyclesToTimer;
} TIMER_DETAILS;

class CC_Core;

class CSystemTimer
{
	friend CC_Core;

	TIMER_DETAILS TimerDetatils[MaxTimer];
	int         Timer; //How many cycles to the next event
	TimerType     CurrentTimerType;
	
	void FixTimers   ( void );
public:
	          CSystemTimer         ( void );
	void      CheckTimer           ( void );
	void      ChangeTimerRelative  ( TimerType Type, DWORD Cycles );
	void      ChangeTimerFixed     ( TimerType Type, DWORD Cycles );
	void      DeactiateTimer       ( TimerType Type );
	void      ResetTimer           ( int NextVITimer );
	void      UpdateTimer          ( int StepIncrease );	
	double    GetTimer             ( TimerType Type ) const;
	
	inline int       GetCurrentTimer      ( void ) const { return Timer; }
	inline TimerType GetCurrentTimerType  ( void ) const { return CurrentTimerType; }
};
