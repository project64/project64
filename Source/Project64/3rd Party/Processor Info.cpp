// Filename: Processor.h
// =====================
// Author: Benjamin Jurke
// File history: 27.02.2002   File created.
///////////////////////////////////////////

// Options:
///////////
#define PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
// Includes <windows.h> --> code gets os-dependend (Win32)


typedef struct ProcessorExtensions
{
	bool FPU_FloatingPointUnit;
	bool VME_Virtual8086ModeEnhancements;
	bool DE_DebuggingExtensions;
	bool PSE_PageSizeExtensions;
	bool TSC_TimeStampCounter;
	bool MSR_ModelSpecificRegisters;
	bool PAE_PhysicalAddressExtension;
	bool MCE_MachineCheckException;
	bool CX8_COMPXCHG8B_Instruction;
	bool APIC_AdvancedProgrammableInterruptController;
	unsigned int APIC_ID;
	bool SEP_FastSystemCall;
	bool MTRR_MemoryTypeRangeRegisters;
	bool PGE_PTE_GlobalFlag;
	bool MCA_MachineCheckArchitecture;
	bool CMOV_ConditionalMoveAndCompareInstructions;
	bool FGPAT_PageAttributeTable;
	bool PSE36_36bitPageSizeExtension;
	bool PN_ProcessorSerialNumber;
	bool CLFSH_CFLUSH_Instruction;
	unsigned int CLFLUSH_InstructionCacheLineSize;
	bool DS_DebugStore;
	bool ACPI_ThermalMonitorAndClockControl;
	bool EMMX_MultimediaExtensions;
	bool MMX_MultimediaExtensions;
	bool FXSR_FastStreamingSIMD_ExtensionsSaveRestore;
	bool SSE_StreamingSIMD_Extensions;
	bool SSE2_StreamingSIMD2_Extensions;
	bool SS_SelfSnoop;
	bool HT_HyperThreading;
	unsigned int HT_HyterThreadingSiblings;
	bool TM_ThermalMonitor;
	bool IA64_Intel64BitArchitecture;
	bool _3DNOW_InstructionExtensions;
	bool _E3DNOW_InstructionExtensions;
	bool AA64_AMD64BitArchitecture;
} ProcessorExtensions;

typedef struct ProcessorCache
{
	bool bPresent;
	char strSize[32];
	unsigned int uiAssociativeWays;
	unsigned int uiLineSize;
	bool bSectored;
	char strCache[128];
} ProcessorCache;

typedef struct ProcessorL1Cache
{
    ProcessorCache Instruction;
	ProcessorCache Data;
} ProcessorL1Cache;

typedef struct ProcessorTLB
{
	bool bPresent;
	char strPageSize[32];
	unsigned int uiAssociativeWays;
	unsigned int uiEntries;
	char strTLB[128];
} ProcessorTLB;

typedef struct ProcessorInfo
{
	char strVendor[16];
	unsigned int uiFamily;
	unsigned int uiExtendedFamily;
	char strFamily[64];
	unsigned int uiModel;
	unsigned int uiExtendedModel;
	char strModel[128];
	unsigned int uiStepping;
	unsigned int uiType;
	char strType[64];
	unsigned int uiBrandID;
	char strBrandID[64];
	char strProcessorSerial[64];
	unsigned long MaxSupportedLevel;
	unsigned long MaxSupportedExtendedLevel;
	ProcessorExtensions _Ext;
	ProcessorL1Cache _L1;
	ProcessorCache _L2;
	ProcessorCache _L3;
	ProcessorCache _Trace;
	ProcessorTLB _Instruction;
	ProcessorTLB _Data;
} ProcessorInfo;


// CProcessor
// ==========
// Class for detecting the processor name, type and available
// extensions as long as it's speed.
/////////////////////////////////////////////////////////////
class CProcessor
{
// Constructor / Destructor:
////////////////////////////
public:
	CProcessor();

// Private vars:
////////////////
private:
	__int64 uqwFrequency;
	char strCPUName[128];
	ProcessorInfo CPUInfo;

// Private functions:
/////////////////////
private:
	bool AnalyzeIntelProcessor();
	bool AnalyzeAMDProcessor();
	bool AnalyzeUnknownProcessor();
	bool CheckCPUIDPresence();
	void DecodeProcessorConfiguration(unsigned int cfg);
	void TranslateProcessorConfiguration();
	void GetStandardProcessorConfiguration();
	void GetStandardProcessorExtensions();

// Public functions:
////////////////////
public:
	__int64 GetCPUFrequency(unsigned int uiMeasureMSecs);
	const ProcessorInfo *GetCPUInfo();
	bool CPUInfoToText(char *strBuffer, unsigned int uiMaxLen);
	bool WriteInfoTextFile(const char *strFilename);
};

bool CPUInfoToText(char * /*strBuffer*/, unsigned int /*uiMaxLen*/)
{
	CProcessor cpu;
	cpu.WriteInfoTextFile("D:\\cpu.txt");
	return false;
	//return cpu.CPUInfoToText(strBuffer,uiMaxLen);
}


// Filename: Processor.cpp
// =======================
// Author: Benjamin Jurke
// File history: 27.02.2002  - File created. Support for Intel and AMD processors
//               05.03.2002  - Fixed the CPUID bug: On Pre-Pentium CPUs the CPUID
//                             command is not available
//                           - The CProcessor::WriteInfoTextFile function do not 
//                             longer use Win32 file functions (-> os independend)
//                           - Optional include of the windows.h header which is
//                             still need for CProcessor::GetCPUFrequency.
//               06.03.2002  - My birthday (18th :-))
//                           - Replaced the '\r\n' line endings in function 
//                             CProcessor::CPUInfoToText by '\n'
//                           - Replaced unsigned __int64 by signed __int64 for
//                             solving some compiler conversion problems
//                           - Fixed a bug at family=6, model=6 (Celeron -> P2)
//////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <string.h>
#include <memory.h>


#ifdef PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
#include <windows.h>
// We need the QueryPerformanceCounter and Sleep functions
#endif


// Some macros we often need
////////////////////////////
#define CheckBit(var, bit)   ((var & (1 << bit)) ? true : false)


// CProcessor::CProcessor
// ======================
// Class constructor:
/////////////////////////
CProcessor::CProcessor()
{
	uqwFrequency = 0;
	memset(&CPUInfo, 0, sizeof(CPUInfo));
}

// unsigned __int64 CProcessor::GetCPUFrequency(unsigned int uiMeasureMSecs)
// =========================================================================
// Function to measure the current CPU frequency
////////////////////////////////////////////////////////////////////////////
__int64 CProcessor::GetCPUFrequency(unsigned int uiMeasureMSecs)
{
#ifndef PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
	return 0;
#else
	// If there are invalid measure time parameters, zero msecs for example,
	// we've to exit the function
	if (uiMeasureMSecs < 1)
	{
		// If theres already a measured frequency available, we return it
        if (uqwFrequency > 0)
			return uqwFrequency;
		else
			return 0;
	}

	// Now we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return 0;

	// First we get the CPUID standard level 0x00000001
	unsigned long reg;
	__asm
	{
		mov eax, 1
        cpuid
		mov reg, edx
	}

	// Then we check, if the RDTSC (Real Date Time Stamp Counter) is available.
	// This function is necessary for our measure process.
	if (!(reg & (1 << 4)))
		return 0;

	// After that we declare some vars and check the frequency of the high
	// resolution timer for the measure process.
	// If there's no high-res timer, we exit.
	__int64 starttime, endtime, timedif, freq, start, end, dif;
	if (!QueryPerformanceFrequency((LARGE_INTEGER *) &freq))
		return 0;

	// Now we can init the measure process. We set the process and thread priority
	// to the highest available level (Realtime priority). Also we focus the
	// first processor in the multiprocessor system.
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	unsigned long dwCurPriorityClass = GetPriorityClass(hProcess);
	int iCurThreadPriority = GetThreadPriority(hThread);
	unsigned long dwProcessMask, dwSystemMask, dwNewMask = 1;
	GetProcessAffinityMask(hProcess, &dwProcessMask, &dwSystemMask);

	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	SetProcessAffinityMask(hProcess, dwNewMask);

	// Now we call a CPUID to ensure, that all other prior called functions are
	// completed now (serialization)
	__asm cpuid

	// We ask the high-res timer for the start time
	QueryPerformanceCounter((LARGE_INTEGER *) &starttime);

	// Then we get the current cpu clock and store it
	__asm 
	{
		rdtsc
		mov dword ptr [start+4], edx
		mov dword ptr [start], eax
	}

	// Now we wart for some msecs
	Sleep(uiMeasureMSecs);

	// We ask for the end time
	QueryPerformanceCounter((LARGE_INTEGER *) &endtime);

	// And also for the end cpu clock
	__asm 
	{
		rdtsc
		mov dword ptr [end+4], edx
		mov dword ptr [end], eax
	}

	// Now we can restore the default process and thread priorities
	SetProcessAffinityMask(hProcess, dwProcessMask);
	SetThreadPriority(hThread, iCurThreadPriority);
	SetPriorityClass(hProcess, dwCurPriorityClass);

	// Then we calculate the time and clock differences
	dif = end - start;
	timedif = endtime - starttime;

	// And finally the frequency is the clock difference divided by the time
	// difference. 
	uqwFrequency = (__int64) (((double) dif) / (((double) timedif) / (__int64)freq));

	// At last we just return the frequency that is also stored in the call
	// member var uqwFrequency
	return uqwFrequency;
#endif
}

