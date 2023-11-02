#include <Project64-plugin-spec/Rsp.h>

class CHle;

extern RSP_INFO RSPInfo;
extern uint32_t RdramSize;
extern CHle * g_hle;

void InitilizeRSP(RSP_INFO & Rsp_Info);
void RspPluginLoaded(void);
void RspRomOpened(void);
void RspRomClosed(void);
void FreeRSP(void);