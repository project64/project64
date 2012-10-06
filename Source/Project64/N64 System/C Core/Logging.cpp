#include "stdafx.h"

#if (!defined(EXTERNAL_RELEASE))

void LoadLogSetting (HKEY hKey,char * String, BOOL * Value);
void SaveLogOptions (void);

LRESULT CALLBACK LogGeneralProc ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK LogPifProc     ( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK LogRegProc     ( HWND, UINT, WPARAM, LPARAM );

LOG_OPTIONS LogOptions,TempOptions;
HANDLE hLogFile = NULL;

void EnterLogOptions(HWND hwndOwner) {
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = GetModuleHandle(NULL);
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_Logging_Registers);
    psp[0].pfnDlgProc = (DLGPROC)LogRegProc;
    psp[0].pszTitle = "Registers";
    psp[0].lParam = 0;
    psp[0].pfnCallback = NULL;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = GetModuleHandle(NULL);
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_Logging_PifRam);
    psp[1].pfnDlgProc = (DLGPROC)LogPifProc;
    psp[1].pszTitle = "Pif Ram";
    psp[1].lParam = 0;
    psp[1].pfnCallback = NULL;

    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = GetModuleHandle(NULL);
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_Logging_General);
    psp[2].pfnDlgProc = (DLGPROC)LogGeneralProc;
    psp[2].pszTitle = "General";
    psp[2].lParam = 0;
    psp[2].pfnCallback = NULL;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = GetModuleHandle(NULL);
    psh.pszCaption = (LPSTR) "Log Options";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

    LoadLogOptions(&TempOptions,TRUE);
	PropertySheet(&psh);
    SaveLogOptions();
	LoadLogOptions(&LogOptions, FALSE);
	return;
}

void LoadLogOptions (LOG_OPTIONS * LogOptions, BOOL AlwaysFill) {
	long lResult;
	HKEY hKeyResults = 0;
	char String[200];

	sprintf(String,"Software\\N64 Emulation\\%s\\Logging",_Settings->LoadString(Setting_ApplicationName));
	lResult = RegOpenKeyEx( HKEY_CURRENT_USER,String,0,KEY_ALL_ACCESS,
		&hKeyResults);
	
	if (lResult == ERROR_SUCCESS) {	
		//LoadLogSetting(hKeyResults,"Generate Log File",&LogOptions->GenerateLog);
		if (LogOptions->GenerateLog || AlwaysFill) {
			LoadLogSetting(hKeyResults,"Log RDRAM",&LogOptions->LogRDRamRegisters);
			LoadLogSetting(hKeyResults,"Log SP",&LogOptions->LogSPRegisters);
			LoadLogSetting(hKeyResults,"Log DP Command",&LogOptions->LogDPCRegisters);
			LoadLogSetting(hKeyResults,"Log DP Span",&LogOptions->LogDPSRegisters);
			LoadLogSetting(hKeyResults,"Log MIPS Interface (MI)",&LogOptions->LogMIPSInterface);
			LoadLogSetting(hKeyResults,"Log Video Interface (VI)",&LogOptions->LogVideoInterface);
			LoadLogSetting(hKeyResults,"Log Audio Interface (AI)",&LogOptions->LogAudioInterface);
			LoadLogSetting(hKeyResults,"Log Peripheral Interface (PI)",&LogOptions->LogPerInterface);
			LoadLogSetting(hKeyResults,"Log RDRAM Interface (RI)",&LogOptions->LogRDRAMInterface);
			LoadLogSetting(hKeyResults,"Log Serial Interface (SI)",&LogOptions->LogSerialInterface);
			LoadLogSetting(hKeyResults,"Log PifRam DMA Operations",&LogOptions->LogPRDMAOperations);
			LoadLogSetting(hKeyResults,"Log PifRam Direct Memory Loads",&LogOptions->LogPRDirectMemLoads);
			LoadLogSetting(hKeyResults,"Log PifRam DMA Memory Loads",&LogOptions->LogPRDMAMemLoads);
			LoadLogSetting(hKeyResults,"Log PifRam Direct Memory Stores",&LogOptions->LogPRDirectMemStores);
			LoadLogSetting(hKeyResults,"Log PifRam DMA Memory Stores",&LogOptions->LogPRDMAMemStores);
			LoadLogSetting(hKeyResults,"Log Controller Pak",&LogOptions->LogControllerPak);
			LoadLogSetting(hKeyResults,"Log CP0 changes",&LogOptions->LogCP0changes);
			LoadLogSetting(hKeyResults,"Log CP0 reads",&LogOptions->LogCP0reads);
			LoadLogSetting(hKeyResults,"Log Exceptions",&LogOptions->LogExceptions);
			LoadLogSetting(hKeyResults,"No Interrupts",&LogOptions->NoInterrupts);
			LoadLogSetting(hKeyResults,"Log TLB",&LogOptions->LogTLB);
			LoadLogSetting(hKeyResults,"Log Cache Operations",&LogOptions->LogCache);
			LoadLogSetting(hKeyResults,"Log Rom Header",&LogOptions->LogRomHeader);
			LoadLogSetting(hKeyResults,"Log Unknown access",&LogOptions->LogUnknown);
			return;
		}
	}

	LogOptions->GenerateLog          = FALSE;
	LogOptions->LogRDRamRegisters    = FALSE;
	LogOptions->LogSPRegisters       = FALSE;
	LogOptions->LogDPCRegisters      = FALSE;
	LogOptions->LogDPSRegisters      = FALSE;
	LogOptions->LogMIPSInterface     = FALSE;
	LogOptions->LogVideoInterface    = FALSE;
	LogOptions->LogAudioInterface    = FALSE;
	LogOptions->LogPerInterface      = FALSE;
	LogOptions->LogRDRAMInterface    = FALSE;
	LogOptions->LogSerialInterface   = FALSE;
	
	LogOptions->LogPRDMAOperations   = FALSE;
	LogOptions->LogPRDirectMemLoads  = FALSE;
	LogOptions->LogPRDMAMemLoads     = FALSE;
	LogOptions->LogPRDirectMemStores = FALSE;
	LogOptions->LogPRDMAMemStores    = FALSE;
	LogOptions->LogControllerPak     = FALSE;
		
	LogOptions->LogCP0changes        = FALSE;
	LogOptions->LogCP0reads          = FALSE;
	LogOptions->LogCache             = FALSE;
	LogOptions->LogExceptions        = FALSE;
	LogOptions->NoInterrupts         = FALSE;
	LogOptions->LogTLB               = FALSE;
	LogOptions->LogRomHeader         = FALSE;
	LogOptions->LogUnknown           = FALSE;
}