// bool CProcessor::AnalyzeIntelProcessor()
// ========================================
// Private class function for analyzing an Intel processor
//////////////////////////////////////////////////////////
bool CProcessor::AnalyzeIntelProcessor()
{
	unsigned long eaxreg, ebxreg, edxreg;

	// First we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return false;

	// Now we get the CPUID standard level 0x00000001
	__asm
	{
		mov eax, 1
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
		mov edxreg, edx
	}
    
	// Then get the cpu model, family, type, stepping and brand id by masking
	// the eax and ebx register
	CPUInfo.uiStepping = eaxreg & 0xF;
	CPUInfo.uiModel    = (eaxreg >> 4) & 0xF;
	CPUInfo.uiFamily   = (eaxreg >> 8) & 0xF;
	CPUInfo.uiType     = (eaxreg >> 12) & 0x3;
	CPUInfo.uiBrandID  = ebxreg & 0xF;

	// Now we can translate the type number to a more understandable string format
    switch (CPUInfo.uiType)
	{
		case 0:			// Type = 0:  Original OEM processor
			strcpy(CPUInfo.strType, "Original OEM");
			strcpy(strCPUName, CPUInfo.strType);
			strcat(strCPUName, " ");
			break;
		case 1:			// Type = 1:  Overdrive processor
			strcpy(CPUInfo.strType, "Overdrive");
			strcpy(strCPUName, CPUInfo.strType);
			strcat(strCPUName, " ");
			break;
		case 2:			// Type = 2:  Dual-capable processor
			strcpy(CPUInfo.strType, "Dual-capable");
			strcpy(strCPUName, CPUInfo.strType);
			strcat(strCPUName, " ");
			break;
		case 3:			// Type = 3:  Reserved for future use
			strcpy(CPUInfo.strType, "Reserved");
			break;
		default:		// This should be never called, cause we just mask 2 bits --> [0..3]
			strcpy(CPUInfo.strType, "Unknown");
			break;
    }

	// Then we translate the brand id:
	switch (CPUInfo.uiBrandID)
	{
		case 0:			// Brand id = 0:  Brand id not supported on this processor
			strcpy(CPUInfo.strBrandID, "Not supported");
			break;
		case 1:			// Brand id = 1:  Intel Celeron (0.18 µm) processor
			strcpy(CPUInfo.strBrandID, "0.18 µm Intel Celeron");
			break;
		case 2:			// Brand id = 2:  Intel Pentium III (0.18 µm) processor
			strcpy(CPUInfo.strBrandID, "0.18 µm Intel Pentium III");
			break;
		case 3:			// Brand id = 3:  Model dependent
			if (CPUInfo.uiModel == 6)	// If the cpu model is Celeron (well, I'm NOT SURE!!!)
				strcpy(CPUInfo.strBrandID, "0.13 µm Intel Celeron");
			else
				strcpy(CPUInfo.strBrandID, "0.18 µm Intel Pentium III Xeon");
			break;
		case 4:			// Brand id = 4:  Intel Pentium III Tualatin (0.13 µm) processor
			strcpy(CPUInfo.strBrandID, "0.13 µm Intel Pentium III");
			break;
		case 6:			// Brand id = 6:  Intel Pentium III mobile (0.13 µm) processor
			strcpy(CPUInfo.strBrandID, "0.13 µm Intel Pentium III mobile");
			break;
		case 7:			// Brand id = 7:  Intel Celeron mobile (0.13 µm) processor
			strcpy(CPUInfo.strBrandID, "0.13 µm Intel Celeron mobile");
			break;
		case 8:			// Brand id = 8:  Intel Pentium 4 Willamette (0.18 µm) processor
			strcpy(CPUInfo.strBrandID, "0.18 µm Intel Pentium 4");
			break;
		case 9:			// Brand id = 9:  Intel Pentium 4 Northwood (0.13 µm) processor
			strcpy(CPUInfo.strBrandID, "0.13 µm Intel Pentium 4");
			break;
		case 0xA:		// Brand id = 0xA:  Intel Pentium 4 Northwood (0.13 µm processor) 
			strcpy(CPUInfo.strBrandID, "0.13 µm Intel Pentium 4");
			break;		// No idea, where the difference to id=9 is
		case 0xB:		// Brand id = 0xB:  Intel Pentium 4 Northwood Xeon (0.13 µm processor)
			strcpy(CPUInfo.strBrandID, "0.13 µm Intel Pentium 4 Xeon");
			break;
		case 0xE:		// Brand id = 0xE:  Intel Pentium 4 Willamette Xeon (0.18 µm processor)
			strcpy(CPUInfo.strBrandID, "0.18 µm Intel Pentium 4 Xeon");
			break;
		default:		// Should be never called, but sure is sure
			strcpy(CPUInfo.strBrandID, "Unknown");
			break;
	}

	// Then we translate the cpu family
    switch (CPUInfo.uiFamily)
	{
		case 3:			// Family = 3:  i386 (80386) processor family
			strcpy(CPUInfo.strFamily, "Intel i386");
			break;
		case 4:			// Family = 4:  i486 (80486) processor family
			strcpy(CPUInfo.strFamily, "Intel i486");
			break;
		case 5:			// Family = 5:  Pentium (80586) processor family
			strcpy(CPUInfo.strFamily, "Intel Pentium");
			break;
		case 6:			// Family = 6:  Pentium Pro (80686) processor family
			strcpy(CPUInfo.strFamily, "Intel Pentium Pro");
			break;
		case 15:		// Family = 15:  Extended family specific
			// Masking the extended family
			CPUInfo.uiExtendedFamily = (eaxreg >> 20) & 0xFF;
			switch (CPUInfo.uiExtendedFamily)
			{
				case 0:			// Family = 15, Ext. Family = 0:  Pentium 4 (80786 ??) processor family
					strcpy(CPUInfo.strFamily, "Intel Pentium 4");
					break;
				case 1:			// Family = 15, Ext. Family = 1:  McKinley (64-bit) processor family
					strcpy(CPUInfo.strFamily, "Intel McKinley (IA-64)");
					break;
				default:		// Sure is sure
					strcpy(CPUInfo.strFamily, "Unknown Intel Pentium 4+");
					break;
			}
			break;
		default:		// Failsave
			strcpy(CPUInfo.strFamily, "Unknown");
			break;
    }

	// Now we come to the big deal, the exact model name
	switch (CPUInfo.uiFamily)
	{
		case 3:			// i386 (80386) processor family
			strcpy(CPUInfo.strModel, "Unknown Intel i386");
			strcat(strCPUName, "Intel i386");
			break;
		case 4:			// i486 (80486) processor family
			switch (CPUInfo.uiModel)
			{
				case 0:			// Model = 0:  i486 DX-25/33 processor model
					strcpy(CPUInfo.strModel, "Intel i486 DX-25/33");
					strcat(strCPUName, "Intel i486 DX-25/33");
					break;
				case 1:			// Model = 1:  i486 DX-50 processor model
					strcpy(CPUInfo.strModel, "Intel i486 DX-50");
					strcat(strCPUName, "Intel i486 DX-50");
					break;
				case 2:			// Model = 2:  i486 SX processor model
					strcpy(CPUInfo.strModel, "Intel i486 SX");
					strcat(strCPUName, "Intel i486 SX");
					break;
				case 3:			// Model = 3:  i486 DX2 (with i487 numeric coprocessor) processor model
					strcpy(CPUInfo.strModel, "Intel i486 487/DX2");
					strcat(strCPUName, "Intel i486 DX2 with i487 numeric coprocessor");
					break;
				case 4:			// Model = 4:  i486 SL processor model (never heard ?!?)
					strcpy(CPUInfo.strModel, "Intel i486 SL");
					strcat(strCPUName, "Intel i486 SL");
					break;
				case 5:			// Model = 5:  i486 SX2 processor model
					strcpy(CPUInfo.strModel, "Intel i486 SX2");
					strcat(strCPUName, "Intel i486 SX2");
					break;
				case 7:			// Model = 7:  i486 write-back enhanced DX2 processor model
					strcpy(CPUInfo.strModel, "Intel i486 write-back enhanced DX2");
					strcat(strCPUName, "Intel i486 write-back enhanced DX2");
					break;
				case 8:			// Model = 8:  i486 DX4 processor model
					strcpy(CPUInfo.strModel, "Intel i486 DX4");
					strcat(strCPUName, "Intel i486 DX4");
					break;
				case 9:			// Model = 9:  i486 write-back enhanced DX4 processor model
					strcpy(CPUInfo.strModel, "Intel i486 write-back enhanced DX4");
					strcat(strCPUName, "Intel i486 DX4");
					break;
				default:		// ...
					strcpy(CPUInfo.strModel, "Unknown Intel i486");
					strcat(strCPUName, "Intel i486 (Unknown model)");
					break;
			}
			break;
		case 5:			// Pentium (80586) processor family
			switch (CPUInfo.uiModel)
			{
				case 0:			// Model = 0:  Pentium (P5 A-Step) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium (P5 A-Step)");
					strcat(strCPUName, "Intel Pentium (P5 A-Step core)");
					break;		// Famous for the DIV bug, as far as I know
				case 1:			// Model = 1:  Pentium 60/66 processor model
					strcpy(CPUInfo.strModel, "Intel Pentium 60/66 (P5)");
					strcat(strCPUName, "Intel Pentium 60/66 (P5 core)");
					break;
				case 2:			// Model = 2:  Pentium 75-200 (P54C) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium 75-200 (P54C)");
					strcat(strCPUName, "Intel Pentium 75-200 (P54C core)");
					break;
				case 3:			// Model = 3:  Pentium overdrive for 486 systems processor model
					strcpy(CPUInfo.strModel, "Intel Pentium for 486 system (P24T Overdrive)");
					strcat(strCPUName, "Intel Pentium for 486 (P24T overdrive core)");
					break;
				case 4:			// Model = 4:  Pentium MMX processor model
					strcpy(CPUInfo.strModel, "Intel Pentium MMX (P55C)");
					strcat(strCPUName, "Intel Pentium MMX (P55C core)");
					break;
				case 7:			// Model = 7:  Pentium processor model (don't know difference to Model=2)
					strcpy(CPUInfo.strModel, "Intel Pentium (P54C)");
					strcat(strCPUName, "Intel Pentium (P54C core)");
					break;
				case 8:			// Model = 8:  Pentium MMX (0.25 µm) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium MMX (P55C), 0.25 µm");
					strcat(strCPUName, "Intel Pentium MMX (P55C core), 0.25 µm");
					break;
				default:		// ...
					strcpy(CPUInfo.strModel, "Unknown Intel Pentium");
					strcat(strCPUName, "Intel Pentium (Unknown P5-model)");
					break;
			}
			break;
		case 6:			// Pentium Pro (80686) processor family
			switch (CPUInfo.uiModel)
			{
				case 0:			// Model = 0:  Pentium Pro (P6 A-Step) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium Pro (P6 A-Step)");
					strcat(strCPUName, "Intel Pentium Pro (P6 A-Step core)");
					break;
				case 1:			// Model = 1:  Pentium Pro
					strcpy(CPUInfo.strModel, "Intel Pentium Pro (P6)");
					strcat(strCPUName, "Intel Pentium Pro (P6 core)");
					break;
				case 3:			// Model = 3:  Pentium II (66 MHz FSB, I think) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium II Model 3, 0.28 µm");
					strcat(strCPUName, "Intel Pentium II (Model 3 core, 0.28 µm process)");
					break;
				case 5:			// Model = 5:  Pentium II/Xeon/Celeron (0.25 µm) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium II Model 5/Xeon/Celeron, 0.25 µm");
					strcat(strCPUName, "Intel Pentium II/Xeon/Celeron (Model 5 core, 0.25 µm process)");
					break;
				case 6:			// Model = 6:  Pentium II with internal L2 cache
					strcpy(CPUInfo.strModel, "Intel Pentium II - internal L2 cache");
					strcat(strCPUName, "Intel Pentium II with internal L2 cache");
					break;
				case 7:			// Model = 7:  Pentium III/Xeon (extern L2 cache) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium III/Pentium III Xeon - external L2 cache, 0.25 µm");
					strcat(strCPUName, "Intel Pentium III/Pentium III Xeon (0.25 µm process) with external L2 cache");
					break;
				case 8:			// Model = 8:  Pentium III/Xeon/Celeron (256 KB on-die L2 cache) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium III/Celeron/Pentium III Xeon - internal L2 cache, 0.18 µm");
					// We want to know it exactly:
					switch (CPUInfo.uiBrandID)
					{
						case 1:			// Model = 8, Brand id = 1:  Celeron (on-die L2 cache) processor model
							strcat(strCPUName, "Intel Celeron (0.18 µm process) with internal L2 cache");
							break;
                        case 2:			// Model = 8, Brand id = 2:  Pentium III (on-die L2 cache) processor model (my current cpu :-))
							strcat(strCPUName, "Intel Pentium III (0.18 µm process) with internal L2 cache");
							break;
						case 3:			// Model = 8, Brand id = 3:  Pentium III Xeon (on-die L2 cache) processor model
                            strcat(strCPUName, "Intel Pentium III Xeon (0.18 µm process) with internal L2 cache");
							break;
						default:		// ...²
							strcat(strCPUName, "Intel Pentium III core (unknown model, 0.18 µm process) with internal L2 cache");
							break;
					}
					break;
				case 0xA:		// Model = 0xA:  Pentium III/Xeon/Celeron (1 or 2 MB on-die L2 cache) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium III/Celeron/Pentium III Xeon - internal L2 cache, 0.18 µm");
					// Exact detection:
					switch (CPUInfo.uiBrandID)
					{
						case 1:			// Model = 0xA, Brand id = 1:  Celeron (1 or 2 MB on-die L2 cache (does it exist??)) processor model
							strcat(strCPUName, "Intel Celeron (0.18 µm process) with internal L2 cache");
							break;
                        case 2:			// Model = 0xA, Brand id = 2:  Pentium III (1 or 2 MB on-die L2 cache (never seen...)) processor model
							strcat(strCPUName, "Intel Pentium III (0.18 µm process) with internal L2 cache");
							break;
						case 3:			// Model = 0xA, Brand id = 3:  Pentium III Xeon (1 or 2 MB on-die L2 cache) processor model
                            strcat(strCPUName, "Intel Pentium III Xeon (0.18 µm process) with internal L2 cache");
							break;
						default:		// Getting bored of this............
							strcat(strCPUName, "Intel Pentium III core (unknown model, 0.18 µm process) with internal L2 cache");
							break;
					}
					break;
				case 0xB:		// Model = 0xB: Pentium III/Xeon/Celeron (Tualatin core, on-die cache) processor model
					strcpy(CPUInfo.strModel, "Intel Pentium III/Celeron/Pentium III Xeon - internal L2 cache, 0.13 µm");
					// Omniscient: ;-)
					switch (CPUInfo.uiBrandID)
					{
						case 3:			// Model = 0xB, Brand id = 3:  Celeron (Tualatin core) processor model
							strcat(strCPUName, "Intel Celeron (Tualatin core, 0.13 µm process) with internal L2 cache");
							break;
                        case 4:			// Model = 0xB, Brand id = 4:  Pentium III (Tualatin core) processor model
							strcat(strCPUName, "Intel Pentium III (Tualatin core, 0.13 µm process) with internal L2 cache");
							break;
						case 7:			// Model = 0xB, Brand id = 7:  Celeron mobile (Tualatin core) processor model
                            strcat(strCPUName, "Intel Celeron mobile (Tualatin core, 0.13 µm process) with internal L2 cache");
							break;
						default:		// *bored*
							strcat(strCPUName, "Intel Pentium III Tualatin core (unknown model, 0.13 µm process) with internal L2 cache");
							break;
					}
					break;
				default:		// *more bored*
					strcpy(CPUInfo.strModel, "Unknown Intel Pentium Pro");
					strcat(strCPUName, "Intel Pentium Pro (Unknown model)");
					break;
			}
			break;
		case 15:		// Extended processor family
			// Masking the extended model
			CPUInfo.uiExtendedModel = (eaxreg >> 16) & 0xFF;
			switch (CPUInfo.uiModel)
			{
				case 0:			// Model = 0:  Pentium 4 Willamette (A-Step) core
					if ((CPUInfo.uiBrandID) == 8)	// Brand id = 8:  P4 Willamette
					{
						strcpy(CPUInfo.strModel, "Intel Pentium 4 Willamette (A-Step)");
						strcat(strCPUName, "Intel Pentium 4 Willamette (A-Step)");
					}
					else							// else Xeon
					{
						strcpy(CPUInfo.strModel, "Intel Pentium 4 Willamette Xeon (A-Step)");
						strcat(strCPUName, "Intel Pentium 4 Willamette Xeon (A-Step)");
					}
					break;
				case 1:			// Model = 1:  Pentium 4 Willamette core
					if ((CPUInfo.uiBrandID) == 8)	// Brand id = 8:  P4 Willamette
					{
						strcpy(CPUInfo.strModel, "Intel Pentium 4 Willamette");
						strcat(strCPUName, "Intel Pentium 4 Willamette");
					}
					else							// else Xeon
					{
						strcpy(CPUInfo.strModel, "Intel Pentium 4 Willamette Xeon");
						strcat(strCPUName, "Intel Pentium 4 Willamette Xeon");
					}
					break;
				case 2:			// Model = 2:  Pentium 4 Northwood core
					if (((CPUInfo.uiBrandID) == 9) || ((CPUInfo.uiBrandID) == 0xA))		// P4 Willamette
					{
						strcpy(CPUInfo.strModel, "Intel Pentium 4 Northwood");
						strcat(strCPUName, "Intel Pentium 4 Northwood");
					}
					else							// Xeon
					{
						strcpy(CPUInfo.strModel, "Intel Pentium 4 Northwood Xeon");
						strcat(strCPUName, "Intel Pentium 4 Northwood Xeon");
					}
					break;
				default:		// Silly stupid never used failsave option
					strcpy(CPUInfo.strModel, "Unknown Intel Pentium 4");
					strcat(strCPUName, "Intel Pentium 4 (Unknown model)");
					break;
			}
			break;
		default:		// *grmpf*
			strcpy(CPUInfo.strModel, "Unknown Intel model");
			strcat(strCPUName, "Intel (Unknown model)");
			break;
    }

	// After the long processor model block we now come to the processors serial
	// number.
	// First of all we check if the processor supports the serial number
	if (CPUInfo.MaxSupportedLevel >= 3)
	{
		// If it supports the serial number CPUID level 0x00000003 we read the data
		unsigned long sig1, sig2, sig3;
		__asm
		{
			mov eax, 1
			cpuid
			mov sig1, eax
			mov eax, 3
			cpuid
			mov sig2, ecx
			mov sig3, edx
		}
		// Then we convert the data to an readable string
		sprintf(CPUInfo.strProcessorSerial, "%04lX-%04lX-%04lX-%04lX-%04lX-%04lX", sig1 >> 16, sig1 & 0xFFFF, sig3 >> 16, sig3 & 0xFFFF, sig2 >> 16, sig2 & 0xFFFF);
	}
	else
	{
		// If there's no serial number support we just mark put "No serial number"
		strcpy(CPUInfo.strProcessorSerial, "No Processor Serial Number");
	}

	// Now we get the standard processor extensions
	GetStandardProcessorExtensions();

	// And finally the processor configuration (caches, TLBs, ...) and translate
	// the data to readable strings
	GetStandardProcessorConfiguration();
	TranslateProcessorConfiguration();

	// At last...
	return true;
}

