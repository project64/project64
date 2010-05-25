extern CN64System    * _N64System;
extern CN64System    * _SyncSystem;
extern CRecompiler   * _Recompiler;
extern CMipsMemory   * _MMU; //Memory of the n64 
extern CTLB          * _TLB; //TLB Unit
extern CRegisters    * _Reg; //Current Register Set attacted to the _MMU
extern CNotification * _Notify;   
extern CSettings     * _Settings;   
extern CPlugins      * _Plugins;
extern CN64Rom       * _Rom;      //The current rom that this system is executing.. it can only execute one file at the time
extern CAudio        * _Audio;
extern CMemoryLabel  * _Labels;
extern CSystemTimer  * _SystemTimer;
extern CTransVaddr   * _TransVaddr;


extern MIPS_DWORD * _GPR, * _FPR, * _RegHI, * _RegLO;
extern DWORD              * _PROGRAM_COUNTER, * _CP0, * _RegMI, * _LLBit, 
						  * _LLAddr, * _FPCR, * _RegSI, * _RegRI, * _RegPI, * _RegAI,
						  * _RegVI, * _RegDPC, * _RegSP, * _RegRDRAM;
extern double ** _FPRDoubleLocation;
extern float  ** _FPRFloatLocation;
extern int * _NextTimer;
