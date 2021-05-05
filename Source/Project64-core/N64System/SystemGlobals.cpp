#include "stdafx.h"
#include "SystemGlobals.h"

CN64System * g_System = NULL;
CN64System * g_BaseSystem = NULL;
CN64System * g_SyncSystem = NULL;
CRecompiler * g_Recompiler = NULL;
CMipsMemoryVM * g_MMU = NULL; // Memory of the N64
CTLB * g_TLB = NULL; // TLB unit
CRegisters * g_Reg = NULL; // Current register set attached to the g_MMU
CNotification * g_Notify = NULL;
CPlugins * g_Plugins = NULL;
CN64Rom * g_Rom = NULL;      // The current ROM that this system is executing, it can only execute one file at the time
CN64Rom * g_DDRom = NULL;    // 64DD IPL ROM
CN64Disk * g_Disk = NULL;     // 64DD disk
CAudio * g_Audio = NULL;
CSystemTimer  * g_SystemTimer = NULL;
CTransVaddr * g_TransVaddr = NULL;
CSystemEvents * g_SystemEvents = NULL;
uint32_t * g_TLBLoadAddress = NULL;
uint32_t * g_TLBStoreAddress = NULL;
CDebugger * g_Debugger = NULL;
uint8_t ** g_RecompPos = NULL;
CMempak * g_Mempak = NULL;
CRandom * g_Random = NULL;
CEnhancements * g_Enhancements = NULL;

int * g_NextTimer;