// bool CProcessor::AnalyzeAMDProcessor()
// ======================================
// Private class function for analyzing an AMD processor
////////////////////////////////////////////////////////
bool CProcessor::AnalyzeAMDProcessor()
{
	unsigned long eaxreg, ebxreg, ecxreg, edxreg;

	// First of all we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return 0;

	// Now we get the CPUID standard level 0x00000001
	__asm
	{
		mov eax, 1
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
		mov edxreg, edx
	}
    
	// Then we mask the model, family, stepping and type (AMD does not support brand id)
	CPUInfo.uiStepping = eaxreg & 0xF;
	CPUInfo.uiModel    = (eaxreg >> 4) & 0xF;
	CPUInfo.uiFamily   = (eaxreg >> 8) & 0xF;
	CPUInfo.uiType     = (eaxreg >> 12) & 0x3;

	// After that, we translate the processor type (see CProcessor::AnalyzeIntelProcessor()
	// for further comments on this)
    switch (CPUInfo.uiType)
	{
		case 0:
			strcpy(CPUInfo.strType, "Original OEM");
			strcpy(strCPUName, CPUInfo.strType);
			strcat(strCPUName, " ");
			break;
		case 1:
			strcpy(CPUInfo.strType, "Overdrive");
			strcpy(strCPUName, CPUInfo.strType);
			strcat(strCPUName, " ");
			break;
		case 2:
			strcpy(CPUInfo.strType, "Dual-capable");
			strcpy(strCPUName, CPUInfo.strType);
			strcat(strCPUName, " ");
			break;
		case 3:
			strcpy(CPUInfo.strType, "Reserved");
			break;
		default:
			strcpy(CPUInfo.strType, "Unknown");
			break;
    }

	// Now we check if the processor supports the brand id string extended CPUID level
	if (CPUInfo.MaxSupportedExtendedLevel >= 0x80000004)
	{
		// If it supports the extended CPUID level 0x80000004 we read the data
		char tmp[52];
		memset(tmp, 0, sizeof(tmp));
        __asm
		{
			mov eax, 0x80000002
			cpuid
			mov dword ptr [tmp], eax
			mov dword ptr [tmp+4], ebx
			mov dword ptr [tmp+8], ecx
			mov dword ptr [tmp+12], edx
			mov eax, 0x80000003
			cpuid
			mov dword ptr [tmp+16], eax
			mov dword ptr [tmp+20], ebx
			mov dword ptr [tmp+24], ecx
			mov dword ptr [tmp+28], edx
			mov eax, 0x80000004
			cpuid
			mov dword ptr [tmp+32], eax
			mov dword ptr [tmp+36], ebx
			mov dword ptr [tmp+40], ecx
			mov dword ptr [tmp+44], edx
		}
		// And copy it to the brand id string
		strcpy(CPUInfo.strBrandID, tmp);
	}
	else
	{
		// Or just tell there is no brand id string support
		strcpy(CPUInfo.strBrandID, "Not supported");
	}

	// After that we translate the processor family
    switch(CPUInfo.uiFamily)
	{
		case 4:			// Family = 4:  486 (80486) or 5x86 (80486) processor family
			switch (CPUInfo.uiModel)
			{
				case 3:			// Thanks to AMD for this nice form of family
				case 7:			// detection.... *grmpf*
				case 8:
				case 9:
					strcpy(CPUInfo.strFamily, "AMD 80486");
					break;
				case 0xE:
				case 0xF:
					strcpy(CPUInfo.strFamily, "AMD 5x86");
					break;
				default:
					strcpy(CPUInfo.strFamily, "Unknown family");
					break;
			}
			break;
		case 5:			// Family = 5:  K5 or K6 processor family
			switch (CPUInfo.uiModel)
			{
				case 0:
				case 1:
				case 2:
				case 3:
					strcpy(CPUInfo.strFamily, "AMD K5");
					break;
				case 6:
				case 7:
				case 8:
				case 9:
					strcpy(CPUInfo.strFamily, "AMD K6");
					break;
				default:
					strcpy(CPUInfo.strFamily, "Unknown family");
					break;
			}
			break;
		case 6:			// Family = 6:  K7 (Athlon, ...) processor family
			strcpy(CPUInfo.strFamily, "AMD K7");
			break;
		default:		// For security
			strcpy(CPUInfo.strFamily, "Unknown family");
			break;
	}

	// After the family detection we come to the specific processor model
	// detection
	switch (CPUInfo.uiFamily)
	{
		case 4:			// Family = 4:  486 (80486) or 5x85 (80486) processor family
			switch (CPUInfo.uiModel)
			{
				case 3:			// Model = 3:  80486 DX2
					strcpy(CPUInfo.strModel, "AMD 80486 DX2");
					strcat(strCPUName, "AMD 80486 DX2");
					break;
				case 7:			// Model = 7:  80486 write-back enhanced DX2
					strcpy(CPUInfo.strModel, "AMD 80486 write-back enhanced DX2");
					strcat(strCPUName, "AMD 80486 write-back enhanced DX2");
					break;
				case 8:			// Model = 8:  80486 DX4
					strcpy(CPUInfo.strModel, "AMD 80486 DX4");
					strcat(strCPUName, "AMD 80486 DX4");
					break;
				case 9:			// Model = 9:  80486 write-back enhanced DX4
					strcpy(CPUInfo.strModel, "AMD 80486 write-back enhanced DX4");
					strcat(strCPUName, "AMD 80486 write-back enhanced DX4");
					break;
				case 0xE:		// Model = 0xE:  5x86
					strcpy(CPUInfo.strModel, "AMD 5x86");
					strcat(strCPUName, "AMD 5x86");
					break;
				case 0xF:		// Model = 0xF:  5x86 write-back enhanced (oh my god.....)
					strcpy(CPUInfo.strModel, "AMD 5x86 write-back enhanced");
					strcat(strCPUName, "AMD 5x86 write-back enhanced");
					break;
				default:		// ...
					strcpy(CPUInfo.strModel, "Unknown AMD 80486 or 5x86 model");
					strcat(strCPUName, "AMD 80486 or 5x86 (Unknown model)");
					break;
			}
			break;
		case 5:			// Family = 5:  K5 / K6 processor family
			switch (CPUInfo.uiModel)
			{
				case 0:			// Model = 0:  K5 SSA 5 (Pentium Rating *ggg* 75, 90 and 100 Mhz)
					strcpy(CPUInfo.strModel, "AMD K5 SSA5 (PR75, PR90, PR100)");
					strcat(strCPUName, "AMD K5 SSA5 (PR75, PR90, PR100)");
					break;
				case 1:			// Model = 1:  K5 5k86 (PR 120 and 133 MHz)
					strcpy(CPUInfo.strModel, "AMD K5 5k86 (PR120, PR133)");
					strcat(strCPUName, "AMD K5 5k86 (PR120, PR133)");
					break;
				case 2:			// Model = 2:  K5 5k86 (PR 166 MHz)
					strcpy(CPUInfo.strModel, "AMD K5 5k86 (PR166)");
					strcat(strCPUName, "AMD K5 5k86 (PR166)");
					break;
				case 3:			// Model = 3:  K5 5k86 (PR 200 MHz)
					strcpy(CPUInfo.strModel, "AMD K5 5k86 (PR200)");
					strcat(strCPUName, "AMD K5 5k86 (PR200)");
					break;
				case 6:			// Model = 6:  K6
					strcpy(CPUInfo.strModel, "AMD K6 (0.30 µm)");
					strcat(strCPUName, "AMD K6 (0.30 µm)");
					break;
				case 7:			// Model = 7:  K6 (0.25 µm)
					strcpy(CPUInfo.strModel, "AMD K6 (0.25 µm)");
					strcat(strCPUName, "AMD K6 (0.25 µm)");
					break;
				case 8:			// Model = 8:  K6-2
					strcpy(CPUInfo.strModel, "AMD K6-2");
					strcat(strCPUName, "AMD K6-2");
					break;
				case 9:			// Model = 9:  K6-III
					strcpy(CPUInfo.strModel, "AMD K6-III");
					strcat(strCPUName, "AMD K6-III");
					break;
				case 0xD:		// Model = 0xD:  K6-2+ / K6-III+
					strcpy(CPUInfo.strModel, "AMD K6-2+ or K6-III+ (0.18 µm)");
					strcat(strCPUName, "AMD K6-2+ or K6-III+ (0.18 µm)");
					break;
				default:		// ...
					strcpy(CPUInfo.strModel, "Unknown AMD K5 or K6 model");
					strcat(strCPUName, "AMD K5 or K6 (Unknown model)");
					break;
			}
			break;
		case 6:			// Family = 6:  K7 processor family (AMDs first good processors)
			switch (CPUInfo.uiModel)
			{
				case 1:			// Athlon
					strcpy(CPUInfo.strModel, "AMD Athlon (0.25 µm)");
					strcat(strCPUName, "AMD Athlon (0.25 µm)");
					break;
				case 2:			// Athlon (0.18 µm)
					strcpy(CPUInfo.strModel, "AMD Athlon (0.18 µm)");
					strcat(strCPUName, "AMD Athlon (0.18 µm)");
					break;
				case 3:			// Duron (Spitfire core)
					strcpy(CPUInfo.strModel, "AMD Duron (Spitfire)");
					strcat(strCPUName, "AMD Duron (Spitfire core)");
					break;
				case 4:			// Athlon (Thunderbird core)
					strcpy(CPUInfo.strModel, "AMD Athlon (Thunderbird)");
					strcat(strCPUName, "AMD Athlon (Thunderbird core)");
					break;
				case 6:			// Athlon MP / Mobile Athlon (Palomino core)
					strcpy(CPUInfo.strModel, "AMD Athlon MP/Mobile Athlon (Palomino)");
					strcat(strCPUName, "AMD Athlon MP/Mobile Athlon (Palomino core)");
					break;
				case 7:			// Mobile Duron (Morgan core)
					strcpy(CPUInfo.strModel, "AMD Mobile Duron (Morgan)");
					strcat(strCPUName, "AMD Mobile Duron (Morgan core)");
					break;
				default:		// ...
					strcpy(CPUInfo.strModel, "Unknown AMD K7 model");
					strcat(strCPUName, "AMD K7 (Unknown model)");
					break;
			}
			break;
		default:		// ...
			strcpy(CPUInfo.strModel, "Unknown AMD model");
			strcat(strCPUName, "AMD (Unknown model)");
			break;
    }

	// Now we read the standard processor extension that are stored in the same
	// way the Intel standard extensions are
	GetStandardProcessorExtensions();

	// Then we check if theres an extended CPUID level support
	if (CPUInfo.MaxSupportedExtendedLevel >= 0x80000001)
	{
		// If we can access the extended CPUID level 0x80000001 we get the
		// edx register
		__asm
		{
			mov eax, 0x80000001
			cpuid
			mov edxreg, edx
		}

		// Now we can mask some AMD specific cpu extensions
		CPUInfo._Ext.EMMX_MultimediaExtensions					= CheckBit(edxreg, 22);
		CPUInfo._Ext.AA64_AMD64BitArchitecture					= CheckBit(edxreg, 29);
		CPUInfo._Ext._E3DNOW_InstructionExtensions				= CheckBit(edxreg, 30);
		CPUInfo._Ext._3DNOW_InstructionExtensions				= CheckBit(edxreg, 31);
	}

	// After that we check if the processor supports the ext. CPUID level
	// 0x80000006
	if (CPUInfo.MaxSupportedExtendedLevel >= 0x80000006)
	{
		// If it's present, we read it out
        __asm
		{
            mov eax, 0x80000005
			cpuid
			mov eaxreg, eax
			mov ebxreg, ebx
			mov ecxreg, ecx
			mov edxreg, edx
		}

		// Then we mask the L1 Data TLB information
		if ((ebxreg >> 16) && (eaxreg >> 16))
		{
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 KB / 2 MB / 4MB");
			CPUInfo._Data.uiAssociativeWays = (eaxreg >> 24) & 0xFF;
			CPUInfo._Data.uiEntries = (eaxreg >> 16) & 0xFF;
		}
		else if (eaxreg >> 16)
		{
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "2 MB / 4MB");
			CPUInfo._Data.uiAssociativeWays = (eaxreg >> 24) & 0xFF;
			CPUInfo._Data.uiEntries = (eaxreg >> 16) & 0xFF;
		}
		else if (ebxreg >> 16)
		{
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 KB");
			CPUInfo._Data.uiAssociativeWays = (ebxreg >> 24) & 0xFF;
			CPUInfo._Data.uiEntries = (ebxreg >> 16) & 0xFF;
		}
		if (CPUInfo._Data.uiAssociativeWays == 0xFF)
			CPUInfo._Data.uiAssociativeWays = (unsigned int) -1;

		// Now the L1 Instruction/Code TLB information
		if ((ebxreg & 0xFFFF) && (eaxreg & 0xFFFF))
		{
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 KB / 2 MB / 4MB");
			CPUInfo._Instruction.uiAssociativeWays = (eaxreg >> 8) & 0xFF;
			CPUInfo._Instruction.uiEntries = eaxreg & 0xFF;
		}
		else if (eaxreg & 0xFFFF)
		{
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "2 MB / 4MB");
			CPUInfo._Instruction.uiAssociativeWays = (eaxreg >> 8) & 0xFF;
			CPUInfo._Instruction.uiEntries = eaxreg & 0xFF;
		}
		else if (ebxreg & 0xFFFF)
		{
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 KB");
			CPUInfo._Instruction.uiAssociativeWays = (ebxreg >> 8) & 0xFF;
			CPUInfo._Instruction.uiEntries = ebxreg & 0xFF;
		}
		if (CPUInfo._Instruction.uiAssociativeWays == 0xFF)
			CPUInfo._Instruction.uiAssociativeWays = (unsigned int) -1;
		
		// Then we read the L1 data cache information
		if ((ecxreg >> 24) > 0)
		{
			CPUInfo._L1.Data.bPresent = true;
			sprintf(CPUInfo._L1.Data.strSize, "%d KB", ecxreg >> 24);
			CPUInfo._L1.Data.uiAssociativeWays = (ecxreg >> 15) & 0xFF;
			CPUInfo._L1.Data.uiLineSize = ecxreg & 0xFF;
		}
		// After that we read the L2 instruction/code cache information
		if ((edxreg >> 24) > 0)
		{
			CPUInfo._L1.Instruction.bPresent = true;
			sprintf(CPUInfo._L1.Instruction.strSize, "%d KB", edxreg >> 24);
			CPUInfo._L1.Instruction.uiAssociativeWays = (edxreg >> 15) & 0xFF;
			CPUInfo._L1.Instruction.uiLineSize = edxreg & 0xFF;
		}

		// Note: I'm not absolutely sure that the L1 page size code (the
		// 'if/else if/else if' structs above) really detects the real page
		// size for the TLB. Somebody should check it....

		// Now we read the ext. CPUID level 0x80000006
        __asm
		{
			mov eax, 0x80000006
			cpuid
			mov eaxreg, eax
			mov ebxreg, ebx
			mov ecxreg, ecx
		}

		// We only mask the unified L2 cache masks (never heard of an
		// L2 cache that is divided in data and code parts)
		if (((ecxreg >> 12) & 0xF) > 0)
		{
			CPUInfo._L2.bPresent = true;
			sprintf(CPUInfo._L2.strSize, "%d KB", ecxreg >> 16);
			switch ((ecxreg >> 12) & 0xF)
			{
				case 1:
					CPUInfo._L2.uiAssociativeWays = 1;
					break;
				case 2:
					CPUInfo._L2.uiAssociativeWays = 2;
					break;
				case 4:
					CPUInfo._L2.uiAssociativeWays = 4;
					break;
				case 6:
					CPUInfo._L2.uiAssociativeWays = 8;
					break;
				case 8:
					CPUInfo._L2.uiAssociativeWays = 16;
					break;
				case 0xF:
					CPUInfo._L2.uiAssociativeWays = (unsigned int) -1;
					break;
				default:
					CPUInfo._L2.uiAssociativeWays = 0;
					break;
			}
			CPUInfo._L2.uiLineSize = ecxreg & 0xFF;
		}
	}
	else
	{
		// If we could not detect the ext. CPUID level 0x80000006 we
		// try to read the standard processor configuration.
		GetStandardProcessorConfiguration();
	}
	// After reading we translate the configuration to strings
	TranslateProcessorConfiguration();

	// And finally exit
	return true;
}

