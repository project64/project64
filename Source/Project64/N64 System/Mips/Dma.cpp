#include "stdafx.h"

CDMA::CDMA(CFlashram & FlashRam, CSram & Sram) :
	m_FlashRam(FlashRam),
	m_Sram(Sram)
{
}

void CDMA::OnFirstDMA (void) {
	switch (_Rom->CicChipID()) {
	case 1: *(DWORD *)&((g_MMU->Rdram())[0x318]) = g_MMU->RdramSize(); break;
	case 2: *(DWORD *)&((g_MMU->Rdram())[0x318]) = g_MMU->RdramSize(); break;
	case 3: *(DWORD *)&((g_MMU->Rdram())[0x318]) = g_MMU->RdramSize(); break;
	case 5: *(DWORD *)&((g_MMU->Rdram())[0x3F0]) = g_MMU->RdramSize(); break;
	case 6: *(DWORD *)&((g_MMU->Rdram())[0x318]) = g_MMU->RdramSize(); break;
	default: g_Notify->DisplayError("Unhandled CicChip(%d) in first DMA",_Rom->CicChipID());
	}
}

void CDMA::PI_DMA_READ (void) {
//	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;

	if ( _Reg->PI_DRAM_ADDR_REG + _Reg->PI_RD_LEN_REG + 1 > g_MMU->RdramSize()) {
#ifndef EXTERNAL_RELEASE
		g_Notify->DisplayError("PI_DMA_READ not in Memory");
#endif
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x08000000 && _Reg->PI_CART_ADDR_REG <= 0x08010000) {
		if (g_System->m_SaveUsing == SaveChip_Auto) { g_System->m_SaveUsing = SaveChip_Sram; }
		if (g_System->m_SaveUsing == SaveChip_Sram) {
			m_Sram.DmaToSram(
				g_MMU->Rdram() + _Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_RD_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
		if (g_System->m_SaveUsing == SaveChip_FlashRam) {
			m_FlashRam.DmaToFlashram(
				g_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
	}
	if (g_System->m_SaveUsing == SaveChip_FlashRam) 
	{
		g_Notify->DisplayError("**** FLashRam DMA Read address %X *****",_Reg->PI_CART_ADDR_REG);
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}
#ifndef EXTERNAL_RELEASE
	g_Notify->DisplayError("PI_DMA_READ where are you dmaing to ?");
#endif	
	_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	_Reg->MI_INTR_REG |= MI_INTR_PI;
	_Reg->CheckInterrupts();
	return;
}

void CDMA::PI_DMA_WRITE (void) {

	_Reg->PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
	if ( _Reg->PI_DRAM_ADDR_REG + _Reg->PI_WR_LEN_REG + 1 > g_MMU->RdramSize()) 
	{
		if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { g_Notify->DisplayError("PI_DMA_WRITE not in Memory"); }
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x08000000 && _Reg->PI_CART_ADDR_REG <= 0x08010000) {
		if (g_System->m_SaveUsing == SaveChip_Auto) { g_System->m_SaveUsing = SaveChip_Sram; }
		if (g_System->m_SaveUsing == SaveChip_Sram) {
			m_Sram.DmaFromSram(
				g_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
		if (g_System->m_SaveUsing == SaveChip_FlashRam) {
			m_FlashRam.DmaFromFlashram(
				g_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
		}
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x10000000 && _Reg->PI_CART_ADDR_REG <= 0x1FBFFFFF) 
	{
	DWORD i;	
#ifdef tofix
#ifdef ROM_IN_MAPSPACE
		if (WrittenToRom) { 
			DWORD OldProtect;
			VirtualProtect(ROM,m_RomFileSize,PAGE_READONLY, &OldProtect);
		}
#endif
#endif
		BYTE * ROM   = _Rom->GetRomAddress();
		BYTE * RDRAM = g_MMU->Rdram();
		_Reg->PI_CART_ADDR_REG -= 0x10000000;
		if (_Reg->PI_CART_ADDR_REG + _Reg->PI_WR_LEN_REG + 1 < _Rom->GetRomSize()) {
			for (i = 0; i < _Reg->PI_WR_LEN_REG + 1; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((_Reg->PI_CART_ADDR_REG + i) ^ 3));
			}
		} else {
			DWORD Len;
			Len = _Rom->GetRomSize() - _Reg->PI_CART_ADDR_REG;
			for (i = 0; i < Len; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((_Reg->PI_CART_ADDR_REG + i) ^ 3));
			}
			for (i = Len; i < _Reg->PI_WR_LEN_REG + 1 - Len; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  0;
			}
		}
		_Reg->PI_CART_ADDR_REG += 0x10000000;

		if (!g_System->DmaUsed())
		{
			g_System->SetDmaUsed(true);
			OnFirstDMA(); 
		}
		if (g_Recompiler && g_Recompiler->bSMM_PIDMA())
		{
			g_Recompiler->ClearRecompCode_Phys(_Reg->PI_DRAM_ADDR_REG, _Reg->PI_WR_LEN_REG,CRecompiler::Remove_DMA);
		}
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9) + 50);
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9));
		return;
	}
	
	if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { g_Notify->DisplayError("PI_DMA_WRITE not in ROM"); }
	_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	_Reg->MI_INTR_REG |= MI_INTR_PI;
	_Reg->CheckInterrupts();

}

void CDMA::SP_DMA_READ (void) { 
	_Reg->SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

	if (_Reg->SP_DRAM_ADDR_REG > g_MMU->RdramSize()) {
#ifndef EXTERNAL_RELEASE
		g_Notify->DisplayError("SP DMA\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		_Reg->SP_DMA_BUSY_REG = 0;
		_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
		return;
	}
	
	if (_Reg->SP_RD_LEN_REG + 1  + (_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		g_Notify->DisplayError("SP DMA\ncould not fit copy in memory segement");
#endif
		return;		
	}
	
	if ((_Reg->SP_MEM_ADDR_REG & 3) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__);  }
	if ((_Reg->SP_DRAM_ADDR_REG & 3) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__);  }
	if (((_Reg->SP_RD_LEN_REG + 1) & 3) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__);  }

	memcpy( g_MMU->Dmem() + (_Reg->SP_MEM_ADDR_REG & 0x1FFF), g_MMU->Rdram() + _Reg->SP_DRAM_ADDR_REG,
		_Reg->SP_RD_LEN_REG + 1 );
		
	_Reg->SP_DMA_BUSY_REG = 0;
	_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void CDMA::SP_DMA_WRITE (void) { 
	if (_Reg->SP_DRAM_ADDR_REG > g_MMU->RdramSize()) {
#ifndef EXTERNAL_RELEASE
		g_Notify->DisplayError("SP DMA WRITE\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	if (_Reg->SP_WR_LEN_REG + 1 + (_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		g_Notify->DisplayError("SP DMA WRITE\ncould not fit copy in memory segement");
#endif
		return;		
	}

	if ((_Reg->SP_MEM_ADDR_REG & 3) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__); }
	if ((_Reg->SP_DRAM_ADDR_REG & 3) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__);  }
	if (((_Reg->SP_WR_LEN_REG + 1) & 3) != 0) { g_Notify->BreakPoint(__FILE__,__LINE__);  }

	memcpy( g_MMU->Rdram() + _Reg->SP_DRAM_ADDR_REG, g_MMU->Dmem() + (_Reg->SP_MEM_ADDR_REG & 0x1FFF),
		_Reg->SP_WR_LEN_REG + 1);
		
	_Reg->SP_DMA_BUSY_REG = 0;
	_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

