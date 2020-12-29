/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
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
extern CMipsMemoryVM * g_MMU; //Memory of the n64

class CTLB;
extern CTLB * g_TLB; //TLB Unit

class CRegisters;
extern CRegisters * g_Reg; //Current Register Set attached to the g_MMU

class CPlugins;
extern CPlugins * g_Plugins;

class CN64Rom;
extern CN64Rom * g_Rom;      //The current rom that this system is executing.. it can only execute one file at the time
extern CN64Rom * g_DDRom;    //64DD IPL ROM

class CN64Disk;
extern CN64Disk * g_Disk;     //64DD DISK

class CAudio;
extern CAudio * g_Audio;

class CSystemTimer;
extern CSystemTimer * g_SystemTimer;

__interface CTransVaddr;
extern CTransVaddr * g_TransVaddr;

class CSystemEvents;
extern CSystemEvents * g_SystemEvents;

extern int32_t * g_NextTimer;
extern uint32_t * g_TLBLoadAddress;
extern uint32_t * g_TLBStoreAddress;

__interface CDebugger;
extern CDebugger * g_Debugger;

extern uint8_t ** g_RecompPos;

class CMempak;
extern CMempak * g_Mempak;

class CRandom;
extern CRandom * g_Random;

class CEnhancements;
extern CEnhancements * g_Enhancements;