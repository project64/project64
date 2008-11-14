/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
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
#include <Windows.h>
#include <stdio.h>
#include "main.h"
#include "debugger.h"
#include "CPU.h"

void OnFirstDMA (void) {
	switch (CicChip) {
	case 1: *(DWORD *)&N64MEM[0x318] = RdramSize; break;
	case 2: *(DWORD *)&N64MEM[0x318] = RdramSize; break;
	case 3: *(DWORD *)&N64MEM[0x318] = RdramSize; break;
	case 5: *(DWORD *)&N64MEM[0x3F0] = RdramSize; break;
	case 6: *(DWORD *)&N64MEM[0x318] = RdramSize; break;
	default: DisplayError("Unhandled CicChip(%d) in first DMA",CicChip);
	}
}

void PI_DMA_READ (void) {
//	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;

	if ( PI_DRAM_ADDR_REG + PI_RD_LEN_REG + 1 > RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("PI_DMA_READ not in Memory");
#endif
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts();
		return;
	}

	if ( PI_CART_ADDR_REG >= 0x08000000 && PI_CART_ADDR_REG <= 0x08010000) {
		if (SaveUsing == SaveChip_Auto) { SaveUsing = SaveChip_Sram; }
		if (SaveUsing == SaveChip_Sram) {
			DmaToSram(
				N64MEM+PI_DRAM_ADDR_REG,
				PI_CART_ADDR_REG - 0x08000000,
				PI_RD_LEN_REG + 1
			);
			PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			MI_INTR_REG |= MI_INTR_PI;
			CheckInterrupts();
			return;
		}
		if (SaveUsing == SaveChip_FlashRam) {
			DmaToFlashram(
				N64MEM+PI_DRAM_ADDR_REG,
				PI_CART_ADDR_REG - 0x08000000,
				PI_WR_LEN_REG + 1
			);
			PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			MI_INTR_REG |= MI_INTR_PI;
			CheckInterrupts();
			return;
		}
	}
	if (SaveUsing == SaveChip_FlashRam) {
#ifndef EXTERNAL_RELEASE
		DisplayError("**** FLashRam DMA Read address %X *****",PI_CART_ADDR_REG);
#endif
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts();
		return;
	}
#ifndef EXTERNAL_RELEASE
	DisplayError("PI_DMA_READ where are you dmaing to ?");
#endif	
	PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	MI_INTR_REG |= MI_INTR_PI;
	CheckInterrupts();
	return;
}

void PI_DMA_WRITE (void) {
	DWORD i;	

	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
	if ( PI_DRAM_ADDR_REG + PI_WR_LEN_REG + 1 > RdramSize) 
	{
		if (ShowUnhandledMemory) { DisplayError("PI_DMA_WRITE not in Memory"); }
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts();
		return;
	}

	if ( PI_CART_ADDR_REG >= 0x08000000 && PI_CART_ADDR_REG <= 0x08010000) {
		if (SaveUsing == SaveChip_Auto) { SaveUsing = SaveChip_Sram; }
		if (SaveUsing == SaveChip_Sram) {
			DmaFromSram(
				N64MEM+PI_DRAM_ADDR_REG,
				PI_CART_ADDR_REG - 0x08000000,
				PI_WR_LEN_REG + 1
			);
			PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			MI_INTR_REG |= MI_INTR_PI;
			CheckInterrupts();
			return;
		}
		if (SaveUsing == SaveChip_FlashRam) {
			DmaFromFlashram(
				N64MEM+PI_DRAM_ADDR_REG,
				PI_CART_ADDR_REG - 0x08000000,
				PI_WR_LEN_REG + 1
			);
			PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			MI_INTR_REG |= MI_INTR_PI;
			CheckInterrupts();
		}
		return;
	}

	if ( PI_CART_ADDR_REG >= 0x10000000 && PI_CART_ADDR_REG <= 0x1FBFFFFF) {
#ifdef ROM_IN_MAPSPACE
		if (WrittenToRom) { 
			DWORD OldProtect;
			VirtualProtect(ROM,RomFileSize,PAGE_READONLY, &OldProtect);
		}
#endif
		PI_CART_ADDR_REG -= 0x10000000;
		if (PI_CART_ADDR_REG + PI_WR_LEN_REG + 1 < RomFileSize) {
			for (i = 0; i < PI_WR_LEN_REG + 1; i ++) {
				*(N64MEM+((PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((PI_CART_ADDR_REG + i) ^ 3));
			}
		} else {
			DWORD Len;
			Len = RomFileSize - PI_CART_ADDR_REG;
			for (i = 0; i < Len; i ++) {
				*(N64MEM+((PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((PI_CART_ADDR_REG + i) ^ 3));
			}
			for (i = Len; i < PI_WR_LEN_REG + 1 - Len; i ++) {
				*(N64MEM+((PI_DRAM_ADDR_REG + i) ^ 3)) =  0;
			}
		}
		PI_CART_ADDR_REG += 0x10000000;

		if (!CPU_Action.DMAUsed) { 
			CPU_Action.DMAUsed = TRUE;
			OnFirstDMA(); 
		}
		if (g_Recompiler && g_Recompiler->bSMM_PIDMA())
		{
			g_Recompiler->ClearRecompCode_Phys(PI_DRAM_ADDR_REG, PI_WR_LEN_REG,CRecompiler::Remove_DMA);
		}
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts();
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9) + 50);
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9));
		return;
	}
	
	if (ShowUnhandledMemory) { DisplayError("PI_DMA_WRITE not in ROM"); }
	PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	MI_INTR_REG |= MI_INTR_PI;
	CheckInterrupts();

}