void LoadLogSetting (HKEY hKey,char * String, BOOL * Value) {
	DWORD Type, dwResult, Bytes = 4;
	long lResult;

	lResult = RegQueryValueEx(hKey,String,0,&Type,(LPBYTE)(&dwResult),&Bytes);		
	if (Type == REG_DWORD && lResult == ERROR_SUCCESS) { 
		*Value = (BOOL)dwResult;
	} else {
		*Value = FALSE;
	}
}

LRESULT CALLBACK LogGeneralProc (HWND hDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		if (TempOptions.LogCP0changes) { CheckDlgButton(hDlg,IDC_CP0_WRITE,BST_CHECKED); }
		if (TempOptions.LogCP0reads)   { CheckDlgButton(hDlg,IDC_CP0_READ,BST_CHECKED); }
		if (TempOptions.LogCache)      { CheckDlgButton(hDlg,IDC_CACHE,BST_CHECKED); }
		if (TempOptions.LogExceptions) { CheckDlgButton(hDlg,IDC_EXCEPTIONS,BST_CHECKED); }
		if (TempOptions.NoInterrupts)  { CheckDlgButton(hDlg,IDC_INTERRUPTS,BST_CHECKED); }
		if (TempOptions.LogTLB)        { CheckDlgButton(hDlg,IDC_TLB,BST_CHECKED); }
		if (TempOptions.LogRomHeader)  { CheckDlgButton(hDlg,IDC_ROM_HEADER,BST_CHECKED); }
		if (TempOptions.LogUnknown)    { CheckDlgButton(hDlg,IDC_UNKOWN,BST_CHECKED); }
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code != PSN_APPLY) { break; }
		TempOptions.LogCP0changes = IsDlgButtonChecked(hDlg,IDC_CP0_WRITE) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogCP0reads   = IsDlgButtonChecked(hDlg,IDC_CP0_READ) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogCache      = IsDlgButtonChecked(hDlg,IDC_CACHE) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogExceptions = IsDlgButtonChecked(hDlg,IDC_EXCEPTIONS) == BST_CHECKED?TRUE:FALSE;
		TempOptions.NoInterrupts  = IsDlgButtonChecked(hDlg,IDC_INTERRUPTS) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogTLB        = IsDlgButtonChecked(hDlg,IDC_TLB) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogRomHeader  = IsDlgButtonChecked(hDlg,IDC_ROM_HEADER) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogUnknown    = IsDlgButtonChecked(hDlg,IDC_UNKOWN) == BST_CHECKED?TRUE:FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void Log_LW (DWORD PC, DWORD VAddr) {
	if (!LogOptions.GenerateLog) { return; }

	if ( VAddr < 0xA0000000 || VAddr >= 0xC0000000 )
	{
		DWORD PAddr;
		if (!_TransVaddr->TranslateVaddr(VAddr,PAddr))
		{
			if (LogOptions.LogUnknown) 
			{ 
				LogMessage("%08X: read from unknown ??? (%08X)",PC,VAddr);
			}
			return; 
		}
		VAddr = PAddr + 0xA0000000;
	} 
	
	DWORD Value;
	if ( VAddr >= 0xA0000000 && VAddr < (0xA0000000 + _MMU->RdramSize())) { return; }
	if ( VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024) {
		if (!LogOptions.LogRDRamRegisters) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA3F00000: LogMessage("%08X: read from RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG (%08X)",PC, Value); return;
		case 0xA3F00004: LogMessage("%08X: read from RDRAM_DEVICE_ID_REG (%08X)",PC, Value); return;
		case 0xA3F00008: LogMessage("%08X: read from RDRAM_DELAY_REG (%08X)",PC, Value); return;
		case 0xA3F0000C: LogMessage("%08X: read from RDRAM_MODE_REG (%08X)",PC, Value); return;
		case 0xA3F00010: LogMessage("%08X: read from RDRAM_REF_INTERVAL_REG (%08X)",PC, Value); return;
		case 0xA3F00014: LogMessage("%08X: read from RDRAM_REF_ROW_REG (%08X)",PC, Value); return;
		case 0xA3F00018: LogMessage("%08X: read from RDRAM_RAS_INTERVAL_REG (%08X)",PC, Value); return;
		case 0xA3F0001C: LogMessage("%08X: read from RDRAM_MIN_INTERVAL_REG (%08X)",PC, Value); return;
		case 0xA3F00020: LogMessage("%08X: read from RDRAM_ADDR_SELECT_REG (%08X)",PC, Value); return;
		case 0xA3F00024: LogMessage("%08X: read from RDRAM_DEVICE_MANUF_REG (%08X)",PC, Value); return;
		}
	}

	if ( VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC ) { return; }
	if ( VAddr >= 0xA4040000 && VAddr <= 0xA404001C ) {
		if (!LogOptions.LogSPRegisters) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4040000: LogMessage("%08X: read from SP_MEM_ADDR_REG (%08X)",PC, Value); break;
		case 0xA4040004: LogMessage("%08X: read from SP_DRAM_ADDR_REG (%08X)",PC, Value); break;
		case 0xA4040008: LogMessage("%08X: read from SP_RD_LEN_REG (%08X)",PC, Value); break;
		case 0xA404000C: LogMessage("%08X: read from SP_WR_LEN_REG (%08X)",PC, Value); break;
		case 0xA4040010: LogMessage("%08X: read from SP_STATUS_REG (%08X)",PC, Value); break;
		case 0xA4040014: LogMessage("%08X: read from SP_DMA_FULL_REG (%08X)",PC, Value); break;
		case 0xA4040018: LogMessage("%08X: read from SP_DMA_BUSY_REG (%08X)",PC, Value); break;
		case 0xA404001C: LogMessage("%08X: read from SP_SEMAPHORE_REG (%08X)",PC, Value); break;
		}
		return;
	}		
	if ( VAddr == 0xA4080000) {
		if (!LogOptions.LogSPRegisters) { return; }
		_MMU->LW_VAddr(VAddr,Value);
		LogMessage("%08X: read from SP_PC (%08X)",PC, Value);
		return;
	}
	if (VAddr >= 0xA4100000 && VAddr <= 0xA410001C) {
		if (!LogOptions.LogDPCRegisters) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4100000: LogMessage("%08X: read from DPC_START_REG (%08X)",PC, Value); return;
		case 0xA4100004: LogMessage("%08X: read from DPC_END_REG (%08X)",PC, Value); return;
		case 0xA4100008: LogMessage("%08X: read from DPC_CURRENT_REG (%08X)",PC, Value); return;
		case 0xA410000C: LogMessage("%08X: read from DPC_STATUS_REG (%08X)",PC, Value); return;
		case 0xA4100010: LogMessage("%08X: read from DPC_CLOCK_REG (%08X)",PC, Value); return;
		case 0xA4100014: LogMessage("%08X: read from DPC_BUFBUSY_REG (%08X)",PC, Value); return;
		case 0xA4100018: LogMessage("%08X: read from DPC_PIPEBUSY_REG (%08X)",PC, Value); return;
		case 0xA410001C: LogMessage("%08X: read from DPC_TMEM_REG (%08X)",PC, Value); return;
		}
	}
	if (VAddr >= 0xA4300000 && VAddr <= 0xA430000C) {
		if (!LogOptions.LogMIPSInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4300000: LogMessage("%08X: read from MI_INIT_MODE_REG/MI_MODE_REG (%08X)",PC, Value); return;
		case 0xA4300004: LogMessage("%08X: read from MI_VERSION_REG/MI_NOOP_REG (%08X)",PC, Value); return;
		case 0xA4300008: LogMessage("%08X: read from MI_INTR_REG (%08X)",PC, Value); return;
		case 0xA430000C: LogMessage("%08X: read from MI_INTR_MASK_REG (%08X)",PC, Value); return;
		}
	}
	if (VAddr >= 0xA4400000 && VAddr <= 0xA4400034) {
		if (!LogOptions.LogVideoInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4400000: LogMessage("%08X: read from VI_STATUS_REG/VI_CONTROL_REG (%08X)",PC, Value); return;
		case 0xA4400004: LogMessage("%08X: read from VI_ORIGIN_REG/VI_DRAM_ADDR_REG (%08X)",PC, Value); return;
		case 0xA4400008: LogMessage("%08X: read from VI_WIDTH_REG/VI_H_WIDTH_REG (%08X)",PC, Value); return;
		case 0xA440000C: LogMessage("%08X: read from VI_INTR_REG/VI_V_INTR_REG (%08X)",PC, Value); return;
		case 0xA4400010: LogMessage("%08X: read from VI_CURRENT_REG/VI_V_CURRENT_LINE_REG (%08X)",PC, Value); return;
		case 0xA4400014: LogMessage("%08X: read from VI_BURST_REG/VI_TIMING_REG (%08X)",PC, Value); return;
		case 0xA4400018: LogMessage("%08X: read from VI_V_SYNC_REG (%08X)",PC, Value); return;
		case 0xA440001C: LogMessage("%08X: read from VI_H_SYNC_REG (%08X)",PC, Value); return;
		case 0xA4400020: LogMessage("%08X: read from VI_LEAP_REG/VI_H_SYNC_LEAP_REG (%08X)",PC, Value); return;
		case 0xA4400024: LogMessage("%08X: read from VI_H_START_REG/VI_H_VIDEO_REG (%08X)",PC, Value); return;
		case 0xA4400028: LogMessage("%08X: read from VI_V_START_REG/VI_V_VIDEO_REG (%08X)",PC, Value); return;
		case 0xA440002C: LogMessage("%08X: read from VI_V_BURST_REG (%08X)",PC, Value); return;
		case 0xA4400030: LogMessage("%08X: read from VI_X_SCALE_REG (%08X)",PC, Value); return;
		case 0xA4400034: LogMessage("%08X: read from VI_Y_SCALE_REG (%08X)",PC, Value); return;
		}
	}
	if (VAddr >= 0xA4500000 && VAddr <= 0xA4500014) {
		if (!LogOptions.LogAudioInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4500000: LogMessage("%08X: read from AI_DRAM_ADDR_REG (%08X)",PC, Value); return;
		case 0xA4500004: LogMessage("%08X: read from AI_LEN_REG (%08X)",PC, Value); return;
		case 0xA4500008: LogMessage("%08X: read from AI_CONTROL_REG (%08X)",PC, Value); return;
		case 0xA450000C: LogMessage("%08X: read from AI_STATUS_REG (%08X)",PC, Value); return;
		case 0xA4500010: LogMessage("%08X: read from AI_DACRATE_REG (%08X)",PC, Value); return;
		case 0xA4500014: LogMessage("%08X: read from AI_BITRATE_REG (%08X)",PC, Value); return;
		}
	}
	if (VAddr >= 0xA4600000 && VAddr <= 0xA4600030) {
		if (!LogOptions.LogPerInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4600000: LogMessage("%08X: read from PI_DRAM_ADDR_REG (%08X)",PC, Value); return;
		case 0xA4600004: LogMessage("%08X: read from PI_CART_ADDR_REG (%08X)",PC, Value); return;
		case 0xA4600008: LogMessage("%08X: read from PI_RD_LEN_REG (%08X)",PC, Value); return;
		case 0xA460000C: LogMessage("%08X: read from PI_WR_LEN_REG (%08X)",PC, Value); return;
		case 0xA4600010: LogMessage("%08X: read from PI_STATUS_REG (%08X)",PC, Value); return;
		case 0xA4600014: LogMessage("%08X: read from PI_BSD_DOM1_LAT_REG/PI_DOMAIN1_REG (%08X)",PC, Value); return;
		case 0xA4600018: LogMessage("%08X: read from PI_BSD_DOM1_PWD_REG (%08X)",PC, Value); return;
		case 0xA460001C: LogMessage("%08X: read from PI_BSD_DOM1_PGS_REG (%08X)",PC, Value); return;
		case 0xA4600020: LogMessage("%08X: read from PI_BSD_DOM1_RLS_REG (%08X)",PC, Value); return;
		case 0xA4600024: LogMessage("%08X: read from PI_BSD_DOM2_LAT_REG/PI_DOMAIN2_REG (%08X)",PC, Value); return;
		case 0xA4600028: LogMessage("%08X: read from PI_BSD_DOM2_PWD_REG (%08X)",PC, Value); return;
		case 0xA460002C: LogMessage("%08X: read from PI_BSD_DOM2_PGS_REG (%08X)",PC, Value); return;
		case 0xA4600030: LogMessage("%08X: read from PI_BSD_DOM2_RLS_REG (%08X)",PC, Value); return;
		}
	}
	if (VAddr >= 0xA4700000 && VAddr <= 0xA470001C) {
		if (!LogOptions.LogRDRAMInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);

		switch (VAddr) {
		case 0xA4700000: LogMessage("%08X: read from RI_MODE_REG (%08X)",PC, Value); return;
		case 0xA4700004: LogMessage("%08X: read from RI_CONFIG_REG (%08X)",PC, Value); return;
		case 0xA4700008: LogMessage("%08X: read from RI_CURRENT_LOAD_REG (%08X)",PC, Value); return;
		case 0xA470000C: LogMessage("%08X: read from RI_SELECT_REG (%08X)",PC, Value); return;
		case 0xA4700010: LogMessage("%08X: read from RI_REFRESH_REG/RI_COUNT_REG (%08X)",PC, Value); return;
		case 0xA4700014: LogMessage("%08X: read from RI_LATENCY_REG (%08X)",PC, Value); return;
		case 0xA4700018: LogMessage("%08X: read from RI_RERROR_REG (%08X)",PC, Value); return;
		case 0xA470001C: LogMessage("%08X: read from RI_WERROR_REG (%08X)",PC, Value); return;
		}
	}
	if ( VAddr == 0xA4800000) {
		if (!LogOptions.LogSerialInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);
		LogMessage("%08X: read from SI_DRAM_ADDR_REG (%08X)",PC, Value);
		return;
	}
	if ( VAddr == 0xA4800004) {
		if (!LogOptions.LogSerialInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);
		LogMessage("%08X: read from SI_PIF_ADDR_RD64B_REG (%08X)",PC, Value);
		return;
	}
	if ( VAddr == 0xA4800010) {
		if (!LogOptions.LogSerialInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);
		LogMessage("%08X: read from SI_PIF_ADDR_WR64B_REG (%08X)",PC, Value);
		return;
	}
	if ( VAddr == 0xA4800018) {
		if (!LogOptions.LogSerialInterface) { return; }
		_MMU->LW_VAddr(VAddr,Value);
		LogMessage("%08X: read from SI_STATUS_REG (%08X)",PC, Value);
		return;
	}
	if ( VAddr >= 0xBFC00000 && VAddr <= 0xBFC007C0 ) { return; }
	if ( VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC ) {
		if (!LogOptions.LogPRDirectMemLoads) { return; }
		_MMU->LW_VAddr(VAddr,Value);
		LogMessage("%08X: read word from Pif Ram at 0x%X (%08X)",PC,VAddr - 0xBFC007C0, Value);
		return;
	}
	if ( VAddr >= 0xB0000040 && ((VAddr - 0xB0000000) < _Rom->GetRomSize())) { return; }
	if ( VAddr >= 0xB0000000 && VAddr < 0xB0000040) {
		if (!LogOptions.LogRomHeader) { return; }

		_MMU->LW_VAddr(VAddr,Value);
	    switch (VAddr) {        
		case 0xB0000004: LogMessage("%08X: read from Rom Clock Rate (%08X)",PC, Value); break;
		case 0xB0000008: LogMessage("%08X: read from Rom Boot address offset (%08X)",PC, Value); break;
		case 0xB000000C: LogMessage("%08X: read from Rom Release offset (%08X)",PC, Value); break;
		case 0xB0000010: LogMessage("%08X: read from Rom CRC1 (%08X)",PC, Value); break;
		case 0xB0000014: LogMessage("%08X: read from Rom CRC2 (%08X)",PC, Value); break;
		default: LogMessage("%08X: read from Rom header 0x%X (%08X)",PC, VAddr & 0xFF,Value);  break;
		}
		return;
	}
	if (!LogOptions.LogUnknown) { return; }
	LogMessage("%08X: read from unknown ??? (%08X)",PC,VAddr);
}

