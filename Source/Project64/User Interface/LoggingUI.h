/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/
#pragma once

typedef struct
{
    bool GenerateLog;

    /* Registers Log */
    bool LogRDRamRegisters;
    bool LogSPRegisters;
    bool LogDPCRegisters;
    bool LogDPSRegisters;
    bool LogMIPSInterface;
    bool LogVideoInterface;
    bool LogAudioInterface;
    bool LogPerInterface;
    bool LogRDRAMInterface;
    bool LogSerialInterface;

    /* Pif Ram Log */
    bool LogPRDMAOperations;
    bool LogPRDirectMemLoads;
    bool LogPRDMAMemLoads;
    bool LogPRDirectMemStores;
    bool LogPRDMAMemStores;
    bool LogControllerPak;

    /* Special Log */
    bool LogCP0changes;
    bool LogCP0reads;
    bool LogTLB;
    bool LogExceptions;
    bool NoInterrupts;
    bool LogCache;
    bool LogRomHeader;
    bool LogUnknown;
} LOG_OPTIONS;

extern LOG_OPTIONS g_LogOptions;

void EnterLogOptions ( HWND hwndOwner );
void StartLog       ( void );
void StopLog        ( void );
void LoadLogOptions ( LOG_OPTIONS * LogOptions, bool AlwaysFill );
void Log_LW         ( uint32_t PC, uint32_t VAddr );
void Log_SW         ( uint32_t PC, uint32_t VAddr, uint32_t Value );
void LogMessage     ( const char * Message, ... );
