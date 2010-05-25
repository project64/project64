#include "N64 System/C Core/C Core.h"
#include "System Globals.h"

CN64System    * _N64System;
CN64System    * _SyncSystem;
CRecompiler   * _Recompiler;
CMipsMemory   * _MMU; //Memory of the n64 
CTLB          * _TLB; //TLB Unit
CRegisters    * _Reg; //Current Register Set attacted to the _MMU
CNotification * _Notify;   
//CSettings     * _Settings;   
CPlugins      * _Plugins;
CN64Rom       * _Rom;      //The current rom that this system is executing.. it can only execute one file at the time
CAudio        * _Audio;
CMemoryLabel  * _Labels;
CSystemTimer  * _SystemTimer;
CTransVaddr   * _TransVaddr;


MIPS_DWORD * _GPR, * _FPR, * _RegHI, * _RegLO;
DWORD              * _PROGRAM_COUNTER, * _CP0, * _RegMI, * _LLBit, 
						  * _LLAddr, * _FPCR, * _RegSI, * _RegRI, * _RegPI, * _RegAI,
						  * _RegVI, * _RegDPC, * _RegSP, * _RegRDRAM;
double ** _FPRDoubleLocation;
float  ** _FPRFloatLocation;
int * _NextTimer;