void SI_DMA_READ (void) {
	BYTE * PifRamPos = &PIF_Ram[0];
	
	if ((int)SI_DRAM_ADDR_REG > (int)RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SI DMA\nSI_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	PifRamRead();
	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	if ((int)SI_DRAM_ADDR_REG < 0) {
		int count, RdramPos;

		RdramPos = (int)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { continue; }
			N64MEM[RdramPos ^3] = PIF_Ram[count];
		}
	} else {
		_asm {
			mov edi, dword ptr [RegSI]
			mov edi, dword ptr [edi]
			add edi, N64MEM
			mov ecx, PifRamPos
			mov edx, 0		
	memcpyloop:
			mov eax, dword ptr [ecx + edx]
			bswap eax
			mov  dword ptr [edi + edx],eax
			mov eax, dword ptr [ecx + edx + 4]
			bswap eax
			mov  dword ptr [edi + edx + 4],eax
			mov eax, dword ptr [ecx + edx + 8]
			bswap eax
			mov  dword ptr [edi + edx + 8],eax
			mov eax, dword ptr [ecx + edx + 12]
			bswap eax
			mov  dword ptr [edi + edx + 12],eax
			add edx, 16
			cmp edx, 64
			jb memcpyloop
		}
	}
	
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.LogPRDMAMemStores) {
		int count;
		char HexData[100], AsciiData[100], Addon[20];
		LogMessage("\tData DMAed to RDRAM:");			
		LogMessage("\t--------------------");
		for (count = 0; count < 16; count ++ ) {
			if ((count % 4) == 0) { 
				sprintf(HexData,"\0"); 
				sprintf(AsciiData,"\0"); 
			}
 			sprintf(Addon,"%02X %02X %02X %02X", 
				PIF_Ram[(count << 2) + 0], PIF_Ram[(count << 2) + 1], 
				PIF_Ram[(count << 2) + 2], PIF_Ram[(count << 2) + 3] );
			strcat(HexData,Addon);
			if (((count + 1) % 4) != 0) {
				sprintf(Addon,"-");
				strcat(HexData,Addon);
			} 
			
			sprintf(Addon,"%c%c%c%c", 
				PIF_Ram[(count << 2) + 0], PIF_Ram[(count << 2) + 1], 
				PIF_Ram[(count << 2) + 2], PIF_Ram[(count << 2) + 3] );
			strcat(AsciiData,Addon);
			
			if (((count + 1) % 4) == 0) {
				LogMessage("\t%s %s",HexData, AsciiData);
			} 
		}
		LogMessage("");
	}
#endif

	if (DelaySI) {
		ChangeTimer(SiTimer,0x900);
	} else {
		MI_INTR_REG |= MI_INTR_SI;
		SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		CheckInterrupts();
	}
}