void __cdecl LogMessage (char * Message, ...) {
	DWORD dwWritten;
	char Msg[400];
	va_list ap;

	if(!_Settings->LoadBool(Debugger_Enabled)) { return; }
	if(hLogFile == NULL) { return; }

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	
	strcat(Msg,"\r\n");

	WriteFile( hLogFile,Msg,strlen(Msg),&dwWritten,NULL );
}

void Log_SW (DWORD PC, DWORD VAddr, DWORD Value) {
	if (!LogOptions.GenerateLog) { return; }

	if ( VAddr < 0xA0000000 || VAddr >= 0xC0000000 )
	{
		DWORD PAddr;
		if (!_TransVaddr->TranslateVaddr(VAddr,PAddr))
		{
			if (LogOptions.LogUnknown) 
			{ 
				LogMessage("%08X: Writing 0x%08X to %08X",PC, Value, VAddr );
			}
			return; 
		}
		VAddr = PAddr + 0xA0000000;
	} 

	if ( VAddr >= 0xA0000000 && VAddr < (0xA0000000 + _MMU->RdramSize())) { return; }
	if ( VAddr >= 0xA3F00000 && VAddr <= 0xA3F00024) {
		if (!LogOptions.LogRDRamRegisters) { return; }
		switch (VAddr) {
		case 0xA3F00000: LogMessage("%08X: Writing 0x%08X to RDRAM_CONFIG_REG/RDRAM_DEVICE_TYPE_REG",PC, Value ); return;
		case 0xA3F00004: LogMessage("%08X: Writing 0x%08X to RDRAM_DEVICE_ID_REG",PC, Value ); return;
		case 0xA3F00008: LogMessage("%08X: Writing 0x%08X to RDRAM_DELAY_REG",PC, Value ); return;
		case 0xA3F0000C: LogMessage("%08X: Writing 0x%08X to RDRAM_MODE_REG",PC, Value ); return;
		case 0xA3F00010: LogMessage("%08X: Writing 0x%08X to RDRAM_REF_INTERVAL_REG",PC, Value ); return;
		case 0xA3F00014: LogMessage("%08X: Writing 0x%08X to RDRAM_REF_ROW_REG",PC, Value ); return;
		case 0xA3F00018: LogMessage("%08X: Writing 0x%08X to RDRAM_RAS_INTERVAL_REG",PC, Value ); return;
		case 0xA3F0001C: LogMessage("%08X: Writing 0x%08X to RDRAM_MIN_INTERVAL_REG",PC, Value ); return;
		case 0xA3F00020: LogMessage("%08X: Writing 0x%08X to RDRAM_ADDR_SELECT_REG",PC, Value ); return;
		case 0xA3F00024: LogMessage("%08X: Writing 0x%08X to RDRAM_DEVICE_MANUF_REG",PC, Value ); return;
		}
	}
	if ( VAddr >= 0xA4000000 && VAddr <= 0xA4001FFC ) { return; }

    if ( VAddr >= 0xA4040000 && VAddr <= 0xA404001C) {
		if (!LogOptions.LogSPRegisters) { return; }
		switch (VAddr) {
		case 0xA4040000: LogMessage("%08X: Writing 0x%08X to SP_MEM_ADDR_REG",PC, Value ); return;
		case 0xA4040004: LogMessage("%08X: Writing 0x%08X to SP_DRAM_ADDR_REG",PC, Value ); return;
		case 0xA4040008: LogMessage("%08X: Writing 0x%08X to SP_RD_LEN_REG",PC, Value ); return;
		case 0xA404000C: LogMessage("%08X: Writing 0x%08X to SP_WR_LEN_REG",PC, Value ); return;
		case 0xA4040010: LogMessage("%08X: Writing 0x%08X to SP_STATUS_REG",PC, Value ); return;
		case 0xA4040014: LogMessage("%08X: Writing 0x%08X to SP_DMA_FULL_REG",PC, Value ); return;
		case 0xA4040018: LogMessage("%08X: Writing 0x%08X to SP_DMA_BUSY_REG",PC, Value ); return;
		case 0xA404001C: LogMessage("%08X: Writing 0x%08X to SP_SEMAPHORE_REG",PC, Value ); return;
		}
	}
    if ( VAddr == 0xA4080000) {
		if (!LogOptions.LogSPRegisters) { return; }		
		LogMessage("%08X: Writing 0x%08X to SP_PC",PC, Value ); return;
	}
	
    if ( VAddr >= 0xA4100000 && VAddr <= 0xA410001C) {
		if (!LogOptions.LogDPCRegisters) { return; }
		switch (VAddr) {
		case 0xA4100000: LogMessage("%08X: Writing 0x%08X to DPC_START_REG",PC, Value ); return;
		case 0xA4100004: LogMessage("%08X: Writing 0x%08X to DPC_END_REG",PC, Value ); return;
		case 0xA4100008: LogMessage("%08X: Writing 0x%08X to DPC_CURRENT_REG",PC, Value ); return;
		case 0xA410000C: LogMessage("%08X: Writing 0x%08X to DPC_STATUS_REG",PC, Value ); return;
		case 0xA4100010: LogMessage("%08X: Writing 0x%08X to DPC_CLOCK_REG",PC, Value ); return;
		case 0xA4100014: LogMessage("%08X: Writing 0x%08X to DPC_BUFBUSY_REG",PC, Value ); return;
		case 0xA4100018: LogMessage("%08X: Writing 0x%08X to DPC_PIPEBUSY_REG",PC, Value ); return;
		case 0xA410001C: LogMessage("%08X: Writing 0x%08X to DPC_TMEM_REG",PC, Value ); return;
		}
	}

    if ( VAddr >= 0xA4200000 && VAddr <= 0xA420000C) {
		if (!LogOptions.LogDPSRegisters) { return; }
		switch (VAddr) {
		case 0xA4200000: LogMessage("%08X: Writing 0x%08X to DPS_TBIST_REG",PC, Value ); return;
		case 0xA4200004: LogMessage("%08X: Writing 0x%08X to DPS_TEST_MODE_REG",PC, Value ); return;
		case 0xA4200008: LogMessage("%08X: Writing 0x%08X to DPS_BUFTEST_ADDR_REG",PC, Value ); return;
		case 0xA420000C: LogMessage("%08X: Writing 0x%08X to DPS_BUFTEST_DATA_REG",PC, Value ); return;
		}
	}

    if ( VAddr >= 0xA4300000 && VAddr <= 0xA430000C) {
		if (!LogOptions.LogMIPSInterface) { return; }
		switch (VAddr) {
		case 0xA4300000: LogMessage("%08X: Writing 0x%08X to MI_INIT_MODE_REG/MI_MODE_REG",PC, Value ); return;
		case 0xA4300004: LogMessage("%08X: Writing 0x%08X to MI_VERSION_REG/MI_NOOP_REG",PC, Value ); return;
		case 0xA4300008: LogMessage("%08X: Writing 0x%08X to MI_INTR_REG",PC, Value ); return;
		case 0xA430000C: LogMessage("%08X: Writing 0x%08X to MI_INTR_MASK_REG",PC, Value ); return;
		}
	}
    if ( VAddr >= 0xA4400000 && VAddr <= 0xA4400034) {
		if (!LogOptions.LogVideoInterface) { return; }
		switch (VAddr) {
		case 0xA4400000: LogMessage("%08X: Writing 0x%08X to VI_STATUS_REG/VI_CONTROL_REG",PC, Value ); return;
		case 0xA4400004: LogMessage("%08X: Writing 0x%08X to VI_ORIGIN_REG/VI_DRAM_ADDR_REG",PC, Value ); return;
		case 0xA4400008: LogMessage("%08X: Writing 0x%08X to VI_WIDTH_REG/VI_H_WIDTH_REG",PC, Value ); return;
		case 0xA440000C: LogMessage("%08X: Writing 0x%08X to VI_INTR_REG/VI_V_INTR_REG",PC, Value ); return;
		case 0xA4400010: LogMessage("%08X: Writing 0x%08X to VI_CURRENT_REG/VI_V_CURRENT_LINE_REG",PC, Value ); return;
		case 0xA4400014: LogMessage("%08X: Writing 0x%08X to VI_BURST_REG/VI_TIMING_REG",PC, Value ); return;
		case 0xA4400018: LogMessage("%08X: Writing 0x%08X to VI_V_SYNC_REG",PC, Value ); return;
		case 0xA440001C: LogMessage("%08X: Writing 0x%08X to VI_H_SYNC_REG",PC, Value ); return;
		case 0xA4400020: LogMessage("%08X: Writing 0x%08X to VI_LEAP_REG/VI_H_SYNC_LEAP_REG",PC, Value ); return;
		case 0xA4400024: LogMessage("%08X: Writing 0x%08X to VI_H_START_REG/VI_H_VIDEO_REG",PC, Value ); return;
		case 0xA4400028: LogMessage("%08X: Writing 0x%08X to VI_V_START_REG/VI_V_VIDEO_REG",PC, Value ); return;
		case 0xA440002C: LogMessage("%08X: Writing 0x%08X to VI_V_BURST_REG",PC, Value ); return;
		case 0xA4400030: LogMessage("%08X: Writing 0x%08X to VI_X_SCALE_REG",PC, Value ); return;
		case 0xA4400034: LogMessage("%08X: Writing 0x%08X to VI_Y_SCALE_REG",PC, Value ); return;
		}
	}

    if ( VAddr >= 0xA4500000 && VAddr <= 0xA4500014) {
		if (!LogOptions.LogAudioInterface) { return; }
		switch (VAddr) {
		case 0xA4500000: LogMessage("%08X: Writing 0x%08X to AI_DRAM_ADDR_REG",PC, Value ); return;
		case 0xA4500004: LogMessage("%08X: Writing 0x%08X to AI_LEN_REG",PC, Value ); return;
		case 0xA4500008: LogMessage("%08X: Writing 0x%08X to AI_CONTROL_REG",PC, Value ); return;
		case 0xA450000C: LogMessage("%08X: Writing 0x%08X to AI_STATUS_REG",PC, Value ); return;
		case 0xA4500010: LogMessage("%08X: Writing 0x%08X to AI_DACRATE_REG",PC, Value ); return;
		case 0xA4500014: LogMessage("%08X: Writing 0x%08X to AI_BITRATE_REG",PC, Value ); return;
		}
	}

    if ( VAddr >= 0xA4600000 && VAddr <= 0xA4600030) {
		if (!LogOptions.LogPerInterface) { return; }
		switch (VAddr) {
		case 0xA4600000: LogMessage("%08X: Writing 0x%08X to PI_DRAM_ADDR_REG",PC, Value ); return;
		case 0xA4600004: LogMessage("%08X: Writing 0x%08X to PI_CART_ADDR_REG",PC, Value ); return;
		case 0xA4600008: LogMessage("%08X: Writing 0x%08X to PI_RD_LEN_REG",PC, Value ); return;
		case 0xA460000C: LogMessage("%08X: Writing 0x%08X to PI_WR_LEN_REG",PC, Value ); return;
		case 0xA4600010: LogMessage("%08X: Writing 0x%08X to PI_STATUS_REG",PC, Value ); return;
		case 0xA4600014: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_LAT_REG/PI_DOMAIN1_REG",PC, Value ); return;
		case 0xA4600018: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_PWD_REG",PC, Value ); return;
		case 0xA460001C: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_PGS_REG",PC, Value ); return;
		case 0xA4600020: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM1_RLS_REG",PC, Value ); return;
		case 0xA4600024: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_LAT_REG/PI_DOMAIN2_REG",PC, Value ); return;
		case 0xA4600028: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_PWD_REG",PC, Value ); return;
		case 0xA460002C: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_PGS_REG",PC, Value ); return;
		case 0xA4600030: LogMessage("%08X: Writing 0x%08X to PI_BSD_DOM2_RLS_REG",PC, Value ); return;
		}
	}
    if ( VAddr >= 0xA4700000 && VAddr <= 0xA470001C) {
		if (!LogOptions.LogRDRAMInterface) { return; }
		switch (VAddr) {
		case 0xA4700000: LogMessage("%08X: Writing 0x%08X to RI_MODE_REG",PC, Value ); return;
		case 0xA4700004: LogMessage("%08X: Writing 0x%08X to RI_CONFIG_REG",PC, Value ); return;
		case 0xA4700008: LogMessage("%08X: Writing 0x%08X to RI_CURRENT_LOAD_REG",PC, Value ); return;
		case 0xA470000C: LogMessage("%08X: Writing 0x%08X to RI_SELECT_REG",PC, Value ); return;
		case 0xA4700010: LogMessage("%08X: Writing 0x%08X to RI_REFRESH_REG/RI_COUNT_REG",PC, Value ); return;
		case 0xA4700014: LogMessage("%08X: Writing 0x%08X to RI_LATENCY_REG",PC, Value ); return;
		case 0xA4700018: LogMessage("%08X: Writing 0x%08X to RI_RERROR_REG",PC, Value ); return;
		case 0xA470001C: LogMessage("%08X: Writing 0x%08X to RI_WERROR_REG",PC, Value ); return;
		}
	}
    if ( VAddr == 0xA4800000) {
		if (!LogOptions.LogSerialInterface) { return; }		
		LogMessage("%08X: Writing 0x%08X to SI_DRAM_ADDR_REG",PC, Value ); return;
	}
    if ( VAddr == 0xA4800004) {
		if (LogOptions.LogPRDMAOperations) {
			LogMessage("%08X: A DMA transfer from the PIF ram has occured",PC );			
		}
		if (!LogOptions.LogSerialInterface) { return; }		
		LogMessage("%08X: Writing 0x%08X to SI_PIF_ADDR_RD64B_REG",PC, Value ); return;
	}
    if ( VAddr == 0xA4800010) {
		if (LogOptions.LogPRDMAOperations) {
			LogMessage("%08X: A DMA transfer to the PIF ram has occured",PC );			
		}
		if (!LogOptions.LogSerialInterface) { return; }		
		LogMessage("%08X: Writing 0x%08X to SI_PIF_ADDR_WR64B_REG",PC, Value ); return;
	}
    if ( VAddr == 0xA4800018) {
		if (!LogOptions.LogSerialInterface) { return; }		
		LogMessage("%08X: Writing 0x%08X to SI_STATUS_REG",PC, Value ); return;
	}

	if ( VAddr >= 0xBFC007C0 && VAddr <= 0xBFC007FC ) {
		if (!LogOptions.LogPRDirectMemStores) { return; }
		LogMessage("%08X: Writing 0x%08X to Pif Ram at 0x%X",PC,Value, VAddr - 0xBFC007C0);
		return;
	}
	if (!LogOptions.LogUnknown) { return; }
	LogMessage("%08X: Writing 0x%08X to %08X ????",PC, Value, VAddr );
}