// bool CProcessor::AnalyzeUnknownProcessor()
// ==========================================
// Private class function to analyze an unknown (No Intel or AMD) processor
///////////////////////////////////////////////////////////////////////////
bool CProcessor::AnalyzeUnknownProcessor()
{
	unsigned long eaxreg, ebxreg;

	// We check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return false;

	// First of all we read the standard CPUID level 0x00000001
	// This level should be available on every x86-processor clone
	__asm
	{
        mov eax, 1
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
	}
	// Then we mask the processor model, family, type and stepping
	CPUInfo.uiStepping = eaxreg & 0xF;
	CPUInfo.uiModel    = (eaxreg >> 4) & 0xF;
	CPUInfo.uiFamily   = (eaxreg >> 8) & 0xF;
	CPUInfo.uiType     = (eaxreg >> 12) & 0x3;

	// To have complete information we also mask the brand id
	CPUInfo.uiBrandID  = ebxreg & 0xF;

	// Then we get the standard processor extensions
	GetStandardProcessorExtensions();

	// Now we mark everything we do not know as unknown
	strcpy(strCPUName, "Unknown");

	strcpy(CPUInfo._Data.strTLB, "Unknown");
	strcpy(CPUInfo._Instruction.strTLB, "Unknown");
	
	strcpy(CPUInfo._Trace.strCache, "Unknown");
	strcpy(CPUInfo._L1.Data.strCache, "Unknown");
	strcpy(CPUInfo._L1.Instruction.strCache, "Unknown");
	strcpy(CPUInfo._L2.strCache, "Unknown");
	strcpy(CPUInfo._L3.strCache, "Unknown");

	strcpy(CPUInfo.strProcessorSerial, "Unknown / Not supported");

	// For the family, model and brand id we can only print the numeric value
	sprintf(CPUInfo.strBrandID, "Brand-ID number %d", CPUInfo.uiBrandID);
	sprintf(CPUInfo.strFamily, "Family number %d", CPUInfo.uiFamily);
	sprintf(CPUInfo.strModel, "Model number %d", CPUInfo.uiModel);

	// Nevertheless we can determine the processor type
    switch (CPUInfo.uiType)
	{
		case 0:
			strcpy(CPUInfo.strType, "Original OEM");
			break;
		case 1:
			strcpy(CPUInfo.strType, "Overdrive");
			break;
		case 2:
			strcpy(CPUInfo.strType, "Dual-capable");
			break;
		case 3:
			strcpy(CPUInfo.strType, "Reserved");
			break;
		default:
			strcpy(CPUInfo.strType, "Unknown");
			break;
    }

	// And thats it
	return true;
}

