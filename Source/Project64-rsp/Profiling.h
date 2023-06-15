enum SPECIAL_TIMERS
{
    Timer_None = 0,
    Timer_Compiling = -1,
    Timer_RSP_Running = -2,
    Timer_R4300_Running = -3,
    Timer_RDP_Running = -5,
    Timer_RefreshScreen = -6,
    Timer_UpdateScreen = -7,
    Timer_UpdateFPS = -9,
    Timer_Idel = -10,
    Timer_FuncLookup = -11,
    Timer_Done = -13,
};

void ResetTimerList(void);
DWORD StartTimer(DWORD Address);
void StopTimer(void);
void GenerateTimerResults(void);
