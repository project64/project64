#pragma once

class CSettings;
extern CSettings * g_Settings;

class CN64System;
extern CN64System * g_System;
extern CN64System * g_BaseSystem;
extern CN64System * g_SyncSystem;

class CRecompiler;
extern CRecompiler * g_Recompiler;

class CMipsMemoryVM;
extern CMipsMemoryVM * g_MMU; // Memory of the N64

class CRegisters;
extern CRegisters * g_Reg; // Current register set attached to the g_MMU

class CPlugins;
extern CPlugins * g_Plugins;

class CN64Rom;
extern CN64Rom * g_Rom;   // The current ROM that this system is executing, it can only execute one file at the time
extern CN64Rom * g_DDRom; // 64DD IPL ROM

class CN64Disk;
extern CN64Disk * g_Disk; // 64DD disk

class CSystemTimer;
extern CSystemTimer * g_SystemTimer;

extern int32_t * g_NextTimer;
extern uint32_t * g_TLBLoadAddress;
extern uint32_t * g_TLBStoreAddress;

__interface CDebugger;
extern CDebugger * g_Debugger;

class CMempak;
extern CMempak * g_Mempak;

class CRandom;
extern CRandom * g_Random;

class CEnhancements;
extern CEnhancements * g_Enhancements;