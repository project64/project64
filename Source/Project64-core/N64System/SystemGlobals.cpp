#include "stdafx.h"

#include "SystemGlobals.h"

CN64System * g_System = nullptr;
CN64System * g_BaseSystem = nullptr;
CN64System * g_SyncSystem = nullptr;
CRecompiler * g_Recompiler = nullptr;
CMipsMemoryVM * g_MMU = nullptr; // Memory of the N64
CRegisters * g_Reg = nullptr;    // Current register set attached to the g_MMU
CNotification * g_Notify = nullptr;
CPlugins * g_Plugins = nullptr;
CN64Rom * g_Rom = nullptr;   // The current ROM that this system is executing, it can only execute one file at the time
CN64Rom * g_DDRom = nullptr; // 64DD IPL ROM
CN64Disk * g_Disk = nullptr; // 64DD disk
CSystemTimer * g_SystemTimer = nullptr;
CDebugger * g_Debugger = nullptr;
CMempak * g_Mempak = nullptr;
CRandom * g_Random = nullptr;
CEnhancements * g_Enhancements = nullptr;

int32_t * g_NextTimer;