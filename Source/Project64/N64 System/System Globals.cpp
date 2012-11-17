#include "stdafx.h"

CN64System    * g_System = NULL;
CN64System    * g_BaseSystem = NULL;
CN64System    * g_SyncSystem = NULL;
CRecompiler   * g_Recompiler = NULL;
CMipsMemory   * g_MMU = NULL; //Memory of the n64 
CTLB          * g_TLB = NULL; //TLB Unit
CRegisters    * g_Reg = NULL; //Current Register Set attacted to the g_MMU
CNotification * g_Notify = NULL;   
CPlugins      * g_Plugins = NULL;
CN64Rom       * g_Rom = NULL;      //The current rom that this system is executing.. it can only execute one file at the time
CAudio        * g_Audio = NULL;
CSystemTimer  * g_SystemTimer = NULL;
CTransVaddr   * g_TransVaddr = NULL;
CSystemEvents * _SystemEvents = NULL;
DWORD         * _TLBLoadAddress = NULL;
DWORD         * _TLBStoreAddress = NULL;

int * _NextTimer;


