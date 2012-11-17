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
extern CTransVaddr   * _TransVaddr;
extern CSystemEvents * _SystemEvents;
extern int           * _NextTimer;
extern DWORD         * _TLBLoadAddress;
extern DWORD         * _TLBStoreAddress;
