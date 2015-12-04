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

extern CNotification * g_Notify;   
extern CSettings     * g_Settings;   

extern CN64System    * g_System;
extern CN64System    * g_BaseSystem;
extern CN64System    * g_SyncSystem;
extern CRecompiler   * g_Recompiler;
extern CMipsMemory   * g_MMU; //Memory of the n64 
extern CTLB          * g_TLB; //TLB Unit
extern CRegisters    * g_Reg; //Current Register Set attached to the g_MMU
extern CPlugins      * g_Plugins;
extern CN64Rom       * g_Rom;      //The current rom that this system is executing.. it can only execute one file at the time
extern CAudio        * g_Audio;
extern CSystemTimer  * g_SystemTimer;
extern CTransVaddr   * g_TransVaddr;
extern CSystemEvents * g_SystemEvents;

extern int32_t       * g_NextTimer;
extern uint32_t      * g_TLBLoadAddress;
extern uint32_t      * g_TLBStoreAddress;

__interface CDebugger;
extern CDebugger     * g_Debugger;
