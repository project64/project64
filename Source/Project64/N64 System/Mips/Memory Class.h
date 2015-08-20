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

#include <objbase.h>

interface CMipsMemory_CallBack 
{
	//Protected memory has been written to, returns true if that memory has been unprotected
	virtual bool WriteToProtectedMemory (DWORD Address, int length) = 0;
};

class CMipsMemory
{
public:
	virtual BYTE * Rdram    () = 0;
	virtual DWORD  RdramSize() = 0;
	virtual BYTE * Dmem     () = 0;
	virtual BYTE * Imem     () = 0;
	virtual BYTE * PifRam   () = 0;
	
	virtual bool  LB_VAddr     ( DWORD VAddr, BYTE & Value ) = 0;
	virtual bool  LH_VAddr     ( DWORD VAddr, WORD & Value ) = 0; 
	virtual bool  LW_VAddr     ( DWORD VAddr, DWORD & Value ) = 0;
	virtual bool  LD_VAddr     ( DWORD VAddr, QWORD & Value ) = 0;

	virtual bool  LB_PAddr     ( DWORD PAddr, BYTE & Value ) = 0;
	virtual bool  LH_PAddr     ( DWORD PAddr, WORD & Value ) = 0; 
	virtual bool  LW_PAddr     ( DWORD PAddr, DWORD & Value ) = 0;
	virtual bool  LD_PAddr     ( DWORD PAddr, QWORD & Value ) = 0;

	virtual bool  SB_VAddr     ( DWORD VAddr, BYTE Value ) = 0;
	virtual bool  SH_VAddr     ( DWORD VAddr, WORD Value ) = 0;
	virtual bool  SW_VAddr     ( DWORD VAddr, DWORD Value ) = 0;
	virtual bool  SD_VAddr     ( DWORD VAddr, QWORD Value ) = 0;

	virtual bool  SB_PAddr     ( DWORD PAddr, BYTE Value ) = 0;
	virtual bool  SH_PAddr     ( DWORD PAddr, WORD Value ) = 0;
	virtual bool  SW_PAddr     ( DWORD PAddr, DWORD Value ) = 0;
	virtual bool  SD_PAddr     ( DWORD PAddr, QWORD Value ) = 0;

	virtual bool  ValidVaddr   ( DWORD VAddr ) const = 0;

	virtual int   MemoryFilter ( DWORD dwExptCode, void * lpExceptionPointer ) = 0;
	virtual void  UpdateFieldSerration ( unsigned int interlaced ) = 0;

	//Protect the Memory from being written to
	virtual void  ProtectMemory    ( DWORD StartVaddr, DWORD EndVaddr ) = 0;
	virtual void  UnProtectMemory  ( DWORD StartVaddr, DWORD EndVaddr ) = 0;

	//Compilation Functions
	virtual void ResetMemoryStack    () = 0;
	
	virtual void Compile_LB          () = 0;
	virtual void Compile_LBU         () = 0;
	virtual void Compile_LH          () = 0;
	virtual void Compile_LHU         () = 0;
	virtual void Compile_LW          () = 0;
	virtual void Compile_LL          () = 0;
	virtual void Compile_LWC1        () = 0;
	virtual void Compile_LWU         () = 0;
	virtual void Compile_LWL         () = 0;
	virtual void Compile_LWR         () = 0;
	virtual void Compile_LD          () = 0;
	virtual void Compile_LDC1        () = 0;
	virtual void Compile_LDL         () = 0;
	virtual void Compile_LDR         () = 0;
	virtual void Compile_SB          () = 0;
	virtual void Compile_SH          () = 0;
	virtual void Compile_SW          () = 0;
	virtual void Compile_SWL         () = 0;
	virtual void Compile_SWR         () = 0;
	virtual void Compile_SD          () = 0;
	virtual void Compile_SDL         () = 0;
	virtual void Compile_SDR         () = 0;
	virtual void Compile_SC          () = 0;
	virtual void Compile_SWC1        () = 0;
	virtual void Compile_SDC1        () = 0;
};
