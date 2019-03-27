/*
 * RSP Compiler plug in for Project64 (A Nintendo 64 emulator).
 *
 * (c) Copyright 2001 jabo (jabo@emulation64.com) and
 * zilmar (zilmar@emulation64.com)
 *
 * pj64 homepage: www.pj64.net
 * 
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#include <stdio.h>
#include <windows.h>
#include <Common/StdString.h>
#include <Common/FileClass.h>
#include <Common/LogClass.h>
#include <Common/path.h>

extern "C" {
#include "Log.h"
#include "Rsp.h"
#include "Rsp Registers.h"
}

CLog * RDPLog = NULL;
CLog * CPULog = NULL;

void StartCPULog ( void )
{
	if (CPULog == NULL)
	{
		CPath LogFile(CPath::MODULE_DIRECTORY,"RSP_x86Log.txt");
		LogFile.AppendDirectory("Logs");

		CPULog = new CLog;
		CPULog->Open(LogFile);
	}
}

void StopCPULog  ( void )
{
	if (CPULog != NULL)
	{
		delete CPULog;
		CPULog = NULL;
	}
}

void CPU_Message ( const char * Message, ...)
{
	if (CPULog == NULL) 
	{
		return;
	}

	stdstr Msg;

	va_list args;
	va_start(args, Message);
	Msg.ArgFormat(Message,args);
	va_end(args);

	Msg += "\r\n";
	
	CPULog->Log(Msg.c_str());
}

void StartRDPLog ( void )
{
	if (RDPLog == NULL)
	{
		CPath LogFile(CPath::MODULE_DIRECTORY,"RDP_Log.txt");
		LogFile.AppendDirectory("Logs");

		RDPLog = new CLog;
		RDPLog->Open(LogFile);
		RDPLog->SetMaxFileSize(400 * 1024 * 1024);
//		RDPLog->SetFlush(true);
	}
}

void StopRDPLog  ( void )
{
	if (RDPLog != NULL)
	{
		delete RDPLog;
		RDPLog = NULL;
	}
}

void RDP_Message ( const char * Message, ...)
{
	if (RDPLog == NULL) 
	{
		return;
	}

	stdstr Msg;

	va_list args;
	va_start(args, Message);
	Msg.ArgFormat(Message,args);
	va_end(args);

	Msg += "\r\n";

	RDPLog->Log(Msg.c_str());
}

void RDP_LogMT0 ( DWORD PC, int Reg, DWORD Value )
{
	if (RDPLog == NULL) 
	{
		return;
	}
	switch (Reg) {
	case 0: RDP_Message("%03X: Stored 0x%08X into SP_MEM_ADDR_REG",PC,Value); break;
	case 1: RDP_Message("%03X: Stored 0x%08X into SP_DRAM_ADDR_REG",PC,Value); break;
	case 2: RDP_Message("%03X: Stored 0x%08X into SP_RD_LEN_REG",PC,Value); break;
	case 3: RDP_Message("%03X: Stored 0x%08X into SP_WR_LEN_REG",PC,Value); break;
	case 4: RDP_Message("%03X: Stored 0x%08X into SP_STATUS_REG",PC,Value); break;
	case 5: RDP_Message("%03X: Stored 0x%08X into Reg 5 ???",PC,Value); break;
	case 6: RDP_Message("%03X: Stored 0x%08X into Reg 6 ???",PC,Value); break;
	case 7: RDP_Message("%03X: Stored 0x%08X into SP_SEMAPHORE_REG",PC,Value); break;
	case 8: RDP_Message("%03X: Stored 0x%08X into DPC_START_REG",PC,Value); break;
	case 9: RDP_Message("%03X: Stored 0x%08X into DPC_END_REG",PC,Value); break;
	case 10: RDP_Message("%03X: Stored 0x%08X into DPC_CURRENT_REG",PC,Value); break;
	case 11: RDP_Message("%03X: Stored 0x%08X into DPC_STATUS_REG",PC,Value); break;
	case 12: RDP_Message("%03X: Stored 0x%08X into DPC_CLOCK_REG",PC,Value); break;
	}
}

void RDP_LogMF0   ( DWORD PC, int Reg )
{
	switch (Reg) {
	case 8: RDP_Message("%03X: Read 0x%08X from DPC_START_REG",PC,*RSPInfo.DPC_START_REG); break;
	case 9: RDP_Message("%03X: Read 0x%08X from DPC_END_REG",PC,*RSPInfo.DPC_END_REG); break;
	case 10: RDP_Message("%03X: Read 0x%08X from DPC_CURRENT_REG",PC,*RSPInfo.DPC_CURRENT_REG); break;
	case 11: RDP_Message("%03X: Read 0x%08X from DPC_STATUS_REG",PC,*RSPInfo.DPC_STATUS_REG); break;
	case 12: RDP_Message("%03X: Read 0x%08X from DPC_CLOCK_REG",PC,*RSPInfo.DPC_CLOCK_REG); break;
	}
}


void RDP_LogDlist ( void )
{
	if (RDPLog == NULL) 
	{
		return;
	}
	DWORD Length = *RSPInfo.DPC_END_REG - *RSPInfo.DPC_CURRENT_REG;
	RDP_Message("    Dlist length = %d bytes",Length);
	
	DWORD Pos = *RSPInfo.DPC_CURRENT_REG;
	while (Pos < *RSPInfo.DPC_END_REG)
	{
 		char Hex[100], Ascii[30];
		DWORD count;

		memset(&Hex,0,sizeof(Hex));
		memset(&Ascii,0,sizeof(Ascii)); 

		BYTE * Mem = RSPInfo.DMEM;
		if ((*RSPInfo.DPC_STATUS_REG & DPC_STATUS_XBUS_DMEM_DMA) == 0)
		{
			Mem = RSPInfo.RDRAM;
		}

		for (count = 0; count < 0x10; count ++, Pos++ ) {
			char tmp[3];
			if ((count % 4) != 0 || count == 0) {
				sprintf(tmp,"%02X",Mem[Pos]);
				strcat(Hex," ");
				strcat(Hex,tmp);
			} else {
				sprintf(tmp,"%02X",Mem[Pos]);
				strcat(Hex," - ");
				strcat(Hex,tmp);
			}

			
			if (Mem[Pos] < 30 || Mem[Pos] > 127) {
				strcat(Ascii,".");
			} else {
				sprintf(tmp,"%c",Mem[Pos]);
				strcat(Ascii,tmp);
			}
		}
		RDP_Message("   %s %s",Hex, Ascii);
	}
}

void RDP_LogLoc   ( DWORD /*PC*/ )
{
//	RDP_Message("%03X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X",PC, RSP_GPR[26].UW, *(DWORD *)&RSPInfo.IMEM[0xDBC], 
//		RSP_Flags[0].UW, RSP_Vect[0].UW[0],RSP_Vect[0].UW[1],RSP_Vect[0].UW[2],RSP_Vect[0].UW[3],
//		RSP_Vect[28].UW[0],RSP_Vect[28].UW[1],RSP_Vect[28].UW[2],RSP_Vect[28].UW[3],RSP_Vect[31].UW[0]);
}


