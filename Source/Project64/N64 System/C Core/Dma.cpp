#include "stdafx.h"
#include "Sram.h"
#include "Flashram.h"

void OnFirstDMA (void) {
	switch (g_CicChip) {
	case 1: *(DWORD *)&((_MMU->Rdram())[0x318]) = g_RdramSize; break;
	case 2: *(DWORD *)&((_MMU->Rdram())[0x318]) = g_RdramSize; break;
	case 3: *(DWORD *)&((_MMU->Rdram())[0x318]) = g_RdramSize; break;
	case 5: *(DWORD *)&((_MMU->Rdram())[0x3F0]) = g_RdramSize; break;
	case 6: *(DWORD *)&((_MMU->Rdram())[0x318]) = g_RdramSize; break;
	default: DisplayError("Unhandled g_CicChip(%d) in first DMA",g_CicChip);
	}
}

void PI_DMA_READ (void) {
//	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;

	if ( _Reg->PI_DRAM_ADDR_REG + _Reg->PI_RD_LEN_REG + 1 > g_RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("PI_DMA_READ not in Memory");
#endif
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x08000000 && _Reg->PI_CART_ADDR_REG <= 0x08010000) {
		if (g_SaveUsing == SaveChip_Auto) { g_SaveUsing = SaveChip_Sram; }
		if (g_SaveUsing == SaveChip_Sram) {
			DmaToSram(
				_MMU->Rdram() + _Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_RD_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
		if (g_SaveUsing == SaveChip_FlashRam) {
			DmaToFlashram(
				_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
	}
	if (g_SaveUsing == SaveChip_FlashRam) {
#ifndef EXTERNAL_RELEASE
		DisplayError("**** FLashRam DMA Read address %X *****",_Reg->PI_CART_ADDR_REG);
#endif
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}
#ifndef EXTERNAL_RELEASE
	DisplayError("PI_DMA_READ where are you dmaing to ?");
#endif	
	_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	_Reg->MI_INTR_REG |= MI_INTR_PI;
	_Reg->CheckInterrupts();
	return;
}

void PI_DMA_WRITE (void) {

	_Reg->PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
	if ( _Reg->PI_DRAM_ADDR_REG + _Reg->PI_WR_LEN_REG + 1 > g_RdramSize) 
	{
		if (g_ShowUnhandledMemory) { DisplayError("PI_DMA_WRITE not in Memory"); }
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x08000000 && _Reg->PI_CART_ADDR_REG <= 0x08010000) {
		if (g_SaveUsing == SaveChip_Auto) { g_SaveUsing = SaveChip_Sram; }
		if (g_SaveUsing == SaveChip_Sram) {
			DmaFromSram(
				_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
		if (g_SaveUsing == SaveChip_FlashRam) {
			DmaFromFlashram(
				_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
		}
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x10000000 && _Reg->PI_CART_ADDR_REG <= 0x1FBFFFFF) {
	DWORD i;	
#ifdef tofix
#ifdef ROM_IN_MAPSPACE
		if (WrittenToRom) { 
			DWORD OldProtect;
			VirtualProtect(ROM,g_RomFileSize,PAGE_READONLY, &OldProtect);
		}
#endif
#endif
		BYTE * ROM   = _Rom->GetRomAddress();
		BYTE * RDRAM = _MMU->Rdram();
		_Reg->PI_CART_ADDR_REG -= 0x10000000;
		if (_Reg->PI_CART_ADDR_REG + _Reg->PI_WR_LEN_REG + 1 < g_RomFileSize) {
			for (i = 0; i < _Reg->PI_WR_LEN_REG + 1; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((_Reg->PI_CART_ADDR_REG + i) ^ 3));
			}
		} else {
			DWORD Len;
			Len = g_RomFileSize - _Reg->PI_CART_ADDR_REG;
			for (i = 0; i < Len; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((_Reg->PI_CART_ADDR_REG + i) ^ 3));
			}
			for (i = Len; i < _Reg->PI_WR_LEN_REG + 1 - Len; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  0;
			}
		}
		_Reg->PI_CART_ADDR_REG += 0x10000000;

		if (!_N64System->DmaUsed())
		{
			_N64System->SetDmaUsed(true);
			OnFirstDMA(); 
		}
		if (_Recompiler && _Recompiler->bSMM_PIDMA())
		{
			_Recompiler->ClearRecompCode_Phys(_Reg->PI_DRAM_ADDR_REG, _Reg->PI_WR_LEN_REG,CRecompiler::Remove_DMA);
		}
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9) + 50);
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9));
		return;
	}
	
	if (g_ShowUnhandledMemory) { DisplayError("PI_DMA_WRITE not in ROM"); }
	_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	_Reg->MI_INTR_REG |= MI_INTR_PI;
	_Reg->CheckInterrupts();

}

void SI_DMA_READ (void) {
	BYTE * PIF_Ram = _MMU->PifRam();
	BYTE * PifRamPos = _MMU->PifRam();
	BYTE * RDRAM = _MMU->Rdram();
	
	DWORD & SI_DRAM_ADDR_REG = _Reg->SI_DRAM_ADDR_REG;
	if ((int)SI_DRAM_ADDR_REG > (int)g_RdramSize) {
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
			RDRAM[RdramPos ^3] = PIF_Ram[count];
		}
	} else {
		_asm {
			mov edi, dword ptr [SI_DRAM_ADDR_REG]
			mov edi, dword ptr [edi]
			add edi, RDRAM
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

	if (g_DelaySI) {
		_SystemTimer->SetTimer(CSystemTimer::SiTimer,0x900,false);
	} else {
		_Reg->MI_INTR_REG |= MI_INTR_SI;
		_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		_Reg->CheckInterrupts();
	}
}

void SI_DMA_WRITE (void) {
	BYTE * PIF_Ram = _MMU->PifRam();
	BYTE * PifRamPos = PIF_Ram;
	
	DWORD & SI_DRAM_ADDR_REG = _Reg->SI_DRAM_ADDR_REG;
	if ((int)SI_DRAM_ADDR_REG > (int)g_RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SI DMA\nSI_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	BYTE * RDRAM = _MMU->Rdram();

	if ((int)SI_DRAM_ADDR_REG < 0) {
		int count, RdramPos;

		RdramPos = (int)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { PIF_Ram[count] = 0; continue; }
			PIF_Ram[count] = RDRAM[RdramPos ^3];
		}
	} else {
		_asm {
			mov ecx, dword ptr [SI_DRAM_ADDR_REG]
			mov ecx, dword ptr [ecx]
			add ecx, RDRAM
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
	
	if (g_DelaySI) {
		_SystemTimer->SetTimer(CSystemTimer::SiTimer,0x900,false);
	} else {
		_Reg->MI_INTR_REG |= MI_INTR_SI;
		_Reg->SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		_Reg->CheckInterrupts();
	}
}

void SP_DMA_READ (void) { 
	_Reg->SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

	if (_Reg->SP_DRAM_ADDR_REG > g_RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		_Reg->SP_DMA_BUSY_REG = 0;
		_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
		return;
	}
	
	if (_Reg->SP_RD_LEN_REG + 1  + (_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA\ncould not fit copy in memory segement");
#endif
		return;		
	}
	
	if ((_Reg->SP_MEM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }
	if ((_Reg->SP_DRAM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }
	if (((_Reg->SP_RD_LEN_REG + 1) & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }

	memcpy( _MMU->Dmem() + (_Reg->SP_MEM_ADDR_REG & 0x1FFF), _MMU->Rdram() + _Reg->SP_DRAM_ADDR_REG,
		_Reg->SP_RD_LEN_REG + 1 );
		
	_Reg->SP_DMA_BUSY_REG = 0;
	_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void SP_DMA_WRITE (void) { 
	if (_Reg->SP_DRAM_ADDR_REG > g_RdramSize) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA WRITE\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	if (_Reg->SP_WR_LEN_REG + 1 + (_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		DisplayError("SP DMA WRITE\ncould not fit copy in memory segement");
#endif
		return;		
	}

	if ((_Reg->SP_MEM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__); }
	if ((_Reg->SP_DRAM_ADDR_REG & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }
	if (((_Reg->SP_WR_LEN_REG + 1) & 3) != 0) { BreakPoint(__FILE__,__LINE__);  }

	memcpy( _MMU->Rdram() + _Reg->SP_DRAM_ADDR_REG, _MMU->Dmem() + (_Reg->SP_MEM_ADDR_REG & 0x1FFF),
		_Reg->SP_WR_LEN_REG + 1);
		
	_Reg->SP_DMA_BUSY_REG = 0;
	_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

