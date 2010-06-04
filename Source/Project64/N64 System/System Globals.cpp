#include "stdafx.h"

CN64System    * _N64System = NULL;
CN64System    * _SyncSystem = NULL;
CRecompiler   * _Recompiler = NULL;
CMipsMemory   * _MMU = NULL; //Memory of the n64 
CTLB          * _TLB = NULL; //TLB Unit
CRegisters    * _Reg = NULL; //Current Register Set attacted to the _MMU
CNotification * _Notify = NULL;   
//CSettings     * _Settings;   
CPlugins      * _Plugins = NULL;
CN64Rom       * _Rom = NULL;      //The current rom that this system is executing.. it can only execute one file at the time
CAudio        * _Audio = NULL;
CMemoryLabel  * _Labels = NULL;
CSystemTimer  * _SystemTimer = NULL;
CTransVaddr   * _TransVaddr = NULL;
CSystemEvents * _SystemEvents = NULL;

MIPS_DWORD * _GPR, * _FPR, * _RegHI, * _RegLO;
DWORD              * _PROGRAM_COUNTER, * _CP0, * _RegMI, * _LLBit, 
						  * _LLAddr, * _FPCR, * _RegSI, * _RegRI, * _RegPI, * _RegAI,
						  * _RegVI, * _RegDPC, * _RegSP, * _RegRDRAM;
double ** _FPRDoubleLocation;
float  ** _FPRFloatLocation;
int * _NextTimer;



