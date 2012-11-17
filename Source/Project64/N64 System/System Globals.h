extern CNotification * g_Notify;   
extern CSettings     * _Settings;   

extern CN64System    * _System;
extern CN64System    * _BaseSystem;
extern CN64System    * _SyncSystem;
extern CRecompiler   * _Recompiler;
extern CMipsMemory   * _MMU; //Memory of the n64 
extern CTLB          * _TLB; //TLB Unit
extern CRegisters    * _Reg; //Current Register Set attacted to the _MMU
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
