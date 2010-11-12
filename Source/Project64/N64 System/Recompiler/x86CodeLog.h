#define CPU_Message(Message,... )  if (bX86Logging) { x86_Log_Message(Message,## __VA_ARGS__); }

void x86_Log_Message (const char * Message, ...);
void Start_x86_Log (void);
void Stop_x86_Log (void);

extern bool bX86Logging;
