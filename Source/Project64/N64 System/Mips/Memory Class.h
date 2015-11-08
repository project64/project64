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

__interface CMipsMemory_CallBack
{
    //Protected memory has been written to, returns true if that memory has been unprotected
    virtual bool WriteToProtectedMemory (uint32_t Address, int32_t length) = 0;
};

__interface CMipsMemory
{
    virtual uint8_t * Rdram    () = 0;
    virtual uint32_t  RdramSize() = 0;
    virtual uint8_t * Dmem     () = 0;
    virtual uint8_t * Imem     () = 0;
    virtual uint8_t * PifRam   () = 0;

    virtual bool  LB_VAddr     ( uint32_t VAddr, uint8_t & Value ) = 0;
    virtual bool  LH_VAddr     ( uint32_t VAddr, uint16_t & Value ) = 0;
    virtual bool  LW_VAddr     ( uint32_t VAddr, uint32_t & Value ) = 0;
    virtual bool  LD_VAddr     ( uint32_t VAddr, uint64_t & Value ) = 0;

    virtual bool  LB_PAddr     ( uint32_t PAddr, uint8_t & Value ) = 0;
    virtual bool  LH_PAddr     ( uint32_t PAddr, uint16_t & Value ) = 0;
    virtual bool  LW_PAddr     ( uint32_t PAddr, uint32_t & Value ) = 0;
    virtual bool  LD_PAddr     ( uint32_t PAddr, uint64_t & Value ) = 0;

    virtual bool  SB_VAddr     ( uint32_t VAddr, uint8_t Value ) = 0;
    virtual bool  SH_VAddr     ( uint32_t VAddr, uint16_t Value ) = 0;
    virtual bool  SW_VAddr     ( uint32_t VAddr, uint32_t Value ) = 0;
    virtual bool  SD_VAddr     ( uint32_t VAddr, uint64_t Value ) = 0;

    virtual bool  SB_PAddr     ( uint32_t PAddr, uint8_t Value ) = 0;
    virtual bool  SH_PAddr     ( uint32_t PAddr, uint16_t Value ) = 0;
    virtual bool  SW_PAddr     ( uint32_t PAddr, uint32_t Value ) = 0;
    virtual bool  SD_PAddr     ( uint32_t PAddr, uint64_t Value ) = 0;

    virtual bool  ValidVaddr   ( uint32_t VAddr ) const = 0;

    virtual int32_t   MemoryFilter ( uint32_t dwExptCode, void * lpExceptionPointer ) = 0;
    virtual void  UpdateFieldSerration ( uint32_t interlaced ) = 0;

    //Protect the Memory from being written to
    virtual void  ProtectMemory    ( uint32_t StartVaddr, uint32_t EndVaddr ) = 0;
    virtual void  UnProtectMemory  ( uint32_t StartVaddr, uint32_t EndVaddr ) = 0;

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