// bool CProcessor::CheckCPUIDPresence()
// =====================================
// This function checks if the CPUID command is available on the current
// processor
////////////////////////////////////////////////////////////////////////
bool CProcessor::CheckCPUIDPresence()
{
	unsigned long BitChanged;
	
	// We've to check if we can toggle the flag register bit 21
	// If we can't the processor does not support the CPUID command
	__asm
	{
		pushfd
		pop eax
		mov ebx, eax
		xor eax, 0x00200000 
		push eax
		popfd
		pushfd
		pop eax
		xor eax,ebx 
		mov BitChanged, eax
	}

	return ((BitChanged) ? true : false);
}

// void CProcessor::DecodeProcessorConfiguration(unsigned int cfg)
// ===============================================================
// This function (or switch ?!) just translates a one-byte processor configuration
// byte to understandable values
//////////////////////////////////////////////////////////////////////////////////
void CProcessor::DecodeProcessorConfiguration(unsigned int cfg)
{
	// First we ensure that there's only one single byte
	cfg &= 0xFF;

	// Then we do a big switch
	switch(cfg)
	{
		case 0:			// cfg = 0:  Unused
			break;
		case 0x1:		// cfg = 0x1:  code TLB present, 4 KB pages, 4 ways, 32 entries
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 KB");
			CPUInfo._Instruction.uiAssociativeWays = 4;
			CPUInfo._Instruction.uiEntries = 32;
			break;
		case 0x2:		// cfg = 0x2:  code TLB present, 4 MB pages, fully associative, 2 entries
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 MB");
			CPUInfo._Instruction.uiAssociativeWays = 4;
			CPUInfo._Instruction.uiEntries = 2;
			break;
		case 0x3:		// cfg = 0x3:  data TLB present, 4 KB pages, 4 ways, 64 entries
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 KB");
			CPUInfo._Data.uiAssociativeWays = 4;
			CPUInfo._Data.uiEntries = 64;
			break;
		case 0x4:		// cfg = 0x4:  data TLB present, 4 MB pages, 4 ways, 8 entries
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 MB");
			CPUInfo._Data.uiAssociativeWays = 4;
			CPUInfo._Data.uiEntries = 8;
			break;
		case 0x6:		// cfg = 0x6:  code L1 cache present, 8 KB, 4 ways, 32 byte lines
			CPUInfo._L1.Instruction.bPresent = true;
			strcpy(CPUInfo._L1.Instruction.strSize, "8 KB");
			CPUInfo._L1.Instruction.uiAssociativeWays = 4;
			CPUInfo._L1.Instruction.uiLineSize = 32;
			break;
		case 0x8:		// cfg = 0x8:  code L1 cache present, 16 KB, 4 ways, 32 byte lines
			CPUInfo._L1.Instruction.bPresent = true;
			strcpy(CPUInfo._L1.Instruction.strSize, "16 KB");
			CPUInfo._L1.Instruction.uiAssociativeWays = 4;
			CPUInfo._L1.Instruction.uiLineSize = 32;
			break;
		case 0xA:		// cfg = 0xA:  data L1 cache present, 8 KB, 2 ways, 32 byte lines
			CPUInfo._L1.Data.bPresent = true;
			strcpy(CPUInfo._L1.Data.strSize, "8 KB");
			CPUInfo._L1.Data.uiAssociativeWays = 2;
			CPUInfo._L1.Data.uiLineSize = 32;
			break;
		case 0xC:		// cfg = 0xC:  data L1 cache present, 16 KB, 4 ways, 32 byte lines
			CPUInfo._L1.Data.bPresent = true;
			strcpy(CPUInfo._L1.Data.strSize, "16 KB");
			CPUInfo._L1.Data.uiAssociativeWays = 4;
			CPUInfo._L1.Data.uiLineSize = 32;
			break;
		case 0x22:		// cfg = 0x22:  code and data L3 cache present, 512 KB, 4 ways, 64 byte lines, sectored
			CPUInfo._L3.bPresent = true;
			strcpy(CPUInfo._L3.strSize, "512 KB");
			CPUInfo._L3.uiAssociativeWays = 4;
			CPUInfo._L3.uiLineSize = 64;
			CPUInfo._L3.bSectored = true;
			break;
		case 0x23:		// cfg = 0x23:  code and data L3 cache present, 1024 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L3.bPresent = true;
			strcpy(CPUInfo._L3.strSize, "1024 KB");
			CPUInfo._L3.uiAssociativeWays = 8;
			CPUInfo._L3.uiLineSize = 64;
			CPUInfo._L3.bSectored = true;
			break;
		case 0x25:		// cfg = 0x25:  code and data L3 cache present, 2048 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L3.bPresent = true;
			strcpy(CPUInfo._L3.strSize, "2048 KB");
			CPUInfo._L3.uiAssociativeWays = 8;
			CPUInfo._L3.uiLineSize = 64;
			CPUInfo._L3.bSectored = true;
			break;
		case 0x29:		// cfg = 0x29:  code and data L3 cache present, 4096 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L3.bPresent = true;
			strcpy(CPUInfo._L3.strSize, "4096 KB");
			CPUInfo._L3.uiAssociativeWays = 8;
			CPUInfo._L3.uiLineSize = 64;
			CPUInfo._L3.bSectored = true;
			break;
		case 0x40:		// cfg = 0x40:  no integrated L2 cache (P6 core) or L3 cache (P4 core)
			break;
		case 0x41:		// cfg = 0x41:  code and data L2 cache present, 128 KB, 4 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "128 KB");
			CPUInfo._L2.uiAssociativeWays = 4;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x42:		// cfg = 0x42:  code and data L2 cache present, 256 KB, 4 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "256 KB");
			CPUInfo._L2.uiAssociativeWays = 4;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x43:		// cfg = 0x43:  code and data L2 cache present, 512 KB, 4 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "512 KB");
			CPUInfo._L2.uiAssociativeWays = 4;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x44:		// cfg = 0x44:  code and data L2 cache present, 1024 KB, 4 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "1 MB");
			CPUInfo._L2.uiAssociativeWays = 4;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x45:		// cfg = 0x45:  code and data L2 cache present, 2048 KB, 4 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "2 MB");
			CPUInfo._L2.uiAssociativeWays = 4;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x50:		// cfg = 0x50:  code TLB present, 4 KB / 4 MB / 2 MB pages, fully associative, 64 entries
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 KB / 2 MB / 4 MB");
			CPUInfo._Instruction.uiAssociativeWays = (unsigned int) -1;
			CPUInfo._Instruction.uiEntries = 64;
			break;
		case 0x51:		// cfg = 0x51:  code TLB present, 4 KB / 4 MB / 2 MB pages, fully associative, 128 entries
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 KB / 2 MB / 4 MB");
			CPUInfo._Instruction.uiAssociativeWays = (unsigned int) -1;
			CPUInfo._Instruction.uiEntries = 128;
			break;
		case 0x52:		// cfg = 0x52:  code TLB present, 4 KB / 4 MB / 2 MB pages, fully associative, 256 entries
			CPUInfo._Instruction.bPresent = true;
			strcpy(CPUInfo._Instruction.strPageSize, "4 KB / 2 MB / 4 MB");
			CPUInfo._Instruction.uiAssociativeWays = (unsigned int) -1;
			CPUInfo._Instruction.uiEntries = 256;
			break;
		case 0x5B:		// cfg = 0x5B:  data TLB present, 4 KB / 4 MB pages, fully associative, 64 entries
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 KB / 4 MB");
			CPUInfo._Data.uiAssociativeWays = (unsigned int) -1;
			CPUInfo._Data.uiEntries = 64;
			break;
		case 0x5C:		// cfg = 0x5C:  data TLB present, 4 KB / 4 MB pages, fully associative, 128 entries
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 KB / 4 MB");
			CPUInfo._Data.uiAssociativeWays = (unsigned int) -1;
			CPUInfo._Data.uiEntries = 128;
			break;
		case 0x5d:		// cfg = 0x5D:  data TLB present, 4 KB / 4 MB pages, fully associative, 256 entries
			CPUInfo._Data.bPresent = true;
			strcpy(CPUInfo._Data.strPageSize, "4 KB / 4 MB");
			CPUInfo._Data.uiAssociativeWays = (unsigned int) -1;
			CPUInfo._Data.uiEntries = 256;
			break;
		case 0x66:		// cfg = 0x66:  data L1 cache present, 8 KB, 4 ways, 64 byte lines, sectored
			CPUInfo._L1.Data.bPresent = true;
			strcpy(CPUInfo._L1.Data.strSize, "8 KB");
			CPUInfo._L1.Data.uiAssociativeWays = 4;
			CPUInfo._L1.Data.uiLineSize = 64;
			break;
		case 0x67:		// cfg = 0x67:  data L1 cache present, 16 KB, 4 ways, 64 byte lines, sectored
			CPUInfo._L1.Data.bPresent = true;
			strcpy(CPUInfo._L1.Data.strSize, "16 KB");
			CPUInfo._L1.Data.uiAssociativeWays = 4;
			CPUInfo._L1.Data.uiLineSize = 64;
			break;
		case 0x68:		// cfg = 0x68:  data L1 cache present, 32 KB, 4 ways, 64 byte lines, sectored
			CPUInfo._L1.Data.bPresent = true;
			strcpy(CPUInfo._L1.Data.strSize, "32 KB");
			CPUInfo._L1.Data.uiAssociativeWays = 4;
			CPUInfo._L1.Data.uiLineSize = 64;
			break;
		case 0x70:		// cfg = 0x70:  trace L1 cache present, 12 KµOPs, 4 ways
			CPUInfo._Trace.bPresent = true;
			strcpy(CPUInfo._Trace.strSize, "12 K-micro-ops");
			CPUInfo._Trace.uiAssociativeWays = 4;
			break;
		case 0x71:		// cfg = 0x71:  trace L1 cache present, 16 KµOPs, 4 ways
			CPUInfo._Trace.bPresent = true;
			strcpy(CPUInfo._Trace.strSize, "16 K-micro-ops");
			CPUInfo._Trace.uiAssociativeWays = 4;
			break;
		case 0x72:		// cfg = 0x72:  trace L1 cache present, 32 KµOPs, 4 ways
			CPUInfo._Trace.bPresent = true;
			strcpy(CPUInfo._Trace.strSize, "32 K-micro-ops");
			CPUInfo._Trace.uiAssociativeWays = 4;
			break;
		case 0x79:		// cfg = 0x79:  code and data L2 cache present, 128 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "128 KB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 64;
			CPUInfo._L2.bSectored = true;
			break;
		case 0x7A:		// cfg = 0x7A:  code and data L2 cache present, 256 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "256 KB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 64;
			CPUInfo._L2.bSectored = true;
			break;
		case 0x7B:		// cfg = 0x7B:  code and data L2 cache present, 512 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "512 KB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 64;
			CPUInfo._L2.bSectored = true;
			break;
		case 0x7C:		// cfg = 0x7C:  code and data L2 cache present, 1024 KB, 8 ways, 64 byte lines, sectored
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "1 MB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 64;
			CPUInfo._L2.bSectored = true;
			break;
		case 0x81:		// cfg = 0x81:  code and data L2 cache present, 128 KB, 8 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "128 KB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x82:		// cfg = 0x82:  code and data L2 cache present, 256 KB, 8 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "256 KB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x83:		// cfg = 0x83:  code and data L2 cache present, 512 KB, 8 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "512 KB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x84:		// cfg = 0x84:  code and data L2 cache present, 1024 KB, 8 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "1 MB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 32;
			break;
		case 0x85:		// cfg = 0x85:  code and data L2 cache present, 2048 KB, 8 ways, 32 byte lines
			CPUInfo._L2.bPresent = true;
			strcpy(CPUInfo._L2.strSize, "2 MB");
			CPUInfo._L2.uiAssociativeWays = 8;
			CPUInfo._L2.uiLineSize = 32;
			break;
	}
}