LRESULT CALLBACK LogPifProc (HWND hDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		if (TempOptions.LogPRDMAOperations)   { CheckDlgButton(hDlg,IDC_SI_DMA,BST_CHECKED); }
		if (TempOptions.LogPRDirectMemLoads)  { CheckDlgButton(hDlg,IDC_DIRECT_WRITE,BST_CHECKED); }
		if (TempOptions.LogPRDMAMemLoads)     { CheckDlgButton(hDlg,IDC_DMA_WRITE,BST_CHECKED); }
		if (TempOptions.LogPRDirectMemStores) { CheckDlgButton(hDlg,IDC_DIRECT_READ,BST_CHECKED); }
		if (TempOptions.LogPRDMAMemStores)    { CheckDlgButton(hDlg,IDC_DMA_READ,BST_CHECKED); }
		if (TempOptions.LogControllerPak)     { CheckDlgButton(hDlg,IDC_CONT_PAK,BST_CHECKED); }
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code != PSN_APPLY) { break; }		
		TempOptions.LogPRDMAOperations   = IsDlgButtonChecked(hDlg,IDC_SI_DMA) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogPRDirectMemLoads  = IsDlgButtonChecked(hDlg,IDC_DIRECT_WRITE) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogPRDMAMemLoads     = IsDlgButtonChecked(hDlg,IDC_DMA_WRITE) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogPRDirectMemStores = IsDlgButtonChecked(hDlg,IDC_DIRECT_READ) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogPRDMAMemStores    = IsDlgButtonChecked(hDlg,IDC_DMA_READ) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogControllerPak    = IsDlgButtonChecked(hDlg,IDC_CONT_PAK) == BST_CHECKED?TRUE:FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK LogRegProc (HWND hDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		if (TempOptions.LogRDRamRegisters)  { CheckDlgButton(hDlg,IDC_RDRAM,BST_CHECKED); }
		if (TempOptions.LogSPRegisters)     { CheckDlgButton(hDlg,IDC_SP_REG,BST_CHECKED); }
		if (TempOptions.LogDPCRegisters)    { CheckDlgButton(hDlg,IDC_DPC_REG,BST_CHECKED); }
		if (TempOptions.LogDPSRegisters)    { CheckDlgButton(hDlg,IDC_DPS_REG,BST_CHECKED); }
		if (TempOptions.LogMIPSInterface)   { CheckDlgButton(hDlg,IDC_MI_REG,BST_CHECKED); }
		if (TempOptions.LogVideoInterface)  { CheckDlgButton(hDlg,IDC_VI_REG,BST_CHECKED); }
		if (TempOptions.LogAudioInterface)  { CheckDlgButton(hDlg,IDC_AI_REG,BST_CHECKED); }
		if (TempOptions.LogPerInterface)    { CheckDlgButton(hDlg,IDC_PI_REG,BST_CHECKED); }
		if (TempOptions.LogRDRAMInterface)  { CheckDlgButton(hDlg,IDC_RI_REG,BST_CHECKED); }
		if (TempOptions.LogSerialInterface) { CheckDlgButton(hDlg,IDC_SI_REG,BST_CHECKED); }
		break;
	case WM_NOTIFY:
		if (((NMHDR FAR *) lParam)->code != PSN_APPLY) { break; }
		TempOptions.LogRDRamRegisters  = IsDlgButtonChecked(hDlg,IDC_RDRAM) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogSPRegisters     = IsDlgButtonChecked(hDlg,IDC_SP_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogDPCRegisters    = IsDlgButtonChecked(hDlg,IDC_DPC_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogDPSRegisters    = IsDlgButtonChecked(hDlg,IDC_DPS_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogMIPSInterface   = IsDlgButtonChecked(hDlg,IDC_MI_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogVideoInterface  = IsDlgButtonChecked(hDlg,IDC_VI_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogAudioInterface  = IsDlgButtonChecked(hDlg,IDC_AI_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogPerInterface    = IsDlgButtonChecked(hDlg,IDC_PI_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogRDRAMInterface  = IsDlgButtonChecked(hDlg,IDC_RI_REG) == BST_CHECKED?TRUE:FALSE;
		TempOptions.LogSerialInterface = IsDlgButtonChecked(hDlg,IDC_SI_REG) == BST_CHECKED?TRUE:FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void SaveLogSetting (HKEY hKey,char * String, BOOL Value) {
	DWORD StoreValue = Value;
	RegSetValueEx(hKey,String,0,REG_DWORD,(CONST BYTE *)&StoreValue,sizeof(DWORD));
}

void SaveLogOptions (void) {	
	long lResult;
	HKEY hKeyResults = 0;
	DWORD Disposition = 0;
	char String[200];
	
	sprintf(String,"Software\\N64 Emulation\\%s\\Logging",_Settings->LoadString(Setting_ApplicationName));
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER,String,0,"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL,&hKeyResults,&Disposition);
	
	SaveLogSetting(hKeyResults,"Log RDRAM",TempOptions.LogRDRamRegisters);
	SaveLogSetting(hKeyResults,"Log SP",TempOptions.LogSPRegisters);
	SaveLogSetting(hKeyResults,"Log DP Command",TempOptions.LogDPCRegisters);
	SaveLogSetting(hKeyResults,"Log DP Span",TempOptions.LogDPSRegisters);
	SaveLogSetting(hKeyResults,"Log MIPS Interface (MI)",TempOptions.LogMIPSInterface);
	SaveLogSetting(hKeyResults,"Log Video Interface (VI)",TempOptions.LogVideoInterface);
	SaveLogSetting(hKeyResults,"Log Audio Interface (AI)",TempOptions.LogAudioInterface);
	SaveLogSetting(hKeyResults,"Log Peripheral Interface (PI)",TempOptions.LogPerInterface);
	SaveLogSetting(hKeyResults,"Log RDRAM Interface (RI)",TempOptions.LogRDRAMInterface);
	SaveLogSetting(hKeyResults,"Log Serial Interface (SI)",TempOptions.LogSerialInterface);
	SaveLogSetting(hKeyResults,"Log PifRam DMA Operations",TempOptions.LogPRDMAOperations);
	SaveLogSetting(hKeyResults,"Log PifRam Direct Memory Loads",TempOptions.LogPRDirectMemLoads);
	SaveLogSetting(hKeyResults,"Log PifRam DMA Memory Loads",TempOptions.LogPRDMAMemLoads);
	SaveLogSetting(hKeyResults,"Log PifRam Direct Memory Stores",TempOptions.LogPRDirectMemStores);
	SaveLogSetting(hKeyResults,"Log PifRam DMA Memory Stores",TempOptions.LogPRDMAMemStores);
	SaveLogSetting(hKeyResults,"Log Controller Pak",TempOptions.LogControllerPak);
	SaveLogSetting(hKeyResults,"Log CP0 changes",TempOptions.LogCP0changes);
	SaveLogSetting(hKeyResults,"Log CP0 reads",TempOptions.LogCP0reads);
	SaveLogSetting(hKeyResults,"Log Exceptions",TempOptions.LogExceptions);
	SaveLogSetting(hKeyResults,"No Interrupts",TempOptions.NoInterrupts);
	SaveLogSetting(hKeyResults,"Log TLB",TempOptions.LogTLB);
	SaveLogSetting(hKeyResults,"Log Cache Operations",TempOptions.LogCache);
	SaveLogSetting(hKeyResults,"Log Rom Header",TempOptions.LogRomHeader);
	SaveLogSetting(hKeyResults,"Log Unknown access",TempOptions.LogUnknown);
	
	RegCloseKey(hKeyResults);
}

void StartLog (void) 
{
	if (!LogOptions.GenerateLog) { 
		StopLog();
		return; 
	}
	if (hLogFile) { return; }

	CPath LogFile(CPath::MODULE_DIRECTORY);
	LogFile.AppendDirectory(_T("Logs"));
	LogFile.SetNameExtension(_T("cpudebug.log"));
		
	hLogFile = CreateFile(LogFile,GENERIC_WRITE, FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);
}

void StopLog (void) {
	if (hLogFile) {
		CloseHandle(hLogFile);
	}
	hLogFile = NULL;
}
#endif