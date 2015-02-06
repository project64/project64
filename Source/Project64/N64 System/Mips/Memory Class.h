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

interface CMipsMemory_CallBack 
{
	//Protected memory has been written to, returns true if that memory has been unprotected
	virtual bool WriteToProtectedMemory (DWORD Address, int length) = 0;
};

class CMipsMemory
{
public:
	virtual BYTE * Rdram        ( void ) = 0;
	virtual DWORD  RdramSize    ( void ) = 0;
	virtual BYTE * Dmem         ( void ) = 0;
	virtual BYTE * Imem         ( void ) = 0;
	virtual BYTE * PifRam       ( void ) = 0;
	
	virtual BOOL  LB_VAddr     ( DWORD VAddr, BYTE & Value ) = 0;
	virtual BOOL  LH_VAddr     ( DWORD VAddr, WORD & Value ) = 0; 
	virtual BOOL  LW_VAddr     ( DWORD VAddr, DWORD & Value ) = 0;
	virtual BOOL  LD_VAddr     ( DWORD VAddr, QWORD & Value ) = 0;

	virtual BOOL  LB_PAddr     ( DWORD PAddr, BYTE & Value ) = 0;
	virtual BOOL  LH_PAddr     ( DWORD PAddr, WORD & Value ) = 0; 
	virtual BOOL  LW_PAddr     ( DWORD PAddr, DWORD & Value ) = 0;
	virtual BOOL  LD_PAddr     ( DWORD PAddr, QWORD & Value ) = 0;

	virtual BOOL  SB_VAddr     ( DWORD VAddr, BYTE Value ) = 0;
	virtual BOOL  SH_VAddr     ( DWORD VAddr, WORD Value ) = 0;
	virtual BOOL  SW_VAddr     ( DWORD VAddr, DWORD Value ) = 0;
	virtual BOOL  SD_VAddr     ( DWORD VAddr, QWORD Value ) = 0;

	virtual BOOL  SB_PAddr     ( DWORD PAddr, BYTE Value ) = 0;
	virtual BOOL  SH_PAddr     ( DWORD PAddr, WORD Value ) = 0;
	virtual BOOL  SW_PAddr     ( DWORD PAddr, DWORD Value ) = 0;
	virtual BOOL  SD_PAddr     ( DWORD PAddr, QWORD Value ) = 0;

	virtual bool  ValidVaddr   ( DWORD VAddr ) const = 0;

	virtual int   MemoryFilter ( DWORD dwExptCode, void * lpExceptionPointer ) = 0;
	virtual void  UpdateFieldSerration ( unsigned int interlaced ) = 0;

	//Protect the Memory from being written to
	virtual void  ProtectMemory    ( DWORD StartVaddr, DWORD EndVaddr ) = 0;
	virtual void  UnProtectMemory  ( DWORD StartVaddr, DWORD EndVaddr ) = 0;

	//Compilation Functions
	virtual void ResetMemoryStack    ( void ) = 0;
	
	virtual void Compile_LB          ( void ) = 0;
	virtual void Compile_LBU         ( void ) = 0;
	virtual void Compile_LH          ( void ) = 0;
	virtual void Compile_LHU         ( void ) = 0;
	virtual void Compile_LW          ( void ) = 0;
	virtual void Compile_LL          ( void ) = 0;
	virtual void Compile_LWC1        ( void ) = 0;
	virtual void Compile_LWU         ( void ) = 0;
	virtual void Compile_LWL         ( void ) = 0;
	virtual void Compile_LWR         ( void ) = 0;
	virtual void Compile_LD          ( void ) = 0;
	virtual void Compile_LDC1        ( void ) = 0;
	virtual void Compile_LDL         ( void ) = 0;
	virtual void Compile_LDR         ( void ) = 0;
	virtual void Compile_SB          ( void ) = 0;
	virtual void Compile_SH          ( void ) = 0;
	virtual void Compile_SW          ( void ) = 0;
	virtual void Compile_SWL         ( void ) = 0;
	virtual void Compile_SWR         ( void ) = 0;
	virtual void Compile_SD          ( void ) = 0;
	virtual void Compile_SDL         ( void ) = 0;
	virtual void Compile_SDR         ( void ) = 0;
	virtual void Compile_SC          ( void ) = 0;
	virtual void Compile_SWC1        ( void ) = 0;
	virtual void Compile_SDC1        ( void ) = 0;
};