__forceinline static char *TranslateAssociativeWays(unsigned int uiWays, char *buf)
{
	// We define 0xFFFFFFFF (= -1) as fully associative
    if (uiWays == ((unsigned int) -1))
		strcpy(buf, "fully associative");
	else
	{
		if (uiWays == 1)			// A one way associative cache is just direct mapped
			strcpy(buf, "direct mapped");
		else if (uiWays == 0)		// This should not happen...
			strcpy(buf, "unknown associative ways");
		else						// The x-way associative cache
			sprintf(buf, "%d ways associative", uiWays);
	}
	// To ease the function use we return the buffer
	return buf;
}
__forceinline static void TranslateTLB(ProcessorTLB *tlb)
{
	char buf[64];

	// We just check if the TLB is present
	if (tlb->bPresent)
        sprintf(tlb->strTLB, "%s page size, %s, %d entries", tlb->strPageSize, TranslateAssociativeWays(tlb->uiAssociativeWays, buf), tlb->uiEntries);
	else
        strcpy(tlb->strTLB, "Not present");
}
__forceinline static void TranslateCache(ProcessorCache *cache)
{
	char buf[64];

	// We just check if the cache is present
    if (cache->bPresent)
	{
		// If present we construct the string
		sprintf(cache->strCache, "%s cache size, %s, %d bytes line size", cache->strSize, TranslateAssociativeWays(cache->uiAssociativeWays, buf), cache->uiLineSize);
		if (cache->bSectored)
			strcat(cache->strCache, ", sectored");
	}
	else
	{
		// Else we just say "Not present"
		strcpy(cache->strCache, "Not present");
	}
}

