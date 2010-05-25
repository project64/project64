#define CPU_Type		g_CPU_Type   
#define CPU_Action		(*g_CPU_Action)
#define IndvidualBlock	g_IndvidualBlock
#define Profiling		g_Profiling

// Registers
/*#define GPR				g_GPR
#define CP0				g_CP0
#define FPR		        g_FPR
#define FPCR		    g_FPCR
#define RegMI			g_RegMI
#define RegSI			g_RegSI
#define RegRI			g_RegRI
#define RegPI			g_RegPI
#define RegAI			g_RegAI
#define RegVI			g_RegVI
#define RegDPC			g_RegDPC
#define RegSP			g_RegSP
#define RegRDRAM		g_RegRDRAM
#define PROGRAM_COUNTER	(*g_PROGRAM_COUNTER)
#define LLBit	        (*g_LLBit)
#define HI		        (*g_HI)
#define LO		        (*g_LO)
#define FPRDoubleLocation	g_FPRDoubleLocation
#define FPRFloatLocation	g_FPRFloatLocation*/

//Settings
#define ShowUnhandledMemory g_ShowUnhandledMemory
#define ShowCPUPer          g_ShowCPUPer
#define ShowTLBMisses       g_ShowTLBMisses
#define UseTlb			    g_UseTlb
#define HaveDebugger	    g_HaveDebugger
#define RomFileSize         g_RomFileSize
#define SaveUsing			g_SaveUsing
#define RomName             g_RomName
#define AudioSignal			g_AudioSignal
#define CicChip				g_CicChip
#define ShowDListAListCount g_ShowDListAListCount
#define ShowPifRamErrors	g_ShowPifRamErrors
#define CountPerOp			g_CountPerOp
#define DelaySI				g_DelaySI
#define DisableRegCaching	g_DisableRegCaching
#define ShowCompMem			g_ShowCompMem
#define UseLinking			g_UseLinking
#define LookUpMode          g_LookUpMode

//Plugins
//#define AudioIntrReg		(*g_AudioIntrReg)
#define Controllers			g_Controllers

//Misc
#define AppName				GetAppName()
#define TLB_WriteMap		g_TLB_WriteMap
#define TLB_ReadMap			g_TLB_ReadMap
#define RdramSize			g_RdramSize
#define CurrentFrame        g_CurrentFrame
#define Frequency	        g_Frequency
#define LastFrame	        g_LastFrame
#define Frames		        g_Frames

