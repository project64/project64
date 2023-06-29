void DumpRSPCode(void);
void DumpRSPData(void);
void Disable_RSP_Commands_Window(void);
void Enable_RSP_Commands_Window(void);
void Enter_RSP_Commands_Window(void);
void RefreshRSPCommands(void);
void SetRSPCommandToRunning(void);
void SetRSPCommandToStepping(void);
void SetRSPCommandViewto(unsigned int NewLocation);

extern DWORD Stepping_Commands, WaitingForStep;
extern bool InRSPCommandsWindow;