// void CProcessor::TranslateProcessorConfiguration()
// ==================================================
// Private class function to translate the processor configuration values
// to strings
/////////////////////////////////////////////////////////////////////////
void CProcessor::TranslateProcessorConfiguration()
{
	// We just call the small functions defined above
	TranslateTLB(&CPUInfo._Data);
	TranslateTLB(&CPUInfo._Instruction);

	TranslateCache(&CPUInfo._Trace);

	TranslateCache(&CPUInfo._L1.Instruction);
	TranslateCache(&CPUInfo._L1.Data);
	TranslateCache(&CPUInfo._L2);
	TranslateCache(&CPUInfo._L3);
}

// void CProcessor::GetStandardProcessorConfiguration()
// ====================================================
// Private class function to read the standard processor configuration
//////////////////////////////////////////////////////////////////////
void CProcessor::GetStandardProcessorConfiguration()
{
	unsigned long eaxreg, ebxreg, ecxreg, edxreg;

	// We check if the CPUID function is available
	if (!CheckCPUIDPresence())
		return;

	// First we check if the processor supports the standard
	// CPUID level 0x00000002
	if (CPUInfo.MaxSupportedLevel >= 2)
	{
		// Now we go read the std. CPUID level 0x00000002 the first time
		unsigned long count, num = 255;
		for (count = 0; count < num; count++)
		{
			__asm
			{
				mov eax, 2
				cpuid
				mov eaxreg, eax
				mov ebxreg, ebx
				mov ecxreg, ecx
				mov edxreg, edx
			}
			// We have to repeat this reading for 'num' times
			num = eaxreg & 0xFF;

			// Then we call the big decode switch function
			DecodeProcessorConfiguration(eaxreg >> 8);
			DecodeProcessorConfiguration(eaxreg >> 16);
			DecodeProcessorConfiguration(eaxreg >> 24);

			// If ebx contains additional data we also decode it
			if ((ebxreg & 0x80000000) == 0)
			{
				DecodeProcessorConfiguration(ebxreg);
				DecodeProcessorConfiguration(ebxreg >> 8);
				DecodeProcessorConfiguration(ebxreg >> 16);
				DecodeProcessorConfiguration(ebxreg >> 24);
			}
			// And also the ecx register
			if ((ecxreg & 0x80000000) == 0)
			{
				DecodeProcessorConfiguration(ecxreg);
				DecodeProcessorConfiguration(ecxreg >> 8);
				DecodeProcessorConfiguration(ecxreg >> 16);
				DecodeProcessorConfiguration(ecxreg >> 24);
			}
			// At last the edx processor register
			if ((edxreg & 0x80000000) == 0)
			{
				DecodeProcessorConfiguration(edxreg);
				DecodeProcessorConfiguration(edxreg >> 8);
				DecodeProcessorConfiguration(edxreg >> 16);
				DecodeProcessorConfiguration(edxreg >> 24);
			}
		}
	}
}

// void CProcessor::GetStandardProcessorExtensions()
// =================================================
// Private class function to read the standard processor extensions
///////////////////////////////////////////////////////////////////
void CProcessor::GetStandardProcessorExtensions()
{
	unsigned long ebxreg, edxreg;

	// We check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return;
	// We just get the standard CPUID level 0x00000001 which should be
	// available on every x86 processor
	__asm
	{
		mov eax, 1
		cpuid
		mov ebxreg, ebx
		mov edxreg, edx
	}
    
	// Then we mask some bits
	CPUInfo._Ext.FPU_FloatingPointUnit							= CheckBit(edxreg, 0);
	CPUInfo._Ext.VME_Virtual8086ModeEnhancements				= CheckBit(edxreg, 1);
	CPUInfo._Ext.DE_DebuggingExtensions							= CheckBit(edxreg, 2);
	CPUInfo._Ext.PSE_PageSizeExtensions							= CheckBit(edxreg, 3);
	CPUInfo._Ext.TSC_TimeStampCounter							= CheckBit(edxreg, 4);
	CPUInfo._Ext.MSR_ModelSpecificRegisters						= CheckBit(edxreg, 5);
	CPUInfo._Ext.PAE_PhysicalAddressExtension					= CheckBit(edxreg, 6);
	CPUInfo._Ext.MCE_MachineCheckException						= CheckBit(edxreg, 7);
	CPUInfo._Ext.CX8_COMPXCHG8B_Instruction						= CheckBit(edxreg, 8);
	CPUInfo._Ext.APIC_AdvancedProgrammableInterruptController	= CheckBit(edxreg, 9);
	CPUInfo._Ext.APIC_ID = (ebxreg >> 24) & 0xFF;
	CPUInfo._Ext.SEP_FastSystemCall								= CheckBit(edxreg, 11);
	CPUInfo._Ext.MTRR_MemoryTypeRangeRegisters					= CheckBit(edxreg, 12);
	CPUInfo._Ext.PGE_PTE_GlobalFlag								= CheckBit(edxreg, 13);
	CPUInfo._Ext.MCA_MachineCheckArchitecture					= CheckBit(edxreg, 14);
	CPUInfo._Ext.CMOV_ConditionalMoveAndCompareInstructions		= CheckBit(edxreg, 15);
	CPUInfo._Ext.FGPAT_PageAttributeTable						= CheckBit(edxreg, 16);
	CPUInfo._Ext.PSE36_36bitPageSizeExtension					= CheckBit(edxreg, 17);
	CPUInfo._Ext.PN_ProcessorSerialNumber						= CheckBit(edxreg, 18);
	CPUInfo._Ext.CLFSH_CFLUSH_Instruction						= CheckBit(edxreg, 19);
	CPUInfo._Ext.CLFLUSH_InstructionCacheLineSize = (ebxreg >> 8) & 0xFF;
	CPUInfo._Ext.DS_DebugStore									= CheckBit(edxreg, 21);
	CPUInfo._Ext.ACPI_ThermalMonitorAndClockControl				= CheckBit(edxreg, 22);
	CPUInfo._Ext.MMX_MultimediaExtensions						= CheckBit(edxreg, 23);
	CPUInfo._Ext.FXSR_FastStreamingSIMD_ExtensionsSaveRestore	= CheckBit(edxreg, 24);
	CPUInfo._Ext.SSE_StreamingSIMD_Extensions					= CheckBit(edxreg, 25);
	CPUInfo._Ext.SSE2_StreamingSIMD2_Extensions					= CheckBit(edxreg, 26);
	CPUInfo._Ext.SS_SelfSnoop									= CheckBit(edxreg, 27);
	CPUInfo._Ext.HT_HyperThreading								= CheckBit(edxreg, 28);
	CPUInfo._Ext.HT_HyterThreadingSiblings = (ebxreg >> 16) & 0xFF;
	CPUInfo._Ext.TM_ThermalMonitor								= CheckBit(edxreg, 29);
	CPUInfo._Ext.IA64_Intel64BitArchitecture					= CheckBit(edxreg, 30);
}

// const ProcessorInfo *CProcessor::GetCPUInfo()
// =============================================
// Calls all the other detection function to create an detailed
// processor information
///////////////////////////////////////////////////////////////
const ProcessorInfo *CProcessor::GetCPUInfo()
{
	unsigned long eaxreg, ebxreg, ecxreg, edxreg;
 
	// First of all we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return NULL;

	// We read the standard CPUID level 0x00000000 which should
	// be available on every x86 processor
	__asm
	{
		mov eax, 0
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
		mov edxreg, edx
		mov ecxreg, ecx
	}
	// Then we connect the single register values to the vendor string
	*((unsigned long *) CPUInfo.strVendor) = ebxreg;
	*((unsigned long *) (CPUInfo.strVendor+4)) = edxreg;
	*((unsigned long *) (CPUInfo.strVendor+8)) = ecxreg;

	// We can also read the max. supported standard CPUID level
	CPUInfo.MaxSupportedLevel = eaxreg & 0xFFFF;

	// Then we read the ext. CPUID level 0x80000000
	__asm
	{
        mov eax, 0x80000000
		cpuid
		mov eaxreg, eax
	}
	// ...to check the max. supportted extended CPUID level
	CPUInfo.MaxSupportedExtendedLevel = eaxreg;

	// Then we switch to the specific processor vendors
	switch (ebxreg)
	{
		case 0x756E6547:	// GenuineIntel
			AnalyzeIntelProcessor();
			break;
		case 0x68747541:	// AuthenticAMD
			AnalyzeAMDProcessor();
			break;
		case 0x69727943:	// CyrixInstead
			// I really do not know anyone owning such a piece of crab
			// So we analyze it as an unknown processor *ggggg*
		default:
			AnalyzeUnknownProcessor();
			break;
	}

	// After all we return the class CPUInfo member var
	return (&CPUInfo);
}