void SI_DMA_WRITE (void) {
	BYTE * PifRamPos = &PIF_Ram[0];
	
	if ((int)SI_DRAM_ADDR_REG > (int)RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SI DMA\nSI_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	if ((int)SI_DRAM_ADDR_REG < 0) {
		int count, RdramPos;

		RdramPos = (int)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { PIF_Ram[count] = 0; continue; }
			PIF_Ram[count] = N64MEM[RdramPos ^3];
		}
	} else {
		_asm {
			mov ecx, dword ptr [RegSI]
			mov ecx, dword ptr [ecx]
			add ecx, N64MEM
			mov edi, PifRamPos
			mov edx, 0		
	memcpyloop:
			mov eax, dword ptr [ecx + edx]
			bswap eax
			mov  dword ptr [edi + edx],eax
			mov eax, dword ptr [ecx + edx + 4]
			bswap eax
			mov  dword ptr [edi + edx + 4],eax
			mov eax, dword ptr [ecx + edx + 8]
			bswap eax
			mov  dword ptr [edi + edx + 8],eax
			mov eax, dword ptr [ecx + edx + 12]
			bswap eax
			mov  dword ptr [edi + edx + 12],eax
			add edx, 16
			cmp edx, 64
			jb memcpyloop
		}
	}
	
#if (!defined(EXTERNAL_RELEASE))
	if (LogOptions.LogPRDMAMemLoads) {
		int count;
		char HexData[100], AsciiData[100], Addon[20];
		LogMessage("");
		LogMessage("\tData DMAed to the Pif Ram:");			
		LogMessage("\t--------------------------");
		for (count = 0; count < 16; count ++ ) {
			if ((count % 4) == 0) { 
				sprintf(HexData,"\0"); 
				sprintf(AsciiData,"\0"); 
			}
			sprintf(Addon,"%02X %02X %02X %02X", 
				PIF_Ram[(count << 2) + 0], PIF_Ram[(count << 2) + 1], 
				PIF_Ram[(count << 2) + 2], PIF_Ram[(count << 2) + 3] );
			strcat(HexData,Addon);
			if (((count + 1) % 4) != 0) {
				sprintf(Addon,"-");
				strcat(HexData,Addon);
			} 
			
			sprintf(Addon,"%c%c%c%c", 
				PIF_Ram[(count << 2) + 0], PIF_Ram[(count << 2) + 1], 
				PIF_Ram[(count << 2) + 2], PIF_Ram[(count << 2) + 3] );
			strcat(AsciiData,Addon);
			
			if (((count + 1) % 4) == 0) {
				LogMessage("\t%s %s",HexData, AsciiData);
			} 
		}
		LogMessage("");
	}
#endif

	PifRamWrite();
	
	if (DelaySI) {
		ChangeTimer(SiTimer,0x900);
	} else {
		MI_INTR_REG |= MI_INTR_SI;
		SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		CheckInterrupts();
	}
}

void SP_DMA_READ (void) { 
	SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

	if (SP_DRAM_ADDR_REG > RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		SP_DMA_BUSY_REG = 0;
		SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
		return;
	}
	
	if (SP_RD_LEN_REG + 1  + (SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA\ncould not fit copy in memory segement");
#endif
		return;		
	}
	
	if ((SP_MEM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }
	if ((SP_DRAM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }
	if (((SP_RD_LEN_REG + 1) & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }

	memcpy( DMEM + (SP_MEM_ADDR_REG & 0x1FFF), N64MEM + SP_DRAM_ADDR_REG,
		SP_RD_LEN_REG + 1 );
		
	SP_DMA_BUSY_REG = 0;
	SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void SP_DMA_WRITE (void) { 
	if (SP_DRAM_ADDR_REG > RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA WRITE\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	if (SP_WR_LEN_REG + 1 + (SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA WRITE\ncould not fit copy in memory segement");
#endif
		return;		
	}

	if ((SP_MEM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__); }
	if ((SP_DRAM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }
	if (((SP_WR_LEN_REG + 1) & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }

	memcpy( N64MEM + SP_DRAM_ADDR_REG, DMEM + (SP_MEM_ADDR_REG & 0x1FFF),
		SP_WR_LEN_REG + 1);
		
	SP_DMA_BUSY_REG = 0;
	SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}
