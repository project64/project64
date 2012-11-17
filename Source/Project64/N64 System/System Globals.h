extern CNotification * g_Notify;   
extern CSettings     * g_Settings;   

extern CN64System    * g_System;
extern CN64System    * g_BaseSystem;
extern CN64System    * g_SyncSystem;
extern CRecompiler   * g_Recompiler;
extern CMipsMemory   * g_MMU; //Memory of the n64 
extern CTLB          * g_TLB; //TLB Unit
extern CRegisters    * _Reg; //Current Register Set attacted to the g_MMU
extern CPlugins      * _Plugins;
extern CN64Rom       * _Rom;      //The current rom that this system is executing.. it can only execute one file at the time
extern CAudio        * _Audio;
extern CMemoryLabel  * _Labels;
extern CSystemTimer  * _SystemTimer;
extern CTransVaddr   * _TransVaddr;
extern CSystemEvents * _SystemEvents;
extern int           * _NextTimer;
extern DWORD         * _TLBLoadAddress;
extern DWORD         * _TLBStoreAddress;
