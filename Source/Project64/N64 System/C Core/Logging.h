/****************************************************************************
*                                                                           *
* Project 64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	BOOL		GenerateLog;

	/* Registers Log */
	BOOL	LogRDRamRegisters;
	BOOL	LogSPRegisters;
	BOOL	LogDPCRegisters;
	BOOL	LogDPSRegisters;
	BOOL	LogMIPSInterface;
	BOOL	LogVideoInterface;
	BOOL	LogAudioInterface;
	BOOL	LogPerInterface;
	BOOL	LogRDRAMInterface;
	BOOL	LogSerialInterface;

	/* Pif Ram Log */
  	BOOL	LogPRDMAOperations;
	BOOL	LogPRDirectMemLoads;  	
	BOOL	LogPRDMAMemLoads;  	
	BOOL	LogPRDirectMemStores;
	BOOL	LogPRDMAMemStores;
	BOOL	LogControllerPak;

	/* Special Log */
	BOOL	LogCP0changes;
	BOOL	LogCP0reads;
	BOOL	LogTLB;
	BOOL	LogExceptions;
	BOOL	NoInterrupts;
	BOOL	LogCache;
	BOOL	LogRomHeader;
	BOOL	LogUnknown;
} LOG_OPTIONS;

extern LOG_OPTIONS LogOptions;

void EnterLogOptions ( HWND hwndOwner );
void LoadLogOptions  ( LOG_OPTIONS * LogOptions, BOOL AlwaysFill );
void Log_LW          ( DWORD PC, DWORD VAddr );
void __cdecl LogMessage      ( char * Message, ... );
void Log_SW          ( DWORD PC, DWORD VAddr, DWORD Value );
void StartLog        ( void );
void StopLog         ( void );

#ifdef __cplusplus
}
#endif