#ifdef old

#include <windows.h>
#include "RSP Registers.h"
#include "log.h"

#ifdef Log_x86Code
static HANDLE hCPULogFile = NULL;
#endif 

#ifdef GenerateLog
static HANDLE hLogFile = NULL;
#endif

#ifdef Log_x86Code
void CPU_Message (char * Message, ...) {
	DWORD dwWritten;
	char Msg[400];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	
	strcat(Msg,"\r\n");
	WriteFile( hCPULogFile,Msg,strlen(Msg),&dwWritten,NULL );
}
#endif

#ifdef GenerateLog
void Log_Message (char * Message, ...) {
	DWORD dwWritten;
	char Msg[400];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	
	strcat(Msg,"\r\n");

	WriteFile( hLogFile,Msg,strlen(Msg),&dwWritten,NULL );
}
#endif 

#ifdef Log_x86Code
void Start_x86_Log (void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char File[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,_MAX_PATH);
	_splitpath(path_buffer, drive, dir, NULL, NULL);
	
	sprintf(File, "%s%s\\RSPx86Log.log", drive, dir);

	hCPULogFile = CreateFile(File,GENERIC_WRITE, FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hCPULogFile,0,NULL,FILE_BEGIN);
}
#endif

#ifdef GenerateLog
void Log_MT_CP0 ( unsigned int PC, int CP0Reg, int Value ) {
	switch (CP0Reg) {
	//case 0: Log_Message("%03X: Stored 0x%08X in SP_MEM_ADDR_REG",PC,Value); break;
	//case 1: Log_Message("%03X: Stored 0x%08X in SP_DRAM_ADDR_REG",PC,Value); break;
	//case 2: Log_Message("%03X: Stored 0x%08X in SP_RD_LEN_REG",PC,Value); break;
	case 3: 
		//Log_Message("%03X: Stored 0x%08X in SP_WR_LEN_REG",PC,Value); 
		Log_Message("Instruction: %08X%08X",RSP_GPR[25].UW,RSP_GPR[24].UW);
		//Log_Message("");
		break;
	/*case 4: Log_Message("%03X: Stored 0x%08X in SP_STATUS_REG",PC,Value); break;
	case 5: Log_Message("%03X: Stored 0x%08X in SP_DMA_FULL_REG",PC,Value); break;
	case 6: Log_Message("%03X: Stored 0x%08X in SP_DMA_BUSY_REG",PC,Value); break;
	case 7: Log_Message("%03X: Stored 0x%08X in SP_SEMAPHORE_REG",PC,Value); break;
	case 8: Log_Message("%03X: Stored 0x%08X in DPC_START_REG",PC,Value); break;
	case 9: Log_Message("%03X: Stored 0x%08X in DPC_END_REG",PC,Value); break;
	case 10: Log_Message("%03X: Stored 0x%08X in DPC_CURRENT_REG",PC,Value); break;
	case 11: Log_Message("%03X: Stored 0x%08X in DPC_STATUS_REG",PC,Value); break;
	case 12: Log_Message("%03X: Stored 0x%08X in DPC_CLOCK_REG",PC,Value); break;
	case 13: Log_Message("%03X: Stored 0x%08X in DPC_BUFBUSY_REG",PC,Value); break;
	case 14: Log_Message("%03X: Stored 0x%08X in DPC_PIPEBUSY_REG",PC,Value); break;
	case 15: Log_Message("%03X: Stored 0x%08X in DPC_TMEM_REG",PC,Value); break;
	default:
		Log_Message("%03X: Unkown RSP CP0 register %d",PC,CP0Reg);
		break;*/
	}
}

void Start_Log (void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char File[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,_MAX_PATH);
	_splitpath(path_buffer, drive, dir, NULL, NULL);
	
	sprintf(File, "%s%s\\RSP.log", drive, dir);
	
	hLogFile = CreateFile(File,GENERIC_WRITE, FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SetFilePointer(hLogFile,0,NULL,FILE_BEGIN);
}

void Stop_Log (void) {
	if (hLogFile) {
		CloseHandle(hLogFile);
		hLogFile = NULL;
	}
}
#endif

#ifdef Log_x86Code
void Stop_x86_Log (void) {
	if (hCPULogFile) {
		CloseHandle(hCPULogFile);
		hCPULogFile = NULL;
	}
}
#endif

#endif