// bool CProcessor::CPUInfoToText(char *strBuffer, unsigned int uiMaxLen)
// ======================================================================
// Gets the frequency and processor information and writes it to a string
/////////////////////////////////////////////////////////////////////////
bool CProcessor::CPUInfoToText(char *strBuffer, unsigned int uiMaxLen)
{
#define LENCHECK                len = (unsigned int) strlen(buf); if (len >= uiMaxLen) return false; strcpy(strBuffer, buf); strBuffer += len;
#define COPYADD(str)            strcpy(buf, str); LENCHECK;
#define FORMATADD(format, var)  sprintf(buf, format, var); LENCHECK;
#define BOOLADD(str, boolvar)   COPYADD(str); if (boolvar) { COPYADD(" Yes\n"); } else { COPYADD(" No\n"); }

	char buf[1024];
	unsigned int len;

	// First we have to get the frequency
    GetCPUFrequency(50);

	// Then we get the processor information
	GetCPUInfo();

    // Now we construct the string (see the macros at function beginning)
	strBuffer[0] = 0;

	COPYADD("// CPU General Information\n//////////////////////////\n");
	FORMATADD("Processor name:   %s\n", strCPUName);
	FORMATADD("Frequency:        %.2f MHz\n\n", (float) uqwFrequency / 1000000.0f);
	FORMATADD("Vendor:           %s\n", CPUInfo.strVendor);
	FORMATADD("Family:           %s\n", CPUInfo.strFamily);
	FORMATADD("Extended family:  %d\n", CPUInfo.uiExtendedFamily);
	FORMATADD("Model:            %s\n", CPUInfo.strModel);
	FORMATADD("Extended model:   %d\n", CPUInfo.uiExtendedModel);
	FORMATADD("Type:             %s\n", CPUInfo.strType);
	FORMATADD("Brand ID:         %s\n", CPUInfo.strBrandID);
	if (CPUInfo._Ext.PN_ProcessorSerialNumber)
	{
		FORMATADD("Processor Serial: %s\n", CPUInfo.strProcessorSerial);
	}
	else
	{
	  COPYADD("Processor Serial: Disabled\n");
	}

	COPYADD("\n\n// CPU Configuration\n////////////////////\n");
	FORMATADD("L1 instruction cache:           %s\n", CPUInfo._L1.Instruction.strCache);
	FORMATADD("L1 data cache:                  %s\n", CPUInfo._L1.Data.strCache);
	FORMATADD("L2 cache:                       %s\n", CPUInfo._L2.strCache);
	FORMATADD("L3 cache:                       %s\n", CPUInfo._L3.strCache);
	FORMATADD("Trace cache:                    %s\n", CPUInfo._Trace.strCache);
	FORMATADD("Instruction TLB:                %s\n", CPUInfo._Instruction.strTLB);
	FORMATADD("Data TLB:                       %s\n", CPUInfo._Data.strTLB);
	FORMATADD("Max Supported CPUID-Level:      0x%08lX\n", CPUInfo.MaxSupportedLevel);
	FORMATADD("Max Supported Ext. CPUID-Level: 0x%08lX\n", CPUInfo.MaxSupportedExtendedLevel);

	COPYADD("\n\n// CPU Extensions\n/////////////////\n");
	BOOLADD("AA64   AMD 64-bit Architecture:                    ", CPUInfo._Ext.AA64_AMD64BitArchitecture);
	BOOLADD("ACPI   Thermal Monitor And Clock Control:          ", CPUInfo._Ext.ACPI_ThermalMonitorAndClockControl);
	BOOLADD("APIC   Advanced Programmable Interrupt Controller: ", CPUInfo._Ext.APIC_AdvancedProgrammableInterruptController);
	FORMATADD("       APIC-ID:                                     %d\n", CPUInfo._Ext.APIC_ID);
	BOOLADD("CLFSH  CLFLUSH Instruction Presence:               ", CPUInfo._Ext.CLFSH_CFLUSH_Instruction);
	FORMATADD("       CLFLUSH Instruction Cache Line Size:         %d\n", CPUInfo._Ext.CLFLUSH_InstructionCacheLineSize);
	BOOLADD("CMOV   Conditional Move And Compare Instructions:  ", CPUInfo._Ext.CMOV_ConditionalMoveAndCompareInstructions);
	BOOLADD("CX8    COMPXCHG8B Instruction:                     ", CPUInfo._Ext.CX8_COMPXCHG8B_Instruction);
	BOOLADD("DE     Debugging Extensions:                       ", CPUInfo._Ext.DE_DebuggingExtensions);
	BOOLADD("DS     Debug Store:                                ", CPUInfo._Ext.DS_DebugStore);
	BOOLADD("FGPAT  Page Attribute Table:                       ", CPUInfo._Ext.FGPAT_PageAttributeTable);
	BOOLADD("FPU    Floating Point Unit:                        ", CPUInfo._Ext.FPU_FloatingPointUnit);
	BOOLADD("FXSR   Fast Streaming SIMD Extensions Save/Restore:", CPUInfo._Ext.FXSR_FastStreamingSIMD_ExtensionsSaveRestore);
	BOOLADD("HT     Hyper Threading:                            ", CPUInfo._Ext.HT_HyperThreading);
	BOOLADD("IA64   Intel 64-Bit Architecture:                  ", CPUInfo._Ext.IA64_Intel64BitArchitecture);
	BOOLADD("MCA    Machine Check Architecture:                 ", CPUInfo._Ext.MCA_MachineCheckArchitecture);
	BOOLADD("MCE    Machine Check Exception:                    ", CPUInfo._Ext.MCE_MachineCheckException);
	BOOLADD("MMX    Multimedia Extensions:                      ", CPUInfo._Ext.MMX_MultimediaExtensions);
	BOOLADD("MMX+   Multimedia Extensions:                      ", CPUInfo._Ext.EMMX_MultimediaExtensions);
	BOOLADD("MSR    Model Specific Registers:                   ", CPUInfo._Ext.MSR_ModelSpecificRegisters);
	BOOLADD("MTRR   Memory Type Range Registers:                ", CPUInfo._Ext.MTRR_MemoryTypeRangeRegisters);
	BOOLADD("PAE    Physical Address Extension:                 ", CPUInfo._Ext.PAE_PhysicalAddressExtension);
	BOOLADD("PGE    PTE Global Flag:                            ", CPUInfo._Ext.PGE_PTE_GlobalFlag);
	if (CPUInfo._Ext.PN_ProcessorSerialNumber)
	{
		FORMATADD("PN     Processor Serial Number:                     %s\n", CPUInfo.strProcessorSerial);
	}
	else
	{
		COPYADD("PN     Processor Serial Number:                     Disables\n");
	}
	BOOLADD("PSE    Page Size Extensions:                       ", CPUInfo._Ext.PSE_PageSizeExtensions);
	BOOLADD("PSE36  36-bit Page Size Extension:                 ", CPUInfo._Ext.PSE36_36bitPageSizeExtension);
	BOOLADD("SEP    Fast System Call:                           ", CPUInfo._Ext.SEP_FastSystemCall);
	BOOLADD("SS     Self Snoop:                                 ", CPUInfo._Ext.SS_SelfSnoop);
	BOOLADD("SSE    Streaming SIMD Extensions:                  ", CPUInfo._Ext.SSE_StreamingSIMD_Extensions);
	BOOLADD("SSE2   Streaming SIMD 2 Extensions:                ", CPUInfo._Ext.SSE2_StreamingSIMD2_Extensions);
	BOOLADD("TM     Thermal Monitor:                            ", CPUInfo._Ext.TM_ThermalMonitor);
	BOOLADD("TSC    Time Stamp Counter:                         ", CPUInfo._Ext.TSC_TimeStampCounter);
	BOOLADD("VME    Virtual 8086 Mode Enhancements:             ", CPUInfo._Ext.VME_Virtual8086ModeEnhancements);
	BOOLADD("3DNow! Instructions:                               ", CPUInfo._Ext._3DNOW_InstructionExtensions);
	BOOLADD("Enhanced 3DNow! Instructions:                      ", CPUInfo._Ext._E3DNOW_InstructionExtensions);

	// Yippie!!!
	return true;
}

// bool CProcessor::WriteInfoTextFile(const char *strFilename)
// ===========================================================
// Takes use of CProcessor::CPUInfoToText and saves the string to a
// file
///////////////////////////////////////////////////////////////////
bool CProcessor::WriteInfoTextFile(const char *strFilename)
{
	char buf[16384];

	// First we get the string
	if (!CPUInfoToText(buf, 16383))
		return false;

	// Then we create a new file (CREATE_ALWAYS)
	FILE *file = fopen(strFilename, "w");
	if (!file)
		return false;

	// After that we write the string to the file
	unsigned long dwBytesToWrite, dwBytesWritten;
	dwBytesToWrite = (unsigned long) strlen(buf);
	dwBytesWritten = (unsigned long) fwrite(buf, 1, dwBytesToWrite, file);
	fclose(file);
	if (dwBytesToWrite != dwBytesWritten)
		return false;

	// Done
	return true;
} 